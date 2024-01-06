// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#include "TolgeeLocalizationSubsystem.h"

#include <Async/Async.h>
#include <Dom/JsonObject.h>
#include <Engine/World.h>
#include <HttpModule.h>
#include <Interfaces/IHttpResponse.h>
#include <Internationalization/TextLocalizationResource.h>
#include <JsonObjectConverter.h>
#include <Misc/FileHelper.h>
#include <Serialization/JsonReader.h>
#include <Serialization/JsonSerializer.h>
#include <TimerManager.h>
#include <Misc/EngineVersionComparison.h>

#include "TolgeeLog.h"
#include "TolgeeRuntimeRequestData.h"
#include "TolgeeSettings.h"
#include "TolgeeTextSource.h"
#include "TolgeeUtils.h"

void UTolgeeLocalizationSubsystem::ManualFetch()
{
	UE_LOG(LogTolgee, Log, TEXT("UTolgeeLocalizationSubsystem::ManualFetch"));

	FetchTranslation();
}

const FLocalizedDictionary& UTolgeeLocalizationSubsystem::GetLocalizedDictionary() const
{
	return LocalizedDictionary;
}

void UTolgeeLocalizationSubsystem::GetLocalizedResources(
	const ELocalizationLoadFlags InLoadFlags, TArrayView<const FString> InPrioritizedCultures, FTextLocalizationResource& InOutNativeResource, FTextLocalizationResource& InOutLocalizedResource
) const
{
	for (const FLocalizedKey& KeyData : LocalizedDictionary.Keys)
	{
		if (!InPrioritizedCultures.Contains(KeyData.Locale))
		{
			continue;
		}

		const FTextKey InNamespace = KeyData.Namespace;
		const FTextKey InKey = KeyData.Name;
		const uint32 InKeyHash = KeyData.Hash;
		const FString InLocalizedString = KeyData.Translation;

		if (InKeyHash)
		{
			InOutLocalizedResource.AddEntry(InNamespace, InKey, InKeyHash, InLocalizedString, 0);
		}
		else
		{
			UE_LOG(LogTolgee, Error, TEXT("KeyData {Namespace: %s Key: %s} has invalid hash."), InNamespace.GetChars(), InKey.GetChars());
		}
	}
}

void UTolgeeLocalizationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogTolgee, Log, TEXT("UTolgeeLocalizationSubsystem::Initialize"));

	Super::Initialize(Collection);

	// OnGameInstanceStart was introduced in 4.27, so we are trying to mimic the behavior for older version
#if UE_VERSION_OLDER_THAN(4, 27, 0)
	UGameViewportClient::OnViewportCreated().AddWeakLambda(this, [this]()
	{
		const UGameViewportClient* ViewportClient = GEngine ? GEngine->GameViewport : nullptr;
		UGameInstance* GameInstance = ViewportClient ? ViewportClient->GetGameInstance() : nullptr;
		if(GameInstance)
		{
			UE_LOG(LogTolgee, Log, TEXT("Workaround for OnGameInstanceStart"));
			OnGameInstanceStart(GameInstance);
		}
		else
		{
			UE_LOG(LogTolgee, Error, TEXT("Workaround for OnGameInstanceStart failed"));
		}
	});
#else
	FWorldDelegates::OnStartGameInstance.AddUObject(this, &ThisClass::OnGameInstanceStart);
#endif

	TextSource = MakeShared<FTolgeeTextSource>();
	TextSource->GetLocalizedResources.BindUObject(this, &ThisClass::GetLocalizedResources);
	FTextLocalizationManager::Get().RegisterTextSource(TextSource.ToSharedRef());
}

void UTolgeeLocalizationSubsystem::OnGameInstanceStart(UGameInstance* GameInstance)
{
	const UTolgeeSettings* Settings = GetDefault<UTolgeeSettings>();
	if (Settings->bLiveTranslationUpdates)
	{
		FetchTranslation();
		GameInstance->GetTimerManager().SetTimer(AutoFetchTimerHandle, this, &ThisClass::FetchTranslation, Settings->UpdateInterval, true);
	}
	else
	{
		LoadLocalData();
	}
}

void UTolgeeLocalizationSubsystem::FetchTranslation()
{
	UE_LOG(LogTolgee, Verbose, TEXT("UTolgeeLocalizationSubsystem::FetchTranslation"));

	const UTolgeeSettings* Settings = GetDefault<UTolgeeSettings>();
	if (!Settings->IsReadyToSendRequests())
	{
		UE_LOG(LogTolgee, Warning, TEXT("Settings are not set up properly. Fetch request will be skipped."));
		return;
	}

	if (bFetchInProgress)
	{
		UE_LOG(LogTolgee, Log, TEXT("Fetch skipped, an update is already in progress."));
		return;
	}

	FOnTranslationFetched OnDoneCallback = FOnTranslationFetched::CreateUObject(this, &UTolgeeLocalizationSubsystem::OnAllTranslationsFetched);
	bFetchInProgress = true;

	FetchNextTranslation(MoveTemp(OnDoneCallback), {}, {});
}

void UTolgeeLocalizationSubsystem::FetchNextTranslation(FOnTranslationFetched Callback, TArray<FTolgeeKeyData> CurrentTranslations, const FString& NextCursor)
{
	const UTolgeeSettings* Settings = GetDefault<UTolgeeSettings>();

	TArray<FString> QueryParameters;
	if (!NextCursor.IsEmpty())
	{
		QueryParameters.Emplace(FString::Printf(TEXT("cursor=%s"), *NextCursor));
	}

	for (const FString& Language : Settings->Languages)
	{
		QueryParameters.Emplace(FString::Printf(TEXT("languages=%s"), *Language));
	}

	const FString EndpointUrl = TolgeeUtils::GetUrlEndpoint(TEXT("v2/projects/translations"));
	const FString RequestUrl = TolgeeUtils::AppendQueryParameters(EndpointUrl, QueryParameters);
	const FHttpRequestRef HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("GET");
	HttpRequest->SetHeader(TEXT("X-API-Key"), Settings->ApiKey);
	HttpRequest->SetHeader(TEXT("X-Tolgee-SDK-Type"), TolgeeUtils::GetSdkType());
	HttpRequest->SetHeader(TEXT("X-Tolgee-SDK-Version"), TolgeeUtils::GetSdkVersion());
	HttpRequest->SetURL(RequestUrl);
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnNextTranslationFetched, Callback, CurrentTranslations);
	HttpRequest->ProcessRequest();
}

void UTolgeeLocalizationSubsystem::OnNextTranslationFetched(
	FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FOnTranslationFetched Callback, TArray<FTolgeeKeyData> CurrentTranslations
)
{
	UE_LOG(LogTolgee, Verbose, TEXT("UTolgeeLocalizationSubsystem::OnNextTranslationFetched"));

	if (!bWasSuccessful)
	{
		bFetchInProgress = false;
		UE_LOG(LogTolgee, Error, TEXT("Request to fetch translations was unsuccessful."));
		return;
	}

	if (!EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		bFetchInProgress = false;
		UE_LOG(LogTolgee, Error, TEXT("Request to fetch translation received unexpected code: %s"), *LexToString(Response->GetResponseCode()));
		return;
	}

	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	const TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Response->GetContentAsString());
	if (!FJsonSerializer::Deserialize(JsonReader, JsonObject))
	{
		bFetchInProgress = false;
		UE_LOG(LogTolgee, Error, TEXT("Could not deserialize response: %s"), *LexToString(Response->GetContentAsString()));
		return;
	}

	const FString NextCursor = [JsonObject]()
	{
		FString OutString;
		JsonObject->TryGetStringField(TEXT("nextCursor"), OutString);
		return OutString;
	}();
	const TSharedPtr<FJsonObject>* EmbeddedData = [JsonObject]()
	{
		const TSharedPtr<FJsonObject>* OutObject = nullptr;
		JsonObject->TryGetObjectField(TEXT("_embedded"), OutObject);
		return OutObject;
	}();
	const TArray<TSharedPtr<FJsonValue>> Keys = EmbeddedData ? (*EmbeddedData)->GetArrayField(TEXT("keys")) : TArray<TSharedPtr<FJsonValue>>();

	for (const TSharedPtr<FJsonValue>& Key : Keys)
	{
		const TOptional<FTolgeeKeyData> KeyData = FTolgeeKeyData::FromJson(Key);
		if (KeyData.IsSet())
		{
			CurrentTranslations.Add(KeyData.GetValue());
		}
	}

	const bool bAllKeysFetched = NextCursor.IsEmpty();
	if (bAllKeysFetched)
	{
		Callback.Execute(CurrentTranslations);
	}
	else
	{
		FetchNextTranslation(Callback, CurrentTranslations, NextCursor);
	}
}

void UTolgeeLocalizationSubsystem::OnAllTranslationsFetched(TArray<FTolgeeKeyData> Translations)
{
	LocalizedDictionary.Keys.Empty();

	for (const FTolgeeKeyData& Translation : Translations)
	{
		for (const TTuple<FString, FTolgeeTranslation>& Language : Translation.Translations)
		{
			FLocalizedKey& Key = LocalizedDictionary.Keys.Emplace_GetRef();
			Key.Name = Translation.KeyName;
			Key.Namespace = Translation.KeyNamespace;
			Key.Hash = Translation.GetKeyHash();
			Key.Locale = Language.Key;
			Key.Translation = Language.Value.Text;
			Key.RemoteId = Translation.KeyId;
		}
	}

	bFetchInProgress = false;

	RefreshTranslationData();
}

void UTolgeeLocalizationSubsystem::RefreshTranslationData()
{
	UE_LOG(LogTolgee, Verbose, TEXT("UTolgeeLocalizationSubsystem::RefreshTranslationData"));

	AsyncTask(
		ENamedThreads::AnyHiPriThreadHiPriTask,
		[=]()
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(Tolgee::LocalizationManager::RefreshResources)

			UE_LOG(LogTolgee, Verbose, TEXT("Tolgee::LocalizationManager::RefreshResources"));

			FTextLocalizationManager::Get().RefreshResources();
		}
	);
}

void UTolgeeLocalizationSubsystem::LoadLocalData()
{
	const FString Filename = TolgeeUtils::GetLocalizationSourceFile();
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *Filename))
	{
		UE_LOG(LogTolgee, Error, TEXT("Couldn't load local data from file: %s"), *Filename);
		return;
	}

	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTolgee, Error, TEXT("Couldn't deserialize local data from file: %s"), *Filename);
		return;
	}

	if (!FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), &LocalizedDictionary))
	{
		UE_LOG(LogTolgee, Error, TEXT("Couldn't convert local data to struct from file: %s"), *Filename);
		return;
	}

	UE_LOG(LogTolgee, Log, TEXT("Local data loaded successfully from file: %s"), *Filename);

	RefreshTranslationData();
}
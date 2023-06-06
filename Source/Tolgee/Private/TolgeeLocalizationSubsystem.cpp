// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#include "TolgeeLocalizationSubsystem.h"

#include <Async/Async.h>
#include <Dom/JsonObject.h>
#include <Engine/World.h>
#include <HttpModule.h>
#include <Interfaces/IHttpResponse.h>
#include <Internationalization/TextLocalizationResource.h>
#include <Serialization/JsonReader.h>
#include <Serialization/JsonSerializer.h>
#include <TimerManager.h>

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

TArray<FTolgeeKeyData> UTolgeeLocalizationSubsystem::GetLastFetchedKeys() const
{
	return TranslatedKeys;
}

void UTolgeeLocalizationSubsystem::GetLocalizedResources(
	const ELocalizationLoadFlags InLoadFlags, TArrayView<const FString> InPrioritizedCultures, FTextLocalizationResource& InOutNativeResource, FTextLocalizationResource& InOutLocalizedResource
) const
{
	for (const FTolgeeKeyData& KeyData : TranslatedKeys)
	{
		TOptional<FTolgeeTranslation> Translation = KeyData.GetFirstTranslation(InPrioritizedCultures);
		if (!Translation.IsSet())
		{
			// No valid translation was found for the desired locals
			continue;
		}

		const FTextKey InNamespace = KeyData.KeyNamespace;
		const FTextKey InKey = KeyData.KeyName;
		const uint32 InKeyHash = KeyData.GetKeyHash();
		const FString InLocalizedString = Translation->Text;

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

	FWorldDelegates::OnStartGameInstance.AddUObject(this, &ThisClass::OnGameInstanceStart);

	TextSource = MakeShared<FTolgeeTextSource>();
	TextSource->GetLocalizedResources.BindUObject(this, &ThisClass::GetLocalizedResources);
	FTextLocalizationManager::Get().RegisterTextSource(TextSource.ToSharedRef());

	ManualFetch();
}

void UTolgeeLocalizationSubsystem::OnGameInstanceStart(UGameInstance* GameInstance)
{
	FetchTranslation();

	const UTolgeeSettings* Settings = GetDefault<UTolgeeSettings>();
	if (Settings->bLiveTranslationUpdates)
	{
		GameInstance->GetTimerManager().SetTimer(AutoFetchTimerHandle, this, &ThisClass::FetchTranslation, Settings->UpdateInterval, true);
	}
}

void UTolgeeLocalizationSubsystem::FetchTranslation()
{
	UE_LOG(LogTolgee, Verbose, TEXT("UTolgeeLocalizationSubsystem::FetchTranslation"));

	if (bFetchInProgress)
	{
		UE_LOG(LogTolgee, Verbose, TEXT("Fetch skipped, an update is already in progress."));
		return;
	}

	FOnTranslationFetched OnDoneCallback = FOnTranslationFetched::CreateUObject(this, &UTolgeeLocalizationSubsystem::RefreshTranslationData);
	bFetchInProgress = true;

	// TODO: Should we feed something into the languages?
	FetchNextTranslation(MoveTemp(OnDoneCallback), {}, {}, {});
}

void UTolgeeLocalizationSubsystem::FetchNextTranslation(FOnTranslationFetched Callback, TArray<FString> Languages, TArray<FTolgeeKeyData> CurrentTranslations, const FString& NextCursor)
{
	TArray<FString> QueryParameters;
	if (!NextCursor.IsEmpty())
	{
		QueryParameters.Emplace(FString::Printf(TEXT("cursor=%s"), *NextCursor));
	}

	for (const FString& Language : Languages)
	{
		QueryParameters.Emplace(FString::Printf(TEXT("languages=%s"), *Language));
	}

	const FString EndpointUrl = TolgeeUtils::GetUrlEndpoint(TEXT("v2/projects/translations"));
	const FString RequestUrl = TolgeeUtils::AppendQueryParameters(EndpointUrl, QueryParameters);
	const TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("GET");
	HttpRequest->SetHeader(TEXT("X-API-Key"), GetDefault<UTolgeeSettings>()->ApiKey);
	HttpRequest->SetURL(RequestUrl);
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnNextTranslationFetched, Callback, Languages, CurrentTranslations);
	HttpRequest->ProcessRequest();
}

void UTolgeeLocalizationSubsystem::OnNextTranslationFetched(
	FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FOnTranslationFetched Callback, TArray<FString> Languages, TArray<FTolgeeKeyData> CurrentTranslations
)
{
	UE_LOG(LogTolgee, Verbose, TEXT("UTolgeeLocalizationSubsystem::OnNextTranslationFetched"));

	if (!bWasSuccessful)
	{
		UE_LOG(LogTolgee, Error, TEXT("Request to fetch translations was unsuccessful."));
		return;
	}

	if (!EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		UE_LOG(LogTolgee, Error, TEXT("Request to fetch translation received unexpected code: %s"), *LexToString(Response->GetResponseCode()));
		return;
	}

	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	const TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Response->GetContentAsString());
	if (!FJsonSerializer::Deserialize(JsonReader, JsonObject))
	{
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
		FetchNextTranslation(Callback, Languages, CurrentTranslations, NextCursor);
	}
}

void UTolgeeLocalizationSubsystem::RefreshTranslationData(TArray<FTolgeeKeyData> Translations)
{
	UE_LOG(LogTolgee, Verbose, TEXT("UTolgeeLocalizationSubsystem::RefreshTranslationData"));

	TranslatedKeys = Translations;

	AsyncTask(
		ENamedThreads::AnyHiPriThreadHiPriTask,
		[=]()
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(Tolgee::LocalizationManager::RefreshResources)

			UE_LOG(LogTolgee, Verbose, TEXT("Tolgee::LocalizationManager::RefreshResources"));

			FTextLocalizationManager::Get().RefreshResources();

			bFetchInProgress = false;
		}
	);
}
// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#include "TolgeeSettings.h"

#include <HttpModule.h>
#include <Interfaces/IHttpResponse.h>

#include "TolgeeLog.h"
#include "TolgeeUtils.h"

#if WITH_EDITOR
#include <ISettingsModule.h>
#endif

#if WITH_EDITOR
void UTolgeeSettings::OpenSettings()
{
	const UTolgeeSettings* Settings = GetDefault<UTolgeeSettings>();

	ISettingsModule& SettingsModule = FModuleManager::LoadModuleChecked<ISettingsModule>("Settings");
	SettingsModule.ShowViewer(Settings->GetContainerName(), Settings->GetCategoryName(), Settings->GetSectionName());
}
#endif

bool UTolgeeSettings::IsReadyToSendRequests() const
{
	return !ApiKey.IsEmpty() && !ApiUrl.IsEmpty() && !ProjectId.IsEmpty();
}

FName UTolgeeSettings::GetContainerName() const
{
	return TEXT("Project");
}

FName UTolgeeSettings::GetCategoryName() const
{
	return TEXT("Localization");
}

FName UTolgeeSettings::GetSectionName() const
{
	return TEXT("Tolgee");
}

#if WITH_EDITOR
FText UTolgeeSettings::GetSectionText() const
{
	const FName DisplaySectionName = GetSectionName();
	return FText::FromName(DisplaySectionName);
}
#endif

#if WITH_EDITOR
void UTolgeeSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UTolgeeSettings, ApiKey))
	{
		FetchProjectId();
	}
}
#endif

void UTolgeeSettings::FetchProjectId()
{
	UE_LOG(LogTolgee, Verbose, TEXT("UTolgeeSettings::FetchProjectId"));

	const FString RequestUrl = TolgeeUtils::GetUrlEndpoint(TEXT("v2/api-keys/current"));
	const FHttpRequestRef HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("GET");
	HttpRequest->SetHeader(TEXT("X-API-Key"), ApiKey);
	HttpRequest->SetHeader(TEXT("X-Tolgee-SDK-Type"), TolgeeUtils::GetSdkType());
	HttpRequest->SetHeader(TEXT("X-Tolgee-SDK-Version"), TolgeeUtils::GetSdkVersion());
	HttpRequest->SetURL(RequestUrl);
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnProjectIdFetched);
	HttpRequest->ProcessRequest();
}

void UTolgeeSettings::OnProjectIdFetched(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	UE_LOG(LogTolgee, Verbose, TEXT("UTolgeeSettings::OnProjectIdFetched"));

	if (!bWasSuccessful)
	{
		UE_LOG(LogTolgee, Error, TEXT("Request to fetch ProjectId was unsuccessful."));
		ProjectId = TEXT("INVALID");
		return;
	}

	if (!EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		UE_LOG(LogTolgee, Error, TEXT("Request to fetch ProjectId received unexpected code: %s"), *LexToString(Response->GetResponseCode()));
		ProjectId = TEXT("INVALID");
		return;
	}

	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	const TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Response->GetContentAsString());
	if (!FJsonSerializer::Deserialize(JsonReader, JsonObject))
	{
		UE_LOG(LogTolgee, Error, TEXT("Could not deserialize response: %s"), *LexToString(Response->GetContentAsString()));
		ProjectId = TEXT("INVALID");
		return;
	}

	ProjectId = [JsonObject]()
	{
		int64 ProjectIdValue;
		JsonObject->TryGetNumberField(TEXT("projectId"), ProjectIdValue);
		return LexToString(ProjectIdValue);
	}();

	SaveToDefaultConfig();

	FetchDefaultLanguages();
}

void UTolgeeSettings::FetchDefaultLanguages()
{
	UE_LOG(LogTolgee, Verbose, TEXT("UTolgeeSettings::FetchDefaultLanguages"));

	const FString RequestUrl = TolgeeUtils::GetUrlEndpoint(TEXT("v2/projects/stats"));
	const FHttpRequestRef HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("GET");
	HttpRequest->SetHeader(TEXT("X-API-Key"), ApiKey);
	HttpRequest->SetHeader(TEXT("X-Tolgee-SDK-Type"), TolgeeUtils::GetSdkType());
	HttpRequest->SetHeader(TEXT("X-Tolgee-SDK-Version"), TolgeeUtils::GetSdkVersion());
	HttpRequest->SetURL(RequestUrl);
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnDefaultLanguagesFetched);
	HttpRequest->ProcessRequest();
}

void UTolgeeSettings::OnDefaultLanguagesFetched(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	UE_LOG(LogTolgee, Verbose, TEXT("UTolgeeSettings::OnDefaultLanguagesFetched"));

	if (!bWasSuccessful)
	{
		UE_LOG(LogTolgee, Error, TEXT("Request to fetch default languages was unsuccessful."));
		return;
	}

	if (!EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		UE_LOG(LogTolgee, Error, TEXT("Request to fetch default languages received unexpected code: %s"), *LexToString(Response->GetResponseCode()));
		return;
	}

	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
	const TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(Response->GetContentAsString());
	if (!FJsonSerializer::Deserialize(JsonReader, JsonObject))
	{
		UE_LOG(LogTolgee, Error, TEXT("Could not deserialize response: %s"), *LexToString(Response->GetContentAsString()));
		return;
	}

	Languages.Empty();

	TArray<TSharedPtr<FJsonValue>> LanguageStats = JsonObject->GetArrayField(TEXT("languageStats"));
	for (const TSharedPtr<FJsonValue>& Language : LanguageStats)
	{
		FString LanguageLocale;
		Language->AsObject()->TryGetStringField(TEXT("languageTag"), LanguageLocale);
		Languages.Add(LanguageLocale);
	}

	SaveToDefaultConfig();
}

void UTolgeeSettings::SaveToDefaultConfig()
{
	SaveConfig(CPF_Config, *GetDefaultConfigFilename());
}

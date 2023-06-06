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

void UTolgeeSettings::FetchProjectId()
{
	const FString RequestUrl = TolgeeUtils::GetUrlEndpoint(TEXT("v2/api-keys/current"));
	const TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("GET");
	HttpRequest->SetHeader(TEXT("X-API-Key"), ApiKey);
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

	SaveConfig();
}

#endif

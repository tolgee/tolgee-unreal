// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#include "TolgeeCdnFetcherSubsystem.h"

#include <HttpModule.h>
#include <Interfaces/IHttpResponse.h>
#include <Kismet/KismetInternationalizationLibrary.h>

#include "TolgeeRuntimeSettings.h"
#include "TolgeeLog.h"

void UTolgeeCdnFetcherSubsystem::OnGameInstanceStart(UGameInstance* GameInstance)
{
	const UTolgeeRuntimeSettings* Settings = GetDefault<UTolgeeRuntimeSettings>();
	if (Settings->CdnAddresses.IsEmpty())
	{
		UE_LOG(LogTolgee, Display, TEXT("No CDN addresses configured. Packaged builds will only use static data."));
		return;
	}
	if (GIsEditor && !Settings->bUseCdnInEditor)
	{
		UE_LOG(LogTolgee, Display, TEXT("CDN was disabled for editor but it will be used in the final game."));
		return;
	}

	FetchAllCdns();
}

void UTolgeeCdnFetcherSubsystem::OnGameInstanceEnd(bool bIsSimulating)
{
	ResetData();
}

TMap<FString, TArray<FTolgeeTranslationData>> UTolgeeCdnFetcherSubsystem::GetDataToInject() const
{
	return CachedTranslations;
}

void UTolgeeCdnFetcherSubsystem::FetchAllCdns()
{
	const UTolgeeRuntimeSettings* Settings = GetDefault<UTolgeeRuntimeSettings>();

	for (const FString& CdnAddress : Settings->CdnAddresses)
	{
		TArray<FString> Cultures = UKismetInternationalizationLibrary::GetLocalizedCultures();
		for (const FString& Culture : Cultures)
		{
			const FString DownloadUrl = FString::Printf(TEXT("%s/%s.po"), *CdnAddress, *Culture);

			UE_LOG(LogTolgee, Display, TEXT("Fetching localization data for culture: %s from CDN: %s"), *Culture, *DownloadUrl);
			FetchFromCdn(Culture, DownloadUrl);
		}
	};

}

void UTolgeeCdnFetcherSubsystem::FetchFromCdn(const FString& Culture, const FString& DownloadUrl)
{
	const FString* LastModifiedDate = LastModifiedDates.Find(DownloadUrl);

	const FHttpRequestRef HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetVerb("GET");
	HttpRequest->SetURL(DownloadUrl);
	HttpRequest->SetHeader(TEXT("accept"), TEXT("application/json"));

	if (LastModifiedDate)
	{
		HttpRequest->SetHeader(TEXT("If-Modified-Since"), *LastModifiedDate);
	}

	HttpRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnFetchedFromCdn, Culture);
	HttpRequest->ProcessRequest();

	NumRequestsSent++;
}

void UTolgeeCdnFetcherSubsystem::OnFetchedFromCdn(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FString InCutlure)
{
	if (bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		UE_LOG(LogTolgee, Display, TEXT("Fetch successfully for %s to %s."), *InCutlure, *Request->GetURL());

		TArray<FTolgeeTranslationData> Translations = ExtractTranslationsFromPO(Response->GetContentAsString());
		CachedTranslations.Emplace(InCutlure, Translations);

		const FString LastModified = Response->GetHeader(TEXT("Last-Modified"));
		if (!LastModified.IsEmpty())
		{
			LastModifiedDates.Emplace(Request->GetURL(), LastModified);
		}
	}
	else if (Response.IsValid() && Response->GetResponseCode() == EHttpResponseCodes::NotModified)
	{
		UE_LOG(LogTolgee, Display, TEXT("No new data for %s to %s."), *InCutlure, *Request->GetURL());
	}
	else
	{
		UE_LOG(LogTolgee, Error, TEXT("Request for %s to %s failed."), *InCutlure, *Request->GetURL());
	}

	NumRequestsCompleted++;

	if (NumRequestsCompleted == NumRequestsSent)
	{
		UE_LOG(LogTolgee, Display, TEXT("All requests completed. Refreshing translation data."));
		RefreshTranslationDataAsync();
	}
}

void UTolgeeCdnFetcherSubsystem::ResetData()
{
	NumRequestsSent = 0;
	NumRequestsCompleted = 0;

	CachedTranslations.Empty();
	LastModifiedDates.Empty();
}
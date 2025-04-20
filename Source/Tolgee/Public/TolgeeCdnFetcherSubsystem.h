// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#pragma once

#include "TolgeeLocalizationInjectorSubsystem.h"

#include <Interfaces/IHttpRequest.h>

#include "TolgeeCdnFetcherSubsystem.generated.h"

/**
 * Subsystem responsible for fetching localization data from a CDN and injecting it into the game.
 */
UCLASS()
class UTolgeeCdnFetcherSubsystem : public UTolgeeLocalizationInjectorSubsystem 
{
	GENERATED_BODY()

	// ~ Begin UTolgeeLocalizationInjectorSubsystem interface
	virtual void OnGameInstanceStart(UGameInstance* GameInstance) override;
	virtual void OnGameInstanceEnd(bool bIsSimulating) override;
	virtual TMap<FString, TArray<FTolgeeTranslationData>> GetDataToInject() const override;
	// ~ End UTolgeeLocalizationInjectorSubsystem interface

	/**
	 * Runs multiple requests to fetch all projects from the CDN.
	 */
	void FetchAllCdns();
	/**
	 * Fetches the localization data from the CDN.
	 */
	void FetchFromCdn(const FString& Culture, const FString& DownloadUrl);
	/**
	 * Callback function for when the CDN request is completed.
	 */
	void OnFetchedFromCdn(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FString InCutlure);
	/**
	 * Clears the cached translations and resets the request counters.
	 */
	void ResetData();
	/**
	 * List of cached translations for each culture.
	 */
	TMap<FString, TArray<FTolgeeTranslationData>> CachedTranslations;
	/**
	 * Counts the number of requests sent.
	 */
	int32 NumRequestsSent = 0;
	/**
	 * Counts the number of requests completed.
	 */
	int32 NumRequestsCompleted = 0;
	/**
	 * Map storing the last modified dates of the translations.
	 */
	TMap<FString, FString> LastModifiedDates;
};
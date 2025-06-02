// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#pragma once

#include "TolgeeLocalizationInjectorSubsystem.h"

#include <Interfaces/IHttpRequest.h>

#include "TolgeeEditorIntegrationSubsystem.generated.h"

/**
 * Subsystem responsible for fetching localization data directly from the Tolgee dashboard (without exporting or CDN publishing).
 */
UCLASS()
class UTolgeeEditorIntegrationSubsystem : public UTolgeeLocalizationInjectorSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Performs an immediate fetch of the localization data from the Tolgee dashboard.
	 */
	void ManualFetch();

private:
	// ~ Begin UTolgeeLocalizationInjectorSubsystem interface
	virtual void OnGameInstanceStart(UGameInstance* GameInstance) override;
	virtual void OnGameInstanceEnd(bool bIsSimulating) override;
	virtual TMap<FString, TArray<FTolgeeTranslationData>> GetDataToInject() const override;
	// ~ End UTolgeeLocalizationInjectorSubsystem interface

	/*
	 * Runs multiple requests to fetch all projects from the Tolgee dashboard.
	 */
	void FetchAllProjects();
	/**
	 * Fetches the localization data from the Tolgee dashboard.
	 */
	void FetchFromDashboard(const FString& ProjectId, const FString& RequestUrl);
	/**
	 * Callback function for when the Tolgee dashboard data is retrived.
	 */
	void OnFetchedFromDashboard(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FString ProjectId);
	/**
	 * Clears the cached translations and resets the request counters.
	 */
	void ResetData();
	/**
	 * Saves a request content to a temporary zip on disk and reads the translations from it.
	 */
	bool ReadTranslationsFromZipContent(const FString& ProjectId, const TArray<uint8>& ResponseContent);
	/**
	 * Callback function executed at a regular interval to refresh the localization data.
	 */
	void OnRefreshTick();
	/**
	 * Runs an asynchronous request to check if any updates are available for the projects.
	 */
	void FetchIUpdatesAreAvailableAsync();
	/**
	 * If any of the projects were updated, fetches resets the data and fetches the latest translations.
	 */
	void FetchIfProjectsWereUpdated();
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
	/*
	 * Handle for the refresh tick delegate used to constantly refresh the localization data.
	 */
	FTimerHandle RefreshTick;
	/**
	 * Last time any translations were fetched from the Tolgee dashboard.
	 */
	FDateTime LastFetchTime = {0};
	/**
	 * Flag used to prevent multiple requests from being sent at the same time.
	 */
	TAtomic<bool> bRequestInProgress = false;
};
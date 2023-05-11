// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Engine/EngineTypes.h>
#include <Engine/GameInstance.h>
#include <Interfaces/IHttpRequest.h>
#include <Subsystems/EngineSubsystem.h>

#include "TolgeeRuntimeRequestData.h"

#include "TolgeeLocalizationSubsystem.generated.h"

class FTolgeeTextSource;

using FOnTranslationFetched = TDelegate<void(TArray<FTolgeeKeyData> Translations)>;

/**
 * Subsystem responsible for fetching the latest translation data from Tolgee backend and applying it inside Unreal.
 */
UCLASS()
class TOLGEE_API UTolgeeLocalizationSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * @brief Immediately sends a fetch request to grab the latest data
	 */
	void ManualFetch();
	/**
	 * @brief Returns the result of the last fetch operation
	 */
	TArray<FTolgeeKeyData> GetLastFetchedKeys() const;

private:
	// Begin UEngineSubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	// End UEngineSubsystem interface
	/**
	 * @brief Callback executed when the TolgeeTextSource needs to update the translation data based on the last fetched keys.
	 * @param InLoadFlags
	 * @param InPrioritizedCultures
	 * @param InOutNativeResource
	 * @param InOutLocalizedResource
	 */
	void GetLocalizedResources(
		const ELocalizationLoadFlags InLoadFlags, TArrayView<const FString> InPrioritizedCultures, FTextLocalizationResource& InOutNativeResource, FTextLocalizationResource& InOutLocalizedResource
	) const;
	/**
	 * @brief Callback executed when the game instance is created and started.
	 */
	void OnGameInstanceStart(UGameInstance* GameInstance);
	/**
	 * @brief Starts the fetching process to get the latest translated version.
	 */
	void FetchTranslation();
	/**
	 * @brief Sends a GET request based on the current cursor to fetch the remaining keys.
	 * @param Callback to be executed when all the keys are fetched
	 * @param Languages Languages we want to retrieve
	 * @param CurrentTranslations Keys retrieved by previous requests
	 * @param NextCursor Indicator for the start of the next requests
	 */
	void FetchNextTranslation(FOnTranslationFetched Callback, TArray<FString> Languages, TArray<FTolgeeKeyData> CurrentTranslations, const FString& NextCursor);
	/**
	 * @brief Callback executed when a translation fetch is retrieved.
	 * @param Request Request that triggered this callback
	 * @param Response Response retrieved from the backend
	 * @param bWasSuccessful Request status
	 * @param Callback Callback to be executed when all the keys are fetched
	 * @param Languages Languages we want to retrieve
	 * @param CurrentTranslations Keys retrieved by previous requests
	 */
	void OnNextTranslationFetched(
		FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FOnTranslationFetched Callback, TArray<FString> Languages, TArray<FTolgeeKeyData> CurrentTranslations
	);
	/**
	 * @brief Triggers an async refresh of the LocalizationManager resources.
	 * @param Translations Keys we should use for the TolgeeTextSource
	 */
	void RefreshTranslationData(TArray<FTolgeeKeyData> Translations);
	/**
	 * @brief Cached response of the last successfully fetched keys.
	 */
	TArray<FTolgeeKeyData> TranslatedKeys;
	/**
	 * @brief Timer responsible for periodically fetching the latest keys.
	 */
	FTimerHandle AutoFetchTimerHandle;
	/**
	 * @brief TextSource hooked into the LocalizationManager to feed in the latest keys in the translation data.
	 */
	TSharedPtr<FTolgeeTextSource> TextSource;
	/**
	 * @brief Are we processing an update from the backend right now? Used to prevent 2 concurrent updates
	 */
	bool bFetchInProgress = false;
};

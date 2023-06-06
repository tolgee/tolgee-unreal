// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#pragma once

#include <Engine/DeveloperSettings.h>
#include <Interfaces/IHttpRequest.h>

#include "TolgeeSettings.generated.h"

/**
 * Holds configuration for integrating the Tolgee localization source
 */
UCLASS(Config = Tolgee)
class TOLGEE_API UTolgeeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	/**
	 * @brief Shortcut to open the project settings windows focused to this config
	 */
	static void OpenSettings();
#endif
	/*
	 * @brief Authentication key for fetching translation data
	 * @see https://tolgee.io/platform/account_settings/api_keys_and_pat_tokens
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Tolgee Localization")
	FString ApiKey = TEXT("");
	/**
	 * @brief Url of the Tolgee server. Useful for self-hosted installations
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Tolgee Localization")
	FString ApiUrl = TEXT("https://app.tolgee.io");
	/**
	 * @brief Id of the Tolgee project we should use for translation
	 */
	UPROPERTY(Config, VisibleAnywhere, Category = "Tolgee Localization")
	FString ProjectId = TEXT("");
	/**
	 * @brief Should we automatically fetch translation data at runtime?
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Tolgee Localization")
	bool bLiveTranslationUpdates = true;
	/**
	 * @brief How often we should update the translation data from the server
	 * @note in seconds
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Tolgee Localization", meta = (EditCondition = "bLiveTranslationUpdates", UIMin = "0"))
	float UpdateInterval = 60.0f;

private:
	// Begin UDeveloperSettings interface
	virtual FName GetContainerName() const override;
	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;
#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// End UDeveloperSettings interface

	/**
	 * @brief Sends a request to the Tolgee server to get information about the current project based on the ApiKey
	 */
	void FetchProjectId();
	/**
	 * @brief Callback executed when the information about the current Project is retried
	 */
	void OnProjectIdFetched(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};

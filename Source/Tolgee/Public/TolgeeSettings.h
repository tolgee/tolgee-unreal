// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#pragma once

#include <Engine/DeveloperSettings.h>
#include <Interfaces/IHttpRequest.h>

#include "TolgeeSettings.generated.h"

/**
 * Holds configuration for integrating the Tolgee localization source
 */
UCLASS(config = Tolgee, defaultconfig)
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
	/**
	 * @brief Checks if all the conditions to send requests to the Tolgee backend are met
	 * @note ApiKey, ApiUrl & ProjectId set properly
	 */
	bool IsReadyToSendRequests() const;
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
	 * @brief Locales we should fetch translations for
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Tolgee Localization")
	TArray<FString> Languages;
	/**
	 * @brief With this enabled only strings from string tables will be uploaded & translated. Generated Keys (produced by Unreal for strings outside of string tables) will be ignored.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Tolgee Localization")
	bool bIgnoreGeneratedKeys = true;
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
	/**
	 * @brief Automatically fetch Translations.json on cook
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Tolgee Localization")
	bool bFetchTranslationsOnCook = true;
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
	 * @brief Callback executed when the information about the current project is retried
	 */
	void OnProjectIdFetched(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	/**
	 * @brief Sends a request to the Tolgee server to get language information about the current project based on the ApiKey
	 */
	void FetchDefaultLanguages();
	/**
	 * @brief Callback executed when the language information about the current project is retrieved
	 */
	void OnDefaultLanguagesFetched(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	/**
	 * @brief Saves the current values to the .ini file inside root Config folder
	 */
	void SaveToDefaultConfig();
};

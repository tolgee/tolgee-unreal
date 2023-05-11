// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#pragma once

#include <Engine/DeveloperSettings.h>

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
	 * @brief Id of the Tolgee project we should use for translation
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Tolgee Localization")
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
#endif
	// End UDeveloperSettings interface
};

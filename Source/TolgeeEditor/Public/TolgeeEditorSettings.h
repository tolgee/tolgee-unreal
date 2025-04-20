// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#pragma once

#include <Engine/DeveloperSettings.h>

#include "TolgeeEditorSettings.generated.h"

/**
 * Contains all the configurable properties for a single Localization Target.
 */
USTRUCT()
struct FTolgeePerTargetSettings 
{
	GENERATED_BODY()

	/**
	 * Id of the project in Tolgee.
	 */
	UPROPERTY(EditAnywhere, Category = "Tolgee Localization")
	FString ProjectId;
};


/*
 * @brief Settings for the Tolgee editor-only functionality.
 */
UCLASS(config = Tolgee, defaultconfig)
class TOLGEEEDITOR_API UTolgeeEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * Api Key used for requests authentication.
	 * IMPORTANT: This will be saved in plain text in the config file.
	 * IMPORTANT: If you have multiple Tolgee projects connected, use a Personal Access Token instead of the API key.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Tolgee")
	FString ApiKey = TEXT("");

	/**
	 * Api Url used for requests.
	 * IMPORTANT: Change this if you are using a self-hosted Tolgee instance.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Tolgee")
	FString ApiUrl = TEXT("https://app.tolgee.io");

	/**
	 * Project IDs we want to fetch translations for during editor PIE sessions.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Tolgee|In-Context")
	TArray<FString> ProjectIds;

	/**
	 * How often we should check the projects above for updates.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Tolgee|In-Context")
	float RefreshInterval = 20.0f;

	/**
	 * Configurable settings for each localization target.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Tolgee|Provider")
	TMap<FGuid, FTolgeePerTargetSettings> PerTargetSettings;

	// ~ Begin UDeveloperSettings Interface
	virtual FName GetCategoryName() const override;
	// ~ End UDeveloperSettings Interface
};


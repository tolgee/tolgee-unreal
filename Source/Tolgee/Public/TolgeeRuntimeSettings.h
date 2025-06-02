// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#pragma once

#include <Engine/DeveloperSettings.h>

#include "TolgeeRuntimeSettings.generated.h"

/**
 * @brief Settings for the Tolgee runtime functionality.
 * NOTE: Settings here will be packaged in the final game
 */
UCLASS(config = Tolgee, defaultconfig)
class TOLGEE_API UTolgeeRuntimeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * Root addresses we should use to fetch the localization data from the CDN.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Tolgee|CDN")
	TArray<FString> CdnAddresses;

	/**
	 * Easy toggle to disable the CDN functionality in the editor.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Tolgee|CDN")
	bool bUseCdnInEditor = false;

	// ~ Begin UDeveloperSettings Interface
	virtual FName GetCategoryName() const override;
	// ~ End UDeveloperSettings Interface
};
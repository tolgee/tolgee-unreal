// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>

#include "TolgeeLocalizationProvider.h"

/**
 * Module responsible for introducing the Tolgee implementation of the LocalizationServiceProvider.
 */
class FTolgeeProviderModule : public IModuleInterface
{
	// ~Begin IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	// ~End IModuleInterface interface

	/**
	 * Re-applys the settings from LocalizationService.LocalizationServiceSettings ini file.
	 */
	void ReApplySettings();
	/**
	 * Ensures editor settings are not packaged in the final game.
	 */
	void DenyEditorSettings();
	/**
	 * LocalizationServiceProvider instance for Tolgee.
	 */
	FTolgeeLocalizationProvider TolgeeLocalizationProvider;
};
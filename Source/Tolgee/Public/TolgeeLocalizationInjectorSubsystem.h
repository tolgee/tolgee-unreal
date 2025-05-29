// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#pragma once

#include <Subsystems/EngineSubsystem.h>

#include "TolgeeLocalizationInjectorSubsystem.generated.h"

class UGameInstance;
class FTolgeeTextSource;

/**
 * Holds the parsed data from the PO file
 */
struct FTolgeeTranslationData
{
	FString ParsedNamespace;
	FString ParsedKey;
	FString SourceText;
	FString Translation;
};

/**
 * Base class for all subsystems responsible for dynamically injecting data at runtime (e.g.: CDN, Dashboard, etc.)
 */
UCLASS(Abstract)
class TOLGEE_API UTolgeeLocalizationInjectorSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

protected:
	/**
	 * Callback executed when the game instance is created and started.
	 */
	virtual void OnGameInstanceStart(UGameInstance* GameInstance);
	/**
	 * Callback executed when the game instance is destroyed.
	 */
	virtual void OnGameInstanceEnd(bool bIsSimulating);
	/**
	 * Callback executed when the TolgeeTextSource needs to load the localized resources.
	 */
	virtual void GetLocalizedResources(const ELocalizationLoadFlags InLoadFlags, TArrayView<const FString> InPrioritizedCultures, FTextLocalizationResource& InOutNativeResource, FTextLocalizationResource& InOutLocalizedResource) const;
	/**
	 * Simplified getter to allow subclasses to provide their own data to inject for GetLocalizedResources.
	 */
	virtual TMap<FString, TArray<FTolgeeTranslationData>> GetDataToInject() const;
	/**
	 * Triggers an async refresh of the LocalizationManager resources.
	 */
	void RefreshTranslationDataAsync();
	/**
	 * Converts PO content to a list of translation data.
	 */
	TArray<FTolgeeTranslationData> ExtractTranslationsFromPO(const FString& PoContent);

	// Begin UEngineSubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	// End UEngineSubsystem interface

private:
	/**
	 * Custom Localization Text Source that allows handling of Localized Resources via delegate
	 */
	TSharedPtr<FTolgeeTextSource> TextSource;
};

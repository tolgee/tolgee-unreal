// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#pragma once

#include <Internationalization/ILocalizedTextSource.h>

using FGetLocalizedResources = TDelegate<void(const ELocalizationLoadFlags InLoadFlags, TArrayView<const FString> InPrioritizedCultures, FTextLocalizationResource& InOutNativeResource, FTextLocalizationResource& InOutLocalizedResource)>;

/**
 * Translation source data for injecting data fetched from Tolgee backend into the localization system.
 */
class TOLGEE_API FTolgeeTextSource : public ILocalizedTextSource
{
public:
	/**
	 * Callback executed when the text source needs to load the localized resources
	 */
	FGetLocalizedResources GetLocalizedResources;

private:
	// Begin ILocalizedTextSource interface
	virtual int32 GetPriority() const override;
	virtual bool GetNativeCultureName(const ELocalizedTextSourceCategory InCategory, FString& OutNativeCultureName) override;
	virtual void GetLocalizedCultureNames(const ELocalizationLoadFlags InLoadFlags, TSet<FString>& OutLocalizedCultureNames) override;
	virtual void LoadLocalizedResources(const ELocalizationLoadFlags InLoadFlags, TArrayView<const FString> InPrioritizedCultures, FTextLocalizationResource& InOutNativeResource, FTextLocalizationResource& InOutLocalizedResource) override;
	// End ILocalizedTextSource interface
};

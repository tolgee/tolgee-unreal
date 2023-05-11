// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#pragma once

#include <Internationalization/ILocalizedTextSource.h>

using FGetLocalizedResources = TDelegate<
	void(const ELocalizationLoadFlags InLoadFlags, TArrayView<const FString> InPrioritizedCultures, FTextLocalizationResource& InOutNativeResource, FTextLocalizationResource& InOutLocalizedResource)>;

/**
 * Translation source data for providing data fetched from Tolgee backend
 */
class FTolgeeTextSource : public ILocalizedTextSource
{
public:
	/**
	 * @brief Callback executed when the text source needs to load the localized resources
	 */
	FGetLocalizedResources GetLocalizedResources;

private:
	// Begin ILocalizedTextSource interface
	virtual int32 GetPriority() const override { return ELocalizedTextSourcePriority::Highest; }
	virtual bool GetNativeCultureName(const ELocalizedTextSourceCategory InCategory, FString& OutNativeCultureName) override;
	virtual void GetLocalizedCultureNames(const ELocalizationLoadFlags InLoadFlags, TSet<FString>& OutLocalizedCultureNames) override;
	virtual void LoadLocalizedResources(
		const ELocalizationLoadFlags InLoadFlags, TArrayView<const FString> InPrioritizedCultures, FTextLocalizationResource& InOutNativeResource, FTextLocalizationResource& InOutLocalizedResource
	) override;
	// End ILocalizedTextSource interface
};

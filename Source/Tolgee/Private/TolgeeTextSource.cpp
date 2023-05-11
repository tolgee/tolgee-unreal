// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#include "TolgeeTextSource.h"

#include "Internationalization/TextLocalizationResource.h"
bool FTolgeeTextSource::GetNativeCultureName(const ELocalizedTextSourceCategory InCategory, FString& OutNativeCultureName)
{
	// TODO: Investigate if we should implement this
	return false;
}

void FTolgeeTextSource::GetLocalizedCultureNames(const ELocalizationLoadFlags InLoadFlags, TSet<FString>& OutLocalizedCultureNames)
{
	// TODO: Investigate if we should implement this
}

void FTolgeeTextSource::LoadLocalizedResources(
	const ELocalizationLoadFlags InLoadFlags, TArrayView<const FString> InPrioritizedCultures, FTextLocalizationResource& InOutNativeResource, FTextLocalizationResource& InOutLocalizedResource
)
{
	GetLocalizedResources.Execute(InLoadFlags, InPrioritizedCultures, InOutNativeResource, InOutLocalizedResource);
}
// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#include "TolgeeTextSource.h"

#include <Internationalization/TextLocalizationResource.h>

int32 FTolgeeTextSource::GetPriority() const
{
	//NOTE: you might expect to see ::Highest, but we actually want to be the last one to load resources so we can re-use the existing SourceStringHash
	// Then, during the load we will inject the new entries the highest priority.
	return ELocalizedTextSourcePriority::Lowest;
}

bool FTolgeeTextSource::GetNativeCultureName(const ELocalizedTextSourceCategory InCategory, FString& OutNativeCultureName)
{
	// TODO: Investigate if we should implement this
	return false;
}

void FTolgeeTextSource::GetLocalizedCultureNames(const ELocalizationLoadFlags InLoadFlags, TSet<FString>& OutLocalizedCultureNames)
{
	// TODO: Investigate if we should implement this
}

void FTolgeeTextSource::LoadLocalizedResources(const ELocalizationLoadFlags InLoadFlags, TArrayView<const FString> InPrioritizedCultures, FTextLocalizationResource& InOutNativeResource, FTextLocalizationResource& InOutLocalizedResource)
{
	GetLocalizedResources.Execute(InLoadFlags, InPrioritizedCultures, InOutNativeResource, InOutLocalizedResource);
}
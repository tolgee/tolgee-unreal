// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#include "TolgeeUtils.h"

#include <Internationalization/TextLocalizationResource.h>

#include "TolgeeSettings.h"

FString TolgeeUtils::GetTranslationHash(const FString& Translation)
{
	const uint32 TranslationHash = FTextLocalizationResource::HashString(Translation);
	return LexToString(TranslationHash);
}

FString TolgeeUtils::AppendQueryParameters(const FString& BaseUrl, const TArray<FString>& Parameters)
{
	FString FinalUrl = BaseUrl;
	TArray<FString> RemainingParameters = Parameters;

	if (RemainingParameters.Num() >= 1)
	{
		FinalUrl.Appendf(TEXT("?%s"), *RemainingParameters[0]);
		RemainingParameters.RemoveAt(0);
	}

	for (const FString& Parameter : RemainingParameters)
	{
		FinalUrl.Appendf(TEXT("&%s"), *Parameter);
	}

	return FinalUrl;
}
FString TolgeeUtils::GetProjectAppUrl()
{
	const FString& ProjectId = GetDefault<UTolgeeSettings>()->ProjectId;
	return FString::Printf(TEXT("https://app.tolgee.io/projects/%s"), *ProjectId);
}
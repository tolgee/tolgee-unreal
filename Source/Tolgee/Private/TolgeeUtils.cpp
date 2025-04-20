// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#include "TolgeeUtils.h"

#include <Interfaces/IPluginManager.h>

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

FString TolgeeUtils::GetSdkType()
{
	return TEXT("Unreal");
}

FString TolgeeUtils::GetSdkVersion()
{
	const TSharedPtr<IPlugin> TolgeePlugin = IPluginManager::Get().FindPlugin("Tolgee");
	return TolgeePlugin->GetDescriptor().VersionName;
}
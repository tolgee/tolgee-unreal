// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#include "TolgeeUtils.h"

#include <Interfaces/IPluginManager.h>
#include <Internationalization/TextLocalizationResource.h>
#include <Misc/Paths.h>

#include "TolgeeSettings.h"

uint32 TolgeeUtils::GetTranslationHash(const FString& Translation)
{
	return FTextLocalizationResource::HashString(Translation);
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

FString TolgeeUtils::GetUrlEndpoint(const FString& EndPoint)
{
	const FString& BaseUrl = GetDefault<UTolgeeSettings>()->ApiUrl;
	return FString::Printf(TEXT("%s/%s"), *BaseUrl, *EndPoint);
}

FString TolgeeUtils::GetProjectUrlEndpoint(const FString& EndPoint /* = TEXT("") */)
{
	const FString ProjectId = GetDefault<UTolgeeSettings>()->ProjectId;
	const FString ProjectEndPoint = FString::Printf(TEXT("projects/%s/%s"), *ProjectId, *EndPoint);
	return GetUrlEndpoint(ProjectEndPoint);
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

FString TolgeeUtils::GetLocalizationSourceFile()
{
	return FPaths::ProjectContentDir() + TEXT("Tolgee/Translations.json");
}

FDirectoryPath TolgeeUtils::GetLocalizationDirectory()
{
	FString Foldername = FPaths::GetPath(GetLocalizationSourceFile());
	FPaths::MakePathRelativeTo(Foldername, *FPaths::ProjectContentDir());
	return {Foldername};
}

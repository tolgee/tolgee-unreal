// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#include "TolgeeSettings.h"

#if WITH_EDITOR
#include <ISettingsModule.h>
#endif

#if WITH_EDITOR
void UTolgeeSettings::OpenSettings()
{
	const UTolgeeSettings* Settings = GetDefault<UTolgeeSettings>();

	ISettingsModule& SettingsModule = FModuleManager::LoadModuleChecked<ISettingsModule>("Settings");
	SettingsModule.ShowViewer(Settings->GetContainerName(), Settings->GetCategoryName(), Settings->GetSectionName());
}
#endif

FName UTolgeeSettings::GetContainerName() const
{
	return TEXT("Project");
}

FName UTolgeeSettings::GetCategoryName() const
{
	return TEXT("Localization");
}

FName UTolgeeSettings::GetSectionName() const
{
	return TEXT("Tolgee");
}

#if WITH_EDITOR
FText UTolgeeSettings::GetSectionText() const
{
	const FName DisplaySectionName = GetSectionName();
	return FText::FromName(DisplaySectionName);
}
#endif

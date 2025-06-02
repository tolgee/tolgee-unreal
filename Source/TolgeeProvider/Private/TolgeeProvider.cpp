// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#include "TolgeeProvider.h"

#include <ILocalizationServiceModule.h>
#include <LocalizationServiceHelpers.h>
#include <Settings/ProjectPackagingSettings.h>

#include "TolgeeLog.h"
#include "TolgeeProviderLocalizationServiceOperations.h"

void FTolgeeProviderModule::StartupModule()
{
	DenyEditorSettings();

	IModularFeatures::Get().RegisterModularFeature("LocalizationService", &TolgeeLocalizationProvider);

	TolgeeLocalizationProvider.RegisterWorker<FTolgeeProviderUploadFileWorker>("UploadLocalizationTargetFile");
	TolgeeLocalizationProvider.RegisterWorker<FTolgeeProviderDownloadFileWorker>("DownloadLocalizationTargetFile");

	// Currently, there is a bug in 5.5 where the saved settings are not getting applied
	// because FLocalizationServiceSettings::LoadSettings reads from the `CoalescedSourceConfigs` config
	ReApplySettings();
}

void FTolgeeProviderModule::ShutdownModule()
{
	IModularFeatures::Get().UnregisterModularFeature("LocalizationService", &TolgeeLocalizationProvider);
}

void FTolgeeProviderModule::ReApplySettings()
{
	const FString& IniFile = LocalizationServiceHelpers::GetSettingsIni();
	if (FConfigFile* ConfigFile = GConfig->Find(IniFile))
	{
		ConfigFile->Read(IniFile);
	}

	ILocalizationServiceModule& LocalizationService = ILocalizationServiceModule::Get();
	FString SavedProvider;
	GConfig->GetString(TEXT("LocalizationService.LocalizationServiceSettings"), TEXT("Provider"), SavedProvider, IniFile);
	if (!SavedProvider.IsEmpty() && LocalizationService.GetProvider().GetName() != FName(SavedProvider))
	{
		UE_LOG(LogTolgee, Display, TEXT("Applying saved Provider: %s"), *SavedProvider);
		LocalizationService.SetProvider(FName(SavedProvider));
	}

	bool bSavedUseGlobalSettings = false;
	const FString& GlobalIniFile = LocalizationServiceHelpers::GetGlobalSettingsIni();
	GConfig->GetBool(TEXT("LocalizationService.LocalizationServiceSettings"), TEXT("UseGlobalSettings"), bSavedUseGlobalSettings, GlobalIniFile);
	if (LocalizationService.GetUseGlobalSettings() != bSavedUseGlobalSettings)
	{
		LocalizationService.SetUseGlobalSettings(bSavedUseGlobalSettings);
	}
}

void FTolgeeProviderModule::DenyEditorSettings()
{
	const FString TolgeeProviderSettingsSection = TEXT("/Script/TolgeeProvider.TolgeeProviderSettings");
	UProjectPackagingSettings* ProjectPackagingSettings = GetMutableDefault<UProjectPackagingSettings>();
	if (!ProjectPackagingSettings->IniSectionDenylist.Contains(TolgeeProviderSettingsSection))
	{
		UE_LOG(LogTolgee, Display, TEXT("Adding %s to ProjectPackagingSettings.IniSectionDenylist"), *TolgeeProviderSettingsSection);
		ProjectPackagingSettings->IniSectionDenylist.Add(TolgeeProviderSettingsSection);
		ProjectPackagingSettings->SaveConfig();
	}
}

IMPLEMENT_MODULE(FTolgeeProviderModule, TolgeeProvider)
// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#include "TolgeeEditor.h"

#include <LevelEditor.h>
#include <Interfaces/IPluginManager.h>

#include "STolgeeTranslationTab.h"
#include "TolgeeEditorSettings.h"
#include "TolgeeStyle.h"

#define LOCTEXT_NAMESPACE "Tolgee"

namespace
{
	const FName MenuTabName = FName("TolgeeDashboardMenuTab");
}

void FTolgeeEditorModule::StartupModule()
{
	RegisterStyle();
	RegisterWindowExtension();
	RegisterToolbarExtension();
}

void FTolgeeEditorModule::ShutdownModule()
{
	UnregisterStyle();
	UnregisterWindowExtension();
	UnregisterToolbarExtension();
}

void FTolgeeEditorModule::RegisterStyle()
{
	FTolgeeStyle::Initialize();
}

void FTolgeeEditorModule::UnregisterStyle()
{
	FTolgeeStyle::Shutdown();
}

void FTolgeeEditorModule::RegisterWindowExtension()
{
	FTabSpawnerEntry& DashboardTab = FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		MenuTabName,
		FOnSpawnTab::CreateLambda(
			[](const FSpawnTabArgs& Args)
			{
				return SNew(STolgeeTranslationTab);
			}
		)
	);

	// clang-format off
	DashboardTab.SetDisplayName(LOCTEXT("DashboardName", "Tolgee Dashboard"))
		.SetTooltipText(LOCTEXT("DashboardTooltip", "Get an overview of your project state in a separate tab."))
		.SetIcon(FSlateIcon(FTolgeeStyle::Get().GetStyleSetName(), "Tolgee.Settings"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
	// clang-format on
}

void FTolgeeEditorModule::UnregisterWindowExtension()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MenuTabName);
}

void FTolgeeEditorModule::RegisterToolbarExtension()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	const TSharedPtr<FExtensibilityManager> ExtensionManager = LevelEditorModule.GetToolBarExtensibilityManager();

#if ENGINE_MAJOR_VERSION > 4
	const FName ToolbarHook = TEXT("ProjectSettings");
#else
	const FName ToolbarHook = TEXT("Settings");
#endif

	ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension(ToolbarHook, EExtensionHook::First, nullptr, FToolBarExtensionDelegate::CreateRaw(this, &FTolgeeEditorModule::ExtendToolbar));
	ExtensionManager->AddExtender(ToolbarExtender);
}

void FTolgeeEditorModule::UnregisterToolbarExtension()
{
	if (ToolbarExtender.IsValid())
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		const TSharedPtr<FExtensibilityManager> ExtensionManager = LevelEditorModule.GetToolBarExtensibilityManager();

		ExtensionManager->RemoveExtender(ToolbarExtender);
		ToolbarExtender.Reset();
	}
}

void FTolgeeEditorModule::ExtendToolbar(FToolBarBuilder& Builder)
{
	Builder.AddComboButton(
		FUIAction(),
		FOnGetContent::CreateLambda(
			[=]()
			{
				FMenuBuilder MenuBuilder(true, NULL);

				MenuBuilder.AddMenuEntry(
					LOCTEXT("TranslationDashboard", "Translation Tab"),
					LOCTEXT("TranslationDashboardTip", "Open a translation widget inside Unreal which allows you translate text by hovering on top"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda(
						[]()
						{
							FGlobalTabmanager::Get()->TryInvokeTab(MenuTabName);
						}
					))
				);

				MenuBuilder.AddMenuEntry(
					LOCTEXT("WebDashboard", "Web dashboard"),
					LOCTEXT("WebDashboardTip", "Launches the Tolgee dashboard in your browser"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda(
						[]()
						{
							const UTolgeeEditorSettings* Settings = GetDefault<UTolgeeEditorSettings>();
							FPlatformProcess::LaunchURL(*Settings->GetBaseUrl(), nullptr, nullptr);
						}
					))
				);

				MenuBuilder.AddMenuEntry(
					LOCTEXT("OpenSettings", "Open Settings"),
					LOCTEXT("OpenSettingsTip", "Open the plugin settings section"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda(
						[]()
						{
							//TODO: Enable this after settings get unified
							//UTolgeeSettings::OpenSettings();
						}
					))
				);

				MenuBuilder.AddMenuEntry(
					LOCTEXT("OpenDocumentation", "Open Documentation"),
					LOCTEXT("OpenDocumentationTip", "Open a step by step guide on how to use our integration"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda(
						[]()
						{
							const TSharedPtr<IPlugin> TolgeePlugin = IPluginManager::Get().FindPlugin("Tolgee");
							const FString DocsURL = TolgeePlugin->GetDescriptor().DocsURL;
							FPlatformProcess::LaunchURL(*DocsURL, nullptr, nullptr);
						}
					))
				);

				MenuBuilder.AddMenuEntry(
					LOCTEXT("GetSupport", "Get support"),
					LOCTEXT("GetSupportTip", "Join our slack and get support directly"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda(
						[]()
						{
							const TSharedPtr<IPlugin> CleanProjectPlugin = IPluginManager::Get().FindPlugin("Tolgee");
							const FString DocsURL = CleanProjectPlugin->GetDescriptor().SupportURL;
							FPlatformProcess::LaunchURL(*DocsURL, nullptr, nullptr);
						}
					))
				);

				MenuBuilder.AddMenuEntry(
					LOCTEXT("ReportIssue", "Report issue"),
					LOCTEXT("ReportIssueTip", "Report an issue on our GitHub"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda(
						[]()
						{
							FPlatformProcess::LaunchURL(TEXT("https://github.com/tolgee/tolgee-unreal/issues"), nullptr, nullptr);
						}
					))
				);

				return MenuBuilder.MakeWidget();
			}
		),
		LOCTEXT("TolgeeDashboardSettingsCombo", "Tolgee Settings"),
		LOCTEXT("TolgeeDashboardSettingsCombo_ToolTip", "Tolgee Dashboard settings"),
		FSlateIcon(FTolgeeStyle::Get().GetStyleSetName(), "Tolgee.Settings"),
		false
	);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTolgeeEditorModule, TolgeeEditor)

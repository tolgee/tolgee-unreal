// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#include "STolgeeEditorTranslationTab.h"

#include <Engine/Engine.h>
#include <Framework/Docking/TabManager.h>
#include <Widgets/Docking/SDockTab.h>

#include "TolgeeEditorIntegrationSubsystem.h"
#include "TolgeeEditorSettings.h"

void STolgeeEditorTranslationTab::Construct(const FArguments& InArgs)
{
	STolgeeTranslationTab::Construct(STolgeeTranslationTab::FArguments());

	FGlobalTabmanager::Get()->OnActiveTabChanged_Subscribe(FOnActiveTabChanged::FDelegate::CreateSP(this, &STolgeeEditorTranslationTab::OnActiveTabChanged));
}

FString STolgeeEditorTranslationTab::GetBaseUrl() const
{
	const UTolgeeEditorSettings* Settings = GetDefault<UTolgeeEditorSettings>();
	return Settings ? Settings->GetBaseUrl() : TEXT("");
}

FString STolgeeEditorTranslationTab::GetApiKey() const
{
	const UTolgeeEditorSettings* Settings = GetDefault<UTolgeeEditorSettings>();
	return Settings ? Settings->ApiKey : TEXT("");
}

TArray<FString> STolgeeEditorTranslationTab::GetProjectIds() const
{
	const UTolgeeEditorSettings* Settings = GetDefault<UTolgeeEditorSettings>();
	return Settings ? Settings->ProjectIds : TArray<FString>();
}

void STolgeeEditorTranslationTab::OnActiveTabChanged(TSharedPtr<SDockTab> PreviouslyActive, TSharedPtr<SDockTab> NewlyActivated)
{
	// When the Tolgee dashboard tab loses focus, refetch in case translations were edited in the browser.
	if (PreviouslyActive.IsValid() && PreviouslyActive->GetLayoutIdentifier().TabType == FName("TolgeeDashboardMenuTab"))
	{
		if (UTolgeeEditorIntegrationSubsystem* Subsystem = GEngine->GetEngineSubsystem<UTolgeeEditorIntegrationSubsystem>())
		{
			Subsystem->ManualFetch();
		}
	}
}

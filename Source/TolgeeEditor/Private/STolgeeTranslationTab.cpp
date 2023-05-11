// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#include "STolgeeTranslationTab.h"

#include <Debug/DebugDrawService.h>
#include <Engine/Canvas.h>
#include <SWebBrowser.h>

#include "TolgeeLocalizationSubsystem.h"
#include "TolgeeLog.h"
#include "TolgeeUtils.h"

namespace
{
	FName STextBlockType(TEXT("STextBlock"));
}

void STolgeeTranslationTab::Construct(const FArguments& InArgs)
{
	// clang-format off
	SDockTab::Construct( SDockTab::FArguments()
		.TabRole(NomadTab)
		.OnTabActivated_Raw(this, &STolgeeTranslationTab::ActiveTab)
		.OnTabClosed_Raw(this, &STolgeeTranslationTab::CloseTab)
	[
		SAssignNew(Browser, SWebBrowser)
		.InitialURL(TolgeeUtils::GetProjectAppUrl())
		.ShowControls(false)
	]);
	// clang-format on
}

void STolgeeTranslationTab::ActiveTab(TSharedRef<SDockTab> DockTab, ETabActivationCause TabActivationCause)
{
	DrawHandle = UDebugDrawService::Register(TEXT("Game"), FDebugDrawDelegate::CreateSP(this, &STolgeeTranslationTab::DebugDrawCallback));
}

void STolgeeTranslationTab::CloseTab(TSharedRef<SDockTab> DockTab)
{
	UDebugDrawService::Unregister(DrawHandle);
}

void STolgeeTranslationTab::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseLeave(MouseEvent);

	GEngine->GetEngineSubsystem<UTolgeeLocalizationSubsystem>()->ManualFetch();
}

void STolgeeTranslationTab::DebugDrawCallback(UCanvas* Canvas, APlayerController* PC)
{
	FSlateApplication& Application = FSlateApplication::Get();
	FWidgetPath WidgetPath = Application.LocateWindowUnderMouse(Application.GetCursorPos(), Application.GetInteractiveTopLevelWindows());
	if (WidgetPath.Widgets.Num() > 0)
	{
		TSharedPtr<SWidget> CurrentHoveredWidget = WidgetPath.GetLastWidget();
		if (CurrentHoveredWidget->GetType() == STextBlockType)
		{
			TSharedPtr<STextBlock> CurrentTextBlock = StaticCastSharedPtr<STextBlock>(CurrentHoveredWidget);
			TSharedPtr<SViewport> GameViewportWidget = GEngine->GameViewport->GetGameViewportWidget();

			// Calculate the Start & End in local space based on widget & parent viewport
			const FGeometry& HoveredGeometry = CurrentHoveredWidget->GetCachedGeometry();
			const FGeometry& ViewportGeometry = GameViewportWidget->GetCachedGeometry();
			// TODO: make this a setting
			const FVector2D Padding = FVector2D{0.2f, 0.2f};
			FVector2D Start = HoveredGeometry.GetAbsolutePositionAtCoordinates(FVector2D::Zero()) - ViewportGeometry.GetAbsolutePositionAtCoordinates(FVector2D::Zero()) + Padding;
			FVector2D End = HoveredGeometry.GetAbsolutePositionAtCoordinates(FVector2D::One()) - ViewportGeometry.GetAbsolutePositionAtCoordinates(FVector2D::Zero()) - Padding;

			FBox2D Box = FBox2D(Start, End);
			// TODO: make this a setting
			const FLinearColor DrawColor = FLinearColor(FColor::Red);
			DrawDebugCanvas2DBox(Canvas, Box, DrawColor);

			// Get information about the currently hovered text
			const FText CurrentText = CurrentTextBlock->GetText();
			const TOptional<FString> Namespace = FTextInspector::GetNamespace(CurrentText);
			const TOptional<FString> Key = FTextInspector::GetKey(CurrentText);

			// Update the browser widget if the current state allows
			const FString NewUrl = FString::Printf(TEXT("%s/translations/single?key=%s&ns=%s"), *TolgeeUtils::GetProjectAppUrl(), *Key.Get(""), *Namespace.Get(""));
			const FString CurrentUrl = Browser->GetUrl();
			if (NewUrl != CurrentUrl && Browser->IsLoaded())
			{
				UE_LOG(LogTolgee, Log, TEXT("CurrentWidget: %s Namespace: %s Key: %s"), *CurrentTextBlock->GetText().ToString(), *Namespace.Get(""), *Key.Get(""));

				Browser->LoadURL(NewUrl);
			}
		}
	}
}

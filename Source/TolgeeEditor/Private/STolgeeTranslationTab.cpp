// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#include "STolgeeTranslationTab.h"

#include <Debug/DebugDrawService.h>
#include <Engine/Canvas.h>
#include <SWebBrowser.h>
#include <DrawDebugHelpers.h>
#include <Misc/EngineVersionComparison.h>

#include "TolgeeLocalizationSubsystem.h"
#include "TolgeeLog.h"
#include "TolgeeUtils.h"

namespace
{
	FName STextBlockType(TEXT("STextBlock"));
}

void STolgeeTranslationTab::Construct(const FArguments& InArgs)
{
	ActiveTab();
	// clang-format off
	SDockTab::Construct(SDockTab::FArguments()
	                    .TabRole(NomadTab)
	                    .OnTabClosed_Raw(this, &STolgeeTranslationTab::CloseTab)
		[
			SAssignNew(Browser, SWebBrowser)
			.InitialURL(TolgeeUtils::GetUrlEndpoint(TEXT("login")))
			.ShowControls(false)
			.ShowErrorMessage(true)
		]);
	// clang-format on

	FGlobalTabmanager::Get()->OnActiveTabChanged_Subscribe(FOnActiveTabChanged::FDelegate::CreateSP(this, &STolgeeTranslationTab::OnActiveTabChanged));
}

void STolgeeTranslationTab::ActiveTab()
{
	DrawHandle = UDebugDrawService::Register(TEXT("Game"), FDebugDrawDelegate::CreateSP(this, &STolgeeTranslationTab::DebugDrawCallback));
}

void STolgeeTranslationTab::CloseTab(TSharedRef<SDockTab> DockTab)
{
	UDebugDrawService::Unregister(DrawHandle);
}

void STolgeeTranslationTab::OnActiveTabChanged(TSharedPtr<SDockTab> PreviouslyActive, TSharedPtr<SDockTab> NewlyActivated)
{
	if (PreviouslyActive == AsShared())
	{
		GEngine->GetEngineSubsystem<UTolgeeLocalizationSubsystem>()->ManualFetch();
	}
}

bool GetTextNamespaceAndKey(const FText InText, FString& OutNamespace, FString& OutKey)
{
	OutNamespace = FTextInspector::GetNamespace(InText).Get(OutNamespace);
	OutKey = FTextInspector::GetKey(InText).Get(OutKey);

	return !OutNamespace.IsEmpty() && !OutKey.IsEmpty();
}

void STolgeeTranslationTab::DebugDrawCallback(UCanvas* Canvas, APlayerController* PC)
{
	if (!GEngine->GameViewport)
	{
		return;
	}

	TSharedPtr<SViewport> GameViewportWidget = GEngine->GameViewport->GetGameViewportWidget();
	if (!GameViewportWidget.IsValid())
	{
		return;
	}

	FSlateApplication& Application = FSlateApplication::Get();
	FWidgetPath WidgetPath = Application.LocateWindowUnderMouse(Application.GetCursorPos(), Application.GetInteractiveTopLevelWindows());

#if UE_VERSION_NEWER_THAN(5, 0, 0)
	const bool bValidHover = WidgetPath.Widgets.Num() > 0 && WidgetPath.ContainsWidget(GameViewportWidget.Get());
#else
	const bool bValidHover = WidgetPath.Widgets.Num() > 0 && GameViewportWidget.IsValid() && WidgetPath.ContainsWidget(GameViewportWidget.ToSharedRef());
#endif

	if (bValidHover)
	{
		TSharedPtr<SWidget> CurrentHoveredWidget = WidgetPath.GetLastWidget();
		if (CurrentHoveredWidget->GetType() == STextBlockType)
		{
			TSharedPtr<STextBlock> CurrentTextBlock = StaticCastSharedPtr<STextBlock>(CurrentHoveredWidget);

			// Calculate the Start & End in local space based on widget & parent viewport
			const FGeometry& HoveredGeometry = CurrentHoveredWidget->GetCachedGeometry();
			const FGeometry& ViewportGeometry = GameViewportWidget->GetCachedGeometry();

			// TODO: make this a setting
			const FVector2D Padding = FVector2D{0.2f, 0.2f};

			const FVector2D UpperLeft = {0, 0};
			const FVector2D LowerRight = {1, 1};

			FVector2D Start = HoveredGeometry.GetAbsolutePositionAtCoordinates(UpperLeft) - ViewportGeometry.GetAbsolutePositionAtCoordinates(UpperLeft) + Padding;
			FVector2D End = HoveredGeometry.GetAbsolutePositionAtCoordinates(LowerRight) - ViewportGeometry.GetAbsolutePositionAtCoordinates(UpperLeft) - Padding;

			FBox2D Box = FBox2D(Start, End);
			// TODO: make this a setting
			const FLinearColor DrawColor = FLinearColor(FColor::Red);
			DrawDebugCanvas2DBox(Canvas, Box, DrawColor);

			// Get information about the currently hovered text
			const FText CurrentText = CurrentTextBlock->GetText();

			const TOptional<FString> CurrentNamespace = FTextInspector::GetNamespace(CurrentText);
			const TOptional<FString> CurrentKey = FTextInspector::GetKey(CurrentText);

			FString Namespace, Key;

			bool bResolved = GetTextNamespaceAndKey(CurrentText, Namespace, Key);
			if (!bResolved)
			{
				// If we failed, maybe we can resolve the initial text that was formatted instead?

				TArray<FHistoricTextFormatData> CurrentFormatData;
				FTextInspector::GetHistoricFormatData(CurrentText, CurrentFormatData);

				if (!CurrentFormatData.IsEmpty())
				{
					GetTextNamespaceAndKey(CurrentFormatData[CurrentFormatData.Num() - 1].SourceFmt.GetSourceText(), Namespace, Key);
				}
			}

			// Update the browser widget if the current state allows
			const FString EndPoint = FString::Printf(TEXT("translations/single?key=%s&ns=%s"), *Key, *Namespace);
			const FString NewUrl = TolgeeUtils::GetProjectUrlEndpoint(EndPoint);
			const FString CurrentUrl = Browser->GetUrl();
			if (NewUrl != CurrentUrl && Browser->IsLoaded())
			{
				UE_LOG(LogTolgee, Log, TEXT("CurrentWidget: %s Namespace: %s Key: %s"), *CurrentTextBlock->GetText().ToString(), *Namespace, *Key);

				Browser->LoadURL(NewUrl);
			}
		}
	}
}
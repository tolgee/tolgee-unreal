// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#include "STolgeeTranslationTab.h"

#include <Debug/DebugDrawService.h>
#include <DrawDebugHelpers.h>
#include <HttpModule.h>
#include <Interfaces/IHttpResponse.h>
#include <Internationalization/TextNamespaceUtil.h>
#include <Misc/EngineVersionComparison.h>
#include <PlatformHttp.h>
#include <SWebBrowser.h>

#include "TolgeeEditorIntegrationSubsystem.h"
#include "TolgeeEditorSettings.h"
#include "TolgeeLog.h"

namespace
{
	FName STextBlockType(TEXT("STextBlock"));
}

void STolgeeTranslationTab::Construct(const FArguments& InArgs)
{
	const UTolgeeEditorSettings* Settings = GetDefault<UTolgeeEditorSettings>();
	const FString LoginUrl = FString::Printf(TEXT("%s/login"), *Settings->ApiUrl);

	DrawHandle = UDebugDrawService::Register(TEXT("Game"), FDebugDrawDelegate::CreateSP(this, &STolgeeTranslationTab::DebugDrawCallback));

	// clang-format off
	SDockTab::Construct( SDockTab::FArguments()
		.TabRole(NomadTab)
		.OnTabClosed_Raw(this, &STolgeeTranslationTab::CloseTab)
	[
		SAssignNew(Browser, SWebBrowser)
		.InitialURL(LoginUrl)
		.ShowControls(false)
		.ShowErrorMessage(true)
	]);
	// clang-format on

	FGlobalTabmanager::Get()->OnActiveTabChanged_Subscribe(FOnActiveTabChanged::FDelegate::CreateSP(this, &STolgeeTranslationTab::OnActiveTabChanged));
}

void STolgeeTranslationTab::CloseTab(TSharedRef<SDockTab> DockTab)
{
	UDebugDrawService::Unregister(DrawHandle);
}

void STolgeeTranslationTab::OnActiveTabChanged(TSharedPtr<SDockTab> PreviouslyActive, TSharedPtr<SDockTab> NewlyActivated)
{
	if (PreviouslyActive == AsShared())
	{
		GEngine->GetEngineSubsystem<UTolgeeEditorIntegrationSubsystem>()->ManualFetch();
	}
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
			const TOptional<FString> Namespace = FTextInspector::GetNamespace(CurrentText);
			const TOptional<FString> Key = FTextInspector::GetKey(CurrentText);

			// Update the browser widget if we have valid data and the URL is different
			if (Namespace && Key)
			{
				const FString CleanNamespace = TextNamespaceUtil::StripPackageNamespace(Namespace.GetValue());

				// NOTE: This might look odd, but we need to mirror the id's used internally by Unreal as those are used for importing the key.
				const FString TolgeeKeyId = FPlatformHttp::UrlEncode(FString::Printf(TEXT("%s,%s"), *CleanNamespace, *Key.GetValue()));

				ShowWidgetForAsync(TolgeeKeyId);
			}
		}
	}
}

void STolgeeTranslationTab::ShowWidgetForAsync(const FString& TolgeeKeyId)
{
	AsyncTask(ENamedThreads::AnyBackgroundHiPriTask,
	          [this, TolgeeKeyId]()
	          {
		          ShowWidgetFor(TolgeeKeyId);
	          });
}

void STolgeeTranslationTab::ShowWidgetFor(const FString& TolgeeKeyId)
{
	if (bRequestInProgress)
	{
		return;
	}

	TGuardValue ScopedRequestProgress(bRequestInProgress, true);

	const FString ProjectId = FindProjectIdFor(TolgeeKeyId);
	if (ProjectId.IsEmpty())
	{
		UE_LOG(LogTolgee, Warning, TEXT("No project found for key '%s'"), *TolgeeKeyId);
		return;
	}

	const UTolgeeEditorSettings* Settings = GetDefault<UTolgeeEditorSettings>();

	const FString NewUrl = FString::Printf(TEXT("%s/projects/%s/translations/single?key=%s"), *Settings->ApiUrl, *ProjectId, *TolgeeKeyId);
	const FString CurrentUrl = Browser->GetUrl();

	if (NewUrl != CurrentUrl && Browser->IsLoaded())
	{
		UE_LOG(LogTolgee, Log, TEXT("CurrentWidget displayed from %s"), *NewUrl);

		Browser->LoadURL(NewUrl);
	}
}

FString STolgeeTranslationTab::FindProjectIdFor(const FString& TolgeeKeyId) const
{
	const UTolgeeEditorSettings* Settings = GetDefault<UTolgeeEditorSettings>();

	TMap<FString, FHttpRequestPtr> PendingRequests;
	for (const FString& ProjectId : Settings->ProjectIds)
	{
		const FString RequestUrl = FString::Printf(TEXT("%s/v2/projects/%s/translations?filterKeyName=%s"), *Settings->ApiUrl, *ProjectId, *TolgeeKeyId);

		const FHttpRequestRef HttpRequest = FHttpModule::Get().CreateRequest();
		HttpRequest->SetVerb("GET");
		HttpRequest->SetHeader(TEXT("X-API-Key"), Settings->ApiKey);
		HttpRequest->SetURL(RequestUrl);
		HttpRequest->ProcessRequest();

		PendingRequests.Add(ProjectId, HttpRequest);
	}

	while (!PendingRequests.IsEmpty())
	{
		FPlatformProcess::Sleep(0.1f);

		for (auto RequestIt = PendingRequests.CreateIterator(); RequestIt; ++RequestIt)
		{
			const TPair<FString, FHttpRequestPtr> Pair = *RequestIt;
			const FHttpRequestPtr Request = Pair.Value;
			const FString& ProjectId = Pair.Key;

			if (EHttpRequestStatus::IsFinished(Request->GetStatus()))
			{
				const FHttpResponsePtr Response = Request->GetResponse();
				const FString ResponseContent = Response.IsValid() ? Response->GetContentAsString() : FString();

				const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(ResponseContent);
				TSharedPtr<FJsonObject> JsonObject;

				if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
				{
					const TSharedPtr<FJsonObject> Embedded = JsonObject->GetObjectField(TEXT("_embedded"));
					const TArray<TSharedPtr<FJsonValue>> Keys = Embedded->GetArrayField(TEXT("keys"));
					if (!Keys.IsEmpty())
					{
						return ProjectId;
					}
				}

				RequestIt.RemoveCurrent();
			}
		}
	}

	return {};
}
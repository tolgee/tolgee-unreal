// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#include "STolgeeTranslationTab.h"

#include <Debug/DebugDrawService.h>
#include <DrawDebugHelpers.h>
#include <HttpModule.h>
#include <Interfaces/IHttpResponse.h>
#include <Internationalization/TextNamespaceUtil.h>
#include <Engine/Engine.h>
#include <Engine/GameViewportClient.h>
#include <Framework/Application/SlateApplication.h>
#include <Widgets/Text/STextBlock.h>
#include <Widgets/SViewport.h>
#include <Serialization/JsonReader.h>
#include <Serialization/JsonSerializer.h>
#include <Misc/EngineVersionComparison.h>
#include <PlatformHttp.h>
#include <SWebBrowser.h>

#include "TolgeeLog.h"
#include "TolgeeUtils.h"

namespace
{
	FName STextBlockType(TEXT("STextBlock"));

	// Text might be HitTestInvisible (button/border swallows hits) so we need to walk the subtree to find it.
	TSharedPtr<STextBlock> FindTextBlockUnderCursor(const TSharedRef<SWidget>& Root, const FVector2D& Cursor)
	{
		TArray<TSharedRef<SWidget>> ToVisit;
		ToVisit.Push(Root);

		while (ToVisit.Num() > 0)
		{
			const TSharedRef<SWidget> Widget = ToVisit.Pop();
			if (!Widget->GetCachedGeometry().IsUnderLocation(Cursor))
			{
				continue;
			}

			if (Widget->GetType() == STextBlockType)
			{
				return StaticCastSharedRef<STextBlock>(Widget);
			}

			if (FChildren* Children = Widget->GetChildren())
			{
				for (int32 Index = 0; Index < Children->Num(); ++Index)
				{
					ToVisit.Push(Children->GetChildAt(Index));
				}
			}
		}

		return nullptr;
	}
}

void STolgeeTranslationTab::Construct(const FArguments& InArgs)
{
	const FString LoginUrl = FString::Printf(TEXT("%s/login"), *GetBaseUrl());

	DrawHandle = UDebugDrawService::Register(TEXT("Game"), FDebugDrawDelegate::CreateSP(this, &STolgeeTranslationTab::DebugDrawCallback));

	// clang-format off
	ChildSlot
	[
		SAssignNew(Browser, SWebBrowser)
		.InitialURL(LoginUrl)
		.ShowControls(false)
		.ShowErrorMessage(true)
	];
	// clang-format on
}

STolgeeTranslationTab::~STolgeeTranslationTab()
{
	UDebugDrawService::Unregister(DrawHandle);
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
	const FVector2D Cursor = Application.GetCursorPos();
	const FWidgetPath WidgetPath = Application.LocateWindowUnderMouse(Cursor, Application.GetInteractiveTopLevelWindows());

#if UE_VERSION_NEWER_THAN(5, 0, 0)
	const bool bValidHover = WidgetPath.Widgets.Num() > 0 && WidgetPath.ContainsWidget(GameViewportWidget.Get());
#else
	const bool bValidHover = WidgetPath.Widgets.Num() > 0 && GameViewportWidget.IsValid() && WidgetPath.ContainsWidget(GameViewportWidget.ToSharedRef());
#endif

	if (bValidHover)
	{
		TSharedPtr<STextBlock> CurrentTextBlock = FindTextBlockUnderCursor(WidgetPath.GetLastWidget(), Cursor);
		if (CurrentTextBlock.IsValid())
		{
			// Calculate the Start & End in local space based on widget & parent viewport
			const FGeometry& HoveredGeometry = CurrentTextBlock->GetCachedGeometry();
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

	const FString NewUrl = FString::Printf(TEXT("%s/projects/%s/translations/single?key=%s"), *GetBaseUrl(), *ProjectId, *TolgeeKeyId);
	const FString CurrentUrl = Browser->GetUrl();

	if (NewUrl != CurrentUrl && Browser->IsLoaded())
	{
		UE_LOG(LogTolgee, Log, TEXT("CurrentWidget displayed from %s"), *NewUrl);

		Browser->LoadURL(NewUrl);
	}
}

FString STolgeeTranslationTab::FindProjectIdFor(const FString& TolgeeKeyId) const
{
	TMap<FString, FHttpRequestPtr> PendingRequests;
	for (const FString& ProjectId : GetProjectIds())
	{
		const FString RequestUrl = FString::Printf(TEXT("%s/v2/projects/%s/translations?filterKeyName=%s"), *GetBaseUrl(), *ProjectId, *TolgeeKeyId);

		FHttpRequestRef HttpRequest = FHttpModule::Get().CreateRequest();
		HttpRequest->SetVerb("GET");
		HttpRequest->SetURL(RequestUrl);
		HttpRequest->SetHeader(TEXT("X-API-Key"), GetApiKey());
		TolgeeUtils::AddSdkHeaders(HttpRequest);

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
// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#include "STolgeeCommandTranslationTab.h"

static FAutoConsoleCommandWithWorldAndArgs GTolgeeOpenRuntimeTranslationWindow(
	TEXT("Tolgee.OpenRuntimeTranslationWindow"),
	TEXT("Opens the Tolgee in-context translation tab in a separated window."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& Args, UWorld* World)
		{
			const FString BaseUrl = Args.IsValidIndex(0) ? Args[0].TrimQuotes() : TEXT("");
			const FString ApiKey = Args.IsValidIndex(1) ? Args[1].TrimQuotes() : TEXT("");
			const FString Ids = Args.IsValidIndex(2) ? Args[2].TrimQuotes() : TEXT("");

			TArray<FString> ProjectIds;
			Ids.ParseIntoArray(ProjectIds, TEXT(","), true);

			const TSharedRef<SWindow> Window = SNew(SWindow)
				.Title(INVTEXT("Tolgee In-Context Translation"))
				.ClientSize(FVector2D(1000.0f, 700.0f));

			const TSharedRef<STolgeeCommandTranslationTab> TranslationTab = SNew(STolgeeCommandTranslationTab)
				.BaseUrl(BaseUrl)
				.ApiKey(ApiKey)
				.ProjectIds(ProjectIds);

			Window->SetContent(TranslationTab);
			FSlateApplication::Get().AddWindow(Window);
		}
));

void STolgeeCommandTranslationTab::Construct(const FArguments& InArgs)
{
	BaseUrl = InArgs._BaseUrl;
	ApiKey = InArgs._ApiKey;
	ProjectIds = InArgs._ProjectIds;

	STolgeeTranslationTab::Construct(STolgeeTranslationTab::FArguments());
}

FString STolgeeCommandTranslationTab::GetBaseUrl() const
{
	return BaseUrl;
}

FString STolgeeCommandTranslationTab::GetApiKey() const
{
	return ApiKey;
}

TArray<FString> STolgeeCommandTranslationTab::GetProjectIds() const
{
	return ProjectIds;
}

// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#include "TolgeeStyle.h"

#include <Interfaces/IPluginManager.h>
#include <Styling/ISlateStyle.h>
#include <Styling/SlateStyle.h>
#include <Styling/SlateStyleRegistry.h>

TSharedPtr<FSlateStyleSet> FTolgeeStyle::StyleSet = nullptr;

TSharedPtr<class ISlateStyle> FTolgeeStyle::Get()
{
	return StyleSet;
}

FName FTolgeeStyle::GetStyleSetName()
{
	static FName TolgeeStyleName(TEXT("TolgeeStyle"));
	return TolgeeStyleName;
}

void FTolgeeStyle::Initialize()
{
	// Only register once
	if (StyleSet.IsValid())
	{
		return;
	}

	StyleSet = MakeShared<FSlateStyleSet>(GetStyleSetName());
	FString Root = IPluginManager::Get().FindPlugin(TEXT("Tolgee"))->GetBaseDir() / TEXT("Resources");

	StyleSet->Set("Tolgee.Settings", new FSlateImageBrush(FName(*(Root + "/Settings-Icon.png")), FVector2D(64, 64)));

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
};


void FTolgeeStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}

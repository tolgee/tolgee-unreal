// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#include "TolgeeStyle.h"

#include <Interfaces/IPluginManager.h>
#include <Styling/ISlateStyle.h>
#include <Styling/SlateStyle.h>
#include <Styling/SlateStyleRegistry.h>
#include <Styling/SlateStyleMacros.h>

FName FTolgeeStyle::StyleName("TolgeeStyle");
TUniquePtr<FTolgeeStyle> FTolgeeStyle::Inst(nullptr);

const FName& FTolgeeStyle::GetStyleSetName() const
{
	return StyleName;
}

const FTolgeeStyle& FTolgeeStyle::Get()
{
	ensure(Inst.IsValid());
	return *Inst.Get();
}

void FTolgeeStyle::Initialize()
{
	if (!Inst.IsValid())
	{
		Inst = TUniquePtr<FTolgeeStyle>(new FTolgeeStyle);
	}
}

void FTolgeeStyle::Shutdown()
{
	if (Inst.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*Inst.Get());
		Inst.Reset();
	}
}

FTolgeeStyle::FTolgeeStyle() : FSlateStyleSet(StyleName)
{

	SetParentStyleName(FAppStyle::GetAppStyleSetName());

	FSlateStyleSet::SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("Tolgee"))->GetBaseDir() / TEXT("Resources"));

	{
		Set("Tolgee.Settings", new IMAGE_BRUSH("Settings-Icon", FVector2D(64, 64)));
	}

	FSlateStyleRegistry::RegisterSlateStyle(*this);
}

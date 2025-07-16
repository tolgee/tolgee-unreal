// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#include "TolgeeEditorSettings.h"

FString UTolgeeEditorSettings::GetBaseUrl() const
{
	return ApiUrl.EndsWith(TEXT("/")) ? ApiUrl.LeftChop(1) : ApiUrl;
}

FName UTolgeeEditorSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>

namespace TolgeeUtils
{
	/**
	 * @brief Utility to produce a hash for a string (as used by SourceStringHash)
	 */
	FString TOLGEE_API GetTranslationHash(const FString& Translation);
	/**
	 * @brief Constructs an endpoint by appending query parameters to a base url
	 */
	FString TOLGEE_API AppendQueryParameters(const FString& BaseUrl, const TArray<FString>& Parameters);
	/**
	 * @brief Constructs the Tolgee app url based on our current settings
	 */
	FString TOLGEE_API GetProjectAppUrl();

	inline FString DefaultTextPrefix = TEXT("OriginalText:");
	inline FString KeyHashPrefix = TEXT("OriginalHash:");
} // namespace TolgeeUtils

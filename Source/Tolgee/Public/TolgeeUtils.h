// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>

namespace TolgeeUtils
{
	/**
	 * @brief Constructs an endpoint by appending query parameters to a base url
	 */
	FString TOLGEE_API AppendQueryParameters(const FString& BaseUrl, const TArray<FString>& Parameters);
	/**
	 * @brief Sdk type of the Tolgee integration
	 */
	FString TOLGEE_API GetSdkType();
	/**
	 * @brief Sdk version of the Tolgee integration
	 */
	FString TOLGEE_API GetSdkVersion();
} // namespace TolgeeUtils

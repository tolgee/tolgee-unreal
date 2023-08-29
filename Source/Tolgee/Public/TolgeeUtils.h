// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>

namespace TolgeeUtils
{
	/**
	 * @brief Utility to produce a hash for a string (as used by SourceStringHash)
	 */
	uint32 TOLGEE_API GetTranslationHash(const FString& Translation);
	/**
	 * @brief Constructs an endpoint by appending query parameters to a base url
	 */
	FString TOLGEE_API AppendQueryParameters(const FString& BaseUrl, const TArray<FString>& Parameters);
	/**
	 * @brief Creates the endpoint url for an API call based on an endpoint path and the currently assigned ApiUrl
	 */
	FString TOLGEE_API GetUrlEndpoint(const FString& EndPoint);
	/**
	 * @brief Creates the endpoint url for a project API call based on an endpoint path and the currently assigned ApiUrl and ProjectId
	 * @note Can be used with an empty endpoint path to get the link to the web dashboard.
	 */
	FString TOLGEE_API GetProjectUrlEndpoint(const FString& EndPoint = TEXT(""));
	/**
	 * @brief Sdk type of the Tolgee integration
	 */
	FString TOLGEE_API GetSdkType();
	/**
	 * @brief Sdk version of the Tolgee integration
	 */
	FString TOLGEE_API GetSdkVersion();

	inline FString KeyHashPrefix = TEXT("OriginalHash:");
} // namespace TolgeeUtils

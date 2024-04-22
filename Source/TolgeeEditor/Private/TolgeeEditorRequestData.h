// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>

#include "TolgeeEditorRequestData.generated.h"

/**
 * @brief Payload for the key purging request
 */
USTRUCT()
struct FKeysDeletePayload
{
	GENERATED_BODY()

	/**
	 * @brief Ids of the keys we will be deleting
	 */
	UPROPERTY()
	TArray<int64> Ids;
};

USTRUCT()
struct FKeyUpdatePayload
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString Namespace;

	UPROPERTY()
	TArray<FString> Tags;
};


/**
 * @brief Representation of a Tolgee upload key
 */
USTRUCT()
struct FImportKeyItem
{
	GENERATED_BODY()

	/**
	 * @brief Key's name in the namespace
	 */
	UPROPERTY()
	FString Name;
	/**
	 * @brief Namespace this key is part of
	 */
	UPROPERTY()
	FString Namespace;
	/**
	 * @brief Tags associated with this key
	 */
	UPROPERTY()
	TArray<FString> Tags;
};

/**
 * @brief Representation of a local localization key
 */
USTRUCT()
struct FLocalizationKey
{
	GENERATED_BODY()

	/**
	 * @brief Key's name in the namespace
	 */
	UPROPERTY()
	FString Key;
	/**
	 * @brief Namespace this key is part of
	 */
	UPROPERTY()
	FString Namespace;
	/**
	 * @brief Default text associcated with this key if no translation data is found
	 * @note also known as "intended usage"
	 */
	UPROPERTY()
	FString DefaultText;
};

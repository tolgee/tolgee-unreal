// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>

#include "TolgeeRuntimeRequestData.generated.h"

/**
 * @brief Representation of a Tolgee tag
 * @see https://tolgee.io/platform/translation_keys/tags
 */
USTRUCT()
struct TOLGEE_API FTolgeeKeyTag
{
	GENERATED_BODY()
	/**
	 * @brief Unique Id of the tag
	 */
	UPROPERTY()
	int64 Id;
	/**
	 * @brief Display name of the tag
	 */
	UPROPERTY()
	FString Name;
};

/**
 * @brief Representation of a Tolgee translation
 */
USTRUCT()
struct TOLGEE_API FTolgeeTranslation
{
	GENERATED_BODY()
	/**
	 * @brief Unique Id of the translation
	 */
	UPROPERTY()
	int64 Id;
	/**
	 * @brief Translated text
	 */
	UPROPERTY()
	FString Text;
};

/**
 * @brief Representation of a Tolgee key
 */
USTRUCT()
struct TOLGEE_API FTolgeeKeyData
{
	GENERATED_BODY()
	/**
	 * @brief Helper constructor to generate data from a JSON payload
	 */
	static TOptional<FTolgeeKeyData> FromJson(TSharedPtr<FJsonValue> InValue);
	/**
	 * @brief Unique Id of the key
	 */
	UPROPERTY()
	int64 KeyId;
	/**
	 * @brief Name of the key
	 */
	UPROPERTY()
	FString KeyName;
	/**
	 * @brief Unique Id of the key's namespace
	 */
	UPROPERTY()
	int64 KeyNamespaceId;
	/**
	 * @brief Display name of the key's namespace
	 */
	UPROPERTY()
	FString KeyNamespace;
	/**
	 * @brief Tags assigned to the current key
	 */
	UPROPERTY()
	TArray<FTolgeeKeyTag> KeyTags;
	/**
	 * @brief Map pairing available locales with their respective translation information
	 */
	TMap<FString, FTolgeeTranslation> Translations;
	/**
	 * @brief Gets all the available locales we have valid translations for
	 */
	TArray<FString> GetAvailableLanguages() const;
	/**
	 * @brief Returns the original key's hash based on the current assigned tags. Or 0 if no valid tag was found.
	 */
	uint32 GetKeyHash() const;

private:
	/**
	 * @brief Helper method to get the value of a specific tag based on the tag's Prefix
	 * @param Prefix Key used inside the tag (e.g.: "Hash:" or "Text:"
	 * @return Rest of the tag's string after the prefix is removed.
	 */
	FString GetTagValue(const FString& Prefix) const;
};

/**
 * Representation of a successfully localized key
 */
USTRUCT()
struct FLocalizedKey
{
	GENERATED_BODY()

	/**
	 * Name of the key
	 */
	UPROPERTY()
	FString Name;
	/**
	 * Namespace of the key
	 */
	UPROPERTY()
	FString Namespace;
	/**
	 * Unique hash of the key (calculated based on the source text)
	 */
	UPROPERTY()
	uint32 Hash;
	/**
	 * Language this key is translated for
	 */
	UPROPERTY()
	FString Locale;
	/**
	 * Translation for the specified locale
	 */
	UPROPERTY()
	FString Translation;
};

/**
 * Representation of a translation dictionary (multiple keys for multiple languages)
 */
USTRUCT()
struct FLocalizedDictionary
{
	GENERATED_BODY()

	/**
	 * Localization keys for all languages
	 */
	UPROPERTY()
	TArray<FLocalizedKey> Keys;
};
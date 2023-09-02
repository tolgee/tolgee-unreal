// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#include "TolgeeRuntimeRequestData.h"

#include <JsonObjectConverter.h>

#include "TolgeeUtils.h"

TOptional<FTolgeeKeyData> FTolgeeKeyData::FromJson(TSharedPtr<FJsonValue> InValue)
{
	if (!InValue.IsValid())
	{
		return {};
	}

	FTolgeeKeyData OutData;
	const TSharedRef<FJsonObject> TranslatedKeyObject = InValue->AsObject().ToSharedRef();
	if (!FJsonObjectConverter::JsonObjectToUStruct(TranslatedKeyObject, &OutData))
	{
		return {};
	}

	const TSharedPtr<FJsonObject> TranslationJsonData = TranslatedKeyObject->GetObjectField(TEXT("translations"));
	for (const auto& TranslationData : TranslationJsonData->Values)
	{
		TSharedPtr<FJsonValue> Translation = TranslationData.Value;
		if (Translation.IsValid())
		{
			FTolgeeTranslation CurrentLanguage;
			FJsonObjectConverter::JsonObjectToUStruct(Translation->AsObject().ToSharedRef(), &CurrentLanguage);
			OutData.Translations.Emplace(TranslationData.Key, CurrentLanguage);
		}
	}

	return OutData;
}

TArray<FString> FTolgeeKeyData::GetAvailableLanguages() const
{
	TArray<FString> Result;
	Translations.GenerateKeyArray(Result);
	return Result;
}

uint32 FTolgeeKeyData::GetKeyHash() const
{
	const FString KeyHashValue = GetTagValue(TolgeeUtils::KeyHashPrefix);

	const uint32 KeyHash = static_cast<uint32>(FCString::Atoi64(*KeyHashValue));
	return KeyHash;
}

FString FTolgeeKeyData::GetTagValue(const FString& Prefix) const
{
	const FTolgeeKeyTag* FoundTag = KeyTags.FindByPredicate(
		[Prefix](const FTolgeeKeyTag& TagText)
		{
			return TagText.Name.Contains(Prefix);
		}
	);

	if (!FoundTag)
	{
		return {};
	}

	FString TagValue = FoundTag->Name;
	TagValue.RemoveFromStart(Prefix);
	return TagValue;
}
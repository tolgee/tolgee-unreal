// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#pragma once

#include "STolgeeTranslationTab.h"

class STolgeeCommandTranslationTab : public STolgeeTranslationTab
{
public:
	SLATE_BEGIN_ARGS(STolgeeCommandTranslationTab) { }
		SLATE_ARGUMENT(FString, BaseUrl)
		SLATE_ARGUMENT(FString, ApiKey)
		SLATE_ARGUMENT(TArray<FString>, ProjectIds)
	SLATE_END_ARGS()

	/**
	* @brief Constructs the translation dashboard widget for the console command
	*/
	void Construct(const FArguments& InArgs);

protected:
	//~Begin STolgeeTranslationTab interface
	virtual FString GetBaseUrl() const override;
	virtual FString GetApiKey() const override;
	virtual TArray<FString> GetProjectIds() const override;
	//~End STolgeeTranslationTab interface

private:
	FString BaseUrl;
	FString ApiKey;
	TArray<FString> ProjectIds;
};

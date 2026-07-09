// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#pragma once

#include "STolgeeTranslationTab.h"

class SDockTab;

class STolgeeEditorTranslationTab : public STolgeeTranslationTab
{
public:
	SLATE_BEGIN_ARGS(STolgeeEditorTranslationTab) { }
	SLATE_END_ARGS()

	/**
	* @brief Constructs the translation dashboard widget for the editor
	*/
	void Construct(const FArguments& InArgs);
protected:
	//~Begin STolgeeTranslationTab interface
	virtual FString GetBaseUrl() const override;
	virtual FString GetApiKey() const override;
	virtual TArray<FString> GetProjectIds() const override;
	//~End STolgeeTranslationTab interface

private:
	/**
	 * @brief Callback executed when the active tab is changed
	 */
	void OnActiveTabChanged(TSharedPtr<SDockTab> PreviouslyActive, TSharedPtr<SDockTab> NewlyActivated);
};

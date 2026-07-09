// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#pragma once

#include <Widgets/SCompoundWidget.h>

class UCanvas;
class SWebBrowser;

/**
 * Widget which displays the web translation page inside Unreal when a Textblock is hovered
 */
class TOLGEE_API STolgeeTranslationTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STolgeeTranslationTab)
		{
		}

	SLATE_END_ARGS()

	/**
	 * @brief Constructs the translation dashboard widget
	 */
	void Construct(const FArguments& InArgs);

	virtual ~STolgeeTranslationTab() override;
private:
	/**
	 * @brief Callback executed when the active tab is changed
	 */
	void OnActiveTabChanged(TSharedPtr<SDockTab> NewlyActivated, TSharedPtr<SDockTab> PreviouslyActive);
	/**
	 * @brief Callback executed when the debug service wants to draw on screen
	 */
	void DebugDrawCallback(UCanvas* Canvas, APlayerController* PC);
	/**
	 * Runs an asynchronous request to find the matching Url to the given TolgeeKeyId.
	 */
	void ShowWidgetForAsync(const FString& TolgeeKeyId);
	/**
	 * Updates the browser widget to display the Tolgee web editor for the given key.
	 */
	void ShowWidgetFor(const FString& TolgeeKeyId);
	/**
	 * Finds the project id for the given TolgeeKeyId.
	 */
	FString FindProjectIdFor(const FString& TolgeeKeyId) const;
	/**
	 * @brief Handle for the registered debug callback
	 */
	FDelegateHandle DrawHandle;
	/**
	 * @brief Browser widget used to display tolgee web editor
	 */
	TSharedPtr<SWebBrowser> Browser;
	/**
	 * Flag used to prevent multiple requests from being sent at the same time.
	 */
	TAtomic<bool> bRequestInProgress = false;
};
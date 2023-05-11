// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Widgets/SCompoundWidget.h>

class SWebBrowser;

/**
 * Nomad Tab used to display the web translation widget inside Unreal when an Textblock is hovered
 */
class STolgeeTranslationTab : public SDockTab
{
public:
	SLATE_BEGIN_ARGS(STolgeeTranslationTab) {}
	SLATE_END_ARGS()
	/**
	 * @brief Constructs the translation dashboard widget
	 */
	void Construct(const FArguments& InArgs);

private:
	// Begin SDockTab interface
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	// End SDockTab interface

	/**
	 * @brief Callback executed when the DockTab is activated
	 */
	void ActiveTab(TSharedRef<SDockTab> DockTab, ETabActivationCause TabActivationCause);
	/**
	 * @brief Callback executed when the DockTab is deactivated
	 */
	void CloseTab(TSharedRef<SDockTab> DockTab);
	/**
	 * @brief Callback executed when the debug service wants to draw on screen
	 */
	void DebugDrawCallback(UCanvas* Canvas, APlayerController* PC);
	/**
	 * @brief Handle for the registered debug callback
	 */
	FDelegateHandle DrawHandle;
	/**
	 * @brief Browser widget used to display tolgee web editor
	 */
	TSharedPtr<SWebBrowser> Browser;
};

// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#pragma once

#include <Modules/ModuleManager.h>

/**
 * @brief TolgeeEditor module is responsible for integrating translation features into the editor (tab to allow in-editor translation, gather & upload local keys)
 */
class FTolgeeEditorModule : public IModuleInterface
{
	// Begin IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	// End IModuleInterface interface

	/**
	 * @brief Registers the SlateStyle for custom editor styling
	 */
	void RegisterStyle();
	/**
	 * @brief Unregisters the SlateStyle for custom editor styling
	 */
	void UnregisterStyle();
	/**
	 * @brief Register the custom nomad tab spawn for the dashboard
	 */
	void RegisterWindowExtension();
	/**
	 * @brief Unregister the custom nomad tab spawn for the dashboard
	 */
	void UnregisterWindowExtension();
	/**
	 * @brief Registers all the extensions for the Toolbar menu
	 */
	void RegisterToolbarExtension();
	/**
	 * @brief Unregisters all the extensions for the Toolbar menu
	 */
	void UnregisterToolbarExtension();
	/**
	 * @brief Fills the toolbar with all the buttons accessible to the user
	 */
	void ExtendToolbar(FToolBarBuilder& Builder);
	/**
	 * @brief The extender passed to the level editor to extend it's toolbar
	 */
	TSharedPtr<FExtender> ToolbarExtender;
};

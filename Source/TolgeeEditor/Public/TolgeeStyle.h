// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>

/**
 * @brief Declares the Tolgee extension visual style.
 */
class FTolgeeStyle : public FSlateStyleSet
{
public:
	
	/**
	 * @brief Access the singleton instance for this SlateStyle
	 */
	static const FTolgeeStyle& Get();
	/**
	 * @brief Creates and Registers the plugin SlateStyle
	 */
	static void Initialize();
	/**
	 * @brief Unregisters the plugin SlateStyle
	 */
	static void Shutdown();

	// Begin FSlateStyleSet Interface
	virtual const FName& GetStyleSetName() const override;
	// End FSlateStyleSet Interface

private:
	FTolgeeStyle();

	/**
	 * @brief Unique name for this SlateStyle
	 */
	static FName StyleName;
	/**
	 * @brief Singleton instances of this SlateStyle.
	 */
	static TUniquePtr<FTolgeeStyle> Inst;
};

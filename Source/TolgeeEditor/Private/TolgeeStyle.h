// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>

/**
 * @brief Declares the Tolgee extension visual style.
 */
class FTolgeeStyle
{
public:
	/**
	 * @brief Initializes the SlateStyle
	 */
	static void Initialize();
	/**
	 * @brief Deinitializes the SlateStyle
	 */
	static void Shutdown();

	/**
	 * @brief Singleton getter for the SlateStyle class
	 */
	static TSharedPtr<class ISlateStyle> Get();
	/**
	 * @brief Convince getter for name of the current SlateStyle
	 */
	static FName GetStyleSetName();

private:
	/**
	 * @brief Singleton instance of the SlateStyle
	 */
	static TSharedPtr<class FSlateStyleSet> StyleSet;
};

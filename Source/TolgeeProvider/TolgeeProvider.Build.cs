// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

using UnrealBuildTool;

public class TolgeeProvider : ModuleRules
{
	public TolgeeProvider(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"DeveloperToolSettings",
				"DeveloperSettings",
				"Engine",
				"HTTP",
				"Json",
				"Localization",
				"LocalizationCommandletExecution",
				"LocalizationService",
				"Slate",
				"SlateCore",
				
				"Tolgee",
				"TolgeeEditor",
			}
		);
	}
}
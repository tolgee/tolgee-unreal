// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

using UnrealBuildTool;

public class Tolgee : ModuleRules
{
	public Tolgee(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", 
				"CoreUObject",
				"DeveloperSettings", 
				"Engine",
				"HTTP",
				"Localization",
				"Json",
				"JsonUtilities",
				"Projects",
			}
		);

		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.Add("UnrealEd");
		}
	}
}

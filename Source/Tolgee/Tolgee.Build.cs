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
				"Json",
				"JsonUtilities",
				"Projects",
			}
		);

		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.Add("UnrealEd");
		}

		bool bLocalizationModuleAvailable = !Target.bIsEngineInstalled || Target.Configuration != UnrealTargetConfiguration.Shipping;
		if (bLocalizationModuleAvailable)
		{
			PublicDefinitions.Add("WITH_LOCALIZATION_MODULE=1");
			PublicDependencyModuleNames.Add("Localization");
		}
		else
		{
			PublicDefinitions.Add("WITH_LOCALIZATION_MODULE=0");
		}
	}
}

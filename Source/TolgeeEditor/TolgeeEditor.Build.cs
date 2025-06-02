// Copyright (c) Tolgee 2022-2025. All Rights Reserved.

using UnrealBuildTool;

public class TolgeeEditor : ModuleRules
{
	public TolgeeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"DeveloperSettings",
				"EditorSubsystem",
				"Engine",
				"FileUtilities",
				"HTTP",
				"Json",
				"JsonUtilities",
				"Localization", 
				"LocalizationCommandletExecution", 
				"MainFrame",
				"Projects",
				"Slate",
				"SlateCore", 
				"UnrealEd", 
				"WebBrowser",

				"Tolgee"
			}
		);
	}
}
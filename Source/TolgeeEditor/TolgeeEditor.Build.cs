// Copyright (c) Tolgee. All Rights Reserved.

using UnrealBuildTool;

public class TolgeeEditor : ModuleRules
{
    public TolgeeEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "EditorSubsystem",
                "Engine",
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

        if (Target.Version.MajorVersion > 4)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "DeveloperToolSettings",
                }
            );
        }
    }
}

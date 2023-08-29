// Copyright (c) Tolgee. All Rights Reserved.

using UnrealBuildTool;

public class TolgeeEditor : ModuleRules
{
    public TolgeeEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

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

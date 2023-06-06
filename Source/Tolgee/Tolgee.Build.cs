// Copyright (c) Tolgee 2022-2023. All Rights Reserved.

using UnrealBuildTool;

public class Tolgee : ModuleRules
{
    public Tolgee(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", 
                "DeveloperSettings", 
                "HTTP",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Json",
                "JsonUtilities",
            }
        );
    }
}

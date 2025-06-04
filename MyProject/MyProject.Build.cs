// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MyProject : ModuleRules
{
	public MyProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"HeadMountedDisplay",
			"EnhancedInput",
			"UMG",
			"GameplayTags",
			"Json",
			"JsonUtilities"
		});
		PrivateDependencyModuleNames.AddRange(new string[] { 
			"Slate", 
			"UMGEditor",
			"SlateCore"  // 如果使用了UMG，通常也需要这些
		});
	}
}

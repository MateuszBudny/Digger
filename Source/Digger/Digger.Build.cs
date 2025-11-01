// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Digger : ModuleRules
{
	public Digger(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}

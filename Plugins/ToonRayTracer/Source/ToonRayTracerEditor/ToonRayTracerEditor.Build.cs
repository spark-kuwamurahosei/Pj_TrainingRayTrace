// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

// ToonRayTracer の設定を切り替えるエディタパネル（エディタでのみ読み込まれる）
public class ToonRayTracerEditor : ModuleRules
{
	public ToonRayTracerEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"UnrealEd",
				"PropertyEditor",
				"WorkspaceMenuStructure",
			}
			);
	}
}

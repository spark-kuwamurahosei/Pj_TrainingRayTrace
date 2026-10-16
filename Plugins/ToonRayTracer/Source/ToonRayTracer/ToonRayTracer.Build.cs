// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class ToonRayTracer : ModuleRules
{
	public ToonRayTracer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);

		// UE5.5 では、TSR の前に描くために、ポストプロセスの直前の入力（FPostProcessingInputs）と
		// ビューの描画範囲（FViewInfo）を読む。どちらも Renderer の公開されていないヘッダーにある
		if (Target.Version.MajorVersion == 5 && Target.Version.MinorVersion < 6)
		{
			string RendererDir = GetModuleDirectory("Renderer");
			PrivateIncludePaths.Add(Path.Combine(RendererDir, "Private"));
			PrivateIncludePaths.Add(Path.Combine(RendererDir, "Internal"));
		}

		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"RenderCore",
				"RHI",
				"Projects",
				"Renderer",
				"CoreUObject",
				"Engine"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}

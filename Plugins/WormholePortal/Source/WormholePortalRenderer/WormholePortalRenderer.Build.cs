// Copyright 2026 Team Beaver. All Rights Reserved.

using UnrealBuildTool;

public class WormholePortalRenderer : ModuleRules
{
	public WormholePortalRenderer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Projects",
				"RenderCore",
				"Renderer",
				"RHI",
				"WormholePortalRuntime"
			});
	}
}

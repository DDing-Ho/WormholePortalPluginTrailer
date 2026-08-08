// Copyright 2026 Team Beaver. All Rights Reserved.

using UnrealBuildTool;

public class WPFinalDemo : ModuleRules
{
	public WPFinalDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"TP_FirstPerson",
			"WormholePortalRuntime"
		});

		PublicIncludePaths.AddRange(new[]
		{
			"WPFinalDemo",
			"WPFinalDemo/PortalGun"
		});
	}
}

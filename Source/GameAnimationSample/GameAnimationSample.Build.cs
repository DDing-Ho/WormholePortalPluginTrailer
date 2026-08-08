using UnrealBuildTool;

public class GameAnimationSample : ModuleRules
{
	public GameAnimationSample(ReadOnlyTargetRules Target) : base(Target)
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
	}
}

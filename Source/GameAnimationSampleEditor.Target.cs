using UnrealBuildTool;

public class GameAnimationSampleEditorTarget : TargetRules
{
	public GameAnimationSampleEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("GameAnimationSample");
	}
}

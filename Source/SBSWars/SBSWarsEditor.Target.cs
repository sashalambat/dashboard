using UnrealBuildTool;
using System.Collections.Generic;

public class SBSWarsEditorTarget : TargetRules
{
	public SBSWarsEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.Add("SBSWars");
	}
}

using UnrealBuildTool;

public class SBSWars : ModuleRules
{
	public SBSWars(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "UMG", "Slate", "SlateCore",
			"AIModule", "NavigationSystem", "NetCore"
		});
		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}

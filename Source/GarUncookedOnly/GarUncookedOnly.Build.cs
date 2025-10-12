using UnrealBuildTool;

public class GARUncookedOnly : ModuleRules
{
	public GARUncookedOnly(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		CppCompileWarningSettings.NonInlinedGenCppWarningLevel = WarningLevel.Warning;

		PublicDependencyModuleNames.AddRange(
		[
			"Core", "CoreUObject", "Engine", "AnimationBlueprintLibrary", "GAR"
		]);

		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.AddRange(
			[
				"AnimGraph", "AnimGraphRuntime"
			]);

			PrivateDependencyModuleNames.AddRange(
			[
				"BlueprintGraph", "Slate", "SlateCore", "Projects"
			]);
		}
	}
}
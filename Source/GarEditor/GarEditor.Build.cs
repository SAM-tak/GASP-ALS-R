using UnrealBuildTool;

public class GAREditor : ModuleRules
{
	public GAREditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		CppCompileWarningSettings.NonInlinedGenCppWarningLevel = WarningLevel.Warning;

		PublicDependencyModuleNames.AddRange(
		[
			"Core", "CoreUObject", "Engine", "AnimationModifiers", "AnimationBlueprintLibrary", "GAR"
		]);

		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.AddRange(
			[
				"AnimGraph"
			]);

			PrivateDependencyModuleNames.AddRange(
			[
				"BlueprintGraph", "Slate", "SlateCore", "Projects"
			]);
		}
	}
}
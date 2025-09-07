using UnrealBuildTool;

public class GARExtras : ModuleRules
{
	public GARExtras(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		CppCompileWarningSettings.NonInlinedGenCppWarningLevel = WarningLevel.Warning;

		PublicDependencyModuleNames.AddRange(
		[
			"Core", "CoreUObject", "Engine", "AIModule", "GAR", "GARCamera"
		]);

		PrivateDependencyModuleNames.AddRange(
		[
			"EnhancedInput", "GameplayTags", "GameplayAbilities"
		]);
	}
}
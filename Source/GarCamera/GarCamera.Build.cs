using UnrealBuildTool;

public class GARCamera : ModuleRules
{
	public GARCamera(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		CppCompileWarningSettings.NonInlinedGenCppWarningLevel = WarningLevel.Warning;

		PublicDependencyModuleNames.AddRange(
		[
			"Core", "CoreUObject", "Engine", "EngineSettings", "GameplayTags", "GameplayCameras", "GAR"
		]);

		PrivateDependencyModuleNames.AddRange(
		[
			"EngineSettings", "GameplayAbilities", "ModularGameplay", "NetCore"
		]);
	}
}
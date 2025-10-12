using UnrealBuildTool;

public class GAR : ModuleRules
{
    public GAR(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

        CppCompileWarningSettings.NonInlinedGenCppWarningLevel = WarningLevel.Warning;

        PublicDependencyModuleNames.AddRange(
		[
			"Core", "CoreUObject", "Engine", "EnhancedInput", "GameplayTags", "GameplayAbilities", "GameplayTasks", "ModularGameplay", "PoseSearch",
			"Mover", "AnimationWarpingRuntime"
		]);

        PrivateDependencyModuleNames.AddRange(
		[
			"EngineSettings", "NetCore", "PhysicsCore", "Niagara", "AnimGraphRuntime", "RigVM", "ControlRig", "Chaos", "MotionWarping", "Chooser",
			"AnimationWarpingRuntime"
		]);

        if (Target.Type == TargetRules.TargetType.Editor)
        {
            PrivateDependencyModuleNames.AddRange(["MessageLog"]);
        }
    }
}
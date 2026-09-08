using UnrealBuildTool;

public class GrimrockPrototype : ModuleRules
{
	public GrimrockPrototype(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// WORLDOBJ-MIG09-E2C-FINAL-A: GridActivationComponent is intentionally split
		// across independent implementation units. Keep this module non-unity so
		// file-local log categories and anonymous-namespace helpers remain isolated.
		bUseUnity = false;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Niagara", "GrimrockLua" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
	}
}

using UnrealBuildTool;

public class GrimrockPrototype : ModuleRules
{
	public GrimrockPrototype(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Niagara", "GrimrockLua" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "AssetRegistry" });
    }
}

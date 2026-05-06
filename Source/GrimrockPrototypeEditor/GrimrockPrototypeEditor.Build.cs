using UnrealBuildTool;

public class GrimrockPrototypeEditor : ModuleRules
{
    public GrimrockPrototypeEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "EditorFramework",
            "Slate",
            "SlateCore",
            "GrimrockPrototype"
        });
    }
}

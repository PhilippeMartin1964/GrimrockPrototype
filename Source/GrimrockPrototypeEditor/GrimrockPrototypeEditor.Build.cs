using UnrealBuildTool;

public class GrimrockPrototypeEditor : ModuleRules
{
    public GrimrockPrototypeEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "ApplicationCore",
            "AssetRegistry",
            "UnrealEd",
            "EditorFramework",
            "LevelEditor",
            "PropertyEditor",
            "Slate",
            "SlateCore",
            "GrimrockPrototype"
        });
    }
}

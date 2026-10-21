using UnrealBuildTool;

public class GrimrockPrototypeEditor : ModuleRules
{
    public GrimrockPrototypeEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Editor tooling contains many file-local helper functions. Building the
        // editor module without unity batches keeps those implementation details
        // isolated and avoids nondeterministic collisions when UBT reshuffles
        // source files after adding or removing editor tests/tools.
        bUseUnity = false;

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
            "BlueprintGraph",
            "PropertyEditor",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "GrimrockPrototype",
            "GrimrockLua"
        });
    }
}

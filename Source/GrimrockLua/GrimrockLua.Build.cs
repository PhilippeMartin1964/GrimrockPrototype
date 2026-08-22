using UnrealBuildTool;
using System.IO;

public class GrimrockLua : ModuleRules
{
    public GrimrockLua(ReadOnlyTargetRules Target) : base(Target)
    {
        // Lua is compiled as one explicit translation unit (Lua54.cpp). Keep
        // it out of Unreal unity/PCH compilation so UE macros cannot leak into
        // the official Lua sources.
        PCHUsage = PCHUsageMode.NoPCHs;
        bUseUnity = false;

        PublicDependencyModuleNames.AddRange(
            new string[] { "Core", "CoreUObject" });

        PrivateIncludePaths.Add(
            Path.GetFullPath(
                Path.Combine(ModuleDirectory, "../../ThirdParty/Lua54")));
    }
}

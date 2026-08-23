#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"
#include "EditorTools/GridLevelEditorActor.h"

class UGridLevelAsset;

struct FGridEditorLuaScriptAnalysis
{
    FName ScriptId = NAME_None;
    bool bEnabled = true;
    bool bValid = false;
    FString Error;
    TArray<FName> CallbackNames;
};

struct FGridEditorLuaAnalysis
{
    bool bDefinitionsValid = false;
    bool bFullVmValid = false;
    FString GlobalError;
    TArray<FGridEditorLuaScriptAnalysis> Scripts;

    bool IsValid () const
    {
        if (!bDefinitionsValid || !bFullVmValid)
        {
            return false;
        }
        for (const FGridEditorLuaScriptAnalysis& Script : Scripts)
        {
            if (Script.bEnabled && !Script.bValid)
            {
                return false;
            }
        }
        return true;
    }
};

/** MON19.6 editor-only Lua authoring, binding and validation helpers. */
namespace GridEditorLuaService
{
    GRIMROCKPROTOTYPEEDITOR_API bool AnalyzeLevel (
        const UGridLevelAsset& LevelAsset,
        FGridEditorLuaAnalysis& OutAnalysis);

    GRIMROCKPROTOTYPEEDITOR_API bool GetCallbacksForScript (
        const UGridLevelAsset& LevelAsset,
        FName ScriptId,
        TArray<FName>& OutCallbacks,
        FString& OutError);

    GRIMROCKPROTOTYPEEDITOR_API bool IsLuaLinkSupported (
        const UGridLevelAsset& LevelAsset,
        const FGridObjectLink& Link,
        FString& OutError);

    GRIMROCKPROTOTYPEEDITOR_API bool AddLuaLink (
        AGridLevelEditorActor& EditorActor,
        const FGridObjectLink& Link,
        FString& OutError);

    GRIMROCKPROTOTYPEEDITOR_API bool RemoveLuaLink (
        AGridLevelEditorActor& EditorActor,
        const FGridObjectLink& Link);

    GRIMROCKPROTOTYPEEDITOR_API FName MakeUniqueScriptId (
        const UGridLevelAsset& LevelAsset);

    GRIMROCKPROTOTYPEEDITOR_API int32 CountScriptReferences (
        const UGridLevelAsset& LevelAsset,
        FName ScriptId);

    GRIMROCKPROTOTYPEEDITOR_API bool AddScript (
        UGridLevelAsset& LevelAsset,
        FName ScriptId,
        const FString& Source,
        FString& OutError);

    GRIMROCKPROTOTYPEEDITOR_API bool RenameScript (
        UGridLevelAsset& LevelAsset,
        FName OldScriptId,
        FName NewScriptId,
        FString& OutError);

    GRIMROCKPROTOTYPEEDITOR_API bool SetScriptEnabled (
        UGridLevelAsset& LevelAsset,
        FName ScriptId,
        bool bEnabled,
        FString& OutError);

    GRIMROCKPROTOTYPEEDITOR_API bool SetScriptSource (
        UGridLevelAsset& LevelAsset,
        FName ScriptId,
        const FString& Source,
        FString& OutError);

    GRIMROCKPROTOTYPEEDITOR_API bool RemoveScript (
        UGridLevelAsset& LevelAsset,
        FName ScriptId,
        FString& OutError);

    /**
     * Runs the historical level validator, removes MON19 false positives for
     * data-only Logic / targetless Lua links, then appends authoritative Logic
     * and Lua diagnostics.
     */
    GRIMROCKPROTOTYPEEDITOR_API TArray<FGridLevelValidationMessage>
        ValidateCurrentLevelWithLua (
            AGridLevelEditorActor& EditorActor);
}

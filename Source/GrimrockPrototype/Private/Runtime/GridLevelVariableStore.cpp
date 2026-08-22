#include "Runtime/GridLevelVariableStore.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridLevelVariableTypes.h"
#include "Runtime/GridDungeonRuntimeState.h"

namespace
{
    const FGridLevelVariableDefinition* FindDefinition (
        const UGridLevelAsset& LevelAsset,
        FName VariableId)
    {
        return LevelAsset.LevelVariables.FindByPredicate (
            [VariableId] (const FGridLevelVariableDefinition& Definition)
        {
            return Definition.VariableId == VariableId;
        });
    }

    bool ValidateVariableId (FName VariableId, FString& OutError)
    {
        if (!VariableId.IsNone ())
        {
            return true;
        }

        OutError = TEXT ("VariableId cannot be None.");
        return false;
    }
}

namespace GridLevelVariableStore
{
    bool ValidateDefinitions (
        const UGridLevelAsset& LevelAsset,
        FString& OutError)
    {
        OutError.Reset ();
        TSet<FName> SeenIds;
        for (int32 Index = 0; Index < LevelAsset.LevelVariables.Num (); ++Index)
        {
            const FGridLevelVariableDefinition& Definition =
                LevelAsset.LevelVariables[Index];
            if (Definition.VariableId.IsNone ())
            {
                OutError = FString::Printf (
                    TEXT ("LevelVariables[%d] has VariableId=None."),
                    Index);
                return false;
            }
            if (SeenIds.Contains (Definition.VariableId))
            {
                OutError = FString::Printf (
                    TEXT ("VariableId '%s' is declared more than once."),
                    *Definition.VariableId.ToString ());
                return false;
            }

            switch (Definition.Type)
            {
                case EGridLevelVariableType::Bool:
                case EGridLevelVariableType::Int32:
                    break;

                default:
                    OutError = FString::Printf (
                        TEXT ("VariableId '%s' has an unsupported type."),
                        *Definition.VariableId.ToString ());
                    return false;
            }
            SeenIds.Add (Definition.VariableId);
        }
        return true;
    }

    bool EnsureInitialized (
        const UGridLevelAsset& LevelAsset,
        FGridLevelRuntimeState& RuntimeState,
        FString& OutError)
    {
        if (!ValidateDefinitions (LevelAsset, OutError))
        {
            return false;
        }

        TMap<FName, bool> ReconciledBoolVariables;
        TMap<FName, int32> ReconciledIntVariables;
        for (const FGridLevelVariableDefinition& Definition :
            LevelAsset.LevelVariables)
        {
            if (Definition.Type == EGridLevelVariableType::Bool)
            {
                const bool* ExistingValue = RuntimeState.bLevelVariablesInitialized
                    ? RuntimeState.BoolVariables.Find (Definition.VariableId)
                    : nullptr;
                ReconciledBoolVariables.Add (
                    Definition.VariableId,
                    ExistingValue
                        ? *ExistingValue
                        : Definition.bDefaultBoolValue);
            }
            else
            {
                const int32* ExistingValue = RuntimeState.bLevelVariablesInitialized
                    ? RuntimeState.IntVariables.Find (Definition.VariableId)
                    : nullptr;
                ReconciledIntVariables.Add (
                    Definition.VariableId,
                    ExistingValue
                        ? *ExistingValue
                        : Definition.DefaultInt32Value);
            }
        }

        RuntimeState.BoolVariables = MoveTemp (ReconciledBoolVariables);
        RuntimeState.IntVariables = MoveTemp (ReconciledIntVariables);
        RuntimeState.bLevelVariablesInitialized = true;
        OutError.Reset ();
        return true;
    }

    bool ResetToDefaults (
        const UGridLevelAsset& LevelAsset,
        FGridLevelRuntimeState& RuntimeState,
        FString& OutError)
    {
        if (!ValidateDefinitions (LevelAsset, OutError))
        {
            return false;
        }

        RuntimeState.BoolVariables.Reset ();
        RuntimeState.IntVariables.Reset ();
        RuntimeState.bLevelVariablesInitialized = false;
        return EnsureInitialized (LevelAsset, RuntimeState, OutError);
    }

    bool TryGetBool (
        const UGridLevelAsset& LevelAsset,
        FGridLevelRuntimeState& RuntimeState,
        FName VariableId,
        bool& OutValue,
        FString& OutError)
    {
        OutValue = false;
        if (!ValidateVariableId (VariableId, OutError) ||
            !EnsureInitialized (LevelAsset, RuntimeState, OutError))
        {
            return false;
        }

        const FGridLevelVariableDefinition* Definition =
            FindDefinition (LevelAsset, VariableId);
        if (!Definition || Definition->Type != EGridLevelVariableType::Bool)
        {
            OutError = FString::Printf (
                TEXT ("Variable '%s' is not a declared Bool."),
                *VariableId.ToString ());
            return false;
        }

        const bool* Value = RuntimeState.BoolVariables.Find (VariableId);
        if (!Value)
        {
            OutError = FString::Printf (
                TEXT ("Bool variable '%s' has no runtime value."),
                *VariableId.ToString ());
            return false;
        }
        OutValue = *Value;
        OutError.Reset ();
        return true;
    }

    bool SetBool (
        const UGridLevelAsset& LevelAsset,
        FGridLevelRuntimeState& RuntimeState,
        FName VariableId,
        bool bValue,
        FString& OutError)
    {
        bool ExistingValue = false;
        if (!TryGetBool (
                LevelAsset,
                RuntimeState,
                VariableId,
                ExistingValue,
                OutError))
        {
            return false;
        }

        RuntimeState.BoolVariables.Add (VariableId, bValue);
        OutError.Reset ();
        return true;
    }

    bool TryGetInt32 (
        const UGridLevelAsset& LevelAsset,
        FGridLevelRuntimeState& RuntimeState,
        FName VariableId,
        int32& OutValue,
        FString& OutError)
    {
        OutValue = 0;
        if (!ValidateVariableId (VariableId, OutError) ||
            !EnsureInitialized (LevelAsset, RuntimeState, OutError))
        {
            return false;
        }

        const FGridLevelVariableDefinition* Definition =
            FindDefinition (LevelAsset, VariableId);
        if (!Definition || Definition->Type != EGridLevelVariableType::Int32)
        {
            OutError = FString::Printf (
                TEXT ("Variable '%s' is not a declared Int32."),
                *VariableId.ToString ());
            return false;
        }

        const int32* Value = RuntimeState.IntVariables.Find (VariableId);
        if (!Value)
        {
            OutError = FString::Printf (
                TEXT ("Int32 variable '%s' has no runtime value."),
                *VariableId.ToString ());
            return false;
        }
        OutValue = *Value;
        OutError.Reset ();
        return true;
    }

    bool SetInt32 (
        const UGridLevelAsset& LevelAsset,
        FGridLevelRuntimeState& RuntimeState,
        FName VariableId,
        int32 Value,
        FString& OutError)
    {
        int32 ExistingValue = 0;
        if (!TryGetInt32 (
                LevelAsset,
                RuntimeState,
                VariableId,
                ExistingValue,
                OutError))
        {
            return false;
        }

        RuntimeState.IntVariables.Add (VariableId, Value);
        OutError.Reset ();
        return true;
    }

    bool ValidateDungeonSnapshots (
        const FGridDungeonRuntimeState& DungeonState,
        FString& OutError)
    {
        OutError.Reset ();
        for (const TPair<FName, FGridLevelRuntimeState>& LevelPair :
            DungeonState.LevelStates)
        {
            const FGridLevelRuntimeState& State = LevelPair.Value;
            if (!State.bLevelVariablesInitialized)
            {
                if (!State.BoolVariables.IsEmpty () ||
                    !State.IntVariables.IsEmpty ())
                {
                    OutError = FString::Printf (
                        TEXT ("Level '%s' has variable values but bLevelVariablesInitialized=false."),
                        *LevelPair.Key.ToString ());
                    return false;
                }
                continue;
            }

            for (const TPair<FName, bool>& Pair : State.BoolVariables)
            {
                if (Pair.Key.IsNone ())
                {
                    OutError = FString::Printf (
                        TEXT ("Level '%s' contains a Bool variable with VariableId=None."),
                        *LevelPair.Key.ToString ());
                    return false;
                }
                if (State.IntVariables.Contains (Pair.Key))
                {
                    OutError = FString::Printf (
                        TEXT ("Level '%s' stores VariableId '%s' as both Bool and Int32."),
                        *LevelPair.Key.ToString (),
                        *Pair.Key.ToString ());
                    return false;
                }
            }
            for (const TPair<FName, int32>& Pair : State.IntVariables)
            {
                if (Pair.Key.IsNone ())
                {
                    OutError = FString::Printf (
                        TEXT ("Level '%s' contains an Int32 variable with VariableId=None."),
                        *LevelPair.Key.ToString ());
                    return false;
                }
            }
        }
        return true;
    }

    void ResetLegacyDungeonSnapshots (
        FGridDungeonRuntimeState& DungeonState)
    {
        for (TPair<FName, FGridLevelRuntimeState>& LevelPair :
            DungeonState.LevelStates)
        {
            FGridLevelRuntimeState& State = LevelPair.Value;
            State.bLevelVariablesInitialized = false;
            State.BoolVariables.Reset ();
            State.IntVariables.Reset ();
        }
    }
}

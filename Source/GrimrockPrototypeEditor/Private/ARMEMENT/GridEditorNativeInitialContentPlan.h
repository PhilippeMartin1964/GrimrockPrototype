#pragma once

#include "Core/GridLevelAsset.h"
#include "EditorTools/GridEditorCanonicalItemLevelPolicy.h"
#include "UObject/UnrealType.h"

/** X.58: only the original 17+1 authored receptacle contents.
 * Plans are ordinary value copies, never native package edits or saves.
 */
struct FGridEditorNativeInitialContentPlan
{
    static constexpr int32 LevelCount = 4;
    static constexpr int32 TargetCount = 2;
    static constexpr int32 ExpectedRows = 18;
    static const TCHAR* Path(int32 Index)
    {
        static const TCHAR* Paths[] = {
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00"),
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_01.DA_GridLevel_01"),
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_New_Level_03.DA_GridLevel_New_Level_03"),
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_Pillars_of_Light.DA_GridLevel_Pillars_of_Light")
        };
        check(Index >= 0 && Index < LevelCount);
        return Paths[Index];
    }

    static int32 OriginalRows(int32 Index)
    {
        return Index == 0 ? 17 : Index == 1 ? 1 : 0;
    }

    struct FLevel
    {
        TArray<FGridWorldObjectInstance> WorldObjects;
        int32 Rows = 0;
        int32 Old = 0;
    };

    static bool SameObjects(const TArray<FGridWorldObjectInstance>& A,
        const TArray<FGridWorldObjectInstance>& B)
    {
        if (A.Num() != B.Num()) return false;
        for (int32 Index = 0; Index < A.Num(); ++Index)
        {
            for (TFieldIterator<FProperty> Field(FGridWorldObjectInstance::StaticStruct());
                Field; ++Field)
                if (!Field->Identical_InContainer(&A[Index], &B[Index])) return false;
        }
        return true;
    }

    static bool TryBuildLevel(const UGridLevelAsset& Source, const UGridItemSubsystem* Registry,
        int32 Expected, FLevel& Out, FString& Error)
    {
        Out = FLevel();
        Error.Reset();
        if (!IsValid(Registry) || !Registry->IsConfigured())
        {
            Error = TEXT("Missing configured ItemSystem.");
            return false;
        }
        FLevel Plan;
        Plan.WorldObjects = Source.WorldObjectInstances;
        TSet<FGuid> Seen;
        for (FGridWorldObjectInstance& Object : Plan.WorldObjects)
        {
            auto& Initial = Object.InstanceConfig.ReceptacleInitialContent;
            if (Initial.IsEmpty()) continue;
            if (Object.Type != EGridLevelObjectType::Receptacle ||
                !Object.InstanceId.IsValid() || Seen.Contains(Object.InstanceId) ||
                !Source.IsValidCoord(Object.CellX, Object.CellY) ||
                Object.WorldObjectDefinitionId.IsNone())
            {
                Error = TEXT("Unexpected initial-content owner, GUID, definition or cell.");
                return false;
            }
            Seen.Add(Object.InstanceId);
            for (const FGridReceptacleInitialItemConfig& Item : Initial)
            {
                ++Plan.Rows;
                FGridResolvedItem Physical;
                if (Item.Quantity < 1 || !FGridEditorCanonicalItemLevelPolicy::CanAuthorCanonicalLevel(
                        Registry, Item.CanonicalItemId, Item.ItemLevel) ||
                    !Registry->ResolveItem(Item.CanonicalItemId, Item.ItemLevel, Physical) ||
                    !Physical.IsValid() || Physical.ItemId != Item.CanonicalItemId ||
                    Physical.ItemLevel != Item.ItemLevel)
                {
                    Error = FString::Printf(TEXT("Invalid registered physical initial item %s on %s."),
                        *Item.CanonicalItemId.ToString(), *Object.InstanceId.ToString());
                    return false;
                }
            }
        }
        if (Plan.Rows != Expected || Plan.Old != 0)
        {
            Error = FString::Printf(TEXT("Expected %d initial rows and zero old sources, got %d/%d."),
                Expected, Plan.Rows, Plan.Old);
            return false;
        }
        Out = MoveTemp(Plan);
        return true;
    }

    static bool TryBuild(const TArray<UGridLevelAsset*>& Levels, const UGridItemSubsystem* Registry,
        TArray<FLevel>& Out, int32& Old, FString& Error)
    {
        Out.Reset();
        Old = 0;
        Error.Reset();
        if (Levels.Num() != LevelCount)
        {
            Error = TEXT("Four exact level sources are required.");
            return false;
        }
        TArray<FLevel> Plans;
        int32 Count = 0;
        for (int32 Index = 0; Index < LevelCount; ++Index)
        {
            const UGridLevelAsset* Level = Levels[Index];
            if (!IsValid(Level) || Level->GetPathName() != FString(Path(Index)))
            {
                Error = TEXT("Unexpected native level path.");
                return false;
            }
            FLevel Candidate;
            if (!TryBuildLevel(*Level, Registry, OriginalRows(Index), Candidate, Error))
                return false;
            Count += Candidate.Old;
            Plans.Add(MoveTemp(Candidate));
        }
        if (Count != 0)
        {
            Error = TEXT("Unexpected historical source.");
            return false;
        }
        Old = Count;
        Out = MoveTemp(Plans);
        return true;
    }

    /** Export the full level-owned reflected authoring, using the
     * expected WorldObjectInstances array for the canonical parity audit.
     * This intentionally preserves the dormant overrides, Lua, links, cells,
     * loose objects, monsters and all positioning/configuration. */
    static FString Manifest(const UGridLevelAsset& Level, const FLevel& Plan)
    {
        FString Result;
        for (TFieldIterator<FProperty> Field(UGridLevelAsset::StaticClass(),
            EFieldIteratorFlags::ExcludeSuper); Field; ++Field)
        {
            if (Field->HasAnyPropertyFlags(CPF_Transient)) continue;
            const void* Value = Field->GetFName() ==
                GET_MEMBER_NAME_CHECKED(UGridLevelAsset, WorldObjectInstances)
                ? static_cast<const void*>(&Plan.WorldObjects)
                : Field->ContainerPtrToValuePtr<void>(&Level);
            FString Exported;
            Field->ExportTextItem_Direct(Exported, Value, nullptr,
                const_cast<UGridLevelAsset*>(&Level), PPF_None);
            // Prefix lengths so values containing separators remain unambiguous.
            const FString Name = Field->GetName();
            Result += FString::Printf(TEXT("%d:%s=%d:%s\n"),
                Name.Len(), *Name, Exported.Len(), *Exported);
        }
        return Result;
    }
};

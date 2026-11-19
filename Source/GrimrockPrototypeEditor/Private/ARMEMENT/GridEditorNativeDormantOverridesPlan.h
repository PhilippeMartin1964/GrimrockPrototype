#pragma once

#include "GridEditorNativeInitialContentPlan.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "EditorTools/GridEditorCanonicalReceptacleRuleAuthoring.h"
#include "EditorTools/GridEditorCanonicalLockRuleAuthoring.h"
#include "UObject/Package.h"

/** X.59: remove only three proven dormant lists. Reuse X.58 reflection helpers. */
struct FGridEditorNativeDormantOverridesPlan
{
    using FBase = FGridEditorNativeInitialContentPlan;
    using FLevel = FBase::FLevel;
    static constexpr int32 LevelCount = FBase::LevelCount;
    static constexpr int32 TargetCount = 1;
    static constexpr int32 ExpectedRows = 3;
    static const TCHAR* Path(int32 Index) { return FBase::Path(Index); }
    static int32 OriginalRows(int32 Index) { return Index == 0 ? 3 : 0; }

    struct FTarget { const TCHAR* Guid; int32 X, Y; const TCHAR* ItemId; const TCHAR* DefinitionPath; };
    static const FTarget& Target(int32 Index)
    {
        static const FTarget Targets[] = {
            {TEXT("0CB018F04E6C3DCB059133A0CA12214F"), 16, 25, TEXT("Item_Torch"),
             TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Receptacle_TorchHolder.DA_Receptacle_TorchHolder")},
            {TEXT("12F822EE439884334380F4AED3223847"), 11, 24, TEXT("Key_Copper"),
             TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall.DA_Lock_CopperWall")},
            {TEXT("22CCD36F42A9F3FE8C7C3B852E2F71FA"), 9, 23, TEXT("Key_Iron"),
             TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall.DA_Lock_IronWall")}
        };
        check(Index >= 0 && Index < ExpectedRows);
        return Targets[Index];
    }
    static int32 TargetIndex(const FGuid& Id)
    {
        const FString Digits = Id.ToString(EGuidFormats::Digits);
        for (int32 Index = 0; Index < ExpectedRows; ++Index)
            if (Digits.Equals(Target(Index).Guid, ESearchCase::IgnoreCase)) return Index;
        return INDEX_NONE;
    }
    static bool SameObjects(const TArray<FGridWorldObjectInstance>& A, const TArray<FGridWorldObjectInstance>& B)
    { return FBase::SameObjects(A, B); }
    static FString Manifest(const UGridLevelAsset& Level, const FLevel& Plan)
    { return FBase::Manifest(Level, Plan); }
    static bool SameBehavior(const FGridObjectBehaviorParams& A, const FGridObjectBehaviorParams& B)
    {
        for (TFieldIterator<FProperty> Field(FGridObjectBehaviorParams::StaticStruct()); Field; ++Field)
            if (!Field->Identical_InContainer(&A, &B)) return false;
        return true;
    }
    static bool TryBuildLevel(const UGridLevelAsset& Source, const UGridItemSubsystem* Registry,
        int32 LevelIndex, FLevel& Out, FString& Error)
    {
        Out = FLevel();
        Error.Reset();
        if (!IsValid(Registry) || !Registry->IsConfigured() || LevelIndex < 0 || LevelIndex >= LevelCount)
        { Error = TEXT("Missing registry or invalid level index."); return false; }
        FLevel Plan;
        Plan.WorldObjects = Source.WorldObjectInstances;
        TSet<int32> Seen;
        for (FGridWorldObjectInstance& Object : Plan.WorldObjects)
        {
            auto& O = Object.InstanceConfig.InteractionOverrides;
            if (O.bOverrideReceptacleRules || O.bOverrideAcceptedKeys)
            { Error = TEXT("Active acceptance override: refused."); return false; }
            const int32 Index = TargetIndex(Object.InstanceId);
            if (Index == INDEX_NONE || LevelIndex != 0)
            {
                if (!O.ReceptacleRules.AcceptedItems.IsEmpty() ||
                    !O.AcceptedKeys.AcceptedKeyIds.IsEmpty() || Index != INDEX_NONE)
                { Error = TEXT("Unexpected acceptance rule outside approved owners."); return false; }
                continue;
            }
            const FTarget& T = Target(Index);
            const FName ItemId(T.ItemId);
            if (Seen.Contains(Index) || Object.CellX != T.X || Object.CellY != T.Y ||
                !Source.IsValidCoord(Object.CellX, Object.CellY) ||
                Object.Type != EGridLevelObjectType::Receptacle ||
                !Registry->HasCanonicalItem(ItemId) || !O.AcceptedKeys.AcceptedKeyIds.IsEmpty())
            { Error = TEXT("Approved owner, cell, type or ID changed."); return false; }
            Seen.Add(Index);
            ++Plan.Rows;
            const UGridWorldObjectDefinitionAsset* Definition =
                LoadObject<UGridWorldObjectDefinitionAsset>(nullptr, T.DefinitionPath);
            if (!IsValid(Definition) || Definition->DefinitionId != Object.WorldObjectDefinitionId ||
                Definition->GetOutermost()->IsDirty())
            { Error = TEXT("Shared definition missing, dirty or mismatched."); return false; }
            const FGridObjectBehaviorParams Before = GridObjectInstanceBehavior::Resolve(Object, Definition);
            if (Index == 0)
            {
                if (!O.ReceptacleRules.AcceptedItems.IsEmpty())
                { Error = TEXT("Unexpected dormant torch list."); return false; }
                TArray<FGridReceptacleAcceptedItemConfig> Reactivated;
                if (!FGridEditorCanonicalReceptacleRuleAuthoring::TryCopyDefaults(
                        Registry, Definition->DefaultBehavior.Receptacle.AcceptedItems, Reactivated) ||
                    Reactivated.Num() != 1 ||
                    Reactivated[0].CanonicalItemId != ItemId)
                { Error = TEXT("Cannot reauthor exact canonical torch rule."); return false; }
            }
            else
            {
                if (!O.ReceptacleRules.AcceptedItems.IsEmpty())
                { Error = TEXT("Unexpected dormant key owner rule list."); return false; }
                FGridLockAcceptedKeyInstanceRuleValues Reactivated;
                if (!FGridEditorCanonicalLockRuleAuthoring::TryCopyDefaults(
                        Registry, Definition->DefaultBehavior.Lock, Reactivated) ||
                    Reactivated.AcceptedKeyIds.Num() != 1 ||
                    Reactivated.AcceptedKeyIds[0] != ItemId)
                { Error = TEXT("Cannot reauthor exact canonical lock rule."); return false; }
            }
            const FGridObjectBehaviorParams After = GridObjectInstanceBehavior::Resolve(Object, Definition);
            if (!SameBehavior(Before, After) || Definition->GetOutermost()->IsDirty())
            { Error = TEXT("Effective behavior changed."); return false; }
        }
        if (Plan.Rows != OriginalRows(LevelIndex) || Plan.Old != 0)
        { Error = TEXT("Missing approved owners or partial cleanup."); return false; }
        Out = MoveTemp(Plan);
        return true;
    }
    static bool TryBuild(const TArray<UGridLevelAsset*>& Levels, const UGridItemSubsystem* Registry,
        TArray<FLevel>& Out, int32& Old, FString& Error)
    {
        Out.Reset();
        Old = 0;
        TArray<FBase::FLevel> Initial;
        int32 InitialOld = INDEX_NONE;
        if (!FBase::TryBuild(Levels, Registry, Initial, InitialOld, Error) || InitialOld != 0)
        { Error = TEXT("X.58B must remain pointerless: ") + Error; return false; }
        TArray<FLevel> Plans;
        int32 Count = 0;
        for (int32 Index = 0; Index < LevelCount; ++Index)
        {
            FLevel Candidate;
            if (!TryBuildLevel(*Levels[Index], Registry, Index, Candidate, Error)) return false;
            Count += Candidate.Old;
            Plans.Add(MoveTemp(Candidate));
        }
        Old = Count;
        Out = MoveTemp(Plans);
        return true;
    }
};

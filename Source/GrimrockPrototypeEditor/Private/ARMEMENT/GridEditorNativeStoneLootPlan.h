#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterLootResolver.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

/** X.60: only Stone has an existing canonical definition. No registry
 * expansion or change of drop probability is authorized in this lot. */
struct FGridEditorNativeStoneLootPlan
{
    static const TCHAR* MonsterPath(int32 Index)
    {
        return Index == 0
            ? TEXT("/Game/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower.DA_MON_GoblinThrower")
            : TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.DA_MON_RatGiant");
    }
    static const TCHAR* SourceStem(int32 Index)
    {
        static const TCHAR* Names[] = { TEXT("GoblinKnife"), TEXT("Stone"), TEXT("EmptyVial"), TEXT("RatMeat"), TEXT("RatTooth") };
        return Names[Index];
    }
    static FName ItemId(int32 Index)
    {
        static const FName Ids[] = { TEXT("GoblinKnife"), TEXT("Stone"), TEXT("EmptyVial"), TEXT("Item_RatMeat"), TEXT("Item_RatTooth") };
        return Ids[Index];
    }
    static FString SourcePath(int32 Index)
    {
        const FString Name = FString(TEXT("DA_Item_")) + SourceStem(Index);
        return TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/") + Name + TEXT(".") + Name;
    }
    static UGridItemDefinitionAsset* LoadSource(int32 Index)
    { return LoadObject<UGridItemDefinitionAsset>(nullptr, *SourcePath(Index)); }

    static bool SameTable(const TArray<FGridMonsterLootEntry>& A, const TArray<FGridMonsterLootEntry>& B)
    {
        if (A.Num() != B.Num()) return false;
        for (int32 Index = 0; Index < A.Num(); ++Index)
            for (TFieldIterator<FProperty> Field(FGridMonsterLootEntry::StaticStruct()); Field; ++Field)
                if (!Field->Identical_InContainer(&A[Index], &B[Index])) return false;
        return true;
    }

    static bool SameRolls(const TArray<FGridMonsterLootEntry>& A, const TArray<FGridMonsterLootEntry>& B)
    {
        for (int32 Seed = -64; Seed < 64; ++Seed)
        {
            const auto Before = FGridMonsterLootResolver::ResolveLoot(A, Seed);
            const auto After = FGridMonsterLootResolver::ResolveLoot(B, Seed);
            if (Before.Num() != A.Num() || Before.Num() != After.Num()) return false;
            for (int32 Index = 0; Index < Before.Num(); ++Index)
            {
                const auto& X = Before[Index];
                const auto& Y = After[Index];
                if (X.EntryIndex != Y.EntryIndex || X.ItemDefinitionId != Y.ItemDefinitionId ||
                    X.DropRoll != Y.DropRoll || X.bDropped != Y.bDropped || X.Quantity != Y.Quantity)
                    return false;
            }
        }
        return true;
    }

    static FString Export(const UObject& Object, const TArray<FGridMonsterLootEntry>* Normalized = nullptr)
    {
        FString Result;
        for (TFieldIterator<FProperty> Field(Object.GetClass(), EFieldIteratorFlags::ExcludeSuper); Field; ++Field)
        {
            if (Field->HasAnyPropertyFlags(CPF_Transient)) continue;
            const void* V = Normalized && Field->GetFName() ==
                GET_MEMBER_NAME_CHECKED(UGridMonsterDefinitionAsset, LootTable)
                ? static_cast<const void*>(Normalized) : Field->ContainerPtrToValuePtr<void>(&Object);
            FString Value;
            Field->ExportTextItem_Direct(Value, V, nullptr, const_cast<UObject*>(&Object), PPF_None);
            const FString Name = Field->GetName();
            Result += FString::Printf(TEXT("%d:%s=%d:%s\n"), Name.Len(), *Name, Value.Len(), *Value);
        }
        return Result;
    }

    static bool TryBuild(const UGridMonsterDefinitionAsset& Goblin,
        const UGridMonsterDefinitionAsset& Rat, const UGridItemSubsystem* Registry,
        TArray<FGridMonsterLootEntry>& Out, int32& Old, FString& Error)
    {
        Out.Reset();
        Old = 0;
        Error.Reset();
        if (!IsValid(Registry) || !Registry->IsConfigured() ||
            Goblin.GetPathName() != FString(MonsterPath(0)) ||
            Rat.GetPathName() != FString(MonsterPath(1)) ||
            Goblin.LootTable.Num() != 3 || Rat.LootTable.Num() != 2 ||
            Goblin.ExperienceReward != 125 || Rat.ExperienceReward != 500)
        { Error = TEXT("Unexpected real monster baseline."); return false; }

        // After X.61, the old commandlet remains pinned to 34; its
        // permanent test can also recheck Stone on the exact completed state.
        const bool bAllRegistered = Registry->HasCanonicalItem(ItemId(0));
        TArray<FGridMonsterLootEntry> Plan = Goblin.LootTable;
        int32 Count = 0;
        for (int32 Index = 0; Index < 5; ++Index)
        {
            const FGridMonsterLootEntry& Row = Index < 3 ? Goblin.LootTable[Index] : Rat.LootTable[Index - 3];
            const FName Id = ItemId(Index);
            const UGridItemDefinitionAsset* Source = LoadSource(Index);
            if (!IsValid(Source) || Source->GetOutermost()->IsDirty() ||
                Source->ItemDefinitionId != Id || !Source->IsValidDefinition() ||
                !Row.IsValidDefinition() || Row.GetResolvedItemDefinitionId() != Id ||
                Row.ItemLevel != 0 || Registry->HasCanonicalItem(Id) != (Index == 1 || bAllRegistered))
            { Error = FString::Printf(TEXT("Unexpected source/loot/registry state: %s"), *Id.ToString()); return false; }
            if (Index < 3)
            {
                const float ExpectedChance = Index == 1 ? 0.5f : 0.25f;
                if (Row.DropChance != ExpectedChance || Row.MinQuantity != 1 || Row.MaxQuantity != 1)
                { Error = TEXT("Goblin MON17.7 balance changed."); return false; }
            }
            if (Index != 1)
            {
                if (bAllRegistered ? (Row.ItemDefinitionAsset != nullptr || Row.ItemDefinitionId != Id) :
                    (Row.ItemDefinitionAsset.Get() != Source || !Row.ItemDefinitionId.IsNone()))
                { Error = TEXT("Nonselected loot changed outside complete X.61 cutover."); return false; }
                continue;
            }
            if (Row.ItemDefinitionAsset)
            {
                if (Row.ItemDefinitionAsset.Get() != Source || !Row.ItemDefinitionId.IsNone())
                { Error = TEXT("Stone source conflicts with exact historical row."); return false; }
                Count = 1;
            }
            else if (Row.ItemDefinitionId != Id)
            { Error = TEXT("Stone must have its exact persisted ItemId."); return false; }
            FGridResolvedItem Physical;
            if (!Registry->ResolveItem(Id, 0, Physical) || !Physical.IsValid() ||
                Physical.bIsArmament || Physical.ItemId != Id || Physical.ItemLevel != 0 ||
                Physical.ItemIcon.ToSoftObjectPath() != Source->Icon.ToSoftObjectPath() ||
                Physical.WorldMesh.ToSoftObjectPath() != Source->WorldMesh.ToSoftObjectPath() ||
                !Physical.DisplayName.EqualTo(Source->DisplayName) ||
                !FMath::IsNearlyEqual(Physical.WeightKg, Source->Weight) ||
                (Physical.LightEmitter.HasEmitter() && Physical.LightEmitter.bDefaultEnabled) != Source->IsLightEnabledByDefault())
            { Error = TEXT("Canonical Stone metadata differs from historical source."); return false; }
            FGridMonsterLootEntry& Target = Plan[Index];
            Target.ItemDefinitionId = Id;
            Target.ItemDefinitionAsset = nullptr;
            int32 Level = INDEX_NONE;
            if (!FGridMonsterLootResolver::TryResolveItemLevel(Target, nullptr, Level, Registry) || Level != 0)
            { Error = TEXT("Pointerless Stone level does not resolve."); return false; }
        }
        if (!SameRolls(Goblin.LootTable, Plan))
        { Error = TEXT("Deterministic loot rolls changed."); return false; }
        Old = Count;
        Out = MoveTemp(Plan);
        return true;
    }
};

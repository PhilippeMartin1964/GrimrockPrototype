#pragma once

#include "GridEditorNativeStoneLootPlan.h"
#include "Runtime/Items/GridCanonicalInventoryCategoryTags.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"

/** X.61: exact projection of the four real, inert historic loot sources.
 * No invented ARM, heal, visual, weight or gameplay tag. The added single
 * InventoryCategory tag has exclusively the existing UI category meaning.
 */
struct FGridEditorLootDefinitionProjection
{
    static constexpr int32 BaseCount = 34;
    static constexpr int32 AddedCount = 4;
    static constexpr int32 FinalCount = BaseCount + AddedCount;
    using FStone = FGridEditorNativeStoneLootPlan;

    static int32 SourceIndex(int32 Index)
    {
        static constexpr int32 Indices[] = {0, 2, 3, 4};
        check(Index >= 0 && Index < AddedCount);
        return Indices[Index];
    }
    static FName Id(int32 Index) { return FStone::ItemId(SourceIndex(Index)); }
    static EGridItemType LegacyType(int32 Index)
    {
        static const EGridItemType Types[] = {
            EGridItemType::Weapon, EGridItemType::Component,
            EGridItemType::Food, EGridItemType::Component };
        return Types[Index];
    }
    static FName CategoryTag(int32 Index)
    {
        using namespace GridCanonicalInventoryCategoryTags;
        return Index == 0 ? Weapon() : Index == 2 ? Food() : Component();
    }
    static int32 Capacity(int32 Index)
    {
        static constexpr int32 Capacities[] = {1, 10, 99, 99};
        return Capacities[Index];
    }
    static float Weight(int32 Index)
    {
        static constexpr float Weights[] = {0.5f, 0.1f, 0.f, 0.f};
        return Weights[Index];
    }
    static UClass* ClassFor(int32 Index)
    {
        if (Index == 0) return UGridNonArmamentDefinitionAsset::StaticClass();
        if (Index == 2) return UGridConsumableDefinitionAsset::StaticClass();
        return UGridGenericStackableDefinitionAsset::StaticClass();
    }
    static FString TargetName(int32 Index) { return TEXT("DA_Canonical_") + Id(Index).ToString(); }
    static FString TargetPackage(int32 Index)
    {
        return TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/") + TargetName(Index);
    }
    static FString TargetPath(int32 Index) { return TargetPackage(Index) + TEXT(".") + TargetName(Index); }

    static bool CopyExact(const UObject& Source, UObject& Target,
        const TCHAR* SourceName, const TCHAR* TargetName)
    {
        const FProperty* A = Source.GetClass()->FindPropertyByName(FName(SourceName));
        const FProperty* B = Target.GetClass()->FindPropertyByName(FName(TargetName));
        if (!A || !B || !B->SameType(A)) return false;
        const void* V = A->ContainerPtrToValuePtr<void>(&Source);
        void* W = B->ContainerPtrToValuePtr<void>(&Target);
        B->CopyCompleteValue(W, V);
        return B->Identical(V, W);
    }

    static bool CopyThrow(const UGridItemDefinitionAsset& Old, FGridNonArmamentThrowPolicy& New)
    {
        New.bCombatRecoverable = Old.bCombatThrowWeapon;
        New.InitialSpeed = Old.ThrowSpeed;
        New.Arc = Old.ThrowArc;
        New.LifeSeconds = Old.ThrowLifeSeconds;
        New.ImpactDropOffset = Old.ThrowImpactDropOffset;
        New.VisualRelativeRotation = Old.ThrowVisualRelativeRotation;
        New.VisualRelativeScale = Old.ThrowVisualRelativeScale;
        New.MeshForwardAxis = Old.ProjectileMeshForwardAxis;
        New.VisualTumbleAxis = Old.ThrowVisualTumbleAxis;
        New.VisualTumbleDegreesPerSecond = Old.ThrowVisualTumbleDegreesPerSecond;
        New.VisualSpinDegreesPerSecond = Old.ThrowVisualSpinDegreesPerSecond;
        switch (Old.ThrowVisualMode)
        {
        case EGridThrowVisualMode::Stable: New.VisualMode = EGridNonArmamentThrowVisualMode::Stable; break;
        case EGridThrowVisualMode::Tumble: New.VisualMode = EGridNonArmamentThrowVisualMode::Tumble; break;
        case EGridThrowVisualMode::Spin: New.VisualMode = EGridNonArmamentThrowVisualMode::Spin; break;
        default: return false;
        }
        return true;
    }

    static bool SameSourceProperty(const UGridItemDefinitionAsset& A, const UGridItemDefinitionAsset& B, const TCHAR* Name)
    {
        const FProperty* Field = A.GetClass()->FindPropertyByName(FName(Name));
        return Field && A.GetClass() == B.GetClass() && Field->Identical_InContainer(&A, &B);
    }

    static bool SourceMatches(const UGridItemDefinitionAsset& Old, int32 Index)
    {
        const auto* Defaults = GetDefault<UGridItemDefinitionAsset>();
        FGridCombatActionDefinition Unused;
        return Old.GetPathName() == FStone::SourcePath(SourceIndex(Index)) &&
            !Old.GetOutermost()->IsDirty() &&
            Old.IsValidDefinition() && Old.ItemDefinitionId == Id(Index) &&
            Old.ItemType == LegacyType(Index) && Old.Weight == Weight(Index) &&
            Old.bStackable == (Index != 0) && Old.MaxStackSize == Capacity(Index) &&
            Old.GetEffectiveHandUsage() == EGridItemHandUsage::NotHandHeld &&
            Old.CompatibleEquipmentSlots.IsEmpty() &&
            !Old.CanEquipToSlot(EGridEquipmentSlot::MainHand) &&
            !Old.CanEquipToSlot(EGridEquipmentSlot::OffHand) &&
            !Old.IsPhysicallyThrowable() && !Old.IsCombatThrowable() &&
            !Old.bUseWeaponCatalogue && Old.WeaponArchetypeId.IsNone() &&
            Old.RequiredAmmunitionItemDefinitionId.IsNone() && Old.WeaponCatalogue.IsNull() &&
            !Old.bUseWeaponVisualCatalogue && Old.WeaponVisualCatalogue.IsNull() &&
            Old.CombatActions.IsEmpty() && !Old.bProvidesQuickItemCombatAction &&
            !Old.BuildQuickItemCombatActionDefinition(Unused) &&
            !Old.BuildInventoryCombatActionDefinition(Unused) &&
            !Old.bProvidesAttackPresentation && Old.QuickItemActionIdOverride.IsNone() &&
            Old.Icon.IsNull() && Old.WorldMesh.IsNull() && Old.EquippedMesh.IsNull() &&
            Old.ItemTags.IsEmpty() && Old.ReadText.IsEmpty() &&
            !Old.HasLightEmitter() && Old.VisualEffects.Effects.IsEmpty() &&
            !Old.bEnableWorldSparkle && Old.WorldSparkleMaterial.IsNull() &&
            !Old.bUseItemWeightAsWorldPhysicsMass && Old.WorldPhysicsInitialTiltDegrees == 0.f &&
            SameSourceProperty(Old, *Defaults, TEXT("EquipmentStatBonus")) &&
            SameSourceProperty(Old, *Defaults, TEXT("EquipmentResistanceBonus"));
    }

    static bool Project(const UGridItemDefinitionAsset& Old, int32 Index, UGridNonArmamentDefinitionAsset& New)
    {
        if (!SourceMatches(Old, Index) || New.GetClass() != ClassFor(Index)) return false;
        New.ItemId = Old.ItemDefinitionId;
        New.DisplayName = Old.DisplayName;
        New.Description = Old.Description;
        New.WeightKg = Old.Weight;
        New.Tags = Old.ItemTags;
        New.Tags.Add(CategoryTag(Index));
        New.Icon = Old.Icon;
        New.WorldMesh = Old.WorldMesh;
        New.bReadable = false;
        New.ActionBindings.Reset();
        const TCHAR* Fields[] = {
            TEXT("ReadText"), TEXT("LightEmitter"), TEXT("VisualEffects"),
            TEXT("bEnableWorldSparkle"), TEXT("WorldSparkleMaterial"),
            TEXT("WorldSparkleColor"), TEXT("WorldSparkleIntensity"), TEXT("WorldSparkleSpeed"),
            TEXT("WorldSparkleVariation"), TEXT("bUseItemWeightAsWorldPhysicsMass"),
            TEXT("WorldPhysicsInitialTiltDegrees")
        };
        for (const TCHAR* Field : Fields)
            if (!CopyExact(Old, New, Field, Field)) return false;
        if (!CopyThrow(Old, New.GenericThrow)) return false;
        if (auto* Stack = Cast<UGridGenericStackableDefinitionAsset>(&New)) Stack->MaxStack = Capacity(Index);
        if (auto* Consumable = Cast<UGridConsumableDefinitionAsset>(&New))
        {
            Consumable->MaxStack = Capacity(Index);
            Consumable->UseActions.Reset();
        }
        return New.HasValidIdentity();
    }

    static bool SameDefinition(const UGridNonArmamentDefinitionAsset& A, const UGridNonArmamentDefinitionAsset& B)
    {
        if (A.GetClass() != B.GetClass()) return false;
        for (TFieldIterator<FProperty> Field(A.GetClass()); Field; ++Field)
        {
            // Only the actual canonical definition inheritance; unrelated
            // asset bookkeeping (PrimaryAsset, UObject) is not authored here.
            if (!Field->GetOwnerStruct()->IsChildOf(UGridCanonicalItemDefinitionAsset::StaticClass()) ||
                Field->HasAnyPropertyFlags(CPF_Transient)) continue;
            if (!Field->Identical_InContainer(&A, &B)) return false;
        }
        return true;
    }

    static bool ResolveParity(const UGridItemSubsystem& Registry, const UGridItemDefinitionAsset& Old, int32 Index)
    {
        FGridResolvedItem Item;
        if (!Registry.ResolveItem(Id(Index), 0, Item) || !Item.IsValid() ||
            Item.bIsArmament || Item.ItemId != Id(Index) || Item.ItemLevel != 0 ||
            !Item.DisplayName.EqualTo(Old.DisplayName) || !Item.Description.EqualTo(Old.Description) ||
            Item.WeightKg != Old.Weight || Item.MaxStack != Capacity(Index) ||
            Item.Tags != TArray<FName>{CategoryTag(Index)} ||
            !Item.ItemIcon.IsNull() || !Item.WorldMesh.IsNull() || !Item.EquippedMesh.IsNull() ||
            !Item.AvailableActions.IsEmpty() || !Item.NonArmEquipmentSlots.IsEmpty() ||
            Item.bNonArmTwoHanded || Item.bReadable || Item.LightEmitter.HasEmitter() ||
            !Item.VisualEffects.Effects.IsEmpty() || Item.bEnableWorldSparkle ||
            Item.bUseItemWeightAsWorldPhysicsMass || Item.WorldPhysicsInitialTiltDegrees != 0.f)
            return false;
        FGridMonsterLootEntry Entry;
        Entry.ItemDefinitionId = Id(Index);
        Entry.DropChance = 1.f;
        int32 Level = INDEX_NONE;
        if (!FGridMonsterLootResolver::TryResolveItemLevel(Entry, nullptr, Level, &Registry) || Level != 0)
            return false;
        Entry.ItemLevel = 1;
        return !FGridMonsterLootResolver::TryResolveItemLevel(Entry, nullptr, Level, &Registry);
    }

    /** Strict all-or-none root state; the old 34 entries remain in order.
     * Existing four projections must compare field-by-field to a fresh
     * projection, rather than trusting names or the new count alone.
     */
    static bool BuildPreview(UGridItemSystemAsset& Root, UGridItemSystemAsset& Preview,
        FString& Error, bool bAllowAlchemyExtension = false)
    {
        Error.Reset();
        const int32 Count = Root.ItemDefinitions.Num();
        if (Count != BaseCount && Count != FinalCount && !(bAllowAlchemyExtension && Count == 48))
        { Error = TEXT("Partial/unexpected canonical root."); return false; }
        if (Root.GetOutermost()->IsDirty())
        { Error = TEXT("Source root dirty."); return false; }
        Preview.WeaponCatalogue = Root.WeaponCatalogue;
        Preview.WeaponVisualCatalogue = Root.WeaponVisualCatalogue;
        Preview.ActionCatalogue = Root.ActionCatalogue;
        Preview.ActionPresentationCatalogue = Root.ActionPresentationCatalogue;
        Preview.ItemDefinitions = Root.ItemDefinitions;
        TSet<FName> Existing;
        for (int32 I = 0; I < BaseCount; ++I)
        {
            const auto* Def = Root.ItemDefinitions[I].Get();
            if (!IsValid(Def) || Def->ItemId.IsNone() || Existing.Contains(Def->ItemId))
            { Error = TEXT("Bad original identity."); return false; }
            for (int32 J = 0; J < AddedCount; ++J)
                if (Def->ItemId == Id(J)) { Error = TEXT("Unexpected early loot identity."); return false; }
            Existing.Add(Def->ItemId);
        }
        for (int32 I = 0; I < AddedCount; ++I)
        {
            const auto* Old = FStone::LoadSource(SourceIndex(I));
            auto* Candidate = Cast<UGridNonArmamentDefinitionAsset>(
                NewObject<UObject>(&Preview, ClassFor(I), NAME_None, RF_Transient));
            if (!IsValid(Old) || !IsValid(Candidate) || !Project(*Old, I, *Candidate))
            { Error = FString::Printf(TEXT("Source projection refused: %s"), *Id(I).ToString()); return false; }
            if (Count >= FinalCount)
            {
                const auto* Actual = Cast<UGridNonArmamentDefinitionAsset>(Root.ItemDefinitions[BaseCount + I].Get());
                if (!IsValid(Actual) || Actual->GetPathName() != TargetPath(I) ||
                    Actual->GetOutermost()->IsDirty() || !SameDefinition(*Actual, *Candidate))
                { Error = FString::Printf(TEXT("Saved canonical loot differs: %s"), *Id(I).ToString()); return false; }
            }
            else Preview.ItemDefinitions.Add(Candidate);
        }
        TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
        TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
        TArray<FString> Errors;
        if (!Registry->RegisterItemSystem(&Preview, Errors))
        { Error = FString::Join(Errors, TEXT("\n")); return false; }
        for (int32 I = 0; I < AddedCount; ++I)
        {
            const auto* Old = FStone::LoadSource(SourceIndex(I));
            if (!IsValid(Old) || !ResolveParity(*Registry.Get(), *Old, I))
            { Error = FString::Printf(TEXT("Resolved parity refused: %s"), *Id(I).ToString()); return false; }
        }
        return !Root.GetOutermost()->IsDirty();
    }

    static bool BuildLootTables(const UGridMonsterDefinitionAsset& Goblin,
        const UGridMonsterDefinitionAsset& Rat, bool bAlreadyMigrated,
        TArray<FGridMonsterLootEntry>& GoblinPlan, TArray<FGridMonsterLootEntry>& RatPlan, FString& Error)
    {
        GoblinPlan.Reset();
        RatPlan.Reset();
        if (Goblin.GetPathName() != FString(FStone::MonsterPath(0)) ||
            Rat.GetPathName() != FString(FStone::MonsterPath(1)) ||
            Goblin.GetOutermost()->IsDirty() || Rat.GetOutermost()->IsDirty() ||
            Goblin.LootTable.Num() != 3 || Rat.LootTable.Num() != 2 ||
            Goblin.ExperienceReward != 125 || Rat.ExperienceReward != 500)
        { Error = TEXT("Unexpected monster source baseline."); return false; }
        GoblinPlan = Goblin.LootTable;
        RatPlan = Rat.LootTable;
        for (int32 J = 0; J < 5; ++J)
        {
            const auto& Original = J < 3 ? Goblin.LootTable[J] : Rat.LootTable[J-3];
            auto& Target = J < 3 ? GoblinPlan[J] : RatPlan[J-3];
            const FName ItemId = FStone::ItemId(J);
            if (!Original.IsValidDefinition() || Original.GetResolvedItemDefinitionId() != ItemId ||
                Original.ItemLevel != 0 || (J < 3 &&
                    (Original.DropChance != (J == 1 ? 0.5f : 0.25f) ||
                     Original.MinQuantity != 1 || Original.MaxQuantity != 1)))
            { Error = TEXT("Loot ID, order, level or Goblin balance differs."); return false; }
            if (J == 1 || bAlreadyMigrated)
            {
                if (Original.ItemDefinitionAsset || Original.ItemDefinitionId != ItemId)
                { Error = TEXT("Expected fully canonical persisted loot row."); return false; }
            }
            else
            {
                const auto* Old = FStone::LoadSource(J);
                if (!IsValid(Old) || Original.ItemDefinitionAsset.Get() != Old || !Original.ItemDefinitionId.IsNone())
                { Error = TEXT("Expected exact old loot source."); return false; }
            }
            Target.ItemDefinitionId = ItemId;
            Target.ItemDefinitionAsset = nullptr;
        }
        if (!FStone::SameRolls(Goblin.LootTable, GoblinPlan) ||
            !FStone::SameRolls(Rat.LootTable, RatPlan))
        { Error = TEXT("Deterministic rolls differ."); return false; }
        return true;
    }

    static FString RootManifest(const UGridItemSystemAsset& Root)
    {
        FString Result;
        for (TFieldIterator<FProperty> Field(Root.GetClass(), EFieldIteratorFlags::ExcludeSuper); Field; ++Field)
        {
            if (Field->HasAnyPropertyFlags(CPF_Transient) ||
                Field->GetFName() == GET_MEMBER_NAME_CHECKED(UGridItemSystemAsset, ItemDefinitions)) continue;
            FString Value;
            Field->ExportTextItem_Direct(Value, Field->ContainerPtrToValuePtr<void>(&Root),
                nullptr, const_cast<UGridItemSystemAsset*>(&Root), PPF_None);
            Result += Field->GetName() + TEXT("=") + Value + TEXT("\n");
        }
        for (int32 I = 0; I < BaseCount; ++I)
            Result += Root.ItemDefinitions[I]->GetPathName() + TEXT("\n");
        for (int32 I = 0; I < AddedCount; ++I)
            Result += TargetPath(I) + TEXT("\n");
        return Result;
    }
};

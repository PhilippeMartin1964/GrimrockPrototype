#include "RPG/RPGStartingEquipmentPresetCatalog.h"

#include "RPG/RPGClassAsset.h"
#include "GridStartingKitEquipmentPolicyReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UI/UILocalizationText.h"

namespace RPGStartingPresets
{
    // ARMEMENT01.13: the eighteen approved N1 orientations. These are
    // source-controlled recipes, not talent grants or equipment restrictions.
    // Missing future support items are not silently created.
    // Production recipes encode canonical ItemIds and physical levels only.
    struct FApprovedItemSpec
    {
        const TCHAR* ItemId;
        EGridEquipmentSlot Slot;
        int32 Quantity = 1;
        int32 ItemLevel = 0;
    };

    struct FApprovedKitSpec
    {
        const TCHAR* ClassId;
        const TCHAR* BranchId;
        FApprovedItemSpec First;
        FApprovedItemSpec Second;
        FApprovedItemSpec Third;
        FApprovedItemSpec Fourth;
    };

    const FApprovedKitSpec ApprovedKits[] =
    {
        {TEXT("Warrior"), TEXT("Guardian"),
            {TEXT("Weapon_StraightSword_01"), EGridEquipmentSlot::MainHand, 1, 1}, {TEXT("Weapon_RoundShield_01"), EGridEquipmentSlot::OffHand, 1, 1},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Warrior"), TEXT("Breaker"),
            {TEXT("Weapon_GreatAxe_01"), EGridEquipmentSlot::MainHand, 1, 1}, {nullptr, EGridEquipmentSlot::None},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Warrior"), TEXT("WeaponMaster"),
            {TEXT("Weapon_LongSword_01"), EGridEquipmentSlot::MainHand, 1, 1}, {TEXT("Weapon_WarPick_01"), EGridEquipmentSlot::None, 1, 1},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Rogue"), TEXT("Shadow"),
            {TEXT("Weapon_Dagger_01"), EGridEquipmentSlot::MainHand, 1, 1}, {TEXT("Weapon_MainGauche_01"), EGridEquipmentSlot::OffHand, 1, 1},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Rogue"), TEXT("Assassin"),
            {TEXT("Weapon_AssassinDagger_01"), EGridEquipmentSlot::MainHand, 1, 1}, {TEXT("Weapon_ThrowingKnife_01"), EGridEquipmentSlot::None, 1, 1},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        // ARMEMENT01.16.3: real reusable N1 tools in the bag. No automatic
        // Skill_Traps rank or level-2 talent is granted by a starter kit.
        {TEXT("Rogue"), TEXT("Saboteur"),
            {TEXT("Weapon_Stiletto_01"), EGridEquipmentSlot::MainHand, 1, 1}, {TEXT("Item_DisarmTools_01"), EGridEquipmentSlot::None, 1, 0},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Ranger"), TEXT("Marksman"),
            {TEXT("Weapon_ShortBow_01"), EGridEquipmentSlot::MainHand, 1, 1}, {TEXT("Weapon_Dagger_01"), EGridEquipmentSlot::None, 1, 1},
            {TEXT("Item_Arrows_01"), EGridEquipmentSlot::None, 12, 0}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Ranger"), TEXT("Hunter"),
            {TEXT("Weapon_ShortSpear_01"), EGridEquipmentSlot::MainHand, 1, 1}, {TEXT("Weapon_Javelin_01"), EGridEquipmentSlot::None, 1, 1},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Ranger"), TEXT("Scout"),
            {TEXT("Weapon_RecurveBow_01"), EGridEquipmentSlot::MainHand, 1, 1}, {TEXT("Weapon_Dagger_01"), EGridEquipmentSlot::None, 1, 1},
            {TEXT("Item_Torch"), EGridEquipmentSlot::None, 1, 0}, {TEXT("Item_Arrows_01"), EGridEquipmentSlot::None, 12, 0} },
        {TEXT("Mage"), TEXT("Arcanist"),
            {TEXT("Weapon_ArcaneWand_01"), EGridEquipmentSlot::MainHand, 1, 1}, {TEXT("Weapon_ArcaneOrb_01"), EGridEquipmentSlot::OffHand, 1, 1},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Mage"), TEXT("Evoker"),
            {TEXT("Weapon_FireWand_01"), EGridEquipmentSlot::MainHand, 1, 1}, {nullptr, EGridEquipmentSlot::None},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        // ARMEMENT01.17.2B: exactly one finished N1 consumable catalyst,
        // distinct from the N2+ Mage SurfaceWeaver talent abilities.
        {TEXT("Mage"), TEXT("SurfaceWeaver"),
            {TEXT("Weapon_ElementalStaff_01"), EGridEquipmentSlot::MainHand, 1, 1}, {nullptr, EGridEquipmentSlot::None},
            {TEXT("Item_SurfaceCatalyst_01"), EGridEquipmentSlot::None, 1, 0}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Priest"), TEXT("Protection"),
            {TEXT("Weapon_Mace_01"), EGridEquipmentSlot::MainHand, 1, 1}, {TEXT("Weapon_RoundShield_01"), EGridEquipmentSlot::OffHand, 1, 1},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Priest"), TEXT("Restoration"),
            {TEXT("Weapon_HolyScepter_01"), EGridEquipmentSlot::MainHand, 1, 1}, {TEXT("Item_Potion_Healing_01"), EGridEquipmentSlot::None, 1, 0},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Priest"), TEXT("Exorcism"),
            {TEXT("Weapon_HolyScepter_01"), EGridEquipmentSlot::MainHand, 1, 1}, {TEXT("Weapon_HolyAegis_01"), EGridEquipmentSlot::OffHand, 1, 1},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Alchemist"), TEXT("Apothecary"),
            {TEXT("Weapon_Dagger_01"), EGridEquipmentSlot::MainHand, 1, 1}, {TEXT("Item_Potion_Healing_01"), EGridEquipmentSlot::None, 1, 0},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Alchemist"), TEXT("Grenadier"),
            {TEXT("Weapon_Dagger_01"), EGridEquipmentSlot::MainHand, 1, 1}, {TEXT("Weapon_Dart_01"), EGridEquipmentSlot::None, 1, 1},
            {TEXT("Item_Bomb_Fire"), EGridEquipmentSlot::None, 1, 0}, {nullptr, EGridEquipmentSlot::None} },
        // ARMEMENT01.17.1: one finished Oil flask is a usable N1
        // QuickItem; it is not a crafting recipe or a granted N2 talent.
        {TEXT("Alchemist"), TEXT("Transmuter"),
            {TEXT("Weapon_Dagger_01"), EGridEquipmentSlot::MainHand, 1, 1}, {TEXT("Weapon_ArcaneOrb_01"), EGridEquipmentSlot::OffHand, 1, 1},
            {TEXT("Item_Flask_Oil"), EGridEquipmentSlot::None, 1, 0}, {nullptr, EGridEquipmentSlot::None} }
    };

    // Production recipes only: all source identities and physical levels are
    // authoritative in DA_ItemSystem. No Editor, Automation or packaged mode
    // may synthesize a legacy DA_Item_/DA_Weapon_ lookup.
    bool AddApprovedItem(
        FRPGStartingEquipmentKit& Kit, const FApprovedItemSpec& Spec,
        FString& OutError, const UGridItemSubsystem* Registry)
    {
        if (!Spec.ItemId) return true;
        const FName ItemId(Spec.ItemId);
        FGridResolvedItem Native;
        if (!IsValid(Registry) || !Registry->IsConfigured() ||
            ItemId.IsNone() || Spec.Quantity < 1 ||
            !Registry->ResolveItem(ItemId, Spec.ItemLevel, Native) ||
            !Native.IsValid() || Native.ItemId != ItemId ||
            Native.ItemLevel != Spec.ItemLevel ||
            Native.bIsArmament != (Spec.ItemLevel > 0) ||
            Spec.Quantity > FMath::Max(1, Native.MaxStack) ||
            (Spec.Slot != EGridEquipmentSlot::None &&
                (Spec.Quantity != 1 ||
                 !FGridStartingKitEquipmentPolicyReadModel::ResolveCanonical(
                     Registry, ItemId, Spec.ItemLevel, Spec.Slot).bResolved)))
        {
            OutError = FString::Printf(
                TEXT("Invalid canonical N%d starter item %s"),
                Spec.ItemLevel, *ItemId.ToString());
            return false;
        }
        if (ItemId == FName(TEXT("Item_Bomb_Fire")) ||
            ItemId == FName(TEXT("Item_Flask_Oil")))
        {
            FGridCombatActionDefinition Action;
            if (Native.AvailableActions.Num() != 1 ||
                !Registry->ResolveDetailedNonArmAction(
                    ItemId, Spec.ItemLevel, Native.AvailableActions[0].ActionId, Action) ||
                Action.SourcePolicy != EGridCombatActionSourcePolicy::QuickItem)
            {
                OutError = TEXT("Invalid native alchemy starting action.");
                return false;
            }
        }
        FRPGStartingEquipmentEntry& Entry = Kit.Items.AddDefaulted_GetRef();
        Entry.CanonicalItemId = ItemId;
        Entry.ItemLevel = Spec.ItemLevel;
        Entry.Quantity = Spec.Quantity;
        Entry.TargetSlot = Spec.Slot;
        return true;
    }

    bool IsSupportedClass(FName ClassId)
    {
        // All six production classes have three approved kits. Never consult
        // the obsolete two-kit list in packaged builds or Editor production.
        for (const FApprovedKitSpec& Spec : ApprovedKits)
        {
            if (ClassId == FName(Spec.ClassId))
            {
                return true;
            }
        }
        return false;
    }


}

bool FRPGStartingEquipmentPresetCatalog::IsProductionClassAsset(const URPGClassAsset* ClassAsset)
{
    if (!IsValid(ClassAsset) ||
        !RPGStartingPresets::IsSupportedClass(ClassAsset->ClassId))
    {
        return false;
    }

    const FString Id = ClassAsset->ClassId.ToString();
    const FString Expected = FString::Printf(
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_%s.DA_Class_%s"),
        *Id, *Id);
    return ClassAsset->GetPathName() == Expected;
}

bool FRPGStartingEquipmentPresetCatalog::TryPopulateTransientClass(
    URPGClassAsset& TransientClass, FString& OutError)
{
    (void)TransientClass;
    // Backward-compatible entry point deliberately fails closed. The old
    // two-kit recipes and DA_Item_*/DA_Weapon_* references no longer exist
    // in the runtime module, including Development Editor Automation.
    OutError = TEXT("Obsolete two-kit materialization has been retired; use the canonical ItemSystem.");
    return false;
}

bool FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(
    URPGClassAsset& TransientClass, FString& OutError, const UGridItemSubsystem* CanonicalRegistry)
{
    OutError.Reset();
    if (!RPGStartingPresets::IsSupportedClass(TransientClass.ClassId))
    {
        OutError = TEXT("Unsupported production class for approved starter kits.");
        return false;
    }
    // Production classes are never modified; only an unconfigured transient
    // copy may be materialized. Explicitly authored kits have precedence.
    if (!TransientClass.StartingEquipmentKits.IsEmpty() ||
        !TransientClass.DefaultStartingEquipmentKitId.IsNone())
    {
        return TransientClass.IsValidDefinition();
    }

    // No test-only exception: every approved recipe must be materialized
    // from a configured canonical ItemSystem, including Automation builds.
    if (!IsValid(CanonicalRegistry) || !CanonicalRegistry->IsConfigured())
    {
        OutError = TEXT("Approved starter kits require a configured canonical ItemSystem.");
        return false;
    }
    TArray<FRPGStartingEquipmentKit> Candidate;
    FName DefaultId = NAME_None;
    for (const RPGStartingPresets::FApprovedKitSpec& Spec : RPGStartingPresets::ApprovedKits)
    {
        if (TransientClass.ClassId != FName(Spec.ClassId))
        {
            continue;
        }
        FRPGStartingEquipmentKit& Kit = Candidate.AddDefaulted_GetRef();
        Kit.KitId = FName(*FString::Printf(TEXT("Kit_%s_%s"), Spec.ClassId, Spec.BranchId));
        const FString LabelKey = FString::Printf(TEXT("CharacterCreation.StartingKit.Name.Kit.%s.%s"), Spec.ClassId, Spec.BranchId);
        Kit.DisplayName = FUILocalizationText::Get(*LabelKey);
        if (Kit.DisplayName.IsEmpty() ||
            !RPGStartingPresets::AddApprovedItem(Kit, Spec.First, OutError, CanonicalRegistry) ||
            !RPGStartingPresets::AddApprovedItem(Kit, Spec.Second, OutError, CanonicalRegistry) ||
            !RPGStartingPresets::AddApprovedItem(Kit, Spec.Third, OutError, CanonicalRegistry) ||
            !RPGStartingPresets::AddApprovedItem(Kit, Spec.Fourth, OutError, CanonicalRegistry))
        {
            if (OutError.IsEmpty())
            {
                OutError = FString::Printf(TEXT("Missing localized name for %s"), *Kit.KitId.ToString());
            }
            return false;
        }
        if (DefaultId.IsNone())
        {
            // The approved list is ordered with one preferred orientation first.
            DefaultId = Kit.KitId;
        }
    }
    if (Candidate.Num() != 3 || DefaultId.IsNone())
    {
        OutError = TEXT("Class approved starter kit catalogue is incomplete.");
        return false;
    }

    TransientClass.StartingEquipmentKits = MoveTemp(Candidate);
    TransientClass.DefaultStartingEquipmentKitId = DefaultId;
    if (!TransientClass.IsValidDefinition())
    {
        TransientClass.StartingEquipmentKits.Reset();
        TransientClass.DefaultStartingEquipmentKitId = NAME_None;
        OutError = TEXT("Approved starter kits failed class validation.");
        return false;
    }
    return true;
}

#include "RPG/RPGStartingEquipmentPresetCatalog.h"

#include "RPG/RPGClassAsset.h"
#include "GridStartingKitEquipmentPolicyReadModel.h"
#include "GridApprovedStarterLegacyFixtureGate.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UI/UILocalizationText.h"
#include "UObject/UObjectGlobals.h"

namespace RPGStartingPresets
{
    struct FItemSpec
    {
        const TCHAR* AssetName;
        EGridEquipmentSlot Slot;
        int32 Quantity = 1;
        // Explicit canonical physical identity for approved production recipes.
        const TCHAR* ItemId = nullptr;
        int32 ItemLevel = 0;
    };

#if WITH_EDITOR
    // Old two-kit authoring fixtures never ship in the packaged game.
    struct FKitSpec
    {
        const TCHAR* ClassId;
        const TCHAR* KitId;
        const TCHAR* DisplayNameKey;
        FItemSpec First;
        FItemSpec Second;
        bool bDefault;
    };

    // ARMEMENT-LOT02.14: production starter kits consume canonical ARM054,
    // not the obsolete unversioned ThrowingKnife item. Transient kits only.
    // Other class weapons require their own validated production assets.
    // All starting weapons inherit their definition's level-1 default.
    const FKitSpec Kits[] =
    {
        { TEXT("Warrior"), TEXT("Warrior_Defender"), TEXT("CharacterCreation.StartingKit.Name.WarriorDefender"),
            {TEXT("DA_Weapon_Dagger_01"), EGridEquipmentSlot::MainHand},
            {TEXT("DA_Weapon_RoundShield_01"), EGridEquipmentSlot::OffHand}, true },
        { TEXT("Warrior"), TEXT("Warrior_Skirmisher"), TEXT("CharacterCreation.StartingKit.Name.WarriorSkirmisher"),
            {TEXT("DA_Weapon_Dagger_01"), EGridEquipmentSlot::MainHand},
            {TEXT("DA_Weapon_ThrowingKnife_01"), EGridEquipmentSlot::None}, false },

        { TEXT("Rogue"), TEXT("Rogue_Shadow"), TEXT("CharacterCreation.StartingKit.Name.RogueShadow"),
            {TEXT("DA_Weapon_Dagger_01"), EGridEquipmentSlot::MainHand},
            {TEXT("DA_Weapon_ThrowingKnife_01"), EGridEquipmentSlot::None}, true },
        { TEXT("Rogue"), TEXT("Rogue_Explorer"), TEXT("CharacterCreation.StartingKit.Name.RogueExplorer"),
            {TEXT("DA_Weapon_Dagger_01"), EGridEquipmentSlot::MainHand},
            {TEXT("DA_Item_Torch"), EGridEquipmentSlot::None}, false },

        { TEXT("Ranger"), TEXT("Ranger_Archer"), TEXT("CharacterCreation.StartingKit.Name.RangerArcher"),
            {TEXT("DA_Weapon_ShortBow_01"), EGridEquipmentSlot::MainHand},
            {TEXT("DA_Weapon_Dagger_01"), EGridEquipmentSlot::None}, true },
        { TEXT("Ranger"), TEXT("Ranger_Pathfinder"), TEXT("CharacterCreation.StartingKit.Name.RangerPathfinder"),
            {TEXT("DA_Weapon_Dagger_01"), EGridEquipmentSlot::MainHand},
            {TEXT("DA_Item_Torch"), EGridEquipmentSlot::None}, false },

        { TEXT("Mage"), TEXT("Mage_Scholar"), TEXT("CharacterCreation.StartingKit.Name.MageScholar"),
            {TEXT("DA_Weapon_Dagger_01"), EGridEquipmentSlot::None},
            {TEXT("DA_Item_BlueGem"), EGridEquipmentSlot::None}, true },
        { TEXT("Mage"), TEXT("Mage_Wanderer"), TEXT("CharacterCreation.StartingKit.Name.MageWanderer"),
            {TEXT("DA_Weapon_Dagger_01"), EGridEquipmentSlot::None},
            {TEXT("DA_Item_Torch"), EGridEquipmentSlot::None}, false },

        { TEXT("Priest"), TEXT("Priest_Pilgrim"), TEXT("CharacterCreation.StartingKit.Name.PriestPilgrim"),
            {TEXT("DA_Weapon_Dagger_01"), EGridEquipmentSlot::MainHand},
            {TEXT("DA_Item_Torch"), EGridEquipmentSlot::None}, true },
        { TEXT("Priest"), TEXT("Priest_Protector"), TEXT("CharacterCreation.StartingKit.Name.PriestProtector"),
            {TEXT("DA_Weapon_Dagger_01"), EGridEquipmentSlot::MainHand},
            {TEXT("DA_Weapon_RoundShield_01"), EGridEquipmentSlot::OffHand}, false },

        { TEXT("Alchemist"), TEXT("Alchemist_Apothecary"), TEXT("CharacterCreation.StartingKit.Name.AlchemistApothecary"),
            {TEXT("DA_Weapon_Dagger_01"), EGridEquipmentSlot::None},
            {TEXT("DA_Item_EmptyVial"), EGridEquipmentSlot::None}, true },
        { TEXT("Alchemist"), TEXT("Alchemist_Firestarter"), TEXT("CharacterCreation.StartingKit.Name.AlchemistFirestarter"),
            {TEXT("DA_Weapon_Dagger_01"), EGridEquipmentSlot::None},
            {TEXT("DA_Item_Bomb_Fire"), EGridEquipmentSlot::None}, false }
    };


#endif // WITH_EDITOR: historical two-kit recipes

    // ARMEMENT01.13: the eighteen approved N1 orientations. These are
    // source-controlled recipes, not talent grants or equipment restrictions.
    // Missing future support items are not silently created.
    // Production recipes encode canonical ItemIds only. Historical package
    // names are synthesized solely when isolated Automation compares old
    // authoring fixtures against the registered ItemSystem.
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

    bool AddItem(FRPGStartingEquipmentKit& Kit, const FItemSpec& Spec, FString& OutError,
        const UGridItemSubsystem* Registry = nullptr);

    bool AddApprovedItem(FRPGStartingEquipmentKit& Kit, const FApprovedItemSpec& Spec, FString& OutError,
        const UGridItemSubsystem* Registry)
    {
        if (!Spec.ItemId)
        {
            return true;
        }
        // A live kit is always resolved by canonical ItemId. Old DA_* names
        // are never stored in approved production recipes. The synthetic
        // authoring lookup remains reachable only by Automation without a
        // registered ItemSystem and must preserve the exact old source name.
        if (!Registry && !FGridApprovedStarterLegacyFixtureGate::Allows(GIsAutomationTesting))
        {
            OutError = TEXT("Unregistered starter kit fixtures require Automation.");
            return false;
        }
        const FString FixtureAssetName = Registry
            ? FString(Spec.ItemId)
            : FString::Printf(TEXT("DA_%s"), Spec.ItemId);
        const FItemSpec SourceSpec{
            *FixtureAssetName, Spec.Slot, Spec.Quantity, Spec.ItemId, Spec.ItemLevel};
        if (Spec.Quantity < 1 || !AddItem(Kit, SourceSpec, OutError, Registry))
        {
            return false;
        }
        FRPGStartingEquipmentEntry& Entry = Kit.Items.Last();
        const UGridItemDefinitionAsset* Definition = Entry.ItemDefinition.Get();
        FGridResolvedItem Canonical;
        const bool bValidQuantity = !Entry.CanonicalItemId.IsNone()
            ? Registry && Registry->ResolveItem(Entry.CanonicalItemId, Entry.ItemLevel, Canonical) &&
                Spec.Quantity <= Canonical.MaxStack
            : IsValid(Definition) && (Spec.Quantity == 1 ||
                (Definition->bStackable && Spec.Quantity <= Definition->MaxStackSize));
        if (!bValidQuantity || (Spec.Quantity > 1 && Spec.Slot != EGridEquipmentSlot::None))
        {
            OutError = FString::Printf(TEXT("Invalid approved starter quantity: %s"), Spec.ItemId);
            Kit.Items.Pop();
            return false;
        }
        Entry.Quantity = Spec.Quantity;
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

#if WITH_EDITOR
    UGridItemDefinitionAsset* ResolveItem(const TCHAR* AssetName)
    {
        const bool bWeapon = FCString::Strncmp(AssetName, TEXT("DA_Weapon_"), 10) == 0;
        const TCHAR* Folder = bWeapon ? TEXT("Weapons") : TEXT("Items");
        const FString Path = FString::Printf(
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/%s/%s.%s"),
            Folder, AssetName, AssetName);
        return LoadObject<UGridItemDefinitionAsset>(nullptr, *Path);
    }

#endif // WITH_EDITOR: old source path loader

    bool AddItem(
        FRPGStartingEquipmentKit& Kit, const FItemSpec& Spec,
        FString& OutError, const UGridItemSubsystem* Registry)
    {
        if (Registry)
        {
            const FName ItemId(Spec.ItemId);
            FGridResolvedItem Resolved;
            if (ItemId.IsNone() || !Registry->ResolveItem(ItemId, Spec.ItemLevel, Resolved) ||
                !Resolved.IsValid() || Resolved.bIsArmament != (Spec.ItemLevel == 1) ||
                Spec.Quantity < 1 || Spec.Quantity > FMath::Max(1, Resolved.MaxStack) ||
                (Spec.Slot != EGridEquipmentSlot::None &&
                    (Spec.Quantity != 1 || !FGridStartingKitEquipmentPolicyReadModel::ResolveCanonical(
                        Registry, ItemId, Spec.ItemLevel, Spec.Slot).bResolved)))
            {
                OutError = FString::Printf(TEXT("Invalid canonical starter: %s N%d"), *ItemId.ToString(), Spec.ItemLevel);
                return false; // Missing registry entry is NOT permission to load an old asset.
            }
            if (ItemId == FName(TEXT("Item_Bomb_Fire")) || ItemId == FName(TEXT("Item_Flask_Oil")))
            {
                FGridCombatActionDefinition Action;
                if (Resolved.AvailableActions.Num() != 1 ||
                    !Registry->ResolveDetailedNonArmAction(ItemId, Spec.ItemLevel, Resolved.AvailableActions[0].ActionId, Action) ||
                    Action.SourcePolicy != EGridCombatActionSourcePolicy::QuickItem)
                {
                    OutError = TEXT("Invalid canonical alchemy starter action.");
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
#if WITH_EDITOR
        // Only Editor fixtures/audits can load the old path. Approved
        // starter kits also require GIsAutomationTesting when registry-less.
        // Shipping never contains this source lookup.
        UGridItemDefinitionAsset* Definition = ResolveItem(Spec.AssetName);
        if (!IsValid(Definition) || Definition->ItemDefinitionId.IsNone() ||
            (Spec.Slot != EGridEquipmentSlot::None && !Definition->CanEquipToSlot(Spec.Slot)))
        {
            OutError = FString::Printf(
                TEXT("Unusable production kit item: %s"), Spec.AssetName);
            return false;
        }
        // Every kit instance is quantity one. The fixed RPG level is resolved
        // by the validated ARMEMENT01.9.1 service at character creation.
        FRPGStartingEquipmentEntry& Entry = Kit.Items.AddDefaulted_GetRef();
        Entry.ItemDefinition = Definition;
        Entry.ItemLevel = 0;
        Entry.Quantity = 1;
        Entry.TargetSlot = Spec.Slot;
        return true;
#else
        OutError = TEXT("Historical kit definitions are Editor-only.");
        return false;
#endif
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
    OutError.Reset();
#if !WITH_EDITOR
    // Retained as a source-compatible API, but never materialize obsolete
    // DA_Weapon/DA_Item assets in a packaged game.
    OutError = TEXT("Historical two-kit previews are Editor-only.");
    return false;
#else
    if (!RPGStartingPresets::IsSupportedClass(TransientClass.ClassId))
    {
        OutError = FString::Printf(
            TEXT("Unsupported production class: %s"), *TransientClass.ClassId.ToString());
        return false;
    }

    // Production data can override every built-in recipe without being
    // overwritten. Mixed states are intentionally rejected, not repaired.
    if (!TransientClass.StartingEquipmentKits.IsEmpty() ||
        !TransientClass.DefaultStartingEquipmentKitId.IsNone())
    {
        return TransientClass.IsValidDefinition();
    }

    TArray<FRPGStartingEquipmentKit> Candidate;
    FName DefaultId = NAME_None;
    for (const RPGStartingPresets::FKitSpec& Spec : RPGStartingPresets::Kits)
    {
        if (TransientClass.ClassId != FName(Spec.ClassId))
        {
            continue;
        }
        FRPGStartingEquipmentKit& Kit = Candidate.AddDefaulted_GetRef();
        Kit.KitId = FName(Spec.KitId);
        Kit.DisplayName = FUILocalizationText::Get(Spec.DisplayNameKey);
        if (!RPGStartingPresets::AddItem(Kit, Spec.First, OutError) ||
            !RPGStartingPresets::AddItem(Kit, Spec.Second, OutError))
        {
            return false;
        }
        if (Spec.bDefault)
        {
            DefaultId = Kit.KitId;
        }
    }
    if (Candidate.Num() != 2 || DefaultId.IsNone())
    {
        OutError = TEXT("Class kit catalogue is incomplete.");
        return false;
    }

    // A transient copy is the only mutation target. Roll back if the
    // definition's complete validation detects an incompatible kit.
    TransientClass.StartingEquipmentKits = MoveTemp(Candidate);
    TransientClass.DefaultStartingEquipmentKitId = DefaultId;
    if (!TransientClass.IsValidDefinition())
    {
        TransientClass.StartingEquipmentKits.Reset();
        TransientClass.DefaultStartingEquipmentKitId = NAME_None;
        OutError = TEXT("Generated starting kits failed class validation.");
        return false;
    }
    return true;
#endif // WITH_EDITOR
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

    // A Development Editor build also compiles Automation tests. That is
    // NOT permission to materialize the old 18-kit definitions during
    // ordinary Editor use without a configured canonical ItemSystem.
    if (!CanonicalRegistry &&
        !FGridApprovedStarterLegacyFixtureGate::Allows(GIsAutomationTesting))
    {
        OutError = TEXT("Production starter kits require the canonical ItemSystem.");
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

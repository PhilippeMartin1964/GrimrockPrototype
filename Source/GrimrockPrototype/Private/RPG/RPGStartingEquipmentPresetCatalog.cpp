#include "RPG/RPGStartingEquipmentPresetCatalog.h"

#include "RPG/RPGClassAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UI/UILocalizationText.h"
#include "UObject/UObjectGlobals.h"

namespace RPGStartingPresets
{
    struct FItemSpec
    {
        const TCHAR* AssetName;
        EGridEquipmentSlot Slot;
        int32 Quantity = 1;
    };

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


    // ARMEMENT01.13: the eighteen approved N1 orientations. These are
    // source-controlled recipes, not talent grants or equipment restrictions.
    // Missing future support items are not silently created.
    struct FApprovedKitSpec
    {
        const TCHAR* ClassId;
        const TCHAR* BranchId;
        FItemSpec First;
        FItemSpec Second;
        FItemSpec Third;
        FItemSpec Fourth;
    };

    const FApprovedKitSpec ApprovedKits[] =
    {
        {TEXT("Warrior"), TEXT("Guardian"),
            {TEXT("DA_Weapon_StraightSword_01"), EGridEquipmentSlot::MainHand}, {TEXT("DA_Weapon_RoundShield_01"), EGridEquipmentSlot::OffHand},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Warrior"), TEXT("Breaker"),
            {TEXT("DA_Weapon_GreatAxe_01"), EGridEquipmentSlot::MainHand}, {nullptr, EGridEquipmentSlot::None},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Warrior"), TEXT("WeaponMaster"),
            {TEXT("DA_Weapon_LongSword_01"), EGridEquipmentSlot::MainHand}, {TEXT("DA_Weapon_WarPick_01"), EGridEquipmentSlot::None},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Rogue"), TEXT("Shadow"),
            {TEXT("DA_Weapon_Dagger_01"), EGridEquipmentSlot::MainHand}, {TEXT("DA_Weapon_MainGauche_01"), EGridEquipmentSlot::OffHand},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Rogue"), TEXT("Assassin"),
            {TEXT("DA_Weapon_AssassinDagger_01"), EGridEquipmentSlot::MainHand}, {TEXT("DA_Weapon_ThrowingKnife_01"), EGridEquipmentSlot::None},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Rogue"), TEXT("Saboteur"),
            {TEXT("DA_Weapon_Stiletto_01"), EGridEquipmentSlot::MainHand}, {nullptr, EGridEquipmentSlot::None},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Ranger"), TEXT("Marksman"),
            {TEXT("DA_Weapon_ShortBow_01"), EGridEquipmentSlot::MainHand}, {TEXT("DA_Weapon_Dagger_01"), EGridEquipmentSlot::None},
            {TEXT("DA_Item_Arrows_01"), EGridEquipmentSlot::None, 12}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Ranger"), TEXT("Hunter"),
            {TEXT("DA_Weapon_ShortSpear_01"), EGridEquipmentSlot::MainHand}, {TEXT("DA_Weapon_Javelin_01"), EGridEquipmentSlot::None},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Ranger"), TEXT("Scout"),
            {TEXT("DA_Weapon_RecurveBow_01"), EGridEquipmentSlot::MainHand}, {TEXT("DA_Weapon_Dagger_01"), EGridEquipmentSlot::None},
            {TEXT("DA_Item_Torch"), EGridEquipmentSlot::None}, {TEXT("DA_Item_Arrows_01"), EGridEquipmentSlot::None, 12} },
        {TEXT("Mage"), TEXT("Arcanist"),
            {TEXT("DA_Weapon_ArcaneWand_01"), EGridEquipmentSlot::MainHand}, {TEXT("DA_Weapon_ArcaneOrb_01"), EGridEquipmentSlot::OffHand},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Mage"), TEXT("Evoker"),
            {TEXT("DA_Weapon_FireWand_01"), EGridEquipmentSlot::MainHand}, {nullptr, EGridEquipmentSlot::None},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Mage"), TEXT("SurfaceWeaver"),
            {TEXT("DA_Weapon_ElementalStaff_01"), EGridEquipmentSlot::MainHand}, {nullptr, EGridEquipmentSlot::None},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Priest"), TEXT("Protection"),
            {TEXT("DA_Weapon_Mace_01"), EGridEquipmentSlot::MainHand}, {TEXT("DA_Weapon_RoundShield_01"), EGridEquipmentSlot::OffHand},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Priest"), TEXT("Restoration"),
            {TEXT("DA_Weapon_HolyScepter_01"), EGridEquipmentSlot::MainHand}, {nullptr, EGridEquipmentSlot::None},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Priest"), TEXT("Exorcism"),
            {TEXT("DA_Weapon_HolyScepter_01"), EGridEquipmentSlot::MainHand}, {TEXT("DA_Weapon_HolyAegis_01"), EGridEquipmentSlot::OffHand},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Alchemist"), TEXT("Apothecary"),
            {TEXT("DA_Weapon_Dagger_01"), EGridEquipmentSlot::MainHand}, {nullptr, EGridEquipmentSlot::None},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Alchemist"), TEXT("Grenadier"),
            {TEXT("DA_Weapon_Dagger_01"), EGridEquipmentSlot::MainHand}, {TEXT("DA_Weapon_Dart_01"), EGridEquipmentSlot::None},
            {TEXT("DA_Item_Bomb_Fire"), EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} },
        {TEXT("Alchemist"), TEXT("Transmuter"),
            {TEXT("DA_Weapon_Dagger_01"), EGridEquipmentSlot::MainHand}, {TEXT("DA_Weapon_ArcaneOrb_01"), EGridEquipmentSlot::OffHand},
            {nullptr, EGridEquipmentSlot::None}, {nullptr, EGridEquipmentSlot::None} }
    };

    bool AddItem(FRPGStartingEquipmentKit& Kit, const FItemSpec& Spec, FString& OutError);

    bool AddApprovedItem(FRPGStartingEquipmentKit& Kit, const FItemSpec& Spec, FString& OutError)
    {
        if (!Spec.AssetName)
        {
            return true;
        }
        if (Spec.Quantity < 1 || !AddItem(Kit, Spec, OutError))
        {
            return false;
        }
        FRPGStartingEquipmentEntry& Entry = Kit.Items.Last();
        UGridItemDefinitionAsset* Definition = Entry.ItemDefinition.Get();
        if ((Spec.Quantity > 1 && (!Definition->bStackable || Spec.Quantity > Definition->MaxStackSize)) ||
            (Spec.Quantity > 1 && Spec.Slot != EGridEquipmentSlot::None))
        {
            OutError = FString::Printf(TEXT("Invalid approved starter quantity: %s"), Spec.AssetName);
            Kit.Items.Pop();
            return false;
        }
        Entry.Quantity = Spec.Quantity;
        return true;
    }

    bool IsSupportedClass(FName ClassId)
    {
        for (const FKitSpec& Spec : Kits)
        {
            if (ClassId == FName(Spec.ClassId))
            {
                return true;
            }
        }
        return false;
    }

    UGridItemDefinitionAsset* ResolveItem(const TCHAR* AssetName)
    {
        const bool bWeapon = FCString::Strncmp(AssetName, TEXT("DA_Weapon_"), 10) == 0;
        const TCHAR* Folder = bWeapon ? TEXT("Weapons") : TEXT("Items");
        const FString Path = FString::Printf(
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/%s/%s.%s"),
            Folder, AssetName, AssetName);
        return LoadObject<UGridItemDefinitionAsset>(nullptr, *Path);
    }

    bool AddItem(
        FRPGStartingEquipmentKit& Kit, const FItemSpec& Spec,
        FString& OutError)
    {
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
    }
}

UGridItemDefinitionAsset* FRPGStartingEquipmentPresetCatalog::ResolveKnownStartingItemDefinition(FName ItemDefinitionId)
{
    if (ItemDefinitionId.IsNone())
    {
        return nullptr;
    }
    // LOT02.5: exact, approved Marksman starter ammunition identity.
    // Never resolve arbitrary unknown save IDs or substitute another item.
    if (ItemDefinitionId == FName(TEXT("Item_Arrows_01")))
    {
        UGridItemDefinitionAsset* Arrows = LoadObject<UGridItemDefinitionAsset>(
            nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Arrows_01.DA_Item_Arrows_01"));
        return IsValid(Arrows) && Arrows->ItemDefinitionId == ItemDefinitionId &&
            Arrows->IsValidDefinition() ? Arrows : nullptr;
    }
    // Only definitions explicitly referenced by a production kit can be
    // loaded as a fallback. A save must never silently substitute another
    // object with a mismatched ItemDefinitionId.
    TSet<FString> AttemptedAssetNames;
    for (const RPGStartingPresets::FKitSpec& Kit : RPGStartingPresets::Kits)
    {
        const RPGStartingPresets::FItemSpec Items[] = {Kit.First, Kit.Second};
        for (const RPGStartingPresets::FItemSpec& Item : Items)
        {
            const FString AssetName(Item.AssetName);
            if (AttemptedAssetNames.Contains(AssetName))
            {
                continue;
            }
            AttemptedAssetNames.Add(AssetName);
            UGridItemDefinitionAsset* Definition = RPGStartingPresets::ResolveItem(Item.AssetName);
            if (IsValid(Definition) && Definition->ItemDefinitionId == ItemDefinitionId)
            {
                return Definition;
            }
        }
    }
    // New-game orientation items also need exact-identity SaveGame rehydration.
    for (const RPGStartingPresets::FApprovedKitSpec& Kit : RPGStartingPresets::ApprovedKits)
    {
        const RPGStartingPresets::FItemSpec Items[] = {Kit.First, Kit.Second, Kit.Third, Kit.Fourth};
        for (const RPGStartingPresets::FItemSpec& Item : Items)
        {
            if (!Item.AssetName)
            {
                continue;
            }
            const FString AssetName(Item.AssetName);
            if (AttemptedAssetNames.Contains(AssetName))
            {
                continue;
            }
            AttemptedAssetNames.Add(AssetName);
            UGridItemDefinitionAsset* Definition = RPGStartingPresets::ResolveItem(Item.AssetName);
            if (IsValid(Definition) && Definition->ItemDefinitionId == ItemDefinitionId)
            {
                return Definition;
            }
        }
    }
    return nullptr;
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
}

bool FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(
    URPGClassAsset& TransientClass, FString& OutError)
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
        const FString LabelKey = FString::Printf(TEXT("CharacterCreation.StartingKit.Name.%s"), *Kit.KitId.ToString());
        Kit.DisplayName = FUILocalizationText::Get(*LabelKey);
        if (Kit.DisplayName.IsEmpty() ||
            !RPGStartingPresets::AddApprovedItem(Kit, Spec.First, OutError) ||
            !RPGStartingPresets::AddApprovedItem(Kit, Spec.Second, OutError) ||
            !RPGStartingPresets::AddApprovedItem(Kit, Spec.Third, OutError) ||
            !RPGStartingPresets::AddApprovedItem(Kit, Spec.Fourth, OutError))
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

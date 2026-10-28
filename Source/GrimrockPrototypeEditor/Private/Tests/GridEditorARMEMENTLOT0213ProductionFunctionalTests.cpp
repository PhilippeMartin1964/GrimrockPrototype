#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridInventoryTypes.h"
#include "UI/GridWeaponTooltipProjection.h"
#include "UObject/UObjectGlobals.h"

/** LOT02.13: real production DataAssets, read-only UE matrix. PIE remains separate. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT0213ProductionMatrix,
    "Grimrock.RPG.ARMEMENT.LOT02.ProductionFunctionalMatrix",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENTLOT0213ProductionMatrix::RunTest(const FString&)
{
    struct FCase
    {
        const TCHAR* Id;
        const TCHAR* Kind;
        bool bPassive;
        bool bThrow;
        bool bTwoHand;
        bool bRanged;
    };
    const FCase Cases[] = {
        { TEXT("ARM038"), TEXT("MeleeTwoHand"),    false, false, true,  false },
        { TEXT("ARM048"), TEXT("Ranged"),          false, false, true,  true },
        { TEXT("ARM054"), TEXT("ThrowingOneHand"), false, true,  false, true },
        { TEXT("ARM057"), TEXT("ThrowingOneHand"), false, true,  false, true },
        { TEXT("ARM058"), TEXT("ThrowingOneHand"), false, true,  false, true },
        { TEXT("ARM061"), TEXT("MagicOneHand"),    false, false, false, true },
        { TEXT("ARM062"), TEXT("MagicOneHand"),    false, false, false, true },
        { TEXT("ARM067"), TEXT("MagicTwoHand"),    false, false, true,  true },
        { TEXT("ARM069"), TEXT("MagicOneHand"),    false, false, false, true },
        { TEXT("ARM070"), TEXT("FocusPassive"),    true,  false, false, false },
        { TEXT("ARM081"), TEXT("ShieldPassive"),   true,  false, false, false }
    };
    TArray<FGridWeaponPipelineRow> Manifest;
    FString Error;
    if (!TestTrue(TEXT("Manifest parses"), FGridWeaponPipelineManifest::Load(
            FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
    {
        AddError(Error);
        return false;
    }
    UGridWeaponCatalogueAsset* Catalogue = LoadObject<UGridWeaponCatalogueAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
    UGridWeaponVisualCatalogueAsset* Visuals = LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue"));
    if (!TestNotNull(TEXT("Canonical N1-N20 catalogue"), Catalogue) ||
        !TestNotNull(TEXT("Canonical visual catalogue"), Visuals)) return false;

    const bool bCatalogueDirty = Catalogue->GetOutermost()->IsDirty();
    const bool bVisualDirty = Visuals->GetOutermost()->IsDirty();
    int32 Ready = 0, LevelsTotal = 0;
    TArray<FString> Report = {
        TEXT("ArchetypeId,BuildKind,Levels,Added,Equipped,Unequipped,ThrowQuick,Ammo,Status")
    };
    for (const FCase& Case : Cases)
    {
        const FString Id(Case.Id);
        const FGridWeaponPipelineRow* Row =
            FGridWeaponPipelineManifest::Find(Manifest, Id);
        if (!TestNotNull(*FString::Printf(TEXT("%s manifest row"), *Id), Row))
        {
            Report.Add(Id + TEXT(",MISSING,0,false,false,false,N_A,N_A,FAILED"));
            continue;
        }
        bool bOkay = true;
        auto Check = [this, &bOkay, &Id](const TCHAR* Label, bool Pass)
        {
            bOkay &= TestTrue(*FString::Printf(TEXT("%s %s"), *Id, Label), Pass);
        };
        Check(TEXT("must remain Planned"), Row->ProductionState == TEXT("Planned"));
        Check(TEXT("BuildKind"), Row->BuildKind == Case.Kind);
        Check(TEXT("passive/throw/two-hand/ranged modes"),
            Row->IsPassive() == Case.bPassive && Row->IsThrowable() == Case.bThrow &&
            Row->IsTwoHanded() == Case.bTwoHand && Row->UsesRangedAction() == Case.bRanged);
        UGridItemDefinitionAsset* Item = LoadObject<UGridItemDefinitionAsset>(nullptr, *Row->ItemPath);
        if (!TestNotNull(*FString::Printf(TEXT("%s real DataAsset"), *Id), Item))
        {
            Report.Add(Id + TEXT(",MISSING,0,false,false,false,N_A,N_A,FAILED"));
            continue;
        }
        const bool bItemDirty = Item->GetOutermost()->IsDirty();
        Check(TEXT("definition valid"), Item->IsValidDefinition());
        Check(TEXT("IDs"), Item->ItemDefinitionId == FName(Row->ItemDefinitionId) &&
            Item->WeaponArchetypeId == FName(Row->ArchetypeId));
        Check(TEXT("catalogue opt-ins"), Item->bUseWeaponCatalogue && Item->bUseWeaponVisualCatalogue);
        Check(TEXT("fixed N1 default"), Item->DefaultWeaponItemLevel == 1);
        Check(TEXT("mesh/icon binding"),
            Item->WorldMesh.ToSoftObjectPath() == FSoftObjectPath(Row->MeshPath) &&
            Item->Icon.ToSoftObjectPath() == FSoftObjectPath(Row->IconPath) &&
            Visuals->FindVisualId(FName(Row->ArchetypeId)) == FName(Row->VisualId));
        Check(TEXT("throw flag"), Item->bCombatThrowWeapon == Case.bThrow);
        const EGridEquipmentSlot Slot = Case.bPassive
            ? EGridEquipmentSlot::OffHand : EGridEquipmentSlot::MainHand;
        Check(TEXT("equip slot"), Item->CanEquipToSlot(Slot));
        Check(TEXT("hand usage"), Item->GetEffectiveHandUsage() ==
            (Case.bPassive ? EGridItemHandUsage::NotHandHeld :
             Case.bTwoHand ? EGridItemHandUsage::TwoHanded : EGridItemHandUsage::OneHanded));
        Check(TEXT("combat action count"), Item->CombatActions.Num() == (Case.bPassive ? 0 : 1));
        const FGridWeaponLevelProfile* N1 = Catalogue->FindProfile(FName(Row->ArchetypeId), 1);
        if (!TestNotNull(*FString::Printf(TEXT("%s N1"), *Id), N1)) continue;
        Check(TEXT("catalogue weight"), FMath::IsNearlyEqual(Item->Weight, N1->WeightKg, 0.0001f));
        if (Case.bPassive)
        {
            Check(TEXT("no passive attack"), !Item->CanProvideAttackFromSlot(Slot));
            Check(TEXT("canonical physical armor"), Item->EquipmentStatBonus.ArmorBonus == N1->PhysicalArmor);
            if (Row->IsFocus())
                Check(TEXT("canonical magical armor"),
                    Item->EquipmentStatBonus.MagicalArmorBonus == N1->MagicalArmor);
        }
        else if (Item->CombatActions.Num() == 1)
        {
            const FGridCombatActionDefinition& Action = Item->CombatActions[0];
            Check(TEXT("equipped combat action"),
                Action.IsValid() && Item->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand));
            Check(TEXT("action ID/AP/range"),
                Action.ActionId == FName(Row->AttackId) &&
                Action.ActionPointCost == N1->ActionPointCost && Action.RangeCells == N1->RangeCells);
            Check(TEXT("line of sight and type"),
                Action.bRequiresLineOfSight == Case.bRanged &&
                Action.ActionType == (Case.bRanged
                  ? EGridCombatActionType::RangedAttack : EGridCombatActionType::MeleeAttack));
            Check(TEXT("physical quantity cost"),
                Action.ResourceCosts.SourceItemQuantityCost == (Case.bThrow ? 1 : 0));
        }

        bool bQuick = false;
        if (Case.bThrow)
        {
            FGridCombatActionDefinition Quick;
            bQuick = Item->IsPhysicallyThrowable() &&
                Item->BuildInventoryCombatActionDefinition(Quick) &&
                Quick.IsValid() &&
                Quick.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem &&
                Quick.ResourceCosts.SourceItemQuantityCost == 1;
            Check(TEXT("quick inventory throw consumes one"), bQuick);
        }
        int32 Levels = 0;
        for (int32 Level = 1; Level <= 20; ++Level)
        {
            FGridItemInstance Instance;
            Instance.RuntimeObjectId = FGuid::NewGuid();
            Instance.ItemDefinitionId = Item->ItemDefinitionId;
            Instance.DisplayName = Item->DisplayName;
            Instance.ItemLevel = Level;
            Instance.Quantity = 1;
            Instance.OwnerType = EGridItemOwnerType::CharacterInventory;
            Instance.OwnerCharacterIndex = 0;
            FGridWeaponTooltipPresentation View;
            if (FGridWeaponTooltipProjection::Build(Item, Instance, View) &&
                View.bHasProfile && View.ItemLevel == Level &&
                View.ArchetypeId == FName(Case.Id) && View.bPassive == Case.bPassive)
                ++Levels;
            else
                Check(*FString::Printf(TEXT("N%d tooltip"), Level), false);
        }
        Check(TEXT("all 20 fixed-level tooltip projections"), Levels == 20);
        LevelsTotal += Levels;

        // Real inventory component roundtrip, without a game world.
        UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
        Inventory->InitializeDefaultPartyIfNeeded();
        const bool bRegistered = Inventory->RegisterItemDefinition(Item);
        Check(TEXT("register real item definition"), bRegistered);
        FGridItemInstance Physical;
        Physical.RuntimeObjectId = FGuid::NewGuid();
        Physical.ItemDefinitionId = Item->ItemDefinitionId;
        Physical.Quantity = 1;
        const bool bAdded = bRegistered &&
            Inventory->ApplyItemDefinitionToInstance(Physical) &&
            Inventory->AddItemToCharacterInventory(0, Physical);
        Check(TEXT("add to inventory"), bAdded);
        int32 Index = INDEX_NONE;
        if (bAdded)
        {
            const TArray<FGridInventorySlot>& Bag =
                Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots;
            for (int32 I = 0; I < Bag.Num(); ++I)
                if (!Bag[I].IsEmpty() && Bag[I].Item.RuntimeObjectId == Physical.RuntimeObjectId)
                { Index = I; break; }
        }
        Check(TEXT("real item in bag"), Index != INDEX_NONE);
        const bool bEquipped = Index != INDEX_NONE &&
            Inventory->EquipItemFromInventorySlot(0, Index, Slot);
        Check(TEXT("equip real item"), bEquipped);
        if (bEquipped)
        {
            FGridItemInstance Held;
            Check(TEXT("physical identity retained"),
                Inventory->GetEquippedItem(0, Slot, Held) &&
                Held.RuntimeObjectId == Physical.RuntimeObjectId);
            Check(TEXT("two-hand occupancy"), Inventory->IsTwoHandedEquipped(0) == Case.bTwoHand);
        }
        const bool bUnequipped = bEquipped &&
            Inventory->UnequipItemToInventory(0, Slot);
        Check(TEXT("unequip real item"), bUnequipped);
        if (bUnequipped)
        {
            int32 Copies = 0;
            for (const FGridInventorySlot& BagSlot :
                Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots)
                if (!BagSlot.IsEmpty() &&
                    BagSlot.Item.RuntimeObjectId == Physical.RuntimeObjectId) ++Copies;
            Check(TEXT("single physical item after roundtrip"), Copies == 1);
        }

        FString Ammo = TEXT("N_A");
        if (Row->BuildKind == TEXT("Ranged"))
        {
            Ammo = Item->RequiredAmmunitionItemDefinitionId == FName(TEXT("Item_Arrows_01"))
                ? TEXT("ARROWS_LINKED_PIE_REQUIRED") : TEXT("BLOCKED_MISSING_ARROWS_LINK");
            AddInfo(FString::Printf(TEXT("[LOT02.13] %s ammunition: %s"), *Id, *Ammo));
        }
        Check(TEXT("DataAsset unmodified"), Item->GetOutermost()->IsDirty() == bItemDirty);
        if (bOkay) ++Ready;
        const FString Status = !bOkay ? TEXT("FAILED") :
            Ammo == TEXT("BLOCKED_MISSING_ARROWS_LINK")
               ? TEXT("AUTOMATED_PASS_PIE_BLOCKED_AMMO")
               : TEXT("AUTOMATED_PASS_PIE_PENDING");
        Report.Add(FString::Printf(TEXT("%s,%s,%d,%s,%s,%s,%s,%s,%s"),
            *Id, *Row->BuildKind, Levels,
            bAdded ? TEXT("true") : TEXT("false"),
            bEquipped ? TEXT("true") : TEXT("false"),
            bUnequipped ? TEXT("true") : TEXT("false"),
            Case.bThrow ? (bQuick ? TEXT("true") : TEXT("false")) : TEXT("N_A"),
            *Ammo, *Status));
    }
    TestEqual(TEXT("Exactly eleven authored weapons validated"), Ready, 11);
    TestEqual(TEXT("220 real N1-N20 tooltip profiles validated"), LevelsTotal, 220);
    TestEqual(TEXT("RPG catalogue not modified"), Catalogue->GetOutermost()->IsDirty(), bCatalogueDirty);
    TestEqual(TEXT("Visual catalogue not modified"), Visuals->GetOutermost()->IsDirty(), bVisualDirty);
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ARMEMENT-LOT02"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString Output = FPaths::Combine(Directory, TEXT("LOT02_13_ProductionFunctionalMatrix.csv"));
    TestTrue(TEXT("Export real asset matrix"), FFileHelper::SaveStringArrayToFile(
        Report, *Output, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
    AddInfo(FString::Printf(TEXT("[LOT02.13] Assets=%d/11 TooltipLevels=%d/220 PIE=NOT_RUN Report=%s"),
        Ready, LevelsTotal, *Output));
    return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "GridEditorCanonicalStarterTestWorld.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridInventoryTypes.h"
#include "UI/GridWeaponTooltipProjection.h"
#include "UObject/UObjectGlobals.h"

/** LOT02.13: real canonical ItemSystem, read-only UE matrix. PIE remains separate. */
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
    FGridCanonicalRegistryFixture Registry;
    FGridEditorCanonicalStarterTestWorld World;
    if (!TestTrue(TEXT("Production ItemSystem config valid"), Registry.bReady) ||
        !TestTrue(TEXT("Canonical world inventory fixture"), World.Ready()))
        return false;

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
        Check(TEXT("manifest ARM is Active"), Row->ProductionState == TEXT("Active"));
        Check(TEXT("BuildKind unchanged"), Row->BuildKind == Case.Kind);
        Check(TEXT("passive / throw / two-hand / ranged are stable"),
            Row->IsPassive() == Case.bPassive && Row->IsThrowable() == Case.bThrow &&
            Row->IsTwoHanded() == Case.bTwoHand && Row->UsesRangedAction() == Case.bRanged);
        FGridResolvedItem Definition;
        if (!TestTrue(*FString::Printf(TEXT("%s actual canonical ARM N1"), *Id),
            Registry.Resolve(FName(Row->ItemDefinitionId), 1, Definition)))
        {
            Report.Add(Id + TEXT(",MISSING,0,false,false,false,N_A,N_A,FAILED"));
            continue;
        }
        Check(TEXT("canonical ARM identity"),
            Definition.bIsArmament && Definition.ItemId == FName(Row->ItemDefinitionId) &&
            Definition.ArchetypeId == FName(Row->ArchetypeId));
        Check(TEXT("world visual identity"),
            Definition.WorldMesh.ToSoftObjectPath() == FSoftObjectPath(Row->MeshPath) &&
            Definition.ItemIcon.ToSoftObjectPath() == FSoftObjectPath(Row->IconPath) &&
            Definition.VisualId == FName(Row->VisualId));
        Check(TEXT("specialized combat throw"),
            Definition.Weapon.Projectile.bEnabled == Case.bThrow);
        const EGridEquipmentSlot Slot = Case.bPassive
            ? EGridEquipmentSlot::OffHand : EGridEquipmentSlot::MainHand;
        Check(TEXT("physical equipment permission"),
            Definition.Weapon.CanEquipInHand(Slot));
        Check(TEXT("two-handed policy"),
            (Definition.Weapon.HandPolicy == EGridCanonicalHandPolicy::TwoHanded) == Case.bTwoHand);
        Check(TEXT("authored attack bindings"),
            Definition.AvailableActions.Num() == (Case.bPassive ? 0 : 1));
        if (Case.bPassive)
        {
            Check(TEXT("no invented passive attack source"),
                !Definition.Weapon.HasAttackSourceInHand(Slot));
        }
        else if (Definition.AvailableActions.Num() == 1)
        {
            FGridResolvedBoundAction Bound;
            const bool bAction = Registry.Registry->ResolveBoundAttackForHand(
                Definition.ItemId, 1, Definition.AvailableActions[0].ActionId,
                EGridEquipmentSlot::MainHand, Bound);
            Check(TEXT("physical MainHand action resolves"), bAction && Bound.IsValid());
            if (bAction)
            {
                Check(TEXT("source action identity"),
                    Bound.ActionId == FName(Row->AttackId));
                Check(TEXT("ranged or melee mode"),
                    Bound.Action.bRangedAttack == Case.bRanged);
                Check(TEXT("source quantity cost"),
                    Bound.Action.SourceItemQuantityCost == (Case.bThrow ? 1 : 0));
            }
        }
        const bool bQuick = !Case.bThrow ||
            (Definition.Weapon.Projectile.bEnabled &&
             Definition.Weapon.Projectile.InitialSpeed > 0.f);
        Check(TEXT("authored physical throw policy"), bQuick);

        int32 Levels = 0;
        for (int32 Level = 1; Level <= 20; ++Level)
        {
            FGridResolvedItem N;
            if (!Registry.Resolve(Definition.ItemId, Level, N))
            {
                Check(TEXT("Missing canonical N1-N20 profile"), false);
                continue;
            }
            FGridItemInstance Instance;
            Instance.RuntimeObjectId = FGuid::NewGuid();
            Instance.ItemDefinitionId = Definition.ItemId;
            Instance.ItemLevel = Level;
            Instance.Quantity = 1;
            Instance.OwnerType = EGridItemOwnerType::CharacterInventory;
            Instance.OwnerCharacterIndex = 0;
            FGridWeaponTooltipPresentation View;
            if (FGridWeaponTooltipProjection::Build(
                nullptr, Instance, View, Registry.Registry.Get()) &&
                View.bHasProfile && View.ItemLevel == Level &&
                View.ArchetypeId == FName(Case.Id) && View.bPassive == Case.bPassive)
                ++Levels;
            else
                Check(TEXT("N1-N20 canonical tooltip profile failed"), false);
        }
        Check(TEXT("All twenty tooltip levels"), Levels == 20);
        LevelsTotal += Levels;

        UGridPartyInventoryComponent* Inventory = World.MakeInventory();
        if (!TestNotNull(TEXT("World-bound physical inventory"), Inventory)) continue;
        FGridItemInstance Physical;
        Physical.RuntimeObjectId = FGuid::NewGuid();
        Physical.ItemDefinitionId = Definition.ItemId;
        Physical.ItemLevel = 1;
        Physical.Quantity = 1;
        const bool bAdded = Inventory->ApplyItemDefinitionToInstance(Physical) &&
            Inventory->AddItemToCharacterInventory(0, Physical);
        Check(TEXT("Physical item enters bag"), bAdded);
        int32 Index = INDEX_NONE;
        if (bAdded)
        {
            const TArray<FGridInventorySlot>& Bag =
                Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots;
            for (int32 I = 0; I < Bag.Num(); ++I)
                if (!Bag[I].IsEmpty() && Bag[I].Item.RuntimeObjectId == Physical.RuntimeObjectId)
                { Index = I; break; }
        }
        Check(TEXT("One real bag slot"), Index != INDEX_NONE);
        const bool bEquipped = Index != INDEX_NONE &&
            Inventory->EquipItemFromInventorySlot(0, Index, Slot);
        Check(TEXT("Real canonical equipment transaction"), bEquipped);
        if (bEquipped)
        {
            FGridItemInstance Held;
            Check(TEXT("Equipped GUID retained"),
                Inventory->GetEquippedItem(0, Slot, Held) &&
                Held.RuntimeObjectId == Physical.RuntimeObjectId);
            Check(TEXT("Physical two-hand occupancy"),
                Inventory->IsTwoHandedEquipped(0) == Case.bTwoHand);
        }
        const bool bUnequipped = bEquipped && Inventory->UnequipItemToInventory(0, Slot);
        Check(TEXT("Real canonical unequip transaction"), bUnequipped);
        if (bUnequipped)
        {
            int32 Count = 0;
            for (const FGridInventorySlot& BagSlot :
                Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots)
                if (!BagSlot.IsEmpty() && BagSlot.Item.RuntimeObjectId == Physical.RuntimeObjectId)
                    ++Count;
            Check(TEXT("One physical GUID after unequip"), Count == 1);
        }
        FString Ammo = TEXT("N_A");
        if (Row->BuildKind == TEXT("Ranged"))
        {
            Ammo = Definition.Weapon.RequiredAmmoKindId == FName(TEXT("Item_Arrows_01"))
                ? TEXT("ARROWS_LINKED_PIE_REQUIRED")
                : TEXT("BLOCKED_MISSING_ARROWS_LINK");
            Check(TEXT("Canonical bow requires arrows"), Ammo == TEXT("ARROWS_LINKED_PIE_REQUIRED"));
        }
        if (bOkay) ++Ready;
        const FString Status = bOkay ? TEXT("AUTOMATED_PASS_PIE_PENDING") : TEXT("FAILED");
        Report.Add(FString::Printf(TEXT("%s,%s,%d,%s,%s,%s,%s,%s,%s"),
            *Id, *Row->BuildKind, Levels,
            bAdded ? TEXT("true") : TEXT("false"),
            bEquipped ? TEXT("true") : TEXT("false"),
            bUnequipped ? TEXT("true") : TEXT("false"),
            Case.bThrow ? (bQuick ? TEXT("true") : TEXT("false")) : TEXT("N_A"),
            *Ammo, *Status));
    }
    TestEqual(TEXT("Exactly eleven canonical weapon cases"), Ready, 11);
    TestEqual(TEXT("All 220 N1-N20 weapon tooltips"), LevelsTotal, 220);
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ARMEMENT-LOT02"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString Output = FPaths::Combine(Directory, TEXT("LOT02_13_ProductionFunctionalMatrix.csv"));
    TestTrue(TEXT("Export canonical production matrix under Saved"),
        FFileHelper::SaveStringArrayToFile(
            Report, *Output, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
    AddInfo(FString::Printf(TEXT("[ARM-CLEAN06] ARM=%d/11 TooltipLevels=%d/220 PIE=NOT_RUN Report=%s"),
        Ready, LevelsTotal, *Output));
    return !HasAnyErrors();
}
#endif

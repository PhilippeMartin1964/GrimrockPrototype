#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "GridEditorCanonicalStarterTestWorld.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"
#include "Serialization/Csv/CsvParser.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENTLOT022
{
    // Approved data only: this table selects test candidates, not kit contents.
    const TCHAR* CandidateIds[] = {
        TEXT("Kit_Warrior_Guardian"),
        TEXT("Kit_Warrior_Breaker"),
        TEXT("Kit_Warrior_WeaponMaster"),
        TEXT("Kit_Rogue_Shadow"),
        TEXT("Kit_Priest_Protection")
    };

    struct FRecipe
    {
        FString ClassId;
        FString KitId;
        FName Primary;
        FName Secondary;
        FString SecondarySlot;
    };

    bool ReadApprovedCandidate(const FString& KitId, FRecipe& Out, FString& Error)
    {
        FString Text;
        if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectDir(),
                TEXT("docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv"))))
        {
            Error = TEXT("Approved recipe CSV is missing");
            return false;
        }
        FCsvParser Parser(Text);
        const auto& Rows = Parser.GetRows();
        int32 Matches = 0;
        for (int32 Index = 1; Index < Rows.Num(); ++Index)
        {
            const auto& Cells = Rows[Index];
            if (Cells.Num() != 15 || FString(Cells[2]) != KitId) continue;
            // The canonical CSV uses literal "None" and level 0 when a
            // kit has no second weapon (e.g. the two-handed Breaker).
            // "None" is not an empty CSV field and must not imply N1.
            const bool bHasSecondary =
                FCString::Strcmp(Cells[6], TEXT("None")) != 0 &&
                Cells[6][0] != 0;
            const bool bSecondaryValid = bHasSecondary
                ? (FCString::Atoi(Cells[7]) == 1 &&
                   (FCString::Strcmp(Cells[8], TEXT("Bag")) == 0 ||
                    FCString::Strcmp(Cells[8], TEXT("OffHand")) == 0))
                : (FCString::Atoi(Cells[7]) == 0 &&
                   FCString::Strcmp(Cells[8], TEXT("None")) == 0);
            if (FCString::Strcmp(Cells[13], TEXT("ApprovedDesign")) != 0 ||
                FCString::Strcmp(Cells[5], TEXT("MainHand")) != 0 ||
                FCString::Atoi(Cells[4]) != 1 ||
                !bSecondaryValid ||
                FCString::Strcmp(Cells[9], TEXT("None")) != 0 ||
                FCString::Strcmp(Cells[11], TEXT("None")) != 0)
            {
                Error = FString::Printf(
                    TEXT("Candidate recipe %s is incomplete or not approved"), *KitId);
                return false;
            }
            Out.ClassId = Cells[0];
            Out.KitId = Cells[2];
            Out.Primary = FName(Cells[3]);
            Out.Secondary = bHasSecondary ? FName(Cells[6]) : NAME_None;
            Out.SecondarySlot = Cells[8];
            ++Matches;
        }
        if (Matches != 1)
        {
            Error = FString::Printf(TEXT("Expected exactly one recipe for %s"), *KitId);
            return false;
        }
        return true;
    }

    bool ResolveActiveWeapon(FName Archetype,
        const TArray<FGridWeaponPipelineRow>& Manifest,
        const UGridItemSubsystem* Registry, FName& OutItemId,
        FGridResolvedItem& OutProfile)
    {
        OutItemId = NAME_None;
        OutProfile = FGridResolvedItem();
        const FGridWeaponPipelineRow* Row =
            FGridWeaponPipelineManifest::Find(Manifest, Archetype.ToString());
        if (!Registry || !Row || Row->ProductionState != TEXT("Active"))
            return false;
        const FName ItemId(*Row->ItemDefinitionId);
        if (!Registry->ResolveItem(ItemId, 1, OutProfile) ||
            !OutProfile.IsValid() || !OutProfile.bIsArmament ||
            OutProfile.ArchetypeId != Archetype || OutProfile.ItemLevel != 1)
            return false;
        OutItemId = ItemId;
        return true;
    }

    int32 CountPhysical(const FGridPartyInventoryState& Party, FName ItemId)
    {
        int32 Count = 0;
        const FGridCharacterInventoryState& Character = Party.ActiveCharacters[0];
        for (const FGridInventorySlot& Slot : Character.InventorySlots)
        {
            if (!Slot.IsEmpty() && Slot.Item.ItemDefinitionId == ItemId) ++Count;
        }
        const FGridCharacterEquipmentState& Gear = Party.ActiveEquipment[0];
        if (Gear.MainHand.IsValid() && Gear.MainHand.ItemDefinitionId == ItemId) ++Count;
        if (Gear.OffHand.IsValid() && Gear.OffHand.ItemDefinitionId == ItemId) ++Count;
        return Count;
    }

    bool CheckOne(const FRecipe& Spec,
                  const TArray<FGridWeaponPipelineRow>& Manifest,
                  const FGridEditorCanonicalStarterTestWorld& Fixture,
                  FAutomationTestBase& Test)
    {
        FName PrimaryId = NAME_None, SecondaryId = NAME_None;
        FGridResolvedItem Primary, Secondary;
        if (!Test.TestTrue(TEXT("Approved primary N1 resolved in production ItemSystem"),
            ResolveActiveWeapon(Spec.Primary, Manifest, Fixture.Registry, PrimaryId, Primary)))
            return false;
        if (!Test.TestTrue(TEXT("Canonical primary may occupy MainHand"),
            Primary.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand)))
            return false;
        const bool bHasSecondary = !Spec.Secondary.IsNone();
        if (bHasSecondary)
        {
            if (!Test.TestTrue(TEXT("Approved secondary N1 resolved in production ItemSystem"),
                ResolveActiveWeapon(Spec.Secondary, Manifest, Fixture.Registry, SecondaryId, Secondary)))
                return false;
            if (!Test.TestTrue(TEXT("Approved second slot remains Bag or OffHand"),
                Spec.SecondarySlot == TEXT("Bag") || Spec.SecondarySlot == TEXT("OffHand")))
                return false;
            if (Spec.SecondarySlot == TEXT("OffHand") &&
                !Test.TestTrue(TEXT("Second item is physically OffHand-compatible"),
                    Secondary.Weapon.CanEquipInHand(EGridEquipmentSlot::OffHand) &&
                    Primary.Weapon.HandPolicy != EGridHandPolicy::TwoHanded))
                return false;
        }

        const FString ClassPath = FString::Printf(
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_%s.DA_Class_%s"),
            *Spec.ClassId, *Spec.ClassId);
        URPGClassAsset* Production = LoadObject<URPGClassAsset>(nullptr, *ClassPath);
        URPGRaceAsset* Human = LoadObject<URPGRaceAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
        if (!Test.TestNotNull(TEXT("Real production class"), Production) ||
            !Test.TestNotNull(TEXT("Real production human race"), Human))
            return false;
        const bool bWasDirty = Production->GetOutermost()->IsDirty();
        const int32 PreviousKits = Production->StartingEquipmentKits.Num();
        const FName PreviousDefault = Production->DefaultStartingEquipmentKitId;

        // The actual NewGame path materializes approved canonical kits on a
        // transient class copy. Never build synthetic legacy item definitions.
        UGridPartyInventoryComponent* Party = Fixture.MakeInventory();
        if (!Test.TestNotNull(TEXT("Real GameInstance-bound inventory"), Party))
            return false;
        FRPGCharacterCreationRequest Request;
        Request.DisplayName = FText::FromString(TEXT("LOT02 canonical test"));
        Request.RaceDefinition = Human;
        Request.ClassDefinition = Production;
        Request.CombatActionSourceClassDefinition = Production;
        Request.StartingEquipmentKitId = FName(*Spec.KitId);
        FText Error;
        if (!Test.TestTrue(TEXT("Real NewGame grants approved ItemSystem kit"),
            Party->CreateInitialCharacter(Request, Error)))
        {
            Test.AddError(FString::Printf(TEXT("Creation failure for %s: %s"),
                *Spec.KitId, *Error.ToString()));
            return false;
        }

        const FGridPartyInventoryState& State = Party->PartyInventoryState;
        const FGridCharacterInventoryState& Hero = State.ActiveCharacters[0];
        const FGridCharacterEquipmentState& Gear = State.ActiveEquipment[0];
        Test.TestEqual(TEXT("Selected canonical kit ID is persisted"),
            Hero.StartingEquipmentKitId, Request.StartingEquipmentKitId);
        Test.TestEqual(TEXT("Primary equipped in MainHand from ItemSystem"),
            Gear.MainHand.ItemDefinitionId, PrimaryId);
        Test.TestEqual(TEXT("Primary physical N1 retained"), Gear.MainHand.ItemLevel, 1);
        Test.TestEqual(TEXT("One physical primary item, never duplicate"),
            CountPhysical(State, PrimaryId), 1);
        Test.TestNull(TEXT("No historical primary definition registered"),
            Party->FindItemDefinition(PrimaryId));
        if (Primary.Weapon.HandPolicy == EGridHandPolicy::TwoHanded)
            Test.TestFalse(TEXT("Two-handed primary leaves no second physical object"),
                Gear.OffHand.IsValid());

        if (bHasSecondary)
        {
            Test.TestEqual(TEXT("Secondary physical identity occurs once"),
                CountPhysical(State, SecondaryId), 1);
            Test.TestNull(TEXT("No historical secondary definition registered"),
                Party->FindItemDefinition(SecondaryId));
            if (Spec.SecondarySlot == TEXT("OffHand"))
            {
                Test.TestEqual(TEXT("OffHand item uses canonical ItemId"),
                    Gear.OffHand.ItemDefinitionId, SecondaryId);
                Test.TestEqual(TEXT("OffHand physical N1 retained"), Gear.OffHand.ItemLevel, 1);
            }
            else
            {
                const bool bInBag = Hero.InventorySlots.ContainsByPredicate(
                    [SecondaryId](const FGridInventorySlot& Slot)
                    {
                        return !Slot.IsEmpty() &&
                            Slot.Item.ItemDefinitionId == SecondaryId &&
                            Slot.Item.ItemLevel == 1;
                    });
                Test.TestTrue(TEXT("Canonical second weapon resides in bag at N1"), bInBag);
            }
        }
        FString OwnershipError;
        Test.TestTrue(TEXT("NewGame yields unique valid physical owners"),
            Party->ValidateInventoryOwnership(OwnershipError));

        // In-memory SaveGame round-trip: zero writes to user save slots.
        UGrimrockPartySaveGame* Snapshot = NewObject<UGrimrockPartySaveGame>();
        Snapshot->PartyInventoryState = State;
        UGridPartyInventoryComponent* Restored = Fixture.MakeInventory();
        if (!Test.TestNotNull(TEXT("GameInstance-bound restore destination"), Restored))
            return false;
        if (!Test.TestTrue(TEXT("Canonical SaveGame snapshot restores"),
            Restored->RestorePartyInventoryState(Snapshot->PartyInventoryState, Error)))
        {
            Test.AddError(FString::Printf(TEXT("Restore failure for %s: %s"),
                *Spec.KitId, *Error.ToString()));
            return false;
        }
        const FGridPartyInventoryState& After = Restored->PartyInventoryState;
        Test.TestEqual(TEXT("Kit identity survives the SaveGame round-trip"),
            After.ActiveCharacters[0].StartingEquipmentKitId, Request.StartingEquipmentKitId);
        Test.TestEqual(TEXT("MainHand GUID survives the round-trip"),
            After.ActiveEquipment[0].MainHand.RuntimeObjectId, Gear.MainHand.RuntimeObjectId);
        Test.TestEqual(TEXT("Physical MainHand level survives the round-trip"),
            After.ActiveEquipment[0].MainHand.ItemLevel, 1);
        if (bHasSecondary && Spec.SecondarySlot == TEXT("OffHand"))
        {
            Test.TestEqual(TEXT("OffHand GUID survives the round-trip"),
                After.ActiveEquipment[0].OffHand.RuntimeObjectId,
                Gear.OffHand.RuntimeObjectId);
        }
        Test.TestEqual(TEXT("Secondary identity survives restore"),
            bHasSecondary ? CountPhysical(After, SecondaryId) : 0,
            bHasSecondary ? 1 : 0);
        Test.TestNull(TEXT("Restored party has no historical primary definition"),
            Restored->FindItemDefinition(PrimaryId));
        Test.TestTrue(TEXT("Restored ownership is valid"),
            Restored->ValidateInventoryOwnership(OwnershipError));
        Test.TestEqual(TEXT("Production kits not modified"),
            Production->StartingEquipmentKits.Num(), PreviousKits);
        Test.TestEqual(TEXT("Production default kit not modified"),
            Production->DefaultStartingEquipmentKitId, PreviousDefault);
        Test.TestEqual(TEXT("Production class package stays clean"),
            Production->GetOutermost()->IsDirty(), bWasDirty);
        Test.AddInfo(FString::Printf(TEXT(
            "[LOT02.2] PASS CanonicalKit=%s Main=%s Second=%s WorldBound=1 SaveGameInMemory=1"),
            *Spec.KitId, *PrimaryId.ToString(),
            bHasSecondary ? *SecondaryId.ToString() : TEXT("None")));
        return true;
    }

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT022NewGame,
    "Grimrock.RPG.ARMEMENT.LOT02.NewGameFiveCandidateKits",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENTLOT022NewGame::RunTest(const FString&)
{
    TArray<FGridWeaponPipelineRow> Manifest;
    FString Error;
    if (!TestTrue(TEXT("Production manifest is readable"),
        FGridWeaponPipelineManifest::Load(
            FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
    {
        AddError(Error);
        return false;
    }
    FGridEditorCanonicalStarterTestWorld Fixture;
    if (!TestTrue(TEXT("Real world-bound production ItemSystem"), Fixture.Ready()))
        return false;
    bool bAllPassed = true;
    for (const TCHAR* KitId : ARMEMENTLOT022::CandidateIds)
    {
        ARMEMENTLOT022::FRecipe Recipe;
        if (!ARMEMENTLOT022::ReadApprovedCandidate(KitId, Recipe, Error))
        {
            AddError(Error);
            bAllPassed = false;
            continue;
        }
        bAllPassed &= ARMEMENTLOT022::CheckOne(Recipe, Manifest, Fixture, *this);
    }
    return bAllPassed && !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

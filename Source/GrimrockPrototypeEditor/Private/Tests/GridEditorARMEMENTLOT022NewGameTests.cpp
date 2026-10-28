#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
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

    UGridItemDefinitionAsset* ResolveActiveWeapon(
        FName Archetype, const TArray<FGridWeaponPipelineRow>& Manifest)
    {
        const FGridWeaponPipelineRow* Row =
            FGridWeaponPipelineManifest::Find(Manifest, Archetype.ToString());
        if (!Row || Row->ProductionState != TEXT("Active")) return nullptr;
        UGridItemDefinitionAsset* Item =
            LoadObject<UGridItemDefinitionAsset>(nullptr, *Row->ItemPath);
        if (!IsValid(Item) || !Item->IsValidDefinition() ||
            Item->ItemDefinitionId != FName(*Row->ItemDefinitionId) ||
            Item->WeaponArchetypeId != Archetype ||
            Item->DefaultWeaponItemLevel != 1) return nullptr;
        return Item;
    }

    void AddEntry(FRPGStartingEquipmentKit& Kit,
                  UGridItemDefinitionAsset* Item, EGridEquipmentSlot Slot)
    {
        FRPGStartingEquipmentEntry& Entry = Kit.Items.AddDefaulted_GetRef();
        Entry.ItemDefinition = Item;
        Entry.ItemLevel = 1;
        Entry.Quantity = 1;
        Entry.TargetSlot = Slot;
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
                  FAutomationTestBase& Test)
    {
        UGridItemDefinitionAsset* Primary = ResolveActiveWeapon(Spec.Primary, Manifest);
        UGridItemDefinitionAsset* Secondary = Spec.Secondary.IsNone()
            ? nullptr : ResolveActiveWeapon(Spec.Secondary, Manifest);
        if (!Test.TestNotNull(TEXT("Approved primary weapon is active"), Primary) ||
            (!Spec.Secondary.IsNone() &&
             !Test.TestNotNull(TEXT("Approved secondary weapon is active"), Secondary))) return false;

        const FString ClassPath = FString::Printf(
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_%s.DA_Class_%s"),
            *Spec.ClassId, *Spec.ClassId);
        URPGClassAsset* Production = LoadObject<URPGClassAsset>(nullptr, *ClassPath);
        if (!Test.TestNotNull(TEXT("Real production class exists"), Production)) return false;
        const bool bWasDirty = Production->GetOutermost()->IsDirty();
        const int32 PreviousKits = Production->StartingEquipmentKits.Num();
        const FName PreviousDefault = Production->DefaultStartingEquipmentKitId;

        URPGClassAsset* Temporary = DuplicateObject<URPGClassAsset>(Production, GetTransientPackage());
        if (!Test.TestNotNull(TEXT("Test class duplicated, not modified"), Temporary)) return false;
        FRPGStartingEquipmentKit& Kit = Temporary->StartingEquipmentKits.AddDefaulted_GetRef();
        Kit.KitId = FName(*Spec.KitId);
        Kit.DisplayName = FText::FromString(Spec.KitId); // Test-only, never displayed in UI.
        AddEntry(Kit, Primary, EGridEquipmentSlot::MainHand);
        if (Secondary)
        {
            if (!Test.TestTrue(TEXT("Approved second slot is Bag or OffHand"),
                Spec.SecondarySlot == TEXT("Bag") || Spec.SecondarySlot == TEXT("OffHand"))) return false;
            AddEntry(Kit, Secondary, Spec.SecondarySlot == TEXT("OffHand")
                ? EGridEquipmentSlot::OffHand : EGridEquipmentSlot::None);
        }
        Temporary->DefaultStartingEquipmentKitId = Kit.KitId;
        if (!Test.TestTrue(TEXT("Transient class with approved kit is valid"),
                Temporary->IsValidDefinition())) return false;

        URPGRaceAsset* Human = LoadObject<URPGRaceAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
        if (!Test.TestNotNull(TEXT("Human race exists"), Human)) return false;

        UGridPartyInventoryComponent* Party = NewObject<UGridPartyInventoryComponent>();
        Party->InitializeDefaultPartyIfNeeded();
        FRPGCharacterCreationRequest Request;
        Request.DisplayName = FText::FromString(TEXT("LOT02 Test"));
        Request.RaceDefinition = Human;
        Request.ClassDefinition = Temporary;
        Request.CombatActionSourceClassDefinition = Temporary;
        Request.StartingEquipmentKitId = Kit.KitId;
        FText Error;
        if (!Test.TestTrue(TEXT("Real new character creation grants approved kit"),
                Party->CreateInitialCharacter(Request, Error)))
        {
            Test.AddError(FString::Printf(TEXT("Creation failure for %s: %s"),
                *Spec.KitId, *Error.ToString()));
            return false;
        }

        const FGridPartyInventoryState& State = Party->PartyInventoryState;
        const FGridCharacterInventoryState& Hero = State.ActiveCharacters[0];
        const FGridCharacterEquipmentState& Gear = State.ActiveEquipment[0];
        Test.TestEqual(TEXT("Chosen kit ID persisted"), Hero.StartingEquipmentKitId, Kit.KitId);
        Test.TestEqual(TEXT("Primary equipped in MainHand"),
            Gear.MainHand.ItemDefinitionId, Primary->ItemDefinitionId);
        Test.TestEqual(TEXT("Primary N1 persisted"), Gear.MainHand.ItemLevel, 1);
        Test.TestEqual(TEXT("Primary is physically unique"),
            CountPhysical(State, Primary->ItemDefinitionId), 1);
        Test.TestTrue(TEXT("Primary combat definition registered"),
            Party->FindItemDefinition(Primary->ItemDefinitionId) == Primary);
        if (Secondary)
        {
            Test.TestEqual(TEXT("Secondary is physically unique"),
                CountPhysical(State, Secondary->ItemDefinitionId), 1);
            Test.TestTrue(TEXT("Secondary definition registered"),
                Party->FindItemDefinition(Secondary->ItemDefinitionId) == Secondary);
            if (Spec.SecondarySlot == TEXT("OffHand"))
            {
                Test.TestEqual(TEXT("Approved OffHand item equipped"),
                    Gear.OffHand.ItemDefinitionId, Secondary->ItemDefinitionId);
                Test.TestEqual(TEXT("Approved OffHand item level is N1"),
                    Gear.OffHand.ItemLevel, 1);
            }
            else
            {
                const bool bBagFound = Hero.InventorySlots.ContainsByPredicate(
                    [Secondary](const FGridInventorySlot& Slot)
                    {
                        return !Slot.IsEmpty() &&
                            Slot.Item.ItemDefinitionId == Secondary->ItemDefinitionId &&
                            Slot.Item.ItemLevel == 1;
                    });
                Test.TestTrue(TEXT("Secondary weapon present in bag at N1"), bBagFound);
            }
        }
        FString OwnershipError;
        Test.TestTrue(TEXT("Creation has unique valid owners"),
            Party->ValidateInventoryOwnership(OwnershipError));

        // In-memory SaveGame snapshot and restore: no game save slot is written.
        UGrimrockPartySaveGame* Snapshot = NewObject<UGrimrockPartySaveGame>();
        Snapshot->PartyInventoryState = State;
        UGridPartyInventoryComponent* Restored = NewObject<UGridPartyInventoryComponent>();
        Restored->InitializeDefaultPartyIfNeeded();
        if (!Test.TestTrue(TEXT("Party snapshot restores successfully"),
                Restored->RestorePartyInventoryState(Snapshot->PartyInventoryState, Error)))
        {
            Test.AddError(FString::Printf(TEXT("Restore failure for %s: %s"),
                *Spec.KitId, *Error.ToString()));
            return false;
        }
        const FGridPartyInventoryState& After = Restored->PartyInventoryState;
        Test.TestEqual(TEXT("Kit identity survives snapshot restore"),
            After.ActiveCharacters[0].StartingEquipmentKitId, Kit.KitId);
        Test.TestEqual(TEXT("MainHand GUID survives snapshot restore"),
            After.ActiveEquipment[0].MainHand.RuntimeObjectId,
            Gear.MainHand.RuntimeObjectId);
        if (Secondary && Spec.SecondarySlot == TEXT("OffHand"))
        {
            Test.TestEqual(TEXT("OffHand GUID survives snapshot restore"),
                After.ActiveEquipment[0].OffHand.RuntimeObjectId,
                Gear.OffHand.RuntimeObjectId);
        }
        Test.TestEqual(TEXT("Original production kits unchanged"),
            Production->StartingEquipmentKits.Num(), PreviousKits);
        Test.TestEqual(TEXT("Original production default unchanged"),
            Production->DefaultStartingEquipmentKitId, PreviousDefault);
        Test.TestEqual(TEXT("Original production package not dirtied"),
            Production->GetOutermost()->IsDirty(), bWasDirty);
        Test.AddInfo(FString::Printf(
            TEXT("[LOT02.2] PASS Kit=%s Main=%s Second=%s RuntimeEnabled=0"),
            *Spec.KitId, *Primary->ItemDefinitionId.ToString(),
            Secondary ? *Secondary->ItemDefinitionId.ToString() : TEXT("None")));
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
        bAllPassed &= ARMEMENTLOT022::CheckOne(Recipe, Manifest, *this);
    }
    return bAllPassed;
}

#endif // WITH_DEV_AUTOMATION_TESTS

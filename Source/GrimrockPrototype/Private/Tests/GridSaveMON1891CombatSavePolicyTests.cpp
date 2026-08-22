#if WITH_DEV_AUTOMATION_TESTS

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Magic/GridPartySpellbookComponent.h"
#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Save/GridCombatSavePolicy.h"
#include "Save/GrimrockPartySaveGame.h"

namespace
{
    struct FGridMON1891TestSaveSlots
    {
        FString MainSlot;
        FString AutoCombatSlot;
        int32 UserIndex = 0;

        FGridMON1891TestSaveSlots ()
            : MainSlot (FString::Printf (
                TEXT ("MON1891_%s"),
                *FGuid::NewGuid ().ToString (EGuidFormats::Digits)))
            , AutoCombatSlot (
                FGridCombatSavePolicy::BuildPreCombatCheckpointSlotName (
                    MainSlot))
        {
            UGameplayStatics::DeleteGameInSlot (MainSlot, UserIndex);
            UGameplayStatics::DeleteGameInSlot (AutoCombatSlot, UserIndex);
        }

        ~FGridMON1891TestSaveSlots ()
        {
            UGameplayStatics::DeleteGameInSlot (MainSlot, UserIndex);
            UGameplayStatics::DeleteGameInSlot (AutoCombatSlot, UserIndex);
        }
    };

    struct FGridMON1891TestWorld
    {
        UWorld* World = nullptr;

        FGridMON1891TestWorld ()
        {
            const UWorld::InitializationValues Values =
                UWorld::InitializationValues ()
                    .AllowAudioPlayback (false)
                    .RequiresHitProxies (false)
                    .CreatePhysicsScene (false)
                    .CreateNavigation (false)
                    .CreateAISystem (false)
                    .ShouldSimulatePhysics (false)
                    .SetTransactional (false);
            World = UWorld::CreateWorld (
                EWorldType::Game,
                false,
                FName (*FString::Printf (
                    TEXT ("MON1891TestWorld_%s"),
                    *FGuid::NewGuid ().ToString (EGuidFormats::Digits))),
                nullptr,
                true,
                ERHIFeatureLevel::Num,
                &Values);
            if (World && GEngine)
            {
                FWorldContext& Context =
                    GEngine->CreateNewWorldContext (EWorldType::Game);
                Context.SetCurrentWorld (World);
            }
        }

        ~FGridMON1891TestWorld ()
        {
            if (!World)
            {
                return;
            }
            World->DestroyWorld (false);
            if (GEngine)
            {
                GEngine->DestroyWorldContext (World);
            }
        }
    };

    UGridLevelAsset* BuildMON1891FloorLevel (UObject* Outer)
    {
        UGridLevelAsset* Level = NewObject<UGridLevelAsset> (Outer);
        if (!Level)
        {
            return nullptr;
        }
        Level->Width = 4;
        Level->Height = 4;
        Level->EnsureCellCount ();
        for (FGridLevelCellData& Cell : Level->Cells)
        {
            Cell.CellType = EGridCellType::Floor;
            Cell.bBlocksOccupancy = false;
        }
        return Level;
    }

    bool ConfigureMON1891PersistentParty (
        FAutomationTestBase& Test,
        FGridMON1891TestWorld& TestWorld,
        const FGridMON1891TestSaveSlots& Slots,
        AGridLevelRuntimeActor*& OutRuntime,
        AGrimrockPartyPawn*& OutParty,
        UGridTurnManagerComponent*& OutTurnManager)
    {
        OutRuntime = nullptr;
        OutParty = nullptr;
        OutTurnManager = nullptr;
        if (!TestWorld.World)
        {
            return false;
        }

        OutRuntime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ();
        OutParty = TestWorld.World->SpawnActor<AGrimrockPartyPawn> ();
        Test.TestNotNull (TEXT ("Runtime actor is created"), OutRuntime);
        Test.TestNotNull (TEXT ("Party pawn is created"), OutParty);
        if (!OutRuntime || !OutParty || !OutParty->PartyInventoryComponent)
        {
            return false;
        }

        OutRuntime->bApplyLevelStartOnBeginPlay = false;
        OutRuntime->LevelAsset = BuildMON1891FloorLevel (OutRuntime);
        OutRuntime->CurrentDungeonLevelId = TEXT ("MON18_9_1_TestLevel");
        OutRuntime->RebuildLevel ();
        OutParty->LevelRuntimeActor = OutRuntime;
        OutParty->PartySaveSlotName = Slots.MainSlot;
        OutParty->PartySaveUserIndex = Slots.UserIndex;
        OutParty->CurrentCellX = 1;
        OutParty->CurrentCellY = 1;
        OutParty->Facing = EGridEdge::North;

        UGridPartyInventoryComponent* Inventory =
            OutParty->PartyInventoryComponent.Get ();
        Inventory->InitializeDefaultPartyIfNeeded ();

        URPGRaceAsset* Race = NewObject<URPGRaceAsset> (OutParty);
        URPGClassAsset* CharacterClass = NewObject<URPGClassAsset> (OutParty);
        if (!Race || !CharacterClass)
        {
            return false;
        }
        Race->RaceId = TEXT ("MON1891_Human");
        Race->DisplayName = FText::FromString (TEXT ("Humain"));
        CharacterClass->ClassId = TEXT ("MON1891_Mage");
        CharacterClass->DisplayName = FText::FromString (TEXT ("Mage"));
        CharacterClass->BaseAttributes = FRPGAttributes { 8, 12, 10, 15, 12, 9 };
        CharacterClass->HealthAtLevelOne = 8;
        CharacterClass->HealthPerLevel = 4;

        FRPGCharacterCreationRequest Request;
        Request.DisplayName = FText::FromString (TEXT ("MON18.9.1 Hero"));
        Request.RaceDefinition = Race;
        Request.ClassDefinition = CharacterClass;
        FText CreationError;
        if (!Inventory->CreateInitialCharacter (Request, CreationError) ||
            Inventory->PartyInventoryState.ActiveCharacters.IsEmpty ())
        {
            Test.AddError (FString::Printf (
                TEXT ("Character creation failed: %s"),
                *CreationError.ToString ()));
            return false;
        }

        UGridPartySpellbookComponent* Spellbook =
            OutParty->FindComponentByClass<UGridPartySpellbookComponent> ();
        Test.TestNotNull (TEXT ("Native Spellbook component is available"), Spellbook);
        if (!Spellbook)
        {
            return false;
        }
        Spellbook->EnsureCharacterSpellbook (
            Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId);

        OutTurnManager = NewObject<UGridTurnManagerComponent> (
            OutRuntime,
            TEXT ("MON1891TurnManager"));
        Test.TestNotNull (TEXT ("TurnManager is created"), OutTurnManager);
        if (!OutTurnManager)
        {
            return false;
        }
        OutRuntime->AddInstanceComponent (OutTurnManager);
        OutTurnManager->RegisterComponent ();
        if (!OutTurnManager->InitializeTurnManager (OutRuntime, OutParty))
        {
            Test.AddError (TEXT ("TurnManager initialization failed."));
            return false;
        }
        OutTurnManager->CurrentPhase = EGridCombatPhase::Exploration;
        OutTurnManager->bCombatActive = false;
        return true;
    }

    const UGrimrockPartySaveGame* LoadMON1891Save (
        const FString& SlotName,
        int32 UserIndex)
    {
        return Cast<UGrimrockPartySaveGame> (
            UGameplayStatics::LoadGameFromSlot (SlotName, UserIndex));
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1891SaveOutsideCombatAcceptedTest,
    "Grimrock.Save.MON18.9.1.SaveOutsideCombatAccepted",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1891SaveOutsideCombatAcceptedTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON1891TestSaveSlots Slots;
    FGridMON1891TestWorld TestWorld;
    AGridLevelRuntimeActor* Runtime = nullptr;
    AGrimrockPartyPawn* Party = nullptr;
    UGridTurnManagerComponent* TurnManager = nullptr;
    if (!ConfigureMON1891PersistentParty (
            *this, TestWorld, Slots, Runtime, Party, TurnManager))
    {
        return false;
    }

    FText Error;
    TestTrue (TEXT ("Exploration save is accepted"), Party->SaveCurrentGame (Error));
    TestTrue (
        TEXT ("Exploration save exists on disk"),
        UGameplayStatics::DoesSaveGameExist (Slots.MainSlot, Slots.UserIndex));

    TurnManager->CurrentPhase = EGridCombatPhase::Victory;
    TestTrue (TEXT ("Victory is a stable saveable phase"), Party->SaveCurrentGame (Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1891SaveDuringCombatRejectedTest,
    "Grimrock.Save.MON18.9.1.SaveDuringCombatRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1891SaveDuringCombatRejectedTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON1891TestSaveSlots Slots;
    FGridMON1891TestWorld TestWorld;
    AGridLevelRuntimeActor* Runtime = nullptr;
    AGrimrockPartyPawn* Party = nullptr;
    UGridTurnManagerComponent* TurnManager = nullptr;
    if (!ConfigureMON1891PersistentParty (
            *this, TestWorld, Slots, Runtime, Party, TurnManager))
    {
        return false;
    }

    TurnManager->CurrentPhase = EGridCombatPhase::PlayerPhase;
    TurnManager->bCombatActive = true;
    FText Error;
    TestFalse (TEXT ("Regular save is rejected during combat"), Party->SaveCurrentGame (Error));
    TestFalse (
        TEXT ("Rejected combat save creates no file"),
        UGameplayStatics::DoesSaveGameExist (Slots.MainSlot, Slots.UserIndex));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1891PreCombatCheckpointCreatedTest,
    "Grimrock.Save.MON18.9.1.PreCombatCheckpointCreated",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1891PreCombatCheckpointCreatedTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON1891TestSaveSlots Slots;
    FGridMON1891TestWorld TestWorld;
    AGridLevelRuntimeActor* Runtime = nullptr;
    AGrimrockPartyPawn* Party = nullptr;
    UGridTurnManagerComponent* TurnManager = nullptr;
    if (!ConfigureMON1891PersistentParty (
            *this, TestWorld, Slots, Runtime, Party, TurnManager))
    {
        return false;
    }

    FText Error;
    bool bSkipped = false;
    TestTrue (
        TEXT ("Pre-combat checkpoint succeeds"),
        FGridCombatSavePolicy::PreparePreCombatCheckpoint (
            Party, Error, bSkipped));
    TestFalse (TEXT ("Persistent party checkpoint is not skipped"), bSkipped);
    TestEqual (
        TEXT ("Checkpoint does not replace the main slot identity"),
        Party->PartySaveSlotName,
        Slots.MainSlot);
    TestTrue (
        TEXT ("Auto-combat checkpoint exists"),
        UGameplayStatics::DoesSaveGameExist (
            Slots.AutoCombatSlot,
            Slots.UserIndex));

    const UGrimrockPartySaveGame* Checkpoint =
        LoadMON1891Save (Slots.AutoCombatSlot, Slots.UserIndex);
    TestNotNull (TEXT ("Checkpoint is readable"), Checkpoint);
    TestTrue (
        TEXT ("Checkpoint captures the stable party cell"),
        Checkpoint && Checkpoint->PartyCellX == 1 && Checkpoint->PartyCellY == 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1891TransientCheckpointSkippedTest,
    "Grimrock.Save.MON18.9.1.TransientCheckpointSkipped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1891TransientCheckpointSkippedTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON1891TestWorld TestWorld;
    TestNotNull (TEXT ("Transient test world is created"), TestWorld.World);
    if (!TestWorld.World)
    {
        return false;
    }
    AGrimrockPartyPawn* Party =
        TestWorld.World->SpawnActor<AGrimrockPartyPawn> ();
    TestNotNull (TEXT ("Transient party can be created"), Party);
    if (!Party)
    {
        return false;
    }
    Party->PartySaveSlotName.Empty ();

    FText Error;
    bool bSkipped = false;
    TestTrue (
        TEXT ("Transient combat fixture does not require disk checkpoint"),
        FGridCombatSavePolicy::PreparePreCombatCheckpoint (
            Party, Error, bSkipped));
    TestTrue (TEXT ("Transient checkpoint is explicitly skipped"), bSkipped);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1891CombatSavePreservesMainSlotTest,
    "Grimrock.Save.MON18.9.1.CombatSaveDoesNotOverwriteMainSlot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1891CombatSavePreservesMainSlotTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON1891TestSaveSlots Slots;
    FGridMON1891TestWorld TestWorld;
    AGridLevelRuntimeActor* Runtime = nullptr;
    AGrimrockPartyPawn* Party = nullptr;
    UGridTurnManagerComponent* TurnManager = nullptr;
    if (!ConfigureMON1891PersistentParty (
            *this, TestWorld, Slots, Runtime, Party, TurnManager))
    {
        return false;
    }

    FText Error;
    TestTrue (TEXT ("Baseline main save succeeds"), Party->SaveCurrentGame (Error));
    Party->CurrentCellX = 2;
    Party->CurrentCellY = 2;
    TurnManager->CurrentPhase = EGridCombatPhase::EnemyPhase;
    TurnManager->bCombatActive = true;
    TestFalse (TEXT ("Combat save cannot overwrite baseline"), Party->SaveCurrentGame (Error));

    const UGrimrockPartySaveGame* Preserved =
        LoadMON1891Save (Slots.MainSlot, Slots.UserIndex);
    TestNotNull (TEXT ("Baseline save remains readable"), Preserved);
    TestTrue (
        TEXT ("Baseline save retains the pre-combat cell"),
        Preserved && Preserved->PartyCellX == 1 && Preserved->PartyCellY == 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1891DefeatPreservesCheckpointTest,
    "Grimrock.Save.MON18.9.1.DefeatPreservesPreCombatCheckpoint",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1891DefeatPreservesCheckpointTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON1891TestSaveSlots Slots;
    FGridMON1891TestWorld TestWorld;
    AGridLevelRuntimeActor* Runtime = nullptr;
    AGrimrockPartyPawn* Party = nullptr;
    UGridTurnManagerComponent* TurnManager = nullptr;
    if (!ConfigureMON1891PersistentParty (
            *this, TestWorld, Slots, Runtime, Party, TurnManager))
    {
        return false;
    }

    FText Error;
    bool bSkipped = false;
    TestTrue (
        TEXT ("Pre-combat checkpoint is written"),
        FGridCombatSavePolicy::PreparePreCombatCheckpoint (
            Party, Error, bSkipped));
    TestFalse (TEXT ("Checkpoint is persistent"), bSkipped);

    Party->CurrentCellX = 3;
    Party->CurrentCellY = 3;
    TurnManager->bCombatActive = false;
    TurnManager->CurrentPhase = EGridCombatPhase::Defeat;
    TestFalse (TEXT ("Defeat cannot create a regular save"), Party->SaveCurrentGame (Error));

    const UGrimrockPartySaveGame* Checkpoint =
        LoadMON1891Save (Slots.AutoCombatSlot, Slots.UserIndex);
    TestNotNull (TEXT ("Pre-combat checkpoint survives defeat"), Checkpoint);
    TestTrue (
        TEXT ("Defeat did not mutate the checkpoint"),
        Checkpoint && Checkpoint->PartyCellX == 1 && Checkpoint->PartyCellY == 1);
    return true;
}

#endif

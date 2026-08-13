#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GrimrockGameInstance.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Save/GrimrockPartySaveGame.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "EditorTools/GridLevelEditorActor.h"

namespace
{
    const FString MON133MapPath =
        TEXT ("/Game/GrimrockPrototype/Maps/L_GrimrockEditor");
    const FGuid MON133RatSpawnId (
        0xF7319908, 0x4F46EDCC, 0x7D64ED9C, 0x42588D57);
    const FIntPoint MON133RatCell (29, 25);
    const FIntPoint MON133StartCell (28, 23);
    const FIntPoint MON133TriggerCell (27, 24);

    struct FMON133RealPIEState
    {
        FString TemporarySaveSlot = FString::Printf (
            TEXT ("MON133_PIE_Integration_%s"),
            *FGuid::NewGuid ().ToString (EGuidFormats::Digits));
        TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
        FString PreparedRuntimeActorName;
        bool bOriginalAutoPreparePIE = true;
        FDelegateHandle PIEWorldInitializationHandle;
        FDateTime TemporarySaveTimestampBeforeFreshPIE;
    };

    UWorld* GetMON133PIEWorld ()
    {
        return GEditor ? GEditor->PlayWorld : nullptr;
    }

    int32 CountMON133RatActors (UWorld* World)
    {
        int32 Count = 0;
        if (!World)
        {
            return Count;
        }
        for (TActorIterator<AGridMonsterActor> It (World); It; ++It)
        {
            if (It->SpawnObjectId == MON133RatSpawnId)
            {
                ++Count;
            }
        }
        return Count;
    }

    class FSetupMON133RealPIE : public IAutomationLatentCommand
    {
    public:
        FSetupMON133RealPIE (
            FAutomationTestBase* InTest,
            TSharedRef<FMON133RealPIEState> InState)
            : Test (InTest), State (MoveTemp (InState))
        {
        }

        virtual bool Update () override
        {
            UWorld* EditorWorld = GEditor
                ? GEditor->GetEditorWorldContext ().World ()
                : nullptr;
            Test->TestNotNull (TEXT ("The real editor map is loaded"), EditorWorld);
            if (!EditorWorld)
            {
                return true;
            }

            TArray<AGridLevelEditorActor*> EditorActors;
            for (TActorIterator<AGridLevelEditorActor> It (EditorWorld); It; ++It)
            {
                EditorActors.Add (*It);
            }
            Test->TestEqual (TEXT ("The editor world has one grid editor actor"), EditorActors.Num (), 1);
            if (EditorActors.Num () != 1)
            {
                return true;
            }

            AGridLevelEditorActor* EditorActor = EditorActors[0];
            Test->TestNotNull (TEXT ("The editor actor has its prepared runtime actor"),
                EditorActor->PreviewRuntimeActor.Get ());
            Test->TestNotNull (TEXT ("The editor actor has a level asset"),
                EditorActor->LevelAsset.Get ());
            if (!EditorActor->PreviewRuntimeActor || !EditorActor->LevelAsset)
            {
                return true;
            }

            const FGridLevelObjectData* RatSpawn =
                EditorActor->LevelAsset->FindMonsterSpawnById (MON133RatSpawnId);
            Test->TestNotNull (TEXT ("The real Rat SpawnId exists"), RatSpawn);
            if (!RatSpawn)
            {
                return true;
            }
            Test->TestFalse (TEXT ("The real Rat has Enabled at Start disabled"),
                RatSpawn->bInitiallyEnabled);
            Test->TestEqual (TEXT ("The real Rat cell is unchanged"),
                FIntPoint (RatSpawn->CellX, RatSpawn->CellY), MON133RatCell);
            Test->TestEqual (TEXT ("The real StartCell is unchanged"),
                EditorActor->LevelAsset->GetStartCell (), MON133StartCell);

            const FGridLevelObjectData* Trigger = nullptr;
            for (const FGridLevelObjectData& Object : EditorActor->LevelAsset->Objects)
            {
                if (Object.Type == EGridLevelObjectType::Trigger &&
                    FIntPoint (Object.CellX, Object.CellY) == MON133TriggerCell)
                {
                    Trigger = &Object;
                    break;
                }
            }
            Test->TestNotNull (TEXT ("The real trigger exists on the expected cell"), Trigger);
            Test->TestTrue (TEXT ("The trigger is not on StartCell"),
                MON133TriggerCell != MON133StartCell);

            bool bHasExpectedLink = false;
            if (Trigger)
            {
                for (const FGridObjectLink& Link : EditorActor->LevelAsset->Links)
                {
                    if (Link.SourceObjectId == Trigger->ObjectId &&
                        Link.TargetObjectId == MON133RatSpawnId &&
                        Link.SourceEvent == EGridObjectEvent::Activated &&
                        Link.Command == EGridObjectCommand::Spawn)
                    {
                        bHasExpectedLink = true;
                        break;
                    }
                }
            }
            Test->TestTrue (TEXT ("Trigger.Activated links to Rat.Spawn"), bHasExpectedLink);

            State->EditorActor = EditorActor;
            State->PreparedRuntimeActorName = EditorActor->PreviewRuntimeActor->GetName ();
            State->bOriginalAutoPreparePIE = EditorActor->bAutoPreparePIE;

            UGameplayStatics::DeleteGameInSlot (State->TemporarySaveSlot, 0);
            UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame> ();
            Save->PartyInventoryState.bInitialCharacterCreationCompleted = true;
            Save->PartyInventoryState.SelectedCharacterIndex = 0;
            Save->PartyInventoryState.MaxActiveCharacters = 6;
            FGridCharacterInventoryState Character;
            Character.CharacterId = FGuid::NewGuid ();
            Character.DisplayName = FText::FromString (TEXT ("MON13.3 PIE Test"));
            Save->PartyInventoryState.ActiveCharacters.Add (Character);
            Save->PartyInventoryState.ActiveEquipment.AddDefaulted ();
            Save->CurrentDungeonLevelId = EditorActor->CurrentDungeonLevelId;
            Save->PartyCellX = MON133StartCell.X;
            Save->PartyCellY = MON133StartCell.Y;
            Save->PartyFacing = EditorActor->LevelAsset->StartFacing;
            FGridLevelRuntimeState& SavedLevel =
                Save->DungeonRuntimeState.LevelStates.FindOrAdd (
                    Save->CurrentDungeonLevelId);
            SavedLevel.LevelId = Save->CurrentDungeonLevelId;
            SavedLevel.bHasBeenVisited = true;
            FGridRuntimeMonsterPlacementState& Placement =
                SavedLevel.MonsterPlacements.FindOrAdd (MON133RatSpawnId);
            Placement.SpawnId = MON133RatSpawnId;
            Placement.bIsSpawned = true;
            Placement.bHasMonsterState = false;

            Test->TestTrue (TEXT ("The dedicated temporary save is written"),
                UGameplayStatics::SaveGameToSlot (Save, State->TemporarySaveSlot, 0));
            const FString SavePath = FPaths::ProjectSavedDir () /
                TEXT ("SaveGames") /
                (State->TemporarySaveSlot + TEXT (".sav"));
            State->TemporarySaveTimestampBeforeFreshPIE =
                IFileManager::Get ().GetTimeStamp (*SavePath);

            EditorActor->bAutoPreparePIE = true;
            State->PIEWorldInitializationHandle =
                FWorldDelegates::OnPostWorldInitialization.AddLambda (
                    [Test = Test, State = State] (
                        UWorld* World,
                        const UWorld::InitializationValues)
                    {
                        if (!World || World->WorldType != EWorldType::PIE)
                        {
                            return;
                        }
                        UGrimrockGameInstance* GameInstance =
                            World->GetGameInstance<UGrimrockGameInstance> ();
                        Test->TestNotNull (
                            TEXT ("The genuine PIE world has GrimrockGameInstance before BeginPlay"),
                            GameInstance);
                        if (GameInstance)
                        {
                            GameInstance->SetPendingLoadSlot (
                                State->TemporarySaveSlot,
                                0);
                            GameInstance->SetPendingStartupMode (
                                EGrimrockPartyStartupMode::Continue);
                            UE_LOG (LogTemp, Log,
                                TEXT ("[MON133PIE] Phase=IntegrationPIEWorldInitialized WorldType=%d WorldName=%s SaveSlot=%s"),
                                static_cast<int32> (World->WorldType),
                                *World->GetName (),
                                *State->TemporarySaveSlot);
                        }
                    });
            UE_LOG (LogTemp, Log,
                TEXT ("[MON133PIE] Phase=IntegrationSetup SaveSlot=%s SpawnId=%s bInitiallyEnabled=false StartCell=(28,23) TriggerCell=(27,24) MonsterPlacements=present bIsSpawned=true"),
                *State->TemporarySaveSlot,
                *MON133RatSpawnId.ToString ());
            return true;
        }

    private:
        FAutomationTestBase* Test;
        TSharedRef<FMON133RealPIEState> State;
    };

    class FWaitForMON133PIE : public IAutomationLatentCommand
    {
    public:
        explicit FWaitForMON133PIE (FAutomationTestBase* InTest)
            : Test (InTest)
        {
        }

        virtual bool Update () override
        {
            if (StartSeconds <= 0.0)
            {
                StartSeconds = FPlatformTime::Seconds ();
            }
            UWorld* World = GetMON133PIEWorld ();
            if (World && World->HasBegunPlay ())
            {
                return true;
            }
            if (FPlatformTime::Seconds () - StartSeconds > 30.0)
            {
                Test->AddError (TEXT ("Timed out waiting for the genuine PIE world to begin play."));
                return true;
            }
            return false;
        }

    private:
        FAutomationTestBase* Test;
        double StartSeconds = 0.0;
    };

    class FCheckMON133FreshPIE : public IAutomationLatentCommand
    {
    public:
        FCheckMON133FreshPIE (
            FAutomationTestBase* InTest,
            TSharedRef<FMON133RealPIEState> InState)
            : Test (InTest), State (MoveTemp (InState))
        {
        }

        virtual bool Update () override
        {
            UWorld* World = GetMON133PIEWorld ();
            Test->TestNotNull (TEXT ("A genuine duplicated PIE world exists"), World);
            if (!World)
            {
                return true;
            }
            Test->TestEqual (TEXT ("The duplicated world is EWorldType::PIE"),
                World->WorldType, EWorldType::PIE);

            TArray<AGridLevelRuntimeActor*> Runtimes;
            for (TActorIterator<AGridLevelRuntimeActor> It (World); It; ++It)
            {
                Runtimes.Add (*It);
                UE_LOG (LogTemp, Log,
                    TEXT ("[MON133PIE] Phase=IntegrationFreshCheck RuntimeActorName=%s RuntimeActorPath=%s"),
                    *It->GetName (),
                    *It->GetPathName ());
            }
            Test->TestEqual (TEXT ("The PIE world has exactly one grid runtime actor"),
                Runtimes.Num (), 1);

            AGrimrockPartyPawn* Party = nullptr;
            int32 PartyCount = 0;
            for (TActorIterator<AGrimrockPartyPawn> It (World); It; ++It)
            {
                ++PartyCount;
                Party = *It;
            }
            Test->TestEqual (TEXT ("The PIE world has exactly one party pawn"), PartyCount, 1);
            if (Runtimes.Num () != 1 || !Party)
            {
                return true;
            }

            AGridLevelRuntimeActor* Runtime = Runtimes[0];
            Test->TestEqual (TEXT ("The duplicated runtime preserves the prepared actor identity"),
                Runtime->GetName (), State->PreparedRuntimeActorName);
            Test->TestEqual (TEXT ("The party uses the actor prepared before PIE"),
                Party->LevelRuntimeActor.Get (), Runtime);
            Test->TestEqual (TEXT ("StartupModeComponent kept Continue for the temporary save"),
                Party->PartyStartupMode, EGrimrockPartyStartupMode::Continue);
            Test->TestEqual (TEXT ("StartupModeComponent used the dedicated temporary slot"),
                Party->PartySaveSlotName, State->TemporarySaveSlot);

            Test->TestEqual (TEXT ("Fresh real PIE has no Rat after all BeginPlay calls"),
                CountMON133RatActors (World), 0);
            Test->TestNull (TEXT ("The Rat SpawnId is absent after real PIE startup"),
                Runtime->FindSpawnedMonsterActor (MON133RatSpawnId));

            Runtime->HandlePartyCellChanged (
                MON133StartCell.X,
                MON133StartCell.Y,
                MON133StartCell.X,
                MON133StartCell.Y);
            Test->TestEqual (TEXT ("StartCell notification away from trigger keeps Rat absent"),
                CountMON133RatActors (World), 0);

            Runtime->HandlePartyCellChanged (
                MON133StartCell.X,
                MON133StartCell.Y,
                MON133TriggerCell.X,
                MON133TriggerCell.Y);
            Test->TestEqual (TEXT ("Entering TriggerCell creates exactly one Rat"),
                CountMON133RatActors (World), 1);
            Test->TestNotNull (TEXT ("Rat SpawnId exists after Trigger.Activated"),
                Runtime->FindSpawnedMonsterActor (MON133RatSpawnId));
            const FGridLevelRuntimeState* SpawnedLevelState =
                Runtime->DungeonRuntimeState.LevelStates.Find (
                    Runtime->CurrentDungeonLevelId);
            const FGridRuntimeMonsterPlacementState* SpawnedPlacement =
                SpawnedLevelState
                    ? SpawnedLevelState->MonsterPlacements.Find (
                        MON133RatSpawnId)
                    : nullptr;
            Test->TestNotNull (TEXT ("Real spawn creates persistent MonsterPlacements state"),
                SpawnedPlacement);
            Test->TestTrue (TEXT ("Real spawn persists bIsSpawned=true"),
                SpawnedPlacement && SpawnedPlacement->bIsSpawned);

            Runtime->HandlePartyCellChanged (
                MON133StartCell.X,
                MON133StartCell.Y,
                MON133TriggerCell.X,
                MON133TriggerCell.Y);
            Test->TestEqual (TEXT ("Second trigger notification creates no duplicate"),
                CountMON133RatActors (World), 1);
            return true;
        }

    private:
        FAutomationTestBase* Test;
        TSharedRef<FMON133RealPIEState> State;
    };

    class FPrepareMON133ContinuePIE : public IAutomationLatentCommand
    {
    public:
        FPrepareMON133ContinuePIE (
            FAutomationTestBase* InTest,
            TSharedRef<FMON133RealPIEState> InState)
            : Test (InTest), State (MoveTemp (InState))
        {
        }

        virtual bool Update () override
        {
            AGridLevelEditorActor* EditorActor = nullptr;
            UWorld* EditorWorld = GEditor
                ? GEditor->GetEditorWorldContext ().World ()
                : nullptr;
            if (EditorWorld)
            {
                for (TActorIterator<AGridLevelEditorActor> It (EditorWorld); It; ++It)
                {
                    EditorActor = *It;
                    break;
                }
            }
            Test->TestNotNull (TEXT ("Editor actor survives the first PIE session"), EditorActor);
            if (EditorActor)
            {
                EditorActor->bAutoPreparePIE = false;
            }
            Test->TestTrue (TEXT ("Temporary save survives until Continue validation"),
                UGameplayStatics::DoesSaveGameExist (State->TemporarySaveSlot, 0));
            const FString SavePath = FPaths::ProjectSavedDir () /
                TEXT ("SaveGames") /
                (State->TemporarySaveSlot + TEXT (".sav"));
            Test->TestEqual (TEXT ("Fresh PIE does not modify the temporary save"),
                IFileManager::Get ().GetTimeStamp (*SavePath),
                State->TemporarySaveTimestampBeforeFreshPIE);
            return true;
        }

    private:
        FAutomationTestBase* Test;
        TSharedRef<FMON133RealPIEState> State;
    };

    class FCheckMON133ContinuePIE : public IAutomationLatentCommand
    {
    public:
        FCheckMON133ContinuePIE (
            FAutomationTestBase* InTest,
            TSharedRef<FMON133RealPIEState> InState)
            : Test (InTest), State (MoveTemp (InState))
        {
        }

        virtual bool Update () override
        {
            UWorld* World = GetMON133PIEWorld ();
            Test->TestNotNull (TEXT ("A second genuine PIE world exists for Continue"), World);
            if (!World)
            {
                return true;
            }
            AGridLevelRuntimeActor* Runtime = nullptr;
            for (TActorIterator<AGridLevelRuntimeActor> It (World); It; ++It)
            {
                Runtime = *It;
                break;
            }
            Test->TestNotNull (TEXT ("Continue PIE has its runtime actor"), Runtime);
            Test->TestEqual (TEXT ("Continue restores Rat from MonsterPlacements"),
                CountMON133RatActors (World), 1);
            if (Runtime)
            {
                Test->TestNotNull (TEXT ("Continue restores the Rat SpawnId"),
                    Runtime->FindSpawnedMonsterActor (MON133RatSpawnId));
            }
            UE_LOG (LogTemp, Log,
                TEXT ("[MON133PIE] Phase=IntegrationContinueCheck SaveSlot=%s RatCount=%d"),
                *State->TemporarySaveSlot,
                CountMON133RatActors (World));
            return true;
        }

    private:
        FAutomationTestBase* Test;
        TSharedRef<FMON133RealPIEState> State;
    };

    class FCleanupMON133RealPIE : public IAutomationLatentCommand
    {
    public:
        FCleanupMON133RealPIE (
            FAutomationTestBase* InTest,
            TSharedRef<FMON133RealPIEState> InState)
            : Test (InTest), State (MoveTemp (InState))
        {
        }

        virtual bool Update () override
        {
            AGridLevelEditorActor* EditorActor = nullptr;
            UWorld* EditorWorld = GEditor
                ? GEditor->GetEditorWorldContext ().World ()
                : nullptr;
            if (EditorWorld)
            {
                for (TActorIterator<AGridLevelEditorActor> It (EditorWorld); It; ++It)
                {
                    EditorActor = *It;
                    break;
                }
            }
            if (EditorActor)
            {
                EditorActor->bAutoPreparePIE = State->bOriginalAutoPreparePIE;
            }
            if (State->PIEWorldInitializationHandle.IsValid ())
            {
                FWorldDelegates::OnPostWorldInitialization.Remove (
                    State->PIEWorldInitializationHandle);
                State->PIEWorldInitializationHandle.Reset ();
            }
            if (UGameplayStatics::DoesSaveGameExist (State->TemporarySaveSlot, 0))
            {
                Test->TestTrue (TEXT ("The dedicated temporary save is deleted"),
                    UGameplayStatics::DeleteGameInSlot (State->TemporarySaveSlot, 0));
            }
            return true;
        }

    private:
        FAutomationTestBase* Test;
        TSharedRef<FMON133RealPIEState> State;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridEditorMON133RealPIEIntegrationTest,
    "Grimrock.Monsters.MON13.3.RealPIEIntegration",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridEditorMON133RealPIEIntegrationTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    TSharedRef<FMON133RealPIEState> State =
        MakeShared<FMON133RealPIEState> ();

    ADD_LATENT_AUTOMATION_COMMAND (FEditorLoadMap (MON133MapPath));
    ADD_LATENT_AUTOMATION_COMMAND (FSetupMON133RealPIE (this, State));
    ADD_LATENT_AUTOMATION_COMMAND (FStartPIECommand (false));
    ADD_LATENT_AUTOMATION_COMMAND (FWaitForMON133PIE (this));
    ADD_LATENT_AUTOMATION_COMMAND (FWaitLatentCommand (2.0f));
    ADD_LATENT_AUTOMATION_COMMAND (FCheckMON133FreshPIE (this, State));
    ADD_LATENT_AUTOMATION_COMMAND (FEndPlayMapCommand ());
    ADD_LATENT_AUTOMATION_COMMAND (FWaitLatentCommand (1.0f));
    ADD_LATENT_AUTOMATION_COMMAND (FPrepareMON133ContinuePIE (this, State));
    ADD_LATENT_AUTOMATION_COMMAND (FStartPIECommand (false));
    ADD_LATENT_AUTOMATION_COMMAND (FWaitForMON133PIE (this));
    ADD_LATENT_AUTOMATION_COMMAND (FWaitLatentCommand (2.0f));
    ADD_LATENT_AUTOMATION_COMMAND (FCheckMON133ContinuePIE (this, State));
    ADD_LATENT_AUTOMATION_COMMAND (FEndPlayMapCommand ());
    ADD_LATENT_AUTOMATION_COMMAND (FWaitLatentCommand (1.0f));
    ADD_LATENT_AUTOMATION_COMMAND (FCleanupMON133RealPIE (this, State));
    return true;
}

#endif

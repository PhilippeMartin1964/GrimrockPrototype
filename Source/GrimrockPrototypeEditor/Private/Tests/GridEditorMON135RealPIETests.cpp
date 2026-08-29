#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GrimrockGameInstance.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterCombatComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Save/GrimrockPartySaveGame.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"
#include "EditorTools/GridLevelEditorActor.h"

namespace
{
	const FString MON135MapPath = TEXT("/Game/GrimrockPrototype/Maps/L_GrimrockEditor");
	const FGuid MON135RatSpawnId(0xF7319908, 0x4F46EDCC, 0x7D64ED9C, 0x42588D57);
	const FGuid MON135Wave0SecondSpawnId(0xE4DC825C, 0x490F3B73, 0xEA579EB7, 0xAC3D2AEA);
	const FGuid MON135Wave1SpawnId(0xAAF0E031, 0x45A2838D, 0x0B4CCB98, 0xFC04126C);
	const FGuid MON135TriggerId(0x21D53B14, 0x5ED94B37, 0xB54A321E, 0x019B2F67);
	const FName MON135EncounterId(TEXT("Encounter_Rats_01"));
	const FIntPoint MON135RatCell(29, 25);
	const FIntPoint MON135Wave0SecondCell(28, 26);
	const FIntPoint MON135Wave1Cell(27, 25);
	const FIntPoint MON135StartCell(28, 23);
	const FIntPoint MON135TriggerCell(27, 24);

	struct FMON135RealPIEState
	{
		FString TemporarySaveSlot = FString::Printf(TEXT("MON135_PIE_Integration_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
		TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
		TStrongObjectPtr<UGridLevelAsset> OriginalLevelAsset;
		TStrongObjectPtr<UGridDungeonAsset> OriginalDungeonAsset;
		TStrongObjectPtr<UGridLevelAsset> OriginalPreviewLevelAsset;
		TStrongObjectPtr<UGridDungeonAsset> OriginalPreviewDungeonAsset;
		TStrongObjectPtr<UGridLevelAsset> FixtureLevelAsset;
		TStrongObjectPtr<UGridDungeonAsset> FixtureDungeonAsset;
		FString PreparedRuntimeActorName;
		FIntPoint StartCell = FIntPoint(0, 0);
		bool bOriginalAutoPreparePIE = true;
		bool bSetupSucceeded = false;
		bool bContinuePIEReady = false;
		FDelegateHandle PIEWorldInitializationHandle;
		TArray<uint8> TemporarySaveBytesBeforeFreshPIE;
	};

	bool InstallMON135TransientPIEFixture(FAutomationTestBase& Test, AGridLevelEditorActor* EditorActor, FMON135RealPIEState& State)
	{
		if (!EditorActor || !EditorActor->LevelAsset || !EditorActor->PreviewRuntimeActor)
		{
			return false;
		}

		UGridMonsterDefinitionAsset* RatDefinition = LoadObject<UGridMonsterDefinitionAsset>(
			nullptr, TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.DA_MON_RatGiant"));
		Test.TestNotNull(TEXT("The MON13.5 transient fixture loads the production Rat definition"), RatDefinition);
		if (!RatDefinition)
		{
			return false;
		}

		State.OriginalLevelAsset.Reset(EditorActor->LevelAsset.Get());
		State.OriginalDungeonAsset.Reset(EditorActor->DungeonAsset.Get());
		State.OriginalPreviewLevelAsset.Reset(EditorActor->PreviewRuntimeActor->LevelAsset.Get());
		State.OriginalPreviewDungeonAsset.Reset(EditorActor->PreviewRuntimeActor->DungeonAsset.Get());

		UGridLevelAsset* FixtureLevel = NewObject<UGridLevelAsset>(GetTransientPackage(), NAME_None, RF_Transient);
		Test.TestNotNull(TEXT("The MON13.5 transient LevelAsset is created"), FixtureLevel);
		if (!FixtureLevel)
		{
			return false;
		}

		FixtureLevel->Width = 32;
		FixtureLevel->Height = 32;
		FixtureLevel->CellSize = EditorActor->LevelAsset->CellSize;
		FixtureLevel->EnsureCellCount();
		for (FGridLevelCellData& Cell : FixtureLevel->Cells)
		{
			Cell = FGridLevelCellData();
			Cell.CellType = EGridCellType::Floor;
			Cell.bHasCeiling = true;
			Cell.bBlocksOccupancy = false;
		}
		FixtureLevel->StartCellX = MON135StartCell.X;
		FixtureLevel->StartCellY = MON135StartCell.Y;
		FixtureLevel->StartFacing = EGridEdge::North;

		FGridLevelObjectData Trigger;
		Trigger.ObjectId = MON135TriggerId;
		Trigger.Type = EGridLevelObjectType::Trigger;
		Trigger.CellX = MON135TriggerCell.X;
		Trigger.CellY = MON135TriggerCell.Y;
		Trigger.Edge = EGridEdge::None;
		Trigger.bInitiallyEnabled = true;
		FixtureLevel->Objects.Add(Trigger);

		auto AddEncounterRat = [FixtureLevel, RatDefinition](const FGuid& SpawnId, const FIntPoint& Cell, int32 WaveIndex)
		{
			FGridLevelObjectData Spawn;
			Spawn.ObjectId = SpawnId;
			Spawn.Type = EGridLevelObjectType::MonsterSpawn;
			Spawn.CellX = Cell.X;
			Spawn.CellY = Cell.Y;
			Spawn.Edge = EGridEdge::None;
			Spawn.InitialFacing = EGridEdge::North;
			Spawn.MonsterDefinitionAsset = RatDefinition;
			Spawn.MonsterDefinitionId = RatDefinition->MonsterId;
			Spawn.EncounterGroupId = MON135EncounterId;
			Spawn.EncounterWaveIndex = WaveIndex;
			Spawn.bInitiallyEnabled = false;
			FixtureLevel->Objects.Add(Spawn);
		};

		AddEncounterRat(MON135RatSpawnId, MON135RatCell, 0);
		AddEncounterRat(MON135Wave0SecondSpawnId, MON135Wave0SecondCell, 0);
		AddEncounterRat(MON135Wave1SpawnId, MON135Wave1Cell, 1);

		FGridObjectLink StartEncounterLink;
		StartEncounterLink.SourceObjectId = MON135TriggerId;
		StartEncounterLink.TargetObjectId = MON135RatSpawnId;
		StartEncounterLink.SourceEvent = EGridObjectEvent::Activated;
		StartEncounterLink.Command = EGridObjectCommand::StartEncounter;
		FixtureLevel->Links.Add(StartEncounterLink);

		TArray<FString> MonsterValidationErrors;
		const bool bMonsterFixtureValid = FixtureLevel->ValidateMonsterSpawns(MonsterValidationErrors);
		Test.TestTrue(TEXT("The MON13.5 transient monster fixture validates"), bMonsterFixtureValid);
		for (const FString& ValidationError : MonsterValidationErrors)
		{
			Test.AddError(FString::Printf(TEXT("MON13.5 transient fixture validation: %s"), *ValidationError));
		}
		if (!bMonsterFixtureValid)
		{
			return false;
		}

		UGridDungeonAsset* FixtureDungeon = nullptr;
		if (UGridDungeonAsset* OriginalDungeon = EditorActor->DungeonAsset.Get())
		{
			const FName FixtureDungeonName(*FString::Printf(
				TEXT("MON135_PIE_Dungeon_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
			FixtureDungeon = DuplicateObject<UGridDungeonAsset>(OriginalDungeon, GetTransientPackage(), FixtureDungeonName);
			Test.TestNotNull(TEXT("The MON13.5 transient DungeonAsset is created"), FixtureDungeon);
			if (!FixtureDungeon)
			{
				return false;
			}
			FixtureDungeon->ClearFlags(RF_Public | RF_Standalone);
			FixtureDungeon->SetFlags(RF_Transient);

			bool bCurrentLevelRebound = false;
			for (FGridDungeonLevelEntry& Entry : FixtureDungeon->Levels)
			{
				if (Entry.LevelId == EditorActor->CurrentDungeonLevelId)
				{
					Entry.LevelAsset = FixtureLevel;
					bCurrentLevelRebound = Entry.bEnabled;
					break;
				}
			}
			Test.TestTrue(TEXT("The transient DungeonAsset rebinds the current enabled level"), bCurrentLevelRebound);
			if (!bCurrentLevelRebound)
			{
				return false;
			}
		}

		State.FixtureLevelAsset.Reset(FixtureLevel);
		State.FixtureDungeonAsset.Reset(FixtureDungeon);
		EditorActor->LevelAsset = FixtureLevel;
		EditorActor->DungeonAsset = FixtureDungeon;
		EditorActor->PreviewRuntimeActor->LevelAsset = FixtureLevel;
		EditorActor->PreviewRuntimeActor->DungeonAsset = FixtureDungeon;
		EditorActor->PreviewRuntimeActor->CurrentDungeonLevelId = EditorActor->CurrentDungeonLevelId;

		UE_LOG(LogTemp, Log,
			TEXT("[MON135PIE] Phase=TransientFixtureInstalled Level=%s Dungeon=%s ProductionRat=%s StartCell=(%d,%d) TriggerCell=(%d,%d)"),
			*GetNameSafe(FixtureLevel), *GetNameSafe(FixtureDungeon), *GetNameSafe(RatDefinition), MON135StartCell.X, MON135StartCell.Y, MON135TriggerCell.X,
			MON135TriggerCell.Y);
		return true;
	}

	UWorld* GetMON135PIEWorld()
	{
		return GEditor ? GEditor->PlayWorld : nullptr;
	}

	bool IsMON135EncounterSpawnId(const FGuid& SpawnId)
	{
		return SpawnId == MON135RatSpawnId || SpawnId == MON135Wave0SecondSpawnId || SpawnId == MON135Wave1SpawnId;
	}

	int32 CountMON135EncounterActors(UWorld* World)
	{
		int32 Count = 0;
		if (!World)
		{
			return Count;
		}
		for (TActorIterator<AGridMonsterActor> It(World); It; ++It)
		{
			if (IsMON135EncounterSpawnId(It->SpawnObjectId))
			{
				++Count;
			}
		}
		return Count;
	}

	int32 IsolateMON135EncounterFromUnrelatedMonsters(UWorld* World)
	{
		int32 IsolatedCount = 0;
		if (!World)
		{
			return IsolatedCount;
		}

		for (TActorIterator<AGridMonsterActor> It(World); It; ++It)
		{
			AGridMonsterActor* Monster = *It;
			if (!IsValid(Monster) || IsMON135EncounterSpawnId(Monster->SpawnObjectId))
			{
				continue;
			}

			// MON13.5 validates the real production encounter/SaveGame path. The
			// production map can legitimately gain unrelated monsters over time;
			// they must not patrol into the historical encounter fixture cells and
			// turn this test into an accidental MON14/MON17 interaction test.
			Monster->bMonsterEnabled = false;
			if (UGridMonsterMovementComponent* Movement = Monster->FindComponentByClass<UGridMonsterMovementComponent>())
			{
				Movement->ReleaseOccupancy();
			}
			Monster->Destroy();
			++IsolatedCount;
		}
		return IsolatedCount;
	}

	class FSetupMON135RealPIE : public IAutomationLatentCommand
	{
	public:
		FSetupMON135RealPIE(FAutomationTestBase* InTest, TSharedRef<FMON135RealPIEState> InState)
			: Test(InTest)
			, State(MoveTemp(InState))
		{
		}

		virtual bool Update() override
		{
			UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			Test->TestNotNull(TEXT("The real editor map is loaded"), EditorWorld);
			if (!EditorWorld)
			{
				return true;
			}

			TArray<AGridLevelEditorActor*> EditorActors;
			for (TActorIterator<AGridLevelEditorActor> It(EditorWorld); It; ++It)
			{
				EditorActors.Add(*It);
			}
			Test->TestEqual(TEXT("The editor world has one grid editor actor"), EditorActors.Num(), 1);
			if (EditorActors.Num() != 1)
			{
				return true;
			}

			AGridLevelEditorActor* EditorActor = EditorActors[0];
			Test->TestNotNull(TEXT("The editor actor has its prepared runtime actor"), EditorActor->PreviewRuntimeActor.Get());
			Test->TestNotNull(TEXT("The editor actor has a level asset"), EditorActor->LevelAsset.Get());
			if (!EditorActor->PreviewRuntimeActor || !EditorActor->LevelAsset)
			{
				return true;
			}

			State->EditorActor = EditorActor;
			if (!InstallMON135TransientPIEFixture(*Test, EditorActor, *State))
			{
				return true;
			}

			const FGridLevelObjectData* RatSpawn = EditorActor->LevelAsset->FindMonsterSpawnById(MON135RatSpawnId);
			const FGridLevelObjectData* Wave0SecondSpawn = EditorActor->LevelAsset->FindMonsterSpawnById(MON135Wave0SecondSpawnId);
			const FGridLevelObjectData* Wave1Spawn = EditorActor->LevelAsset->FindMonsterSpawnById(MON135Wave1SpawnId);
			Test->TestNotNull(TEXT("The transient PIE encounter anchor exists"), RatSpawn);
			Test->TestNotNull(TEXT("The transient second wave-zero Rat exists"), Wave0SecondSpawn);
			Test->TestNotNull(TEXT("The transient wave-one Rat exists"), Wave1Spawn);
			if (!RatSpawn || !Wave0SecondSpawn || !Wave1Spawn)
			{
				return true;
			}
			Test->TestFalse(TEXT("The encounter anchor starts disabled"), RatSpawn->bInitiallyEnabled);
			Test->TestFalse(TEXT("The second wave-zero Rat starts disabled"), Wave0SecondSpawn->bInitiallyEnabled);
			Test->TestFalse(TEXT("The wave-one Rat starts disabled"), Wave1Spawn->bInitiallyEnabled);
			Test->TestEqual(TEXT("The encounter anchor cell is unchanged"), FIntPoint(RatSpawn->CellX, RatSpawn->CellY), MON135RatCell);
			Test->TestEqual(TEXT("The second wave-zero cell is unchanged"), FIntPoint(Wave0SecondSpawn->CellX, Wave0SecondSpawn->CellY), MON135Wave0SecondCell);
			Test->TestEqual(TEXT("The wave-one cell is unchanged"), FIntPoint(Wave1Spawn->CellX, Wave1Spawn->CellY), MON135Wave1Cell);
			Test->TestEqual(TEXT("The encounter anchor uses the transient encounter"), RatSpawn->EncounterGroupId, MON135EncounterId);
			Test->TestEqual(TEXT("The second Rat uses the transient encounter"), Wave0SecondSpawn->EncounterGroupId, MON135EncounterId);
			Test->TestEqual(TEXT("The future Rat uses the transient encounter"), Wave1Spawn->EncounterGroupId, MON135EncounterId);
			Test->TestEqual(TEXT("The encounter anchor belongs to wave zero"), RatSpawn->EncounterWaveIndex, 0);
			Test->TestEqual(TEXT("The second Rat belongs to wave zero"), Wave0SecondSpawn->EncounterWaveIndex, 0);
			Test->TestEqual(TEXT("The future Rat belongs to wave one"), Wave1Spawn->EncounterWaveIndex, 1);

			State->StartCell = EditorActor->LevelAsset->GetStartCell();
			const bool bStartCellValid = EditorActor->LevelAsset->IsStartCellValid();
			Test->TestTrue(TEXT("The transient fixture StartCell is valid"), bStartCellValid);
			Test->TestTrue(TEXT("The transient fixture StartCell remains outside the encounter trigger"), State->StartCell != MON135TriggerCell);
			if (RatSpawn->bInitiallyEnabled || Wave0SecondSpawn->bInitiallyEnabled || Wave1Spawn->bInitiallyEnabled ||
				FIntPoint(RatSpawn->CellX, RatSpawn->CellY) != MON135RatCell || !bStartCellValid || State->StartCell == MON135TriggerCell)
			{
				return true;
			}

			const FGridLevelObjectData* Trigger = nullptr;
			for (const FGridLevelObjectData& Object : EditorActor->LevelAsset->Objects)
			{
				if (Object.Type == EGridLevelObjectType::Trigger && FIntPoint(Object.CellX, Object.CellY) == MON135TriggerCell)
				{
					Trigger = &Object;
					break;
				}
			}
			Test->TestNotNull(TEXT("The transient fixture trigger exists on the expected cell"), Trigger);
			if (!Trigger)
			{
				return true;
			}

			bool bHasExpectedLink = false;
			for (const FGridObjectLink& Link : EditorActor->LevelAsset->Links)
			{
				if (Link.SourceObjectId == Trigger->ObjectId && Link.TargetObjectId == MON135RatSpawnId && Link.SourceEvent == EGridObjectEvent::Activated &&
					Link.Command == EGridObjectCommand::StartEncounter)
				{
					bHasExpectedLink = true;
					break;
				}
			}
			Test->TestTrue(TEXT("Trigger.Activated links to Rat.StartEncounter"), bHasExpectedLink);
			if (!bHasExpectedLink)
			{
				return true;
			}

			State->EditorActor = EditorActor;
			State->PreparedRuntimeActorName = EditorActor->PreviewRuntimeActor->GetName();
			State->bOriginalAutoPreparePIE = EditorActor->bAutoPreparePIE;

			UGameplayStatics::DeleteGameInSlot(State->TemporarySaveSlot, 0);
			UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
			Save->PartyInventoryState.bInitialCharacterCreationCompleted = true;
			Save->PartyInventoryState.SelectedCharacterIndex = 0;
			Save->PartyInventoryState.MaxActiveCharacters = 6;
			FGridCharacterInventoryState Character;
			Character.CharacterId = FGuid::NewGuid();
			Character.DisplayName = FText::FromString(TEXT("MON13.5 PIE Test"));
			Save->PartyInventoryState.ActiveCharacters.Add(Character);
			Save->PartyInventoryState.ActiveEquipment.AddDefaulted();
			Save->CurrentDungeonLevelId = EditorActor->CurrentDungeonLevelId;
			Save->PartyCellX = State->StartCell.X;
			Save->PartyCellY = State->StartCell.Y;
			Save->PartyFacing = EditorActor->LevelAsset->StartFacing;
			FGridLevelRuntimeState& SavedLevel = Save->DungeonRuntimeState.LevelStates.FindOrAdd(Save->CurrentDungeonLevelId);
			SavedLevel.LevelId = Save->CurrentDungeonLevelId;
			SavedLevel.bHasBeenVisited = true;
			FGridRuntimeMonsterPlacementState& Placement = SavedLevel.MonsterPlacements.FindOrAdd(MON135RatSpawnId);
			Placement.SpawnId = MON135RatSpawnId;
			Placement.bIsSpawned = true;
			Placement.bHasMonsterState = false;
			FGridRuntimeMonsterPlacementState& SecondPlacement = SavedLevel.MonsterPlacements.FindOrAdd(MON135Wave0SecondSpawnId);
			SecondPlacement.SpawnId = MON135Wave0SecondSpawnId;
			SecondPlacement.bIsSpawned = true;
			SecondPlacement.bHasMonsterState = false;
			FGridRuntimeMonsterEncounterState& EncounterState = SavedLevel.MonsterEncounters.FindOrAdd(MON135EncounterId);
			EncounterState.EncounterGroupId = MON135EncounterId;
			EncounterState.AnchorSpawnId = MON135RatSpawnId;
			EncounterState.ActiveWaveIndex = 0;
			EncounterState.bStarted = true;
			EncounterState.bCompleted = false;

			const bool bSaveWritten = UGameplayStatics::SaveGameToSlot(Save, State->TemporarySaveSlot, 0);
			Test->TestTrue(TEXT("The dedicated temporary save is written"), bSaveWritten);
			if (!bSaveWritten)
			{
				return true;
			}
			const FString SavePath = FPaths::ProjectSavedDir() / TEXT("SaveGames") / (State->TemporarySaveSlot + TEXT(".sav"));
			const bool bSaveBytesLoaded = FFileHelper::LoadFileToArray(State->TemporarySaveBytesBeforeFreshPIE, *SavePath);
			Test->TestTrue(TEXT("The dedicated temporary save can be read back"), bSaveBytesLoaded);
			if (!bSaveBytesLoaded)
			{
				return true;
			}

			EditorActor->bAutoPreparePIE = true;
			State->PIEWorldInitializationHandle = FWorldDelegates::OnPostWorldInitialization.AddLambda(
				[Test = Test, State = State](UWorld* World, const UWorld::InitializationValues)
				{
					if (!World || World->WorldType != EWorldType::PIE)
					{
						return;
					}
					UGrimrockGameInstance* GameInstance = World->GetGameInstance<UGrimrockGameInstance>();
					Test->TestNotNull(TEXT("The genuine PIE world has GrimrockGameInstance before BeginPlay"), GameInstance);
					if (GameInstance)
					{
						GameInstance->SetPendingLoadSlot(State->TemporarySaveSlot, 0);
						GameInstance->SetPendingStartupMode(EGrimrockPartyStartupMode::Continue);
						UE_LOG(LogTemp, Log, TEXT("[MON135PIE] Phase=IntegrationPIEWorldInitialized WorldType=%d WorldName=%s SaveSlot=%s"),
							static_cast<int32>(World->WorldType), *World->GetName(), *State->TemporarySaveSlot);
					}
				});
			State->bSetupSucceeded = State->PIEWorldInitializationHandle.IsValid();
			Test->TestTrue(TEXT("The PIE startup injection is registered"), State->bSetupSucceeded);
			UE_LOG(LogTemp, Log,
				TEXT(
					"[MON135PIE] Phase=IntegrationSetup SaveSlot=%s AnchorSpawnId=%s Encounter=Encounter_Rats_01 Wave0=2 Wave1=1 bInitiallyEnabled=false StartCell=(%d,%d) TriggerCell=(27,24)"),
				*State->TemporarySaveSlot, *MON135RatSpawnId.ToString(), State->StartCell.X, State->StartCell.Y);
			return true;
		}

	private:
		FAutomationTestBase* Test;
		TSharedRef<FMON135RealPIEState> State;
	};

	class FWaitForMON135PIE : public IAutomationLatentCommand
	{
	public:
		FWaitForMON135PIE(FAutomationTestBase* InTest, TSharedRef<FMON135RealPIEState> InState)
			: Test(InTest)
			, State(MoveTemp(InState))
		{
		}

		virtual bool Update() override
		{
			if (!State->bSetupSucceeded)
			{
				return true;
			}
			if (StartSeconds <= 0.0)
			{
				StartSeconds = FPlatformTime::Seconds();
			}
			UWorld* World = GetMON135PIEWorld();
			if (World && World->HasBegunPlay())
			{
				return true;
			}
			if (FPlatformTime::Seconds() - StartSeconds > 30.0)
			{
				Test->AddError(TEXT("Timed out waiting for the genuine PIE world to begin play."));
				return true;
			}
			return false;
		}

	private:
		FAutomationTestBase* Test;
		TSharedRef<FMON135RealPIEState> State;
		double StartSeconds = 0.0;
	};

	class FIsolateMON135PIE : public IAutomationLatentCommand
	{
	public:
		FIsolateMON135PIE(FAutomationTestBase* InTest, TSharedRef<FMON135RealPIEState> InState)
			: Test(InTest)
			, State(MoveTemp(InState))
		{
		}

		virtual bool Update() override
		{
			if (!State->bSetupSucceeded)
			{
				return true;
			}

			UWorld* World = GetMON135PIEWorld();
			Test->TestNotNull(TEXT("The PIE world exists before MON13.5 fixture isolation"), World);
			if (!World)
			{
				return true;
			}

			const int32 IsolatedCount = IsolateMON135EncounterFromUnrelatedMonsters(World);
			UE_LOG(
				LogTemp, Log, TEXT("[MON135PIE] Phase=IntegrationIsolation UnrelatedMonsters=%d Encounter=%s"), IsolatedCount, *MON135EncounterId.ToString());
			return true;
		}

	private:
		FAutomationTestBase* Test;
		TSharedRef<FMON135RealPIEState> State;
	};

	class FStartMON135PIEIfReady : public IAutomationLatentCommand
	{
	public:
		explicit FStartMON135PIEIfReady(TSharedRef<FMON135RealPIEState> InState)
			: State(MoveTemp(InState))
			, StartPIECommand(false)
		{
		}

		virtual bool Update() override
		{
			return !State->bSetupSucceeded || StartPIECommand.Update();
		}

	private:
		TSharedRef<FMON135RealPIEState> State;
		FStartPIECommand StartPIECommand;
	};

	class FCheckMON135FreshPIE : public IAutomationLatentCommand
	{
	public:
		FCheckMON135FreshPIE(FAutomationTestBase* InTest, TSharedRef<FMON135RealPIEState> InState)
			: Test(InTest)
			, State(MoveTemp(InState))
		{
		}

		virtual bool Update() override
		{
			if (!State->bSetupSucceeded)
			{
				return true;
			}
			UWorld* World = GetMON135PIEWorld();
			Test->TestNotNull(TEXT("A genuine duplicated PIE world exists"), World);
			if (!World)
			{
				return true;
			}
			Test->TestEqual(TEXT("The duplicated world is EWorldType::PIE"), World->WorldType, EWorldType::PIE);

			TArray<AGridLevelRuntimeActor*> Runtimes;
			for (TActorIterator<AGridLevelRuntimeActor> It(World); It; ++It)
			{
				Runtimes.Add(*It);
				UE_LOG(
					LogTemp, Log, TEXT("[MON135PIE] Phase=IntegrationFreshCheck RuntimeActorName=%s RuntimeActorPath=%s"), *It->GetName(), *It->GetPathName());
			}
			Test->TestEqual(TEXT("The PIE world has exactly one grid runtime actor"), Runtimes.Num(), 1);

			AGrimrockPartyPawn* Party = nullptr;
			int32 PartyCount = 0;
			for (TActorIterator<AGrimrockPartyPawn> It(World); It; ++It)
			{
				++PartyCount;
				Party = *It;
			}
			Test->TestEqual(TEXT("The PIE world has exactly one party pawn"), PartyCount, 1);
			if (Runtimes.Num() != 1 || !Party)
			{
				return true;
			}

			AGridLevelRuntimeActor* Runtime = Runtimes[0];
			Test->TestEqual(TEXT("The duplicated runtime preserves the prepared actor identity"), Runtime->GetName(), State->PreparedRuntimeActorName);
			Test->TestEqual(TEXT("The party uses the actor prepared before PIE"), Party->LevelRuntimeActor.Get(), Runtime);
			Test->TestEqual(TEXT("StartupModeComponent kept Continue for the temporary save"), Party->PartyStartupMode, EGrimrockPartyStartupMode::Continue);
			Test->TestEqual(TEXT("StartupModeComponent used the dedicated temporary slot"), Party->PartySaveSlotName, State->TemporarySaveSlot);
			State->bContinuePIEReady = Party->PartySaveSlotName == State->TemporarySaveSlot;

			Test->TestEqual(TEXT("Fresh real PIE has no encounter Actor after all BeginPlay calls"), CountMON135EncounterActors(World), 0);
			Test->TestNull(TEXT("The encounter anchor is absent after real PIE startup"), Runtime->FindSpawnedMonsterActor(MON135RatSpawnId));
			Test->TestNull(TEXT("The second wave-zero Rat is absent after startup"), Runtime->FindSpawnedMonsterActor(MON135Wave0SecondSpawnId));
			Test->TestNull(TEXT("The wave-one Rat is absent after startup"), Runtime->FindSpawnedMonsterActor(MON135Wave1SpawnId));

			Runtime->HandlePartyCellChanged(State->StartCell.X, State->StartCell.Y, State->StartCell.X, State->StartCell.Y);
			Test->TestEqual(TEXT("StartCell notification away from trigger keeps encounter absent"), CountMON135EncounterActors(World), 0);

			Runtime->HandlePartyCellChanged(State->StartCell.X, State->StartCell.Y, MON135TriggerCell.X, MON135TriggerCell.Y);
			Test->TestEqual(TEXT("Entering TriggerCell creates both wave-zero Rats"), CountMON135EncounterActors(World), 2);
			AGridMonsterActor* AnchorMonster = Runtime->FindSpawnedMonsterActor(MON135RatSpawnId);
			AGridMonsterActor* SecondMonster = Runtime->FindSpawnedMonsterActor(MON135Wave0SecondSpawnId);
			Test->TestNotNull(TEXT("Encounter anchor exists after Trigger.Activated"), AnchorMonster);
			Test->TestNotNull(TEXT("Second wave-zero Rat exists after Trigger.Activated"), SecondMonster);
			Test->TestNull(TEXT("Wave-one Rat remains absent"), Runtime->FindSpawnedMonsterActor(MON135Wave1SpawnId));
			Test->TestEqual(TEXT("Wave zero is the active encounter wave"), Runtime->GetMonsterEncounterActiveWave(MON135EncounterId), 0);
			for (AGridMonsterActor* Monster : { AnchorMonster, SecondMonster })
			{
				if (!Monster)
				{
					continue;
				}
				Test->TestNotEqual(TEXT("Real encounter Rat is not the native base class"), Monster->GetClass(), AGridMonsterActor::StaticClass());
				Test->TestNotNull(TEXT("Real encounter Rat has MonsterMovement"), Monster->FindComponentByClass<UGridMonsterMovementComponent>());
				Test->TestNotNull(TEXT("Real encounter Rat has MonsterBehavior"), Monster->FindComponentByClass<UGridMonsterBehaviorComponent>());
				Test->TestNotNull(TEXT("Real encounter Rat has MonsterCombat"), Monster->FindComponentByClass<UGridMonsterCombatComponent>());
			}
			const FGridLevelRuntimeState* SpawnedLevelState = Runtime->DungeonRuntimeState.LevelStates.Find(Runtime->CurrentDungeonLevelId);
			const FGridRuntimeMonsterPlacementState* SpawnedPlacement =
				SpawnedLevelState ? SpawnedLevelState->MonsterPlacements.Find(MON135RatSpawnId) : nullptr;
			Test->TestNotNull(TEXT("Real spawn creates persistent MonsterPlacements state"), SpawnedPlacement);
			Test->TestTrue(TEXT("Real spawn persists bIsSpawned=true"), SpawnedPlacement && SpawnedPlacement->bIsSpawned);

			Runtime->HandlePartyCellChanged(State->StartCell.X, State->StartCell.Y, MON135TriggerCell.X, MON135TriggerCell.Y);
			Test->TestEqual(TEXT("Second trigger notification creates no duplicate wave"), CountMON135EncounterActors(World), 2);
			return true;
		}

	private:
		FAutomationTestBase* Test;
		TSharedRef<FMON135RealPIEState> State;
	};

	class FProtectMON135SaveSlot : public IAutomationLatentCommand
	{
	public:
		explicit FProtectMON135SaveSlot(TSharedRef<FMON135RealPIEState> InState)
			: State(MoveTemp(InState))
		{
		}

		virtual bool Update() override
		{
			UWorld* World = State->bSetupSucceeded ? GetMON135PIEWorld() : nullptr;
			if (World)
			{
				for (TActorIterator<AGrimrockPartyPawn> It(World); It; ++It)
				{
					It->PartySaveSlotName = State->TemporarySaveSlot;
					It->PartySaveUserIndex = 0;
				}
			}
			return true;
		}

	private:
		TSharedRef<FMON135RealPIEState> State;
	};

	class FPrepareMON135ContinuePIE : public IAutomationLatentCommand
	{
	public:
		FPrepareMON135ContinuePIE(FAutomationTestBase* InTest, TSharedRef<FMON135RealPIEState> InState)
			: Test(InTest)
			, State(MoveTemp(InState))
		{
		}

		virtual bool Update() override
		{
			if (!State->bSetupSucceeded || !State->bContinuePIEReady)
			{
				State->bSetupSucceeded = false;
				return true;
			}
			AGridLevelEditorActor* EditorActor = State->EditorActor.Get();
			Test->TestNotNull(TEXT("Editor actor survives the first PIE session"), EditorActor);
			if (!EditorActor)
			{
				State->bSetupSucceeded = false;
				return true;
			}
			const bool bTemporarySaveExists = UGameplayStatics::DoesSaveGameExist(State->TemporarySaveSlot, 0);
			Test->TestTrue(TEXT("Temporary save survives until Continue validation"), bTemporarySaveExists);
			if (!bTemporarySaveExists)
			{
				State->bSetupSucceeded = false;
				return true;
			}
			const FString SavePath = FPaths::ProjectSavedDir() / TEXT("SaveGames") / (State->TemporarySaveSlot + TEXT(".sav"));
			TArray<uint8> CurrentSaveBytes;
			const bool bSaveBytesLoaded = FFileHelper::LoadFileToArray(CurrentSaveBytes, *SavePath);
			Test->TestTrue(TEXT("The temporary save remains readable"), bSaveBytesLoaded);
			Test->TestTrue(
				TEXT("Fresh PIE does not modify the temporary save"), bSaveBytesLoaded && CurrentSaveBytes == State->TemporarySaveBytesBeforeFreshPIE);
			if (!bSaveBytesLoaded || CurrentSaveBytes != State->TemporarySaveBytesBeforeFreshPIE)
			{
				State->bSetupSucceeded = false;
				return true;
			}
			EditorActor->bAutoPreparePIE = false;
			return true;
		}

	private:
		FAutomationTestBase* Test;
		TSharedRef<FMON135RealPIEState> State;
	};

	class FCheckMON135ContinuePIE : public IAutomationLatentCommand
	{
	public:
		FCheckMON135ContinuePIE(FAutomationTestBase* InTest, TSharedRef<FMON135RealPIEState> InState)
			: Test(InTest)
			, State(MoveTemp(InState))
		{
		}

		virtual bool Update() override
		{
			if (!State->bSetupSucceeded)
			{
				return true;
			}
			UWorld* World = GetMON135PIEWorld();
			Test->TestNotNull(TEXT("A second genuine PIE world exists for Continue"), World);
			if (!World)
			{
				return true;
			}
			AGridLevelRuntimeActor* Runtime = nullptr;
			for (TActorIterator<AGridLevelRuntimeActor> It(World); It; ++It)
			{
				Runtime = *It;
				break;
			}
			Test->TestNotNull(TEXT("Continue PIE has its runtime actor"), Runtime);
			Test->TestEqual(TEXT("Continue restores both active wave-zero Rats"), CountMON135EncounterActors(World), 2);
			AGrimrockPartyPawn* Party = nullptr;
			int32 PartyCount = 0;
			for (TActorIterator<AGrimrockPartyPawn> It(World); It; ++It)
			{
				++PartyCount;
				Party = *It;
			}
			Test->TestEqual(TEXT("Continue PIE has exactly one party pawn"), PartyCount, 1);
			Test->TestTrue(TEXT("Continue PIE uses the dedicated temporary slot"), Party && Party->PartySaveSlotName == State->TemporarySaveSlot);
			if (Runtime)
			{
				Test->TestNotNull(TEXT("Continue restores the Rat SpawnId"), Runtime->FindSpawnedMonsterActor(MON135RatSpawnId));
				Test->TestNotNull(TEXT("Continue restores the second wave-zero Rat"), Runtime->FindSpawnedMonsterActor(MON135Wave0SecondSpawnId));
				Test->TestNull(TEXT("Continue keeps the future wave absent"), Runtime->FindSpawnedMonsterActor(MON135Wave1SpawnId));
				Test->TestEqual(TEXT("Continue restores active encounter wave zero"), Runtime->GetMonsterEncounterActiveWave(MON135EncounterId), 0);
			}
			UE_LOG(LogTemp, Log, TEXT("[MON135PIE] Phase=IntegrationContinueCheck SaveSlot=%s EncounterActorCount=%d ActiveWave=%d"), *State->TemporarySaveSlot,
				CountMON135EncounterActors(World), Runtime ? Runtime->GetMonsterEncounterActiveWave(MON135EncounterId) : INDEX_NONE);
			return true;
		}

	private:
		FAutomationTestBase* Test;
		TSharedRef<FMON135RealPIEState> State;
	};

	class FCleanupMON135RealPIE : public IAutomationLatentCommand
	{
	public:
		FCleanupMON135RealPIE(FAutomationTestBase* InTest, TSharedRef<FMON135RealPIEState> InState)
			: Test(InTest)
			, State(MoveTemp(InState))
		{
		}

		virtual bool Update() override
		{
			AGridLevelEditorActor* EditorActor = State->EditorActor.Get();
			if (EditorActor)
			{
				EditorActor->bAutoPreparePIE = State->bOriginalAutoPreparePIE;

				if (UGridLevelAsset* OriginalLevelAsset = State->OriginalLevelAsset.Get())
				{
					EditorActor->LevelAsset = OriginalLevelAsset;
					EditorActor->DungeonAsset = State->OriginalDungeonAsset.Get();
				}

				if (EditorActor->PreviewRuntimeActor)
				{
					EditorActor->PreviewRuntimeActor->LevelAsset = State->OriginalPreviewLevelAsset.Get();
					EditorActor->PreviewRuntimeActor->DungeonAsset = State->OriginalPreviewDungeonAsset.Get();
					EditorActor->PreviewRuntimeActor->CurrentDungeonLevelId = EditorActor->CurrentDungeonLevelId;
					EditorActor->PreviewRuntimeActor->RebuildLevel();
				}
			}

			State->FixtureDungeonAsset.Reset();
			State->FixtureLevelAsset.Reset();
			if (State->PIEWorldInitializationHandle.IsValid())
			{
				FWorldDelegates::OnPostWorldInitialization.Remove(State->PIEWorldInitializationHandle);
				State->PIEWorldInitializationHandle.Reset();
			}
			if (UGameplayStatics::DoesSaveGameExist(State->TemporarySaveSlot, 0))
			{
				Test->TestTrue(TEXT("The dedicated temporary save is deleted"), UGameplayStatics::DeleteGameInSlot(State->TemporarySaveSlot, 0));
			}
			return true;
		}

	private:
		FAutomationTestBase* Test;
		TSharedRef<FMON135RealPIEState> State;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorMON135RealPIEIntegrationTest, "Grimrock.Monsters.MON13.5.RealPIEIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorMON135RealPIEIntegrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	AddExpectedError(TEXT("PartySave EndPlay Failed Slot="), EAutomationExpectedErrorFlags::Contains, 1);
	TSharedRef<FMON135RealPIEState> State = MakeShared<FMON135RealPIEState>();

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(MON135MapPath));
	ADD_LATENT_AUTOMATION_COMMAND(FSetupMON135RealPIE(this, State));
	ADD_LATENT_AUTOMATION_COMMAND(FStartMON135PIEIfReady(State));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForMON135PIE(this, State));
	ADD_LATENT_AUTOMATION_COMMAND(FIsolateMON135PIE(this, State));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FCheckMON135FreshPIE(this, State));
	ADD_LATENT_AUTOMATION_COMMAND(FProtectMON135SaveSlot(State));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FPrepareMON135ContinuePIE(this, State));
	ADD_LATENT_AUTOMATION_COMMAND(FStartMON135PIEIfReady(State));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForMON135PIE(this, State));
	ADD_LATENT_AUTOMATION_COMMAND(FIsolateMON135PIE(this, State));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FCheckMON135ContinuePIE(this, State));
	ADD_LATENT_AUTOMATION_COMMAND(FProtectMON135SaveSlot(State));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FCleanupMON135RealPIE(this, State));
	return true;
}

#endif

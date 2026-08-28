#if WITH_DEV_AUTOMATION_TESTS

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockGameInstance.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace
{
	const FName SAVEFIX2MissingItemId(TEXT("SAVEFIX2_MissingItemDefinition"));

	struct FGridSAVEFIX2TestSaveSlot
	{
		FString SlotName;
		int32 UserIndex = 0;

		FGridSAVEFIX2TestSaveSlot()
			: SlotName(FString::Printf(TEXT("SAVEFIX2_ContinueFailure_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)))
		{
			UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
		}

		~FGridSAVEFIX2TestSaveSlot()
		{
			UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
		}
	};

	struct FGridSAVEFIX2TestWorld
	{
		UWorld* World = nullptr;
		UGrimrockGameInstance* GameInstance = nullptr;

		FGridSAVEFIX2TestWorld()
		{
			const UWorld::InitializationValues InitializationValues = UWorld::InitializationValues()
																		  .AllowAudioPlayback(false)
																		  .RequiresHitProxies(false)
																		  .CreatePhysicsScene(false)
																		  .CreateNavigation(false)
																		  .CreateAISystem(false)
																		  .ShouldSimulatePhysics(false)
																		  .SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("SAVEFIX2TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&InitializationValues);
			if (!World || !GEngine)
			{
				return;
			}

			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);

			GameInstance = NewObject<UGrimrockGameInstance>(GEngine);
			if (GameInstance)
			{
				World->SetGameInstance(GameInstance);
			}
		}

		~FGridSAVEFIX2TestWorld()
		{
			if (!World)
			{
				return;
			}

			World->DestroyWorld(false);
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
		}
	};

	bool BuildSAVEFIX2LoadablePartyState(FGridPartyInventoryState& OutState, FGuid& OutCharacterId)
	{
		UGridPartyInventoryComponent* Source = NewObject<UGridPartyInventoryComponent>();
		if (!Source)
		{
			return false;
		}
		Source->InitializeDefaultPartyIfNeeded();

		URPGRaceAsset* Race = NewObject<URPGRaceAsset>(Source);
		URPGClassAsset* CharacterClass = NewObject<URPGClassAsset>(Source);
		if (!Race || !CharacterClass)
		{
			return false;
		}

		Race->RaceId = TEXT("SAVEFIX2_Human");
		Race->DisplayName = FText::FromString(TEXT("Humain"));
		CharacterClass->ClassId = TEXT("SAVEFIX2_Warrior");
		CharacterClass->DisplayName = FText::FromString(TEXT("Guerrier"));
		CharacterClass->BaseAttributes = FRPGAttributes{ 15, 11, 13, 9, 9, 9 };
		CharacterClass->HealthAtLevelOne = 18;
		CharacterClass->HealthPerLevel = 8;

		FRPGCharacterCreationRequest Request;
		Request.DisplayName = FText::FromString(TEXT("SAVEFIX2 Hero"));
		Request.RaceDefinition = Race;
		Request.ClassDefinition = CharacterClass;

		FText CreationError;
		if (!Source->CreateInitialCharacter(Request, CreationError) || Source->PartyInventoryState.ActiveCharacters.IsEmpty())
		{
			return false;
		}

		UGridItemDefinitionAsset* MissingDefinition = NewObject<UGridItemDefinitionAsset>(Source);
		if (!MissingDefinition)
		{
			return false;
		}
		MissingDefinition->ItemDefinitionId = SAVEFIX2MissingItemId;
		MissingDefinition->DisplayName = FText::FromString(TEXT("SAVEFIX2 missing item"));
		MissingDefinition->Weight = 1.0f;
		if (!Source->RegisterItemDefinition(MissingDefinition))
		{
			return false;
		}

		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = SAVEFIX2MissingItemId;
		Item.Quantity = 1;
		if (!Source->ApplyItemDefinitionToInstance(Item) || !Source->AddItemToSelectedCharacterInventory(Item))
		{
			return false;
		}

		OutState = Source->PartyInventoryState;
		OutCharacterId = OutState.ActiveCharacters[0].CharacterId;
		return OutCharacterId.IsValid();
	}

	UGridLevelAsset* BuildSAVEFIX2FloorLevel(UObject* Outer)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Outer);
		if (!Level)
		{
			return nullptr;
		}

		Level->Width = 4;
		Level->Height = 4;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		return Level;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridSAVEFIX2ContinueFailureIsNonDestructiveTest, "Grimrock.Save.SAVEFIX.2.ContinueFailureIsNonDestructive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridSAVEFIX2ContinueFailureIsNonDestructiveTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridSAVEFIX2TestSaveSlot SaveSlot;

	FGridPartyInventoryState SavedState;
	FGuid SavedCharacterId;
	TestTrue(TEXT("A loadable party snapshot can be built"), BuildSAVEFIX2LoadablePartyState(SavedState, SavedCharacterId));
	if (!SavedCharacterId.IsValid())
	{
		return false;
	}

	UGrimrockPartySaveGame* SaveGame = NewObject<UGrimrockPartySaveGame>();
	TestNotNull(TEXT("The temporary SaveGame is created"), SaveGame);
	if (!SaveGame)
	{
		return false;
	}

	SaveGame->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion;
	SaveGame->PartyInventoryState = SavedState;
	SaveGame->PartyCellX = 2;
	SaveGame->PartyCellY = 2;
	SaveGame->PartyFacing = EGridEdge::North;
	TestTrue(TEXT("The isolated SAVEFIX.2 save is written"), UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlot.SlotName, SaveSlot.UserIndex));
	TestTrue(TEXT("The temporary save is loadable from the main-menu contract"),
		NewObject<UGrimrockGameInstance>()->HasPartySaveGame(SaveSlot.SlotName, SaveSlot.UserIndex));

	const FGuid SentinelCharacterId = FGuid::NewGuid();

	{
		FGridSAVEFIX2TestWorld TestWorld;
		TestNotNull(TEXT("The test world is created"), TestWorld.World);
		TestNotNull(TEXT("The test GameInstance is created"), TestWorld.GameInstance);
		if (!TestWorld.World || !TestWorld.GameInstance)
		{
			return false;
		}

		FNameProperty* MainMenuLevelProperty = FindFProperty<FNameProperty>(UGrimrockGameInstance::StaticClass(), TEXT("MainMenuLevelName"));
		TestNotNull(TEXT("The main-menu level property is available"), MainMenuLevelProperty);
		if (!MainMenuLevelProperty)
		{
			return false;
		}
		MainMenuLevelProperty->SetPropertyValue_InContainer(TestWorld.GameInstance, NAME_None);
		TestWorld.GameInstance->SetPendingLoadSlot(TEXT("SAVEFIX2_PendingSentinel"), 0);
		TestWorld.GameInstance->SetPendingStartupMode(EGrimrockPartyStartupMode::Continue);

		AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
		AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
		TestNotNull(TEXT("The runtime actor is created"), Runtime);
		TestNotNull(TEXT("The party pawn is created"), Party);
		if (!Runtime || !Party || !Party->PartyInventoryComponent)
		{
			return false;
		}

		Runtime->bApplyLevelStartOnBeginPlay = false;
		Runtime->LevelAsset = BuildSAVEFIX2FloorLevel(Runtime);
		Party->LevelRuntimeActor = Runtime;
		Party->PartyStartupMode = EGrimrockPartyStartupMode::Continue;
		Party->PartySaveSlotName = SaveSlot.SlotName;
		Party->PartySaveUserIndex = SaveSlot.UserIndex;

		FGridPartyInventoryState SentinelState = SavedState;
		SentinelState.ActiveCharacters[0].CharacterId = SentinelCharacterId;
		Party->PartyInventoryComponent->PartyInventoryState = SentinelState;

		AddExpectedError(TEXT("PartySave Load Failed Slot="), EAutomationExpectedErrorFlags::Contains, 1);
		AddExpectedError(TEXT("PartySave ContinueAborted Slot="), EAutomationExpectedErrorFlags::Contains, 1);
		AddExpectedError(TEXT("GrimrockGameInstance ReturnToMainMenu Failed Reason=NoMainMenuLevelName"), EAutomationExpectedErrorFlags::Contains, 1);

		Party->DispatchBeginPlay();

		TestTrue(TEXT("Failed Continue disarms this pawn's EndPlay autosave"), Party->PartySaveSlotName.IsEmpty());
		TestTrue(TEXT("Failed Continue does not reset the runtime party"),
			Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex(0) &&
				Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].CharacterId == SentinelCharacterId);
		TestTrue(TEXT("Failed Continue never opens Character Creation"), !Party->IsCharacterCreationModalActive());
		TestTrue(TEXT("Return-to-menu request clears the pending load slot"), !TestWorld.GameInstance->HasPendingLoadSlot());
		TestTrue(TEXT("The isolated save still exists after failed Continue"), UGameplayStatics::DoesSaveGameExist(SaveSlot.SlotName, SaveSlot.UserIndex));
	}

	TestTrue(TEXT("The isolated save survives world teardown"), UGameplayStatics::DoesSaveGameExist(SaveSlot.SlotName, SaveSlot.UserIndex));

	const UGrimrockPartySaveGame* PreservedSave = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlot.SlotName, SaveSlot.UserIndex));
	TestNotNull(TEXT("The original save remains readable after the failed Continue"), PreservedSave);
	TestTrue(TEXT("The failed Continue did not replace the saved party"),
		PreservedSave && PreservedSave->PartyInventoryState.ActiveCharacters.IsValidIndex(0) &&
			PreservedSave->PartyInventoryState.ActiveCharacters[0].CharacterId == SavedCharacterId);

	return true;
}

#endif

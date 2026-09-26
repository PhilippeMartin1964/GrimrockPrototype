#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Sound/SoundWave.h"

namespace
{
	struct FGridPartyWeightTestWorld
	{
		UWorld* World = nullptr;

		FGridPartyWeightTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("PartyWeight01World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridPartyWeightTestWorld()
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

	UGridLevelAsset* BuildOpenWeightTestLevel(UObject* Outer)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Outer);
		if (!Level)
		{
			return nullptr;
		}
		Level->Width = 3;
		Level->Height = 3;
		Level->CellSize = 200.0f;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
			Cell.NorthWall = EGridWallType::None;
			Cell.EastWall = EGridWallType::None;
			Cell.SouthWall = EGridWallType::None;
			Cell.WestWall = EGridWallType::None;
		}
		return Level;
	}

	bool SetCharacterTestWeight(UGridPartyInventoryComponent* Inventory, int32 CharacterIndex, float Weight)
	{
		if (!Inventory || !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
		{
			return false;
		}

		FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
		if (Character.InventorySlots.IsEmpty())
		{
			Character.InventorySlots.SetNum(1);
		}

		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = TEXT("PARTY_WEIGHT01_TestLoad");
		Item.Quantity = 1;
		Item.Weight = FMath::Max(0.0f, Weight);
		Item.OwnerType = EGridItemOwnerType::CharacterInventory;
		Item.OwnerGuid = Character.CharacterId;
		Item.OwnerCharacterIndex = CharacterIndex;
		Item.EquipmentSlot = EGridEquipmentSlot::None;

		Character.InventorySlots[0].bOccupied = true;
		Character.InventorySlots[0].Item = Item;
		return true;
	}

	void ClearCharacterTestWeight(UGridPartyInventoryComponent* Inventory, int32 CharacterIndex)
	{
		if (Inventory && Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex) &&
			!Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex].InventorySlots.IsEmpty())
		{
			Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex].InventorySlots[0] = FGridInventorySlot();
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyWeightMovementLockTest, "Grimrock.Runtime.PartyWeight01.MovementLock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyWeightMovementLockTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridPartyWeightTestWorld TestWorld;
	TestNotNull(TEXT("Transient test world is created"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	TestNotNull(TEXT("Runtime is spawned"), Runtime);
	TestNotNull(TEXT("Party is spawned"), Party);
	if (!Runtime || !Party || !Party->PartyInventoryComponent)
	{
		return false;
	}

	Runtime->bApplyLevelStartOnBeginPlay = false;
	Runtime->LevelAsset = BuildOpenWeightTestLevel(Runtime);
	TestNotNull(TEXT("Open movement level is created"), Runtime->LevelAsset.Get());
	if (!Runtime->LevelAsset)
	{
		return false;
	}

	UGridPartyInventoryComponent* Inventory = Party->PartyInventoryComponent.Get();
	Inventory->InitializeDefaultPartyIfNeeded();
	TestTrue(TEXT("Default party contains an active character"), Inventory->GetActiveCharacterCount() > 0);
	if (Inventory->GetActiveCharacterCount() <= 0)
	{
		return false;
	}

	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("Weight summary is available"), Inventory->GetCharacterSummary(0, Summary));
	TestFalse(TEXT("Default party is not overloaded"), Inventory->IsAnyActiveCharacterOverloaded());

	TestTrue(TEXT("Weight exactly at capacity can be authored"), SetCharacterTestWeight(Inventory, 0, Summary.MaxWeight));
	TestFalse(TEXT("Weight exactly at MaxWeight is Heavy, not Overloaded"), Inventory->IsAnyActiveCharacterOverloaded());

	TestTrue(TEXT("Weight above capacity can be authored"), SetCharacterTestWeight(Inventory, 0, Summary.MaxWeight + 1.0f));
	TestTrue(TEXT("One overloaded active character overloads party movement"), Inventory->IsAnyActiveCharacterOverloaded());

	Party->SetGridStart(Runtime, 1, 1, EGridEdge::North);
	const FVector StartLocation = Party->GetActorLocation();

	USoundWave* BlockedSound = NewObject<USoundWave>(Party, TEXT("S_PARTY_WEIGHT01_Blocked"));
	Party->BlockedMoveSounds = { BlockedSound };
	Party->bMovementAudioEnabled = true;
	Party->bNativeMovementAudioPlaybackEnabled = false;
	Party->bEnableBlockedMoveFeedback = true;
	Party->BlockedMoveDistance = 15.0f;
	Party->BlockedMoveForwardDuration = 0.08f;
	Party->BlockedMoveReturnDuration = 0.10f;

	const EGridEdge Directions[] = { EGridEdge::North, EGridEdge::South, EGridEdge::West, EGridEdge::East };
	for (int32 DirectionIndex = 0; DirectionIndex < UE_ARRAY_COUNT(Directions); ++DirectionIndex)
	{
		const int32 PreviousSoundRequests = Party->BlockedMoveAudioPlaybackRequestCount;
		TestFalse(TEXT("Overload rejects translation"), Party->TryStartMove(Directions[DirectionIndex]));
		TestTrue(TEXT("Overload starts canonical blocked-movement feedback"), Party->bIsBlockedMoveFeedbackActive);
		TestFalse(TEXT("Overload feedback never enters real movement state"), Party->bIsMoving);
		TestEqual(TEXT("Overload feedback preserves logical X"), Party->CurrentCellX, 1);
		TestEqual(TEXT("Overload feedback preserves logical Y"), Party->CurrentCellY, 1);

		Party->UpdateBlockedMoveFeedback(Party->BlockedMoveForwardDuration);
		TestEqual(TEXT("Overload feedback plays one blocked-impact sound at impact"),
			Party->BlockedMoveAudioPlaybackRequestCount, PreviousSoundRequests + 1);
		TestFalse(TEXT("Overload impact still is not a real translation"), Party->bIsMoving);

		Party->UpdateBlockedMoveFeedback(Party->BlockedMoveReturnDuration);
		TestFalse(TEXT("Overload feedback completes after returning"), Party->bIsBlockedMoveFeedbackActive);
		TestTrue(TEXT("Overload feedback returns exactly to the logical cell"),
			Party->GetActorLocation().Equals(StartLocation, KINDA_SMALL_NUMBER));
	}

	TestTrue(TEXT("Overload still allows rotation in place"), Party->TryStartTurn(true));
	Party->UpdateTurn(Party->TurnDuration);
	TestFalse(TEXT("Rotation completes normally"), Party->bIsTurning);

	ClearCharacterTestWeight(Inventory, 0);
	TestFalse(TEXT("Removing excess weight clears party movement lock"), Inventory->IsAnyActiveCharacterOverloaded());
	TestTrue(TEXT("Translation resumes immediately after overload clears"), Party->TryStartMove(EGridEdge::North));
	TestTrue(TEXT("Normal translation enters movement state"), Party->bIsMoving);
	TestEqual(TEXT("Normal translation updates logical Y"), Party->CurrentCellY, 2);

	return true;
}

#endif

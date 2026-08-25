#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace GridTD012Tests
{
	const FName SentinelHeldItemId(TEXT("TD012_SentinelHeldItem"));

	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
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
				FName(*FString::Printf(TEXT("TD012_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (!World || !GEngine)
			{
				return;
			}

			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
		}

		~FTestWorld()
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

	AGrimrockPartyPawn* SpawnPartyWithTwoCharacters(FAutomationTestBase& Test, UWorld* World)
	{
		AGrimrockPartyPawn* PartyPawn = World ? World->SpawnActor<AGrimrockPartyPawn>() : nullptr;
		if (!PartyPawn || !PartyPawn->PartyInventoryComponent)
		{
			Test.AddError(TEXT("TD01.2 could not spawn a party pawn with its inventory component."));
			return nullptr;
		}

		UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent;
		Inventory->PartyInventoryState = FGridPartyInventoryState();
		Inventory->PartyInventoryState.ActiveCharacters.SetNum(2);
		Inventory->PartyInventoryState.ActiveEquipment.SetNum(2);
		Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId = FGuid::NewGuid();
		Inventory->PartyInventoryState.ActiveCharacters[1].CharacterId = FGuid::NewGuid();
		Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
		Inventory->PartyInventoryState.MaxActiveCharacters = 2;
		return PartyPawn;
	}

	void SetHeldVisualSentinel(AGrimrockPartyPawn* PartyPawn)
	{
		PartyPawn->HeldItemDefinitionId = SentinelHeldItemId;
		PartyPawn->bHasTorchInHand = true;
	}
}

using namespace GridTD012Tests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD012SelectionChangeTest, "Grimrock.TechnicalDebt.TD01_2.PartySelectionHeldVisual.SelectionChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD012SelectionChangeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FTestWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("TD01.2 could not create a transient game world."));
		return false;
	}

	AGrimrockPartyPawn* PartyPawn = SpawnPartyWithTwoCharacters(*this, TestWorld.World);
	if (!PartyPawn)
	{
		return false;
	}
	UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent;

	SetHeldVisualSentinel(PartyPawn);
	TestTrue(TEXT("Selection changes from character 0 to 1 through the authoritative setter"), Inventory->SetSelectedCharacterIndex(1));
	TestEqual(TEXT("Character 1 becomes selected"), Inventory->GetSelectedCharacterIndex(), 1);
	TestTrue(TEXT("Selection notification resynchronizes and clears the stale held visual"), PartyPawn->GetHeldItemDefinitionId().IsNone());
	TestFalse(TEXT("Selection notification clears stale torch presentation state"), PartyPawn->bHasTorchInHand);

	SetHeldVisualSentinel(PartyPawn);
	TestTrue(TEXT("Selection changes from character 1 back to 0 through the authoritative setter"), Inventory->SetSelectedCharacterIndex(0));
	TestEqual(TEXT("Character 0 becomes selected again"), Inventory->GetSelectedCharacterIndex(), 0);
	TestTrue(TEXT("Reverse selection notification also resynchronizes the held visual"), PartyPawn->GetHeldItemDefinitionId().IsNone());
	TestFalse(TEXT("Reverse selection notification also clears stale torch presentation state"), PartyPawn->bHasTorchInHand);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD012SelectedCharacterFilterTest,
	"Grimrock.TechnicalDebt.TD01_2.PartySelectionHeldVisual.SelectedCharacterFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD012SelectedCharacterFilterTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FTestWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("TD01.2 could not create a transient game world."));
		return false;
	}

	AGrimrockPartyPawn* PartyPawn = SpawnPartyWithTwoCharacters(*this, TestWorld.World);
	if (!PartyPawn)
	{
		return false;
	}
	UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent;
	TestTrue(TEXT("Character 1 can be selected for notification filtering"), Inventory->SetSelectedCharacterIndex(1));

	SetHeldVisualSentinel(PartyPawn);
	Inventory->NotifyPartyInventoryChanged(0);
	TestEqual(TEXT("An unrelated character notification does not resynchronize the selected held visual"), PartyPawn->GetHeldItemDefinitionId(), SentinelHeldItemId);
	TestTrue(TEXT("Unrelated character notification preserves current presentation state"), PartyPawn->bHasTorchInHand);

	Inventory->NotifyPartyInventoryChanged(1);
	TestTrue(TEXT("The selected character notification resynchronizes the held visual"), PartyPawn->GetHeldItemDefinitionId().IsNone());
	TestFalse(TEXT("The selected character notification clears stale torch presentation state"), PartyPawn->bHasTorchInHand);

	SetHeldVisualSentinel(PartyPawn);
	Inventory->NotifyPartyInventoryChanged(INDEX_NONE);
	TestTrue(TEXT("Party-wide selection or registry notification resynchronizes the held visual"), PartyPawn->GetHeldItemDefinitionId().IsNone());
	TestFalse(TEXT("Party-wide notification clears stale torch presentation state"), PartyPawn->bHasTorchInHand);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

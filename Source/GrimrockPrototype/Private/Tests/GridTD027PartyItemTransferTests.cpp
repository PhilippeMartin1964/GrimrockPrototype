#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace
{
	struct FGridTD027TestWorld
	{
		UWorld* World = nullptr;

		FGridTD027TestWorld()
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
				FName(*FString::Printf(TEXT("TD027PartyItemTransferWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridTD027TestWorld()
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

	FGridItemInstance GridTD027MakeCursorItem()
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = TEXT("TD027_CursorTransferItem");
		Item.DisplayName = FText::FromString(TEXT("TD02.7 Cursor Transfer Item"));
		Item.Quantity = 2;
		Item.Weight = 0.5f;
		Item.OwnerType = EGridItemOwnerType::World;
		return Item;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD027PartyItemTransferCursorFacadeContractTest, "Grimrock.TechnicalDebt.TD02_7.PartyItemTransfer.CursorFacadeContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD027PartyItemTransferCursorFacadeContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridTD027TestWorld TestWorld;
	TestNotNull(TEXT("The transient world is created"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	TestNotNull(TEXT("The party pawn is spawned"), Party);
	if (!Party || !Party->PartyInventoryComponent)
	{
		return false;
	}

	UGridPartyInventoryComponent* Inventory = Party->PartyInventoryComponent;
	Inventory->InitializeDefaultPartyIfNeeded();
	TestEqual(TEXT("The characterization fixture selects character zero"), Inventory->GetSelectedCharacterIndex(), 0);

	const FGridItemInstance SourceItem = GridTD027MakeCursorItem();
	TestTrue(TEXT("The cursor accepts the transfer stack"), Inventory->SetCursorItem(SourceItem));
	TestTrue(TEXT("The pawn reports the cursor item"), Party->HasCursorItem());

	FGridItemInstance CursorItem;
	TestTrue(TEXT("The pawn exposes the current cursor item"), Party->GetCursorItem(CursorItem));
	TestEqual(TEXT("The cursor preserves the definition id"), CursorItem.ItemDefinitionId, SourceItem.ItemDefinitionId);
	TestEqual(TEXT("The cursor preserves the stack quantity"), CursorItem.Quantity, 2);
	TestEqual(TEXT("The inventory component owns the cursor item as Cursor"), CursorItem.OwnerType, EGridItemOwnerType::Cursor);

	TestFalse(TEXT("Dropping without a runtime level is rejected"), Party->TryDropCursorItemAtCell(0, 0, EGridEdge::None, FVector::ZeroVector));
	TestFalse(TEXT("Throwing without a runtime level is rejected"), Party->TryThrowOneCursorItem(FVector::ForwardVector, EGridItemThrowMode::Throw));
	TestTrue(TEXT("Rejected world transfers leave the cursor populated"), Party->GetCursorItem(CursorItem));
	TestEqual(TEXT("Rejected world transfers do not consume the stack"), CursorItem.Quantity, 2);
	TestEqual(TEXT("Rejected world transfers preserve the runtime identity"), CursorItem.RuntimeObjectId, SourceItem.RuntimeObjectId);

	TestTrue(TEXT("The pawn can return the cursor stack to the selected inventory"), Party->DebugPlaceCursorItemInSelectedInventory());
	TestFalse(TEXT("Returning the stack clears the cursor"), Party->HasCursorItem());
	TestEqual(TEXT("The returned stack is stored in the selected inventory"),
		Inventory->CountItemDefinitionInSelectedCharacterInventory(SourceItem.ItemDefinitionId), 2);

	TestTrue(TEXT("The pawn can take the stored stack back to the cursor"), Party->DebugTakeInventorySlotToCursor(0, 0));
	TestTrue(TEXT("Taking the stored stack repopulates the cursor"), Party->GetCursorItem(CursorItem));
	TestEqual(TEXT("The round trip preserves the stack quantity"), CursorItem.Quantity, 2);
	TestEqual(TEXT("The round trip preserves the runtime identity"), CursorItem.RuntimeObjectId, SourceItem.RuntimeObjectId);
	TestEqual(TEXT("The round trip restores cursor ownership"), CursorItem.OwnerType, EGridItemOwnerType::Cursor);

	FString OwnershipError;
	TestTrue(FString::Printf(TEXT("The cursor round trip keeps exclusive ownership valid: %s"), *OwnershipError),
		Inventory->ValidateInventoryOwnership(OwnershipError));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD027PhysicalThrowRulesTest, "Grimrock.TechnicalDebt.TD02_7.PartyItemTransfer.PhysicalThrowRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD027PhysicalThrowRulesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridItemDefinitionAsset* StoneDefinition = NewObject<UGridItemDefinitionAsset>();
	StoneDefinition->ItemDefinitionId = TEXT("Stone_TD027");
	StoneDefinition->DisplayName = FText::FromString(TEXT("Pierre"));
	StoneDefinition->Weight = 1.0f;
	StoneDefinition->HandUsage = EGridItemHandUsage::OneHanded;
	StoneDefinition->ThrowVisualMode = EGridThrowVisualMode::Tumble;

	TestEqual(TEXT("A stone is explicitly one-handed"), StoneDefinition->GetEffectiveHandUsage(), EGridItemHandUsage::OneHanded);
	TestEqual(TEXT("Strength 10 can throw up to 2.5 kg"), StoneDefinition->GetMaxThrowableWeightForStrength(10), 2.5f);
	TestTrue(TEXT("Strength 10 can throw a 1 kg stone"), StoneDefinition->CanBeThrownByStrength(10));
	const float Strength10Scale = StoneDefinition->GetThrowSpeedScaleForStrength(10);
	TestTrue(TEXT("An allowed stone receives a positive throw speed scale"), Strength10Scale > 0.0f);

	StoneDefinition->Weight = 3.0f;
	TestFalse(TEXT("Strength 10 cannot throw a 3 kg one-handed object"), StoneDefinition->CanBeThrownByStrength(10));
	TestTrue(TEXT("Strength 12 can throw a 3 kg one-handed object"), StoneDefinition->CanBeThrownByStrength(12));

	StoneDefinition->Weight = 1.0f;
	const float Strength20Scale = StoneDefinition->GetThrowSpeedScaleForStrength(20);
	TestTrue(TEXT("Higher Strength increases launch speed for the same object"), Strength20Scale > Strength10Scale);

	StoneDefinition->HandUsage = EGridItemHandUsage::TwoHanded;
	TestFalse(TEXT("A two-handed object is not eligible for the generic one-hand throw"), StoneDefinition->CanBeThrownByStrength(20));

	StoneDefinition->HandUsage = EGridItemHandUsage::OneHanded;
	FGridTD027TestWorld TestWorld;
	if (!TestNotNull(TEXT("The context-action world is created"), TestWorld.World))
	{
		return false;
	}

	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	if (!TestNotNull(TEXT("The party is spawned"), Party) || !Party->PartyInventoryComponent)
	{
		return false;
	}

	UGridPartyInventoryComponent* Inventory = Party->PartyInventoryComponent;
	Inventory->InitializeDefaultPartyIfNeeded();
	const int32 CharacterIndex = Inventory->GetSelectedCharacterIndex();
	if (!Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
	{
		AddError(TEXT("Selected character fixture is invalid."));
		return false;
	}
	Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex].Attributes.Strength = 10;
	TestTrue(TEXT("Stone definition is registered"), Inventory->RegisterItemDefinition(StoneDefinition));

	FGridItemInstance EquippedStone;
	EquippedStone.RuntimeObjectId = FGuid::NewGuid();
	EquippedStone.ItemDefinitionId = StoneDefinition->ItemDefinitionId;
	EquippedStone.DisplayName = StoneDefinition->DisplayName;
	EquippedStone.Quantity = 1;
	EquippedStone.Weight = StoneDefinition->Weight;
	EquippedStone.OwnerType = EGridItemOwnerType::EquipmentSlot;
	EquippedStone.OwnerCharacterIndex = CharacterIndex;
	EquippedStone.EquipmentSlot = EGridEquipmentSlot::MainHand;

	FGridItemActionContext Context;
	Context.PartyPawn = Party;
	Context.Item = EquippedStone;
	Context.ItemDefinition = StoneDefinition;
	Context.CharacterIndex = CharacterIndex;
	Context.EquipmentSlot = EGridEquipmentSlot::MainHand;

	FGridFacingTargetContext FacingTarget;
	TArray<FGridItemContextAction> Actions;
	TestTrue(TEXT("MainHand context actions build"), UGridItemContextActionLibrary::BuildItemContextActions(Context, FacingTarget, Actions));
	const FGridItemContextAction* ThrowAction = Actions.FindByPredicate(
		[](const FGridItemContextAction& Action)
		{
			return Action.ActionType == EGridItemActionType::Throw;
		});
	if (!TestNotNull(TEXT("MainHand exposes the utility Throw action"), ThrowAction))
	{
		return false;
	}
	TestTrue(TEXT("The 1 kg stone throw is enabled for Strength 10"), ThrowAction->bEnabled);

	StoneDefinition->Weight = 3.0f;
	Actions.Reset();
	TestTrue(TEXT("Heavy MainHand context actions still build"), UGridItemContextActionLibrary::BuildItemContextActions(Context, FacingTarget, Actions));
	ThrowAction = Actions.FindByPredicate(
		[](const FGridItemContextAction& Action)
		{
			return Action.ActionType == EGridItemActionType::Throw;
		});
	if (!TestNotNull(TEXT("Too-heavy MainHand still exposes a diagnosed Throw action"), ThrowAction))
	{
		return false;
	}
	TestFalse(TEXT("The 3 kg throw is disabled for Strength 10"), ThrowAction->bEnabled);
	TestFalse(TEXT("The disabled heavy throw explains the reason"), ThrowAction->DisabledReason.IsEmpty());

	return true;
}

#endif

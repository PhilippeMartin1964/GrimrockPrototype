#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridCanonicalLootTestScope.h"

#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "../Runtime/GridPhysicalThrowStrengthReadModel.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridThrownItemActor.h"
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
		Item.Quantity = 2;
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

	FGridCanonicalLootTestScope Scope;
	if (!TestTrue(TEXT("Canonical transfer stack"), Scope.AttachWithWeights(TestWorld.World, {}, [](UGridItemSystemAsset& Root)
	{
		auto* Stack = NewObject<UGridGenericStackableDefinitionAsset>(&Root);
		Stack->ItemId = TEXT("TD027_CursorTransferItem");
		Stack->DisplayName = FText::FromString(TEXT("TD02.7 Cursor Transfer Item"));
		Stack->WeightKg = 0.5f;
		Stack->MaxStack = 2;
		Root.ItemDefinitions.Add(Stack);
	}))) return false;
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
	TestFalse(TEXT("Throwing without a runtime level is rejected"), Party->TryThrowOneCursorItem(FVector::ForwardVector));
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

	FGridTD027TestWorld TestWorld;
	if (!TestNotNull(TEXT("The context-action world is created"), TestWorld.World))
	{
		return false;
	}

	const FName LightStoneId(TEXT("Stone_TD027"));
	const FName HeavyStoneId(TEXT("HeavyStone_TD027"));
	const FName TwoHandedId(TEXT("TwoHanded_TD027"));
	FGridCanonicalLootTestScope Registry;
	if (!TestTrue(TEXT("Register light and heavy stone definitions"), Registry.AttachWithWeights(
		TestWorld.World, {}, [&](UGridItemSystemAsset& Root)
		{
			for (const auto& Pair : TMap<FName, float>{{LightStoneId, 1.0f}, {HeavyStoneId, 3.0f}, {TwoHandedId, 1.0f}})
			{
				auto* Stone = NewObject<UGridOtherEquipmentDefinitionAsset>(&Root);
				Stone->ItemId = Pair.Key;
				Stone->DisplayName = FText::FromName(Pair.Key);
				Stone->WeightKg = Pair.Value;
				Stone->HandPolicy = Pair.Key == TwoHandedId
					? EGridNonArmamentHandPolicy::TwoHanded : EGridNonArmamentHandPolicy::OneHanded;
				Stone->GenericThrow.InitialSpeed = 800.f;
				Stone->EquipmentSlots.Add(EGridEquipmentSlot::MainHand);
				Root.ItemDefinitions.Add(Stone);
			}
		}))) return false;

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

	FGridItemInstance EquippedStone;
	EquippedStone.RuntimeObjectId = FGuid::NewGuid();
	EquippedStone.ItemDefinitionId = LightStoneId;
	EquippedStone.Quantity = 1;
	EquippedStone.OwnerType = EGridItemOwnerType::EquipmentSlot;
	EquippedStone.OwnerCharacterIndex = CharacterIndex;
	EquippedStone.EquipmentSlot = EGridEquipmentSlot::MainHand;

	const auto ResolveThrow = [&](FName Id, int32 Strength)
	{
		FGridItemInstance Physical = EquippedStone;
		Physical.ItemDefinitionId = Id;
		return FGridPhysicalThrowStrengthReadModel::Resolve(Registry.Registry, Physical, Strength);
	};
	const auto Light = ResolveThrow(LightStoneId, 10);
	TestTrue(TEXT("A stone is physically one handed"), Light.bOneHanded);
	TestEqual(TEXT("Strength 10 can throw up to 2.5 kg"), Light.MaxWeightKg, 2.5f);
	TestTrue(TEXT("Strength 10 can throw a 1 kg stone"), Light.bCanThrow && Light.SpeedScale > 0.0f);
	TestFalse(TEXT("Strength 10 refuses a 3 kg stone"), ResolveThrow(HeavyStoneId, 10).bCanThrow);
	TestTrue(TEXT("Strength 12 accepts a 3 kg stone"), ResolveThrow(HeavyStoneId, 12).bCanThrow);
	TestTrue(TEXT("Higher Strength increases launch speed"), ResolveThrow(LightStoneId, 20).SpeedScale > Light.SpeedScale);
	TestFalse(TEXT("Two hands are incompatible with the generic throw"), ResolveThrow(TwoHandedId, 20).bCanThrow);

	FGridItemActionContext Context;
	Context.PartyPawn = Party;
	Context.Item = EquippedStone;
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

	Context.Item.ItemDefinitionId = HeavyStoneId;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD027PhysicalThrowAimingContractTest,
	"Grimrock.TechnicalDebt.TD02_7.PartyItemTransfer.PhysicalThrowAimingContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD027PhysicalThrowAimingContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridTD027TestWorld TestWorld;
	if (!TestNotNull(TEXT("The projectile contract world is created"), TestWorld.World))
	{
		return false;
	}
	AGridThrownItemActor* Projectile = TestWorld.World->SpawnActor<AGridThrownItemActor>();
	if (!TestNotNull(TEXT("The thrown-item projectile is spawned"), Projectile) || !Projectile->CollisionComponent || !Projectile->ProjectileMovementComponent)
	{
		return false;
	}
	TestTrue(TEXT("Projectile movement uses swept collision"), Projectile->ProjectileMovementComponent->bSweepCollision);
	TestEqual(TEXT("Thrown items block on WorldStatic walls"), Projectile->CollisionComponent->GetCollisionResponseToChannel(ECC_WorldStatic), ECR_Block);
	TestEqual(TEXT("Thrown items block on WorldDynamic doors"), Projectile->CollisionComponent->GetCollisionResponseToChannel(ECC_WorldDynamic), ECR_Block);
	return true;
}

#endif

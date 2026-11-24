#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "GridCanonicalWeightTestWorld.h"
#include "../UI/GridInventoryItemMetadataReadModel.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FGridItemInstance GridTD068CreateItem(FName ItemDefinitionId, int32 Quantity = 1)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = ItemDefinitionId;
		Item.Quantity = Quantity;
		return Item;
	}

	void GridTD068PlaceInventoryItem(FGridCharacterInventoryState& Character, int32 SlotIndex, FName ItemDefinitionId)
	{
		if (Character.InventorySlots.Num() <= SlotIndex)
		{
			Character.InventorySlots.SetNum(SlotIndex + 1);
		}
		Character.InventorySlots[SlotIndex].bOccupied = true;
		Character.InventorySlots[SlotIndex].Item = GridTD068CreateItem(ItemDefinitionId);
	}

	FGridCombatHotbarBinding GridTD068MakeBinding(int32 SlotIndex, EGridCombatActionSourcePolicy SourcePolicy, FName SourceDefinitionId)
	{
		FGridCombatHotbarBinding Binding;
		Binding.SlotIndex = SlotIndex;
		Binding.ActionId = FName(*FString::Printf(TEXT("TD068_Action_%d"), SlotIndex));
		Binding.SourcePolicy = SourcePolicy;
		Binding.SourceDefinitionId = SourceDefinitionId;
		if (SourcePolicy == EGridCombatActionSourcePolicy::Equipment)
		{
			Binding.PreferredSourceRuntimeId = FGuid::NewGuid();
			Binding.PreferredEquipmentSlot = EGridEquipmentSlot::MainHand;
		}
		return Binding;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD068PartyInventoryItemDefinitionRegistryContractTest,
	"Grimrock.TechnicalDebt.TD06_8.PartyInventoryItemDefinitionRegistry.Contract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD068PartyInventoryItemDefinitionRegistryContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FName StackId(TEXT("StackApply_TD068"));
	const FName SingleId(TEXT("SingleApply_TD068"));
	const FName ActiveInventoryId(TEXT("ActiveInventory_TD068"));
	const FName PoolInventoryId(TEXT("PoolInventory_TD068"));
	const FName EquipmentId(TEXT("Weapon_StraightSword_01"));
	const FName CursorId(TEXT("Cursor_TD068"));
	const FName HotbarEquipmentId(TEXT("Weapon_Dagger_01"));
	const FName HotbarQuickItemId(TEXT("HotbarQuickItem_TD068"));
	const FName OtherRegisteredId(TEXT("OtherRegistered_TD068"));
	const FName IgnoredSpellId(TEXT("IgnoredSpell_TD068"));
	const FName IgnoredAbilityId(TEXT("IgnoredAbility_TD068"));
	const FText StackName = FText::FromString(TEXT("Registered stack"));
	FGridCanonicalWeightTestWorld World({
		{ SingleId, 4.0f }, { ActiveInventoryId, 1.0f }, { PoolInventoryId, 1.0f },
		{ CursorId, 1.0f }, { HotbarQuickItemId, 1.0f }, { OtherRegisteredId, 1.0f }
	}, [&](UGridItemSystemAsset& Root)
	{
		auto* Stack = NewObject<UGridGenericStackableDefinitionAsset>(&Root);
		Stack->ItemId = StackId;
		Stack->DisplayName = StackName;
		Stack->WeightKg = 2.5f;
		Stack->MaxStack = 3;
		Stack->LightEmitter.bUsePointLight = true;
		Stack->LightEmitter.bDefaultEnabled = true;
		Root.ItemDefinitions.Add(Stack);
	});
	UGridPartyInventoryComponent* Component = World.MakeInventory();
	if (!TestNotNull(TEXT("Inventory with isolated registry"), Component)) return false;
	Component->InitializeDefaultPartyIfNeeded();
	UGridItemSubsystem* Registry = World.Scope.Registry;

	// ARM-CLEAN06.5G.38: historical registration/lookup are native-only.
	TestNull(TEXT("RegisterItemDefinition is no longer reflected"), Component->FindFunction(TEXT("RegisterItemDefinition")));
	TestNull(TEXT("FindItemDefinition is no longer reflected"), Component->FindFunction(TEXT("FindItemDefinition")));
	const UFunction* Projection = Component->FindFunction(TEXT("ApplyItemDefinitionToInstance"));
	TestNotNull(TEXT("ApplyItemDefinitionToInstance remains reflected"), Projection);
	TestTrue(TEXT("ApplyItemDefinitionToInstance remains BlueprintCallable"),
		Projection && Projection->HasAnyFunctionFlags(FUNC_BlueprintCallable));

	// Definition registration is atomic for an entire registry. Reject a
	// colliding root in isolation without touching the working registry.
	auto* DuplicateRoot = DuplicateObject<UGridItemSystemAsset>(
		Registry->LoadConfiguredItemSystemForPreview(), GetTransientPackage());
	auto* Duplicate = NewObject<UGridNonArmamentDefinitionAsset>(DuplicateRoot);
	Duplicate->ItemId = TEXT("Item_Torch");
	Duplicate->DisplayName = FText::FromName(Duplicate->ItemId);
	Duplicate->WeightKg = 99.0f;
	DuplicateRoot->ItemDefinitions.Add(Duplicate);
	auto* RejectedRegistry = NewObject<UGridItemSubsystem>(World.Scope.GI.Get());
	TArray<FString> Errors;
	TestFalse(TEXT("A different asset cannot claim an already registered ItemId"),
		RejectedRegistry->RegisterItemSystem(DuplicateRoot, Errors));
	TestTrue(TEXT("Registry reports invalid or duplicate ItemId"), Errors.ContainsByPredicate(
		[](const FString& Error) { return Error == TEXT("Item system: invalid or duplicate ItemId"); }));
	TestFalse(TEXT("The rejected registry exposes no partial entries"), RejectedRegistry->IsConfigured());
	FGridResolvedItem Torch;
	TestTrue(TEXT("The working registry retains the original torch"),
		Registry->ResolveItem(TEXT("Item_Torch"), 0, Torch) && Torch.WeightKg != 99.0f);
	TestFalse(TEXT("Registry rejects NAME_None"), Registry->ContainsItem(NAME_None));

	FGridItemInstance StackItem = GridTD068CreateItem(StackId, 99);
	TestTrue(TEXT("ApplyItemDefinitionToInstance succeeds for a registered definition"), Component->ApplyItemDefinitionToInstance(StackItem));
	const auto StackMetadata = FGridInventoryItemMetadataReadModel::Resolve(Registry, StackItem);
	TestTrue(TEXT("Isolated definition provides weight without an instance cache"),
		StackMetadata.bResolved && FMath::IsNearlyEqual(StackMetadata.UnitWeightKg, 2.5f));
	TestTrue(TEXT("Isolated definition provides name without an instance cache"),
		StackMetadata.DisplayName.EqualTo(StackName));
	TestEqual(TEXT("Apply clamps a stack to MaxStackSize"), StackItem.Quantity, 3);
	TestFalse(TEXT("Apply preserves the instance-owned disabled light state"), StackItem.bLightsEnabled);

	FGridItemInstance NamedStackItem = GridTD068CreateItem(StackId, 0);
	TestTrue(TEXT("Apply accepts a zero incoming stack quantity and normalizes it"), Component->ApplyItemDefinitionToInstance(NamedStackItem));
	TestTrue(TEXT("Definition name is read from the registry"),
		FGridInventoryItemMetadataReadModel::Resolve(Registry, NamedStackItem)
			.DisplayName.EqualTo(StackName));
	TestEqual(TEXT("Apply clamps a stack quantity to at least one"), NamedStackItem.Quantity, 1);

	FGridItemInstance SingleItem = GridTD068CreateItem(SingleId, 8);
	TestTrue(TEXT("Apply succeeds for a non-stackable definition"), Component->ApplyItemDefinitionToInstance(SingleItem));
	TestEqual(TEXT("Apply forces non-stackable quantity to one"), SingleItem.Quantity, 1);

	FGridItemInstance MissingApplyItem = GridTD068CreateItem(TEXT("MissingApply_TD068"), 7);
	const FGridItemInstance BeforeMissingApply = MissingApplyItem;
	TestFalse(TEXT("Apply rejects a missing definition"), Component->ApplyItemDefinitionToInstance(MissingApplyItem));
	TestEqual(TEXT("Missing-definition apply leaves quantity unchanged"), MissingApplyItem.Quantity, 7);
	TestTrue(TEXT("Missing-definition apply leaves all durable fields unchanged"),
		FGridItemInstance::StaticStruct()->CompareScriptStruct(&MissingApplyItem, &BeforeMissingApply, 0));

	UGridPartyInventoryComponent* SourcesComponent = World.MakeInventory();
	if (!TestNotNull(TEXT("Rehydration source inventory"), SourcesComponent)) return false;
	SourcesComponent->InitializeDefaultPartyIfNeeded();

	GridTD068PlaceInventoryItem(SourcesComponent->PartyInventoryState.ActiveCharacters[0], 0, ActiveInventoryId);

	FGridCharacterInventoryState PoolCharacter;
	PoolCharacter.CharacterId = FGuid::NewGuid();
	GridTD068PlaceInventoryItem(PoolCharacter, 0, PoolInventoryId);
	SourcesComponent->PartyInventoryState.CharacterPool.Add(PoolCharacter);

	if (!SourcesComponent->PartyInventoryState.ActiveEquipment.IsValidIndex(0))
	{
		AddError(TEXT("Default party must provide one ActiveEquipment entry for TD06.8"));
		return false;
	}
	SourcesComponent->PartyInventoryState.ActiveEquipment[0].MainHand = GridTD068CreateItem(EquipmentId);
	SourcesComponent->PartyInventoryState.ActiveEquipment[0].MainHand.ItemLevel = 3;

	SourcesComponent->PartyInventoryState.CursorItem = GridTD068CreateItem(CursorId);
	SourcesComponent->PartyInventoryState.bHasCursorItem = true;

	FGridCharacterInventoryState& ActiveCharacter = SourcesComponent->PartyInventoryState.ActiveCharacters[0];
	if (ActiveCharacter.CombatHotbarSlots.Num() != FGridCombatHotbarBinding::MinimumSlotCount)
	{
		AddError(TEXT("Default hotbar must contain the current minimum slot capacity for TD06.8"));
		return false;
	}
	ActiveCharacter.CombatHotbarSlots[0] = GridTD068MakeBinding(0, EGridCombatActionSourcePolicy::Equipment, HotbarEquipmentId);
	ActiveCharacter.CombatHotbarSlots[1] = GridTD068MakeBinding(1, EGridCombatActionSourcePolicy::QuickItem, HotbarQuickItemId);
	ActiveCharacter.CombatHotbarSlots[2] = GridTD068MakeBinding(2, EGridCombatActionSourcePolicy::Spell, IgnoredSpellId);
	ActiveCharacter.CombatHotbarSlots[3] = GridTD068MakeBinding(3, EGridCombatActionSourcePolicy::Ability, IgnoredAbilityId);

	FName MissingDefinitionId = NAME_None;
	const FGridPartyInventoryState Before = SourcesComponent->PartyInventoryState;
	TestTrue(TEXT("All owned sources resolve via the active registry"),
		SourcesComponent->RehydrateOwnedItems(MissingDefinitionId));
	TestTrue(TEXT("Successful rehydration reports no missing definition"), MissingDefinitionId.IsNone());
	TestTrue(TEXT("All physical instances and shortcuts retain their exact fields"),
		FGridPartyInventoryState::StaticStruct()->CompareScriptStruct(
			&Before, &SourcesComponent->PartyInventoryState, 0));
	TestFalse(TEXT("The spell has no item definition"), Registry->ContainsItem(IgnoredSpellId));
	TestFalse(TEXT("The ability has no item definition"), Registry->ContainsItem(IgnoredAbilityId));
	TestTrue(TEXT("Rehydration does not prune the shared register"), Registry->ContainsItem(OtherRegisteredId));

	const FName MissingOwnedId(TEXT("MissingOwned_TD068"));
	GridTD068PlaceInventoryItem(SourcesComponent->PartyInventoryState.ActiveCharacters[0], 0, MissingOwnedId);
	const FGridPartyInventoryState BeforeFailure = SourcesComponent->PartyInventoryState;
	AddExpectedError(TEXT("Canonical party rehydrate rejected Item=MissingOwned_TD068"),
		EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("Rehydration refuses an unregistered owned ID"),
		SourcesComponent->RehydrateOwnedItems(MissingDefinitionId));
	TestEqual(TEXT("Failed rehydration reports the missing owned ID"), MissingDefinitionId, MissingOwnedId);
	TestTrue(TEXT("Failure preserves the roster, quantity, ownership and GUID"),
		FGridPartyInventoryState::StaticStruct()->CompareScriptStruct(
			&BeforeFailure, &SourcesComponent->PartyInventoryState, 0));
	TestTrue(TEXT("Failure does not alter the shared registry"), Registry->ContainsItem(OtherRegisteredId));

	return true;
}

#endif

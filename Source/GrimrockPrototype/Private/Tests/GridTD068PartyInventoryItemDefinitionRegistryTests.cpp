#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	UGridPartyInventoryComponent* GridTD068CreateInventory()
	{
		UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent>();
		Component->InitializeDefaultPartyIfNeeded();
		return Component;
	}

	UGridItemDefinitionAsset* GridTD068CreateDefinition(
		UObject* Outer, FName ItemDefinitionId, float Weight = 1.0f, bool bStackable = false, int32 MaxStackSize = 1)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Outer);
		Definition->ItemDefinitionId = ItemDefinitionId;
		Definition->DisplayName = FText::FromName(ItemDefinitionId);
		Definition->Weight = Weight;
		Definition->bStackable = bStackable;
		Definition->MaxStackSize = MaxStackSize;
		return Definition;
	}

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

	FGridCombatHotbarBinding GridTD068MakeBinding(
		int32 SlotIndex, EGridCombatActionSourcePolicy SourcePolicy, FName SourceDefinitionId)
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
	"Grimrock.TechnicalDebt.TD06_8.PartyInventoryItemDefinitionRegistry.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD068PartyInventoryItemDefinitionRegistryContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Component = GridTD068CreateInventory();
	if (!TestNotNull(TEXT("The characterized party inventory component is created"), Component))
	{
		return false;
	}

	const FName ReflectedFunctions[] = {
		TEXT("RegisterItemDefinition"),
		TEXT("FindItemDefinition"),
		TEXT("ApplyItemDefinitionToInstance")
	};
	for (const FName FunctionName : ReflectedFunctions)
	{
		const UFunction* Function = Component->FindFunction(FunctionName);
		TestNotNull(FString::Printf(TEXT("%s remains reflected"), *FunctionName.ToString()), Function);
		TestTrue(FString::Printf(TEXT("%s remains BlueprintCallable"), *FunctionName.ToString()),
			Function && Function->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	}

	TestFalse(TEXT("RegisterItemDefinition rejects null"), Component->RegisterItemDefinition(nullptr));
	UGridItemDefinitionAsset* InvalidDefinition = GridTD068CreateDefinition(Component, NAME_None);
	TestFalse(TEXT("RegisterItemDefinition rejects NAME_None"), Component->RegisterItemDefinition(InvalidDefinition));

	UGridItemDefinitionAsset* FirstDuplicateDefinition =
		GridTD068CreateDefinition(Component, TEXT("Duplicate_TD068"), 1.0f);
	UGridItemDefinitionAsset* SecondDuplicateDefinition =
		GridTD068CreateDefinition(Component, TEXT("Duplicate_TD068"), 9.0f);
	TestTrue(TEXT("The first definition registers"), Component->RegisterItemDefinition(FirstDuplicateDefinition));
	TestTrue(TEXT("Registering the same ID again remains successful"), Component->RegisterItemDefinition(SecondDuplicateDefinition));
	TestTrue(TEXT("Duplicate registration keeps the first registered asset"),
		Component->FindItemDefinition(TEXT("Duplicate_TD068")) == FirstDuplicateDefinition);
	TestTrue(TEXT("FindItemDefinition rejects NAME_None"), Component->FindItemDefinition(NAME_None) == nullptr);

	UGridItemDefinitionAsset* StackDefinition =
		GridTD068CreateDefinition(Component, TEXT("StackApply_TD068"), 2.5f, true, 3);
	StackDefinition->DisplayName = FText::FromString(TEXT("Registered stack"));
	StackDefinition->bCanEmitLight = true;
	StackDefinition->bDefaultLightEnabled = true;
	TestTrue(TEXT("The stack definition registers"), Component->RegisterItemDefinition(StackDefinition));

	FGridItemInstance StackItem = GridTD068CreateItem(StackDefinition->ItemDefinitionId, 99);
	TestTrue(TEXT("ApplyItemDefinitionToInstance succeeds for a registered definition"),
		Component->ApplyItemDefinitionToInstance(StackItem));
	TestTrue(TEXT("Apply copies weight"), FMath::IsNearlyEqual(StackItem.Weight, 2.5f));
	TestTrue(TEXT("Apply fills an empty display name"), StackItem.DisplayName.EqualTo(StackDefinition->DisplayName));
	TestEqual(TEXT("Apply clamps a stack to MaxStackSize"), StackItem.Quantity, 3);
	TestTrue(TEXT("Apply copies the default light state"), StackItem.bLightsEnabled);

	FGridItemInstance NamedStackItem = GridTD068CreateItem(StackDefinition->ItemDefinitionId, 0);
	NamedStackItem.DisplayName = FText::FromString(TEXT("Runtime override"));
	TestTrue(TEXT("Apply accepts a zero incoming stack quantity and normalizes it"),
		Component->ApplyItemDefinitionToInstance(NamedStackItem));
	TestTrue(TEXT("Apply preserves an existing display name"),
		NamedStackItem.DisplayName.EqualTo(FText::FromString(TEXT("Runtime override"))));
	TestEqual(TEXT("Apply clamps a stack quantity to at least one"), NamedStackItem.Quantity, 1);

	UGridItemDefinitionAsset* SingleDefinition =
		GridTD068CreateDefinition(Component, TEXT("SingleApply_TD068"), 4.0f, false, 1);
	TestTrue(TEXT("The non-stackable definition registers"), Component->RegisterItemDefinition(SingleDefinition));
	FGridItemInstance SingleItem = GridTD068CreateItem(SingleDefinition->ItemDefinitionId, 8);
	TestTrue(TEXT("Apply succeeds for a non-stackable definition"), Component->ApplyItemDefinitionToInstance(SingleItem));
	TestEqual(TEXT("Apply forces non-stackable quantity to one"), SingleItem.Quantity, 1);

	FGridItemInstance MissingApplyItem = GridTD068CreateItem(TEXT("MissingApply_TD068"), 7);
	MissingApplyItem.Weight = 6.0f;
	MissingApplyItem.DisplayName = FText::FromString(TEXT("Before"));
	TestFalse(TEXT("Apply rejects a missing definition"), Component->ApplyItemDefinitionToInstance(MissingApplyItem));
	TestEqual(TEXT("Missing-definition apply leaves quantity unchanged"), MissingApplyItem.Quantity, 7);
	TestTrue(TEXT("Missing-definition apply leaves weight unchanged"), FMath::IsNearlyEqual(MissingApplyItem.Weight, 6.0f));
	TestTrue(TEXT("Missing-definition apply leaves display name unchanged"),
		MissingApplyItem.DisplayName.EqualTo(FText::FromString(TEXT("Before"))));

	UGridPartyInventoryComponent* SourcesComponent = GridTD068CreateInventory();
	if (!TestNotNull(TEXT("The rehydration source component is created"), SourcesComponent))
	{
		return false;
	}

	const FName ActiveInventoryId = TEXT("ActiveInventory_TD068");
	const FName PoolInventoryId = TEXT("PoolInventory_TD068");
	const FName EquipmentId = TEXT("Equipment_TD068");
	const FName CursorId = TEXT("Cursor_TD068");
	const FName HotbarEquipmentId = TEXT("HotbarEquipment_TD068");
	const FName HotbarQuickItemId = TEXT("HotbarQuickItem_TD068");
	const FName IgnoredSpellId = TEXT("IgnoredSpell_TD068");
	const FName IgnoredAbilityId = TEXT("IgnoredAbility_TD068");
	const FName StaleRegistryId = TEXT("StaleRegistry_TD068");

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

	SourcesComponent->PartyInventoryState.CursorItem = GridTD068CreateItem(CursorId);
	SourcesComponent->PartyInventoryState.bHasCursorItem = true;

	FGridCharacterInventoryState& ActiveCharacter = SourcesComponent->PartyInventoryState.ActiveCharacters[0];
	if (ActiveCharacter.CombatHotbarSlots.Num() != FGridCombatHotbarBinding::SlotCount)
	{
		AddError(TEXT("Default hotbar must contain exactly ten slots for TD06.8"));
		return false;
	}
	ActiveCharacter.CombatHotbarSlots[0] =
		GridTD068MakeBinding(0, EGridCombatActionSourcePolicy::Equipment, HotbarEquipmentId);
	ActiveCharacter.CombatHotbarSlots[1] =
		GridTD068MakeBinding(1, EGridCombatActionSourcePolicy::QuickItem, HotbarQuickItemId);
	ActiveCharacter.CombatHotbarSlots[2] =
		GridTD068MakeBinding(2, EGridCombatActionSourcePolicy::Spell, IgnoredSpellId);
	ActiveCharacter.CombatHotbarSlots[3] =
		GridTD068MakeBinding(3, EGridCombatActionSourcePolicy::Ability, IgnoredAbilityId);

	UGridItemDefinitionAsset* StaleDefinition =
		GridTD068CreateDefinition(SourcesComponent, StaleRegistryId);
	TestTrue(TEXT("A stale registry entry can be seeded before successful rehydration"),
		SourcesComponent->RegisterItemDefinition(StaleDefinition));

	const FName ExpectedIds[] = {
		ActiveInventoryId,
		PoolInventoryId,
		EquipmentId,
		CursorId,
		HotbarEquipmentId,
		HotbarQuickItemId
	};

	TMap<FName, UGridItemDefinitionAsset*> ResolverDefinitions;
	for (const FName ExpectedId : ExpectedIds)
	{
		ResolverDefinitions.Add(ExpectedId, GridTD068CreateDefinition(SourcesComponent, ExpectedId));
	}

	TSet<FName> RequestedIds;
	FName MissingDefinitionId = NAME_None;
	TestTrue(TEXT("RehydrateOwnedItemDefinitions resolves every owned item source"),
		SourcesComponent->RehydrateOwnedItemDefinitions(
			[&ResolverDefinitions, &RequestedIds](FName DefinitionId) -> UGridItemDefinitionAsset*
			{
				RequestedIds.Add(DefinitionId);
				if (UGridItemDefinitionAsset** Found = ResolverDefinitions.Find(DefinitionId))
				{
					return *Found;
				}
				return nullptr;
			},
			MissingDefinitionId));
	TestTrue(TEXT("Successful rehydration reports no missing definition"), MissingDefinitionId.IsNone());
	TestEqual(TEXT("Rehydration deduplicates owned definition IDs"), RequestedIds.Num(), static_cast<int32>(UE_ARRAY_COUNT(ExpectedIds)));
	for (const FName ExpectedId : ExpectedIds)
	{
		TestTrue(FString::Printf(TEXT("Rehydration requests %s"), *ExpectedId.ToString()), RequestedIds.Contains(ExpectedId));
		TestTrue(FString::Printf(TEXT("Rehydration registers %s"), *ExpectedId.ToString()),
			SourcesComponent->FindItemDefinition(ExpectedId) == ResolverDefinitions.FindRef(ExpectedId));
	}
	TestFalse(TEXT("Spell hotbar bindings are not item definitions"), RequestedIds.Contains(IgnoredSpellId));
	TestFalse(TEXT("Ability hotbar bindings are not item definitions"), RequestedIds.Contains(IgnoredAbilityId));
	TestTrue(TEXT("Successful rehydration replaces stale transient registry entries"),
		SourcesComponent->FindItemDefinition(StaleRegistryId) == nullptr);

	UGridPartyInventoryComponent* FailureComponent = GridTD068CreateInventory();
	if (!TestNotNull(TEXT("The failure-path component is created"), FailureComponent))
	{
		return false;
	}

	const FName KeepRegistryId = TEXT("KeepRegistry_TD068");
	const FName MissingOwnedId = TEXT("MissingOwned_TD068");
	UGridItemDefinitionAsset* KeepDefinition = GridTD068CreateDefinition(FailureComponent, KeepRegistryId);
	TestTrue(TEXT("The pre-existing registry entry is seeded"), FailureComponent->RegisterItemDefinition(KeepDefinition));
	GridTD068PlaceInventoryItem(FailureComponent->PartyInventoryState.ActiveCharacters[0], 0, MissingOwnedId);

	MissingDefinitionId = NAME_None;
	TestFalse(TEXT("Rehydration fails when an owned definition cannot be resolved"),
		FailureComponent->RehydrateOwnedItemDefinitions(
			[](FName) -> UGridItemDefinitionAsset*
			{
				return nullptr;
			},
			MissingDefinitionId));
	TestEqual(TEXT("Failed rehydration reports the missing owned ID"), MissingDefinitionId, MissingOwnedId);
	TestTrue(TEXT("Failed rehydration leaves the existing transient registry untouched"),
		FailureComponent->FindItemDefinition(KeepRegistryId) == KeepDefinition);

	UGridItemDefinitionAsset* MismatchedDefinition =
		GridTD068CreateDefinition(FailureComponent, TEXT("WrongResolverId_TD068"));
	MissingDefinitionId = NAME_None;
	TestFalse(TEXT("Rehydration rejects a resolver asset whose ItemDefinitionId does not match the request"),
		FailureComponent->RehydrateOwnedItemDefinitions(
			[MismatchedDefinition](FName) -> UGridItemDefinitionAsset*
			{
				return MismatchedDefinition;
			},
			MissingDefinitionId));
	TestEqual(TEXT("Mismatched rehydration reports the originally requested ID"), MissingDefinitionId, MissingOwnedId);
	TestTrue(TEXT("Mismatched rehydration also preserves the previous registry"),
		FailureComponent->FindItemDefinition(KeepRegistryId) == KeepDefinition);

	return true;
}

#endif

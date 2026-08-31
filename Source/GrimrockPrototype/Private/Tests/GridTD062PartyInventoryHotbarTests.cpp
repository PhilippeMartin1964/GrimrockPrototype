#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	UGridPartyInventoryComponent* GridTD062CreateInventory()
	{
		UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent>();
		Component->InitializeDefaultPartyIfNeeded();
		return Component;
	}

	FGridCombatHotbarBinding GridTD062MakeUniversalBinding(FName ActionId)
	{
		FGridCombatHotbarBinding Binding;
		Binding.ActionId = ActionId;
		Binding.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
		return Binding;
	}

	FGridCombatHotbarBinding GridTD062MakeEquipmentBinding(const FGuid& RuntimeId)
	{
		FGridCombatHotbarBinding Binding;
		Binding.ActionId = TEXT("Attack_TD062Weapon");
		Binding.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
		Binding.SourceDefinitionId = TEXT("Weapon_TD062");
		Binding.PreferredSourceRuntimeId = RuntimeId;
		Binding.PreferredEquipmentSlot = EGridEquipmentSlot::MainHand;
		return Binding;
	}

	FGridCombatHotbarBinding GridTD062MakeQuickItemBinding(FName ItemDefinitionId)
	{
		FGridCombatHotbarBinding Binding;
		Binding.ActionId = FGridCombatHotbarBinding::MakeQuickItemActionId(ItemDefinitionId);
		Binding.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
		Binding.SourceDefinitionId = ItemDefinitionId;
		return Binding;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD062PartyInventoryHotbarContractTest, "Grimrock.TechnicalDebt.TD06_2.PartyInventoryHotbar.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD062PartyInventoryHotbarContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridPartyInventoryComponent* Component = GridTD062CreateInventory();
	if (!TestNotNull(TEXT("The characterized party inventory component is created"), Component))
	{
		return false;
	}

	const FName ReflectedFunctions[] = { TEXT("GetCombatHotbarSlotCount"), TEXT("GetCharacterCombatHotbarBinding"), TEXT("SetCharacterCombatHotbarBinding"),
		TEXT("ClearCharacterCombatHotbarBinding"), TEXT("SetCharacterCombatHotbarBindingFromItem"), TEXT("MoveOrSwapCharacterCombatHotbarBinding") };
	for (const FName FunctionName : ReflectedFunctions)
	{
		const UFunction* Function = Component->FindFunction(FunctionName);
		TestNotNull(FString::Printf(TEXT("%s remains reflected"), *FunctionName.ToString()), Function);
		TestTrue(
			FString::Printf(TEXT("%s remains BlueprintCallable"), *FunctionName.ToString()), Function && Function->HasAnyFunctionFlags(FUNC_BlueprintCallable));
	}

	TestEqual(TEXT("The public hotbar contract exposes exactly ten slots"), Component->GetCombatHotbarSlotCount(), FGridCombatHotbarBinding::SlotCount);
	TestEqual(TEXT("The default character owns exactly ten hotbar slots"), Component->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots.Num(),
		FGridCombatHotbarBinding::SlotCount);
	for (int32 SlotIndex = 0; SlotIndex < FGridCombatHotbarBinding::SlotCount; ++SlotIndex)
	{
		const FGridCombatHotbarBinding& Binding = Component->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[SlotIndex];
		TestEqual(FString::Printf(TEXT("Default slot %d keeps its normalized index"), SlotIndex), Binding.SlotIndex, SlotIndex);
		if (SlotIndex == FGridCombatHotbarBinding::PrimaryAttackSlotIndex)
		{
			TestTrue(TEXT("Default slot 1 owns the protected PrimaryAttack alias"), Binding.IsPrimaryAttackBinding());
		}
		else
		{
			TestTrue(FString::Printf(TEXT("Default slot %d is empty"), SlotIndex), Binding.IsEmpty());
		}
		TestTrue(FString::Printf(TEXT("Default slot %d is structurally valid"), SlotIndex), Binding.IsValid());
	}

	FGridCombatHotbarBinding UniversalBinding = GridTD062MakeUniversalBinding(TEXT("Attack_Unarmed"));
	UniversalBinding.SlotIndex = 9;
	TestFalse(TEXT("A negative character index is rejected"), Component->SetCharacterCombatHotbarBinding(-1, 0, UniversalBinding));
	TestFalse(TEXT("A negative hotbar slot is rejected"), Component->SetCharacterCombatHotbarBinding(0, -1, UniversalBinding));
	TestFalse(TEXT("A slot past the fixed hotbar size is rejected"),
		Component->SetCharacterCombatHotbarBinding(0, FGridCombatHotbarBinding::SlotCount, UniversalBinding));
	TestTrue(TEXT("A valid universal action can be assigned"), Component->SetCharacterCombatHotbarBinding(0, 3, UniversalBinding));

	FGridCombatHotbarBinding ReadBinding;
	TestTrue(TEXT("The assigned universal action can be read"), Component->GetCharacterCombatHotbarBinding(0, 3, ReadBinding));
	TestEqual(TEXT("Set normalizes the stored slot index"), ReadBinding.SlotIndex, 3);
	TestEqual(TEXT("Set preserves the action identity"), ReadBinding.ActionId, FName(TEXT("Attack_Unarmed")));

	FGridCharacterInventoryState SecondCharacter;
	SecondCharacter.CharacterId = FGuid::NewGuid();
	SecondCharacter.DisplayName = FText::FromString(TEXT("TD06 Second Character"));
	Component->PartyInventoryState.ActiveCharacters.Add(SecondCharacter);
	Component->PartyInventoryState.ActiveEquipment.AddDefaulted();
	Component->InitializeDefaultPartyIfNeeded();

	FGridCombatHotbarBinding SecondCharacterBinding;
	TestTrue(TEXT("The corresponding slot of the second character can be read"), Component->GetCharacterCombatHotbarBinding(1, 3, SecondCharacterBinding));
	TestTrue(TEXT("Hotbar bindings remain isolated per character"), SecondCharacterBinding.IsEmpty());

	const FGuid EquipmentRuntimeId = FGuid::NewGuid();
	const FGridCombatHotbarBinding EquipmentBinding = GridTD062MakeEquipmentBinding(EquipmentRuntimeId);
	TestTrue(TEXT("An equipment shortcut can be assigned"), Component->SetCharacterCombatHotbarBinding(0, 1, EquipmentBinding));
	TestTrue(TEXT("Reassigning the same runtime equipment source succeeds"), Component->SetCharacterCombatHotbarBinding(0, 7, EquipmentBinding));

	FGridCombatHotbarBinding OldEquipmentBinding;
	FGridCombatHotbarBinding NewEquipmentBinding;
	Component->GetCharacterCombatHotbarBinding(0, 1, OldEquipmentBinding);
	Component->GetCharacterCombatHotbarBinding(0, 7, NewEquipmentBinding);
	TestTrue(TEXT("Reassigning equipment clears its previous shortcut"), OldEquipmentBinding.IsEmpty());
	TestTrue(TEXT("The new equipment shortcut keeps the runtime identity"), NewEquipmentBinding.PreferredSourceRuntimeId == EquipmentRuntimeId);
	TestEqual(TEXT("The moved equipment shortcut owns its target slot index"), NewEquipmentBinding.SlotIndex, 7);

	FGridItemInstance QuickItem;
	QuickItem.RuntimeObjectId = FGuid::NewGuid();
	QuickItem.ItemDefinitionId = TEXT("Potion_TD062");
	QuickItem.DisplayName = FText::FromString(TEXT("Potion TD06.2"));
	QuickItem.Quantity = 2;
	TestTrue(TEXT("The quick-item source enters the authoritative inventory"), Component->AddItemToCharacterInventory(0, QuickItem));

	const FGridCombatHotbarBinding QuickItemBinding = GridTD062MakeQuickItemBinding(QuickItem.ItemDefinitionId);
	TestTrue(TEXT("A quick-item shortcut can be assigned while its source exists"), Component->SetCharacterCombatHotbarBinding(0, 4, QuickItemBinding));
	TestTrue(TEXT("Reassigning the same quick-item definition succeeds"), Component->SetCharacterCombatHotbarBinding(0, 8, QuickItemBinding));

	FGridCombatHotbarBinding OldQuickItemBinding;
	FGridCombatHotbarBinding NewQuickItemBinding;
	Component->GetCharacterCombatHotbarBinding(0, 4, OldQuickItemBinding);
	Component->GetCharacterCombatHotbarBinding(0, 8, NewQuickItemBinding);
	TestTrue(TEXT("Reassigning a quick item clears its previous shortcut"), OldQuickItemBinding.IsEmpty());
	TestEqual(TEXT("The quick-item shortcut keeps the definition identity"), NewQuickItemBinding.SourceDefinitionId, QuickItem.ItemDefinitionId);
	TestEqual(TEXT("The quick-item source remains in inventory after shortcut assignment"),
		Component->CountItemDefinitionInCharacterInventory(0, QuickItem.ItemDefinitionId), 2);

	TestTrue(TEXT("Clearing a quick-item shortcut succeeds"), Component->ClearCharacterCombatHotbarBinding(0, 8));
	TestEqual(
		TEXT("Clearing a shortcut never consumes its quick-item source"), Component->CountItemDefinitionInCharacterInventory(0, QuickItem.ItemDefinitionId), 2);

	TestTrue(TEXT("Swapping two occupied hotbar slots succeeds atomically"), Component->MoveOrSwapCharacterCombatHotbarBinding(0, 3, 7));
	FGridCombatHotbarBinding SwappedSlotThree;
	FGridCombatHotbarBinding SwappedSlotSeven;
	Component->GetCharacterCombatHotbarBinding(0, 3, SwappedSlotThree);
	Component->GetCharacterCombatHotbarBinding(0, 7, SwappedSlotSeven);
	TestTrue(TEXT("The equipment shortcut moved to slot three"), SwappedSlotThree.PreferredSourceRuntimeId == EquipmentRuntimeId);
	TestEqual(TEXT("The swapped equipment shortcut is reindexed"), SwappedSlotThree.SlotIndex, 3);
	TestEqual(TEXT("The universal action moved to slot seven"), SwappedSlotSeven.ActionId, FName(TEXT("Attack_Unarmed")));
	TestEqual(TEXT("The swapped universal shortcut is reindexed"), SwappedSlotSeven.SlotIndex, 7);

	TestTrue(TEXT("Moving an occupied shortcut into an empty slot succeeds"), Component->MoveOrSwapCharacterCombatHotbarBinding(0, 7, 9));
	Component->GetCharacterCombatHotbarBinding(0, 7, SwappedSlotSeven);
	FGridCombatHotbarBinding MovedSlotNine;
	Component->GetCharacterCombatHotbarBinding(0, 9, MovedSlotNine);
	TestTrue(TEXT("Moving into an empty slot clears the source slot"), SwappedSlotSeven.IsEmpty());
	TestEqual(TEXT("The empty target receives the moved action"), MovedSlotNine.ActionId, FName(TEXT("Attack_Unarmed")));
	TestEqual(TEXT("The moved shortcut is normalized to the target index"), MovedSlotNine.SlotIndex, 9);

	UGridItemDefinitionAsset* StoneDefinition = NewObject<UGridItemDefinitionAsset>(Component);
	StoneDefinition->ItemDefinitionId = TEXT("Stone_TD062");
	StoneDefinition->DisplayName = FText::FromString(TEXT("Pierre TD06.2"));
	StoneDefinition->ItemType = EGridItemType::Misc;
	StoneDefinition->HandUsage = EGridItemHandUsage::OneHanded;
	StoneDefinition->Weight = 1.0f;
	StoneDefinition->ThrowSpeed = 800.0f;
	TestTrue(TEXT("The physical throw definition is registered"), Component->RegisterItemDefinition(StoneDefinition));

	FGridItemInstance Stone;
	Stone.RuntimeObjectId = FGuid::NewGuid();
	Stone.ItemDefinitionId = StoneDefinition->ItemDefinitionId;
	Stone.DisplayName = StoneDefinition->DisplayName;
	Stone.Quantity = 1;
	TestTrue(TEXT("The stone enters inventory"), Component->AddItemToCharacterInventory(0, Stone));
	TestTrue(TEXT("A physical throwable can be assigned directly from inventory"),
		Component->SetCharacterCombatHotbarBindingFromItem(0, 6, Stone, EGridEquipmentSlot::None));
	FGridCombatHotbarBinding StoneBinding;
	Component->GetCharacterCombatHotbarBinding(0, 6, StoneBinding);
	TestTrue(TEXT("The stone shortcut uses the persistent physical-throw identity"), StoneBinding.IsPhysicalThrowItemBinding());
	TestTrue(TEXT("Consuming the last stone succeeds"), Component->RemoveItemDefinitionFromCharacterInventory(0, Stone.ItemDefinitionId, 1));
	Component->GetCharacterCombatHotbarBinding(0, 6, StoneBinding);
	TestTrue(TEXT("The exhausted physical-throw shortcut remains assigned"), StoneBinding.IsPhysicalThrowItemBinding());


	UGridPartyInventoryComponent* LegacySource = GridTD062CreateInventory();
	if (!TestNotNull(TEXT("The legacy hotbar source component is created"), LegacySource))
	{
		return false;
	}
	FGridPartyInventoryState LegacyState = LegacySource->PartyInventoryState;
	LegacyState.bInitialCharacterCreationCompleted = true;
	FGridCharacterInventoryState& LegacyCharacter = LegacyState.ActiveCharacters[0];

	const FGuid DuplicateRuntimeId = FGuid::NewGuid();
	FGridCombatHotbarBinding FirstLegacyEquipment = GridTD062MakeEquipmentBinding(DuplicateRuntimeId);
	FirstLegacyEquipment.SlotIndex = 1;
	LegacyCharacter.CombatHotbarSlots[1] = FirstLegacyEquipment;
	FGridCombatHotbarBinding DuplicateLegacyEquipment = FirstLegacyEquipment;
	DuplicateLegacyEquipment.SlotIndex = 7;
	LegacyCharacter.CombatHotbarSlots[7] = DuplicateLegacyEquipment;
	FGridCombatHotbarBinding MissingQuickItem = GridTD062MakeQuickItemBinding(TEXT("Potion_TD062_Missing"));
	MissingQuickItem.SlotIndex = 4;
	LegacyCharacter.CombatHotbarSlots[4] = MissingQuickItem;

	UGridPartyInventoryComponent* RestoredComponent = NewObject<UGridPartyInventoryComponent>();
	FText RestoreError;
	TestTrue(TEXT("A structurally recoverable legacy hotbar restores successfully"), RestoredComponent->RestorePartyInventoryState(LegacyState, RestoreError));

	FGridCombatHotbarBinding PreservedLegacyEquipment;
	FGridCombatHotbarBinding ClearedLegacyDuplicate;
	FGridCombatHotbarBinding ClearedMissingQuickItem;
	RestoredComponent->GetCharacterCombatHotbarBinding(0, 1, PreservedLegacyEquipment);
	RestoredComponent->GetCharacterCombatHotbarBinding(0, 7, ClearedLegacyDuplicate);
	RestoredComponent->GetCharacterCombatHotbarBinding(0, 4, ClearedMissingQuickItem);
	TestTrue(TEXT("Restore preserves the first equipment shortcut"), PreservedLegacyEquipment.PreferredSourceRuntimeId == DuplicateRuntimeId);
	TestTrue(TEXT("Restore sanitizes duplicate equipment shortcuts"), ClearedLegacyDuplicate.IsEmpty());
	TestTrue(TEXT("Restore sanitizes quick-item shortcuts whose source no longer exists"), ClearedMissingQuickItem.IsEmpty());

	UGridPartyInventoryComponent* AtomicRestoreComponent = GridTD062CreateInventory();
	if (!TestNotNull(TEXT("The atomic restore component is created"), AtomicRestoreComponent))
	{
		return false;
	}
	const FGuid OriginalCharacterId = AtomicRestoreComponent->PartyInventoryState.ActiveCharacters[0].CharacterId;
	FGridPartyInventoryState InvalidState = AtomicRestoreComponent->PartyInventoryState;
	InvalidState.bInitialCharacterCreationCompleted = true;
	InvalidState.ActiveCharacters[0].CombatHotbarSlots.SetNum(FGridCombatHotbarBinding::SlotCount - 1);
	FText InvalidRestoreError;
	TestFalse(TEXT("Restore rejects a hotbar with the wrong fixed slot count"),
		AtomicRestoreComponent->RestorePartyInventoryState(InvalidState, InvalidRestoreError));
	TestTrue(TEXT("Rejected restore reports a hotbar error"), !InvalidRestoreError.IsEmpty());
	TestTrue(TEXT("Rejected restore preserves the previous authoritative character"),
		AtomicRestoreComponent->PartyInventoryState.ActiveCharacters[0].CharacterId == OriginalCharacterId);
	TestEqual(TEXT("Rejected restore preserves the previous valid hotbar"),
		AtomicRestoreComponent->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots.Num(), FGridCombatHotbarBinding::SlotCount);

	return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FRPGCharacterCreationRequest CreateValidCC5Request()
	{
		URPGRaceAsset* Race = NewObject<URPGRaceAsset>();
		Race->RaceId = TEXT("Human");
		Race->DisplayName = FText::FromString(TEXT("Humain"));
		Race->AttributeBonuses = FRPGAttributes{ 1, 1, 1, 1, 1, 1 };

		URPGClassAsset* CharacterClass = NewObject<URPGClassAsset>();
		CharacterClass->ClassId = TEXT("Warrior");
		CharacterClass->DisplayName = FText::FromString(TEXT("Guerrier"));
		CharacterClass->BaseAttributes = FRPGAttributes{ 15, 11, 13, 9, 9, 9 };
		CharacterClass->HealthAtLevelOne = 18;
		CharacterClass->HealthPerLevel = 8;

		FRPGCharacterCreationRequest Request;
		Request.DisplayName = FText::FromString(TEXT("Elias"));
		Request.RaceDefinition = Race;
		Request.ClassDefinition = CharacterClass;
		return Request;
	}

	UGridPartyInventoryComponent* CreateCC5PartyWithEquippedTorch(FGuid& OutTorchId)
	{
		UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent>();
		Component->InitializeDefaultPartyIfNeeded();

		FText Error;
		if (!Component->CreateInitialCharacter(CreateValidCC5Request(), Error))
		{
			return nullptr;
		}

		UGridItemDefinitionAsset* TorchDefinition = NewObject<UGridItemDefinitionAsset>();
		TorchDefinition->ItemDefinitionId = TEXT("Item_Torch");
		TorchDefinition->DisplayName = FText::FromString(TEXT("Torche"));
		TorchDefinition->ItemType = EGridItemType::Torch;
		TorchDefinition->Weight = 1.0f;
		TorchDefinition->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::MainHand);
		TorchDefinition->bCanEmitLight = true;
		TorchDefinition->bDefaultLightEnabled = true;
		Component->RegisterItemDefinition(TorchDefinition);

		FGridItemInstance Torch;
		Torch.RuntimeObjectId = FGuid::NewGuid();
		Torch.ItemDefinitionId = TorchDefinition->ItemDefinitionId;
		Component->ApplyItemDefinitionToInstance(Torch);
		OutTorchId = Torch.RuntimeObjectId;

		if (!Component->AddItemToSelectedCharacterInventory(Torch) || !Component->EquipItemFromInventorySlot(0, 0, EGridEquipmentSlot::MainHand))
		{
			return nullptr;
		}

		return Component;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGPartySaveMemoryRoundTripCC5Test, "Grimrock.CharacterCreation.CC5.SaveMemoryRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGPartySaveMemoryRoundTripCC5Test::RunTest(const FString& Parameters)
{
	FGuid TorchId;
	UGridPartyInventoryComponent* SourceComponent = CreateCC5PartyWithEquippedTorch(TorchId);
	TestNotNull(TEXT("The source party is created"), SourceComponent);
	if (!SourceComponent)
	{
		return false;
	}

	UGrimrockPartySaveGame* SourceSave = NewObject<UGrimrockPartySaveGame>();
	SourceSave->PartyInventoryState = SourceComponent->PartyInventoryState;
	SourceSave->CurrentDungeonLevelId = TEXT("TestLevel");
	SourceSave->PartyCellX = 12;
	SourceSave->PartyCellY = 7;
	SourceSave->PartyFacing = EGridEdge::East;

	FGridLevelRuntimeState LevelState;
	LevelState.LevelId = TEXT("TestLevel");
	LevelState.bHasBeenVisited = true;
	FGridRuntimeObjectPresenceState PresenceState;
	PresenceState.ObjectId = TorchId;
	PresenceState.bRemovedFromInitialPlacement = true;
	LevelState.ObjectPresence.Add(TorchId, PresenceState);
	SourceSave->DungeonRuntimeState.LevelStates.Add(LevelState.LevelId, LevelState);

	TArray<uint8> SaveBytes;
	TestTrue(TEXT("The SaveGame serializes to memory"), UGameplayStatics::SaveGameToMemory(SourceSave, SaveBytes));

	UGrimrockPartySaveGame* LoadedSave = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(SaveBytes));
	TestNotNull(TEXT("The SaveGame deserializes from memory"), LoadedSave);
	if (!LoadedSave)
	{
		return false;
	}

	TestTrue(TEXT("The save version is compatible"), LoadedSave->IsCompatible());
	TestEqual(TEXT("The saved character name survives serialization"), LoadedSave->PartyInventoryState.ActiveCharacters[0].DisplayName.ToString(),
		FString(TEXT("Elias")));
	TestEqual(TEXT("The saved party cell X survives serialization"), LoadedSave->PartyCellX, 12);
	TestEqual(TEXT("The saved party cell Y survives serialization"), LoadedSave->PartyCellY, 7);
	TestTrue(TEXT("The saved facing survives serialization"), LoadedSave->PartyFacing == EGridEdge::East);

	const FGridLevelRuntimeState* LoadedLevelState = LoadedSave->DungeonRuntimeState.LevelStates.Find(TEXT("TestLevel"));
	TestNotNull(TEXT("The dungeon runtime state survives serialization"), LoadedLevelState);
	const FGridRuntimeObjectPresenceState* LoadedPresence = LoadedLevelState ? LoadedLevelState->ObjectPresence.Find(TorchId) : nullptr;
	TestNotNull(TEXT("The picked-up world item presence survives serialization"), LoadedPresence);
	TestTrue(TEXT("The picked-up world item remains removed"), LoadedPresence && LoadedPresence->bRemovedFromInitialPlacement);

	UGridPartyInventoryComponent* RestoredComponent = NewObject<UGridPartyInventoryComponent>();
	FText RestoreError;
	TestTrue(TEXT("The party snapshot restores atomically"), RestoredComponent->RestorePartyInventoryState(LoadedSave->PartyInventoryState, RestoreError));

	TestTrue(TEXT("The transient definition registry is not serialized"), RestoredComponent->FindItemDefinition(TEXT("Item_Torch")) == nullptr);

	UGridItemDefinitionAsset* RestoredTorchDefinition = NewObject<UGridItemDefinitionAsset>();
	RestoredTorchDefinition->ItemDefinitionId = TEXT("Item_Torch");
	RestoredTorchDefinition->DisplayName = FText::FromString(TEXT("Torche"));
	RestoredTorchDefinition->ItemType = EGridItemType::Torch;
	RestoredTorchDefinition->Weight = 1.0f;
	RestoredTorchDefinition->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::MainHand);

	FName MissingDefinitionId = NAME_None;
	TestTrue(TEXT("Owned item definitions are rehydrated after restore"),
		RestoredComponent->RehydrateOwnedItemDefinitions(
			[RestoredTorchDefinition](FName DefinitionId)
			{
				return DefinitionId == RestoredTorchDefinition->ItemDefinitionId ? RestoredTorchDefinition : nullptr;
			},
			MissingDefinitionId));
	TestTrue(TEXT("The restored torch definition is registered"), RestoredComponent->FindItemDefinition(TEXT("Item_Torch")) == RestoredTorchDefinition);
	TestTrue(TEXT("The restored definition keeps its equipment actions"), RestoredTorchDefinition->CanEquipToSlot(EGridEquipmentSlot::MainHand));

	FGridItemInstance EquippedItem;
	TestTrue(TEXT("The torch remains equipped in the main hand"), RestoredComponent->GetEquippedItem(0, EGridEquipmentSlot::MainHand, EquippedItem));
	TestTrue(TEXT("The equipped runtime id is preserved"), EquippedItem.RuntimeObjectId == TorchId);

	FString OwnershipError;
	TestTrue(TEXT("Ownership remains valid after restore"), RestoredComponent->ValidateInventoryOwnership(OwnershipError));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGRejectInvalidPartySaveCC5Test, "Grimrock.CharacterCreation.CC5.RejectInvalidSnapshotAtomically",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGRejectInvalidPartySaveCC5Test::RunTest(const FString& Parameters)
{
	UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent>();
	Component->InitializeDefaultPartyIfNeeded();
	const FGuid PlaceholderId = Component->PartyInventoryState.ActiveCharacters[0].CharacterId;

	FGridPartyInventoryState InvalidState;
	InvalidState.bInitialCharacterCreationCompleted = true;
	InvalidState.MaxActiveCharacters = 6;
	InvalidState.SelectedCharacterIndex = 0;
	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.DisplayName = FText::FromString(TEXT("Corrompu"));
	InvalidState.ActiveCharacters.Add(Character);

	FText Error;
	TestFalse(TEXT("A snapshot with missing equipment is rejected"), Component->RestorePartyInventoryState(InvalidState, Error));
	TestTrue(TEXT("The invalid snapshot returns an error"), !Error.IsEmpty());
	TestTrue(TEXT("The previous party is preserved"), Component->PartyInventoryState.ActiveCharacters[0].CharacterId == PlaceholderId);
	return true;
}

#endif

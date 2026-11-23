#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "GridCanonicalWeightTestWorld.h"
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

	UGridPartyInventoryComponent* CreateCC5PartyWithEquippedTorch(const FGridCanonicalWeightTestWorld& World, FGuid& OutTorchId)
	{
		UGridPartyInventoryComponent* Component = World.MakeInventory();
		if (!Component) return nullptr;
		Component->InitializeDefaultPartyIfNeeded();

		FText Error;
		if (!Component->CreateInitialCharacter(CreateValidCC5Request(), Error))
		{
			return nullptr;
		}

		FGridItemInstance Torch;
		Torch.RuntimeObjectId = FGuid::NewGuid();
		Torch.ItemDefinitionId = TEXT("Item_Torch");
		Torch.bLightsEnabled = true;
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
	FGridCanonicalWeightTestWorld SourceWorld({});
	FGuid TorchId;
	UGridPartyInventoryComponent* SourceComponent = CreateCC5PartyWithEquippedTorch(SourceWorld, TorchId);
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

	FGridCanonicalWeightTestWorld RestoreWorld({});
	UGridPartyInventoryComponent* RestoredComponent = RestoreWorld.MakeInventory();
	if (!TestNotNull(TEXT("Independent restore inventory with ItemSystem"), RestoredComponent)) return false;
	RestoredComponent->DefaultInventorySlotCountPerCharacter = 80;
	FText RestoreError;
	TestTrue(TEXT("The party snapshot restores atomically"), RestoredComponent->RestorePartyInventoryState(LoadedSave->PartyInventoryState, RestoreError));
	TestEqual(TEXT("Restore applies the currently configured inventory capacity"),
		RestoredComponent->PartyInventoryState.ActiveCharacters[0].InventorySlots.Num(), 80);

	FName MissingDefinitionId = NAME_None;
	TestTrue(TEXT("Owned items resolve in the independent registry after restore"),
		RestoredComponent->RehydrateOwnedItems(MissingDefinitionId));
	TestTrue(TEXT("No ID is missing from the restored party"), MissingDefinitionId.IsNone());
	FGridResolvedItem TorchDefinition;
	TestTrue(TEXT("Restored torch resolves from the destination registry"),
		RestoreWorld.Scope.Registry->ResolveItem(TEXT("Item_Torch"), 0, TorchDefinition));
	TestTrue(TEXT("The restored torch can occupy the main hand"),
		TorchDefinition.NonArmEquipmentSlots.Contains(EGridEquipmentSlot::MainHand));

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

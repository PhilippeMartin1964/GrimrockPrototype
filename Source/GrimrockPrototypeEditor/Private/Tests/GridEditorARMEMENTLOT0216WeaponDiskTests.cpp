#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Save/GrimrockPartySaveGame.h"

namespace ARMEMENTLOT0216
{
	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
				.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
				.SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("ARMLOT0216_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FTestWorld()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
			}
		}
	};

	int32 FindBagSlot(const UGridPartyInventoryComponent& Inventory, FName ItemId)
	{
		if (!Inventory.PartyInventoryState.ActiveCharacters.IsValidIndex(0))
		{
			return INDEX_NONE;
		}
		const TArray<FGridInventorySlot>& Bag = Inventory.PartyInventoryState.ActiveCharacters[0].InventorySlots;
		for (int32 Index = 0; Index < Bag.Num(); ++Index)
		{
			if (!Bag[Index].IsEmpty() && Bag[Index].Item.ItemDefinitionId == ItemId)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	void CollectOwnedItems(const FGridPartyInventoryState& State, TArray<const FGridItemInstance*>& Out)
	{
		Out.Reset();
		if (!State.ActiveCharacters.IsValidIndex(0) || !State.ActiveEquipment.IsValidIndex(0))
		{
			return;
		}
		for (const FGridInventorySlot& Slot : State.ActiveCharacters[0].InventorySlots)
		{
			if (!Slot.IsEmpty())
			{
				Out.Add(&Slot.Item);
			}
		}
		const FGridCharacterEquipmentState& Equipment = State.ActiveEquipment[0];
		if (Equipment.MainHand.IsValid())
		{
			Out.Add(&Equipment.MainHand);
		}
		if (Equipment.OffHand.IsValid())
		{
			Out.Add(&Equipment.OffHand);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT0216DiskPersistence,
	"Grimrock.RPG.ARMEMENT.LOT02.DiskPersistence22N1",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENTLOT0216DiskPersistence::RunTest(const FString&)
{
	using namespace ARMEMENTLOT0216;

	// A real disk slot, isolated from player saves and removed on exit.
	const FString SlotName = FString::Printf(TEXT("ARMLOT0216_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	const int32 UserIndex = 0;
	struct FSlotCleanup
	{
		FString Name;
		int32 Index;
		~FSlotCleanup() { UGameplayStatics::DeleteGameInSlot(Name, Index); }
	} Cleanup{SlotName, UserIndex};

	FTestWorld Fixture;
	if (!TestNotNull(TEXT("Transient game world"), Fixture.World))
	{
		return false;
	}
	AGridLevelRuntimeActor* Runtime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Party = Fixture.World->SpawnActor<AGrimrockPartyPawn>();
	if (!TestNotNull(TEXT("Runtime actor"), Runtime) ||
		!TestNotNull(TEXT("Party pawn"), Party) ||
		!TestNotNull(TEXT("Inventory component"), Party->PartyInventoryComponent.Get()))
	{
		return false;
	}
	Runtime->bApplyLevelStartOnBeginPlay = false;
	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
	Level->Width = 4;
	Level->Height = 4;
	Level->EnsureCellCount();
	for (FGridLevelCellData& Cell : Level->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.bBlocksOccupancy = false;
	}
	Runtime->LevelAsset = Level;
	Runtime->CurrentDungeonLevelId = TEXT("ARMLOT0216_Level");
	Runtime->RebuildLevel();
	Party->LevelRuntimeActor = Runtime;
	Party->PartySaveSlotName = SlotName;
	Party->PartySaveUserIndex = UserIndex;
	Party->CurrentCellX = 1;
	Party->CurrentCellY = 1;
	Party->Facing = EGridEdge::North;
	Party->SnapToCurrentCell();

	UGridPartyInventoryComponent* Inventory = Party->PartyInventoryComponent;
	Inventory->DefaultInventorySlotCountPerCharacter = 48;
	Inventory->InitializeDefaultPartyIfNeeded();

	URPGRaceAsset* Race = NewObject<URPGRaceAsset>(Party);
	URPGClassAsset* Class = NewObject<URPGClassAsset>(Party);
	const FString IdentityToken = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	Race->RaceId = FName(*FString::Printf(TEXT("ARMLOT0216_Race_%s"), *IdentityToken));
	Race->DisplayName = FText::FromString(TEXT("ARMLOT0216 Test Race"));
	Class->ClassId = FName(*FString::Printf(TEXT("ARMLOT0216_Class_%s"), *IdentityToken));
	Class->DisplayName = FText::FromString(TEXT("ARMLOT0216 Test Class"));
	Class->BaseAttributes = FRPGAttributes{15, 11, 13, 9, 9, 9};
	Class->HealthAtLevelOne = 20;
	Class->HealthPerLevel = 8;
	Class->BasePhysicalArmor = 2;
	Class->BaseMagicalArmor = 3;
	FRPGCharacterCreationRequest Creation;
	Creation.DisplayName = FText::FromString(TEXT("ARMLOT0216 Hero"));
	Creation.RaceDefinition = Race;
	Creation.ClassDefinition = Class;
	FText Error;
	if (!TestTrue(TEXT("Create finalized character"), Inventory->CreateInitialCharacter(Creation, Error)))
	{
		AddError(Error.ToString());
		return false;
	}

	TArray<FGridWeaponPipelineRow> Rows;
	FString ManifestError;
	if (!TestTrue(TEXT("Read current production manifest"),
		FGridWeaponPipelineManifest::Load(FGridWeaponPipelineManifest::DefaultFile(), Rows, ManifestError)))
	{
		AddError(ManifestError);
		return false;
	}

	TMap<FName, FGridItemInstance> Expected;
	for (const FGridWeaponPipelineRow& Row : Rows)
	{
		if (Row.ProductionState != TEXT("Active"))
		{
			continue;
		}
		UGridItemDefinitionAsset* Definition = LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
		if (!TestNotNull(*FString::Printf(TEXT("%s actual DataAsset"), *Row.ArchetypeId), Definition))
		{
			return false;
		}
		if (!TestTrue(*FString::Printf(TEXT("%s canonical N1 definition"), *Row.ArchetypeId),
			Definition->IsValidDefinition() && Definition->ItemDefinitionId == FName(*Row.ItemDefinitionId) &&
			Definition->WeaponArchetypeId == FName(*Row.ArchetypeId) &&
			Definition->bUseWeaponCatalogue && Definition->DefaultWeaponItemLevel == 1) ||
			!TestTrue(*FString::Printf(TEXT("%s registered"), *Row.ArchetypeId),
				Inventory->RegisterItemDefinition(Definition)))
		{
			return false;
		}
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = Definition->ItemDefinitionId;
		Item.ItemLevel = 1;
		Item.Quantity = 1;
		if (!TestTrue(*FString::Printf(TEXT("%s N1 instance resolved"), *Row.ArchetypeId),
			Inventory->ApplyItemDefinitionToInstance(Item)) ||
			!TestTrue(*FString::Printf(TEXT("%s added to real bag"), *Row.ArchetypeId),
				Inventory->AddItemToCharacterInventory(0, Item)))
		{
			return false;
		}
		Expected.Add(Item.ItemDefinitionId, Item);
	}
	if (!TestEqual(TEXT("Exactly 22 Active N1 items exercised"), Expected.Num(), 22))
	{
		return false;
	}

	// One real ammunition stack accompanies both bows.
	UGridItemDefinitionAsset* Arrows = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Arrows_01.DA_Item_Arrows_01"));
	if (!TestNotNull(TEXT("Real arrow definition"), Arrows) ||
		!TestTrue(TEXT("Register real arrows"), Inventory->RegisterItemDefinition(Arrows)))
	{
		return false;
	}
	FGridItemInstance ArrowStack;
	ArrowStack.RuntimeObjectId = FGuid::NewGuid();
	ArrowStack.ItemDefinitionId = Arrows->ItemDefinitionId;
	ArrowStack.Quantity = 3;
	if (!TestTrue(TEXT("Resolve real arrows"), Inventory->ApplyItemDefinitionToInstance(ArrowStack)) ||
		!TestTrue(TEXT("Add three arrows to bag"), Inventory->AddItemToCharacterInventory(0, ArrowStack)))
	{
		return false;
	}

	auto Equip = [this, Inventory](FName ItemId, EGridEquipmentSlot Slot) -> bool
	{
		const int32 Index = FindBagSlot(*Inventory, ItemId);
		return TestTrue(*FString::Printf(TEXT("Find %s in bag"), *ItemId.ToString()), Index != INDEX_NONE) &&
			TestTrue(*FString::Printf(TEXT("Equip %s"), *ItemId.ToString()),
				Inventory->EquipItemFromInventorySlot(0, Index, Slot));
	};
	if (!Equip(TEXT("Weapon_Stiletto_01"), EGridEquipmentSlot::MainHand) ||
		!Equip(TEXT("Weapon_ArcaneOrb_01"), EGridEquipmentSlot::OffHand))
	{
		return false;
	}
	const int32 MagicBefore = Inventory->GetEffectiveCurrentMagicalArmor(0);
	const FGridItemInstance* ExpectedFocus = Expected.Find(TEXT("Weapon_ArcaneOrb_01"));
	if (!TestNotNull(TEXT("Focus N1 expected"), ExpectedFocus))
	{
		return false;
	}
	UGridItemDefinitionAsset* Focus = Inventory->FindItemDefinition(ExpectedFocus->ItemDefinitionId);
	const int32 FocusBonus = Focus ? Focus->EquipmentStatBonus.MagicalArmorBonus : 0;
	const int32 MainHandMagic = Inventory->FindItemDefinition(TEXT("Weapon_Stiletto_01"))->EquipmentStatBonus.MagicalArmorBonus;
	const int32 FocusSpend = FocusBonus > 0 ? 1 : 0;
	if (!TestTrue(TEXT("Focus N1 magic reserve is nonnegative and catalogue-owned"), FocusBonus >= 0) ||
		!TestEqual(TEXT("Focus counted exactly once"), MagicBefore,
			Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentMagicalArmor + MainHandMagic + FocusBonus) ||
		!TestEqual(TEXT("Focus absorbs an available N1 point only"), Inventory->ApplyMagicalArmorDamage(0, FocusSpend), FocusSpend))
	{
		return false;
	}

	auto CheckRestored = [this, Inventory, &Expected, &ArrowStack](const FString& Phase) -> bool
	{
		const FGridPartyInventoryState& State = Inventory->PartyInventoryState;
		if (!TestTrue(*FString::Printf(TEXT("%s: character/equipment restored"), *Phase),
			State.ActiveCharacters.Num() == 1 && State.ActiveEquipment.Num() == 1))
		{
			return false;
		}
		TArray<const FGridItemInstance*> Owned;
		CollectOwnedItems(State, Owned);
		if (!TestEqual(*FString::Printf(TEXT("%s: 22 weapons plus arrows, no extra instance"), *Phase),
			Owned.Num(), 23))
		{
			return false;
		}
		for (const TPair<FName, FGridItemInstance>& Entry : Expected)
		{
			int32 Matches = 0;
			for (const FGridItemInstance* Item : Owned)
			{
				if (Item->ItemDefinitionId == Entry.Key)
				{
					++Matches;
					if (!TestTrue(*FString::Printf(TEXT("%s: %s GUID, N1 and quantity preserved"), *Phase, *Entry.Key.ToString()),
						Item->RuntimeObjectId == Entry.Value.RuntimeObjectId &&
						Item->ItemLevel == 1 && Item->Quantity == 1))
					{
						return false;
					}
				}
			}
			if (!TestEqual(*FString::Printf(TEXT("%s: %s exists exactly once"), *Phase, *Entry.Key.ToString()), Matches, 1) ||
				!TestNotNull(*FString::Printf(TEXT("%s: %s definition rehydrated"), *Phase, *Entry.Key.ToString()),
					Inventory->FindItemDefinition(Entry.Key)))
			{
				return false;
			}
		}
		const FGridItemInstance* LoadedArrows = nullptr;
		for (const FGridItemInstance* Item : Owned)
		{
			if (Item->ItemDefinitionId == ArrowStack.ItemDefinitionId)
			{
				LoadedArrows = Item;
				break;
			}
		}
		return TestTrue(*FString::Printf(TEXT("%s: arrow stack GUID/quantity/level preserved"), *Phase),
			LoadedArrows && LoadedArrows->RuntimeObjectId == ArrowStack.RuntimeObjectId &&
			LoadedArrows->ItemLevel == ArrowStack.ItemLevel && LoadedArrows->Quantity == 3);
	};

	auto DiskRoundTrip = [this, Party, Inventory, &CheckRestored, &SlotName, UserIndex](const FString& Phase) -> bool
	{
		FString OwnershipError;
		if (!TestTrue(*FString::Printf(TEXT("%s: valid ownership before save"), *Phase),
			Inventory->ValidateInventoryOwnership(OwnershipError)))
		{
			AddError(OwnershipError);
			return false;
		}
		FText SaveError;
		if (!TestTrue(*FString::Printf(TEXT("%s: Pawn writes real disk slot"), *Phase),
			Party->SaveCurrentGame(SaveError)))
		{
			AddError(SaveError.ToString());
			return false;
		}
		const UGrimrockPartySaveGame* Disk = Cast<UGrimrockPartySaveGame>(
			UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
		if (!TestNotNull(*FString::Printf(TEXT("%s: disk SaveGame readable"), *Phase), Disk) ||
			!TestTrue(*FString::Printf(TEXT("%s: compatible version"), *Phase), Disk->IsCompatible()))
		{
			return false;
		}
		// Make the in-memory state visibly different before the real load.
		Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth -= 1;
		FText LoadError;
		if (!TestTrue(*FString::Printf(TEXT("%s: Pawn loads real disk slot"), *Phase),
			Party->LoadCurrentGame(LoadError)))
		{
			AddError(LoadError.ToString());
			return false;
		}
		return CheckRestored(Phase);
	};

	const int32 SavedHealth = Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth;
	const int32 MagicAfterSpend = Inventory->GetEffectiveCurrentMagicalArmor(0);
	if (!DiskRoundTrip(TEXT("ARM070 focus")) ||
		!TestEqual(TEXT("Focus load restores HP"), Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth, SavedHealth) ||
		!TestEqual(TEXT("Focus magic depletion persisted"),
			Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.AbsorbedEquipmentMagicalArmor, FocusSpend) ||
		!TestEqual(TEXT("No double focus bonus after load"), Inventory->GetEffectiveCurrentMagicalArmor(0), MagicAfterSpend))
	{
		return false;
	}

	// A second disk pass exercises the real ARM081 physical reserve.
	if (!TestTrue(TEXT("Unequip focus to bag"),
		Inventory->UnequipItemToInventory(0, EGridEquipmentSlot::OffHand)) ||
		!Equip(TEXT("Weapon_HolyAegis_01"), EGridEquipmentSlot::OffHand))
	{
		return false;
	}
	const int32 PhysicalBefore = Inventory->GetEffectiveCurrentPhysicalArmor(0);
	UGridItemDefinitionAsset* Aegis = Inventory->FindItemDefinition(TEXT("Weapon_HolyAegis_01"));
	const int32 ShieldBonus = Aegis ? Aegis->EquipmentStatBonus.ArmorBonus : 0;
	const int32 MainHandPhysical = Inventory->FindItemDefinition(TEXT("Weapon_Stiletto_01"))->EquipmentStatBonus.ArmorBonus;
	const int32 ShieldSpend = ShieldBonus > 0 ? 1 : 0;
	if (!TestTrue(TEXT("ARM081 N1 physical reserve is nonnegative and catalogue-owned"), ShieldBonus >= 0) ||
		!TestEqual(TEXT("ARM081 protection counted once"), PhysicalBefore,
			Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentPhysicalArmor + MainHandPhysical + ShieldBonus) ||
		!TestEqual(TEXT("ARM081 absorbs an available N1 point only"), Inventory->ApplyPhysicalArmorDamage(0, ShieldSpend), ShieldSpend))
	{
		return false;
	}
	const int32 PhysicalAfterSpend = Inventory->GetEffectiveCurrentPhysicalArmor(0);
	if (!DiskRoundTrip(TEXT("ARM081 aegis")) ||
		!TestEqual(TEXT("ARM081 spent physical reserve persisted"),
			Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, ShieldSpend) ||
		!TestEqual(TEXT("No double physical reserve after load"), Inventory->GetEffectiveCurrentPhysicalArmor(0), PhysicalAfterSpend))
	{
		return false;
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

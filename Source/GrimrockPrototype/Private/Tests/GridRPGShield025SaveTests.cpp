#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Save/GridCombatSavePolicy.h"
#include "Save/GrimrockPartySaveGame.h"

namespace Shield025
{
	struct FIsolatedSlots
	{
		FString Main;
		FString Checkpoint;
		int32 UserIndex = 0;

		FIsolatedSlots()
			: Main(FString::Printf(TEXT("SHIELD025_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)))
			, Checkpoint(FGridCombatSavePolicy::BuildPreCombatCheckpointSlotName(Main))
		{
			UGameplayStatics::DeleteGameInSlot(Main, UserIndex);
			UGameplayStatics::DeleteGameInSlot(Checkpoint, UserIndex);
		}

		~FIsolatedSlots()
		{
			UGameplayStatics::DeleteGameInSlot(Main, UserIndex);
			UGameplayStatics::DeleteGameInSlot(Checkpoint, UserIndex);
		}
	};

	struct FTestWorld
	{
		UWorld* World = nullptr;
		FTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false)
				.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false)
				.ShouldSimulatePhysics(false).SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("SHIELD025World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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

	struct FFixture
	{
		FIsolatedSlots Slots;
		FTestWorld TestWorld;
		AGridLevelRuntimeActor* Runtime = nullptr;
		AGrimrockPartyPawn* Party = nullptr;
		UGridTurnManagerComponent* Manager = nullptr;
		UGridItemDefinitionAsset* ShieldDef = nullptr;
		FGuid EquippedId;
		FGuid BagId;
		FGuid CursorId;
		FGuid CharacterId;
		int32 BagIndex = INDEX_NONE;
		int32 InitialHealth = 0;
		int32 InitialMana = 0;

		bool Initialize()
		{
			if (!TestWorld.World)
			{
				return false;
			}
			Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
			Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
			if (!Runtime || !Party || !Party->PartyInventoryComponent)
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
			Runtime->CurrentDungeonLevelId = TEXT("SHIELD025_Level");
			Runtime->RebuildLevel();
			Party->LevelRuntimeActor = Runtime;
			Party->PartySaveSlotName = Slots.Main;
			Party->PartySaveUserIndex = Slots.UserIndex;
			Party->CurrentCellX = 1;
			Party->CurrentCellY = 1;
			Party->Facing = EGridEdge::North;
			Party->SnapToCurrentCell();

			UGridPartyInventoryComponent* Inventory = Party->PartyInventoryComponent;
			Inventory->InitializeDefaultPartyIfNeeded();

			const FString Token = FGuid::NewGuid().ToString(EGuidFormats::Digits);
			URPGRaceAsset* Race = NewObject<URPGRaceAsset>(Party);
			URPGClassAsset* Class = NewObject<URPGClassAsset>(Party);
			Race->RaceId = FName(*FString::Printf(TEXT("SHIELD025_Race_%s"), *Token));
			Race->DisplayName = FText::FromString(TEXT("SHIELD025 Human"));
			Class->ClassId = FName(*FString::Printf(TEXT("SHIELD025_Class_%s"), *Token));
			Class->DisplayName = FText::FromString(TEXT("SHIELD025 Warrior"));
			Class->BaseAttributes = FRPGAttributes{15, 11, 13, 9, 9, 9};
			Class->HealthAtLevelOne = 20;
			Class->HealthPerLevel = 8;
			Class->BasePhysicalArmor = 2;
			Class->BaseMagicalArmor = 3;

			FRPGCharacterCreationRequest Request;
			Request.DisplayName = FText::FromString(TEXT("SHIELD025 Hero"));
			Request.RaceDefinition = Race;
			Request.ClassDefinition = Class;
			FText Error;
			if (!Inventory->CreateInitialCharacter(Request, Error) ||
				Inventory->PartyInventoryState.ActiveCharacters.Num() != 1 ||
				Inventory->PartyInventoryState.ActiveEquipment.Num() != 1)
			{
				return false;
			}

			ShieldDef = LoadObject<UGridItemDefinitionAsset>(nullptr,
				TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RoundShield_01.DA_Weapon_RoundShield_01"));
			if (!ShieldDef || ShieldDef->EquipmentStatBonus.ArmorBonus != 5 ||
				Runtime->ResolveRuntimeItemDefinition(ShieldDef->ItemDefinitionId) != ShieldDef ||
				!Inventory->RegisterItemDefinition(ShieldDef))
			{
				return false;
			}

			FGridPartyInventoryState& State = Inventory->PartyInventoryState;
			FGridCharacterInventoryState& Character = State.ActiveCharacters[0];
			CharacterId = Character.CharacterId;
			Character.Resources.CurrentHealth = 15;
			Character.Resources.CurrentMana = 7;
			Character.Resources.CurrentPhysicalArmor = 1;
			Character.Resources.CurrentMagicalArmor = 1;
			InitialHealth = Character.Resources.CurrentHealth;
			InitialMana = Character.Resources.CurrentMana;

			auto MakeShield = [this](EGridItemOwnerType Owner, int32 Spent) -> FGridItemInstance
			{
				FGridItemInstance Item;
				Item.RuntimeObjectId = FGuid::NewGuid();
				Item.ItemDefinitionId = ShieldDef->ItemDefinitionId;
				Item.ItemLevel = 1;
				Item.DisplayName = ShieldDef->DisplayName;
				Item.Quantity = 1;
				Item.Weight = ShieldDef->Weight;
				Item.AbsorbedEquipmentPhysicalArmor = Spent;
				Item.OwnerType = Owner;
				Item.OwnerGuid = Owner == EGridItemOwnerType::Cursor ? FGuid() : CharacterId;
				Item.OwnerCharacterIndex = Owner == EGridItemOwnerType::Cursor ? INDEX_NONE : 0;
				Item.EquipmentSlot = Owner == EGridItemOwnerType::EquipmentSlot ? EGridEquipmentSlot::OffHand : EGridEquipmentSlot::None;
				return Item;
			};

			State.ActiveEquipment[0].OffHand = MakeShield(EGridItemOwnerType::EquipmentSlot, 3);
			EquippedId = State.ActiveEquipment[0].OffHand.RuntimeObjectId;
			for (int32 Index = 0; Index < Character.InventorySlots.Num(); ++Index)
			{
				if (Character.InventorySlots[Index].IsEmpty())
				{
					BagIndex = Index;
					break;
				}
			}
			if (BagIndex == INDEX_NONE)
			{
				return false;
			}
			Character.InventorySlots[BagIndex].bOccupied = true;
			Character.InventorySlots[BagIndex].Item = MakeShield(EGridItemOwnerType::CharacterInventory, 4);
			BagId = Character.InventorySlots[BagIndex].Item.RuntimeObjectId;

			State.bHasCursorItem = true;
			State.CursorItem = MakeShield(EGridItemOwnerType::Cursor, 2);
			CursorId = State.CursorItem.RuntimeObjectId;

			FString OwnershipError;
			if (!Inventory->ValidateInventoryOwnership(OwnershipError))
			{
				return false;
			}

			Manager = NewObject<UGridTurnManagerComponent>(Runtime, TEXT("SHIELD025_TurnManager"));
			if (!Manager)
			{
				return false;
			}
			Runtime->AddInstanceComponent(Manager);
			Manager->RegisterComponent();
			if (!Manager->InitializeTurnManager(Runtime, Party))
			{
				return false;
			}
			Manager->CurrentPhase = EGridCombatPhase::Exploration;
			Manager->bCombatActive = false;
			return true;
		}

		const FGridPartyInventoryState& State() const
		{
			return Party->PartyInventoryComponent->PartyInventoryState;
		}

		FGridPartyInventoryState& State()
		{
			return Party->PartyInventoryComponent->PartyInventoryState;
		}

		const UGrimrockPartySaveGame* Read(const FString& Slot) const
		{
			return Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, Slots.UserIndex));
		}

		bool Save()
		{
			FText Error;
			return Party->SaveCurrentGame(Error);
		}

		bool Load()
		{
			FText Error;
			return Party->LoadCurrentGame(Error);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShield025DiskRoundTrip,
	"Grimrock.RPG.SHIELD02.5.ExplorationDiskRoundTripNoFreeRefill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShield025DiskRoundTrip::RunTest(const FString&)
{
	Shield025::FFixture F;
	if (!TestTrue(TEXT("Complete persistent party fixture with real round shield"), F.Initialize()))
	{
		return false;
	}
	auto* Inventory = F.Party->PartyInventoryComponent.Get();
	TestEqual(TEXT("Equipped shield is partially spent before save"), F.State().ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 3);
	TestEqual(TEXT("Effective current physical pool is base1 + shield2"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 3);
	TestTrue(TEXT("Normal exploration save written to an isolated disk slot"), F.Save());
	TestTrue(TEXT("Disk save exists"), UGameplayStatics::DoesSaveGameExist(F.Slots.Main, F.Slots.UserIndex));

	const UGrimrockPartySaveGame* Disk = F.Read(F.Slots.Main);
	if (!TestNotNull(TEXT("Real SaveGame reloaded from disk"), Disk)) return false;
	TestTrue(TEXT("Current save version is compatible"), Disk->IsCompatible());
	TestEqual(TEXT("Disk equipped shield spent amount"), Disk->PartyInventoryState.ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 3);
	TestEqual(TEXT("Disk bag shield spent amount"),
		Disk->PartyInventoryState.ActiveCharacters[0].InventorySlots[F.BagIndex].Item.AbsorbedEquipmentPhysicalArmor, 4);
	TestEqual(TEXT("Disk cursor shield spent amount"), Disk->PartyInventoryState.CursorItem.AbsorbedEquipmentPhysicalArmor, 2);
	TestEqual(TEXT("Disk base physical unchanged"), Disk->PartyInventoryState.ActiveCharacters[0].Resources.CurrentPhysicalArmor, 1);
	TestEqual(TEXT("Disk magical unchanged"), Disk->PartyInventoryState.ActiveCharacters[0].Resources.CurrentMagicalArmor, 1);

	// Make the live state completely different; the load must use disk authority
	// rather than a cached in-memory snapshot or an implicit full-armor reset.
	F.State().ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor = 5;
	F.State().ActiveCharacters[0].InventorySlots[F.BagIndex].Item.AbsorbedEquipmentPhysicalArmor = 0;
	F.State().CursorItem.AbsorbedEquipmentPhysicalArmor = 0;
	F.State().ActiveCharacters[0].Resources.CurrentPhysicalArmor = 0;
	F.State().ActiveCharacters[0].Resources.CurrentMagicalArmor = 0;
	TestTrue(TEXT("Normal pawn LoadCurrentGame accepts disk save"), F.Load());

	const FGridPartyInventoryState& Restored = F.State();
	TestEqual(TEXT("Equipped depletion survived real pawn load"), Restored.ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 3);
	TestEqual(TEXT("Bag depletion survived real pawn load"),
		Restored.ActiveCharacters[0].InventorySlots[F.BagIndex].Item.AbsorbedEquipmentPhysicalArmor, 4);
	TestEqual(TEXT("Cursor depletion survived real pawn load"), Restored.CursorItem.AbsorbedEquipmentPhysicalArmor, 2);
	TestEqual(TEXT("Loaded base pool was not refilled"), Restored.ActiveCharacters[0].Resources.CurrentPhysicalArmor, 1);
	TestEqual(TEXT("Loaded magical pool was not refilled"), Restored.ActiveCharacters[0].Resources.CurrentMagicalArmor, 1);
	TestEqual(TEXT("Effective pool after load is still 3/7"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 3);
	TestEqual(TEXT("Equipped definition rehydrated"), Inventory->FindItemDefinition(F.ShieldDef->ItemDefinitionId), F.ShieldDef);
	TestEqual(TEXT("Equipped GUID persisted"), Restored.ActiveEquipment[0].OffHand.RuntimeObjectId, F.EquippedId);
	TestEqual(TEXT("Bag GUID persisted"), Restored.ActiveCharacters[0].InventorySlots[F.BagIndex].Item.RuntimeObjectId, F.BagId);
	TestEqual(TEXT("Cursor GUID persisted"), Restored.CursorItem.RuntimeObjectId, F.CursorId);
	TestEqual(TEXT("Character identity persisted"), Restored.ActiveCharacters[0].CharacterId, F.CharacterId);
	TestEqual(TEXT("HP unchanged"), Restored.ActiveCharacters[0].Resources.CurrentHealth, F.InitialHealth);
	TestEqual(TEXT("Mana unchanged"), Restored.ActiveCharacters[0].Resources.CurrentMana, F.InitialMana);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShield025PreCombatCheckpoint,
	"Grimrock.RPG.SHIELD02.5.CheckpointAndCombatSavePolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShield025PreCombatCheckpoint::RunTest(const FString&)
{
	Shield025::FFixture F;
	if (!TestTrue(TEXT("Persistent party prepared for checkpoint"), F.Initialize())) return false;
	TestTrue(TEXT("Baseline exploration main slot saved"), F.Save());
	FText Error;
	bool bSkipped = true;
	TestTrue(TEXT("Pre-combat checkpoint uses established MON18.9.1 policy"),
		FGridCombatSavePolicy::PreparePreCombatCheckpoint(F.Party, Error, bSkipped));
	TestFalse(TEXT("Persistent party checkpoint is not skipped"), bSkipped);
	TestTrue(TEXT("Pre-combat disk checkpoint exists"), UGameplayStatics::DoesSaveGameExist(F.Slots.Checkpoint, F.Slots.UserIndex));

	const UGrimrockPartySaveGame* Before = F.Read(F.Slots.Checkpoint);
	if (!TestNotNull(TEXT("Pre-combat checkpoint readable"), Before)) return false;
	TestEqual(TEXT("Checkpoint records shield spent before combat"), Before->PartyInventoryState.ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 3);
	TestEqual(TEXT("Checkpoint records pre-combat magical armor"), Before->PartyInventoryState.ActiveCharacters[0].Resources.CurrentMagicalArmor, 1);

	F.Manager->bCombatActive = true;
	F.Manager->CurrentPhase = EGridCombatPhase::PlayerPhase;
	F.Party->PartyInventoryComponent->ApplyPhysicalArmorDamage(0, 2);
	F.State().ActiveCharacters[0].Resources.CurrentMagicalArmor = 0;
	TestEqual(TEXT("In-combat shield now more depleted"), F.State().ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 5);
	TestFalse(TEXT("Normal disk save denied during active combat"), F.Save());

	const UGrimrockPartySaveGame* CheckpointStill = F.Read(F.Slots.Checkpoint);
	const UGrimrockPartySaveGame* MainStill = F.Read(F.Slots.Main);
	if (!TestNotNull(TEXT("Checkpoint remains available"), CheckpointStill) ||
		!TestNotNull(TEXT("Normal main save remains available"), MainStill)) return false;
	TestEqual(TEXT("Checkpoint was not rewritten mid-combat"), CheckpointStill->PartyInventoryState.ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 3);
	TestEqual(TEXT("Main save was not overwritten by rejected combat save"), MainStill->PartyInventoryState.ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 3);
	TestEqual(TEXT("Checkpoint magic remains pre-combat"), CheckpointStill->PartyInventoryState.ActiveCharacters[0].Resources.CurrentMagicalArmor, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShield025RecoveryDiskRoundTrip,
	"Grimrock.RPG.SHIELD02.5.PostRecoveryDiskRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShield025RecoveryDiskRoundTrip::RunTest(const FString&)
{
	Shield025::FFixture F;
	if (!TestTrue(TEXT("Persistent party with depleted equipment prepared"), F.Initialize())) return false;
	auto* Inventory = F.Party->PartyInventoryComponent.Get();
	TestEqual(TEXT("Pre-rest effective physical armor is 3"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 3);
	TestTrue(TEXT("Explicit rest was accepted outside combat"), F.Manager->TryRestParty());
	TestEqual(TEXT("Rest refilled all effective physical armor"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 7);
	TestEqual(TEXT("Rest refilled magical armor"), F.State().ActiveCharacters[0].Resources.CurrentMagicalArmor, 3);
	TestEqual(TEXT("Rest cleared bag shield depletion"), F.State().ActiveCharacters[0].InventorySlots[F.BagIndex].Item.AbsorbedEquipmentPhysicalArmor, 0);
	TestEqual(TEXT("Rest cleared party cursor shield depletion"), F.State().CursorItem.AbsorbedEquipmentPhysicalArmor, 0);
	TestTrue(TEXT("Post-rest full armor save committed to disk"), F.Save());

	F.State().ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor = 5;
	F.State().ActiveCharacters[0].Resources.CurrentPhysicalArmor = 0;
	F.State().ActiveCharacters[0].Resources.CurrentMagicalArmor = 0;
	TestTrue(TEXT("Post-rest full armor reloaded through Pawn"), F.Load());
	TestEqual(TEXT("Loaded rest restored effective physical pool remains 7"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 7);
	TestEqual(TEXT("Loaded rest restored magical pool remains 3"), F.State().ActiveCharacters[0].Resources.CurrentMagicalArmor, 3);
	TestEqual(TEXT("Loaded full shield is not depleted"), F.State().ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 0);
	TestEqual(TEXT("Loaded full bag shield stays full"), F.State().ActiveCharacters[0].InventorySlots[F.BagIndex].Item.AbsorbedEquipmentPhysicalArmor, 0);
	TestEqual(TEXT("Loaded full cursor shield stays full"), F.State().CursorItem.AbsorbedEquipmentPhysicalArmor, 0);

	// The same recovery service was already covered by SHIELD02.2 Victory tests.
	// Characterize its persistence under the stable post-victory save phase.
	F.Party->PartyInventoryComponent->ApplyPhysicalArmorDamage(0, 4);
	F.State().ActiveCharacters[0].Resources.CurrentMagicalArmor = 0;
	F.Manager->CurrentPhase = EGridCombatPhase::Victory;
	F.Manager->bCombatActive = false;
	TestEqual(TEXT("Authoritative victory service refills the character"), Inventory->RefillArmorReservesAfterVictory(), 1);
	TestTrue(TEXT("Stable victory-phase save accepted"), F.Save());
	F.State().ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor = 5;
	F.State().ActiveCharacters[0].Resources.CurrentMagicalArmor = 0;
	TestTrue(TEXT("Saved victory-phase armor loads successfully"), F.Load());
	TestEqual(TEXT("Loaded victory shield is restored"), F.State().ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 0);
	TestEqual(TEXT("Loaded victory physical pool is 7"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 7);
	TestEqual(TEXT("Loaded victory magical pool is 3"), F.State().ActiveCharacters[0].Resources.CurrentMagicalArmor, 3);
	TestEqual(TEXT("HP never healed by saving/reloading"), F.State().ActiveCharacters[0].Resources.CurrentHealth, F.InitialHealth);
	TestEqual(TEXT("Mana never changed by saving/reloading"), F.State().ActiveCharacters[0].Resources.CurrentMana, F.InitialMana);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

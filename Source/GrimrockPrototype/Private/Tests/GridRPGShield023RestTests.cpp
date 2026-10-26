#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridDirectionUtils.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "RPG/RPGClassAsset.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace Shield023
{
	struct FRestWorld
	{
		UWorld* World = nullptr;

		FRestWorld()
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
				FName(*FString::Printf(TEXT("Shield023_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FRestWorld()
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

	struct FFixture
	{
		FRestWorld TestWorld;
		AGridLevelRuntimeActor* Runtime = nullptr;
		AGrimrockPartyPawn* Party = nullptr;
		UGridTurnManagerComponent* TurnManager = nullptr;
		UGridItemDefinitionAsset* Shield = nullptr;
		URPGClassAsset* CharacterClass = nullptr;

		bool Initialize()
		{
			if (!TestWorld.World)
				return false;

			Shield = LoadObject<UGridItemDefinitionAsset>(nullptr,
				TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RoundShield_01.DA_Weapon_RoundShield_01"));
			Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
			Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
			if (!Shield || !Runtime || !Party || !Party->PartyInventoryComponent ||
				!Party->PartyInventoryComponent->RegisterItemDefinition(Shield))
			{
				return false;
			}
			Runtime->bApplyLevelStartOnBeginPlay = false;
			UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
			Level->Width = 8;
			Level->Height = 8;
			Level->EnsureCellCount();
			for (FGridLevelCellData& Cell : Level->Cells)
			{
				Cell.CellType = EGridCellType::Floor;
				Cell.bBlocksOccupancy = false;
			}
			Runtime->LevelAsset = Level;

			Party->LevelRuntimeActor = Runtime;
			Party->CurrentCellX = 1;
			Party->CurrentCellY = 1;
			Party->Facing = EGridEdge::North;
			Party->SetActorLocation(Runtime->GetCellCenterWorld(1, 1, Party->EyeHeight));
			Party->SetActorRotation(FRotator(0.0f, GridDirectionUtils::ToYaw(Party->Facing), 0.0f));

			CharacterClass = NewObject<URPGClassAsset>(Runtime);
			CharacterClass->ClassId = TEXT("Shield023TestClass");
			CharacterClass->DisplayName = FText::FromString(TEXT("Rest class"));
			CharacterClass->HealthAtLevelOne = 20;
			CharacterClass->BasePhysicalArmor = 2;
			CharacterClass->BaseMagicalArmor = 3;
			if (!CharacterClass->IsValidDefinition())
				return false;

			FGridCharacterInventoryState Character;
			Character.CharacterId = FGuid::NewGuid();
			Character.ClassId = CharacterClass->ClassId;
			Character.ClassDefinition = CharacterClass;
			Character.DisplayName = FText::FromString(TEXT("Elias"));
			Character.Resources.CurrentHealth = 14;
			Character.Resources.CurrentMana = 7;
			Character.Resources.CurrentPhysicalArmor = 0;
			Character.Resources.CurrentMagicalArmor = 0;
			Character.DerivedStats.MaxHealth = 20;
			Character.DerivedStats.MaxMana = 12;
			Character.InventorySlots.SetNum(2);
			auto& State = Party->PartyInventoryComponent->PartyInventoryState;
			State.ActiveCharacters = {Character};
			State.ActiveEquipment.SetNum(1);

			FGridItemInstance ShieldInstance;
			ShieldInstance.RuntimeObjectId = FGuid::NewGuid();
			ShieldInstance.ItemDefinitionId = Shield->ItemDefinitionId;
			ShieldInstance.ItemLevel = 1;
			ShieldInstance.Quantity = 1;
			ShieldInstance.OwnerType = EGridItemOwnerType::EquipmentSlot;
			ShieldInstance.OwnerCharacterIndex = 0;
			ShieldInstance.EquipmentSlot = EGridEquipmentSlot::OffHand;
			ShieldInstance.AbsorbedEquipmentPhysicalArmor = 5;
			State.ActiveEquipment[0].OffHand = ShieldInstance;

			TurnManager = NewObject<UGridTurnManagerComponent>(Runtime, TEXT("Shield023TurnManager"));
			TurnManager->bAutoInitialize = false;
			Runtime->AddInstanceComponent(TurnManager);
			TurnManager->RegisterComponent();
			return TurnManager->InitializeTurnManager(Runtime, Party);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShield023ExplicitRest,
	"Grimrock.RPG.SHIELD02.3.ExplicitRestRefillsBothPools",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShield023ExplicitRest::RunTest(const FString&)
{
	Shield023::FFixture Fixture;
	if (!TestTrue(TEXT("Real world / party / TurnManager / N1 shield initialized"), Fixture.Initialize()))
		return false;

	UGridPartyInventoryComponent* Inventory = Fixture.Party->PartyInventoryComponent;
	auto& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
	auto& Shield = Inventory->PartyInventoryState.ActiveEquipment[0].OffHand;
	const FGuid ShieldId = Shield.RuntimeObjectId;
	const int32 Hp = Character.Resources.CurrentHealth;
	const int32 Mana = Character.Resources.CurrentMana;
	TestEqual(TEXT("Armor consumed before explicit rest"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 0);
	TestEqual(TEXT("Magical armor consumed before explicit rest"), Character.Resources.CurrentMagicalArmor, 0);
	TestEqual(TEXT("Real shield physical bonus"), Fixture.Shield->EquipmentStatBonus.ArmorBonus, 5);

	TestTrue(TEXT("Explicit out-of-combat rest accepted"), Fixture.TurnManager->TryRestParty());
	TestEqual(TEXT("Equipped shield replenished"), Shield.AbsorbedEquipmentPhysicalArmor, 0);
	TestEqual(TEXT("Base physical armor replenished"), Character.Resources.CurrentPhysicalArmor, 2);
	TestEqual(TEXT("Effective armor restored base2 + shield5"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 7);
	TestEqual(TEXT("Magical armor restored"), Character.Resources.CurrentMagicalArmor, 3);
	TestEqual(TEXT("HP not restored"), Character.Resources.CurrentHealth, Hp);
	TestEqual(TEXT("Mana not restored"), Character.Resources.CurrentMana, Mana);
	TestEqual(TEXT("Shield identity preserved"), Shield.RuntimeObjectId, ShieldId);

	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("Inventory summary valid"), Inventory->GetCharacterSummary(0, Summary));
	TestEqual(TEXT("UI summary sees restored physical armor"), Summary.Resources.CurrentPhysicalArmor, 7);
	TestEqual(TEXT("UI summary sees restored magical armor"), Summary.Resources.CurrentMagicalArmor, 3);
	TestTrue(TEXT("Repeated explicit rest accepted, idempotent"), Fixture.TurnManager->TryRestParty());
	TestEqual(TEXT("Repeated rest cannot overfill physical armor"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 7);
	TestEqual(TEXT("Repeated rest cannot overfill magical armor"), Character.Resources.CurrentMagicalArmor, 3);

	// Victory is a terminal presentation phase but still out of combat.
	Fixture.TurnManager->CurrentPhase = EGridCombatPhase::Victory;
	Inventory->ApplyPhysicalArmorDamage(0, 5);
	Character.Resources.CurrentMagicalArmor = 0;
	TestTrue(TEXT("Explicit rest can be accepted after a finished victory"), Fixture.TurnManager->TryRestParty());
	TestEqual(TEXT("Rest after victory refills the depleted shield again"), Shield.AbsorbedEquipmentPhysicalArmor, 0);
	TestEqual(TEXT("Rest after victory restores magic"), Character.Resources.CurrentMagicalArmor, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShield023RestGate,
	"Grimrock.RPG.SHIELD02.3.RestRejectsCombatMovementAndDefeat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShield023RestGate::RunTest(const FString&)
{
	Shield023::FFixture Fixture;
	if (!TestTrue(TEXT("Fixture initialized"), Fixture.Initialize()))
		return false;

	auto& Character = Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0];
	auto& Shield = Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment[0].OffHand;
	Fixture.TurnManager->bCombatActive = true;
	Fixture.TurnManager->CurrentPhase = EGridCombatPhase::PlayerPhase;
	TestFalse(TEXT("Rest forbidden during combat"), Fixture.TurnManager->TryRestParty());
	TestEqual(TEXT("Combat rejection leaves shield depleted"), Shield.AbsorbedEquipmentPhysicalArmor, 5);
	TestEqual(TEXT("Combat rejection leaves magic depleted"), Character.Resources.CurrentMagicalArmor, 0);

	Fixture.TurnManager->bCombatActive = false;
	Fixture.TurnManager->CurrentPhase = EGridCombatPhase::Defeat;
	TestFalse(TEXT("Rest forbidden in Defeat"), Fixture.TurnManager->TryRestParty());

	Fixture.TurnManager->CurrentPhase = EGridCombatPhase::Exploration;
	const FVector OriginalLocation = Fixture.Party->GetActorLocation();
	Fixture.Party->SetActorLocation(OriginalLocation + FVector(120.0f, 0.0f, 0.0f));
	TestFalse(TEXT("Rest denied while party is off grid-cell center"), Fixture.TurnManager->TryRestParty());
	Fixture.Party->SetActorLocation(OriginalLocation);

	Character.Resources.CurrentHealth = 0;
	TestFalse(TEXT("Rest denied if no living party member"), Fixture.TurnManager->TryRestParty());
	TestEqual(TEXT("Dead character's armor untouched"), Shield.AbsorbedEquipmentPhysicalArmor, 5);
	TestEqual(TEXT("Dead character's magic untouched"), Character.Resources.CurrentMagicalArmor, 0);
	TestEqual(TEXT("Dead character remains dead"), Character.Resources.CurrentHealth, 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

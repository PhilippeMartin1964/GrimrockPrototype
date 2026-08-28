#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDirectionUtils.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"

namespace
{
	struct FGridMON11EquipmentWorld
	{
		UWorld* World = nullptr;

		FGridMON11EquipmentWorld()
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
				FName(*FString::Printf(TEXT("MON11EquipmentWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridMON11EquipmentWorld()
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

	FGridOffensiveEquipmentProfile MakeOffensiveProfile(FName AttackId, EGridDamageType DamageType, EGridPhysicalDamageSubtype PhysicalSubtype, int32 MinDamage,
		int32 MaxDamage, EGridAttackScalingAttribute ScalingAttribute, int32 RangeCells, int32 FlatDamageBonus = 0)
	{
		FGridOffensiveEquipmentProfile Profile;
		Profile.AttackId = AttackId;
		Profile.AttackDefinition.DamageType = DamageType;
		Profile.AttackDefinition.PhysicalSubtype = PhysicalSubtype;
		Profile.AttackDefinition.MinDamage = MinDamage;
		Profile.AttackDefinition.MaxDamage = MaxDamage;
		Profile.AttackDefinition.AccuracyBonus = 0;
		Profile.FlatDamageBonus = FlatDamageBonus;
		Profile.DamageScalingAttribute = ScalingAttribute;
		Profile.RangeCells = RangeCells;
		return Profile;
	}

	UGridItemDefinitionAsset* MakeEquipmentDefinition(UObject* Outer, FName ItemDefinitionId, EGridItemType ItemType,
		const FGridOffensiveEquipmentProfile& Profile, EGridEquipmentSlot CompatibleSlot, bool bProvideAttack = true)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Outer);
		Definition->ItemDefinitionId = ItemDefinitionId;
		Definition->DisplayName = FText::FromName(ItemDefinitionId);
		Definition->ItemType = ItemType;
		if (CompatibleSlot != EGridEquipmentSlot::None)
		{
			Definition->CompatibleEquipmentSlots.Add(CompatibleSlot);
		}
		if (bProvideAttack)
		{
			FGridCombatActionDefinition Action;
			Action.ActionId = Profile.AttackId;
			Action.DisplayName = Definition->DisplayName;
			Action.ActionType = Profile.RangeCells > 1 ? EGridCombatActionType::RangedAttack : EGridCombatActionType::MeleeAttack;
			Action.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
			Action.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
			Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
			Action.ActionPointCost = 2;
			Action.RangeCells = Profile.RangeCells;
			Action.PresentationProfileId = Profile.AttackId;
			Action.OffensiveProfile = Profile;
			Definition->CombatActions.Add(Action);
		}
		return Definition;
	}

	FGridItemInstance MakeEquippedItem(FName ItemDefinitionId, int32 CharacterIndex, EGridEquipmentSlot Slot)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = ItemDefinitionId;
		Item.DisplayName = FText::FromName(ItemDefinitionId);
		Item.Quantity = 1;
		Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Item.OwnerCharacterIndex = CharacterIndex;
		Item.EquipmentSlot = Slot;
		return Item;
	}

	int32 FindSeedForNaturalRoll(int32 DesiredRoll)
	{
		for (int32 Seed = 1; Seed < 100000; ++Seed)
		{
			FRandomStream Candidate(Seed);
			if (Candidate.RandRange(1, 20) == DesiredRoll)
			{
				return Seed;
			}
		}
		return INDEX_NONE;
	}

	int32 CountPlayerAttackEntries(const UGridTurnManagerComponent* TurnManager)
	{
		int32 Count = 0;
		if (TurnManager)
		{
			for (const FGridCombatLogEntry& Entry : TurnManager->CombatLogEntries)
			{
				Count += Entry.Type == EGridCombatLogEntryType::AttackHit || Entry.Type == EGridCombatLogEntryType::AttackMiss ? 1 : 0;
			}
		}
		return Count;
	}

	struct FGridMON11EquipmentFixture
	{
		FGridMON11EquipmentWorld TestWorld;
		AGridLevelRuntimeActor* Runtime = nullptr;
		UGridLevelAsset* LevelAsset = nullptr;
		AGrimrockPartyPawn* Party = nullptr;
		UGridMonsterDefinitionAsset* MonsterDefinition = nullptr;
		AGridMonsterActor* Monster = nullptr;
		UGridMonsterOccupancySubsystem* Occupancy = nullptr;
		UGridTurnManagerComponent* TurnManager = nullptr;
		int32 NextMonsterId = 1;

		explicit FGridMON11EquipmentFixture(FIntPoint MonsterCell = FIntPoint(2, 2), bool bSpawnInitialMonster = true)
		{
			if (!TestWorld.World)
			{
				return;
			}

			Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
			LevelAsset = NewObject<UGridLevelAsset>(Runtime);
			LevelAsset->Width = 7;
			LevelAsset->Height = 7;
			LevelAsset->EnsureCellCount();
			for (FGridLevelCellData& Cell : LevelAsset->Cells)
			{
				Cell.CellType = EGridCellType::Floor;
				Cell.bBlocksOccupancy = false;
			}
			Runtime->LevelAsset = LevelAsset;
			Runtime->CurrentDungeonLevelId = TEXT("MON11_EquipmentTest");

			Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
			Party->LevelRuntimeActor = Runtime;
			Party->CurrentCellX = 2;
			Party->CurrentCellY = 1;
			Party->Facing = EGridEdge::North;
			Party->SetActorLocation(Runtime->GetCellCenterWorld(2, 1, Party->EyeHeight));
			Party->SetActorRotation(FRotator(0.0f, GridDirectionUtils::ToYaw(Party->Facing), 0.0f));

			FGridCharacterInventoryState Character;
			Character.CharacterId = FGuid(113, 1, 0, 1);
			Character.DisplayName = FText::FromString(TEXT("Mina"));
			Character.Attributes = FRPGAttributes{ 18, 16, 14, 18, 12, 10 };
			Character.DerivedStats.MaxHealth = 30;
			Character.Resources.CurrentHealth = 30;
			Character.DerivedStats.Accuracy = 6;
			Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { Character };
			Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment.SetNum(1);
			Party->PartyInventoryComponent->PartyInventoryState.SelectedCharacterIndex = 0;

			MonsterDefinition = NewObject<UGridMonsterDefinitionAsset>(Runtime);
			MonsterDefinition->MonsterId = TEXT("MON11_EquipmentRat");
			MonsterDefinition->DisplayName = FText::FromString(TEXT("Rat d'équipement"));
			MonsterDefinition->CategoryId = TEXT("Vermin");
			MonsterDefinition->MaxHealth = 1000;
			MonsterDefinition->PhysicalArmor = 8;
			MonsterDefinition->MagicalArmor = 6;
			MonsterDefinition->Evasion = 3;
			MonsterDefinition->ActionPointsPerTurn = 2;

			Occupancy = TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
			if (bSpawnInitialMonster)
			{
				Monster = SpawnMonster(MonsterCell);
			}

			TurnManager = NewObject<UGridTurnManagerComponent>(Runtime, TEXT("MON11EquipmentTurnManager"));
			TurnManager->bAutoInitialize = false;
			Runtime->AddInstanceComponent(TurnManager);
			TurnManager->RegisterComponent();
			TurnManager->InitializeTurnManager(Runtime, Party);
			TurnManager->bCombatActive = true;
			TurnManager->CurrentPhase = EGridCombatPhase::PlayerPhase;
			TurnManager->RoundNumber = 1;
			if (Monster)
			{
				TurnManager->CombatMonsters = { Monster };
			}
		}

		AGridMonsterActor* SpawnMonster(FIntPoint Cell)
		{
			if (!TestWorld.World || !MonsterDefinition || !Occupancy)
			{
				return nullptr;
			}

			AGridMonsterActor* NewMonster = TestWorld.World->SpawnActor<AGridMonsterActor>();
			NewMonster->InitializeMonster(MonsterDefinition, FGuid(113, 2, 0, NextMonsterId++), Cell, EGridEdge::South);
			Occupancy->RegisterMonster(NewMonster, Cell);
			return NewMonster;
		}

		void SetOnlyCombatMonsters(std::initializer_list<AGridMonsterActor*> Monsters)
		{
			TurnManager->CombatMonsters.Reset();
			for (AGridMonsterActor* Candidate : Monsters)
			{
				TurnManager->CombatMonsters.Add(Candidate);
			}
		}

		void Equip(const UGridItemDefinitionAsset* Definition, EGridEquipmentSlot Slot, bool bRegisterDefinition = true)
		{
			if (bRegisterDefinition)
			{
				Party->PartyInventoryComponent->RegisterItemDefinition(const_cast<UGridItemDefinitionAsset*>(Definition));
			}
			FGridItemInstance* TargetSlot = Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment[0].GetMutableSlot(Slot);
			if (TargetSlot)
			{
				*TargetSlot = MakeEquippedItem(Definition->ItemDefinitionId, 0, Slot);
			}
		}

		void ClearEquipment()
		{
			Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment[0] = FGridCharacterEquipmentState();
		}

		bool IsReady() const
		{
			return TestWorld.World && Runtime && LevelAsset && Party && Party->PartyInventoryComponent && MonsterDefinition && Occupancy && TurnManager;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON11OffensiveProfileValidationTest, "Grimrock.Monsters.MON11.OffensiveProfileValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON11OffensiveProfileValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGridOffensiveEquipmentProfile PhysicalProfile = MakeOffensiveProfile(
		TEXT("Attack_TestPhysical"), EGridDamageType::Physical, EGridPhysicalDamageSubtype::Slashing, 1, 8, EGridAttackScalingAttribute::Strength, 1);
	TestTrue(TEXT("A physical offensive profile is valid"), PhysicalProfile.IsValid());

	const FGridOffensiveEquipmentProfile ElementalProfile = MakeOffensiveProfile(
		TEXT("Attack_TestFire"), EGridDamageType::Fire, EGridPhysicalDamageSubtype::None, 1, 6, EGridAttackScalingAttribute::Intelligence, 3);
	TestTrue(TEXT("An elemental profile with no physical subtype is valid"), ElementalProfile.IsValid());

	FGridOffensiveEquipmentProfile Invalid = PhysicalProfile;
	Invalid.AttackId = NAME_None;
	TestFalse(TEXT("An empty AttackId is rejected"), Invalid.IsValid());
	Invalid = PhysicalProfile;
	Invalid.AttackDefinition.MinDamage = 9;
	TestFalse(TEXT("MinDamage above MaxDamage is rejected"), Invalid.IsValid());
	Invalid = PhysicalProfile;
	Invalid.AttackDefinition.MinDamage = 0;
	Invalid.AttackDefinition.MaxDamage = 0;
	TestFalse(TEXT("A zero MaxDamage is rejected"), Invalid.IsValid());
	Invalid = PhysicalProfile;
	Invalid.RangeCells = 0;
	TestFalse(TEXT("Range zero is rejected"), Invalid.IsValid());
	Invalid.RangeCells = 33;
	TestFalse(TEXT("Range above 32 is rejected"), Invalid.IsValid());
	Invalid = ElementalProfile;
	Invalid.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
	TestFalse(TEXT("An elemental physical subtype is rejected"), Invalid.IsValid());

	UGridItemDefinitionAsset* HeadDefinition =
		MakeEquipmentDefinition(GetTransientPackage(), TEXT("Item_HeadCombatAction"), EGridItemType::Weapon, PhysicalProfile, EGridEquipmentSlot::Head);
	TestTrue(TEXT("A well-formed CombatActions item remains structurally valid outside hand slots"), HeadDefinition->IsValidDefinition());
	TestFalse(TEXT("A head slot never exposes an equipment attack"), HeadDefinition->CanProvideAttackFromSlot(EGridEquipmentSlot::Head));
	TestFalse(TEXT("The head-only definition is not a main-hand attack source"), HeadDefinition->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand));

	FGridMON11EquipmentFixture Fixture;
	if (!TestTrue(TEXT("The rejection fixture is ready"), Fixture.IsReady() && Fixture.Monster))
	{
		return false;
	}

	UGridItemDefinitionAsset* MissingDefinition =
		MakeEquipmentDefinition(Fixture.Runtime, TEXT("Item_MissingDefinition"), EGridItemType::Weapon, PhysicalProfile, EGridEquipmentSlot::MainHand);
	Fixture.Equip(MissingDefinition, EGridEquipmentSlot::MainHand, false);
	constexpr int32 RefusalSeed = 424242;
	Fixture.TurnManager->CombatRandomStream.Initialize(RefusalSeed);
	const int32 HealthBefore = Fixture.Monster->CurrentHealth;
	const int32 ArmorBefore = Fixture.Monster->CurrentPhysicalArmor;
	FGridPlayerAttackRequest Request;
	FGridAttackResult Result;
	EGridPlayerAttackRejectReason RejectReason = EGridPlayerAttackRejectReason::None;
	TestFalse(TEXT("A missing equipped definition is rejected"), Fixture.TurnManager->RequestCharacterAttack(0, Request, Result, RejectReason));
	TestEqual(TEXT("The missing definition reason is explicit"), RejectReason, EGridPlayerAttackRejectReason::EquippedItemDefinitionUnavailable);
	TestEqual(TEXT("The missing definition consumes no random draw"), Fixture.TurnManager->CombatRandomStream.GetCurrentSeed(), RefusalSeed);

	Fixture.ClearEquipment();
	UGridItemDefinitionAsset* InvalidDefinition =
		MakeEquipmentDefinition(Fixture.Runtime, TEXT("Item_InvalidOffense"), EGridItemType::Weapon, PhysicalProfile, EGridEquipmentSlot::MainHand);
	InvalidDefinition->CombatActions[0].OffensiveProfile.RangeCells = 0;
	Fixture.Equip(InvalidDefinition, EGridEquipmentSlot::MainHand);
	TestFalse(TEXT("An invalid offensive profile is rejected"), Fixture.TurnManager->RequestCharacterAttack(0, Request, Result, RejectReason));
	TestEqual(TEXT("The invalid profile reason is explicit"), RejectReason, EGridPlayerAttackRejectReason::InvalidOffensiveEquipment);
	FGridPlayerCharacterTurnState TurnState;
	TestTrue(TEXT("The rejected character turn state is available"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, TurnState));
	TestEqual(TEXT("No rejected attack consumes action points"), TurnState.RemainingActionPoints, 4);
	TestEqual(TEXT("No rejected attack emits an attack log"), CountPlayerAttackEntries(Fixture.TurnManager), 0);
	TestEqual(TEXT("No rejected attack emits Requested"), Fixture.TurnManager->PlayerAttackRequestedBroadcastCount, 0);
	TestEqual(TEXT("No rejected attack emits Resolved"), Fixture.TurnManager->PlayerAttackResolvedBroadcastCount, 0);
	TestEqual(TEXT("Rejected attacks leave health unchanged"), Fixture.Monster->CurrentHealth, HealthBefore);
	TestEqual(TEXT("Rejected attacks leave armor unchanged"), Fixture.Monster->CurrentPhysicalArmor, ArmorBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON11EquippedWeaponMappingTest, "Grimrock.Monsters.MON11.EquippedWeaponMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON11EquippedWeaponMappingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON11EquipmentFixture Fixture;
	if (!TestTrue(TEXT("The sword fixture is ready"), Fixture.IsReady() && Fixture.Monster))
	{
		return false;
	}

	const FGridOffensiveEquipmentProfile SwordProfile = MakeOffensiveProfile(
		TEXT("Attack_Sword"), EGridDamageType::Physical, EGridPhysicalDamageSubtype::Slashing, 1, 8, EGridAttackScalingAttribute::Strength, 1);
	UGridItemDefinitionAsset* SwordDefinition =
		MakeEquipmentDefinition(Fixture.Runtime, TEXT("Item_TestSword"), EGridItemType::Weapon, SwordProfile, EGridEquipmentSlot::MainHand);
	SwordDefinition->EquipmentStatBonus.StrengthBonus = 2;
	Fixture.Equip(SwordDefinition, EGridEquipmentSlot::MainHand);

	FGridMonsterDamageModifier SlashingModifier;
	SlashingModifier.DamageType = EGridDamageType::Physical;
	SlashingModifier.PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
	SlashingModifier.DamageMultiplier = 1.25f;
	Fixture.MonsterDefinition->DamageModifiers.Add(SlashingModifier);

	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("The final sword wielder summary is available"), Fixture.Party->PartyInventoryComponent->GetCharacterSummary(0, Summary));
	FGridAttackSourceStats Source;
	FGridAttackTargetStats Target;
	FGridAttackDefinition Attack;
	TestTrue(TEXT("The exact sword inputs are built"),
		Fixture.TurnManager->BuildPlayerAttackResolutionInputs(Summary, Fixture.Monster, SwordProfile, Source, Target, Attack));
	TestEqual(TEXT("Final derived accuracy is used"), Source.Accuracy, Summary.DerivedStats.Accuracy);
	TestEqual(TEXT("Final Strength drives sword damage"), Source.DamageBonus, URPGCharacterRulesLibrary::GetAttributeModifier(Summary.Attributes.Strength));
	TestEqual(TEXT("The sword definition is copied exactly"), Attack.MaxDamage, SwordProfile.AttackDefinition.MaxDamage);
	TestEqual(TEXT("The Slashing multiplier is selected"), Target.DamageMultiplier, 1.25f);

	const int32 HealthBefore = Fixture.Monster->CurrentHealth;
	const int32 ArmorBefore = Fixture.Monster->CurrentPhysicalArmor;
	FGridPlayerAttackRequest Request;
	FGridAttackResult Result;
	EGridPlayerAttackRejectReason RejectReason = EGridPlayerAttackRejectReason::None;
	TestTrue(TEXT("The equipped sword attack resolves"), Fixture.TurnManager->RequestCharacterAttack(0, Request, Result, RejectReason));
	TestEqual(TEXT("The sword AttackId is retained"), Request.AttackId, FName(TEXT("Attack_Sword")));
	TestEqual(TEXT("The real sword item id is retained"), Request.OffensiveItemDefinitionId, SwordDefinition->ItemDefinitionId);
	TestEqual(TEXT("The real MainHand slot is retained"), Request.OffensiveEquipmentSlot, EGridEquipmentSlot::MainHand);
	TestEqual(TEXT("The result retains Slashing"), Result.PhysicalSubtype, EGridPhysicalDamageSubtype::Slashing);
	TestEqual(TEXT("Exactly one result delegate is emitted"), Fixture.TurnManager->PlayerAttackResolvedBroadcastCount, 1);
	TestEqual(TEXT("Exactly one attack log is emitted"), CountPlayerAttackEntries(Fixture.TurnManager), 1);
	TestEqual(TEXT("Physical armor is applied exactly once"), Fixture.Monster->CurrentPhysicalArmor, ArmorBefore - Result.PhysicalArmorDamage);
	TestEqual(TEXT("Health is applied exactly once"), Fixture.Monster->CurrentHealth, HealthBefore - Result.HealthDamage);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON11HandPriorityAndFallbackTest, "Grimrock.Monsters.MON11.HandPriorityAndFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON11HandPriorityAndFallbackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON11EquipmentFixture Fixture;
	if (!TestTrue(TEXT("The hand-priority fixture is ready"), Fixture.IsReady() && Fixture.Monster))
	{
		return false;
	}

	const FGridOffensiveEquipmentProfile MainProfile = MakeOffensiveProfile(
		TEXT("Attack_Main"), EGridDamageType::Physical, EGridPhysicalDamageSubtype::Slashing, 1, 4, EGridAttackScalingAttribute::Strength, 1);
	const FGridOffensiveEquipmentProfile OffProfile = MakeOffensiveProfile(
		TEXT("Attack_Off"), EGridDamageType::Physical, EGridPhysicalDamageSubtype::Piercing, 1, 6, EGridAttackScalingAttribute::Dexterity, 1);
	UGridItemDefinitionAsset* TorchDefinition = MakeEquipmentDefinition(
		Fixture.Runtime, TEXT("Item_Torch"), EGridItemType::Torch, FGridOffensiveEquipmentProfile(), EGridEquipmentSlot::MainHand, false);
	UGridItemDefinitionAsset* MainDefinition =
		MakeEquipmentDefinition(Fixture.Runtime, TEXT("Item_MainWeapon"), EGridItemType::Weapon, MainProfile, EGridEquipmentSlot::MainHand);
	UGridItemDefinitionAsset* OffDefinition =
		MakeEquipmentDefinition(Fixture.Runtime, TEXT("Item_OffWeapon"), EGridItemType::Weapon, OffProfile, EGridEquipmentSlot::OffHand);
	Fixture.Equip(TorchDefinition, EGridEquipmentSlot::MainHand);
	Fixture.Equip(OffDefinition, EGridEquipmentSlot::OffHand);

	FGridPlayerAttackRequest Request;
	FGridAttackResult Result;
	EGridPlayerAttackRejectReason RejectReason = EGridPlayerAttackRejectReason::None;
	TestTrue(TEXT("A non-offensive main hand allows the off hand"), Fixture.TurnManager->RequestCharacterAttack(0, Request, Result, RejectReason));
	TestEqual(TEXT("OffHand is selected behind a non-offensive MainHand"), Request.OffensiveEquipmentSlot, EGridEquipmentSlot::OffHand);
	TestEqual(TEXT("The off-hand attack is not combined"), Request.AttackId, OffProfile.AttackId);

	Fixture.TurnManager->BeginPlayerCharacterPhase();
	Fixture.Equip(MainDefinition, EGridEquipmentSlot::MainHand);
	TestTrue(TEXT("Two offensive hands still resolve once"), Fixture.TurnManager->RequestCharacterAttack(0, Request, Result, RejectReason));
	TestEqual(TEXT("MainHand wins over an offensive OffHand"), Request.OffensiveEquipmentSlot, EGridEquipmentSlot::MainHand);
	TestEqual(TEXT("Only the main-hand attack is selected"), Request.AttackId, MainProfile.AttackId);

	Fixture.TurnManager->BeginPlayerCharacterPhase();
	Fixture.ClearEquipment();
	Fixture.Equip(TorchDefinition, EGridEquipmentSlot::MainHand);
	UGridItemDefinitionAsset* ShieldDefinition = MakeEquipmentDefinition(
		Fixture.Runtime, TEXT("Item_Shield"), EGridItemType::Shield, FGridOffensiveEquipmentProfile(), EGridEquipmentSlot::OffHand, false);
	Fixture.Equip(ShieldDefinition, EGridEquipmentSlot::OffHand);
	TestTrue(TEXT("Two non-offensive hands fall back"), Fixture.TurnManager->RequestCharacterAttack(0, Request, Result, RejectReason));
	TestEqual(TEXT("The fallback is Attack_Unarmed"), Request.AttackId, FName(TEXT("Attack_Unarmed")));
	TestTrue(TEXT("The fallback has no item identity"), Request.OffensiveItemDefinitionId.IsNone());

	Fixture.TurnManager->BeginPlayerCharacterPhase();
	Fixture.ClearEquipment();
	UGridItemDefinitionAsset* HeadOffense =
		MakeEquipmentDefinition(Fixture.Runtime, TEXT("Item_HeadOffense"), EGridItemType::Misc, MainProfile, EGridEquipmentSlot::Head);
	Fixture.Equip(HeadOffense, EGridEquipmentSlot::Head);
	TestTrue(TEXT("An offensive non-hand slot does not become active"), Fixture.TurnManager->RequestCharacterAttack(0, Request, Result, RejectReason));
	TestEqual(TEXT("The non-hand profile is ignored"), Request.AttackId, FName(TEXT("Attack_Unarmed")));
	TestEqual(TEXT("Four commands produce four resolutions only"), Fixture.TurnManager->PlayerAttackResolvedBroadcastCount, 4);
	TestEqual(TEXT("Four commands produce four attack logs only"), CountPlayerAttackEntries(Fixture.TurnManager), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON11RangedWeaponTargetingTest, "Grimrock.Monsters.MON11.RangedWeaponTargeting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON11RangedWeaponTargetingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGridOffensiveEquipmentProfile RangedProfile = MakeOffensiveProfile(
		TEXT("Attack_RangeThree"), EGridDamageType::Physical, EGridPhysicalDamageSubtype::Piercing, 1, 4, EGridAttackScalingAttribute::Dexterity, 3);

	{
		FGridMON11EquipmentFixture Fixture(FIntPoint(2, 4));
		UGridItemDefinitionAsset* Weapon =
			MakeEquipmentDefinition(Fixture.Runtime, TEXT("Item_RangeThree"), EGridItemType::Weapon, RangedProfile, EGridEquipmentSlot::MainHand);
		Fixture.Equip(Weapon, EGridEquipmentSlot::MainHand);
		FGridPlayerAttackRequest Request;
		FGridAttackResult Result;
		EGridPlayerAttackRejectReason RejectReason = EGridPlayerAttackRejectReason::None;
		TestTrue(TEXT("A target at three cells is reached"), Fixture.TurnManager->RequestCharacterAttack(0, Request, Result, RejectReason));
		TestEqual(TEXT("The exact three-cell target is retained"), Request.TargetCell, FIntPoint(2, 4));
		TestEqual(TEXT("The request retains range three"), Request.RangeCells, 3);
	}

	{
		FGridMON11EquipmentFixture Fixture(FIntPoint(2, 3));
		AGridMonsterActor* RearMonster = Fixture.SpawnMonster(FIntPoint(2, 4));
		Fixture.SetOnlyCombatMonsters({ Fixture.Monster, RearMonster });
		UGridItemDefinitionAsset* Weapon =
			MakeEquipmentDefinition(Fixture.Runtime, TEXT("Item_FirstTarget"), EGridItemType::Weapon, RangedProfile, EGridEquipmentSlot::MainHand);
		Fixture.Equip(Weapon, EGridEquipmentSlot::MainHand);
		FGridPlayerAttackRequest Request;
		FGridAttackResult Result;
		EGridPlayerAttackRejectReason RejectReason = EGridPlayerAttackRejectReason::None;
		TestTrue(TEXT("The first monster in line is selected"), Fixture.TurnManager->RequestCharacterAttack(0, Request, Result, RejectReason));
		TestEqual(TEXT("No attack passes through the first monster"), Request.TargetMonsterId, Fixture.Monster->ResolvePersistenceId());
		TestEqual(TEXT("The first monster cell is retained"), Request.TargetCell, FIntPoint(2, 3));
	}

	{
		FGridMON11EquipmentFixture Fixture(FIntPoint(2, 4));
		UGridItemDefinitionAsset* Weapon =
			MakeEquipmentDefinition(Fixture.Runtime, TEXT("Item_BlockedRange"), EGridItemType::Weapon, RangedProfile, EGridEquipmentSlot::MainHand);
		Fixture.Equip(Weapon, EGridEquipmentSlot::MainHand);
		Fixture.LevelAsset->GetCellMutable(2, 2).NorthWall = EGridWallType::Solid;
		constexpr int32 BlockedSeed = 777;
		Fixture.TurnManager->CombatRandomStream.Initialize(BlockedSeed);
		FGridPlayerAttackRequest Request;
		FGridAttackResult Result;
		EGridPlayerAttackRejectReason RejectReason = EGridPlayerAttackRejectReason::None;
		TestFalse(TEXT("A wall blocks the ranged attack"), Fixture.TurnManager->RequestCharacterAttack(0, Request, Result, RejectReason));
		TestEqual(TEXT("The wall reports PassageBlocked"), RejectReason, EGridPlayerAttackRejectReason::PassageBlocked);
		TestEqual(TEXT("The blocked request consumes no random draw"), Fixture.TurnManager->CombatRandomStream.GetCurrentSeed(), BlockedSeed);
	}

	{
		FGridMON11EquipmentFixture Fixture(FIntPoint(2, 5));
		UGridItemDefinitionAsset* Weapon =
			MakeEquipmentDefinition(Fixture.Runtime, TEXT("Item_OutOfRange"), EGridItemType::Weapon, RangedProfile, EGridEquipmentSlot::MainHand);
		Fixture.Equip(Weapon, EGridEquipmentSlot::MainHand);
		FGridPlayerAttackRequest Request;
		FGridAttackResult Result;
		EGridPlayerAttackRejectReason RejectReason = EGridPlayerAttackRejectReason::None;
		TestFalse(TEXT("A target at four cells is rejected"), Fixture.TurnManager->RequestCharacterAttack(0, Request, Result, RejectReason));
		TestEqual(TEXT("The fourth cell reports TargetOutOfRange"), RejectReason, EGridPlayerAttackRejectReason::TargetOutOfRange);
	}

	{
		FGridMON11EquipmentFixture Fixture(FIntPoint::ZeroValue, false);
		UGridItemDefinitionAsset* Weapon =
			MakeEquipmentDefinition(Fixture.Runtime, TEXT("Item_NoTarget"), EGridItemType::Weapon, RangedProfile, EGridEquipmentSlot::MainHand);
		Fixture.Equip(Weapon, EGridEquipmentSlot::MainHand);
		FGridPlayerAttackRequest Request;
		FGridAttackResult Result;
		EGridPlayerAttackRejectReason RejectReason = EGridPlayerAttackRejectReason::None;
		TestFalse(TEXT("An empty line has no target"), Fixture.TurnManager->RequestCharacterAttack(0, Request, Result, RejectReason));
		TestEqual(TEXT("An empty line reports NoMonsterInFront"), RejectReason, EGridPlayerAttackRejectReason::NoMonsterInFront);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON11ElementalOffensiveEquipmentTest, "Grimrock.Monsters.MON11.ElementalOffensiveEquipment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON11ElementalOffensiveEquipmentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON11EquipmentFixture Fixture(FIntPoint(2, 3));
	if (!TestTrue(TEXT("The elemental fixture is ready"), Fixture.IsReady() && Fixture.Monster))
	{
		return false;
	}

	const FGridOffensiveEquipmentProfile FireProfile = MakeOffensiveProfile(
		TEXT("Attack_FireFocus"), EGridDamageType::Fire, EGridPhysicalDamageSubtype::None, 1, 6, EGridAttackScalingAttribute::Intelligence, 3, 2);
	UGridItemDefinitionAsset* FocusDefinition =
		MakeEquipmentDefinition(Fixture.Runtime, TEXT("Item_FireFocus"), EGridItemType::Misc, FireProfile, EGridEquipmentSlot::MainHand);
	FocusDefinition->EquipmentStatBonus.IntelligenceBonus = 2;
	Fixture.Equip(FocusDefinition, EGridEquipmentSlot::MainHand);
	TestTrue(TEXT("A non-Weapon item can provide an attack"), FocusDefinition->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand));

	FGridMonsterDamageModifier FireModifier;
	FireModifier.DamageType = EGridDamageType::Fire;
	FireModifier.PhysicalSubtype = EGridPhysicalDamageSubtype::None;
	FireModifier.DamageMultiplier = 2.0f;
	Fixture.MonsterDefinition->DamageModifiers.Add(FireModifier);

	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("The final focus wielder summary is available"), Fixture.Party->PartyInventoryComponent->GetCharacterSummary(0, Summary));
	FGridAttackSourceStats Source;
	FGridAttackTargetStats Target;
	FGridAttackDefinition Attack;
	TestTrue(TEXT("The elemental inputs are built"),
		Fixture.TurnManager->BuildPlayerAttackResolutionInputs(Summary, Fixture.Monster, FireProfile, Source, Target, Attack));
	TestEqual(TEXT("Final Intelligence plus the flat bonus is used"), Source.DamageBonus,
		2 + URPGCharacterRulesLibrary::GetAttributeModifier(Summary.Attributes.Intelligence));
	TestEqual(TEXT("The Fire multiplier is selected"), Target.DamageMultiplier, 2.0f);

	const int32 NaturalTwentySeed = FindSeedForNaturalRoll(20);
	TestTrue(TEXT("A critical seed is available"), NaturalTwentySeed != INDEX_NONE);
	Fixture.TurnManager->CombatRandomStream.Initialize(NaturalTwentySeed);
	const int32 PhysicalArmorBefore = Fixture.Monster->CurrentPhysicalArmor;
	const int32 MagicalArmorBefore = Fixture.Monster->CurrentMagicalArmor;
	const int32 HealthBefore = Fixture.Monster->CurrentHealth;
	FGridPlayerAttackRequest Request;
	FGridAttackResult Result;
	EGridPlayerAttackRejectReason RejectReason = EGridPlayerAttackRejectReason::None;
	TestTrue(TEXT("The Fire focus attack resolves"), Fixture.TurnManager->RequestCharacterAttack(0, Request, Result, RejectReason));
	TestEqual(TEXT("The request retains the focus id"), Request.OffensiveItemDefinitionId, FocusDefinition->ItemDefinitionId);
	TestEqual(TEXT("The request retains MainHand"), Request.OffensiveEquipmentSlot, EGridEquipmentSlot::MainHand);
	TestEqual(TEXT("The request retains Attack_FireFocus"), Request.AttackId, FireProfile.AttackId);
	TestEqual(TEXT("Physical armor is unchanged by Fire"), Fixture.Monster->CurrentPhysicalArmor, PhysicalArmorBefore);
	TestEqual(TEXT("Magical armor absorbs Fire first"), Fixture.Monster->CurrentMagicalArmor, MagicalArmorBefore - Result.MagicalArmorDamage);
	TestEqual(TEXT("Only remaining Fire damage reaches health"), Fixture.Monster->CurrentHealth, HealthBefore - Result.HealthDamage);
	TestTrue(TEXT("The structured log retains the equipment identity"),
		!Fixture.TurnManager->CombatLogEntries.IsEmpty() &&
			Fixture.TurnManager->CombatLogEntries.Last().OffensiveItemDefinitionId == FocusDefinition->ItemDefinitionId);
	TestEqual(TEXT("The structured log retains the slot"), Fixture.TurnManager->CombatLogEntries.Last().OffensiveEquipmentSlot, EGridEquipmentSlot::MainHand);
	return true;
}

#endif

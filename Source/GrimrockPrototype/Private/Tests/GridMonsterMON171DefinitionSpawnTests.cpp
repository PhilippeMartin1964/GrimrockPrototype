#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

namespace
{
	UGridMonsterDefinitionAsset* MakeMON171GoblinDefinition(UObject* Outer)
	{
		UGridMonsterDefinitionAsset* Definition = NewObject<UGridMonsterDefinitionAsset>(Outer);
		Definition->MonsterId = TEXT("MON_GoblinThrower");
		Definition->DisplayName = FText::FromString(TEXT("Gobelin lanceur"));
		Definition->CategoryId = TEXT("Goblin");
		Definition->DangerLevel = 3;
		Definition->MaxHealth = 10;
		Definition->Initiative = 12;
		Definition->Accuracy = 2;
		Definition->ActionPointsPerTurn = 3;
		Definition->SightRangeCells = 8;
		Definition->HearingRangeCells = 4;
		Definition->PrimaryAIProfile = EGridMonsterAIProfile::RangedKeeper;
		Definition->PreferredMinDistance = 3;
		Definition->PreferredMaxDistance = 5;
		Definition->ExperienceReward = 125;

		FGridMonsterAttackDefinition ThrowKnife;
		ThrowKnife.AttackId = TEXT("Attack_ThrowKnife");
		ThrowKnife.DisplayName = FText::FromString(TEXT("Couteau lancé"));
		ThrowKnife.DamageType = EGridDamageType::Physical;
		ThrowKnife.MinDamage = 2;
		ThrowKnife.MaxDamage = 5;
		ThrowKnife.MinRangeCells = 2;
		ThrowKnife.RangeCells = 6;
		ThrowKnife.Delivery = EGridMonsterAttackDelivery::Projectile;
		ThrowKnife.bRequiresLineOfSight = true;
		ThrowKnife.ActionPointCost = 2;
		ThrowKnife.CooldownTurns = 0;
		ThrowKnife.Priority = 100;
		Definition->Attacks.Add(ThrowKnife);

		return Definition;
	}

	UGridLevelAsset* MakeMON171Level(UObject* Outer)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Outer);
		Level->Width = 4;
		Level->Height = 4;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		return Level;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON171RangedAttackContractTest, "Grimrock.Monsters.MON17.1.RangedAttackContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON171RangedAttackContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridMonsterAttackDefinition Attack;
	Attack.AttackId = TEXT("Attack_GenericProjectile");
	Attack.MinDamage = 1;
	Attack.MaxDamage = 3;
	Attack.MinRangeCells = 2;
	Attack.RangeCells = 6;
	Attack.Delivery = EGridMonsterAttackDelivery::Projectile;
	Attack.bRequiresLineOfSight = true;
	Attack.ActionPointCost = 2;
	Attack.CooldownTurns = 1;
	Attack.Priority = 25;

	FString Error;
	TestTrue(TEXT("Generic projectile attack validates"), Attack.ValidateDefinition(Error));
	TestTrue(TEXT("Projectile attack is classified as ranged"), Attack.IsRangedAttack());
	TestFalse(TEXT("Minimum range excludes adjacent target"), Attack.SupportsDistance(1));
	TestTrue(TEXT("Minimum range is inclusive"), Attack.SupportsDistance(2));
	TestTrue(TEXT("Maximum range is inclusive"), Attack.SupportsDistance(6));
	TestFalse(TEXT("Maximum range rejects farther target"), Attack.SupportsDistance(7));

	Attack.MinRangeCells = 7;
	TestFalse(TEXT("Minimum range greater than maximum is rejected"), Attack.ValidateDefinition(Error));
	TestTrue(TEXT("Invalid range reports the generic range contract"), Error.Contains(TEXT("RangeCells must be at least MinRangeCells")));

	Attack.MinRangeCells = 2;
	Attack.CooldownTurns = -1;
	TestFalse(TEXT("Negative cooldown is rejected"), Attack.ValidateDefinition(Error));
	TestTrue(TEXT("Invalid cooldown is reported"), Error.Contains(TEXT("CooldownTurns")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON171GoblinDefinitionTest, "Grimrock.Monsters.MON17.1.GoblinDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON171GoblinDefinitionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridMonsterDefinitionAsset* Definition = MakeMON171GoblinDefinition(GetTransientPackage());
	FString Error;
	TestTrue(TEXT("Goblin Thrower definition validates without monster-id special cases"), Definition->ValidateDefinition(Error));
	TestTrue(TEXT("Goblin Thrower uses the existing RangedKeeper profile"), Definition->HasAIProfile(EGridMonsterAIProfile::RangedKeeper));
	TestEqual(TEXT("Preferred minimum distance remains data-driven"), Definition->PreferredMinDistance, 3);
	TestEqual(TEXT("Preferred maximum distance remains data-driven"), Definition->PreferredMaxDistance, 5);
	TestEqual(TEXT("Definition defaults to the generic native monster actor"), Definition->MonsterActorClass.Get(), AGridMonsterActor::StaticClass());

	const FPrimaryAssetId PrimaryId = Definition->GetPrimaryAssetId();
	TestEqual(TEXT("Monster primary asset type is generic"), PrimaryId.PrimaryAssetType, FPrimaryAssetType(TEXT("GridMonster")));
	TestEqual(TEXT("Monster primary asset name uses MonsterId"), PrimaryId.PrimaryAssetName, FName(TEXT("MON_GoblinThrower")));

	TestEqual(TEXT("Goblin has one authored attack in MON17.1 fixture"), Definition->Attacks.Num(), 1);
	if (Definition->Attacks.Num() == 1)
	{
		const FGridMonsterAttackDefinition& Attack = Definition->Attacks[0];
		TestEqual(TEXT("Goblin attack delivery is projectile"), Attack.Delivery, EGridMonsterAttackDelivery::Projectile);
		TestTrue(TEXT("Goblin projectile requires line of sight"), Attack.bRequiresLineOfSight);
		TestFalse(TEXT("Goblin projectile cannot attack adjacent targets"), Attack.SupportsDistance(1));
		TestEqual(TEXT("Goblin fixture uses the ArtBook-aligned thrown-knife attack"), Attack.AttackId, FName(TEXT("Attack_ThrowKnife")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON171SpawnPersistenceContractTest, "Grimrock.Monsters.MON17.1.SpawnPersistenceContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON171SpawnPersistenceContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridLevelAsset* Level = MakeMON171Level(GetTransientPackage());
	UGridMonsterDefinitionAsset* Definition = MakeMON171GoblinDefinition(Level);

	UGridObjectArchetypeAsset* Archetype = NewObject<UGridObjectArchetypeAsset>(Level);
	Archetype->ArchetypeId = TEXT("Monster_GoblinThrower");
	Archetype->DisplayName = FText::FromString(TEXT("Gobelin lanceur"));
	Archetype->SupportedType = EGridLevelObjectType::MonsterSpawn;
	Archetype->PlacementKind = EGridObjectPlacementKind::Center;

	UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>(Level);
	FGridObjectPaletteEntry Entry;
	Entry.EntryId = TEXT("MON_GoblinThrower");
	Entry.DefaultArchetype = Archetype;
	Entry.DefaultMonsterDefinition = Definition;
	Palette->Entries.Add(Entry);

	TArray<FGridArchetypeValidationMessage> PaletteMessages;
	TestTrue(TEXT("Goblin Thrower enters the existing MonsterSpawn palette contract"), Palette->ValidatePalette(PaletteMessages));

	FGridLevelObjectData Spawn;
	Spawn.ObjectId = FGuid(17, 1, 1, 1);
	Spawn.Type = EGridLevelObjectType::MonsterSpawn;
	Spawn.CellX = 2;
	Spawn.CellY = 1;
	Spawn.Edge = EGridEdge::None;
	Spawn.InitialFacing = EGridEdge::West;
	Spawn.InitialMonsterState = EGridMonsterState::Dormant;
	Spawn.MonsterDefinitionAsset = Definition;
	Spawn.MonsterDefinitionId = Definition->MonsterId;
	Spawn.bInitiallyEnabled = true;

	const FGuid SpawnId = Level->AddObject(Spawn);
	const FGridLevelObjectData* StoredSpawn = Level->FindMonsterSpawnById(SpawnId);
	TestNotNull(TEXT("Goblin Thrower placement is stored as a generic MonsterSpawn"), StoredSpawn);

	TArray<FString> SpawnErrors;
	TestTrue(TEXT("Goblin Thrower MonsterSpawn validates"), Level->ValidateMonsterSpawns(SpawnErrors));

	FGridRuntimeMonsterState RuntimeState;
	RuntimeState.PersistenceId = FGuid(17, 1, 2, 1);
	RuntimeState.SpawnObjectId = SpawnId;
	RuntimeState.MonsterDefinitionId = Definition->MonsterId;
	RuntimeState.CellX = 2;
	RuntimeState.CellY = 1;
	RuntimeState.Facing = EGridEdge::West;
	RuntimeState.MonsterState = EGridMonsterState::Dormant;
	RuntimeState.CurrentHealth = Definition->MaxHealth;
	RuntimeState.bMonsterEnabled = true;

	FGridRuntimeMonsterPlacementState PlacementState;
	PlacementState.SpawnId = SpawnId;
	PlacementState.bIsSpawned = true;
	PlacementState.bHasMonsterState = true;
	PlacementState.MonsterState = RuntimeState;

	TestEqual(TEXT("Persistent runtime state keeps the second monster definition id"), PlacementState.MonsterState.MonsterDefinitionId,
		FName(TEXT("MON_GoblinThrower")));
	TestEqual(TEXT("Persistent runtime state keeps the MonsterSpawn id"), PlacementState.MonsterState.SpawnObjectId, SpawnId);
	TestEqual(TEXT("Dormant state remains representable for the second family"), PlacementState.MonsterState.MonsterState, EGridMonsterState::Dormant);
	return true;
}

#endif

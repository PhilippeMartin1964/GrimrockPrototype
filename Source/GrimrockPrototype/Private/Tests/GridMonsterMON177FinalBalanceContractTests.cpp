#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

namespace MON177FinalBalance
{
	constexpr int32 GoblinExperienceReward = 125;
	constexpr int32 RatExperienceReward = 500;
	constexpr float GoblinKnifeDropChance = 0.25f;
	constexpr float StoneDropChance = 0.50f;
	constexpr float EmptyVialDropChance = 0.25f;
	constexpr float ExpectedItemsPerKill = 1.00f;

	UGridMonsterDefinitionAsset* LoadMonsterDefinition(FAutomationTestBase& Test, const TCHAR* AssetPath, const TCHAR* Label)
	{
		UGridMonsterDefinitionAsset* Definition = LoadObject<UGridMonsterDefinitionAsset>(nullptr, AssetPath);
		Test.TestNotNull(Label, Definition);
		return Definition;
	}

	const FGridMonsterLootEntry* FindLootEntry(const UGridMonsterDefinitionAsset* Definition, FName ItemDefinitionId)
	{
		return Definition ? Definition->LootTable.FindByPredicate(
								[ItemDefinitionId](const FGridMonsterLootEntry& Entry)
								{
									return Entry.GetResolvedItemDefinitionId() == ItemDefinitionId;
								})
						  : nullptr;
	}

	void ValidateLootEntry(FAutomationTestBase& Test, const TCHAR* Label, const FGridMonsterLootEntry* Entry, float ExpectedDropChance)
	{
		Test.TestNotNull(Label, Entry);
		if (!Entry)
		{
			return;
		}

		Test.TestTrue(*FString::Printf(TEXT("%s drop chance"), Label), FMath::IsNearlyEqual(Entry->DropChance, ExpectedDropChance));
		Test.TestEqual(*FString::Printf(TEXT("%s minimum quantity"), Label), Entry->MinQuantity, 1);
		Test.TestEqual(*FString::Printf(TEXT("%s maximum quantity"), Label), Entry->MaxQuantity, 1);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON177FinalBalanceContractTest, "Grimrock.Monsters.MON17.7.FinalBalanceContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON177FinalBalanceContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace MON177FinalBalance;

	UGridMonsterDefinitionAsset* Goblin =
		LoadMonsterDefinition(*this, TEXT("/Game/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower.DA_MON_GoblinThrower"),
			TEXT("Production Goblin Thrower definition loads"));
	UGridMonsterDefinitionAsset* Rat = LoadMonsterDefinition(
		*this, TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.DA_MON_RatGiant"), TEXT("Production Rat Giant definition loads"));
	if (!Goblin || !Rat)
	{
		return false;
	}

	TestEqual(TEXT("Goblin stable MonsterId"), Goblin->MonsterId, FName(TEXT("MON_GoblinThrower")));
	TestEqual(TEXT("Rat Giant stable MonsterId"), Rat->MonsterId, FName(TEXT("MON_RatGiant")));

	TestEqual(TEXT("Goblin danger level remains three"), Goblin->DangerLevel, 3);
	TestEqual(TEXT("Goblin health remains ten"), Goblin->MaxHealth, 10);
	TestEqual(TEXT("Goblin initiative remains twelve"), Goblin->Initiative, 12);
	TestEqual(TEXT("Goblin accuracy remains two"), Goblin->Accuracy, 2);
	TestEqual(TEXT("Goblin evasion remains three"), Goblin->Evasion, 3);
	TestEqual(TEXT("Goblin AP remains three"), Goblin->ActionPointsPerTurn, 3);
	TestEqual(TEXT("Goblin remains RangedKeeper"), Goblin->PrimaryAIProfile, EGridMonsterAIProfile::RangedKeeper);
	TestEqual(TEXT("Goblin preferred minimum distance remains three"), Goblin->PreferredMinDistance, 3);
	TestEqual(TEXT("Goblin preferred maximum distance remains five"), Goblin->PreferredMaxDistance, 5);

	FGridMonsterAttackDefinition ThrowKnife;
	TestTrue(TEXT("ThrowKnife resolves"), Goblin->GetAttackDefinition(FName(TEXT("Attack_ThrowKnife")), ThrowKnife));
	TestEqual(TEXT("ThrowKnife minimum damage remains two"), ThrowKnife.MinDamage + ThrowKnife.DamageBonus, 2);
	TestEqual(TEXT("ThrowKnife maximum damage remains five"), ThrowKnife.MaxDamage + ThrowKnife.DamageBonus, 5);
	TestEqual(TEXT("ThrowKnife minimum range remains two"), ThrowKnife.MinRangeCells, 2);
	TestEqual(TEXT("ThrowKnife maximum range remains six"), ThrowKnife.MaxRangeCells, 6);
	TestEqual(TEXT("ThrowKnife AP cost remains two"), ThrowKnife.ActionPointCost, 2);

	TestEqual(TEXT("Goblin reward remains 125 XP"), Goblin->ExperienceReward, GoblinExperienceReward);
	TestEqual(TEXT("Rat Giant reward remains 500 XP"), Rat->ExperienceReward, RatExperienceReward);
	TestEqual(TEXT("Four Goblins equal one Rat Giant reward"), Goblin->ExperienceReward * 4, Rat->ExperienceReward);

	TestEqual(TEXT("Goblin final loot table has exactly three entries"), Goblin->LootTable.Num(), 3);

	const FGridMonsterLootEntry* KnifeEntry = FindLootEntry(Goblin, FName(TEXT("GoblinKnife")));
	const FGridMonsterLootEntry* StoneEntry = FindLootEntry(Goblin, FName(TEXT("Stone")));
	const FGridMonsterLootEntry* VialEntry = FindLootEntry(Goblin, FName(TEXT("EmptyVial")));

	ValidateLootEntry(*this, TEXT("GoblinKnife"), KnifeEntry, GoblinKnifeDropChance);
	ValidateLootEntry(*this, TEXT("Stone"), StoneEntry, StoneDropChance);
	ValidateLootEntry(*this, TEXT("EmptyVial"), VialEntry, EmptyVialDropChance);

	float ActualExpectedItemsPerKill = 0.0f;
	for (const FGridMonsterLootEntry& Entry : Goblin->LootTable)
	{
		ActualExpectedItemsPerKill += Entry.DropChance * (static_cast<float>(Entry.MinQuantity + Entry.MaxQuantity) * 0.5f);
	}

	TestTrue(TEXT("Final expected loot is exactly one item per Goblin"), FMath::IsNearlyEqual(ActualExpectedItemsPerKill, ExpectedItemsPerKill));

	AddInfo(FString::Printf(TEXT("MON17.7 final balance: HP=%d Initiative=%d Accuracy=%d Evasion=%d AP=%d Damage=%d..%d XP=%d LootExpected=%.3f"),
		Goblin->MaxHealth, Goblin->Initiative, Goblin->Accuracy, Goblin->Evasion, Goblin->ActionPointsPerTurn, ThrowKnife.MinDamage + ThrowKnife.DamageBonus,
		ThrowKnife.MaxDamage + ThrowKnife.DamageBonus, Goblin->ExperienceReward, ActualExpectedItemsPerKill));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	UGridMonsterDefinitionAsset* CreateGiantRatDefinition()
	{
		UGridMonsterDefinitionAsset* Rat = NewObject<UGridMonsterDefinitionAsset>();
		Rat->MonsterId = TEXT("MON_RatGiant");
		Rat->DisplayName = FText::FromString(TEXT("Giant Rat"));
		Rat->CategoryId = TEXT("Vermin");
		Rat->DangerLevel = 1;
		Rat->MaxHealth = 8;
		Rat->Initiative = 12;
		Rat->Accuracy = 2;
		Rat->Evasion = 1;
		Rat->ActionPointsPerTurn = 2;
		Rat->SightRangeCells = 5;
		Rat->HearingRangeCells = 3;
		Rat->AggroPropagationRange = 3;
		Rat->bSharesAggroWithGroup = true;
		Rat->PrimaryAIProfile = EGridMonsterAIProfile::DirectMelee;
		Rat->AdditionalAIProfiles.Add(EGridMonsterAIProfile::FastHarasser);
		Rat->PreferredMinDistance = 1;
		Rat->PreferredMaxDistance = 1;
		Rat->RetreatChance = 0.40f;
		Rat->ExperienceReward = 10;

		FGridMonsterAttackDefinition Bite;
		Bite.AttackId = TEXT("Attack_Bite");
		Bite.DisplayName = FText::FromString(TEXT("Bite"));
		Bite.DamageType = EGridDamageType::Physical;
		Bite.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
		Bite.MinDamage = 2;
		Bite.MaxDamage = 5;
		Bite.MaxRangeCells = 1;
		Bite.ActionPointCost = 1;
		Rat->Attacks.Add(Bite);

		FGridMonsterDamageModifier FireWeakness;
		FireWeakness.DamageType = EGridDamageType::Fire;
		FireWeakness.DamageMultiplier = 1.50f;
		Rat->DamageModifiers.Add(FireWeakness);

		FGridMonsterDamageModifier SlashingWeakness;
		SlashingWeakness.DamageType = EGridDamageType::Physical;
		SlashingWeakness.PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
		SlashingWeakness.DamageMultiplier = 1.25f;
		Rat->DamageModifiers.Add(SlashingWeakness);

		FGridMonsterLootEntry MeatLoot;
		MeatLoot.ItemDefinitionId = TEXT("Item_RatMeat");
		MeatLoot.DropChance = 0.30f;
		Rat->LootTable.Add(MeatLoot);

		FGridMonsterLootEntry ToothLoot;
		ToothLoot.ItemDefinitionId = TEXT("Item_RatTooth");
		ToothLoot.DropChance = 0.15f;
		Rat->LootTable.Add(ToothLoot);

		return Rat;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterDefinitionMON1ValidationTest, "Grimrock.Monsters.MON1.DefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterDefinitionMON1ValidationTest::RunTest(const FString& Parameters)
{
	const UGridMonsterDefinitionAsset* Rat = CreateGiantRatDefinition();

	FString Error;
	TestTrue(TEXT("The Giant Rat definition validates"), Rat->ValidateDefinition(Error));
	TestTrue(TEXT("Validation does not return an error"), Error.IsEmpty());
	TestTrue(TEXT("The Giant Rat definition is valid"), Rat->IsValidDefinition());
	TestTrue(TEXT("The primary asset id is valid"), Rat->GetPrimaryAssetId().IsValid());
	TestTrue(TEXT("The DirectMelee profile is present"), Rat->HasAIProfile(EGridMonsterAIProfile::DirectMelee));
	TestTrue(TEXT("The FastHarasser profile is present"), Rat->HasAIProfile(EGridMonsterAIProfile::FastHarasser));
	TestFalse(TEXT("The RangedKeeper profile is absent"), Rat->HasAIProfile(EGridMonsterAIProfile::RangedKeeper));

	FGridMonsterAttackDefinition Bite;
	TestTrue(TEXT("The bite attack can be resolved"), Rat->GetAttackDefinition(TEXT("Attack_Bite"), Bite));
	TestTrue(TEXT("The bite uses piercing damage"), Bite.PhysicalSubtype == EGridPhysicalDamageSubtype::Piercing);
	TestEqual(TEXT("The bite costs one action point"), Bite.ActionPointCost, 1);

	TestTrue(TEXT("Fire vulnerability is 1.50"), FMath::IsNearlyEqual(Rat->GetDamageMultiplier(EGridDamageType::Fire), 1.50f));
	TestTrue(TEXT("Slashing vulnerability is 1.25"),
		FMath::IsNearlyEqual(Rat->GetDamageMultiplier(EGridDamageType::Physical, EGridPhysicalDamageSubtype::Slashing), 1.25f));
	TestTrue(TEXT("Piercing damage has no modifier"),
		FMath::IsNearlyEqual(Rat->GetDamageMultiplier(EGridDamageType::Physical, EGridPhysicalDamageSubtype::Piercing), 1.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterDefinitionMON1InvalidDataTest, "Grimrock.Monsters.MON1.InvalidData", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterDefinitionMON1InvalidDataTest::RunTest(const FString& Parameters)
{
	UGridMonsterDefinitionAsset* Rat = CreateGiantRatDefinition();
	const FGridMonsterAttackDefinition DuplicateAttack = Rat->Attacks[0];
	Rat->Attacks.Add(DuplicateAttack);

	FString Error;
	TestFalse(TEXT("Duplicate attack ids invalidate the definition"), Rat->ValidateDefinition(Error));
	TestTrue(TEXT("The duplicate attack id is reported"), Error.Contains(TEXT("Duplicate AttackId")));

	Rat = CreateGiantRatDefinition();
	Rat->Attacks[0].ActionPointCost = 0;
	Error.Reset();
	TestFalse(TEXT("A free attack invalidates the definition"), Rat->ValidateDefinition(Error));
	TestTrue(TEXT("The invalid attack index is reported"), Error.Contains(TEXT("Attacks[0]")));
	TestTrue(TEXT("The invalid attack field is reported"), Error.Contains(TEXT("ActionPointCost must be greater than zero.")));

	Rat = CreateGiantRatDefinition();
	Rat->LootTable[0].DropChance = 0.90f;
	Rat->LootTable[1].DropChance = 0.80f;

	Error.Reset();
	TestTrue(TEXT("Independent loot chances may total above 100 percent"), Rat->ValidateDefinition(Error));
	TestTrue(TEXT("An independent total above 100 percent reports no error"), Error.IsEmpty());

	Rat->LootTable[0].DropChance = 1.01f;
	Error.Reset();
	TestFalse(TEXT("Each individual loot chance remains capped at 100 percent"), Rat->ValidateDefinition(Error));
	TestTrue(TEXT("The invalid individual loot entry is reported"), Error.Contains(TEXT("Loot entry at index 0")));

	UGridMonsterDefinitionAsset* EmptyDefinition = NewObject<UGridMonsterDefinitionAsset>();
	Error.Reset();
	TestFalse(TEXT("An empty monster definition is invalid"), EmptyDefinition->ValidateDefinition(Error));
	TestTrue(TEXT("The missing MonsterId is reported"), Error.Contains(TEXT("MonsterId")));

	return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterTypes.h"
#include "UObject/UnrealType.h"

namespace GridTD0735Characterization
{
	bool LoadProjectFile(const TCHAR* RelativePath, FString& OutText)
	{
		return FFileHelper::LoadFileToString(OutText, *FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}

	FGridOffensiveEquipmentProfile MakeLegacyProfile()
	{
		FGridOffensiveEquipmentProfile Profile;
		Profile.AttackId = TEXT("TD0735_LegacySword");
		Profile.AttackDefinition.DamageType = EGridDamageType::Physical;
		Profile.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
		Profile.AttackDefinition.MinDamage = 2;
		Profile.AttackDefinition.MaxDamage = 5;
		Profile.DamageScalingAttribute = EGridAttackScalingAttribute::Strength;
		Profile.RangeCells = 1;
		return Profile;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0735ItemSchemaMultiplicityTest,
	"Grimrock.TechnicalDebt.TD07_3_5.Characterization.ItemSchemaMultiplicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0735ItemSchemaMultiplicityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* ItemClass = UGridItemDefinitionAsset::StaticClass();
	TestNotNull(TEXT("Item definition class exists"), ItemClass);
	if (!ItemClass)
	{
		return false;
	}

	const FProperty* LegacyFlag = ItemClass->FindPropertyByName(TEXT("bProvidesAttack"));
	const FProperty* LegacyProfile = ItemClass->FindPropertyByName(TEXT("OffensiveProfile"));
	const FProperty* CurrentActions = ItemClass->FindPropertyByName(TEXT("CombatActions"));

	TestNotNull(TEXT("Legacy bProvidesAttack still exists"), LegacyFlag);
	TestNotNull(TEXT("Legacy OffensiveProfile still exists"), LegacyProfile);
	TestNotNull(TEXT("Current CombatActions exists"), CurrentActions);
	TestTrue(TEXT("Legacy flag remains an authoring property"), LegacyFlag && !LegacyFlag->HasAnyPropertyFlags(CPF_Transient));
	TestTrue(TEXT("Legacy profile remains an authoring property"), LegacyProfile && !LegacyProfile->HasAnyPropertyFlags(CPF_Transient));
	TestTrue(TEXT("CombatActions remains an authoring property"), CurrentActions && !CurrentActions->HasAnyPropertyFlags(CPF_Transient));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0735LegacyItemAdapterActiveTest,
	"Grimrock.TechnicalDebt.TD07_3_5.Characterization.LegacyItemAdapterActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0735LegacyItemAdapterActiveTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0735Characterization;

	UGridItemDefinitionAsset* Item = NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
	Item->ItemDefinitionId = TEXT("TD0735_LegacyWeapon");
	Item->ItemType = EGridItemType::Weapon;
	Item->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::MainHand);
	Item->bProvidesAttack = true;
	Item->OffensiveProfile = MakeLegacyProfile();

	TestTrue(TEXT("Legacy-only weapon definition is still valid"), Item->IsValidDefinition());
	TestTrue(TEXT("Legacy-only weapon can still provide a main-hand attack"), Item->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("Legacy-only weapon has no current CombatActions"), Item->CombatActions.IsEmpty());

	const FGridCombatActionDefinition Adapted = FGridCombatActionCatalog::MakeLegacyEquipmentAttackDefinition(*Item, 2);
	TestTrue(TEXT("Legacy adapter creates a valid CombatAction"), Adapted.IsValid());
	TestEqual(TEXT("Adapted action keeps legacy AttackId"), Adapted.ActionId, Item->OffensiveProfile.AttackId);
	TestEqual(TEXT("Adapted action keeps legacy range"), Adapted.RangeCells, Item->OffensiveProfile.RangeCells);
	TestTrue(TEXT("Adapted action copies the legacy offensive profile"),
		Adapted.OffensiveProfile.AttackId == Item->OffensiveProfile.AttackId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0735MonsterSchemaMultiplicityTest,
	"Grimrock.TechnicalDebt.TD07_3_5.Characterization.MonsterSchemaMultiplicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0735MonsterSchemaMultiplicityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UScriptStruct* AttackStruct = FGridMonsterAttackDefinition::StaticStruct();
	TestNotNull(TEXT("Monster attack struct exists"), AttackStruct);
	if (!AttackStruct)
	{
		return false;
	}

	TestNotNull(TEXT("Legacy AttackSound still exists"), AttackStruct->FindPropertyByName(TEXT("AttackSound")));
	TestNotNull(TEXT("Current AttackAudio exists"), AttackStruct->FindPropertyByName(TEXT("AttackAudio")));
	TestNotNull(TEXT("Legacy ImpactVFX still exists"), AttackStruct->FindPropertyByName(TEXT("ImpactVFX")));
	TestNotNull(TEXT("Current ImpactHitVFXDefinition exists"), AttackStruct->FindPropertyByName(TEXT("ImpactHitVFXDefinition")));
	TestNotNull(TEXT("Serialized legacy range name RangeCells still exists"), AttackStruct->FindPropertyByName(TEXT("RangeCells")));
	TestNull(TEXT("Target current field MaxRangeCells does not exist yet"), AttackStruct->FindPropertyByName(TEXT("MaxRangeCells")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0735MonsterRuntimeFallbacksTest,
	"Grimrock.TechnicalDebt.TD07_3_5.Characterization.MonsterRuntimeFallbacks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0735MonsterRuntimeFallbacksTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0735Characterization;

	FString AudioSource;
	FString VFXSource;
	FString TypesSource;
	TestTrue(TEXT("Monster audio source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/Monsters/GridMonsterAudioComponent.cpp"), AudioSource));
	TestTrue(TEXT("Monster VFX source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/Monsters/GridMonsterVFXComponent.cpp"), VFXSource));
	TestTrue(TEXT("Monster types source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Runtime/Monsters/GridMonsterTypes.h"), TypesSource));

	TestTrue(TEXT("Attack audio still falls back to legacy AttackSound"),
		AudioSource.Contains(TEXT("Attack.AttackSound")) && AudioSource.Contains(TEXT("LegacyDefinition.Sounds.Add")));
	TestTrue(TEXT("Hit VFX still falls back to legacy ImpactVFX"),
		VFXSource.Contains(TEXT("Attack.ImpactVFX")) && VFXSource.Contains(TEXT("LegacyDefinition.Systems.Add")));
	TestTrue(TEXT("Monster range helpers still use serialized RangeCells"),
		TypesSource.Contains(TEXT("DistanceCells <= RangeCells")) && TypesSource.Contains(TEXT("RangeCells > 1")));
	TestTrue(TEXT("RangeCells comment explicitly exists for serialized compatibility"),
		TypesSource.Contains(TEXT("Kept as RangeCells for serialized asset compatibility")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

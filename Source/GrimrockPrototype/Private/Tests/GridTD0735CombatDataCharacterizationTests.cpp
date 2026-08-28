#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterTypes.h"
#include "UObject/UnrealType.h"

namespace GridTD0735Characterization
{
	bool LoadProjectFile(const TCHAR* RelativePath, FString& OutText)
	{
		return FFileHelper::LoadFileToString(OutText, *FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0735ItemSchemaAuthorityTest, "Grimrock.TechnicalDebt.TD07_3_5.Characterization.ItemSchemaAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0735ItemSchemaAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* ItemClass = UGridItemDefinitionAsset::StaticClass();
	TestNotNull(TEXT("Item definition class exists"), ItemClass);
	if (!ItemClass)
	{
		return false;
	}

	TestNull(TEXT("Legacy bProvidesAttack is removed"), ItemClass->FindPropertyByName(TEXT("bProvidesAttack")));
	TestNull(TEXT("Legacy item-level OffensiveProfile is removed"), ItemClass->FindPropertyByName(TEXT("OffensiveProfile")));

	const FProperty* CurrentActions = ItemClass->FindPropertyByName(TEXT("CombatActions"));
	TestNotNull(TEXT("Current CombatActions exists"), CurrentActions);
	TestTrue(TEXT("CombatActions is the durable authoring property"), CurrentActions && !CurrentActions->HasAnyPropertyFlags(CPF_Transient));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0735LegacyItemAdapterRemovedTest, "Grimrock.TechnicalDebt.TD07_3_5.Characterization.LegacyItemAdapterRemoved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0735LegacyItemAdapterRemovedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0735Characterization;

	FString CatalogHeader;
	FString CatalogSource;
	FString PlayerCatalogSource;
	FString HotbarSource;
	TestTrue(TEXT("CombatActionCatalog header loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Runtime/Combat/GridCombatActionCatalog.h"), CatalogHeader));
	TestTrue(TEXT("CombatActionCatalog source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridCombatActionCatalog.cpp"), CatalogSource));
	TestTrue(TEXT("Player action catalogue source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerPlayerActionCatalog.cpp"), PlayerCatalogSource));
	TestTrue(
		TEXT("Hotbar source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponentHotbar.cpp"), HotbarSource));

	TestFalse(TEXT("Legacy item adapter declaration is removed"), CatalogHeader.Contains(TEXT("MakeLegacyEquipmentAttackDefinition")));
	TestFalse(TEXT("Legacy item adapter implementation is removed"), CatalogSource.Contains(TEXT("MakeLegacyEquipmentAttackDefinition")));
	TestFalse(TEXT("Player catalogue no longer invokes the legacy adapter"), PlayerCatalogSource.Contains(TEXT("MakeLegacyEquipmentAttackDefinition")));
	TestFalse(TEXT("Hotbar no longer reads an item-level OffensiveProfile"), HotbarSource.Contains(TEXT("Definition->OffensiveProfile")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0735MonsterPresentationAuthorityTest, "Grimrock.TechnicalDebt.TD07_3_5.Characterization.MonsterPresentationAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0735MonsterPresentationAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UScriptStruct* AttackStruct = FGridMonsterAttackDefinition::StaticStruct();
	TestNotNull(TEXT("Monster attack struct exists"), AttackStruct);
	if (!AttackStruct)
	{
		return false;
	}

	TestNull(TEXT("Legacy AttackSound is removed"), AttackStruct->FindPropertyByName(TEXT("AttackSound")));
	TestNotNull(TEXT("Current AttackAudio exists"), AttackStruct->FindPropertyByName(TEXT("AttackAudio")));
	TestNotNull(TEXT("Current ImpactHitAudio exists"), AttackStruct->FindPropertyByName(TEXT("ImpactHitAudio")));
	TestNotNull(TEXT("Current ImpactMissAudio exists"), AttackStruct->FindPropertyByName(TEXT("ImpactMissAudio")));
	TestNull(TEXT("Legacy ImpactVFX is removed"), AttackStruct->FindPropertyByName(TEXT("ImpactVFX")));
	TestNotNull(TEXT("Current AttackVFXDefinition exists"), AttackStruct->FindPropertyByName(TEXT("AttackVFXDefinition")));
	TestNotNull(TEXT("Current ImpactHitVFXDefinition exists"), AttackStruct->FindPropertyByName(TEXT("ImpactHitVFXDefinition")));
	TestNotNull(TEXT("Current ImpactMissVFXDefinition exists"), AttackStruct->FindPropertyByName(TEXT("ImpactMissVFXDefinition")));
	TestNull(TEXT("Legacy RangeCells is removed"), AttackStruct->FindPropertyByName(TEXT("RangeCells")));
	TestNotNull(TEXT("Current MinRangeCells exists"), AttackStruct->FindPropertyByName(TEXT("MinRangeCells")));
	TestNotNull(TEXT("Current MaxRangeCells exists"), AttackStruct->FindPropertyByName(TEXT("MaxRangeCells")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0735MonsterPresentationRuntimeNormalizedTest,
	"Grimrock.TechnicalDebt.TD07_3_5.Characterization.MonsterPresentationRuntimeNormalized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0735MonsterPresentationRuntimeNormalizedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0735Characterization;

	FString AudioSource;
	FString VFXSource;
	FString TypesSource;
	TestTrue(TEXT("Monster audio source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/Monsters/GridMonsterAudioComponent.cpp"), AudioSource));
	TestTrue(
		TEXT("Monster VFX source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/Monsters/GridMonsterVFXComponent.cpp"), VFXSource));
	TestTrue(TEXT("Monster types source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Runtime/Monsters/GridMonsterTypes.h"), TypesSource));

	TestFalse(TEXT("Attack audio no longer falls back to legacy AttackSound"),
		AudioSource.Contains(TEXT("Attack.AttackSound")) || AudioSource.Contains(TEXT("LegacyDefinition.Sounds.Add")));
	TestFalse(TEXT("Hit VFX no longer falls back to legacy ImpactVFX"),
		VFXSource.Contains(TEXT("Attack.ImpactVFX")) || VFXSource.Contains(TEXT("LegacyDefinition.Systems.Add")));
	TestTrue(TEXT("Monster range helpers use the current MaxRangeCells authority"),
		TypesSource.Contains(TEXT("DistanceCells <= MaxRangeCells")) && TypesSource.Contains(TEXT("MaxRangeCells > 1")));
	TestFalse(TEXT("Legacy RangeCells declaration is absent"), TypesSource.Contains(TEXT("int32 RangeCells = 1")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

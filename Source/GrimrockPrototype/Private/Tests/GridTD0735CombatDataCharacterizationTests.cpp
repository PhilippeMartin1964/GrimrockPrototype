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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0735ItemSchemaAuthorityTest,
	"Grimrock.TechnicalDebt.TD07_3_5.Characterization.ItemSchemaAuthority",
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
	TestTrue(TEXT("CombatActions is the durable authoring property"),
		CurrentActions && !CurrentActions->HasAnyPropertyFlags(CPF_Transient));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0735LegacyItemAdapterRemovedTest,
	"Grimrock.TechnicalDebt.TD07_3_5.Characterization.LegacyItemAdapterRemoved",
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
	TestTrue(TEXT("Hotbar source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponentHotbar.cpp"), HotbarSource));

	TestFalse(TEXT("Legacy item adapter declaration is removed"),
		CatalogHeader.Contains(TEXT("MakeLegacyEquipmentAttackDefinition")));
	TestFalse(TEXT("Legacy item adapter implementation is removed"),
		CatalogSource.Contains(TEXT("MakeLegacyEquipmentAttackDefinition")));
	TestFalse(TEXT("Player catalogue no longer invokes the legacy adapter"),
		PlayerCatalogSource.Contains(TEXT("MakeLegacyEquipmentAttackDefinition")));
	TestFalse(TEXT("Hotbar no longer reads an item-level OffensiveProfile"),
		HotbarSource.Contains(TEXT("Definition->OffensiveProfile")));
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

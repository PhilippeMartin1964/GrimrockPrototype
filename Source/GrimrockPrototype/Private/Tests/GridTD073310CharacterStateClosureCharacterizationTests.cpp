#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/StatusEffects/GridStatusEffectTypes.h"
#include "RPGMON155TestHelpers.h"
#include "Runtime/GridInventoryTypes.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace GridTD073310Characterization
{
	const FProperty* FindCharacterProperty(const TCHAR* PropertyName)
	{
		return FindFProperty<FProperty>(FGridCharacterInventoryState::StaticStruct(), PropertyName);
	}

	bool IsDurableCharacterProperty(const TCHAR* PropertyName)
	{
		const FProperty* Property = FindCharacterProperty(PropertyName);
		return Property && !Property->HasAnyPropertyFlags(CPF_Transient);
	}

	void TestTD073310CharacterizationDerivedStatsEqual(
		FAutomationTestBase& Test, const FRPGDerivedStats& Actual, const FRPGDerivedStats& Expected)
	{
		Test.TestEqual(TEXT("Rebuilt MaxHealth"), Actual.MaxHealth, Expected.MaxHealth);
		Test.TestEqual(TEXT("Rebuilt MaxMana"), Actual.MaxMana, Expected.MaxMana);
		Test.TestEqual(TEXT("Rebuilt Initiative"), Actual.Initiative, Expected.Initiative);
		Test.TestEqual(TEXT("Rebuilt Accuracy"), Actual.Accuracy, Expected.Accuracy);
		Test.TestEqual(TEXT("Rebuilt Evasion"), Actual.Evasion, Expected.Evasion);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD073310SaveEnvelopeTest,
	"Grimrock.TechnicalDebt.TD07_3_3_10.Characterization.SaveEnvelope",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD073310SaveEnvelopeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestTrue(TEXT("TD07.3.3.10 establishes SaveGame v20 or later"), UGrimrockPartySaveGame::CurrentSaveVersion >= 20);
	TestNotNull(TEXT("PartyInventoryState remains the character-state Save envelope"),
		FindFProperty<FProperty>(UGrimrockPartySaveGame::StaticClass(), TEXT("PartyInventoryState")));

	const TCHAR* RemovedSnapshots[] = {
		TEXT("ClassProgressionStates"),
		TEXT("CharacterSkillStates"),
		TEXT("CharacterSpellbookStates"),
		TEXT("CharacterStatusEffectStates"),
		TEXT("PendingLevelUpNotifications")
	};
	for (const TCHAR* SnapshotName : RemovedSnapshots)
	{
		TestNull(FString::Printf(TEXT("Auxiliary character snapshot %s remains absent"), SnapshotName),
			FindFProperty<FProperty>(UGrimrockPartySaveGame::StaticClass(), SnapshotName));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD073310AuthorityFlagsTest,
	"Grimrock.TechnicalDebt.TD07_3_3_10.Characterization.AuthorityFlags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD073310AuthorityFlagsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD073310Characterization;

	const FProperty* LevelProperty = FindCharacterProperty(TEXT("Level"));
	const FProperty* DerivedStatsProperty = FindCharacterProperty(TEXT("DerivedStats"));
	TestTrue(TEXT("Level is transient projection"), LevelProperty && LevelProperty->HasAnyPropertyFlags(CPF_Transient));
	TestTrue(TEXT("DerivedStats is transient projection"),
		DerivedStatsProperty && DerivedStatsProperty->HasAnyPropertyFlags(CPF_Transient));

	TestTrue(TEXT("Experience is durable authority"), IsDurableCharacterProperty(TEXT("Experience")));
	TestTrue(TEXT("Resources are durable mutable authority"), IsDurableCharacterProperty(TEXT("Resources")));
	TestTrue(TEXT("Progression choices are durable"), IsDurableCharacterProperty(TEXT("SelectedClassProgressionChoiceIds")));
	TestTrue(TEXT("SkillRanks are durable"), IsDurableCharacterProperty(TEXT("SkillRanks")));
	TestTrue(TEXT("KnownSpellIds are durable"), IsDurableCharacterProperty(TEXT("KnownSpellIds")));
	TestTrue(TEXT("StatusEffects are durable"), IsDurableCharacterProperty(TEXT("StatusEffects")));
	TestTrue(TEXT("LastAcknowledgedLevel is durable"), IsDurableCharacterProperty(TEXT("LastAcknowledgedLevel")));

	TestNull(TEXT("Legacy Strength bridge remains absent"), FindCharacterProperty(TEXT("Strength")));
	TestNull(TEXT("Legacy RPG initialization marker remains absent"), FindCharacterProperty(TEXT("bRPGAttributesInitialized")));
	TestNull(TEXT("CurrentWeight cache remains absent"), FindCharacterProperty(TEXT("CurrentWeight")));
	TestNull(TEXT("MaxCarryWeight cache remains absent"), FindCharacterProperty(TEXT("MaxCarryWeight")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD073310DerivedStatsPersistenceGapTest,
	"Grimrock.TechnicalDebt.TD07_3_3_10.Characterization.DerivedStatsPersistenceGap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD073310DerivedStatsPersistenceGapTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD073310Characterization;

	const FProperty* DerivedStatsProperty = FindCharacterProperty(TEXT("DerivedStats"));
	const FProperty* ResourcesProperty = FindCharacterProperty(TEXT("Resources"));
	TestTrue(TEXT("Reconstructible DerivedStats no longer participates in durable serialization"),
		DerivedStatsProperty && DerivedStatsProperty->HasAnyPropertyFlags(CPF_Transient));
	TestTrue(TEXT("Mutable Resources remain durable"),
		ResourcesProperty && !ResourcesProperty->HasAnyPropertyFlags(CPF_Transient));

	const FProperty* DefinitionAssetProperty =
		FindFProperty<FProperty>(FGridStatusEffectRuntimeState::StaticStruct(), TEXT("DefinitionAsset"));
	TestTrue(TEXT("Status-effect definition cache remains transient"),
		DefinitionAssetProperty && DefinitionAssetProperty->HasAnyPropertyFlags(CPF_Transient));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD073310DerivedStatsValidationGapTest,
	"Grimrock.TechnicalDebt.TD07_3_3_10.Characterization.DerivedStatsValidationGap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD073310DerivedStatsValidationGapTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD073310Characterization;

	FMON155RuntimeStateGuard RuntimeGuard;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(2, 1000, ClassDefinition);
	if (!TestNotNull(TEXT("Characterization component exists"), Component) ||
		!TestNotNull(TEXT("Characterization class definition exists"), ClassDefinition))
	{
		return false;
	}

	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	const FRPGDerivedStats Expected =
		URPGCharacterRulesLibrary::CalculateDerivedStats(Character.Attributes, ClassDefinition, Character.Level);
	Character.DerivedStats.MaxHealth += 777;
	Character.DerivedStats.MaxMana += 333;
	Character.DerivedStats.Initiative += 111;

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
	Save->PartyInventoryState = Component->PartyInventoryState;
	TArray<uint8> Bytes;
	TestTrue(TEXT("Transient projection corruption cannot block durable save"), UGameplayStatics::SaveGameToMemory(Save, Bytes));

	UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	TestNotNull(TEXT("Current schema loads after transient projection corruption"), Loaded);
	if (!Loaded)
	{
		return false;
	}

	TestTD073310CharacterizationDerivedStatsEqual(*this, Loaded->PartyInventoryState.ActiveCharacters[0].DerivedStats, Expected);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

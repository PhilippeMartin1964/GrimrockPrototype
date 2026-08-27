#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD073310SaveEnvelopeTest,
	"Grimrock.TechnicalDebt.TD07_3_3_10.Characterization.SaveEnvelope",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD073310SaveEnvelopeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(TEXT("TD07.3.3.9 leaves the exact-match schema at v19"), UGrimrockPartySaveGame::CurrentSaveVersion, 19);
	TestNotNull(TEXT("PartyInventoryState is the character-state Save envelope"),
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
		TestNull(FString::Printf(TEXT("Auxiliary character snapshot %s is absent"), SnapshotName),
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
	TestNotNull(TEXT("Level exists"), LevelProperty);
	TestTrue(TEXT("Level is transient projection"), LevelProperty && LevelProperty->HasAnyPropertyFlags(CPF_Transient));

	TestTrue(TEXT("Experience is durable authority"), IsDurableCharacterProperty(TEXT("Experience")));
	TestTrue(TEXT("Resources are durable mutable authority"), IsDurableCharacterProperty(TEXT("Resources")));
	TestTrue(TEXT("Progression choices are durable"), IsDurableCharacterProperty(TEXT("SelectedClassProgressionChoiceIds")));
	TestTrue(TEXT("SkillRanks are durable"), IsDurableCharacterProperty(TEXT("SkillRanks")));
	TestTrue(TEXT("KnownSpellIds are durable"), IsDurableCharacterProperty(TEXT("KnownSpellIds")));
	TestTrue(TEXT("StatusEffects are durable"), IsDurableCharacterProperty(TEXT("StatusEffects")));
	TestTrue(TEXT("LastAcknowledgedLevel is durable"), IsDurableCharacterProperty(TEXT("LastAcknowledgedLevel")));

	TestNull(TEXT("Legacy Strength bridge is absent"), FindCharacterProperty(TEXT("Strength")));
	TestNull(TEXT("Legacy RPG initialization marker is absent"), FindCharacterProperty(TEXT("bRPGAttributesInitialized")));
	TestNull(TEXT("CurrentWeight cache is absent"), FindCharacterProperty(TEXT("CurrentWeight")));
	TestNull(TEXT("MaxCarryWeight cache is absent"), FindCharacterProperty(TEXT("MaxCarryWeight")));
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
	TestNotNull(TEXT("DerivedStats exists"), DerivedStatsProperty);
	TestFalse(TEXT("Current v19 still serializes the reconstructible DerivedStats projection"),
		DerivedStatsProperty && DerivedStatsProperty->HasAnyPropertyFlags(CPF_Transient));

	const FProperty* DefinitionAssetProperty =
		FindFProperty<FProperty>(FGridStatusEffectRuntimeState::StaticStruct(), TEXT("DefinitionAsset"));
	TestTrue(TEXT("Status-effect definition cache is already transient"),
		DefinitionAssetProperty && DefinitionAssetProperty->HasAnyPropertyFlags(CPF_Transient));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD073310DerivedStatsValidationGapTest,
	"Grimrock.TechnicalDebt.TD07_3_3_10.Characterization.DerivedStatsValidationGap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD073310DerivedStatsValidationGapTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FMON155RuntimeStateGuard RuntimeGuard;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(2, 1000, ClassDefinition);
	TestNotNull(TEXT("Characterization class definition exists"), ClassDefinition);
	if (!Component || !ClassDefinition)
	{
		return false;
	}

	FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	Character.DerivedStats.MaxHealth += 777;
	Character.DerivedStats.MaxMana += 333;
	Character.DerivedStats.Initiative += 111;

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
	Save->PartyInventoryState = Component->PartyInventoryState;

	FText Error;
	TestTrue(TEXT("Current v19 validation does not reject a corrupted reconstructible DerivedStats cache"),
		Save->ValidateCurrentState(Error));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

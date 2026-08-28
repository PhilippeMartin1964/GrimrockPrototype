#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Magic/GridSpellCastTransaction.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "Runtime/GridInventoryTypes.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace GridTD07333Normalization
{
	URPGClassAsset* MakeClass()
	{
		URPGClassAsset* ClassDefinition = NewObject<URPGClassAsset>();
		ClassDefinition->ClassId = TEXT("TD07333_NormalizedClass");
		ClassDefinition->DisplayName = FText::FromString(TEXT("TD07.3.3.3 Normalized Class"));
		ClassDefinition->HealthAtLevelOne = 20;
		ClassDefinition->HealthPerLevel = 5;
		ClassDefinition->ManaAtLevelOne = 8;
		ClassDefinition->ManaPerLevel = 2;
		ClassDefinition->BasePhysicalArmor = 3;
		ClassDefinition->BaseMagicalArmor = 2;
		return ClassDefinition;
	}

	FGridSpellDefinition MakeSpell()
	{
		FGridSpellDefinition Definition;
		Definition.SpellId = TEXT("TD07333_Spell");
		Definition.DisplayName = FText::FromString(TEXT("TD07.3.3.3 Spell"));
		Definition.School = EGridSpellSchool::Arcane;
		Definition.ManaCost = 3;
		Definition.ActionPointCost = 2;
		Definition.MinRangeCells = 0;
		Definition.MaxRangeCells = 1;
		Definition.TargetingPolicy = EGridCombatTargetingPolicy::Self;

		FGridSpellEffectDefinition Effect;
		Effect.Type = EGridSpellEffectType::Heal;
		Effect.Magnitude = 1;
		Definition.Effects.Add(Effect);
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07333SchemaSeparationTest, "Grimrock.TechnicalDebt.TD07_3_3_3.Normalization.SchemaSeparation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07333SchemaSeparationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UScriptStruct* DerivedStruct = FRPGDerivedStats::StaticStruct();
	UScriptStruct* ResourcesStruct = FRPGCharacterResources::StaticStruct();
	UScriptStruct* CharacterStruct = FGridCharacterInventoryState::StaticStruct();

	TestNotNull(TEXT("FRPGDerivedStats reflected"), DerivedStruct);
	TestNotNull(TEXT("FRPGCharacterResources reflected"), ResourcesStruct);
	TestNotNull(TEXT("Character state reflected"), CharacterStruct);

	TestNull(TEXT("DerivedStats no longer owns CurrentHealth"), FindFProperty<FProperty>(DerivedStruct, TEXT("CurrentHealth")));
	TestNull(TEXT("DerivedStats no longer owns CurrentMana"), FindFProperty<FProperty>(DerivedStruct, TEXT("CurrentMana")));
	TestNull(TEXT("DerivedStats no longer owns PhysicalArmor"), FindFProperty<FProperty>(DerivedStruct, TEXT("PhysicalArmor")));
	TestNull(TEXT("DerivedStats no longer owns MagicalArmor"), FindFProperty<FProperty>(DerivedStruct, TEXT("MagicalArmor")));

	TestNotNull(TEXT("Resources owns CurrentHealth"), FindFProperty<FProperty>(ResourcesStruct, TEXT("CurrentHealth")));
	TestNotNull(TEXT("Resources owns CurrentMana"), FindFProperty<FProperty>(ResourcesStruct, TEXT("CurrentMana")));
	TestNotNull(TEXT("Resources owns CurrentPhysicalArmor"), FindFProperty<FProperty>(ResourcesStruct, TEXT("CurrentPhysicalArmor")));
	TestNotNull(TEXT("Resources owns CurrentMagicalArmor"), FindFProperty<FProperty>(ResourcesStruct, TEXT("CurrentMagicalArmor")));
	TestNotNull(TEXT("Character state owns Resources"), FindFProperty<FProperty>(CharacterStruct, TEXT("Resources")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07333ResourceInitializationTest, "Grimrock.TechnicalDebt.TD07_3_3_3.Normalization.ResourceInitialization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07333ResourceInitializationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07333Normalization;

	URPGClassAsset* ClassDefinition = MakeClass();
	const FRPGAttributes Attributes(10, 14, 12, 10, 10, 10);
	const FRPGDerivedStats DerivedStats = URPGCharacterRulesLibrary::CalculateDerivedStats(Attributes, ClassDefinition, 1);
	const FRPGCharacterResources Resources = URPGCharacterRulesLibrary::InitializeCharacterResources(DerivedStats, ClassDefinition);

	TestEqual(TEXT("Calculated MaxHealth"), DerivedStats.MaxHealth, 21);
	TestEqual(TEXT("Calculated MaxMana"), DerivedStats.MaxMana, 8);
	TestEqual(TEXT("Calculated Initiative"), DerivedStats.Initiative, 2);
	TestEqual(TEXT("Calculated Accuracy"), DerivedStats.Accuracy, 2);
	TestEqual(TEXT("Calculated Evasion"), DerivedStats.Evasion, 2);
	TestEqual(TEXT("CurrentHealth starts full"), Resources.CurrentHealth, DerivedStats.MaxHealth);
	TestEqual(TEXT("CurrentMana starts full"), Resources.CurrentMana, DerivedStats.MaxMana);
	TestEqual(TEXT("Physical armor starts from class base"), Resources.CurrentPhysicalArmor, 3);
	TestEqual(TEXT("Magical armor starts from class base"), Resources.CurrentMagicalArmor, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07333MagicResourceBoundaryTest, "Grimrock.TechnicalDebt.TD07_3_3_3.Normalization.MagicResourceBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07333MagicResourceBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07333Normalization;

	const FGuid CharacterId(7, 3, 3, 33);
	const FGridSpellDefinition Definition = MakeSpell();

	FGridCharacterSpellbookState Spellbook;
	Spellbook.CharacterId = CharacterId;
	Spellbook.KnownSpellIds.Add(Definition.SpellId);

	FGridSpellCastRequest Request;
	Request.CasterCharacterId = CharacterId;
	Request.SpellId = Definition.SpellId;

	FRPGCharacterResources Resources;
	Resources.CurrentMana = 8;

	FGridPlayerCharacterTurnState TurnState;
	TurnState.CharacterIndex = 0;
	TurnState.CharacterId = CharacterId;
	TurnState.State = EGridCombatantTurnState::Active;
	TurnState.MaximumActionPoints = 4;
	TurnState.RemainingActionPoints = 4;

	FGridSpellCastCostReceipt Receipt;
	EGridSpellCastTransactionRejectReason RejectReason = EGridSpellCastTransactionRejectReason::InvalidRequest;
	TestTrue(TEXT("Spell costs commit against mutable resources"),
		FGridSpellCastTransactionService::TryCommitCosts(Definition, Request, Spellbook, Resources, TurnState, Receipt, RejectReason));
	TestEqual(TEXT("Mana mutates in Resources"), Resources.CurrentMana, 5);
	TestEqual(TEXT("Action points commit independently"), TurnState.RemainingActionPoints, 2);
	TestEqual(TEXT("Transaction accepted"), RejectReason, EGridSpellCastTransactionRejectReason::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07333SaveSchemaVersionTest, "Grimrock.TechnicalDebt.TD07_3_3_3.Normalization.SaveSchemaVersion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07333SaveSchemaVersionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestTrue(TEXT("The current exact-match schema remains newer than the TD07.3.3.3 v12 generation"), UGrimrockPartySaveGame::CurrentSaveVersion >= 12);

	UGrimrockPartySaveGame* Current = NewObject<UGrimrockPartySaveGame>();
	TestEqual(TEXT("New SaveGame starts on the current schema"), Current->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);
	TestTrue(TEXT("Current exact-match schema is compatible"), Current->IsCompatible());

	UGrimrockPartySaveGame* Previous = NewObject<UGrimrockPartySaveGame>();
	Previous->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion - 1;
	FText Error;
	TestFalse(TEXT("Previous prototype schema is rejected without migration"), Previous->ValidateCurrentState(Error));
	TestFalse(TEXT("Previous prototype schema is incompatible"), Previous->IsCompatible());
	TestEqual(TEXT("Validation does not rewrite the previous schema"), Previous->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion - 1);
	TestTrue(TEXT("Rejected previous schema reports an error"), !Error.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

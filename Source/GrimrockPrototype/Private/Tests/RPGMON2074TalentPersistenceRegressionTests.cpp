#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGTalentRuntimeService.h"
#include "RPGMON155TestHelpers.h"
#include "Save/GrimrockPartySaveGame.h"

namespace RPGMON2074TalentPersistenceRegressionTests
{
	const FName TalentA = TEXT("Choice_A");
	const FName TalentB = TEXT("Choice_B");
	const FName TalentARequirement = TEXT("Feature_A");
	const FName AutomaticLevelRequirement = TEXT("Feature_Level2");

	bool CommitTalentA(UGridPartyInventoryComponent* Component, int32 CharacterIndex)
	{
		FRPGClassProgressionCommitResult Result;
		return FRPGClassProgressionTransactionService::TryCommitChoices(Component, CharacterIndex, { TalentA }, Result);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2074RequirementBeforeTalentTest, "Grimrock.MON20.7.Talents.RequirementBeforeTalent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2074RequirementBeforeTalentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2074TalentPersistenceRegressionTests;
	FMON155RuntimeStateGuard RuntimeGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(3, 3000, ClassDefinition);
	TestTrue(TEXT("Initial MON15 projection refresh succeeds"), FRPGClassProgressionTransactionService::RefreshCharacterProjection(Component, 0));

	const TSet<FName> Requirements = GetMON155RuntimeRequirements(Component, 0);
	TestTrue(TEXT("Automatic level requirement is projected before talents"), Requirements.Contains(AutomaticLevelRequirement));
	TestFalse(TEXT("Talent ChoiceId is absent before acquisition"), Requirements.Contains(TalentA));
	TestFalse(TEXT("Talent granted requirement is absent before acquisition"), Requirements.Contains(TalentARequirement));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2074RequirementAfterTalentTest, "Grimrock.MON20.7.Talents.RequirementAfterTalent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2074RequirementAfterTalentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2074TalentPersistenceRegressionTests;
	FMON155RuntimeStateGuard RuntimeGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(3, 3000, ClassDefinition);
	TestTrue(TEXT("Talent A commits"), CommitTalentA(Component, 0));

	const TSet<FName> Requirements = GetMON155RuntimeRequirements(Component, 0);
	TestTrue(TEXT("Automatic level requirement remains projected"), Requirements.Contains(AutomaticLevelRequirement));
	TestTrue(TEXT("Acquired ChoiceId is projected as a requirement"), Requirements.Contains(TalentA));
	TestTrue(TEXT("GrantedRequirementIds are projected immediately"), Requirements.Contains(TalentARequirement));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2074RequirementCharacterIsolationTest, "Grimrock.MON20.7.Talents.RequirementCharacterIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2074RequirementCharacterIsolationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2074TalentPersistenceRegressionTests;
	FMON155RuntimeStateGuard RuntimeGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(3, 3000, ClassDefinition);
	Component->PartyInventoryState.ActiveCharacters.Add(MakeMON155Character(ClassDefinition, 3, 3000, TEXT("Mina")));
	Component->PartyInventoryState.ActiveEquipment.SetNum(2);

	TestTrue(TEXT("Talent A commits only for character zero"), CommitTalentA(Component, 0));
	TestTrue(TEXT("Character one projection refresh succeeds"), FRPGClassProgressionTransactionService::RefreshCharacterProjection(Component, 1));

	const TSet<FName> CharacterZeroRequirements = GetMON155RuntimeRequirements(Component, 0);
	const TSet<FName> CharacterOneRequirements = GetMON155RuntimeRequirements(Component, 1);
	TestTrue(TEXT("Character zero owns Talent A projection"), CharacterZeroRequirements.Contains(TalentA));
	TestTrue(TEXT("Character zero owns Feature A projection"), CharacterZeroRequirements.Contains(TalentARequirement));
	TestFalse(TEXT("Character one does not inherit Talent A"), CharacterOneRequirements.Contains(TalentA));
	TestFalse(TEXT("Character one does not inherit Feature A"), CharacterOneRequirements.Contains(TalentARequirement));
	TestTrue(TEXT("Character one still owns automatic level requirement"), CharacterOneRequirements.Contains(AutomaticLevelRequirement));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2074CaptureUsesMON15SnapshotTest, "Grimrock.MON20.7.Talents.CaptureUsesMON15Snapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2074CaptureUsesMON15SnapshotTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2074TalentPersistenceRegressionTests;
	FMON155RuntimeStateGuard RuntimeGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(3, 3000, ClassDefinition);
	TestTrue(TEXT("Talent A commits"), CommitTalentA(Component, 0));

	const FGridPartyInventoryState SavedState = Component->PartyInventoryState;
	TestEqual(TEXT("One active character remains in ordinary party snapshot"), SavedState.ActiveCharacters.Num(), 1);
	TestEqual(TEXT("Exactly one talent ChoiceId is persisted on the character"), SavedState.ActiveCharacters[0].SelectedClassProgressionChoiceIds.Num(), 1);
	TestTrue(TEXT("Persisted talent uses the original ChoiceId"), SavedState.ActiveCharacters[0].SelectedClassProgressionChoiceIds.Contains(TalentA));
	TestTrue(TEXT("MON20.7 character-owned persistence remains compatible with later SaveGame versions"), UGrimrockPartySaveGame::CurrentSaveVersion >= 7);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2074RestoreTalentReadModelTest, "Grimrock.MON20.7.Talents.RestoreTalentReadModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2074RestoreTalentReadModelTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2074TalentPersistenceRegressionTests;
	FMON155RuntimeStateGuard RuntimeGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(3, 3000, ClassDefinition);
	TestTrue(TEXT("Talent A commits"), CommitTalentA(Component, 0));

	FRPGClassProgressionTransactionService::ResetRuntimeState(Component);
	bool bHasTalent = false;
	TestTrue(TEXT("MON20.7 read facade remains usable after cache reset"), FRPGTalentRuntimeService::HasTalent(Component, 0, TalentA, bHasTalent));
	TestTrue(TEXT("Character-owned ChoiceId is still exposed as acquired talent"), bHasTalent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2074RestoreDetachedRequirementsTest, "Grimrock.MON20.7.Talents.RestoreDetachedRequirements",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2074RestoreDetachedRequirementsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2074TalentPersistenceRegressionTests;
	FMON155RuntimeStateGuard RuntimeGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(3, 3000, ClassDefinition);
	TestTrue(TEXT("Talent A commits"), CommitTalentA(Component, 0));

	const FGridPartyInventoryState SavedState = Component->PartyInventoryState;
	const FGuid CharacterId = SavedState.ActiveCharacters[0].CharacterId;
	FRPGClassProgressionTransactionService::ResetRuntimeState();
	FText RestoreError;
	TestTrue(TEXT("Detached progression projection rebuilds from character-owned state"),
		FRPGClassProgressionTransactionService::RebuildRuntimeProjection(SavedState, RestoreError));

	TSet<FName> Requirements;
	FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements(CharacterId, Requirements);
	TestTrue(TEXT("Detached rebuild immediately projects automatic requirement"), Requirements.Contains(AutomaticLevelRequirement));
	TestTrue(TEXT("Detached rebuild immediately projects ChoiceId"), Requirements.Contains(TalentA));
	TestTrue(TEXT("Detached rebuild immediately projects GrantedRequirementIds"), Requirements.Contains(TalentARequirement));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2074InvalidRestoreAtomicTest, "Grimrock.MON20.7.Talents.InvalidRestoreAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2074InvalidRestoreAtomicTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2074TalentPersistenceRegressionTests;
	FMON155RuntimeStateGuard RuntimeGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(3, 3000, ClassDefinition);
	TestTrue(TEXT("Talent A commits"), CommitTalentA(Component, 0));
	const FGuid CharacterId = Component->PartyInventoryState.ActiveCharacters[0].CharacterId;
	const TSet<FName> RequirementsBefore = GetMON155RuntimeRequirements(Component, 0);

	FGridPartyInventoryState InvalidState = Component->PartyInventoryState;
	InvalidState.ActiveCharacters[0].SelectedClassProgressionChoiceIds = { TalentB };
	FText RestoreError;
	TestFalse(TEXT("Character state missing Talent B prerequisite is rejected"),
		FRPGClassProgressionTransactionService::RebuildRuntimeProjection(InvalidState, RestoreError));

	TSet<FName> RequirementsAfter;
	FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements(CharacterId, RequirementsAfter);
	TestTrue(TEXT("Rejected rebuild preserves existing Talent A projection"), RequirementsAfter.Contains(TalentA));
	TestTrue(TEXT("Rejected rebuild preserves Feature A projection"), RequirementsAfter.Contains(TalentARequirement));
	TestEqual(TEXT("Rejected rebuild preserves prior requirement count"), RequirementsAfter.Num(), RequirementsBefore.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2074RestoreByCharacterIdTest, "Grimrock.MON20.7.Talents.RestoreByCharacterId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2074RestoreByCharacterIdTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2074TalentPersistenceRegressionTests;
	FMON155RuntimeStateGuard RuntimeGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(3, 3000, ClassDefinition);
	Component->PartyInventoryState.ActiveCharacters.Add(MakeMON155Character(ClassDefinition, 3, 3000, TEXT("Mina")));
	Component->PartyInventoryState.ActiveEquipment.SetNum(2);

	TestTrue(TEXT("Character zero commits Talent A"), CommitTalentA(Component, 0));
	FRPGClassProgressionCommitResult CharacterOneResult;
	TestTrue(TEXT("Character one commits prerequisite chain A+B"),
		FRPGClassProgressionTransactionService::TryCommitChoices(Component, 1, { TalentA, TalentB }, CharacterOneResult));

	FGridPartyInventoryState SavedState = Component->PartyInventoryState;
	SavedState.ActiveCharacters.Swap(0, 1);
	SavedState.ActiveEquipment.Swap(0, 1);

	FRPGClassProgressionTransactionService::ResetRuntimeState();
	FText RestoreError;
	TestTrue(TEXT("Projection rebuild follows character-owned choices after party reordering"),
		FRPGClassProgressionTransactionService::RebuildRuntimeProjection(SavedState, RestoreError));

	TestEqual(TEXT("First reordered character retains two talents"), SavedState.ActiveCharacters[0].SelectedClassProgressionChoiceIds.Num(), 2);
	TestTrue(TEXT("First reordered character retains Talent A"), SavedState.ActiveCharacters[0].SelectedClassProgressionChoiceIds.Contains(TalentA));
	TestTrue(TEXT("First reordered character retains Talent B"), SavedState.ActiveCharacters[0].SelectedClassProgressionChoiceIds.Contains(TalentB));
	TestEqual(TEXT("Second reordered character retains one talent"), SavedState.ActiveCharacters[1].SelectedClassProgressionChoiceIds.Num(), 1);
	TestTrue(TEXT("Second reordered character retains Talent A"), SavedState.ActiveCharacters[1].SelectedClassProgressionChoiceIds.Contains(TalentA));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

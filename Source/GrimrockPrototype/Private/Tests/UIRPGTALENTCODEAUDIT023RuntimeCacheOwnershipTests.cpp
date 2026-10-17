#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPGMON155TestHelpers.h"
#include "Save/GrimrockPartySaveGame.h"

namespace UIRPGTalentCodeAudit023
{
	UGridPartyInventoryComponent* MakeSiblingInventory(URPGClassAsset* ClassDefinition, const FGuid& CharacterId, const TCHAR* DisplayName)
	{
		UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent>();
		FGridCharacterInventoryState Character = MakeMON155Character(ClassDefinition, 3, 3000, DisplayName);
		Character.CharacterId = CharacterId;
		Component->PartyInventoryState.ActiveCharacters.Add(MoveTemp(Character));
		Component->PartyInventoryState.ActiveEquipment.SetNum(1);
		return Component;
	}

	TSet<FName> ReadCachedRequirements(UGridPartyInventoryComponent* Component, const FGuid& CharacterId)
	{
		TSet<FName> Requirements;
		FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements(Component, CharacterId, Requirements);
		return Requirements;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGTalentCodeAudit023ComponentIsolationTest,
	"Grimrock.UI.RPG.CODEAUDIT02.RuntimeCacheOwnership.ComponentIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGTalentCodeAudit023ComponentIsolationTest::RunTest(const FString&)
{
	using namespace UIRPGTalentCodeAudit023;
	FMON155RuntimeStateGuard RuntimeGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* ComponentA = MakeMON155Inventory(3, 3000, ClassDefinition);
	const FGuid SharedCharacterId = ComponentA->PartyInventoryState.ActiveCharacters[0].CharacterId;
	UGridPartyInventoryComponent* ComponentB = MakeSiblingInventory(ClassDefinition, SharedCharacterId, TEXT("Mina"));

	FRPGClassProgressionCommitResult CommitResult;
	TestTrue(TEXT("Component A commits Choice A"),
		FRPGClassProgressionTransactionService::TryCommitChoices(ComponentA, 0, { TEXT("Choice_A") }, CommitResult));
	TestTrue(TEXT("Component B builds its independent projection"),
		FRPGClassProgressionTransactionService::RefreshCharacterProjection(ComponentB, 0));

	const TSet<FName> RequirementsA = ReadCachedRequirements(ComponentA, SharedCharacterId);
	const TSet<FName> RequirementsB = ReadCachedRequirements(ComponentB, SharedCharacterId);
	TestTrue(TEXT("Component A owns Choice A"), RequirementsA.Contains(TEXT("Choice_A")));
	TestTrue(TEXT("Component A owns Feature A"), RequirementsA.Contains(TEXT("Feature_A")));
	TestFalse(TEXT("Component B does not inherit Choice A despite the same CharacterId"), RequirementsB.Contains(TEXT("Choice_A")));
	TestFalse(TEXT("Component B does not inherit Feature A despite the same CharacterId"), RequirementsB.Contains(TEXT("Feature_A")));
	TestTrue(TEXT("Component B still owns its automatic level requirement"), RequirementsB.Contains(TEXT("Feature_Level2")));

	FRPGClassProgressionTransactionService::ResetRuntimeState(ComponentA);
	TestTrue(TEXT("Scoped reset clears only component A"), ReadCachedRequirements(ComponentA, SharedCharacterId).IsEmpty());
	TestTrue(TEXT("Scoped reset preserves component B"),
		ReadCachedRequirements(ComponentB, SharedCharacterId).Contains(TEXT("Feature_Level2")));

	ComponentA->PartyInventoryState.ActiveCharacters[0].SelectedClassProgressionChoiceIds.Reset();
	TestTrue(TEXT("Reused CharacterId can rebuild component A from its current durable state"),
		FRPGClassProgressionTransactionService::RefreshCharacterProjection(ComponentA, 0));
	const TSet<FName> ReusedRequirementsA = ReadCachedRequirements(ComponentA, SharedCharacterId);
	TestFalse(TEXT("Reused CharacterId does not retain the previous Choice A projection"), ReusedRequirementsA.Contains(TEXT("Choice_A")));
	TestTrue(TEXT("Reused CharacterId keeps current automatic requirements"), ReusedRequirementsA.Contains(TEXT("Feature_Level2")));
	TestFalse(TEXT("Component A rebuild does not alter component B"), ReadCachedRequirements(ComponentB, SharedCharacterId).Contains(TEXT("Choice_A")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGTalentCodeAudit023SaveLoadIsolationTest,
	"Grimrock.UI.RPG.CODEAUDIT02.RuntimeCacheOwnership.SaveLoadIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGTalentCodeAudit023SaveLoadIsolationTest::RunTest(const FString&)
{
	using namespace UIRPGTalentCodeAudit023;
	FMON155RuntimeStateGuard RuntimeGuard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* SourceComponent = MakeMON155Inventory(3, 3000, ClassDefinition);
	const FGuid SharedCharacterId = SourceComponent->PartyInventoryState.ActiveCharacters[0].CharacterId;
	UGridPartyInventoryComponent* LiveComponent = MakeSiblingInventory(ClassDefinition, SharedCharacterId, TEXT("Live"));

	FRPGClassProgressionCommitResult CommitResult;
	TestTrue(TEXT("Source component commits Choice A"),
		FRPGClassProgressionTransactionService::TryCommitChoices(SourceComponent, 0, { TEXT("Choice_A") }, CommitResult));
	TestTrue(TEXT("Live component builds a no-choice projection"),
		FRPGClassProgressionTransactionService::RefreshCharacterProjection(LiveComponent, 0));
	const TSet<FName> LiveRequirementsBefore = ReadCachedRequirements(LiveComponent, SharedCharacterId);
	TestFalse(TEXT("Live component starts without Choice A"), LiveRequirementsBefore.Contains(TEXT("Choice_A")));

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
	Save->PartyInventoryState = SourceComponent->PartyInventoryState;
	TArray<uint8> Bytes;
	TestTrue(TEXT("Source SaveGame serializes"), UGameplayStatics::SaveGameToMemory(Save, Bytes));

	UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("Source SaveGame deserializes"), Loaded))
	{
		return false;
	}

	const TSet<FName> LiveRequirementsAfter = ReadCachedRequirements(LiveComponent, SharedCharacterId);
	TestEqual(TEXT("SaveGame deserialization leaves the live cache unchanged"), LiveRequirementsAfter.Num(), LiveRequirementsBefore.Num());
	TestFalse(TEXT("SaveGame deserialization does not leak Choice A into the live component"), LiveRequirementsAfter.Contains(TEXT("Choice_A")));
	TestTrue(TEXT("Loaded durable character still owns Choice A"),
		Loaded->PartyInventoryState.ActiveCharacters[0].SelectedClassProgressionChoiceIds.Contains(TEXT("Choice_A")));

	UGridPartyInventoryComponent* LoadedComponent = NewObject<UGridPartyInventoryComponent>();
	LoadedComponent->PartyInventoryState = Loaded->PartyInventoryState;
	FText RebuildError;
	TestTrue(TEXT("Loaded durable state explicitly rebuilds after attachment to a live component"),
		FRPGClassProgressionTransactionService::RebuildRuntimeProjection(LoadedComponent, RebuildError));
	TestTrue(TEXT("Explicit live rebuild reports no error"), RebuildError.IsEmpty());

	const TSet<FName> LoadedRequirements = ReadCachedRequirements(LoadedComponent, SharedCharacterId);
	TestTrue(TEXT("Explicit live rebuild restores Choice A requirement"), LoadedRequirements.Contains(TEXT("Choice_A")));
	TestTrue(TEXT("Explicit live rebuild restores Feature A requirement"), LoadedRequirements.Contains(TEXT("Feature_A")));
	TestFalse(TEXT("Loaded component projection remains isolated from the other live component"),
		ReadCachedRequirements(LiveComponent, SharedCharacterId).Contains(TEXT("Choice_A")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

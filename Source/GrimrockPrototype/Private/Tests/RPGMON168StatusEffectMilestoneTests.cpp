#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RPG/StatusEffects/GridStatusEffectControlResolver.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectInitiativeResolver.h"
#include "RPG/StatusEffects/GridStatusEffectPersistence.h"
#include "RPG/StatusEffects/GridStatusEffectPresentation.h"
#include "Save/GrimrockPartySaveGame.h"

namespace
{
	UGridStatusEffectDefinitionAsset* MON168MakeDefinition(
		FName EffectId, EGridStatusEffectDurationUnit DurationUnit = EGridStatusEffectDurationUnit::Rounds, int32 DefaultDuration = 3)
	{
		UGridStatusEffectDefinitionAsset* Definition = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
		Definition->EffectId = EffectId;
		Definition->DisplayName = FText::FromName(EffectId);
		Definition->Description = FText::FromString(TEXT("MON16.8 milestone contract definition."));
		Definition->Disposition = EGridStatusEffectDisposition::Debuff;
		Definition->DurationUnit = DurationUnit;
		Definition->DefaultDuration = DurationUnit == EGridStatusEffectDurationUnit::Permanent ? 0 : DefaultDuration;
		Definition->DefaultPotency = 0;
		Definition->StackPolicy = EGridStatusEffectStackPolicy::NoStack;
		Definition->MaxStacks = 1;
		return Definition;
	}

	bool MON168LoadProjectFile(const TCHAR* RelativePath, FString& OutText)
	{
		const FString Path = FPaths::Combine(FPaths::ProjectDir(), RelativePath);
		return FFileHelper::LoadFileToString(OutText, *Path);
	}

	int32 MON168CountOccurrences(const FString& Text, const FString& Token)
	{
		if (Token.IsEmpty())
		{
			return 0;
		}

		int32 Count = 0;
		int32 SearchFrom = 0;
		while (SearchFrom < Text.Len())
		{
			const int32 Found = Text.Find(Token, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
			if (Found == INDEX_NONE)
			{
				break;
			}
			++Count;
			SearchFrom = Found + Token.Len();
		}
		return Count;
	}

	bool MON168ExtractSection(const FString& Text, const TCHAR* StartToken, const TCHAR* EndToken, FString& OutSection)
	{
		const int32 Start = Text.Find(StartToken, ESearchCase::CaseSensitive);
		if (Start == INDEX_NONE)
		{
			return false;
		}

		const int32 End = Text.Find(EndToken, ESearchCase::CaseSensitive, ESearchDir::FromStart, Start + FCString::Strlen(StartToken));
		if (End == INDEX_NONE || End <= Start)
		{
			return false;
		}

		OutSection = Text.Mid(Start, End - Start);
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON168PrimaryAssetIdentityContractTest, "Grimrock.RPG.MON16.8.PrimaryAssetIdentityContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON168PrimaryAssetIdentityContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridStatusEffectDefinitionAsset* Definition = MON168MakeDefinition(TEXT("MON168_Identity"));
	const FPrimaryAssetId AssetId = Definition->GetPrimaryAssetId();

	TestEqual(TEXT("Primary asset type remains GridStatusEffect"), AssetId.PrimaryAssetType.ToString(), FString(TEXT("GridStatusEffect")));
	TestEqual(TEXT("Primary asset name is the stable EffectId"), AssetId.PrimaryAssetName, FName(TEXT("MON168_Identity")));
	TestEqual(TEXT("Primary asset string is stable"), AssetId.ToString(), FString(TEXT("GridStatusEffect:MON168_Identity")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON168CrossFeatureCompositionTest, "Grimrock.RPG.MON16.8.CrossFeatureComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON168CrossFeatureCompositionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridStatusEffectDefinitionAsset* Definition = MON168MakeDefinition(TEXT("MON168_Composed"));
	Definition->StackPolicy = EGridStatusEffectStackPolicy::AddStacks;
	Definition->MaxStacks = 3;
	Definition->DefaultPotency = 7;
	Definition->InitiativeModifier = 4;
	Definition->PeriodicDamage.DamageType = EGridDamageType::Fire;
	Definition->PeriodicDamage.DamagePerStack = 2;
	Definition->Control.bBlockSpellActions = true;
	Definition->Control.bBlockTranslation = true;

	FString Error;
	TestTrue(TEXT("Composed definition remains valid"), Definition->ValidateDefinition(Error));

	FGridStatusEffectCollection Collection;
	TestTrue(TEXT("Composed definition enters canonical runtime collection"), Collection.TryAdd(*Definition, FGuid(16, 8, 2, 1), 2, 3, Error));

	const FGridStatusEffectRuntimeState* State = Collection.FindByEffectId(Definition->EffectId);
	TestNotNull(TEXT("Composed runtime state exists"), State);
	if (!State)
	{
		return false;
	}

	TestEqual(TEXT("Initiative resolver consumes stack count"), FGridStatusEffectInitiativeResolver::ComputeModifier(Collection), 8);

	const FGridStatusEffectControlProfile Control = FGridStatusEffectControlResolver::Resolve(Collection);
	TestFalse(TEXT("Composed status does not implicitly skip activation"), Control.bSkipActivation);
	TestTrue(TEXT("Composed status blocks spells from data"), Control.bBlockSpellActions);
	TestTrue(TEXT("Composed status blocks translation from data"), Control.bBlockTranslation);

	FGridStatusEffectPresentationView View;
	TestTrue(TEXT("Presentation projects the same runtime state"), FGridStatusEffectPresentationBuilder::BuildOne(*State, View));
	TestEqual(TEXT("Presentation keeps stack count"), View.StackCount, 2);
	TestEqual(TEXT("Presentation keeps potency"), View.Potency, 7);
	TestEqual(TEXT("Presentation initiative matches resolver"), View.InitiativeContribution, 8);
	TestTrue(TEXT("Presentation exposes periodic damage"), View.bPeriodicDamage);
	TestTrue(TEXT("Presentation exposes spell control"), View.bBlockSpellActions);
	TestTrue(TEXT("Presentation exposes translation control"), View.bBlockTranslation);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON168PersistenceRoundTripSemanticsTest, "Grimrock.RPG.MON16.8.PersistenceRoundTripSemantics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON168PersistenceRoundTripSemanticsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridStatusEffectDefinitionAsset* Definition = MON168MakeDefinition(TEXT("MON168_RoundTrip"), EGridStatusEffectDurationUnit::Turns, 5);
	Definition->StackPolicy = EGridStatusEffectStackPolicy::AddStacks;
	Definition->MaxStacks = 4;
	Definition->DefaultPotency = 9;
	Definition->InitiativeModifier = -3;
	Definition->PeriodicDamage.DamageType = EGridDamageType::Fire;
	Definition->PeriodicDamage.DamagePerStack = 1;
	Definition->Control.bSkipActivation = true;
	Definition->Control.bBlockSpellActions = true;

	FGridStatusEffectCollection Before;
	FString Error;
	TestTrue(TEXT("Round-trip source status enters runtime"), Before.TryAdd(*Definition, FGuid(16, 8, 3, 1), 3, 4, Error));

	TArray<FGridStatusEffectSaveState> Saved;
	TestTrue(TEXT("Canonical persistence captures runtime"), FGridStatusEffectPersistence::CaptureCollection(Before, Saved, Error));
	TestEqual(TEXT("Exactly one snapshot is captured"), Saved.Num(), 1);

	FGridStatusEffectCollection After;
	TestTrue(TEXT("Canonical persistence restores with definition rebind"),
		FGridStatusEffectPersistence::RestoreCollection(
			Saved,
			[Definition](FName EffectId)
			{
				return EffectId == Definition->EffectId ? Definition : nullptr;
			},
			After, Error));

	const FGridStatusEffectRuntimeState* BeforeState = Before.FindByEffectId(Definition->EffectId);
	const FGridStatusEffectRuntimeState* AfterState = After.FindByEffectId(Definition->EffectId);
	TestNotNull(TEXT("Before state exists"), BeforeState);
	TestNotNull(TEXT("After state exists"), AfterState);
	if (!BeforeState || !AfterState)
	{
		return false;
	}

	TestEqual(TEXT("EffectId survives"), AfterState->EffectId, BeforeState->EffectId);
	TestEqual(TEXT("SourceId survives"), AfterState->SourceId, BeforeState->SourceId);
	TestEqual(TEXT("Stacks survive"), AfterState->StackCount, BeforeState->StackCount);
	TestEqual(TEXT("Duration unit survives"), AfterState->DurationUnit, BeforeState->DurationUnit);
	TestEqual(TEXT("Remaining duration survives"), AfterState->RemainingDuration, BeforeState->RemainingDuration);
	TestEqual(TEXT("Potency survives"), AfterState->Potency, BeforeState->Potency);
	TestTrue(TEXT("Static definition is rebound, not serialized"), AfterState->DefinitionAsset.Get() == Definition);

	TestEqual(TEXT("Initiative semantics survive restore"), FGridStatusEffectInitiativeResolver::ComputeModifier(After),
		FGridStatusEffectInitiativeResolver::ComputeModifier(Before));

	const FGridStatusEffectControlProfile BeforeControl = FGridStatusEffectControlResolver::Resolve(Before);
	const FGridStatusEffectControlProfile AfterControl = FGridStatusEffectControlResolver::Resolve(After);
	TestEqual(TEXT("Skip activation semantics survive restore"), AfterControl.bSkipActivation, BeforeControl.bSkipActivation);
	TestEqual(TEXT("Spell blocking semantics survive restore"), AfterControl.bBlockSpellActions, BeforeControl.bBlockSpellActions);

	FGridStatusEffectPresentationView BeforeView;
	FGridStatusEffectPresentationView AfterView;
	TestTrue(TEXT("Before presentation builds"), FGridStatusEffectPresentationBuilder::BuildOne(*BeforeState, BeforeView));
	TestTrue(TEXT("After presentation builds"), FGridStatusEffectPresentationBuilder::BuildOne(*AfterState, AfterView));
	TestEqual(TEXT("Presentation initiative survives restore"), AfterView.InitiativeContribution, BeforeView.InitiativeContribution);
	TestEqual(TEXT("Presentation periodic flag survives restore"), AfterView.bPeriodicDamage, BeforeView.bPeriodicDamage);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON168DeterministicPersistenceOrderTest, "Grimrock.RPG.MON16.8.DeterministicPersistenceOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON168DeterministicPersistenceOrderTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridStatusEffectDefinitionAsset* DefinitionA = MON168MakeDefinition(TEXT("MON168_A"));
	UGridStatusEffectDefinitionAsset* DefinitionB = MON168MakeDefinition(TEXT("MON168_B"));

	FGridStatusEffectCollection Collection;
	FString Error;
	TestTrue(TEXT("B adds first"), Collection.TryAdd(*DefinitionB, FGuid(16, 8, 4, 2), Error));
	TestTrue(TEXT("A adds second"), Collection.TryAdd(*DefinitionA, FGuid(16, 8, 4, 1), Error));
	TestEqual(TEXT("Runtime collection has two entries"), Collection.Num(), 2);
	if (Collection.Num() != 2)
	{
		return false;
	}

	TestEqual(TEXT("Runtime collection is sorted by EffectId"), Collection.ActiveEffects[0].EffectId, DefinitionA->EffectId);

	TArray<FGridStatusEffectSaveState> Saved;
	TestTrue(TEXT("Capture succeeds"), FGridStatusEffectPersistence::CaptureCollection(Collection, Saved, Error));
	TestEqual(TEXT("Capture has two entries"), Saved.Num(), 2);
	if (Saved.Num() != 2)
	{
		return false;
	}
	TestEqual(TEXT("Saved collection is sorted by EffectId"), Saved[0].EffectId, DefinitionA->EffectId);

	TArray<FGridStatusEffectSaveState> UnsortedSaved;
	UnsortedSaved.Add(Saved[1]);
	UnsortedSaved.Add(Saved[0]);

	FGridStatusEffectCollection Restored;
	TestTrue(TEXT("Restore accepts structurally valid unsorted snapshot"),
		FGridStatusEffectPersistence::RestoreCollection(
			UnsortedSaved,
			[DefinitionA, DefinitionB](FName EffectId)
			{
				if (EffectId == DefinitionA->EffectId)
				{
					return DefinitionA;
				}
				return EffectId == DefinitionB->EffectId ? DefinitionB : nullptr;
			},
			Restored, Error));
	TestEqual(TEXT("Restored collection has two entries"), Restored.Num(), 2);
	if (Restored.Num() == 2)
	{
		TestEqual(TEXT("Restore normalizes deterministic order"), Restored.ActiveEffects[0].EffectId, DefinitionA->EffectId);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON168RuntimeSaveBoundaryTest, "Grimrock.RPG.MON16.8.RuntimeSaveBoundary", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON168RuntimeSaveBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FString InventoryText;
	FString MonsterText;
	FString TypesText;
	FString SaveText;
	TestTrue(
		TEXT("GridInventoryTypes source loads"), MON168LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Runtime/GridInventoryTypes.h"), InventoryText));
	TestTrue(
		TEXT("GridMonsterActor source loads"), MON168LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Runtime/Monsters/GridMonsterActor.h"), MonsterText));
	TestTrue(TEXT("GridStatusEffectTypes source loads"),
		MON168LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectTypes.h"), TypesText));
	TestTrue(TEXT("GrimrockPartySaveGame source loads"), MON168LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Save/GrimrockPartySaveGame.h"), SaveText));

	TestTrue(TEXT("Party runtime status collection remains Transient"),
		InventoryText.Contains(TEXT("UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = \"RPG|Status Effects\")")));
	TestTrue(TEXT("Monster runtime status collection remains Transient"),
		MonsterText.Contains(TEXT("UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = \"Monster|Status Effects\")")));

	FString RuntimeSection;
	FString SaveSection;
	TestTrue(TEXT("Runtime status section can be isolated"),
		MON168ExtractSection(TypesText, TEXT("struct GRIMROCKPROTOTYPE_API FGridStatusEffectRuntimeState"),
			TEXT("struct GRIMROCKPROTOTYPE_API FGridStatusEffectSaveState"), RuntimeSection));
	TestTrue(TEXT("Save status section can be isolated"),
		MON168ExtractSection(TypesText, TEXT("struct GRIMROCKPROTOTYPE_API FGridStatusEffectSaveState"),
			TEXT("struct GRIMROCKPROTOTYPE_API FGridStatusEffectApplyResult"), SaveSection));

	TestTrue(
		TEXT("Runtime state owns transient DefinitionAsset"), RuntimeSection.Contains(TEXT("Transient")) && RuntimeSection.Contains(TEXT("DefinitionAsset")));
	TestTrue(TEXT("Save state fields are marked SaveGame"), SaveSection.Contains(TEXT("UPROPERTY(SaveGame")));
	TestFalse(TEXT("Save state never contains DefinitionAsset"), SaveSection.Contains(TEXT("DefinitionAsset")));
	TestTrue(TEXT("SaveGame owns party status snapshots"), SaveText.Contains(TEXT("CharacterStatusEffectStates")) && SaveText.Contains(TEXT("SaveGame")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON168SaveVersionContractTest, "Grimrock.RPG.MON16.8.SaveVersionContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON168SaveVersionContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestTrue(TEXT("Current prototype SaveVersion remains at least the MON16.8 v13 generation"), UGrimrockPartySaveGame::CurrentSaveVersion >= 13);

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>(GetTransientPackage());
	TestEqual(TEXT("New save object starts on current version"), Save->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);
	UGrimrockPartySaveGame* Previous = NewObject<UGrimrockPartySaveGame>(GetTransientPackage());
	Previous->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion - 1;
	TestFalse(TEXT("Previous prototype SaveVersion is incompatible"), Previous->IsCompatible());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON168LifecycleArchitectureBoundaryTest, "Grimrock.RPG.MON16.8.LifecycleArchitectureBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON168LifecycleArchitectureBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FString HeaderText;
	FString SourceText;
	TestTrue(TEXT("Lifecycle header loads"),
		MON168LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"), HeaderText));
	TestTrue(TEXT("Lifecycle source loads"),
		MON168LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.cpp"), SourceText));

	TestTrue(TEXT("Lifecycle remains a world subsystem"), HeaderText.Contains(TEXT("public UWorldSubsystem")));
	TestTrue(TEXT("Lifecycle binds combatant state events"), SourceText.Contains(TEXT("OnCombatantStateChanged.AddUniqueDynamic")));
	TestTrue(TEXT("Lifecycle binds round events"), SourceText.Contains(TEXT("OnRoundStarted.AddUniqueDynamic")));
	TestTrue(TEXT("Lifecycle binds combat-end events"), SourceText.Contains(TEXT("OnCombatEnded.AddUniqueDynamic")));

	const TCHAR* Forbidden[] = { TEXT("TickComponent"), TEXT("SetTimer"), TEXT("FTimerManager"), TEXT("FDateTime::"), TEXT("FPlatformTime::"),
		TEXT("UUserWidget"), TEXT("WBP_") };
	for (const TCHAR* Token : Forbidden)
	{
		TestFalse(*FString::Printf(TEXT("Lifecycle has no forbidden dependency: %s"), Token), HeaderText.Contains(Token) || SourceText.Contains(Token));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON168NoHardCodedStatusIdentityTest, "Grimrock.RPG.MON16.8.NoHardCodedStatusIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON168NoHardCodedStatusIdentityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const TCHAR* Sources[] = { TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectDefinitionAsset.cpp"),
		TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.cpp"),
		TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectPeriodicDamageResolver.cpp"),
		TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectInitiativeResolver.cpp"),
		TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectControlResolver.cpp"),
		TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectPresentation.cpp"),
		TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectPersistence.cpp"),
		TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerInitiative.cpp"),
		TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerPartyMovement.cpp"),
		TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerPlayerActionCatalog.cpp") };

	const TCHAR* ForbiddenIdentities[] = { TEXT("TEXT (\"Poison\")"), TEXT("TEXT (\"Bleeding\")"), TEXT("TEXT (\"Burning\")"), TEXT("TEXT (\"Haste\")"),
		TEXT("TEXT (\"Slow\")"), TEXT("TEXT (\"Stun\")"), TEXT("TEXT (\"Silence\")"), TEXT("TEXT (\"Immobilize\")") };

	for (const TCHAR* RelativePath : Sources)
	{
		FString Text;
		TestTrue(*FString::Printf(TEXT("Production source loads: %s"), RelativePath), MON168LoadProjectFile(RelativePath, Text));

		TestFalse(*FString::Printf(TEXT("No text-literal EffectId equality branch in %s"), RelativePath),
			Text.Contains(TEXT("EffectId == TEXT")) || Text.Contains(TEXT("EffectId != TEXT")));

		for (const TCHAR* ForbiddenIdentity : ForbiddenIdentities)
		{
			TestFalse(*FString::Printf(TEXT("No hard-coded status identity %s in %s"), ForbiddenIdentity, RelativePath), Text.Contains(ForbiddenIdentity));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON168RegressionNamespaceCoverageTest, "Grimrock.RPG.MON16.8.RegressionNamespaceCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON168RegressionNamespaceCoverageTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	struct FRegressionFile
	{
		const TCHAR* RelativePath;
		const TCHAR* NamespaceToken;
		int32 ExpectedCount;
	};

	const FRegressionFile Files[] = { { TEXT("Source/GrimrockPrototype/Private/Tests/RPGMON161StatusEffectTests.cpp"), TEXT("Grimrock.RPG.MON16.1."), 7 },
		{ TEXT("Source/GrimrockPrototype/Private/Tests/RPGMON162StatusEffectLifecycleTests.cpp"), TEXT("Grimrock.RPG.MON16.2."), 10 },
		{ TEXT("Source/GrimrockPrototype/Private/Tests/RPGMON163StatusEffectPeriodicDamageTests.cpp"), TEXT("Grimrock.RPG.MON16.3."), 11 },
		{ TEXT("Source/GrimrockPrototype/Private/Tests/RPGMON164StatusEffectInitiativeTests.cpp"), TEXT("Grimrock.RPG.MON16.4."), 11 },
		{ TEXT("Source/GrimrockPrototype/Private/Tests/RPGMON165StatusEffectControlTests.cpp"), TEXT("Grimrock.RPG.MON16.5."), 11 },
		{ TEXT("Source/GrimrockPrototype/Private/Tests/RPGMON166StatusEffectPresentationTests.cpp"), TEXT("Grimrock.RPG.MON16.6."), 10 },
		{ TEXT("Source/GrimrockPrototype/Private/Tests/RPGMON167StatusEffectPersistenceTests.cpp"), TEXT("Grimrock.RPG.MON16.7."), 10 } };

	int32 Total = 0;
	for (const FRegressionFile& File : Files)
	{
		FString Text;
		TestTrue(*FString::Printf(TEXT("Regression source loads: %s"), File.RelativePath), MON168LoadProjectFile(File.RelativePath, Text));
		const int32 Count = MON168CountOccurrences(Text, File.NamespaceToken);
		TestEqual(*FString::Printf(TEXT("Frozen regression count for %s"), File.NamespaceToken), Count, File.ExpectedCount);
		Total += Count;
	}

	TestEqual(TEXT("MON16.1-MON16.7 frozen regression baseline is 70 tests"), Total, 70);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON168SingleCanonicalModelTest, "Grimrock.RPG.MON16.8.SingleCanonicalModel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON168SingleCanonicalModelTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FString TypesText;
	FString LifecycleText;
	FString PersistenceText;
	TestTrue(TEXT("Canonical types source loads"),
		MON168LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectTypes.h"), TypesText));
	TestTrue(TEXT("Canonical lifecycle source loads"),
		MON168LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"), LifecycleText));
	TestTrue(TEXT("Canonical persistence source loads"),
		MON168LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectPersistence.h"), PersistenceText));

	TestEqual(TEXT("Exactly one canonical runtime collection declaration"),
		MON168CountOccurrences(TypesText, TEXT("struct GRIMROCKPROTOTYPE_API FGridStatusEffectCollection")), 1);
	TestEqual(TEXT("Exactly one canonical lifecycle declaration"),
		MON168CountOccurrences(LifecycleText, TEXT("class GRIMROCKPROTOTYPE_API UGridStatusEffectLifecycleSubsystem")), 1);
	TestEqual(TEXT("Exactly one canonical persistence boundary declaration"),
		MON168CountOccurrences(PersistenceText, TEXT("struct GRIMROCKPROTOTYPE_API FGridStatusEffectPersistence")), 1);

	TestFalse(TEXT("Canonical status headers do not introduce a parallel generic status collection"), TypesText.Contains(TEXT("FStatusEffectCollection")));
	TestFalse(
		TEXT("Canonical lifecycle header does not introduce a parallel generic status subsystem"), LifecycleText.Contains(TEXT("UStatusEffectSubsystem")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

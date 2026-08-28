#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillRequirementProjectionService.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridSkillsPageService.h"

namespace RPGMON2094Tests
{
	struct FRuntimeStateGuard
	{
		FRuntimeStateGuard()
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState();
		}

		~FRuntimeStateGuard()
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState();
		}
	};

	URPGClassAsset* MakeMON2094Class(UObject* Outer)
	{
		URPGClassAsset* ClassDefinition = NewObject<URPGClassAsset>(Outer);
		ClassDefinition->ClassId = TEXT("MON2094_Rogue");
		ClassDefinition->DisplayName = FText::FromString(TEXT("Voleur"));
		ClassDefinition->HealthAtLevelOne = 16;
		return ClassDefinition;
	}

	FGridCharacterInventoryState MakeMON2094Character(URPGClassAsset* ClassDefinition, const TCHAR* Name, const FGuid& CharacterId)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = CharacterId;
		Character.DisplayName = FText::FromString(Name);
		Character.ClassId = ClassDefinition->ClassId;
		Character.ClassDisplayName = ClassDefinition->DisplayName;
		Character.ClassDefinition = ClassDefinition;
		Character.Level = 3;
		return Character;
	}

	UGridPartyInventoryComponent* MakeMON2094Party(int32 CharacterCount = 1)
	{
		UGridPartyInventoryComponent* Party = NewObject<UGridPartyInventoryComponent>();
		Party->PartyInventoryState = FGridPartyInventoryState();
		URPGClassAsset* ClassDefinition = MakeMON2094Class(Party);
		for (int32 Index = 0; Index < CharacterCount; ++Index)
		{
			Party->PartyInventoryState.ActiveCharacters.Add(
				MakeMON2094Character(ClassDefinition, Index == 0 ? TEXT("Elias") : TEXT("Elarion"), FGuid(20, 9, 4, Index + 1)));
		}
		Party->PartyInventoryState.ActiveEquipment.SetNum(CharacterCount);
		Party->PartyInventoryState.SelectedCharacterIndex = 0;
		Party->PartyInventoryState.MaxActiveCharacters = FMath::Max(CharacterCount, 1);
		Party->PartyInventoryState.bInitialCharacterCreationCompleted = true;
		return Party;
	}

	URPGSkillAsset* MakeMON2094Skill(UObject* Outer, FName SkillId = TEXT("Skill_Lockpicking"), int32 MaxRank = 5)
	{
		URPGSkillAsset* Skill = NewObject<URPGSkillAsset>(Outer);
		Skill->SkillId = SkillId;
		Skill->DisplayName = FText::FromString(TEXT("Crochetage"));
		Skill->Description = FText::FromString(TEXT("Compétence restaurée de test."));
		Skill->GoverningAttribute = ERPGSkillGoverningAttribute::Dexterity;
		Skill->MaxRank = MaxRank;
		Skill->bAllowUntrainedChecks = true;
		return Skill;
	}

	FRPGSkillRequirementGrant MakeMON2094Grant(int32 MinimumRank, FName RequirementId)
	{
		FRPGSkillRequirementGrant Grant;
		Grant.MinimumRank = MinimumRank;
		Grant.GrantedRequirementIds.Add(RequirementId);
		return Grant;
	}

	struct FMON2094DurableSkillState
	{
		FGuid CharacterId;
		FName SkillId = NAME_None;
		int32 Rank = 0;
	};

	TArray<FMON2094DurableSkillState> MakeMON2094Snapshot(const FGuid& CharacterId, FName SkillId, int32 Rank)
	{
		FMON2094DurableSkillState State;
		State.CharacterId = CharacterId;
		State.SkillId = SkillId;
		State.Rank = Rank;
		return { State };
	}

	bool RestoreMON2094Skill(
		UGridPartyInventoryComponent* Party, const TArray<FMON2094DurableSkillState>& SavedStates, URPGSkillAsset* Skill, FString& OutError)
	{
		if (!Party || !IsValid(Skill))
		{
			OutError = TEXT("Invalid durable Skill restore fixture.");
			return false;
		}

		FGridPartyInventoryState Candidate = Party->PartyInventoryState;
		for (FGridCharacterInventoryState& Character : Candidate.ActiveCharacters)
		{
			Character.SkillRanks.Reset();
		}
		for (FGridCharacterInventoryState& Character : Candidate.CharacterPool)
		{
			Character.SkillRanks.Reset();
		}

		for (const FMON2094DurableSkillState& Saved : SavedStates)
		{
			FGridCharacterInventoryState* Target = Candidate.ActiveCharacters.FindByPredicate(
				[&Saved](const FGridCharacterInventoryState& Character)
				{
					return Character.CharacterId == Saved.CharacterId;
				});
			if (!Target)
			{
				Target = Candidate.CharacterPool.FindByPredicate(
					[&Saved](const FGridCharacterInventoryState& Character)
					{
						return Character.CharacterId == Saved.CharacterId;
					});
			}
			if (!Target || Saved.SkillId != Skill->SkillId)
			{
				OutError = TEXT("Durable Skill restore fixture cannot resolve the target.");
				return false;
			}

			FRPGSkillMutationResult Mutation;
			if (!FRPGSkillService::TrySetSkillRank(*Target, Skill, Saved.Rank, Mutation))
			{
				OutError = TEXT("Durable Skill restore fixture rejects the rank.");
				return false;
			}
		}

		Party->PartyInventoryState = MoveTemp(Candidate);
		OutError.Reset();
		return true;
	}

	bool SetMON2094Rank(FGridCharacterInventoryState& Character, URPGSkillAsset* Skill, int32 Rank)
	{
		FRPGSkillMutationResult Result;
		return FRPGSkillService::TrySetSkillRank(Character, Skill, Rank, Result);
	}

	bool ProjectMON2094Requirements(const FGridCharacterInventoryState& Character, URPGSkillAsset* Skill, TSet<FName>& OutRequirements, FString& OutError)
	{
		return FRPGSkillRequirementProjectionService::AppendSatisfiedRequirements(
			Character,
			[Skill](FName SkillId) -> const URPGSkillAsset*
			{
				return SkillId == Skill->SkillId ? Skill : nullptr;
			},
			OutRequirements, OutError);
	}

	FGridCombatActionCatalogContext MakeMON2094Context()
	{
		FGridCombatActionCatalogContext Context;
		Context.CharacterIndex = 0;
		Context.CharacterId = FGuid(20, 9, 4, 1);
		Context.bCombatActive = true;
		Context.bActiveCombatant = true;
		Context.bEnableClassActionExecutors = true;
		Context.RemainingActionPoints = 6;
		Context.CurrentHealth = 10;
		Context.MaximumHealth = 10;
		Context.CurrentMana = 10;
		Context.MaximumMana = 10;
		return Context;
	}

	FGridCombatActionContribution MakeMON2094Action(FName ActionId, FName RequirementId)
	{
		FGridCombatActionContribution Contribution;
		Contribution.Definition = FGridCombatActionCatalog::MakeUnarmedAttackDefinition(2);
		Contribution.Definition.ActionId = ActionId;
		Contribution.Definition.OffensiveProfile.AttackId = ActionId;
		Contribution.Definition.Requirements = { RequirementId };
		return Contribution;
	}

	const FGridAvailableCombatAction* BuildMON2094Action(
		const FGridCombatActionCatalogContext& Context, const FGridCombatActionContribution& Contribution, TArray<FGridAvailableCombatAction>& OutActions)
	{
		FGridCombatActionCatalog::Build(Context, { Contribution }, OutActions);
		return OutActions.Num() == 1 ? &OutActions[0] : nullptr;
	}

	const FGridSkillEntryView* FindMON2094SkillView(const FGridSkillsPageView& View, FName SkillId)
	{
		return View.Skills.FindByPredicate(
			[SkillId](const FGridSkillEntryView& Entry)
			{
				return Entry.SkillId == SkillId;
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2094RestoreProjectsSkillIdTest, "Grimrock.MON20.9.RestoredConsumers.RestoreProjectsSkillId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2094RestoreProjectsSkillIdTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2094Tests;
	FRuntimeStateGuard Guard;
	UGridPartyInventoryComponent* Party = MakeMON2094Party();
	URPGSkillAsset* Skill = MakeMON2094Skill(Party);
	const FGuid CharacterId = Party->PartyInventoryState.ActiveCharacters[0].CharacterId;
	FString Error;
	TestTrue(TEXT("Skill snapshot restores"), RestoreMON2094Skill(Party, MakeMON2094Snapshot(CharacterId, Skill->SkillId, 2), Skill, Error));

	TSet<FName> Requirements;
	Requirements.Add(TEXT("Class_Rogue"));
	TestTrue(
		TEXT("Restored rank projects requirements"), ProjectMON2094Requirements(Party->PartyInventoryState.ActiveCharacters[0], Skill, Requirements, Error));
	TestTrue(TEXT("Restored SkillId is satisfied"), Requirements.Contains(Skill->SkillId));
	TestTrue(TEXT("Existing requirements are preserved"), Requirements.Contains(TEXT("Class_Rogue")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2094RestoreProjectsThresholdTest, "Grimrock.MON20.9.RestoredConsumers.RestoreProjectsThresholdRequirement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2094RestoreProjectsThresholdTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2094Tests;
	FRuntimeStateGuard Guard;
	UGridPartyInventoryComponent* Party = MakeMON2094Party();
	URPGSkillAsset* Skill = MakeMON2094Skill(Party);
	Skill->RequirementGrants.Add(MakeMON2094Grant(3, TEXT("Req_Lockpicking_Expert")));
	const FGuid CharacterId = Party->PartyInventoryState.ActiveCharacters[0].CharacterId;
	FString Error;
	TestTrue(TEXT("Threshold rank restores"), RestoreMON2094Skill(Party, MakeMON2094Snapshot(CharacterId, Skill->SkillId, 3), Skill, Error));

	TSet<FName> Requirements;
	TestTrue(TEXT("Restored threshold projects"), ProjectMON2094Requirements(Party->PartyInventoryState.ActiveCharacters[0], Skill, Requirements, Error));
	TestTrue(TEXT("SkillId remains satisfied"), Requirements.Contains(Skill->SkillId));
	TestTrue(TEXT("Threshold RequirementId is restored derivatively"), Requirements.Contains(TEXT("Req_Lockpicking_Expert")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2094RestoreUnlocksActionTest, "Grimrock.MON20.9.RestoredConsumers.RestoreUnlocksAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2094RestoreUnlocksActionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2094Tests;
	FRuntimeStateGuard Guard;
	UGridPartyInventoryComponent* Party = MakeMON2094Party();
	URPGSkillAsset* Skill = MakeMON2094Skill(Party);
	Skill->RequirementGrants.Add(MakeMON2094Grant(2, TEXT("Req_Lockpicking_Advanced")));
	const FGuid CharacterId = Party->PartyInventoryState.ActiveCharacters[0].CharacterId;
	FString Error;
	TestTrue(TEXT("Action-gating rank restores"), RestoreMON2094Skill(Party, MakeMON2094Snapshot(CharacterId, Skill->SkillId, 2), Skill, Error));

	FGridCombatActionCatalogContext Context = MakeMON2094Context();
	TestTrue(TEXT("Restored requirements project into action context"),
		ProjectMON2094Requirements(Party->PartyInventoryState.ActiveCharacters[0], Skill, Context.SatisfiedRequirements, Error));
	TArray<FGridAvailableCombatAction> Actions;
	const FGridAvailableCombatAction* Action =
		BuildMON2094Action(Context, MakeMON2094Action(TEXT("Action_RestoredLockpick"), TEXT("Req_Lockpicking_Advanced")), Actions);
	TestNotNull(TEXT("Action is present"), Action);
	if (!Action)
	{
		return false;
	}
	TestTrue(TEXT("Restored Skill threshold unlocks action"), Action->bEnabled);
	TestTrue(TEXT("Unlocked action exposes no missing requirement"), Action->MissingRequirements.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2094BelowThresholdStaysLockedTest, "Grimrock.MON20.9.RestoredConsumers.RestoreBelowThresholdKeepsActionLocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2094BelowThresholdStaysLockedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2094Tests;
	FRuntimeStateGuard Guard;
	UGridPartyInventoryComponent* Party = MakeMON2094Party();
	URPGSkillAsset* Skill = MakeMON2094Skill(Party);
	Skill->RequirementGrants.Add(MakeMON2094Grant(2, TEXT("Req_Lockpicking_Advanced")));
	const FGuid CharacterId = Party->PartyInventoryState.ActiveCharacters[0].CharacterId;
	FString Error;
	TestTrue(TEXT("Below-threshold rank restores"), RestoreMON2094Skill(Party, MakeMON2094Snapshot(CharacterId, Skill->SkillId, 1), Skill, Error));

	FGridCombatActionCatalogContext Context = MakeMON2094Context();
	TestTrue(TEXT("Rank one projection succeeds"),
		ProjectMON2094Requirements(Party->PartyInventoryState.ActiveCharacters[0], Skill, Context.SatisfiedRequirements, Error));
	TestTrue(TEXT("Training SkillId is satisfied"), Context.SatisfiedRequirements.Contains(Skill->SkillId));
	TestFalse(TEXT("Unreached threshold is not over-granted"), Context.SatisfiedRequirements.Contains(TEXT("Req_Lockpicking_Advanced")));

	TArray<FGridAvailableCombatAction> Actions;
	const FGridAvailableCombatAction* Action =
		BuildMON2094Action(Context, MakeMON2094Action(TEXT("Action_StillLocked"), TEXT("Req_Lockpicking_Advanced")), Actions);
	TestNotNull(TEXT("Locked action remains inspectable"), Action);
	if (!Action)
	{
		return false;
	}
	TestFalse(TEXT("Below-threshold restore keeps action locked"), Action->bEnabled);
	TestTrue(TEXT("Missing threshold remains diagnosed"), Action->MissingRequirements.Contains(TEXT("Req_Lockpicking_Advanced")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2094RestoreSkillsPageRankTest, "Grimrock.MON20.9.RestoredConsumers.RestoreUpdatesSkillsPage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2094RestoreSkillsPageRankTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2094Tests;
	FRuntimeStateGuard Guard;
	UGridPartyInventoryComponent* Party = MakeMON2094Party();
	URPGSkillAsset* Skill = MakeMON2094Skill(Party);
	const FGuid CharacterId = Party->PartyInventoryState.ActiveCharacters[0].CharacterId;
	FString Error;
	TestTrue(TEXT("UI rank restores"), RestoreMON2094Skill(Party, MakeMON2094Snapshot(CharacterId, Skill->SkillId, 4), Skill, Error));

	FGridSkillsPageView View;
	TestTrue(TEXT("Skills page builds after restore"), FGridSkillsPageService::TryBuildCharacterView(Party, 0, { Skill }, View));
	const FGridSkillEntryView* SkillView = FindMON2094SkillView(View, Skill->SkillId);
	TestNotNull(TEXT("Restored Skill is visible"), SkillView);
	if (!SkillView)
	{
		return false;
	}
	TestEqual(TEXT("Page shows restored rank"), SkillView->Rank, 4);
	TestTrue(TEXT("Page marks restored Skill trained"), SkillView->bTrained);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2094SelectedCharacterPageTest, "Grimrock.MON20.9.RestoredConsumers.RestoreUpdatesSelectedCharacterPage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2094SelectedCharacterPageTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2094Tests;
	FRuntimeStateGuard Guard;
	UGridPartyInventoryComponent* Party = MakeMON2094Party(2);
	URPGSkillAsset* Skill = MakeMON2094Skill(Party);
	TestTrue(TEXT("Second character selection succeeds"), Party->SetSelectedCharacterIndex(1));
	const FGuid CharacterId = Party->PartyInventoryState.ActiveCharacters[1].CharacterId;
	FString Error;
	TestTrue(TEXT("Selected character rank restores"), RestoreMON2094Skill(Party, MakeMON2094Snapshot(CharacterId, Skill->SkillId, 3), Skill, Error));

	FGridSkillsPageView View;
	TestTrue(TEXT("Selected-character page builds after restore"), FGridSkillsPageService::TryBuildSelectedCharacterView(Party, { Skill }, View));
	TestEqual(TEXT("Selected character remains authoritative"), View.CharacterIndex, 1);
	TestTrue(TEXT("Selected CharacterId is preserved"), View.CharacterId == CharacterId);
	const FGridSkillEntryView* SkillView = FindMON2094SkillView(View, Skill->SkillId);
	TestNotNull(TEXT("Selected restored Skill is visible"), SkillView);
	if (SkillView)
	{
		TestEqual(TEXT("Selected page shows restored rank"), SkillView->Rank, 3);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2094EmptySnapshotClearsConsumersTest, "Grimrock.MON20.9.RestoredConsumers.EmptySnapshotClearsTransientConsumers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2094EmptySnapshotClearsConsumersTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2094Tests;
	FRuntimeStateGuard Guard;
	UGridPartyInventoryComponent* Party = MakeMON2094Party();
	URPGSkillAsset* Skill = MakeMON2094Skill(Party);
	TestTrue(TEXT("Stale transient rank setup succeeds"), SetMON2094Rank(Party->PartyInventoryState.ActiveCharacters[0], Skill, 3));

	FString Error;
	const TArray<FMON2094DurableSkillState> EmptySnapshot;
	TestTrue(TEXT("Authoritative empty snapshot restores"), RestoreMON2094Skill(Party, EmptySnapshot, Skill, Error));
	TestEqual(TEXT("Transient rank is cleared"), FRPGSkillService::GetSkillRank(Party->PartyInventoryState.ActiveCharacters[0], Skill->SkillId), 0);

	TSet<FName> Requirements;
	TestTrue(TEXT("Zero-rank projection succeeds"), ProjectMON2094Requirements(Party->PartyInventoryState.ActiveCharacters[0], Skill, Requirements, Error));
	TestFalse(TEXT("Cleared Skill grants no SkillId requirement"), Requirements.Contains(Skill->SkillId));

	FGridSkillsPageView View;
	TestTrue(TEXT("Skills page builds after empty restore"), FGridSkillsPageService::TryBuildCharacterView(Party, 0, { Skill }, View));
	const FGridSkillEntryView* SkillView = FindMON2094SkillView(View, Skill->SkillId);
	TestNotNull(TEXT("Canonical Skill remains visible untrained"), SkillView);
	if (SkillView)
	{
		TestEqual(TEXT("Page shows zero after empty restore"), SkillView->Rank, 0);
		TestFalse(TEXT("Page marks Skill untrained"), SkillView->bTrained);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON2094InvalidRestorePreservesConsumersTest, "Grimrock.MON20.9.RestoredConsumers.InvalidRestorePreservesConsumers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2094InvalidRestorePreservesConsumersTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGMON2094Tests;
	FRuntimeStateGuard Guard;
	UGridPartyInventoryComponent* Party = MakeMON2094Party();
	URPGSkillAsset* Skill = MakeMON2094Skill(Party, TEXT("Skill_Lockpicking"), 5);
	TestTrue(TEXT("Existing valid rank setup succeeds"), SetMON2094Rank(Party->PartyInventoryState.ActiveCharacters[0], Skill, 2));
	const FGuid CharacterId = Party->PartyInventoryState.ActiveCharacters[0].CharacterId;

	FString Error;
	TestFalse(TEXT("Rank beyond MaxRank restore is rejected"), RestoreMON2094Skill(Party, MakeMON2094Snapshot(CharacterId, Skill->SkillId, 6), Skill, Error));
	TestEqual(TEXT("Atomic rejection preserves previous runtime rank"),
		FRPGSkillService::GetSkillRank(Party->PartyInventoryState.ActiveCharacters[0], Skill->SkillId), 2);

	TSet<FName> Requirements;
	TestTrue(
		TEXT("Existing valid rank still projects"), ProjectMON2094Requirements(Party->PartyInventoryState.ActiveCharacters[0], Skill, Requirements, Error));
	TestTrue(TEXT("Existing Skill consumer remains available"), Requirements.Contains(Skill->SkillId));

	FGridSkillsPageView View;
	TestTrue(TEXT("Skills page remains buildable after rejected restore"), FGridSkillsPageService::TryBuildCharacterView(Party, 0, { Skill }, View));
	const FGridSkillEntryView* SkillView = FindMON2094SkillView(View, Skill->SkillId);
	TestNotNull(TEXT("Skill view remains present"), SkillView);
	if (SkillView)
	{
		TestEqual(TEXT("Page preserves previous rank"), SkillView->Rank, 2);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

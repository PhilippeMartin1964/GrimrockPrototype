#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridSkillsPageService.h"

namespace RPGUIRPG013Tests
{
	const FName AlphaBranch(TEXT("Alpha"));
	const FName BetaBranch(TEXT("Beta"));
	const FName GammaBranch(TEXT("Gamma"));
	const FName BetaAffinityNode(TEXT("Talent_Beta_Affinity"));
	const FName BetaAffinityGroup(TEXT("TalentGroup_Beta_Affinity"));
	const FName BetaFire(TEXT("Talent_Beta_Affinity_Fire"));
	const FName BetaIce(TEXT("Talent_Beta_Affinity_Ice"));

	struct FRuntimeStateGuard
	{
		FRuntimeStateGuard() { FRPGClassProgressionTransactionService::ResetRuntimeState(); }
		~FRuntimeStateGuard() { FRPGClassProgressionTransactionService::ResetRuntimeState(); }
	};

	FRPGClassProgressionChoiceDefinition MakeSimpleChoice(FName ChoiceId, FName BranchId, int32 MinimumLevel, FName Prerequisite = NAME_None)
	{
		FRPGClassProgressionChoiceDefinition Choice;
		Choice.ChoiceId = ChoiceId;
		Choice.TalentBranchId = BranchId;
		Choice.TalentNodeId = ChoiceId;
		Choice.DisplayName = FText::FromName(ChoiceId);
		Choice.Description = FText::FromString(TEXT("UI-RPG01.3 test talent."));
		Choice.MinimumLevel = MinimumLevel;
		Choice.PointCost = 1;
		if (!Prerequisite.IsNone()) Choice.PrerequisiteChoiceIds.Add(Prerequisite);
		return Choice;
	}

	void AddSimpleBranch(URPGClassAsset& ClassDefinition, FName BranchId, const TCHAR* Prefix)
	{
		const int32 Levels[] = { 2, 6, 10, 14, 18 };
		FName Previous = NAME_None;
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Levels); ++Index)
		{
			const FName ChoiceId(*FString::Printf(TEXT("%s_%d"), Prefix, Index + 1));
			ClassDefinition.ProgressionChoices.Add(MakeSimpleChoice(ChoiceId, BranchId, Levels[Index], Previous));
			Previous = ChoiceId;
		}
	}

	URPGClassAsset* MakeClass(UObject* Outer, int32 GrantedPoints = 10)
	{
		URPGClassAsset* ClassDefinition = NewObject<URPGClassAsset>(Outer);
		ClassDefinition->ClassId = TEXT("UIRPG013_Class");
		ClassDefinition->DisplayName = FText::FromString(TEXT("Classe UI-RPG01.3"));
		ClassDefinition->HealthAtLevelOne = 10;
		FRPGClassProgressionLevelGrant Grant;
		Grant.Level = 2;
		Grant.ChoicePointsGranted = GrantedPoints;
		ClassDefinition->ProgressionLevelGrants.Add(Grant);

		AddSimpleBranch(*ClassDefinition, AlphaBranch, TEXT("Talent_Alpha"));

		FRPGClassProgressionChoiceDefinition Fire;
		Fire.ChoiceId = BetaFire;
		Fire.TalentBranchId = BetaBranch;
		Fire.TalentNodeId = BetaAffinityNode;
		Fire.DisplayName = FText::FromString(TEXT("Affinité — Feu"));
		Fire.Description = FText::FromString(TEXT("Variante Feu."));
		Fire.MinimumLevel = 2;
		Fire.PointCost = 1;
		Fire.ExclusiveChoiceGroupId = BetaAffinityGroup;
		Fire.GrantedRequirementIds = { BetaAffinityNode };
		ClassDefinition->ProgressionChoices.Add(Fire);

		FRPGClassProgressionChoiceDefinition Ice = Fire;
		Ice.ChoiceId = BetaIce;
		Ice.DisplayName = FText::FromString(TEXT("Affinité — Glace"));
		Ice.Description = FText::FromString(TEXT("Variante Glace."));
		ClassDefinition->ProgressionChoices.Add(Ice);

		const int32 TailLevels[] = { 6, 10, 14, 18 };
		FName PreviousChoice = NAME_None;
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(TailLevels); ++Index)
		{
			const FName ChoiceId(*FString::Printf(TEXT("Talent_Beta_%d"), Index + 2));
			FRPGClassProgressionChoiceDefinition Choice = MakeSimpleChoice(ChoiceId, BetaBranch, TailLevels[Index], PreviousChoice);
			if (Index == 0)
			{
				Choice.PrerequisiteChoiceIds.Reset();
				Choice.PrerequisiteRequirementIds = { BetaAffinityNode };
			}
			ClassDefinition->ProgressionChoices.Add(Choice);
			PreviousChoice = ChoiceId;
		}
		AddSimpleBranch(*ClassDefinition, GammaBranch, TEXT("Talent_Gamma"));
		return ClassDefinition;
	}

	UGridPartyInventoryComponent* MakeParty(URPGClassAsset* ClassDefinition, int32 Level = 10)
	{
		UGridPartyInventoryComponent* Party = NewObject<UGridPartyInventoryComponent>();
		Party->PartyInventoryState = FGridPartyInventoryState();
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.DisplayName = FText::FromString(TEXT("Elias"));
		Character.ClassId = ClassDefinition->ClassId;
		Character.ClassDisplayName = ClassDefinition->DisplayName;
		Character.ClassDefinition = ClassDefinition;
		Character.Level = Level;
		Party->PartyInventoryState.ActiveCharacters.Add(Character);
		Party->PartyInventoryState.ActiveEquipment.SetNum(1);
		Party->PartyInventoryState.SelectedCharacterIndex = 0;
		return Party;
	}

	const FGridTalentBranchView* FindBranch(const FGridTalentTreeView& Tree, FName BranchId)
	{
		return Tree.Branches.FindByPredicate([BranchId](const FGridTalentBranchView& Branch) { return Branch.TalentBranchId == BranchId; });
	}
	const FGridTalentNodeView* FindNode(const FGridTalentBranchView* Branch, FName NodeId)
	{
		return Branch ? Branch->Nodes.FindByPredicate([NodeId](const FGridTalentNodeView& Node) { return Node.TalentNodeId == NodeId; }) : nullptr;
	}
	const FGridTalentVariantView* FindVariant(const FGridTalentNodeView* Node, FName ChoiceId)
	{
		return Node ? Node->Variants.FindByPredicate([ChoiceId](const FGridTalentVariantView& Variant) { return Variant.ChoiceId == ChoiceId; }) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGUIRPG013TreeShapeTest, "Grimrock.UI.RPG01.ReadModel.TreeShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGUIRPG013TreeShapeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGUIRPG013Tests;
	FRuntimeStateGuard Guard;
	UGridPartyInventoryComponent* Outer = NewObject<UGridPartyInventoryComponent>();
	URPGClassAsset* ClassDefinition = MakeClass(Outer);
	UGridPartyInventoryComponent* Party = MakeParty(ClassDefinition);
	FGridSkillsPageView View;
	TestTrue(TEXT("Talent tree view builds"), FGridSkillsPageService::TryBuildCharacterView(Party, 0, {}, View));
	TestEqual(TEXT("Character level is projected"), View.CharacterLevel, 10);
	TestEqual(TEXT("ClassId is projected"), View.ClassId, ClassDefinition->ClassId);
	TestEqual(TEXT("Three structural branches are projected"), View.TalentTree.Branches.Num(), 3);
	if (View.TalentTree.Branches.Num() == 3)
	{
		TestEqual(TEXT("Branch ordering is deterministic"), View.TalentTree.Branches[0].TalentBranchId, AlphaBranch);
		TestEqual(TEXT("Second branch is Beta"), View.TalentTree.Branches[1].TalentBranchId, BetaBranch);
		TestEqual(TEXT("Third branch is Gamma"), View.TalentTree.Branches[2].TalentBranchId, GammaBranch);
	}
	const FGridTalentBranchView* Alpha = FindBranch(View.TalentTree, AlphaBranch);
	const FGridTalentBranchView* Beta = FindBranch(View.TalentTree, BetaBranch);
	TestNotNull(TEXT("Alpha branch exists"), Alpha);
	TestNotNull(TEXT("Beta branch exists"), Beta);
	if (Alpha)
	{
		TestEqual(TEXT("Alpha exposes five conceptual nodes"), Alpha->Nodes.Num(), 5);
		if (Alpha->Nodes.Num() == 5)
		{
			TestEqual(TEXT("Tier I is derived from level 2"), Alpha->Nodes[0].Tier, 1);
			TestEqual(TEXT("Tier V is derived from level 18"), Alpha->Nodes[4].Tier, 5);
			TestEqual(TEXT("Tier II points to Tier I"), Alpha->Nodes[1].PreviousNodeId, Alpha->Nodes[0].TalentNodeId);
			TestEqual(TEXT("Tier I is available"), Alpha->Nodes[0].State, EGridTalentNodeState::Available);
			TestEqual(TEXT("Tier II is prerequisite-locked"), Alpha->Nodes[1].State, EGridTalentNodeState::LockedPrerequisite);
			TestEqual(TEXT("Tier IV is level-locked at character level 10"), Alpha->Nodes[3].State, EGridTalentNodeState::LockedLevel);
		}
	}
	if (Beta)
	{
		TestEqual(TEXT("Beta exposes five conceptual nodes despite six ChoiceIds"), Beta->Nodes.Num(), 5);
		const FGridTalentNodeView* Affinity = FindNode(Beta, BetaAffinityNode);
		TestNotNull(TEXT("Variant node exists"), Affinity);
		if (Affinity) TestEqual(TEXT("Two ChoiceIds are grouped into one conceptual node"), Affinity->Variants.Num(), 2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGUIRPG013VariantSelectionTest, "Grimrock.UI.RPG01.ReadModel.VariantSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGUIRPG013VariantSelectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGUIRPG013Tests;
	FRuntimeStateGuard Guard;
	UGridPartyInventoryComponent* Outer = NewObject<UGridPartyInventoryComponent>();
	URPGClassAsset* ClassDefinition = MakeClass(Outer);
	UGridPartyInventoryComponent* Party = MakeParty(ClassDefinition);
	FRPGClassProgressionCommitResult Result;
	TestTrue(TEXT("Variant commit succeeds"), FRPGClassProgressionTransactionService::TryCommitChoices(Party, 0, { BetaFire }, Result));
	FGridSkillsPageView View;
	TestTrue(TEXT("Tree rebuilds after variant acquisition"), FGridSkillsPageService::TryBuildCharacterView(Party, 0, {}, View));
	const FGridTalentBranchView* Beta = FindBranch(View.TalentTree, BetaBranch);
	const FGridTalentNodeView* Affinity = FindNode(Beta, BetaAffinityNode);
	TestNotNull(TEXT("Acquired variant node exists"), Affinity);
	if (!Affinity) return false;
	TestEqual(TEXT("Conceptual node is acquired"), Affinity->State, EGridTalentNodeState::Acquired);
	TestEqual(TEXT("Concrete selected ChoiceId remains authoritative"), Affinity->SelectedChoiceId, BetaFire);
	const FGridTalentVariantView* Fire = FindVariant(Affinity, BetaFire);
	const FGridTalentVariantView* Ice = FindVariant(Affinity, BetaIce);
	TestNotNull(TEXT("Fire variant exists"), Fire);
	TestNotNull(TEXT("Ice variant exists"), Ice);
	if (Fire)
	{
		TestTrue(TEXT("Selected variant is marked acquired"), Fire->bAcquired);
		TestEqual(TEXT("Selected variant is acquired"), Fire->State, EGridTalentNodeState::Acquired);
	}
	if (Ice)
	{
		TestFalse(TEXT("Sibling variant is not acquired"), Ice->bAcquired);
		TestEqual(TEXT("Sibling variant is exclusivity-locked"), Ice->State, EGridTalentNodeState::LockedExclusive);
	}
	if (Beta && Beta->Nodes.Num() >= 2) TestEqual(TEXT("Next node is unlocked by logical requirement alias"), Beta->Nodes[1].State, EGridTalentNodeState::Available);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGUIRPG013PointsLockTest, "Grimrock.UI.RPG01.ReadModel.PointsLock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGUIRPG013PointsLockTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGUIRPG013Tests;
	FRuntimeStateGuard Guard;
	UGridPartyInventoryComponent* Outer = NewObject<UGridPartyInventoryComponent>();
	URPGClassAsset* ClassDefinition = MakeClass(Outer, 1);
	UGridPartyInventoryComponent* Party = MakeParty(ClassDefinition);
	const FName AlphaOne(TEXT("Talent_Alpha_1"));
	FRPGClassProgressionCommitResult Result;
	TestTrue(TEXT("First Alpha node commit succeeds"), FRPGClassProgressionTransactionService::TryCommitChoices(Party, 0, { AlphaOne }, Result));
	FGridSkillsPageView View;
	TestTrue(TEXT("Tree rebuilds with empty Talent budget"), FGridSkillsPageService::TryBuildCharacterView(Party, 0, {}, View));
	const FGridTalentBranchView* Alpha = FindBranch(View.TalentTree, AlphaBranch);
	TestNotNull(TEXT("Alpha branch exists"), Alpha);
	if (Alpha && Alpha->Nodes.Num() >= 2)
	{
		TestEqual(TEXT("Acquired Tier I is projected"), Alpha->Nodes[0].State, EGridTalentNodeState::Acquired);
		TestEqual(TEXT("Satisfied Tier II is points-locked"), Alpha->Nodes[1].State, EGridTalentNodeState::LockedPoints);
	}
	TestEqual(TEXT("Read model reuses authoritative remaining balance"), View.RemainingTalentPoints, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGUIRPG013PartialMetadataAtomicTest, "Grimrock.UI.RPG01.ReadModel.PartialMetadataAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGUIRPG013PartialMetadataAtomicTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGUIRPG013Tests;
	FRuntimeStateGuard Guard;
	UGridPartyInventoryComponent* Outer = NewObject<UGridPartyInventoryComponent>();
	URPGClassAsset* ClassDefinition = MakeClass(Outer);
	ClassDefinition->ProgressionChoices[0].TalentNodeId = NAME_None;
	UGridPartyInventoryComponent* Party = MakeParty(ClassDefinition);
	FGridSkillsPageView View;
	View.CharacterIndex = 99;
	TestFalse(TEXT("Partial structural metadata is rejected atomically"), FGridSkillsPageService::TryBuildCharacterView(Party, 0, {}, View));
	TestFalse(TEXT("Failed read model is reset"), View.IsValid());
	TestTrue(TEXT("Failed tree exposes no partial branches"), View.TalentTree.Branches.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGUIRPG013LegacyMetadataBridgeTest, "Grimrock.UI.RPG01.ReadModel.PreMaterializationBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGUIRPG013LegacyMetadataBridgeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPGUIRPG013Tests;
	FRuntimeStateGuard Guard;
	UGridPartyInventoryComponent* Outer = NewObject<UGridPartyInventoryComponent>();
	URPGClassAsset* ClassDefinition = MakeClass(Outer);
	for (FRPGClassProgressionChoiceDefinition& Choice : ClassDefinition->ProgressionChoices)
	{
		Choice.TalentBranchId = NAME_None;
		Choice.TalentNodeId = NAME_None;
	}
	UGridPartyInventoryComponent* Party = MakeParty(ClassDefinition);
	FGridSkillsPageView View;
	TestTrue(TEXT("Pre-materialization assets keep the existing flat Skills page"), FGridSkillsPageService::TryBuildCharacterView(Party, 0, {}, View));
	TestEqual(TEXT("Class identity is still projected"), View.TalentTree.ClassId, ClassDefinition->ClassId);
	TestTrue(TEXT("Tree stays empty until structural metadata is materialized"), View.TalentTree.Branches.IsEmpty());
	return true;
}

#endif

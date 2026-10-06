#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridSkillsPageService.h"

namespace UIRPG014Production
{
	struct FClassSpec
	{
		FName ClassId = NAME_None;
		const TCHAR* ObjectPath = nullptr;
	};

	const FClassSpec Specs[] = {
		{ TEXT("Warrior"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Warrior.DA_Class_Warrior") },
		{ TEXT("Rogue"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Rogue.DA_Class_Rogue") },
		{ TEXT("Ranger"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.DA_Class_Ranger") },
		{ TEXT("Mage"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Mage.DA_Class_Mage") },
		{ TEXT("Priest"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Priest.DA_Class_Priest") },
		{ TEXT("Alchemist"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Alchemist.DA_Class_Alchemist") }
	};

	using FChoiceArray = TArray<const FRPGClassProgressionChoiceDefinition*>;

	TArray<URPGClassAsset*> LoadProductionClasses(FAutomationTestBase& Test)
	{
		TArray<URPGClassAsset*> Classes;
		for (const FClassSpec& Spec : Specs)
		{
			URPGClassAsset* Asset = LoadObject<URPGClassAsset>(nullptr, Spec.ObjectPath);
			Test.TestNotNull(*FString::Printf(TEXT("Production class %s loads"), *Spec.ClassId.ToString()), Asset);
			if (!Asset) continue;
			Test.TestEqual(*FString::Printf(TEXT("%s keeps its ClassId"), *Spec.ClassId.ToString()), Asset->ClassId, Spec.ClassId);
			Test.TestTrue(*FString::Printf(TEXT("%s remains a valid RPG class"), *Spec.ClassId.ToString()), Asset->IsValidDefinition());
			Classes.Add(Asset);
		}
		return Classes;
	}

	void GroupChoices(const URPGClassAsset& ClassAsset, TMap<FName, TMap<FName, FChoiceArray>>& OutByBranch)
	{
		OutByBranch.Reset();
		for (const FRPGClassProgressionChoiceDefinition& Choice : ClassAsset.ProgressionChoices)
		{
			OutByBranch.FindOrAdd(Choice.TalentBranchId).FindOrAdd(Choice.TalentNodeId).Add(&Choice);
		}
	}

	void BuildSatisfiedIds(const FRPGClassProgressionChoiceDefinition& Choice, TSet<FName>& OutIds)
	{
		OutIds.Reset();
		OutIds.Add(Choice.ChoiceId);
		for (const FName RequirementId : Choice.GrantedRequirementIds) OutIds.Add(RequirementId);
	}

	bool BuildCommonSatisfiedIds(const FChoiceArray& Choices, TSet<FName>& OutIds)
	{
		OutIds.Reset();
		if (Choices.IsEmpty() || !Choices[0]) return false;
		BuildSatisfiedIds(*Choices[0], OutIds);
		for (int32 Index = 1; Index < Choices.Num(); ++Index)
		{
			if (!Choices[Index]) return false;
			TSet<FName> Current;
			BuildSatisfiedIds(*Choices[Index], Current);
			TArray<FName> Remove;
			for (const FName Id : OutIds)
			{
				if (!Current.Contains(Id)) Remove.Add(Id);
			}
			for (const FName Id : Remove) OutIds.Remove(Id);
		}
		return !OutIds.IsEmpty();
	}

	bool NodeDependsOnPrevious(const FChoiceArray& Current, const FChoiceArray& Previous)
	{
		TSet<FName> PreviousIds;
		if (!BuildCommonSatisfiedIds(Previous, PreviousIds)) return false;
		for (const FRPGClassProgressionChoiceDefinition* Choice : Current)
		{
			if (!Choice) return false;
			bool bDepends = false;
			for (const FName Id : Choice->PrerequisiteChoiceIds) bDepends |= PreviousIds.Contains(Id);
			for (const FName Id : Choice->PrerequisiteRequirementIds) bDepends |= PreviousIds.Contains(Id);
			if (!bDepends) return false;
		}
		return true;
	}

	FChoiceArray FindNodeChoices(const URPGClassAsset& ClassAsset, FName NodeId)
	{
		FChoiceArray Result;
		for (const FRPGClassProgressionChoiceDefinition& Choice : ClassAsset.ProgressionChoices)
		{
			if (Choice.TalentNodeId == NodeId) Result.Add(&Choice);
		}
		return Result;
	}

	void ValidateVariantNode(FAutomationTestBase& Test, const URPGClassAsset& ClassAsset,
		FName NodeId, int32 ExactVariantCount, bool bAllowAnyPositiveCount = false)
	{
		const FChoiceArray Choices = FindNodeChoices(ClassAsset, NodeId);
		if (bAllowAnyPositiveCount)
		{
			Test.TestTrue(*FString::Printf(TEXT("%s node %s exposes at least one production variant"),
				*ClassAsset.ClassId.ToString(), *NodeId.ToString()), Choices.Num() > 0);
		}
		else
		{
			Test.TestEqual(*FString::Printf(TEXT("%s node %s exposes the expected variants"),
				*ClassAsset.ClassId.ToString(), *NodeId.ToString()), Choices.Num(), ExactVariantCount);
		}

		FName GroupId = NAME_None;
		for (const FRPGClassProgressionChoiceDefinition* Choice : Choices)
		{
			if (!Choice) continue;
			Test.TestEqual(TEXT("Variant Choice uses the requested conceptual TalentNodeId"), Choice->TalentNodeId, NodeId);
			Test.TestTrue(TEXT("Variant grants the conceptual node requirement alias"), Choice->GrantedRequirementIds.Contains(NodeId));
			Test.TestFalse(TEXT("Variant uses a non-empty exclusive group"), Choice->ExclusiveChoiceGroupId.IsNone());
			if (GroupId.IsNone()) GroupId = Choice->ExclusiveChoiceGroupId;
			else Test.TestEqual(TEXT("All variants share one exclusive group"), Choice->ExclusiveChoiceGroupId, GroupId);
		}
	}

	URPGClassAsset* FindClass(const TArray<URPGClassAsset*>& Classes, FName ClassId)
	{
		URPGClassAsset* const* Found = Classes.FindByPredicate(
			[ClassId](const URPGClassAsset* Asset) { return Asset && Asset->ClassId == ClassId; });
		return Found ? *Found : nullptr;
	}

	UGridPartyInventoryComponent* MakeParty(URPGClassAsset* ClassAsset)
	{
		UGridPartyInventoryComponent* Party = NewObject<UGridPartyInventoryComponent>();
		Party->PartyInventoryState = FGridPartyInventoryState();
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.DisplayName = FText::FromName(ClassAsset->ClassId);
		Character.ClassId = ClassAsset->ClassId;
		Character.ClassDisplayName = ClassAsset->DisplayName;
		Character.ClassDefinition = ClassAsset;
		Character.Level = 20;
		Party->PartyInventoryState.ActiveCharacters.Add(Character);
		Party->PartyInventoryState.ActiveEquipment.AddDefaulted();
		Party->PartyInventoryState.SelectedCharacterIndex = 0;
		return Party;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG014SixClassesMetadataTest,
	"Grimrock.UI.RPG01.ProductionAssets.SixClassesAndMetadata",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUIRPG014SixClassesMetadataTest::RunTest(const FString&)
{
	using namespace UIRPG014Production;
	const TArray<URPGClassAsset*> Classes = LoadProductionClasses(*this);
	TestEqual(TEXT("Exactly six production classes load"), Classes.Num(), 6);
	for (const URPGClassAsset* ClassAsset : Classes)
	{
		if (!ClassAsset) continue;
		for (const FRPGClassProgressionChoiceDefinition& Choice : ClassAsset->ProgressionChoices)
		{
			TestFalse(*FString::Printf(TEXT("%s/%s has TalentBranchId"),
				*ClassAsset->ClassId.ToString(), *Choice.ChoiceId.ToString()), Choice.TalentBranchId.IsNone());
			TestFalse(*FString::Printf(TEXT("%s/%s has TalentNodeId"),
				*ClassAsset->ClassId.ToString(), *Choice.ChoiceId.ToString()), Choice.TalentNodeId.IsNone());
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG014GlobalShapeTest,
	"Grimrock.UI.RPG01.ProductionAssets.EighteenBranchesNinetyNodes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUIRPG014GlobalShapeTest::RunTest(const FString&)
{
	using namespace UIRPG014Production;
	int32 TotalBranches = 0;
	int32 TotalNodes = 0;
	for (const URPGClassAsset* ClassAsset : LoadProductionClasses(*this))
	{
		if (!ClassAsset) continue;
		TMap<FName, TMap<FName, FChoiceArray>> ByBranch;
		GroupChoices(*ClassAsset, ByBranch);
		TestEqual(*FString::Printf(TEXT("%s exposes exactly three Talent branches"), *ClassAsset->ClassId.ToString()), ByBranch.Num(), 3);
		TotalBranches += ByBranch.Num();

		int32 ClassNodes = 0;
		for (const TPair<FName, TMap<FName, FChoiceArray>>& Branch : ByBranch)
		{
			TestFalse(TEXT("Production branch id is not empty"), Branch.Key.IsNone());
			TestEqual(*FString::Printf(TEXT("%s/%s exposes exactly five conceptual nodes"),
				*ClassAsset->ClassId.ToString(), *Branch.Key.ToString()), Branch.Value.Num(), 5);
			ClassNodes += Branch.Value.Num();
		}
		TestEqual(*FString::Printf(TEXT("%s exposes fifteen conceptual Talent nodes"), *ClassAsset->ClassId.ToString()), ClassNodes, 15);
		TotalNodes += ClassNodes;
	}
	TestEqual(TEXT("Six classes expose eighteen branches"), TotalBranches, 18);
	TestEqual(TEXT("Six classes expose ninety conceptual Talent nodes"), TotalNodes, 90);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG014TierChainsTest,
	"Grimrock.UI.RPG01.ProductionAssets.CanonicalTierChains",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUIRPG014TierChainsTest::RunTest(const FString&)
{
	using namespace UIRPG014Production;
	const int32 Levels[] = { 2, 6, 10, 14, 18 };

	for (const URPGClassAsset* ClassAsset : LoadProductionClasses(*this))
	{
		if (!ClassAsset) continue;
		TMap<FName, TMap<FName, FChoiceArray>> ByBranch;
		GroupChoices(*ClassAsset, ByBranch);

		for (const TPair<FName, TMap<FName, FChoiceArray>>& Branch : ByBranch)
		{
			TMap<int32, FName> NodeByLevel;
			for (const TPair<FName, FChoiceArray>& Node : Branch.Value)
			{
				TestTrue(TEXT("Conceptual node contains at least one ChoiceId"), !Node.Value.IsEmpty());
				if (Node.Value.IsEmpty() || !Node.Value[0]) continue;
				const int32 MinimumLevel = Node.Value[0]->MinimumLevel;
				for (const FRPGClassProgressionChoiceDefinition* Choice : Node.Value)
				{
					if (!Choice) continue;
					TestEqual(TEXT("All variants in one node share MinimumLevel"), Choice->MinimumLevel, MinimumLevel);
					TestEqual(TEXT("All variants in one node share PointCost"), Choice->PointCost, Node.Value[0]->PointCost);
				}
				TestFalse(TEXT("A branch has only one conceptual node per tier"), NodeByLevel.Contains(MinimumLevel));
				NodeByLevel.Add(MinimumLevel, Node.Key);
			}

			TestEqual(TEXT("A production branch spans five canonical tiers"), NodeByLevel.Num(), 5);
			for (int32 Index = 0; Index < UE_ARRAY_COUNT(Levels); ++Index)
			{
				const FName* NodeId = NodeByLevel.Find(Levels[Index]);
				TestNotNull(*FString::Printf(TEXT("%s/%s has a node at level %d"),
					*ClassAsset->ClassId.ToString(), *Branch.Key.ToString(), Levels[Index]), NodeId);
				if (Index == 0 || !NodeId) continue;

				const FName* PreviousNodeId = NodeByLevel.Find(Levels[Index - 1]);
				if (!PreviousNodeId) continue;
				const FChoiceArray* Current = Branch.Value.Find(*NodeId);
				const FChoiceArray* Previous = Branch.Value.Find(*PreviousNodeId);
				TestTrue(*FString::Printf(TEXT("%s/%s level %d depends on the preceding conceptual node"),
					*ClassAsset->ClassId.ToString(), *Branch.Key.ToString(), Levels[Index]),
					Current && Previous && NodeDependsOnPrevious(*Current, *Previous));
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG014VariantGroupingTest,
	"Grimrock.UI.RPG01.ProductionAssets.VariantGrouping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUIRPG014VariantGroupingTest::RunTest(const FString&)
{
	using namespace UIRPG014Production;
	const TArray<URPGClassAsset*> Classes = LoadProductionClasses(*this);
	if (Classes.Num() != 6) return false;

	URPGClassAsset* Warrior = FindClass(Classes, TEXT("Warrior"));
	URPGClassAsset* Ranger = FindClass(Classes, TEXT("Ranger"));
	URPGClassAsset* Mage = FindClass(Classes, TEXT("Mage"));
	TestNotNull(TEXT("Warrior production class is available"), Warrior);
	TestNotNull(TEXT("Ranger production class is available"), Ranger);
	TestNotNull(TEXT("Mage production class is available"), Mage);
	if (!Warrior || !Ranger || !Mage) return false;

	ValidateVariantNode(*this, *Warrior, TEXT("Talent_Warrior_WeaponMaster_MartialSpecialization"), 3);
	ValidateVariantNode(*this, *Ranger, TEXT("Talent_Ranger_Hunter_FavoredEnemy"), 0, true);
	ValidateVariantNode(*this, *Mage, TEXT("Talent_Mage_Evoker_ElementalAffinity"), 4);
	ValidateVariantNode(*this, *Mage, TEXT("Talent_Mage_SurfaceWeaver_Imbuement"), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG014ReadModelProjectionTest,
	"Grimrock.UI.RPG01.ProductionAssets.ReadModelProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUIRPG014ReadModelProjectionTest::RunTest(const FString&)
{
	using namespace UIRPG014Production;
	FRPGClassProgressionTransactionService::ResetRuntimeState();

	for (URPGClassAsset* ClassAsset : LoadProductionClasses(*this))
	{
		if (!ClassAsset) continue;
		UGridPartyInventoryComponent* Party = MakeParty(ClassAsset);
		FGridSkillsPageView View;
		TestTrue(*FString::Printf(TEXT("%s production read model builds"), *ClassAsset->ClassId.ToString()),
			FGridSkillsPageService::TryBuildCharacterView(Party, 0, {}, View));
		TestEqual(TEXT("Read model preserves ClassId"), View.ClassId, ClassAsset->ClassId);
		TestEqual(TEXT("Level 20 is projected"), View.CharacterLevel, 20);
		TestEqual(TEXT("Production tree exposes three branches"), View.TalentTree.Branches.Num(), 3);

		int32 NodeCount = 0;
		for (const FGridTalentBranchView& Branch : View.TalentTree.Branches)
		{
			TestEqual(TEXT("Projected production branch exposes five nodes"), Branch.Nodes.Num(), 5);
			NodeCount += Branch.Nodes.Num();
		}
		TestEqual(TEXT("Projected class exposes fifteen conceptual nodes"), NodeCount, 15);
		TestEqual(TEXT("Empty level-20 selection keeps all ten Talent Points"), View.RemainingTalentPoints, 10);
		FRPGClassProgressionTransactionService::ResetRuntimeState(Party);
	}

	FRPGClassProgressionTransactionService::ResetRuntimeState();
	return true;
}

#endif

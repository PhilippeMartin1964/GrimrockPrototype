#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridSkillsPageService.h"
#include "UI/GridTalentDetailWidget.h"
#include "UI/RPGTalentPresentationAsset.h"
#include "UI/UILocalizationText.h"

namespace UIRPGDESC016QA
{
	struct FClassSpec
	{
		FName ClassId;
		const TCHAR* ObjectPath;
	};

	const FClassSpec ClassSpecs[] = {
		{ TEXT("Warrior"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Warrior.DA_Class_Warrior") },
		{ TEXT("Rogue"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Rogue.DA_Class_Rogue") },
		{ TEXT("Ranger"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.DA_Class_Ranger") },
		{ TEXT("Mage"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Mage.DA_Class_Mage") },
		{ TEXT("Priest"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Priest.DA_Class_Priest") },
		{ TEXT("Alchemist"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Alchemist.DA_Class_Alchemist") }
	};

	const TCHAR* PresentationPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/UI/RPG/DA_RPGTalentPresentation.DA_RPGTalentPresentation");

	UGridPartyInventoryComponent* MakeParty(URPGClassAsset* ClassAsset)
	{
		UGridPartyInventoryComponent* Party = NewObject<UGridPartyInventoryComponent>();
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

	const FRPGTalentBranchPresentationDefinition* FindBranchPresentation(
		const FRPGClassPresentationDefinition& ClassPresentation,
		FName BranchId)
	{
		return ClassPresentation.Branches.FindByPredicate(
			[BranchId](const FRPGTalentBranchPresentationDefinition& Branch)
			{
				return Branch.TalentBranchId == BranchId;
			});
	}

	bool IsCanonicalTierLevel(int32 Level)
	{
		return Level == 2 || Level == 6 || Level == 10 || Level == 14 || Level == 18;
	}

	void ValidatePlayerFacingText(
		FAutomationTestBase& Test,
		const FString& Context,
		const FText& Text)
	{
		if (Text.IsEmpty())
		{
			return;
		}

		const FString Value = Text.ToString();
		for (const TCHAR* Forbidden : {
			TEXT("Status_"),
			TEXT("Action_"),
			TEXT("Choice_"),
			TEXT("Talent_"),
			TEXT("Skill_"),
			TEXT("Recipe_")
		})
		{
			Test.TestFalse(
				*FString::Printf(TEXT("%s does not leak %s"), *Context, Forbidden),
				Value.Contains(Forbidden, ESearchCase::CaseSensitive));
		}

		for (const TCHAR* LegacyStatus : {
			TEXT("ACTION DISPONIBLE"),
			TEXT("ACTION DÉBLOQUÉE"),
			TEXT("ACTION ACCORDÉE")
		})
		{
			Test.TestFalse(
				*FString::Printf(TEXT("%s does not revive legacy action status '%s'"), *Context, LegacyStatus),
				Value.Contains(LegacyStatus, ESearchCase::IgnoreCase));
		}

		for (const TCHAR* TechnicalFragment : {
			TEXT(" WD"),
			TEXT("RawDamage"),
			TEXT("Accuracy"),
			TEXT("AoE"),
			TEXT("DoT"),
			TEXT("friendly fire"),
			TEXT("InitiativeModifier"),
			TEXT("ArmorGate"),
			TEXT("ActionPointCost"),
			TEXT("AreaRadius"),
			TEXT("DamageType"),
			TEXT("Alchemy Rank"),
			TEXT("Arcana Rank"),
			TEXT("Religion Rank"),
			TEXT("WIS mod"),
			TEXT("DEX mod"),
			TEXT("INT mod"),
			TEXT("SourcePolicy"),
			TEXT("MaxStacks"),
			TEXT("Mechanical"),
			TEXT("Construct"),
			TEXT("Physical"),
			TEXT("Piercing"),
			TEXT("ManaCost"),
			TEXT("Evasion"),
			TEXT("tick"),
			TEXT("clamp"),
			TEXT("hard-cod"),
			TEXT("Item_"),
			TEXT("Surface_"),
			TEXT("Trap_")
		})
		{
			Test.TestFalse(
				*FString::Printf(TEXT("%s does not leak technical wording '%s'"), *Context, TechnicalFragment),
				Value.Contains(TechnicalFragment, ESearchCase::CaseSensitive));
		}
	}

	void ValidateDetailLines(
		FAutomationTestBase& Test,
		const FString& Context,
		const TArray<FGridTalentDetailLineView>& Lines)
	{
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			ValidatePlayerFacingText(
				Test,
				FString::Printf(TEXT("%s line %d value"), *Context, Index),
				Lines[Index].Value);
		}
	}

	void ValidateTextIdentity(
		FAutomationTestBase& Test,
		const FString& Context,
		FName TextId)
	{
		if (TextId.IsNone())
		{
			return;
		}

		const FText Resolved = FUILocalizationText::Get(TextId);
		Test.TestFalse(
			*FString::Printf(TEXT("%s resolves %s"), *Context, *TextId.ToString()),
			Resolved.IsEmpty());
		if (!Resolved.IsEmpty())
		{
			ValidatePlayerFacingText(Test, Context + TEXT("/") + TextId.ToString(), Resolved);
		}
	}

	void ValidateDetailTextCoverage(
		FAutomationTestBase& Test,
		const FString& Context,
		const TArray<FGridTalentDetailLineView>& Lines,
		int32& InOutNumericLineCount)
	{
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			const FGridTalentDetailLineView& Line = Lines[Index];
			const FString LineContext = FString::Printf(TEXT("%s line %d"), *Context, Index);

			if (Line.bHasNumericValue)
			{
				++InOutNumericLineCount;
				Test.TestFalse(
					*FString::Printf(TEXT("%s numeric value has an explanatory label identity"), *LineContext),
					Line.LabelTextId.IsNone());
			}

			if (!Line.LabelTextId.IsNone())
			{
				ValidateTextIdentity(Test, LineContext + TEXT("/Label"), Line.LabelTextId);
			}
			if (!Line.ValueTextId.IsNone())
			{
				ValidateTextIdentity(Test, LineContext + TEXT("/Value"), Line.ValueTextId);
			}

			const bool bHasPayload =
				Line.bHasNumericValue ||
				!Line.ValueTextId.IsNone() ||
				!Line.Value.IsEmpty();
			Test.TestTrue(
				*FString::Printf(TEXT("%s has a displayable payload"), *LineContext),
				bHasPayload);
		}
	}

	const FGridTalentNodeView* FindNode(const FGridSkillsPageView& View, FName NodeId)
	{
		for (const FGridTalentBranchView& Branch : View.TalentTree.Branches)
		{
			if (const FGridTalentNodeView* Node = Branch.Nodes.FindByPredicate(
				[NodeId](const FGridTalentNodeView& Candidate)
				{
					return Candidate.TalentNodeId == NodeId;
				}))
			{
				return Node;
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC016SixClassCanonicalSurfaceTest,
	"Grimrock.UI.RPG.DESC01.QA16.SixClassCanonicalSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC016SixClassCanonicalSurfaceTest::RunTest(const FString&)
{
	using namespace UIRPGDESC016QA;
	FRPGClassProgressionTransactionService::ResetRuntimeState();

	URPGTalentPresentationAsset* Presentation =
		LoadObject<URPGTalentPresentationAsset>(nullptr, PresentationPath);
	if (!TestNotNull(TEXT("Production Talent presentation catalog loads"), Presentation))
	{
		return false;
	}

	int32 TotalNodes = 0;
	int32 TotalSimpleNodes = 0;
	int32 TotalVariantNodes = 0;

	for (const FClassSpec& Spec : ClassSpecs)
	{
		URPGClassAsset* ClassAsset = LoadObject<URPGClassAsset>(nullptr, Spec.ObjectPath);
		if (!TestNotNull(*FString::Printf(TEXT("%s production class loads"), *Spec.ClassId.ToString()), ClassAsset))
		{
			continue;
		}

		const FRPGClassPresentationDefinition* ClassPresentation = Presentation->FindClass(Spec.ClassId);
		if (!TestNotNull(*FString::Printf(TEXT("%s presentation exists"), *Spec.ClassId.ToString()), ClassPresentation))
		{
			continue;
		}

		UGridPartyInventoryComponent* Party = MakeParty(ClassAsset);
		FGridSkillsPageView View;
		if (!TestTrue(
			*FString::Printf(TEXT("%s canonical read-model builds"), *Spec.ClassId.ToString()),
			FGridSkillsPageService::TryBuildCharacterView(Party, 0, {}, View)))
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState(Party);
			continue;
		}

		TestEqual(
			*FString::Printf(TEXT("%s exposes three projected branches"), *Spec.ClassId.ToString()),
			View.TalentTree.Branches.Num(), 3);

		int32 ClassNodeCount = 0;
		for (const FGridTalentBranchView& Branch : View.TalentTree.Branches)
		{
			const FRPGTalentBranchPresentationDefinition* BranchPresentation =
				FindBranchPresentation(*ClassPresentation, Branch.TalentBranchId);
			if (!TestNotNull(
				*FString::Printf(TEXT("%s/%s presentation branch exists"),
					*Spec.ClassId.ToString(), *Branch.TalentBranchId.ToString()),
				BranchPresentation))
			{
				continue;
			}

			TestEqual(
				*FString::Printf(TEXT("%s/%s exposes five Talent nodes"),
					*Spec.ClassId.ToString(), *Branch.TalentBranchId.ToString()),
				Branch.Nodes.Num(), 5);

			for (const FGridTalentNodeView& Node : Branch.Nodes)
			{
				++ClassNodeCount;
				++TotalNodes;
				const FString Context = FString::Printf(
					TEXT("%s/%s"), *Spec.ClassId.ToString(), *Node.TalentNodeId.ToString());

				TestFalse(*FString::Printf(TEXT("%s has a display name"), *Context), Node.DisplayName.IsEmpty());
				TestTrue(*FString::Printf(TEXT("%s has an explicit TYPE"), *Context),
					Node.Type != ERPGTalentPresentationType::None);
				TestFalse(*FString::Printf(TEXT("%s has a PRINCIPE"), *Context), Node.Principle.IsEmpty());
				TestTrue(*FString::Printf(TEXT("%s uses a canonical tier level"), *Context),
					IsCanonicalTierLevel(Node.Acquisition.MinimumLevel));
				TestTrue(*FString::Printf(TEXT("%s has a positive Talent cost"), *Context),
					Node.Acquisition.PointCost > 0);

				UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
				TestTrue(*FString::Printf(TEXT("%s initializes the canonical detail presenter"), *Context),
					Detail->InitializeTalentDetail(Node, *BranchPresentation));

				if (Node.bHasExclusiveVariants)
				{
					++TotalVariantNodes;
					TestTrue(*FString::Printf(TEXT("%s has at least two variants"), *Context), Node.Variants.Num() >= 2);
					TestTrue(*FString::Printf(TEXT("%s has no SimpleChoiceId"), *Context), Node.SimpleChoiceId.IsNone());
					TestFalse(*FString::Printf(TEXT("%s cannot use simple acquisition"), *Context), Node.bCanAcquireSimple);

					for (const FGridTalentVariantView& Variant : Node.Variants)
					{
						const FString VariantContext = Context + TEXT("/") + Variant.ChoiceId.ToString();
						TestFalse(*FString::Printf(TEXT("%s has ChoiceId"), *VariantContext), Variant.ChoiceId.IsNone());
						TestFalse(*FString::Printf(TEXT("%s has display name"), *VariantContext), Variant.DisplayName.IsEmpty());
						TestEqual(*FString::Printf(TEXT("%s TYPE matches conceptual node"), *VariantContext),
							Variant.Type, Node.Type);
						TestFalse(*FString::Printf(TEXT("%s has PRINCIPE"), *VariantContext), Variant.Principle.IsEmpty());

					}
				}
				else
				{
					++TotalSimpleNodes;
					TestTrue(*FString::Printf(TEXT("%s has no fake Variants payload"), *Context), Node.Variants.IsEmpty());
					TestFalse(*FString::Printf(TEXT("%s has SimpleChoiceId"), *Context), Node.SimpleChoiceId.IsNone());
				}
			}

		}

		TestEqual(
			*FString::Printf(TEXT("%s exposes fifteen Talent nodes"), *Spec.ClassId.ToString()),
			ClassNodeCount, 15);
		FRPGClassProgressionTransactionService::ResetRuntimeState(Party);
	}

	FRPGClassProgressionTransactionService::ResetRuntimeState();
	TestEqual(TEXT("Six classes expose ninety conceptual Talent nodes"), TotalNodes, 90);
	TestEqual(TEXT("Exactly eighty-six Talents are simple nodes"), TotalSimpleNodes, 86);
	TestEqual(TEXT("Exactly four Talents are exclusive-variant nodes"), TotalVariantNodes, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC016PlayerFacingLanguageTest,
	"Grimrock.UI.RPG.DESC01.QA16.PlayerFacingLanguage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC016PlayerFacingLanguageTest::RunTest(const FString&)
{
	using namespace UIRPGDESC016QA;
	FRPGClassProgressionTransactionService::ResetRuntimeState();

	int32 AuditedNodes = 0;
	for (const FClassSpec& Spec : ClassSpecs)
	{
		URPGClassAsset* ClassAsset = LoadObject<URPGClassAsset>(nullptr, Spec.ObjectPath);
		if (!TestNotNull(*FString::Printf(TEXT("%s production class loads"), *Spec.ClassId.ToString()), ClassAsset))
		{
			continue;
		}

		UGridPartyInventoryComponent* Party = MakeParty(ClassAsset);
		FGridSkillsPageView View;
		if (!TestTrue(
			*FString::Printf(TEXT("%s read-model builds for language audit"), *Spec.ClassId.ToString()),
			FGridSkillsPageService::TryBuildCharacterView(Party, 0, {}, View)))
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState(Party);
			continue;
		}

		for (const FGridTalentBranchView& Branch : View.TalentTree.Branches)
		{
			for (const FGridTalentNodeView& Node : Branch.Nodes)
			{
				++AuditedNodes;
				const FString Context = FString::Printf(
					TEXT("%s/%s"), *Spec.ClassId.ToString(), *Node.TalentNodeId.ToString());

				ValidatePlayerFacingText(*this, Context + TEXT("/Name"), Node.DisplayName);
				ValidatePlayerFacingText(*this, Context + TEXT("/Principle"), Node.Principle);
				ValidateDetailLines(*this, Context + TEXT("/Effects"), Node.Effects);
				ValidateDetailLines(*this, Context + TEXT("/Usage"), Node.Usage);
				for (const FText& Name : Node.Acquisition.PrerequisiteTalentNames)
				{
					ValidatePlayerFacingText(*this, Context + TEXT("/Prerequisite"), Name);
				}
				if (Node.bHasExclusiveVariants)
				{
					TestEqual(
						*FString::Printf(TEXT("%s exclusivity uses stable text identity"), *Context),
						Node.Acquisition.ExclusivityTextId,
						FName(TEXT("Talent.Acquisition.Exclusivity.SingleVariant")));
				}
				for (const FGridRecipeDisplayView& Recipe : Node.Acquisition.GrantedRecipes)
				{
					TestTrue(
						*FString::Printf(TEXT("%s recipe keeps Recipe_* gameplay identity"), *Context),
						Recipe.RecipeId.ToString().StartsWith(TEXT("Recipe_"), ESearchCase::CaseSensitive));
					ValidatePlayerFacingText(*this, Context + TEXT("/Recipe"), Recipe.DisplayName);
				}

				if (Node.Type == ERPGTalentPresentationType::AutomaticReaction)
				{
					TestTrue(*FString::Printf(TEXT("%s automatic reaction has no voluntary UTILISATION"), *Context),
						Node.Usage.IsEmpty());
				}

				for (const FGridTalentVariantView& Variant : Node.Variants)
				{
					const FString VariantContext = Context + TEXT("/") + Variant.ChoiceId.ToString();
					ValidatePlayerFacingText(*this, VariantContext + TEXT("/Name"), Variant.DisplayName);
					ValidatePlayerFacingText(*this, VariantContext + TEXT("/Principle"), Variant.Principle);
					ValidateDetailLines(*this, VariantContext + TEXT("/Effects"), Variant.Effects);
					ValidateDetailLines(*this, VariantContext + TEXT("/Usage"), Variant.Usage);
				}
			}
		}

		FRPGClassProgressionTransactionService::ResetRuntimeState(Party);
	}

	FRPGClassProgressionTransactionService::ResetRuntimeState();
	TestEqual(TEXT("Player-facing audit covers all ninety Talent nodes"), AuditedNodes, 90);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC016TextCoverageTest,
	"Grimrock.UI.RPG.DESC01.QA16.TextCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC016TextCoverageTest::RunTest(const FString&)
{
	using namespace UIRPGDESC016QA;
	FRPGClassProgressionTransactionService::ResetRuntimeState();

	int32 AuditedNodes = 0;
	int32 NumericLineCount = 0;
	for (const FClassSpec& Spec : ClassSpecs)
	{
		URPGClassAsset* ClassAsset = LoadObject<URPGClassAsset>(nullptr, Spec.ObjectPath);
		if (!TestNotNull(*FString::Printf(TEXT("%s production class loads"), *Spec.ClassId.ToString()), ClassAsset))
		{
			continue;
		}

		UGridPartyInventoryComponent* Party = MakeParty(ClassAsset);
		FGridSkillsPageView View;
		if (!TestTrue(
			*FString::Printf(TEXT("%s read-model builds for text coverage"), *Spec.ClassId.ToString()),
			FGridSkillsPageService::TryBuildCharacterView(Party, 0, {}, View)))
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState(Party);
			continue;
		}

		for (const FGridTalentBranchView& Branch : View.TalentTree.Branches)
		{
			for (const FGridTalentNodeView& Node : Branch.Nodes)
			{
				++AuditedNodes;
				const FString Context = FString::Printf(
					TEXT("%s/%s"), *Spec.ClassId.ToString(), *Node.TalentNodeId.ToString());

				ValidateDetailTextCoverage(
					*this, Context + TEXT("/Effects"), Node.Effects, NumericLineCount);
				ValidateDetailTextCoverage(
					*this, Context + TEXT("/Usage"), Node.Usage, NumericLineCount);

				if (!Node.Acquisition.ExclusivityTextId.IsNone())
				{
					ValidateTextIdentity(
						*this,
						Context + TEXT("/Acquisition/Exclusivity"),
						Node.Acquisition.ExclusivityTextId);
				}
				for (const FGridRecipeDisplayView& Recipe : Node.Acquisition.GrantedRecipes)
				{
					TestFalse(
						*FString::Printf(TEXT("%s Recipe display name resolves"), *Context),
						Recipe.DisplayName.IsEmpty());
					ValidatePlayerFacingText(
						*this,
						Context + TEXT("/Acquisition/Recipe"),
						Recipe.DisplayName);
				}

				for (const FGridTalentVariantView& Variant : Node.Variants)
				{
					const FString VariantContext =
						Context + TEXT("/") + Variant.ChoiceId.ToString();
					ValidateDetailTextCoverage(
						*this, VariantContext + TEXT("/Effects"), Variant.Effects, NumericLineCount);
					ValidateDetailTextCoverage(
						*this, VariantContext + TEXT("/Usage"), Variant.Usage, NumericLineCount);
				}
			}
		}

		FRPGClassProgressionTransactionService::ResetRuntimeState(Party);
	}

	FRPGClassProgressionTransactionService::ResetRuntimeState();
	TestEqual(TEXT("Systematic text coverage audits all ninety Talent nodes"), AuditedNodes, 90);
	TestTrue(TEXT("Systematic text coverage exercises numeric effect/usage rows"), NumericLineCount > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC016VariantFamiliesTest,
	"Grimrock.UI.RPG.DESC01.QA16.FourVariantFamilies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC016VariantFamiliesTest::RunTest(const FString&)
{
	using namespace UIRPGDESC016QA;
	FRPGClassProgressionTransactionService::ResetRuntimeState();

	struct FExpectedVariantNode
	{
		FName ClassId;
		FName NodeId;
		int32 ExactCount;
	};

	const FExpectedVariantNode Expected[] = {
		{ TEXT("Warrior"), TEXT("Talent_Warrior_WeaponMaster_MartialSpecialization"), 3 },
		{ TEXT("Ranger"), TEXT("Talent_Ranger_Hunter_FavoredEnemy"), 0 },
		{ TEXT("Mage"), TEXT("Talent_Mage_Evoker_ElementalAffinity"), 4 },
		{ TEXT("Mage"), TEXT("Talent_Mage_SurfaceWeaver_Imbuement"), 4 }
	};

	TMap<FName, FGridSkillsPageView> ViewsByClass;
	TArray<UGridPartyInventoryComponent*> Parties;
	for (const FClassSpec& Spec : ClassSpecs)
	{
		URPGClassAsset* ClassAsset = LoadObject<URPGClassAsset>(nullptr, Spec.ObjectPath);
		if (!ClassAsset) continue;
		UGridPartyInventoryComponent* Party = MakeParty(ClassAsset);
		Parties.Add(Party);
		FGridSkillsPageView View;
		if (FGridSkillsPageService::TryBuildCharacterView(Party, 0, {}, View))
		{
			ViewsByClass.Add(Spec.ClassId, MoveTemp(View));
		}
	}

	int32 FoundExpected = 0;
	for (const FExpectedVariantNode& Spec : Expected)
	{
		const FGridSkillsPageView* View = ViewsByClass.Find(Spec.ClassId);
		if (!TestNotNull(*FString::Printf(TEXT("%s QA view exists"), *Spec.ClassId.ToString()), View))
		{
			continue;
		}

		const FGridTalentNodeView* Node = FindNode(*View, Spec.NodeId);
		if (!TestNotNull(
			*FString::Printf(TEXT("%s/%s expected variant node exists"),
				*Spec.ClassId.ToString(), *Spec.NodeId.ToString()),
			Node))
		{
			continue;
		}

		++FoundExpected;
		TestTrue(TEXT("Expected family is explicitly an exclusive-variant node"), Node->bHasExclusiveVariants);
		if (Spec.ExactCount > 0)
		{
			TestEqual(TEXT("Expected family has exact variant count"), Node->Variants.Num(), Spec.ExactCount);
		}
		else
		{
			TestTrue(TEXT("Favored Enemy has multiple bestiary-backed variants"), Node->Variants.Num() >= 2);
		}
	}

	int32 ActualVariantNodes = 0;
	for (const TPair<FName, FGridSkillsPageView>& Pair : ViewsByClass)
	{
		for (const FGridTalentBranchView& Branch : Pair.Value.TalentTree.Branches)
		{
			for (const FGridTalentNodeView& Node : Branch.Nodes)
			{
				if (Node.bHasExclusiveVariants)
				{
					++ActualVariantNodes;
				}
			}
		}
	}

	TestEqual(TEXT("All four expected variant families are found"), FoundExpected, 4);
	TestEqual(TEXT("No fifth production variant family exists"), ActualVariantNodes, 4);

	for (UGridPartyInventoryComponent* Party : Parties)
	{
		FRPGClassProgressionTransactionService::ResetRuntimeState(Party);
	}
	FRPGClassProgressionTransactionService::ResetRuntimeState();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC016CanonicalRecipeDisplayNamesTest,
	"Grimrock.UI.RPG.DESC01.QA16.CanonicalRecipeDisplayNames",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC016CanonicalRecipeDisplayNamesTest::RunTest(const FString&)
{
	using namespace UIRPGDESC016QA;
	FRPGClassProgressionTransactionService::ResetRuntimeState();

	const TMap<FName, TArray<FName>> ExpectedByTalent = {
		{ TEXT("Talent_Alchemist_Grenadier_FireBomb"), { TEXT("Recipe_Bomb_Fire") } },
		{ TEXT("Talent_Alchemist_Grenadier_ToxicBomb"), { TEXT("Recipe_Bomb_Toxic") } },
		{ TEXT("Talent_Alchemist_Apothecary_Antidote"), { TEXT("Recipe_Antidote") } },
		{ TEXT("Talent_Alchemist_Apothecary_DefensiveElixir"), {
			TEXT("Recipe_DefensiveElixir_Fire"),
			TEXT("Recipe_DefensiveElixir_Ice"),
			TEXT("Recipe_DefensiveElixir_Lightning"),
			TEXT("Recipe_DefensiveElixir_Poison")
		} },
		{ TEXT("Talent_Alchemist_Apothecary_Panacea"), { TEXT("Recipe_Panacea") } },
		{ TEXT("Talent_Alchemist_Transmuter_OilSlick"), { TEXT("Recipe_Flask_Oil") } },
		{ TEXT("Talent_Alchemist_Transmuter_AcidFlask"), { TEXT("Recipe_Flask_Acid") } },
		{ TEXT("Talent_Alchemist_Transmuter_CorrosiveCloud"), { TEXT("Recipe_Flask_CorrosiveCloud") } },
		{ TEXT("Talent_Alchemist_Transmuter_MajorTransmutation"), {
			TEXT("Recipe_MajorTransmutation_Fire"),
			TEXT("Recipe_MajorTransmutation_Ice"),
			TEXT("Recipe_MajorTransmutation_Poison"),
			TEXT("Recipe_MajorTransmutation_Oil")
		} }
	};

	int32 TotalGrantedRecipeIds = 0;
	int32 TotalProjectedRecipeNames = 0;

	for (const FClassSpec& Spec : ClassSpecs)
	{
		URPGClassAsset* ClassAsset = LoadObject<URPGClassAsset>(nullptr, Spec.ObjectPath);
		if (!TestNotNull(*FString::Printf(TEXT("%s production class loads for recipe audit"), *Spec.ClassId.ToString()), ClassAsset))
		{
			continue;
		}

		for (const FRPGClassProgressionChoiceDefinition& Choice : ClassAsset->ProgressionChoices)
		{
			for (const FName GrantedId : Choice.GrantedRequirementIds)
			{
				if (GrantedId.ToString().StartsWith(TEXT("Recipe_"), ESearchCase::CaseSensitive))
				{
					++TotalGrantedRecipeIds;
				}
			}
		}

		UGridPartyInventoryComponent* Party = MakeParty(ClassAsset);
		FGridSkillsPageView View;
		if (!TestTrue(
			*FString::Printf(TEXT("%s read-model builds for recipe audit"), *Spec.ClassId.ToString()),
			FGridSkillsPageService::TryBuildCharacterView(Party, 0, {}, View)))
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState(Party);
			continue;
		}

		for (const FGridTalentBranchView& Branch : View.TalentTree.Branches)
		{
			for (const FGridTalentNodeView& Node : Branch.Nodes)
			{
				TotalProjectedRecipeNames += Node.Acquisition.GrantedRecipes.Num();

				if (const TArray<FName>* ExpectedRecipeIds = ExpectedByTalent.Find(Node.TalentNodeId))
				{
					TestEqual(
						*FString::Printf(TEXT("%s has exact canonical recipe count"), *Node.TalentNodeId.ToString()),
						Node.Acquisition.GrantedRecipes.Num(),
						ExpectedRecipeIds->Num());

					for (int32 Index = 0;
						Index < ExpectedRecipeIds->Num() && Index < Node.Acquisition.GrantedRecipes.Num();
						++Index)
					{
						const FGridRecipeDisplayView& Recipe = Node.Acquisition.GrantedRecipes[Index];
						ValidatePlayerFacingText(
							*this,
							FString::Printf(TEXT("%s/%s/Recipe"), *Spec.ClassId.ToString(), *Node.TalentNodeId.ToString()),
							Recipe.DisplayName);
						TestEqual(
							*FString::Printf(TEXT("%s recipe %d keeps canonical RecipeId"), *Node.TalentNodeId.ToString(), Index),
							Recipe.RecipeId,
							(*ExpectedRecipeIds)[Index]);
					}
				}
				else
				{
					TestTrue(
						*FString::Printf(TEXT("%s has no unexpected recipe presentation"), *Node.TalentNodeId.ToString()),
						Node.Acquisition.GrantedRecipes.IsEmpty());
				}
			}
		}

		FRPGClassProgressionTransactionService::ResetRuntimeState(Party);
	}

	FRPGClassProgressionTransactionService::ResetRuntimeState();
	TestEqual(TEXT("Production classes declare fifteen Recipe_* grants"), TotalGrantedRecipeIds, 15);
	TestEqual(TEXT("Read-model projects fifteen canonical Recipe records"), TotalProjectedRecipeNames, 15);
	TestEqual(TEXT("Exactly nine Talents expose recipes"), ExpectedByTalent.Num(), 9);
	return true;
}

#endif

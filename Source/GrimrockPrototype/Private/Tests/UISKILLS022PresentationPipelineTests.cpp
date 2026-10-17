#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillPointService.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridSkillsPageService.h"
#include "UI/GridSkillsWidget.h"
#include "UI/RPGSkillPresentationAsset.h"

namespace UISKILLS022Tests
{
	struct FRuntimeStateGuard
	{
		FRuntimeStateGuard()
		{
			FRPGAuthoringIdentityResolver::ResetRuntimeCache();
			FRPGClassProgressionTransactionService::ResetRuntimeState();
		}

		~FRuntimeStateGuard()
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState();
			FRPGAuthoringIdentityResolver::ResetRuntimeCache();
		}
	};

	URPGClassAsset* MakeClass(UObject* Outer, FName ClassId, const TCHAR* DisplayName)
	{
		URPGClassAsset* ClassDefinition = NewObject<URPGClassAsset>(Outer);
		ClassDefinition->ClassId = ClassId;
		ClassDefinition->DisplayName = FText::FromString(DisplayName);
		ClassDefinition->HealthAtLevelOne = 16;
		return FRPGAuthoringIdentityResolver::RememberClassDefinition(ClassDefinition)
			? ClassDefinition
			: nullptr;
	}

	URPGSkillAsset* MakeSkill(
		UObject* Outer,
		FName SkillId,
		const TCHAR* DisplayName,
		ERPGSkillGoverningAttribute Attribute,
		bool bAllowUntrainedChecks = true)
	{
		URPGSkillAsset* Skill = NewObject<URPGSkillAsset>(Outer);
		Skill->SkillId = SkillId;
		Skill->DisplayName = FText::FromString(DisplayName);
		Skill->Description = FText::FromString(TEXT("Description de test."));
		Skill->GoverningAttribute = Attribute;
		Skill->MaxRank = 5;
		Skill->bAllowUntrainedChecks = bAllowUntrainedChecks;
		return Skill;
	}

	UGridPartyInventoryComponent* MakeParty(URPGClassAsset* CharacterClass, const FRPGAttributes& Attributes)
	{
		UGridPartyInventoryComponent* Party = NewObject<UGridPartyInventoryComponent>();
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.DisplayName = FText::FromString(TEXT("Test"));
		Character.ClassId = CharacterClass->ClassId;
		Character.ClassDisplayName = CharacterClass->DisplayName;
		Character.ClassDefinition = CharacterClass;
		Character.Level = 3;
		Character.Attributes = Attributes;
		Party->PartyInventoryState.ActiveCharacters.Add(Character);
		Party->PartyInventoryState.ActiveEquipment.SetNum(1);
		Party->PartyInventoryState.SelectedCharacterIndex = 0;
		return Party;
	}

	bool SetRank(FGridCharacterInventoryState& Character, URPGSkillAsset* Skill, int32 Rank)
	{
		FRPGSkillMutationResult Mutation;
		return FRPGSkillService::TrySetSkillRank(Character, Skill, Rank, Mutation);
	}

	URPGSkillPresentationAsset* MakePresentationCatalog()
	{
		URPGSkillPresentationAsset* Catalog = NewObject<URPGSkillPresentationAsset>();

		FRPGSkillPresentationDefinition Acrobatics;
		Acrobatics.SkillId = TEXT("Skill_Acrobatics");
		Acrobatics.UsageDescription = FText::FromString(TEXT("Utilisations de test."));
		Acrobatics.BuildAdvice = FText::FromString(TEXT("Conseil de build."));
		Acrobatics.RecommendedClassIds = { TEXT("Rogue") };
		Acrobatics.UsefulClassIds = { TEXT("Warrior") };
		Acrobatics.DiscouragedClassIds = { TEXT("Priest") };
		Acrobatics.SynergySkillIds = { TEXT("Skill_Athletics") };

		FRPGSkillPresentationDefinition Athletics;
		Athletics.SkillId = TEXT("Skill_Athletics");

		Catalog->Skills = { Acrobatics, Athletics };
		return Catalog;
	}

	FGridSkillEntryView MakeSelectionEntry(FName SkillId)
	{
		FGridSkillEntryView Entry;
		Entry.SkillId = SkillId;
		Entry.MaxRank = 5;
		Entry.CurrentRankCap = 2;
		return Entry;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLS022DetailProjectionTest,
	"Grimrock.UI.Skills02.Pipeline.DetailProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLS022DetailProjectionTest::RunTest(const FString&)
{
	using namespace UISKILLS022Tests;
	FRuntimeStateGuard Guard;

	URPGClassAsset* Rogue = MakeClass(GetTransientPackage(), TEXT("Rogue"), TEXT("Voleur"));
	URPGClassAsset* Warrior = MakeClass(GetTransientPackage(), TEXT("Warrior"), TEXT("Guerrier"));
	URPGClassAsset* Priest = MakeClass(GetTransientPackage(), TEXT("Priest"), TEXT("Prêtre"));
	TestNotNull(TEXT("Rogue test class is valid"), Rogue);
	TestNotNull(TEXT("Warrior affinity class is valid"), Warrior);
	TestNotNull(TEXT("Priest discouraged class is valid"), Priest);
	if (!Rogue || !Warrior || !Priest) return false;

	FRPGAttributes Attributes;
	Attributes.Dexterity = 14;
	UGridPartyInventoryComponent* Party = MakeParty(Rogue, Attributes);

	URPGSkillAsset* Acrobatics = MakeSkill(
		Party, TEXT("Skill_Acrobatics"), TEXT("Acrobaties"), ERPGSkillGoverningAttribute::Dexterity);
	URPGSkillAsset* Athletics = MakeSkill(
		Party, TEXT("Skill_Athletics"), TEXT("Athlétisme"), ERPGSkillGoverningAttribute::Strength);
	TestTrue(TEXT("Acrobatics rank setup succeeds"),
		SetRank(Party->PartyInventoryState.ActiveCharacters[0], Acrobatics, 1));

	const TArray<const URPGSkillAsset*> Definitions = { Acrobatics, Athletics };
	FGridSkillsPageView PageView;
	TestTrue(TEXT("Skills page view builds"),
		FGridSkillsPageService::TryBuildSelectedCharacterView(Party, Definitions, PageView));

	FGridSkillEntryView* AcrobaticsEntry = PageView.Skills.FindByPredicate(
		[](const FGridSkillEntryView& Entry)
		{
			return Entry.SkillId == TEXT("Skill_Acrobatics");
		});
	TestNotNull(TEXT("Acrobatics row exists"), AcrobaticsEntry);
	if (!AcrobaticsEntry) return false;
	AcrobaticsEntry->bCanDecreaseRank = true;

	URPGSkillPresentationAsset* Presentation = MakePresentationCatalog();
	TestTrue(TEXT("Presentation catalog is structurally valid"), Presentation->IsValidDefinition());

	FGridSkillDetailView Detail;
	TestTrue(TEXT("Detail projection builds"),
		FGridSkillsPageService::TryBuildSkillDetailView(
			Party, PageView, TEXT("Skill_Acrobatics"), Definitions, Presentation, Detail));

	TestEqual(TEXT("Selected SkillId is preserved"), Detail.SkillId, FName(TEXT("Skill_Acrobatics")));
	TestEqual(TEXT("Governing attribute value is projected"), Detail.GoverningAttributeValue, 14);
	TestEqual(TEXT("Attribute modifier comes from character rules"),
		Detail.GoverningAttributeModifier, URPGCharacterRulesLibrary::GetAttributeModifier(14));
	TestEqual(TEXT("Base check bonus is rank plus attribute modifier"),
		Detail.BaseCheckBonus, 1 + URPGCharacterRulesLibrary::GetAttributeModifier(14));
	TestEqual(TEXT("Current rank cap is projected"), Detail.CurrentRankCap, 2);
	TestEqual(TEXT("Remaining Skill Points are projected"), Detail.RemainingSkillPoints, 5);
	TestEqual(TEXT("Next rank is projected"), Detail.NextRank, 2);
	TestEqual(TEXT("Rank point cost comes from Skill Point authority"),
		Detail.NextRankCost, FRPGSkillPointService::GetRankPointCost());
	TestTrue(TEXT("Increase availability is projected"), Detail.bCanIncreaseRank);
	TestTrue(TEXT("Session undo availability is projected"), Detail.bCanDecreaseRank);
	TestEqual(TEXT("Training semantic is trained"), Detail.TrainingState, EGridSkillTrainingState::Trained);
	TestEqual(TEXT("Recommended class resolves"), Detail.RecommendedClasses.Num(), 1);
	TestEqual(TEXT("Useful class resolves"), Detail.UsefulClasses.Num(), 1);
	TestEqual(TEXT("Discouraged class resolves"), Detail.DiscouragedClasses.Num(), 1);
	TestEqual(TEXT("Synergy resolves"), Detail.Synergies.Num(), 1);
	if (Detail.RecommendedClasses.Num() == 1)
	{
		TestTrue(TEXT("Recommended class label is Voleur"),
			Detail.RecommendedClasses[0].DisplayName.EqualTo(FText::FromString(TEXT("Voleur"))));
	}
	if (Detail.DiscouragedClasses.Num() == 1)
	{
		TestTrue(TEXT("Discouraged class label is Prêtre"),
			Detail.DiscouragedClasses[0].DisplayName.EqualTo(FText::FromString(TEXT("Prêtre"))));
	}
	if (Detail.Synergies.Num() == 1)
	{
		TestTrue(TEXT("Synergy label is Athlétisme"),
			Detail.Synergies[0].DisplayName.EqualTo(FText::FromString(TEXT("Athlétisme"))));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLS022TrainingAndCapTest,
	"Grimrock.UI.Skills02.Pipeline.TrainingAndRankCap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLS022TrainingAndCapTest::RunTest(const FString&)
{
	using namespace UISKILLS022Tests;
	FRuntimeStateGuard Guard;

	URPGClassAsset* Rogue = MakeClass(GetTransientPackage(), TEXT("Rogue"), TEXT("Voleur"));
	TestNotNull(TEXT("Rogue test class is valid"), Rogue);
	if (!Rogue) return false;

	UGridPartyInventoryComponent* Party = MakeParty(Rogue, FRPGAttributes());
	URPGSkillAsset* Lockpicking = MakeSkill(
		Party, TEXT("Skill_Lockpicking"), TEXT("Crochetage"), ERPGSkillGoverningAttribute::Dexterity, false);
	URPGSkillAsset* Athletics = MakeSkill(
		Party, TEXT("Skill_Athletics"), TEXT("Athlétisme"), ERPGSkillGoverningAttribute::Strength, true);
	const TArray<const URPGSkillAsset*> Definitions = { Lockpicking, Athletics };

	FGridSkillsPageView PageView;
	TestTrue(TEXT("Untrained page view builds"),
		FGridSkillsPageService::TryBuildSelectedCharacterView(Party, Definitions, PageView));

	FGridSkillDetailView Detail;
	TestTrue(TEXT("Trained-only detail builds"),
		FGridSkillsPageService::TryBuildSkillDetailView(
			Party, PageView, TEXT("Skill_Lockpicking"), Definitions, nullptr, Detail));
	TestEqual(TEXT("Rank zero trained-only maps to TrainingRequired"),
		Detail.TrainingState, EGridSkillTrainingState::TrainingRequired);

	TestTrue(TEXT("Untrained-allowed detail builds"),
		FGridSkillsPageService::TryBuildSkillDetailView(
			Party, PageView, TEXT("Skill_Athletics"), Definitions, nullptr, Detail));
	TestEqual(TEXT("Rank zero allowed maps to UntrainedAllowed"),
		Detail.TrainingState, EGridSkillTrainingState::UntrainedAllowed);

	TestTrue(TEXT("Direct high-rank storage setup succeeds"),
		SetRank(Party->PartyInventoryState.ActiveCharacters[0], Athletics, 4));
	TestFalse(TEXT("Rank above the level cap rejects the page read model"),
		FGridSkillsPageService::TryBuildSelectedCharacterView(Party, Definitions, PageView));
	TestFalse(TEXT("Rejected page read model remains invalid"), PageView.IsValid());
	TestEqual(TEXT("Read-model rejection never mutates the stored rank"),
		FRPGSkillService::GetSkillRank(
			Party->PartyInventoryState.ActiveCharacters[0],
			Athletics->SkillId),
		4);

	FRPGSkillPointBalance InvalidBalance;
	TestFalse(TEXT("Rank above the level cap rejects the derived Skill Point balance"),
		FRPGSkillPointService::TryGetBalance(
			Party->PartyInventoryState.ActiveCharacters[0],
			InvalidBalance));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLS022RejectIncoherentProjectionTest,
	"Grimrock.UI.Skills02.Pipeline.RejectIncoherentProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLS022RejectIncoherentProjectionTest::RunTest(const FString&)
{
	using namespace UISKILLS022Tests;
	FRuntimeStateGuard Guard;

	URPGClassAsset* Rogue = MakeClass(GetTransientPackage(), TEXT("Rogue"), TEXT("Voleur"));
	TestNotNull(TEXT("Rogue test class is valid"), Rogue);
	if (!Rogue) return false;

	UGridPartyInventoryComponent* Party = MakeParty(Rogue, FRPGAttributes());
	URPGSkillAsset* Acrobatics = MakeSkill(
		Party, TEXT("Skill_Acrobatics"), TEXT("Acrobaties"), ERPGSkillGoverningAttribute::Dexterity);
	URPGSkillAsset* Athletics = MakeSkill(
		Party, TEXT("Skill_Athletics"), TEXT("Athlétisme"), ERPGSkillGoverningAttribute::Strength);
	const TArray<const URPGSkillAsset*> Definitions = { Acrobatics, Athletics };

	FGridSkillsPageView PageView;
	TestTrue(TEXT("Baseline page view builds"),
		FGridSkillsPageService::TryBuildSelectedCharacterView(Party, Definitions, PageView));

	FGridSkillDetailView Detail;
	TestFalse(TEXT("Unknown selected SkillId is rejected"),
		FGridSkillsPageService::TryBuildSkillDetailView(
			Party, PageView, TEXT("Skill_Unknown"), Definitions, nullptr, Detail));

	FGridSkillsPageView MismatchedPage = PageView;
	MismatchedPage.CharacterId = FGuid::NewGuid();
	TestFalse(TEXT("Page/character identity mismatch is rejected"),
		FGridSkillsPageService::TryBuildSkillDetailView(
			Party, MismatchedPage, TEXT("Skill_Acrobatics"), Definitions, nullptr, Detail));

	URPGSkillPresentationAsset* InvalidClassCatalog = MakePresentationCatalog();
	InvalidClassCatalog->Skills[0].RecommendedClassIds = { TEXT("MissingClass") };
	TestTrue(TEXT("Unknown class remains structurally valid metadata"), InvalidClassCatalog->IsValidDefinition());
	TestFalse(TEXT("Unknown class is rejected during detail resolution"),
		FGridSkillsPageService::TryBuildSkillDetailView(
			Party, PageView, TEXT("Skill_Acrobatics"), Definitions, InvalidClassCatalog, Detail));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLS022SelectionPolicyTest,
	"Grimrock.UI.Skills02.Selection.Policy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLS022SelectionPolicyTest::RunTest(const FString&)
{
	using namespace UISKILLS022Tests;

	FGridSkillsPageView PageView;
	PageView.Skills = {
		MakeSelectionEntry(TEXT("Skill_Acrobatics")),
		MakeSelectionEntry(TEXT("Skill_Athletics"))
	};

	TestEqual(TEXT("Existing preferred Skill is preserved"),
		FGridSkillsPageService::ResolveSkillSelection(PageView, TEXT("Skill_Athletics")),
		FName(TEXT("Skill_Athletics")));
	TestEqual(TEXT("Unknown preferred Skill falls back to first"),
		FGridSkillsPageService::ResolveSkillSelection(PageView, TEXT("Skill_Unknown")),
		FName(TEXT("Skill_Acrobatics")));
	TestEqual(TEXT("Empty preferred Skill falls back to first"),
		FGridSkillsPageService::ResolveSkillSelection(PageView, NAME_None),
		FName(TEXT("Skill_Acrobatics")));

	PageView.Skills.Reset();
	TestTrue(TEXT("Empty list resolves no selection"),
		FGridSkillsPageService::ResolveSkillSelection(PageView, TEXT("Skill_Acrobatics")).IsNone());

	UGridSkillsWidget* Widget = NewObject<UGridSkillsWidget>();
	Widget->View.CharacterIndex = 0;
	Widget->View.CharacterId = FGuid::NewGuid();
	Widget->View.Skills = {
		MakeSelectionEntry(TEXT("Skill_Acrobatics")),
		MakeSelectionEntry(TEXT("Skill_Athletics"))
	};

	TestTrue(TEXT("Widget accepts known presentation-only selection"),
		Widget->SelectSkill(TEXT("Skill_Athletics")));
	TestEqual(TEXT("Widget stores selected SkillId"),
		Widget->SelectedSkillId, FName(TEXT("Skill_Athletics")));
	TestFalse(TEXT("Widget rejects unknown SkillId"), Widget->SelectSkill(TEXT("Skill_Unknown")));
	TestEqual(TEXT("Rejected selection preserves current SkillId"),
		Widget->SelectedSkillId, FName(TEXT("Skill_Athletics")));
	Widget->ClearSkillSelection();
	TestTrue(TEXT("Clear selection resets SkillId"), Widget->SelectedSkillId.IsNone());
	return true;
}

#endif

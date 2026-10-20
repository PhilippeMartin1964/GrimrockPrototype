#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGAlchemistAuthoring.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGMageAuthoring.h"
#include "RPG/RPGPriestAuthoring.h"
#include "RPG/RPGRangerAuthoring.h"
#include "RPG/RPGRogueAuthoring.h"
#include "RPG/RPGWarriorAuthoring.h"
#include "UI/RPGTalentPresentationAsset.h"
#include "UI/UIRPGTalentPresentationAuthoring.h"

namespace UIRPGCONTENTTEXT011
{
	struct FClassSpec
	{
		FName ClassId = NAME_None;
		const TCHAR* ObjectPath = nullptr;
	};

	const FClassSpec ClassSpecs[] = {
		{ TEXT("Warrior"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Warrior.DA_Class_Warrior") },
		{ TEXT("Rogue"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Rogue.DA_Class_Rogue") },
		{ TEXT("Ranger"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.DA_Class_Ranger") },
		{ TEXT("Mage"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Mage.DA_Class_Mage") },
		{ TEXT("Priest"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Priest.DA_Class_Priest") },
		{ TEXT("Alchemist"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Alchemist.DA_Class_Alchemist") }
	};

	const FGridCombatActionDefinition* FindAction(const URPGClassAsset& ClassAsset, FName ActionId)
	{
		return ClassAsset.CombatActions.FindByPredicate(
			[ActionId](const FGridCombatActionDefinition& Action)
			{
				return Action.ActionId == ActionId;
			});
	}

	bool ConfigureFromCurrentAuthoring(FName ClassId, URPGClassAsset& ClassAsset, FString& OutError)
	{
		OutError.Reset();

		if (ClassId == TEXT("Warrior"))
		{
			FRPGWarriorAuthoring::ConfigureClass(ClassAsset);
			return true;
		}
		if (ClassId == TEXT("Rogue"))
		{
			FRPGRogueAuthoring::ConfigureClass(ClassAsset);
			return true;
		}
		if (ClassId == TEXT("Ranger"))
		{
			TArray<FRPGRangerFavoredEnemyCategoryDefinition> Categories;
			if (!FRPGRangerAuthoring::CollectProductionFavoredEnemyCategories(Categories, OutError))
			{
				return false;
			}
			FRPGRangerAuthoring::ConfigureClass(ClassAsset, Categories);
			return true;
		}
		if (ClassId == TEXT("Mage"))
		{
			FRPGMageAuthoring::ConfigureClass(ClassAsset);
			return true;
		}
		if (ClassId == TEXT("Priest"))
		{
			FRPGPriestAuthoring::ConfigureClass(ClassAsset);
			return true;
		}
		if (ClassId == TEXT("Alchemist"))
		{
			FRPGAlchemistAuthoring::ConfigureClass(ClassAsset);
			return true;
		}

		OutError = FString::Printf(TEXT("Unsupported production class '%s'."), *ClassId.ToString());
		return false;
	}


	/** Favored Enemy names are derived from canonical bestiary categories, not preserved class choice copy. */
	bool TryResolveFavoredEnemyCategoryTitle(FName ClassId, FName ChoiceId, FString& OutTitle)
	{
		OutTitle.Reset();
		if (ClassId != TEXT("Ranger") ||
			!ChoiceId.ToString().StartsWith(TEXT("Talent_Ranger_Hunter_FavoredEnemy_")))
		{
			return false;
		}

		TArray<FRPGRangerFavoredEnemyCategoryDefinition> Categories;
		FString Error;
		if (!FRPGRangerAuthoring::CollectProductionFavoredEnemyCategories(Categories, Error))
		{
			return false;
		}
		for (const FRPGRangerFavoredEnemyCategoryDefinition& Category : Categories)
		{
			const FName ExpectedId(*FString::Printf(
				TEXT("Talent_Ranger_Hunter_FavoredEnemy_%s"), *Category.CategoryId.ToString()));
			if (ChoiceId == ExpectedId)
			{
				OutTitle = FString::Printf(TEXT("Ennemi juré — %s"), *Category.DisplayName.ToString());
				return true;
			}
		}
		return false;
	}

	void LogSerializedText(const URPGClassAsset& ClassAsset)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[UI-RPG-CONTENT-TEXT01.1][CLASS] ClassId=%s DisplayName=\"%s\" Description=\"%s\""),
			*ClassAsset.ClassId.ToString(),
			*ClassAsset.DisplayName.ToString(),
			*ClassAsset.Description.ToString());

		for (const FRPGClassProgressionChoiceDefinition& Choice : ClassAsset.ProgressionChoices)
		{
			UE_LOG(
				LogTemp,
				Display,
				TEXT("[UI-RPG-CONTENT-TEXT01.1][CHOICE] ClassId=%s ChoiceId=%s TalentNodeId=%s DisplayName=\"%s\" Description=\"%s\""),
				*ClassAsset.ClassId.ToString(),
				*Choice.ChoiceId.ToString(),
				*Choice.TalentNodeId.ToString(),
				*Choice.DisplayName.ToString(),
				*Choice.Description.ToString());
		}

		for (const FGridCombatActionDefinition& Action : ClassAsset.CombatActions)
		{
			UE_LOG(
				LogTemp,
				Display,
				TEXT("[UI-RPG-CONTENT-TEXT01.1][ACTION] ClassId=%s ActionId=%s DisplayName=\"%s\" Description=\"%s\""),
				*ClassAsset.ClassId.ToString(),
				*Action.ActionId.ToString(),
				*Action.DisplayName.ToString(),
				*Action.Description.ToString());
		}
	}

	void LogTextDrift(
		FName ClassId,
		const TCHAR* Kind,
		FName StableId,
		const FText& SerializedDisplayName,
		const FText& AuthoredDisplayName,
		const FText& SerializedDescription,
		const FText& AuthoredDescription)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[UI-RPG-CONTENT-TEXT01.1][TEXT-DRIFT] ClassId=%s Kind=%s Id=%s SerializedDisplayName=\"%s\" AuthoringDisplayName=\"%s\" SerializedDescription=\"%s\" AuthoringDescription=\"%s\""),
			*ClassId.ToString(),
			Kind,
			*StableId.ToString(),
			*SerializedDisplayName.ToString(),
			*AuthoredDisplayName.ToString(),
			*SerializedDescription.ToString(),
			*AuthoredDescription.ToString());
	}
}

/**
 * UI-RPG-CONTENT-TEXT01.1 is a characterization audit only.
 *
 * It deliberately never saves a package. Production DA_Class_* assets are loaded as
 * the serialized text authority candidate, while current RPG*Authoring ConfigureClass
 * functions are executed only against transient duplicates.
 *
 * TEXT-DRIFT log entries are informational in this audit: a difference is exactly the
 * content that a current rematerialization would overwrite. Mechanical identity drift
 * (missing ChoiceId/ActionId or changed counts) remains a test failure.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGCONTENTTEXT011ClassTextOwnershipAuditTest,
	"Grimrock.UI.RPG.CONTENTTEXT01.OwnershipAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT011ClassTextOwnershipAuditTest::RunTest(const FString&)
{
	using namespace UIRPGCONTENTTEXT011;

	int32 TotalSerializedChoices = 0;
	int32 TotalSerializedActions = 0;
	int32 ChoiceTextDriftCount = 0;
	int32 ActionTextDriftCount = 0;

	for (const FClassSpec& Spec : ClassSpecs)
	{
		URPGClassAsset* Serialized = LoadObject<URPGClassAsset>(nullptr, Spec.ObjectPath);
		if (!TestNotNull(
				*FString::Printf(TEXT("%s serialized DA_Class loads"), *Spec.ClassId.ToString()),
				Serialized))
		{
			continue;
		}

		TestEqual(
			*FString::Printf(TEXT("%s keeps canonical ClassId"), *Spec.ClassId.ToString()),
			Serialized->ClassId,
			Spec.ClassId);
		TestFalse(
			*FString::Printf(TEXT("%s serialized class DisplayName is authored"), *Spec.ClassId.ToString()),
			Serialized->DisplayName.IsEmpty());

		if (Serialized->Description.IsEmpty())
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[UI-RPG-CONTENT-TEXT01.1][CLASS-DESCRIPTION-EMPTY] ClassId=%s"),
				*Spec.ClassId.ToString());
		}

		for (const FRPGClassProgressionChoiceDefinition& Choice : Serialized->ProgressionChoices)
		{
			++TotalSerializedChoices;
			const FString Context = Spec.ClassId.ToString() + TEXT("/") + Choice.ChoiceId.ToString();
			TestFalse(*FString::Printf(TEXT("%s ChoiceId is stable"), *Context), Choice.ChoiceId.IsNone());
			TestFalse(*FString::Printf(TEXT("%s DisplayName is serialized"), *Context), Choice.DisplayName.IsEmpty());
			TestFalse(*FString::Printf(TEXT("%s Description is serialized"), *Context), Choice.Description.IsEmpty());
		}

		for (const FGridCombatActionDefinition& Action : Serialized->CombatActions)
		{
			++TotalSerializedActions;
			const FString Context = Spec.ClassId.ToString() + TEXT("/") + Action.ActionId.ToString();
			TestFalse(*FString::Printf(TEXT("%s ActionId is stable"), *Context), Action.ActionId.IsNone());
			TestFalse(*FString::Printf(TEXT("%s DisplayName is serialized"), *Context), Action.DisplayName.IsEmpty());
			TestFalse(*FString::Printf(TEXT("%s Description is serialized"), *Context), Action.Description.IsEmpty());
		}

		LogSerializedText(*Serialized);

		URPGClassAsset* Reauthored = DuplicateObject<URPGClassAsset>(Serialized, GetTransientPackage());
		if (!TestNotNull(
				*FString::Printf(TEXT("%s transient audit duplicate is created"), *Spec.ClassId.ToString()),
				Reauthored))
		{
			continue;
		}

		const FText SerializedClassDisplayName = Serialized->DisplayName;
		const FText SerializedClassDescription = Serialized->Description;

		FString AuthoringError;
		if (!TestTrue(
				*FString::Printf(TEXT("%s current authoring can rebuild a transient duplicate"), *Spec.ClassId.ToString()),
				ConfigureFromCurrentAuthoring(Spec.ClassId, *Reauthored, AuthoringError)))
		{
			AddError(FString::Printf(TEXT("%s authoring error: %s"), *Spec.ClassId.ToString(), *AuthoringError));
			continue;
		}

		TestTrue(
			*FString::Printf(TEXT("%s ConfigureClass preserves class DisplayName"), *Spec.ClassId.ToString()),
			SerializedClassDisplayName.EqualTo(Reauthored->DisplayName));
		TestTrue(
			*FString::Printf(TEXT("%s ConfigureClass preserves class Description"), *Spec.ClassId.ToString()),
			SerializedClassDescription.EqualTo(Reauthored->Description));

		TestEqual(
			*FString::Printf(TEXT("%s rematerialization preserves ChoiceId record count"), *Spec.ClassId.ToString()),
			Reauthored->ProgressionChoices.Num(),
			Serialized->ProgressionChoices.Num());
		TestEqual(
			*FString::Printf(TEXT("%s rematerialization preserves ActionId record count"), *Spec.ClassId.ToString()),
			Reauthored->CombatActions.Num(),
			Serialized->CombatActions.Num());

		for (const FRPGClassProgressionChoiceDefinition& SerializedChoice : Serialized->ProgressionChoices)
		{
			const FRPGClassProgressionChoiceDefinition* AuthoredChoice =
				Reauthored->FindProgressionChoice(SerializedChoice.ChoiceId);
			if (!TestNotNull(
					*FString::Printf(
						TEXT("%s/%s remains addressable by ChoiceId after rematerialization"),
						*Spec.ClassId.ToString(),
						*SerializedChoice.ChoiceId.ToString()),
					AuthoredChoice))
			{
				continue;
			}

			if (!SerializedChoice.DisplayName.EqualTo(AuthoredChoice->DisplayName) ||
				!SerializedChoice.Description.EqualTo(AuthoredChoice->Description))
			{
				++ChoiceTextDriftCount;
				LogTextDrift(
					Spec.ClassId,
					TEXT("Choice"),
					SerializedChoice.ChoiceId,
					SerializedChoice.DisplayName,
					AuthoredChoice->DisplayName,
					SerializedChoice.Description,
					AuthoredChoice->Description);
			}
		}

		for (const FGridCombatActionDefinition& SerializedAction : Serialized->CombatActions)
		{
			const FGridCombatActionDefinition* AuthoredAction = FindAction(*Reauthored, SerializedAction.ActionId);
			if (!TestNotNull(
					*FString::Printf(
						TEXT("%s/%s remains addressable by ActionId after rematerialization"),
						*Spec.ClassId.ToString(),
						*SerializedAction.ActionId.ToString()),
					AuthoredAction))
			{
				continue;
			}

			if (!SerializedAction.DisplayName.EqualTo(AuthoredAction->DisplayName) ||
				!SerializedAction.Description.EqualTo(AuthoredAction->Description))
			{
				++ActionTextDriftCount;
				LogTextDrift(
					Spec.ClassId,
					TEXT("Action"),
					SerializedAction.ActionId,
					SerializedAction.DisplayName,
					AuthoredAction->DisplayName,
					SerializedAction.Description,
					AuthoredAction->Description);
			}
		}
	}

	TestTrue(TEXT("Serialized ownership audit covers all production progression choices"), TotalSerializedChoices >= 90);
	TestTrue(TEXT("Serialized ownership audit covers class combat actions"), TotalSerializedActions > 0);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT01.1][SUMMARY] SerializedChoices=%d SerializedActions=%d ChoiceTextDrift=%d ActionTextDrift=%d"),
		TotalSerializedChoices,
		TotalSerializedActions,
		ChoiceTextDriftCount,
		ActionTextDriftCount);

	return true;
}


/**
 * UI-RPG-CONTENT-TEXT01.2 proves that ConfigureClass rematerializes mechanics
 * without taking text ownership back from an existing DA_Class record.
 *
 * Sentinels deliberately differ from the bootstrap C++ strings. Mechanical
 * fields are poisoned at the same time so the test also proves that the
 * rematerialization itself still happened.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGCONTENTTEXT012ClassTextPreservationTest,
	"Grimrock.UI.RPG.CONTENTTEXT01.Preservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT012ClassTextPreservationTest::RunTest(const FString&)
{
	using namespace UIRPGCONTENTTEXT011;

	int32 PreservedChoiceCount = 0;
	int32 PreservedActionCount = 0;

	for (const FClassSpec& Spec : ClassSpecs)
	{
		URPGClassAsset* Serialized = LoadObject<URPGClassAsset>(nullptr, Spec.ObjectPath);
		if (!TestNotNull(
				*FString::Printf(TEXT("%s serialized DA_Class loads for preservation test"), *Spec.ClassId.ToString()),
				Serialized))
		{
			continue;
		}

		URPGClassAsset* Working = DuplicateObject<URPGClassAsset>(Serialized, GetTransientPackage());
		if (!TestNotNull(
				*FString::Printf(TEXT("%s transient preservation duplicate is created"), *Spec.ClassId.ToString()),
				Working))
		{
			continue;
		}

		TArray<FName> ChoiceIds;
		for (FRPGClassProgressionChoiceDefinition& Choice : Working->ProgressionChoices)
		{
			ChoiceIds.Add(Choice.ChoiceId);
			Choice.DisplayName = FText::FromString(FString::Printf(
				TEXT("TEXT01.2 Choice Name %s"), *Choice.ChoiceId.ToString()));
			Choice.Description = FText::FromString(FString::Printf(
				TEXT("TEXT01.2 Choice Description %s"), *Choice.ChoiceId.ToString()));
			Choice.MinimumLevel = 777;
		}

		TArray<FName> ActionIds;
		for (FGridCombatActionDefinition& Action : Working->CombatActions)
		{
			ActionIds.Add(Action.ActionId);
			Action.DisplayName = FText::FromString(FString::Printf(
				TEXT("TEXT01.2 Action Name %s"), *Action.ActionId.ToString()));
			Action.Description = FText::FromString(FString::Printf(
				TEXT("TEXT01.2 Action Description %s"), *Action.ActionId.ToString()));
			Action.ActionPointCost = 777;
		}

		FString AuthoringError;
		if (!TestTrue(
				*FString::Printf(TEXT("%s mechanics rematerialize with authored text preservation"), *Spec.ClassId.ToString()),
				ConfigureFromCurrentAuthoring(Spec.ClassId, *Working, AuthoringError)))
		{
			AddError(FString::Printf(TEXT("%s authoring error: %s"), *Spec.ClassId.ToString(), *AuthoringError));
			continue;
		}

		for (const FName ChoiceId : ChoiceIds)
		{
			const FRPGClassProgressionChoiceDefinition* Choice = Working->FindProgressionChoice(ChoiceId);
			if (!TestNotNull(
					*FString::Printf(TEXT("%s/%s survives rematerialization"), *Spec.ClassId.ToString(), *ChoiceId.ToString()),
					Choice))
			{
				continue;
			}

			FString ExpectedDisplayName = FString::Printf(TEXT("TEXT01.2 Choice Name %s"), *ChoiceId.ToString());
			FString CategoryTitle;
			if (TryResolveFavoredEnemyCategoryTitle(Spec.ClassId, ChoiceId, CategoryTitle))
			{
				// The only choice whose name is canonically rebuilt during rematerialization.
				ExpectedDisplayName = CategoryTitle;
			}
			const FString ExpectedDescription = FString::Printf(TEXT("TEXT01.2 Choice Description %s"), *ChoiceId.ToString());
			TestEqual(
				*FString::Printf(TEXT("%s/%s preserves Choice DisplayName"), *Spec.ClassId.ToString(), *ChoiceId.ToString()),
				Choice->DisplayName.ToString(),
				ExpectedDisplayName);
			TestEqual(
				*FString::Printf(TEXT("%s/%s preserves Choice Description"), *Spec.ClassId.ToString(), *ChoiceId.ToString()),
				Choice->Description.ToString(),
				ExpectedDescription);
			TestTrue(
				*FString::Printf(TEXT("%s/%s still rematerializes Choice mechanics"), *Spec.ClassId.ToString(), *ChoiceId.ToString()),
				Choice->MinimumLevel != 777);
			++PreservedChoiceCount;
		}

		for (const FName ActionId : ActionIds)
		{
			const FGridCombatActionDefinition* Action = FindAction(*Working, ActionId);
			if (!TestNotNull(
					*FString::Printf(TEXT("%s/%s survives rematerialization"), *Spec.ClassId.ToString(), *ActionId.ToString()),
					Action))
			{
				continue;
			}

			const FString ExpectedDisplayName = FString::Printf(TEXT("TEXT01.2 Action Name %s"), *ActionId.ToString());
			const FString ExpectedDescription = FString::Printf(TEXT("TEXT01.2 Action Description %s"), *ActionId.ToString());
			TestEqual(
				*FString::Printf(TEXT("%s/%s preserves Action DisplayName"), *Spec.ClassId.ToString(), *ActionId.ToString()),
				Action->DisplayName.ToString(),
				ExpectedDisplayName);
			TestEqual(
				*FString::Printf(TEXT("%s/%s preserves Action Description"), *Spec.ClassId.ToString(), *ActionId.ToString()),
				Action->Description.ToString(),
				ExpectedDescription);
			TestTrue(
				*FString::Printf(TEXT("%s/%s still rematerializes Action mechanics"), *Spec.ClassId.ToString(), *ActionId.ToString()),
				Action->ActionPointCost != 777);
			++PreservedActionCount;
		}
	}

	TestTrue(TEXT("Preservation test covers every production class choice family"), PreservedChoiceCount >= 90);
	TestTrue(TEXT("Preservation test covers production class combat actions"), PreservedActionCount > 0);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT01.2][SUMMARY] PreservedChoices=%d PreservedActions=%d"),
		PreservedChoiceCount,
		PreservedActionCount);

	return true;
}


/**
 * UI-RPG-CONTENT-TEXT01.3 keeps C++ as a mechanics/bootstrap producer only.
 * A fresh class has no serialized text to preserve, so every generated Choice
 * and class Action must receive the deterministic technical bootstrap marker.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGCONTENTTEXT013BootstrapOnlyTest,
	"Grimrock.UI.RPG.CONTENTTEXT01.BootstrapOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT013BootstrapOnlyTest::RunTest(const FString&)
{
	using namespace UIRPGCONTENTTEXT011;

	int32 BootstrapChoiceCount = 0;
	int32 BootstrapActionCount = 0;

	for (const FClassSpec& Spec : ClassSpecs)
	{
		URPGClassAsset* Fresh = NewObject<URPGClassAsset>(GetTransientPackage());
		Fresh->ClassId = Spec.ClassId;
		Fresh->HealthAtLevelOne = 10;

		FString AuthoringError;
		if (!TestTrue(
				*FString::Printf(TEXT("%s fresh mechanics can be authored"), *Spec.ClassId.ToString()),
				ConfigureFromCurrentAuthoring(Spec.ClassId, *Fresh, AuthoringError)))
		{
			AddError(FString::Printf(TEXT("%s authoring error: %s"), *Spec.ClassId.ToString(), *AuthoringError));
			continue;
		}

		TestTrue(
			*FString::Printf(TEXT("%s fresh mechanics remain structurally valid"), *Spec.ClassId.ToString()),
			Fresh->IsValidDefinition());

		for (const FRPGClassProgressionChoiceDefinition& Choice : Fresh->ProgressionChoices)
		{
			const FString StableId = Choice.ChoiceId.ToString();
			FString CanonicalCategoryTitle;
			const bool bCategoryOwned = TryResolveFavoredEnemyCategoryTitle(
				Spec.ClassId, Choice.ChoiceId, CanonicalCategoryTitle);
			TestEqual(
				*FString::Printf(TEXT("%s/%s Choice bootstrap name uses its correct text authority"), *Spec.ClassId.ToString(), *StableId),
				Choice.DisplayName.ToString(),
				bCategoryOwned ? CanonicalCategoryTitle : StableId);
			TestEqual(
				*FString::Printf(TEXT("%s/%s Choice bootstrap description is technical only"), *Spec.ClassId.ToString(), *StableId),
				Choice.Description.ToString(),
				FString::Printf(TEXT("AUTHOR_TEXT_REQUIRED:%s"), *StableId));
			++BootstrapChoiceCount;
		}

		for (const FGridCombatActionDefinition& Action : Fresh->CombatActions)
		{
			const FString StableId = Action.ActionId.ToString();
			TestEqual(
				*FString::Printf(TEXT("%s/%s Action bootstrap name is its stable id"), *Spec.ClassId.ToString(), *StableId),
				Action.DisplayName.ToString(),
				StableId);
			TestEqual(
				*FString::Printf(TEXT("%s/%s Action bootstrap description is technical only"), *Spec.ClassId.ToString(), *StableId),
				Action.Description.ToString(),
				FString::Printf(TEXT("AUTHOR_TEXT_REQUIRED:%s"), *StableId));
			++BootstrapActionCount;
		}
	}

	TestTrue(TEXT("Bootstrap-only audit covers every production class choice family"), BootstrapChoiceCount >= 90);
	TestTrue(TEXT("Bootstrap-only audit covers production class combat actions"), BootstrapActionCount > 0);
	return true;
}


/**
 * UI-RPG-CONTENT-TEXT01.4 closes the remaining Talent presentation text writer.
 *
 * DA_RPGTalentPresentation is the serialized authority for branch names and
 * the four grouped conceptual-node overrides. ConfigureCatalog may rebuild
 * structure/colors, but must preserve existing text by stable IDs. Fresh IDs
 * receive only the same explicit technical bootstrap used by DA_Class authoring.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGCONTENTTEXT014FinalOwnershipAuditTest,
	"Grimrock.UI.RPG.CONTENTTEXT01.FinalAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT014FinalOwnershipAuditTest::RunTest(const FString&)
{
	URPGTalentPresentationAsset* Serialized =
		LoadObject<URPGTalentPresentationAsset>(nullptr, FUIRPGTalentPresentationAuthoring::ObjectPath());
	if (!TestNotNull(TEXT("Serialized DA_RPGTalentPresentation loads"), Serialized))
	{
		return false;
	}
	TestTrue(TEXT("Serialized presentation catalog is structurally valid"), Serialized->IsValidDefinition());

	int32 SerializedBranchCount = 0;
	int32 SerializedOverrideCount = 0;
	for (const FRPGClassPresentationDefinition& ClassPresentation : Serialized->Classes)
	{
		SerializedBranchCount += ClassPresentation.Branches.Num();
		for (const FRPGTalentBranchPresentationDefinition& Branch : ClassPresentation.Branches)
		{
			TestFalse(
				*FString::Printf(TEXT("%s/%s serialized branch DisplayName is authored"),
					*ClassPresentation.ClassId.ToString(), *Branch.TalentBranchId.ToString()),
				Branch.DisplayName.IsEmpty());

			for (const FRPGTalentNodePresentationDefinition& Node : Branch.NodePresentationOverrides)
			{
				++SerializedOverrideCount;
				TestFalse(
					*FString::Printf(TEXT("%s/%s/%s serialized conceptual DisplayName is authored"),
						*ClassPresentation.ClassId.ToString(), *Branch.TalentBranchId.ToString(), *Node.TalentNodeId.ToString()),
					Node.DisplayName.IsEmpty());
				TestFalse(
					*FString::Printf(TEXT("%s/%s/%s serialized conceptual Description is authored"),
						*ClassPresentation.ClassId.ToString(), *Branch.TalentBranchId.ToString(), *Node.TalentNodeId.ToString()),
					Node.Description.IsEmpty());
			}
		}
	}

	TestEqual(TEXT("Presentation text authority covers eighteen branches"), SerializedBranchCount, 18);
	TestEqual(TEXT("Presentation text authority covers four grouped conceptual nodes"), SerializedOverrideCount, 4);

	URPGTalentPresentationAsset* Working =
		DuplicateObject<URPGTalentPresentationAsset>(Serialized, GetTransientPackage());
	if (!TestNotNull(TEXT("Transient presentation duplicate is created"), Working))
	{
		return false;
	}

	for (FRPGClassPresentationDefinition& ClassPresentation : Working->Classes)
	{
		for (FRPGTalentBranchPresentationDefinition& Branch : ClassPresentation.Branches)
		{
			const FString Prefix =
				ClassPresentation.ClassId.ToString() + TEXT("/") + Branch.TalentBranchId.ToString();
			Branch.DisplayName = FText::FromString(TEXT("TEXT01.4 Branch Name ") + Prefix);
			Branch.ShortDescription = FText::FromString(TEXT("TEXT01.4 Branch Description ") + Prefix);

			for (FRPGTalentNodePresentationDefinition& Node : Branch.NodePresentationOverrides)
			{
				const FString NodePrefix = Prefix + TEXT("/") + Node.TalentNodeId.ToString();
				Node.DisplayName = FText::FromString(TEXT("TEXT01.4 Node Name ") + NodePrefix);
				Node.Description = FText::FromString(TEXT("TEXT01.4 Node Description ") + NodePrefix);
			}
		}
	}

	FUIRPGTalentPresentationAuthoring::ConfigureCatalog(*Working);
	TestTrue(TEXT("Presentation rematerialization remains structurally valid"), Working->IsValidDefinition());

	for (const FRPGClassPresentationDefinition& SerializedClass : Serialized->Classes)
	{
		const FRPGClassPresentationDefinition* RematerializedClass = Working->FindClass(SerializedClass.ClassId);
		if (!TestNotNull(
				*FString::Printf(TEXT("%s class presentation survives rematerialization"), *SerializedClass.ClassId.ToString()),
				RematerializedClass))
		{
			continue;
		}

		for (const FRPGTalentBranchPresentationDefinition& SerializedBranch : SerializedClass.Branches)
		{
			const FRPGTalentBranchPresentationDefinition* Branch =
				RematerializedClass->FindBranch(SerializedBranch.TalentBranchId);
			if (!TestNotNull(
					*FString::Printf(TEXT("%s/%s branch survives rematerialization"),
						*SerializedClass.ClassId.ToString(), *SerializedBranch.TalentBranchId.ToString()),
					Branch))
			{
				continue;
			}

			const FString Prefix =
				SerializedClass.ClassId.ToString() + TEXT("/") + SerializedBranch.TalentBranchId.ToString();
			TestEqual(
				*FString::Printf(TEXT("%s preserves branch DisplayName"), *Prefix),
				Branch->DisplayName.ToString(),
				TEXT("TEXT01.4 Branch Name ") + Prefix);
			TestEqual(
				*FString::Printf(TEXT("%s preserves branch ShortDescription"), *Prefix),
				Branch->ShortDescription.ToString(),
				TEXT("TEXT01.4 Branch Description ") + Prefix);

			for (const FRPGTalentNodePresentationDefinition& SerializedNode : SerializedBranch.NodePresentationOverrides)
			{
				const FRPGTalentNodePresentationDefinition* Node =
					Branch->FindNodeOverride(SerializedNode.TalentNodeId);
				if (!TestNotNull(
						*FString::Printf(TEXT("%s/%s conceptual override survives rematerialization"),
							*Prefix, *SerializedNode.TalentNodeId.ToString()),
						Node))
				{
					continue;
				}

				const FString NodePrefix = Prefix + TEXT("/") + SerializedNode.TalentNodeId.ToString();
				TestEqual(
					*FString::Printf(TEXT("%s preserves conceptual DisplayName"), *NodePrefix),
					Node->DisplayName.ToString(),
					TEXT("TEXT01.4 Node Name ") + NodePrefix);
				TestEqual(
					*FString::Printf(TEXT("%s preserves conceptual Description"), *NodePrefix),
					Node->Description.ToString(),
					TEXT("TEXT01.4 Node Description ") + NodePrefix);
			}
		}
	}

	URPGTalentPresentationAsset* Fresh = NewObject<URPGTalentPresentationAsset>(GetTransientPackage());
	FUIRPGTalentPresentationAuthoring::ConfigureCatalog(*Fresh);
	TestTrue(TEXT("Fresh presentation bootstrap is structurally valid"), Fresh->IsValidDefinition());

	int32 BootstrapBranchCount = 0;
	int32 BootstrapOverrideCount = 0;
	for (const FRPGClassPresentationDefinition& ClassPresentation : Fresh->Classes)
	{
		for (const FRPGTalentBranchPresentationDefinition& Branch : ClassPresentation.Branches)
		{
			const FString StableId = Branch.TalentBranchId.ToString();
			TestEqual(TEXT("Fresh branch bootstrap name is stable id"), Branch.DisplayName.ToString(), StableId);
			TestEqual(
				TEXT("Fresh branch bootstrap description is technical only"),
				Branch.ShortDescription.ToString(),
				FString::Printf(TEXT("AUTHOR_TEXT_REQUIRED:%s"), *StableId));
			++BootstrapBranchCount;

			for (const FRPGTalentNodePresentationDefinition& Node : Branch.NodePresentationOverrides)
			{
				const FString NodeId = Node.TalentNodeId.ToString();
				TestEqual(TEXT("Fresh conceptual bootstrap name is stable id"), Node.DisplayName.ToString(), NodeId);
				TestEqual(
					TEXT("Fresh conceptual bootstrap description is technical only"),
					Node.Description.ToString(),
					FString::Printf(TEXT("AUTHOR_TEXT_REQUIRED:%s"), *NodeId));
				++BootstrapOverrideCount;
			}
		}
	}

	TestEqual(TEXT("Fresh bootstrap covers eighteen branches"), BootstrapBranchCount, 18);
	TestEqual(TEXT("Fresh bootstrap covers four grouped conceptual nodes"), BootstrapOverrideCount, 4);
	return true;
}

#endif

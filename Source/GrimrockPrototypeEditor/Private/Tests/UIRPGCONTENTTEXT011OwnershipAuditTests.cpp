#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGAlchemistAuthoring.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGMageAuthoring.h"
#include "RPG/RPGPriestAuthoring.h"
#include "RPG/RPGRangerAuthoring.h"
#include "RPG/RPGRogueAuthoring.h"
#include "RPG/RPGWarriorAuthoring.h"

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

#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/PackageName.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace GridTD07352ShurikenRepair
{
	const TCHAR* ShurikenAssetPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Shuriken.DA_Weapon_Shuriken");

	bool IsCanonicalShurikenAction(const FGridCombatActionDefinition& Action)
	{
		return Action.IsValid() &&
			Action.ActionId == FName(TEXT("Attack_Shuriken")) &&
			Action.ActionType == EGridCombatActionType::RangedAttack &&
			Action.SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
			Action.TargetingPolicy == EGridCombatTargetingPolicy::FirstAxialTarget &&
			Action.ResolutionProfile == EGridCombatActionResolutionProfile::Attack &&
			Action.ActionPointCost == 2 &&
			Action.ResourceCosts.SourceItemQuantityCost == 1 &&
			Action.RangeCells == 3 &&
			Action.PresentationProfileId == FName(TEXT("Attack_Shuriken")) &&
			Action.OffensiveProfile.AttackId == FName(TEXT("Attack_Shuriken")) &&
			Action.OffensiveProfile.AttackDefinition.DamageType == EGridDamageType::Physical &&
			Action.OffensiveProfile.AttackDefinition.PhysicalSubtype == EGridPhysicalDamageSubtype::Piercing &&
			Action.OffensiveProfile.AttackDefinition.MinDamage == 1 &&
			Action.OffensiveProfile.AttackDefinition.MaxDamage == 4 &&
			Action.OffensiveProfile.AttackDefinition.AccuracyBonus == 0 &&
			Action.OffensiveProfile.FlatDamageBonus == 0 &&
			Action.OffensiveProfile.DamageScalingAttribute == EGridAttackScalingAttribute::Dexterity &&
			Action.OffensiveProfile.RangeCells == 3;
	}

	bool SaveAssetPackage(UGridItemDefinitionAsset* Asset)
	{
		if (!IsValid(Asset))
		{
			return false;
		}

		UPackage* Package = Asset->GetOutermost();
		if (!Package)
		{
			return false;
		}

		const FString Filename =
			FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		SaveArgs.Error = GError;
		return UPackage::SavePackage(Package, Asset, *Filename, SaveArgs);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07352ShurikenAssetRepairTest,
	"Grimrock.TechnicalDebt.TD07_3_5_2.AssetRepair.ShurikenCombatActions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07352ShurikenAssetRepairTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07352ShurikenRepair;

	UGridItemDefinitionAsset* Shuriken =
		LoadObject<UGridItemDefinitionAsset>(nullptr, ShurikenAssetPath);
	TestNotNull(TEXT("DA_Weapon_Shuriken loads"), Shuriken);
	if (!Shuriken)
	{
		return false;
	}

	TestEqual(TEXT("Shuriken business id is stable"), Shuriken->ItemDefinitionId, FName(TEXT("Shuriken")));
	TestTrue(TEXT("Shuriken is throwable"), Shuriken->bThrowable);
	TestTrue(TEXT("Shuriken can equip MainHand"), Shuriken->CompatibleEquipmentSlots.Contains(EGridEquipmentSlot::MainHand));

	const FGridCombatActionDefinition* ExistingCanonicalAction =
		Shuriken->CombatActions.FindByPredicate(
			[](const FGridCombatActionDefinition& Action)
			{
				return IsCanonicalShurikenAction(Action);
			});

	bool bChanged = false;
	if (!ExistingCanonicalAction)
	{
		TestTrue(TEXT("Pre-repair Shuriken still exposes valid legacy offense"), Shuriken->HasValidOffensiveProfile());
		if (!Shuriken->HasValidOffensiveProfile())
		{
			return false;
		}

		const FGridCombatActionDefinition CanonicalAction =
			FGridCombatActionCatalog::MakeLegacyEquipmentAttackDefinition(*Shuriken, 2);
		TestTrue(TEXT("Legacy Shuriken profile converts to the exact canonical CombatAction"), IsCanonicalShurikenAction(CanonicalAction));
		if (!IsCanonicalShurikenAction(CanonicalAction))
		{
			return false;
		}

		Shuriken->Modify();
		Shuriken->CombatActions.Reset();
		Shuriken->CombatActions.Add(CanonicalAction);
		Shuriken->bProvidesAttack = false;
		Shuriken->OffensiveProfile = FGridOffensiveEquipmentProfile();
		Shuriken->MarkPackageDirty();
		bChanged = true;
	}
	else if (Shuriken->bProvidesAttack || !Shuriken->OffensiveProfile.AttackId.IsNone())
	{
		Shuriken->Modify();
		Shuriken->bProvidesAttack = false;
		Shuriken->OffensiveProfile = FGridOffensiveEquipmentProfile();
		Shuriken->MarkPackageDirty();
		bChanged = true;
	}

	if (bChanged)
	{
		TestTrue(TEXT("Repaired Shuriken package saves"), SaveAssetPackage(Shuriken));
	}

	TestTrue(TEXT("Shuriken contains valid CombatActions after repair"), Shuriken->HasValidCombatActions());
	TestTrue(TEXT("Shuriken definition is valid after repair"), Shuriken->IsValidDefinition());
	TestFalse(TEXT("Legacy bProvidesAttack is cleared"), Shuriken->bProvidesAttack);
	TestTrue(TEXT("Legacy item-level OffensiveProfile is cleared"), Shuriken->OffensiveProfile.AttackId.IsNone());

	const FGridCombatActionDefinition* CanonicalAction =
		Shuriken->CombatActions.FindByPredicate(
			[](const FGridCombatActionDefinition& Action)
			{
				return IsCanonicalShurikenAction(Action);
			});
	TestNotNull(TEXT("Canonical Attack_Shuriken CombatAction exists"), CanonicalAction);

	AddInfo(bChanged
		? TEXT("DA_Weapon_Shuriken was converted and saved to CombatActions-only authoring.")
		: TEXT("DA_Weapon_Shuriken was already CombatActions-only; no package mutation was required."));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

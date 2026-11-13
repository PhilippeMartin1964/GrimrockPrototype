#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean06ActionProjection.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062HGeneratedActionProjectionTest,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionGeneratedActionProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062HGeneratedActionProjectionTest::RunTest(const FString&)
{
	TArray<FGridWeaponPipelineRow> Manifest;
	FString Error;
	if (!TestTrue(TEXT("Canonical production manifest is valid"),
		FGridWeaponPipelineManifest::Load(
			FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
	{
		AddError(Error);
		return false;
	}
	if (!TestEqual(TEXT("22 ARM production entries"), Manifest.Num(), 22))
	{
		return false;
	}
	const UGridWeaponCatalogueAsset* Catalogue = LoadObject<UGridWeaponCatalogueAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
	if (!TestNotNull(TEXT("Source compact catalogue"), Catalogue) ||
		!TestTrue(TEXT("82 ARM progressions"), Catalogue->IsValidCatalogue()))
	{
		return false;
	}
	int32 Standard = 0;
	int32 Customized = 0;
	int32 Passive = 0;
	int32 Unexpected = 0;
	int32 Ranged = 0;
	int32 QuantityCost = 0;
	TSet<FName> Seen;
	TStrongObjectPtr<UGridActionCatalogueAsset> TransientCatalogue(
		NewObject<UGridActionCatalogueAsset>(GetTransientPackage()));
	for (const FGridWeaponPipelineRow& Row : Manifest)
	{
		const FName ArmId(*Row.ArchetypeId);
		if (Seen.Contains(ArmId) || Row.ProductionState != TEXT("Active"))
		{
			AddError(TEXT("Unexpected duplicate/inactive ARM row"));
			return false;
		}
		Seen.Add(ArmId);
		const UGridItemDefinitionAsset* Item =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
		const FGridWeaponLevelProfile* N1 = Catalogue->FindProfile(ArmId, 1);
		if (!IsValid(Item) || !N1 || Item->WeaponArchetypeId != ArmId ||
			Item->ItemDefinitionId != FName(*Row.ItemDefinitionId))
		{
			AddError(TEXT("ARM item catalogue identity cannot be proven"));
			return false;
		}
		const bool bWasDirty = Item->GetOutermost()->IsDirty();
		if (Row.IsPassive())
		{
			++Passive;
			if (!Item->CombatActions.IsEmpty())
			{
				AddError(TEXT("Passive ARM unexpectedly contains a combat action"));
				return false;
			}
		}
		else
		{
			const FGridCombatActionDefinition* Primary = Item->CombatActions.FindByPredicate(
				[&Row](const FGridCombatActionDefinition& Action)
				{
					return Action.ActionId == FName(*Row.AttackId);
				});
			if (!Primary || !Primary->IsValid())
			{
				AddError(TEXT("Active ARM primary action not authoritatively present"));
				return false;
			}
			FGridCanonicalAction Projected;
			switch (GridArmClean06ActionProjection::TryProjectGeneratedAttack(
				Row, *Item, *N1, *Primary, Projected))
			{
			case GridArmClean06ActionProjection::EResult::ProvenStandard:
				++Standard;
				if (Projected.ActionId != Primary->ActionId ||
					Projected.Targeting != Primary->TargetingPolicy ||
					Projected.bRangedAttack != (Primary->ActionType == EGridCombatActionType::RangedAttack) ||
					Projected.bRequiresLineOfSight != Primary->bRequiresLineOfSight ||
					Projected.SourceItemQuantityCost != Primary->ResourceCosts.SourceItemQuantityCost ||
					Projected.ManaCost != Primary->ResourceCosts.ManaCost ||
					Projected.CooldownRounds != Primary->CooldownRounds)
				{
					AddError(TEXT("Projected ARM action changed a supported mechanism"));
					return false;
				}
				if (Projected.bRangedAttack) { ++Ranged; }
				if (Projected.SourceItemQuantityCost != 0) { ++QuantityCost; }
				TransientCatalogue->Actions.Add(Projected);
				break;
			case GridArmClean06ActionProjection::EResult::NonStandardAction:
				++Customized;
				break;
			default:
				++Unexpected;
				break;
			}
		}
		if (Item->GetOutermost()->IsDirty() != bWasDirty)
		{
			AddError(TEXT("Projection modified a production asset package"));
			return false;
		}
	}
	TArray<FString> Errors;
	TestTrue(TEXT("Projected action candidates use valid canonical schema"),
		TransientCatalogue->Validate(Errors));
	TestEqual(TEXT("All 22 production ARM entries classified"), Standard + Customized + Passive + Unexpected, 22);
	TestEqual(TEXT("Three passive items have no attack projection"), Passive, 3);
	TestEqual(TEXT("No unexpected input or missing identity"), Unexpected, 0);
	AddInfo(FString::Printf(TEXT(
		"ARM 06.2H: 22 inputs; %d standard fully proven; %d customized explicitly preserved;"
		" %d passive; %d ranged; %d source-item consumption. No assets written."),
		Standard, Customized, Passive, Ranged, QuantityCost));
	// A nonstandard action is not a failure: it is a mandatory explicit
	// conversion case later, never a reason to drop the original definition.
	return Errors.IsEmpty() && Unexpected == 0 &&
		Standard + Customized + Passive == 22 && Passive == 3;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062HRejectModifiedActionTest,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.RejectModifiedActionProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062HRejectModifiedActionTest::RunTest(const FString&)
{
	FGridWeaponPipelineRow Row;
	Row.ArchetypeId = TEXT("ARM014");
	Row.BuildKind = TEXT("MeleeOneHand");
	Row.AttackId = TEXT("TestAttack");
	Row.ItemDefinitionId = TEXT("TestSword");
	TStrongObjectPtr<UGridItemDefinitionAsset> Item(
		NewObject<UGridItemDefinitionAsset>(GetTransientPackage()));
	Item->ItemDefinitionId = TEXT("TestSword");
	Item->WeaponArchetypeId = TEXT("ARM014");
	FGridWeaponLevelProfile N1;
	N1.ArchetypeId = TEXT("ARM014");
	N1.DisplayName = FText::FromString(TEXT("Test Sword"));
	N1.Family = TEXT("Sword");
	N1.HandUsage = TEXT("1 main");
	N1.WeightKg = 1.0f;
	N1.ActionPointCost = 2;
	N1.RangeCells = 1;
	N1.DamageDice.bEnabled = true;
	N1.DamageDice.DiceCount = 1;
	N1.DamageDice.DiceSides = 6;
	FGridAttackDefinition Damage;
	if (!TestTrue(TEXT("Synthetic N1 yields valid attack"), N1.TryBuildAttackDefinition(Damage)))
	{
		return false;
	}
	FGridCombatActionDefinition Attack;
	Attack.ActionId = TEXT("TestAttack");
	Attack.DisplayName = N1.DisplayName;
	Attack.ActionType = EGridCombatActionType::MeleeAttack;
	Attack.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
	Attack.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
	Attack.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
	Attack.ActionPointCost = 2;
	Attack.RangeCells = 1;
	Attack.OffensiveProfile.AttackId = Attack.ActionId;
	Attack.OffensiveProfile.AttackDefinition = Damage;
	Attack.OffensiveProfile.DamageScalingAttribute = N1.ScalingAttribute;
	Attack.OffensiveProfile.RangeCells = 1;
	FGridCanonicalAction Out;
	using EResult = GridArmClean06ActionProjection::EResult;
	TestEqual(TEXT("Generated baseline is representable"),
		GridArmClean06ActionProjection::TryProjectGeneratedAttack(Row, *Item, N1, Attack, Out),
		EResult::ProvenStandard);
	Attack.CooldownRounds = 2;
	TestEqual(TEXT("A source custom cooldown must not silently disappear"),
		GridArmClean06ActionProjection::TryProjectGeneratedAttack(Row, *Item, N1, Attack, Out),
		EResult::NonStandardAction);
	TestTrue(TEXT("Failed projection clears the previous ActionId"),
		Out.ActionId.IsNone());
	Attack.CooldownRounds = 0;
	Attack.Requirements.Add(TEXT("Skill_Requisite"));
	TestEqual(TEXT("A source requirement must not silently disappear"),
		GridArmClean06ActionProjection::TryProjectGeneratedAttack(Row, *Item, N1, Attack, Out),
		EResult::NonStandardAction);
	return true;
}
#endif

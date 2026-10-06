#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatReactionResolver.h"
#include "Runtime/Combat/GridCombatSurfaceResolver.h"
#include "Runtime/GridInventoryTypes.h"

namespace RPG0394F1
{
	URPGClassAsset* MakeAffinityClass()
	{
		URPGClassAsset* ClassAsset = NewObject<URPGClassAsset>(GetTransientPackage());
		ClassAsset->ClassId = TEXT("Class_RPG0394F1_Affinity");
		ClassAsset->DisplayName = FText::FromString(TEXT("Affinity source"));
		ClassAsset->HealthAtLevelOne = 10;

		FRPGClassProgressionLevelGrant Grant;
		Grant.Level = 1;
		Grant.ChoicePointsGranted = 1;
		ClassAsset->ProgressionLevelGrants.Add(Grant);

		for (const FName ChoiceId : { FName(TEXT("Affinity_Fire")), FName(TEXT("Affinity_Air")) })
		{
			FRPGClassProgressionChoiceDefinition Choice;
			Choice.ChoiceId = ChoiceId;
			Choice.DisplayName = FText::FromName(ChoiceId);
			Choice.MinimumLevel = 1;
			Choice.PointCost = 1;
			Choice.ExclusiveChoiceGroupId = TEXT("Affinity_Group");
			ClassAsset->ProgressionChoices.Add(Choice);
		}
		return ClassAsset;
	}

	FGridCombatActionDefinition MakeSurfaceAction()
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = TEXT("Action_RPG0394F1_SurfaceVariant");
		Action.DisplayName = FText::FromString(TEXT("Surface variant"));
		Action.ActionType = EGridCombatActionType::Ability;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
		Action.TargetingPolicy = EGridCombatTargetingPolicy::Cell;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Action.ActionPointCost = 2;
		Action.ResourceCosts.ManaCost = 5;
		Action.RangeCells = 4;

		FGridCombatActionOwnerVariantProfile Fire;
		Fire.RequiredOwnerRequirementIds = { TEXT("Affinity_Fire") };
		FGridCombatSurfaceEffectProfile FireSurface;
		FireSurface.SurfaceType = EGridCombatSurfaceType::Fire;
		FireSurface.DurationRounds = 3;
		Fire.SurfaceEffects.Add(FireSurface);
		Action.OwnerVariants.Add(Fire);

		FGridCombatActionOwnerVariantProfile Air;
		Air.RequiredOwnerRequirementIds = { TEXT("Affinity_Air") };
		FGridCombatSurfaceConversionProfile Electrify;
		Electrify.InputSurfaceTypes = { EGridCombatSurfaceType::Water, EGridCombatSurfaceType::Blood };
		Electrify.OutputSurfaceType = EGridCombatSurfaceType::ElectrifiedWater;
		Air.SurfaceConversions.Add(Electrify);
		Action.OwnerVariants.Add(Air);
		return Action;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394F1ReactionSecondaryDamageTest,
	"Grimrock.RPG.RPG03.9.4F1.ReactionSecondaryDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394F1ReactionSecondaryDamageTest::RunTest(const FString&)
{
	using namespace RPG0394F1;
	URPGClassAsset* SourceClass = MakeAffinityClass();
	TestTrue(TEXT("Affinity source class is valid"), SourceClass->IsValidDefinition());

	FGridCharacterInventoryState SourceMage;
	SourceMage.CharacterId = FGuid::NewGuid();
	SourceMage.ClassId = SourceClass->ClassId;
	SourceMage.ClassDefinition = SourceClass;
	SourceMage.Level = 1;
	SourceMage.Attributes.Intelligence = 16;
	SourceMage.SelectedClassProgressionChoiceIds = { TEXT("Affinity_Fire") };

	UGridStatusEffectDefinitionAsset* Imbuement = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	Imbuement->EffectId = TEXT("Status_RPG0394F1_Imbuement");
	Imbuement->DisplayName = FText::FromString(TEXT("Imbuement"));
	Imbuement->DurationUnit = EGridStatusEffectDurationUnit::Rounds;
	Imbuement->DefaultDuration = 2;
	Imbuement->StackPolicy = EGridStatusEffectStackPolicy::NoStack;

	FGridCombatReactionProfile FireReaction;
	FireReaction.ReactionId = TEXT("Reaction_RPG0394F1_Imbuement_Fire");
	FireReaction.Trigger = EGridCombatReactionTrigger::AttackHit;
	FireReaction.Limit = EGridCombatReactionLimit::OncePerAction;
	FireReaction.RequiredStatusSourceRequirementIds = { TEXT("Affinity_Fire") };
	FireReaction.bRequireWeaponAttack = true;
	FireReaction.bConsumeOwningStatus = true;
	FireReaction.SecondaryDirectDamage = 3;
	FireReaction.SecondaryDirectDamageType = EGridDamageType::Fire;
	FireReaction.SecondaryDirectDamageScalingAttribute = EGridAttackScalingAttribute::Intelligence;
	FireReaction.SecondaryDirectDamageAttributeModifierScale = 1;
	Imbuement->CombatReactions.Add(FireReaction);
	TestTrue(TEXT("Cross-owner imbuement status is structurally valid"), Imbuement->IsValidDefinition());

	FGridCharacterInventoryState Ally;
	Ally.CharacterId = FGuid::NewGuid();
	Ally.Attributes.Intelligence = 8;
	FGridStatusEffectApplyResult ApplyResult;
	FString ApplyError;
	TestTrue(TEXT("Mage can be the source of an ally-held imbuement status"),
		Ally.StatusEffects.TryApply(*Imbuement, SourceMage.CharacterId, ApplyResult, ApplyError));

	FGridPartyInventoryState Party;
	Party.ActiveCharacters = { SourceMage, Ally };
	TArray<FGridCombatReactionBinding> Bindings;
	TestTrue(TEXT("Cross-owner status reactions project from authoritative source progression"),
		FGridCombatReactionResolver::CollectCharacterBindings(Party.ActiveCharacters[1], Party, Bindings));
	TestEqual(TEXT("Exactly the Fire source requirement reaction projects"), Bindings.Num(), 1);
	if (Bindings.Num() != 1)
	{
		return false;
	}
	TestEqual(TEXT("Binding preserves the status source Mage"), Bindings[0].OwningStatusSourceId, SourceMage.CharacterId);
	TestTrue(TEXT("Source requirements are consumed before runtime matching"),
		Bindings[0].Profile.RequiredStatusSourceRequirementIds.IsEmpty());

	FGridCombatReactionEvent Event;
	Event.EventId = FGuid::NewGuid();
	Event.ActionInstanceId = FGuid::NewGuid();
	Event.RoundNumber = 1;
	Event.Trigger = EGridCombatReactionTrigger::AttackHit;
	Event.SourceCombatantId = Ally.CharacterId;
	Event.TargetCombatantId = FGuid::NewGuid();
	Event.ActionId = TEXT("Attack_RPG0394F1");
	Event.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
	Event.ActionType = EGridCombatActionType::MeleeAttack;
	Event.DamageType = EGridDamageType::Physical;
	Event.bOffensiveAction = true;
	Event.bWeaponAttack = false;

	FGridCombatReactionLedger Ledger;
	TArray<FGridCombatReactionMatch> Matches;
	FGridCombatReactionResolver::ResolveMatches(Bindings, Ally.CharacterId, Event, Ledger, false, Matches);
	TestEqual(TEXT("Unarmed/non-weapon hit does not consume imbuement"), Matches.Num(), 0);

	Event.bWeaponAttack = true;
	FGridCombatReactionResolver::ResolveMatches(Bindings, Ally.CharacterId, Event, Ledger, false, Matches);
	TestEqual(TEXT("Weapon hit resolves the imbuement packet"), Matches.Num(), 1);
	TestTrue(TEXT("Weapon hit requests status consumption"), Matches.Num() == 1 && Matches[0].bConsumeOwningStatus);
	TestEqual(TEXT("Secondary packet keeps Fire damage type"),
		Matches.Num() == 1 ? Matches[0].SecondaryDirectDamageType : EGridDamageType::Physical, EGridDamageType::Fire);
	TestEqual(TEXT("3 + source Mage INT modifier 3 resolves to six damage"),
		FGridCombatReactionResolver::ResolveSecondaryDirectDamageAmount(Bindings[0].Profile, SourceMage.Attributes), 6);
	TestEqual(TEXT("Carrier INT is not the scaling authority"),
		FGridCombatReactionResolver::ResolveSecondaryDirectDamageAmount(Bindings[0].Profile, Ally.Attributes), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394F1SurfaceConversionTest,
	"Grimrock.RPG.RPG03.9.4F1.SurfaceConversion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394F1SurfaceConversionTest::RunTest(const FString&)
{
	const FGuid MageId = FGuid::NewGuid();
	FGridResolvedCombatModifiers Modifiers;
	Modifiers.SurfaceDurationRoundsModifier = 2;

	FGridCombatSurfaceState Water;
	Water.SurfaceType = EGridCombatSurfaceType::Water;
	Water.RemainingRounds = 2;
	Water.SourceCombatantId = FGuid::NewGuid();
	Water.SourceActionId = TEXT("Action_OldSurface");

	FGridCombatSurfaceConversionProfile Freeze;
	Freeze.InputSurfaceTypes = { EGridCombatSurfaceType::Water };
	Freeze.OutputSurfaceType = EGridCombatSurfaceType::Ice;

	FGridCombatSurfaceState Converted;
	TestTrue(TEXT("Water converts to Ice"),
		FGridCombatSurfaceResolver::ResolveConversion(
			Freeze, &Water, MageId, TEXT("Action_Convert"), Modifiers, Converted));
	TestEqual(TEXT("Conversion selects Ice"), Converted.SurfaceType, EGridCombatSurfaceType::Ice);
	TestEqual(TEXT("Existing duration plus source modifier is clamped/applied"), Converted.RemainingRounds, 4);
	TestEqual(TEXT("Conversion snapshots the new source"), Converted.SourceCombatantId, MageId);

	FGridCombatSurfaceState Fire = Water;
	Fire.SurfaceType = EGridCombatSurfaceType::Fire;
	TestFalse(TEXT("Invalid source surface is a no-op"),
		FGridCombatSurfaceResolver::ResolveConversion(
			Freeze, &Fire, MageId, TEXT("Action_Convert"), Modifiers, Converted));

	FGridCombatSurfaceConversionProfile Electrify;
	Electrify.InputSurfaceTypes = { EGridCombatSurfaceType::Water, EGridCombatSurfaceType::Blood };
	Electrify.OutputSurfaceType = EGridCombatSurfaceType::ElectrifiedWater;
	FGridCombatSurfaceState Blood = Water;
	Blood.SurfaceType = EGridCombatSurfaceType::Blood;
	TestTrue(TEXT("Blood is representable and converts to ElectrifiedWater"),
		FGridCombatSurfaceResolver::ResolveConversion(
			Electrify, &Blood, MageId, TEXT("Action_Convert"), FGridResolvedCombatModifiers(), Converted));
	TestEqual(TEXT("Blood conversion output"), Converted.SurfaceType, EGridCombatSurfaceType::ElectrifiedWater);

	FGridCombatSurfaceConversionProfile NeutralToOil;
	NeutralToOil.bAllowEmptyCell = true;
	NeutralToOil.OutputSurfaceType = EGridCombatSurfaceType::Oil;
	NeutralToOil.EmptyCellDurationRounds = 3;
	TestTrue(TEXT("Neutral cell can create Oil"),
		FGridCombatSurfaceResolver::ResolveConversion(
			NeutralToOil, nullptr, MageId, TEXT("Action_Convert"), Modifiers, Converted));
	TestEqual(TEXT("Neutral-to-Oil honors duration modifier and cap"), Converted.RemainingRounds, 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394F1AffinitySurfaceProjectionTest,
	"Grimrock.RPG.RPG03.9.4F1.AffinitySurfaceProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394F1AffinitySurfaceProjectionTest::RunTest(const FString&)
{
	using namespace RPG0394F1;
	const FGridCombatActionDefinition Authored = MakeSurfaceAction();
	TestTrue(TEXT("Owner-variant surface action is valid before projection"), Authored.IsValid());

	FGridCombatActionDefinition Fire = Authored;
	TSet<FName> FireRequirements;
	FireRequirements.Add(TEXT("Affinity_Fire"));
	TestTrue(TEXT("Fire affinity projects surface creation"),
		FGridCombatActionCatalog::ApplyOwnerRequirementVariant(Fire, FireRequirements));
	TestEqual(TEXT("Fire projection adds one surface effect"), Fire.SurfaceEffects.Num(), 1);
	TestEqual(TEXT("Fire projection adds no conversion"), Fire.SurfaceConversions.Num(), 0);
	TestEqual(TEXT("Fire projection creates Fire"), Fire.SurfaceEffects[0].SurfaceType, EGridCombatSurfaceType::Fire);
	TestTrue(TEXT("Runtime action consumes owner variants"), Fire.OwnerVariants.IsEmpty());

	FGridCombatActionDefinition Air = Authored;
	TSet<FName> AirRequirements;
	AirRequirements.Add(TEXT("Affinity_Air"));
	TestTrue(TEXT("Air affinity projects conversion table"),
		FGridCombatActionCatalog::ApplyOwnerRequirementVariant(Air, AirRequirements));
	TestEqual(TEXT("Air projection adds no direct surface creation"), Air.SurfaceEffects.Num(), 0);
	TestEqual(TEXT("Air projection adds one conversion"), Air.SurfaceConversions.Num(), 1);
	TestTrue(TEXT("Air conversion accepts Water and Blood"),
		Air.SurfaceConversions[0].InputSurfaceTypes.Contains(EGridCombatSurfaceType::Water) &&
		Air.SurfaceConversions[0].InputSurfaceTypes.Contains(EGridCombatSurfaceType::Blood));
	TestEqual(TEXT("Air conversion produces ElectrifiedWater"),
		Air.SurfaceConversions[0].OutputSurfaceType, EGridCombatSurfaceType::ElectrifiedWater);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394F1TargetEnvironmentContextTest,
	"Grimrock.RPG.RPG03.9.4F1.TargetEnvironmentContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394F1TargetEnvironmentContextTest::RunTest(const FString&)
{
	UGridStatusEffectDefinitionAsset* Burning = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	Burning->EffectId = TEXT("Status_Burning");
	Burning->DisplayName = FText::FromString(TEXT("Burning"));
	Burning->DurationUnit = EGridStatusEffectDurationUnit::Rounds;
	Burning->DefaultDuration = 2;
	Burning->StatusTags = { TEXT("Elemental.Fire") };

	FGridStatusEffectCollection Statuses;
	FGridStatusEffectApplyResult ApplyResult;
	FString ApplyError;
	TestTrue(TEXT("Environment status applies"),
		Statuses.TryApply(*Burning, FGuid::NewGuid(), ApplyResult, ApplyError));

	FGridCombatActionDefinition Action;
	Action.ActionId = TEXT("Spell_RPG0394F1_Elemental");
	Action.ActionType = EGridCombatActionType::Ability;
	Action.SourcePolicy = EGridCombatActionSourcePolicy::Spell;

	FGridCombatModifierProfile Conduction;
	Conduction.SourcePolicies = { EGridCombatActionSourcePolicy::Spell };
	Conduction.AnyTargetEnvironmentTags = { TEXT("Surface.Oil"), TEXT("Status_Burning"), TEXT("Elemental.Fire") };
	Conduction.OutgoingDamagePercentModifier = 20;
	TestTrue(TEXT("Environment-filtered modifier is valid"), Conduction.IsValid());

	FGridCombatModifierContext Context = FGridCombatModifierResolver::MakeActionContext(Action, Action.ActionId);
	FGridCombatModifierResolver::AddTargetStatusContext(Context, Statuses, FGuid::NewGuid(), NAME_None);
	TestTrue(TEXT("Status EffectId enters target environment"),
		Context.TargetEnvironmentTags.Contains(TEXT("Status_Burning")));
	TestTrue(TEXT("Status semantic tags enter target environment"),
		Context.TargetEnvironmentTags.Contains(TEXT("Elemental.Fire")));
	FGridCombatModifierResolver::AddTargetSurfaceContext(Context, EGridCombatSurfaceType::Oil);
	TestTrue(TEXT("Cell surface enters target environment"),
		Context.TargetEnvironmentTags.Contains(TEXT("Surface.Oil")));

	FGridResolvedCombatModifiers Resolved;
	FGridCombatModifierResolver::Resolve({ Conduction }, Context, Resolved);
	TestEqual(TEXT("Compatible environment grants +20 percent"), Resolved.OutgoingDamagePercentModifier, 20);

	FGridCombatModifierContext NeutralContext = FGridCombatModifierResolver::MakeActionContext(Action, Action.ActionId);
	FGridCombatModifierResolver::Resolve({ Conduction }, NeutralContext, Resolved);
	TestEqual(TEXT("Neutral target environment grants no bonus"), Resolved.OutgoingDamagePercentModifier, 0);
	return true;
}

#endif

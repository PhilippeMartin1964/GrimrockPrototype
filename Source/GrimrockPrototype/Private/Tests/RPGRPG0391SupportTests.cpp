#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionService.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatReactionResolver.h"
#include "Runtime/Combat/GridCombatTargetingResolver.h"
#include "Runtime/GridInventoryTypes.h"

namespace RPG0391Support
{
	URPGClassAsset* MakeVariantClass()
	{
		URPGClassAsset* ClassAsset = NewObject<URPGClassAsset>(GetTransientPackage());
		ClassAsset->ClassId = TEXT("Class_RPG0391");
		ClassAsset->DisplayName = FText::FromString(TEXT("RPG03.9.1"));
		ClassAsset->HealthAtLevelOne = 10;

		FRPGClassProgressionLevelGrant Level2;
		Level2.Level = 2;
		Level2.ChoicePointsGranted = 1;
		ClassAsset->ProgressionLevelGrants.Add(Level2);

		FRPGClassProgressionLevelGrant Level6;
		Level6.Level = 6;
		Level6.ChoicePointsGranted = 1;
		ClassAsset->ProgressionLevelGrants.Add(Level6);

		const FName Alias(TEXT("Talent_Specialization"));
		const FName Group(TEXT("TalentGroup_Specialization"));
		for (const FName ChoiceId : { FName(TEXT("Talent_Specialization_A")), FName(TEXT("Talent_Specialization_B")) })
		{
			FRPGClassProgressionChoiceDefinition Variant;
			Variant.ChoiceId = ChoiceId;
			Variant.DisplayName = FText::FromName(ChoiceId);
			Variant.MinimumLevel = 2;
			Variant.PointCost = 1;
			Variant.ExclusiveChoiceGroupId = Group;
			Variant.GrantedRequirementIds = { Alias };
			ClassAsset->ProgressionChoices.Add(Variant);
		}

		FRPGClassProgressionChoiceDefinition Followup;
		Followup.ChoiceId = TEXT("Talent_Followup");
		Followup.DisplayName = FText::FromString(TEXT("Follow-up"));
		Followup.MinimumLevel = 6;
		Followup.PointCost = 1;
		Followup.PrerequisiteRequirementIds = { Alias };
		ClassAsset->ProgressionChoices.Add(Followup);
		return ClassAsset;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0391VariantProgressionTest, "Grimrock.RPG.RPG03.9.1.Support.VariantProgression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0391VariantProgressionTest::RunTest(const FString&)
{
	URPGClassAsset* ClassAsset = RPG0391Support::MakeVariantClass();
	TestTrue(TEXT("Generic variant class is structurally valid"), ClassAsset->IsValidDefinition());

	TSet<FName> Selected = { TEXT("Talent_Specialization_A") };
	TestTrue(TEXT("Logical requirement unlocks the downstream choice"),
		FRPGClassProgressionService::GetChoiceAvailability(ClassAsset, 6, Selected, TEXT("Talent_Followup")) ==
			ERPGClassProgressionChoiceAvailabilityReason::None);
	TestTrue(TEXT("The sibling variant is mutually exclusive"),
		FRPGClassProgressionService::GetChoiceAvailability(ClassAsset, 6, Selected, TEXT("Talent_Specialization_B")) ==
			ERPGClassProgressionChoiceAvailabilityReason::MutuallyExclusiveChoice);

	Selected.Add(TEXT("Talent_Specialization_B"));
	int32 Granted = 0;
	int32 Spent = 0;
	int32 Remaining = 0;
	TestFalse(TEXT("A durable selection cannot contain two variants from one exclusive group"),
		FRPGClassProgressionService::TryGetChoicePointBalance(ClassAsset, 6, Selected, Granted, Spent, Remaining));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0391ConditionalModifierTest, "Grimrock.RPG.RPG03.9.1.Support.ConditionalModifier",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0391ConditionalModifierTest::RunTest(const FString&)
{
	URPGClassAsset* ClassAsset = NewObject<URPGClassAsset>(GetTransientPackage());
	ClassAsset->ClassId = TEXT("Class_RPG0391_Modifier");
	ClassAsset->DisplayName = FText::FromString(TEXT("Modifier"));
	ClassAsset->HealthAtLevelOne = 10;

	FRPGClassProgressionLevelGrant Grant;
	Grant.Level = 1;
	Grant.ChoicePointsGranted = 2;
	ClassAsset->ProgressionLevelGrants.Add(Grant);

	FRPGClassProgressionChoiceDefinition Slash;
	Slash.ChoiceId = TEXT("Spec_Slashing");
	Slash.DisplayName = FText::FromString(TEXT("Slashing"));
	ClassAsset->ProgressionChoices.Add(Slash);

	FRPGClassProgressionChoiceDefinition Critical;
	Critical.ChoiceId = TEXT("Critical");
	Critical.DisplayName = FText::FromString(TEXT("Critical"));

	FGridCombatModifierProfile SlashProfile;
	SlashProfile.PhysicalSubtypes = { EGridPhysicalDamageSubtype::Slashing };
	SlashProfile.RequiredOwnerRequirementIds = { TEXT("Spec_Slashing") };
	SlashProfile.CriticalChancePercentModifier = 10;
	Critical.CombatModifiers.Add(SlashProfile);

	FGridCombatModifierProfile PierceProfile = SlashProfile;
	PierceProfile.PhysicalSubtypes = { EGridPhysicalDamageSubtype::Piercing };
	PierceProfile.RequiredOwnerRequirementIds = { TEXT("Spec_Piercing") };
	Critical.CombatModifiers.Add(PierceProfile);
	ClassAsset->ProgressionChoices.Add(Critical);

	FGridCharacterInventoryState Character;
	Character.ClassId = ClassAsset->ClassId;
	Character.ClassDefinition = ClassAsset;
	Character.Level = 1;
	Character.SelectedClassProgressionChoiceIds = { TEXT("Spec_Slashing"), TEXT("Critical") };

	TArray<FGridCombatModifierProfile> Profiles;
	TestTrue(TEXT("Choice modifier projection succeeds"),
		FGridCombatModifierResolver::CollectCharacterChoiceModifiers(Character, Profiles));
	TestEqual(TEXT("Only the modifier for the selected specialization is projected"), Profiles.Num(), 1);
	if (Profiles.Num() == 1)
	{
		TestTrue(TEXT("Projected subtype is Slashing"), Profiles[0].PhysicalSubtypes.Contains(EGridPhysicalDamageSubtype::Slashing));
		TestTrue(TEXT("Owner requirements are consumed before ordinary combat matching"), Profiles[0].RequiredOwnerRequirementIds.IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0391TargetVitalsTest, "Grimrock.RPG.RPG03.9.1.Support.TargetVitals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0391TargetVitalsTest::RunTest(const FString&)
{
	FGridCombatTargetFilterProfile Filter;
	Filter.bRequirePhysicalArmorDepleted = true;
	Filter.MaximumHealthPercent = 35;

	FGridAttackTargetStats Target;
	Target.CurrentHealth = 35;
	Target.PhysicalArmor = 0;
	FGridStatusEffectCollection Statuses;

	TestTrue(TEXT("Exactly 35 percent HP with no physical armor is eligible"),
		FGridCombatTargetingResolver::MatchesTargetFilter(Filter, NAME_None, Statuses, FGuid(), &Target, 100));

	Target.CurrentHealth = 36;
	TestFalse(TEXT("Above 35 percent HP is rejected"),
		FGridCombatTargetingResolver::MatchesTargetFilter(Filter, NAME_None, Statuses, FGuid(), &Target, 100));

	Target.CurrentHealth = 20;
	Target.PhysicalArmor = 1;
	TestFalse(TEXT("Remaining physical armor is rejected"),
		FGridCombatTargetingResolver::MatchesTargetFilter(Filter, NAME_None, Statuses, FGuid(), &Target, 100));

	TestFalse(TEXT("Vital filters never silently pass without a runtime vital snapshot"),
		FGridCombatTargetingResolver::MatchesTargetFilter(Filter, NAME_None, Statuses, FGuid()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0391MaximumResourcePercentTest, "Grimrock.RPG.RPG03.9.1.Support.MaximumResourcePercent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0391MaximumResourcePercentTest::RunTest(const FString&)
{
	FGridCombatActionEffectProfile Effect;
	Effect.RestoreHealthMaximumPercent = 20;
	TestTrue(TEXT("A percent-only effect profile is valid"), Effect.IsValid());
	TestEqual(TEXT("20 percent of 31 MaxHP is rounded upward to seven"), Effect.ResolveHealthRestore(31), 7);

	Effect.RestoreHealth = 2;
	TestEqual(TEXT("Flat and maximum-health restoration compose deterministically"), Effect.ResolveHealthRestore(31), 9);
	TestEqual(TEXT("Zero maximum mana produces no percentage restoration"), Effect.ResolveManaRestore(0), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0391ReactionResponseTest, "Grimrock.RPG.RPG03.9.1.Support.ReactionResponse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0391ReactionResponseTest::RunTest(const FString&)
{
	FGridCombatReactionProfile Riposte;
	Riposte.ReactionId = TEXT("Reaction_Riposte");
	Riposte.Trigger = EGridCombatReactionTrigger::AttackMiss;
	Riposte.Limit = EGridCombatReactionLimit::OncePerRound;
	Riposte.ActionTypes = { EGridCombatActionType::MeleeAttack };
	Riposte.CounterAttackActionId = TEXT("Action_Riposte");
	Riposte.CounterAttackWeaponProfile.bUseEquippedWeapon = true;
	Riposte.CounterAttackWeaponProfile.WeaponDamagePercent = 75;
	Riposte.CounterAttackWeaponProfile.bAllowUnarmed = true;
	TestTrue(TEXT("Weapon counterattack reaction is structurally valid"), Riposte.IsValid());

	FGridCombatReactionBinding Binding;
	Binding.Profile = Riposte;

	FGridCombatReactionEvent Event;
	Event.EventId = FGuid::NewGuid();
	Event.ActionInstanceId = FGuid::NewGuid();
	Event.RoundNumber = 2;
	Event.Trigger = EGridCombatReactionTrigger::AttackMiss;
	Event.SourceCombatantId = FGuid::NewGuid();
	Event.TargetCombatantId = FGuid::NewGuid();
	Event.ActionId = TEXT("Attack_Monster");
	Event.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
	Event.ActionType = EGridCombatActionType::MeleeAttack;
	Event.DamageType = EGridDamageType::Physical;

	FGridCombatReactionLedger Ledger;
	TArray<FGridCombatReactionMatch> Matches;
	FGridCombatReactionResolver::ResolveMatches({ Binding }, Event.TargetCombatantId, Event, Ledger, true, Matches);
	TestEqual(TEXT("One Riposte match resolves"), Matches.Num(), 1);
	if (Matches.Num() == 1)
	{
		TestEqual(TEXT("Counterattack identity survives resolution"), Matches[0].CounterAttackActionId, FName(TEXT("Action_Riposte")));
		TestEqual(TEXT("Counterattack WD coefficient survives resolution"), Matches[0].CounterAttackWeaponProfile.WeaponDamagePercent, 75);
	}

	FGridCombatReactionProfile Interception;
	Interception.ReactionId = TEXT("Reaction_Interception");
	Interception.Trigger = EGridCombatReactionTrigger::IncomingAttackHit;
	Interception.Limit = EGridCombatReactionLimit::OncePerRound;
	Interception.DamageTypes = { EGridDamageType::Physical };
	Interception.InterceptFinalDamagePercent = 50;
	Interception.bRequireOwnerFrontRow = true;
	Interception.bRequireEventTargetFrontRow = true;
	TestTrue(TEXT("Incoming physical damage interception is structurally valid"), Interception.IsValid());

	Interception.Trigger = EGridCombatReactionTrigger::AttackHit;
	TestFalse(TEXT("Interception cannot be authored on a post-application trigger"), Interception.IsValid());
	return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatArmorEffectResolver.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Combat/GridCombatTargetingResolver.h"
#include "Runtime/Combat/GridCombatTypes.h"

namespace RPG038
{
	UGridStatusEffectDefinitionAsset* MakeStatus(
		FName EffectId, EGridStatusEffectDisposition Disposition, const TArray<FName>& Tags = {}, bool bBlockDirectTargeting = false)
	{
		UGridStatusEffectDefinitionAsset* Definition = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
		Definition->EffectId = EffectId;
		Definition->DisplayName = FText::FromName(EffectId);
		Definition->Disposition = Disposition;
		Definition->DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		Definition->DefaultDuration = 2;
		Definition->StatusTags = Tags;
		Definition->Control.bBlockDirectHostileTargeting = bBlockDirectTargeting;
		return Definition;
	}

	FGridStatusEffectRuntimeState MakeState(UGridStatusEffectDefinitionAsset* Definition, const FGuid& SourceId)
	{
		FGridStatusEffectRuntimeState State;
		FString Error;
		if (IsValid(Definition))
		{
			Definition->BuildRuntimeState(SourceId, 1, INDEX_NONE, State, Error);
		}
		return State;
	}

	FGridCharacterInventoryState MakeCharacter(int32 Health)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.Resources.CurrentHealth = Health;
		return Character;
	}

	FGridCombatActionDefinition MakeBaseEffect(EGridCombatTargetingPolicy Policy)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = TEXT("Action_RPG038_Effect");
		Action.DisplayName = FText::FromString(TEXT("RPG03.8 Effect"));
		Action.ActionType = EGridCombatActionType::Ability;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
		Action.TargetingPolicy = Policy;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Action.ActionPointCost = 1;
		Action.RangeCells = Policy == EGridCombatTargetingPolicy::Hostile ? 4 : 0;
		return Action;
	}

	FGridCombatActionDefinition MakeAreaAttack()
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = TEXT("Action_RPG038_Area");
		Action.DisplayName = FText::FromString(TEXT("RPG03.8 Area"));
		Action.ActionType = EGridCombatActionType::RangedAttack;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
		Action.TargetingPolicy = EGridCombatTargetingPolicy::Area;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
		Action.ActionPointCost = 2;
		Action.RangeCells = 4;
		Action.AreaRadiusCells = 1;
		Action.OffensiveProfile.AttackId = TEXT("Attack_RPG038_Area");
		Action.OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Physical;
		Action.OffensiveProfile.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
		Action.OffensiveProfile.AttackDefinition.MinDamage = 2;
		Action.OffensiveProfile.AttackDefinition.MaxDamage = 4;
		Action.OffensiveProfile.DamageScalingAttribute = EGridAttackScalingAttribute::None;
		Action.OffensiveProfile.RangeCells = 4;
		return Action;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG038TargetabilityTest, "Grimrock.RPG.RPG03.8.Targetability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG038TargetabilityTest::RunTest(const FString&)
{
	FGridStatusEffectCollection Effects;
	TestTrue(TEXT("No status permits direct hostile targeting"), FGridCombatTargetingResolver::IsDirectHostileTargetable(Effects));

	UGridStatusEffectDefinitionAsset* Hidden =
		RPG038::MakeStatus(TEXT("Status_RPG038_Hidden"), EGridStatusEffectDisposition::Buff, {}, true);
	Effects.ActiveEffects.Add(RPG038::MakeState(Hidden, FGuid::NewGuid()));
	TestFalse(TEXT("Data-driven hidden control blocks direct hostile targeting"),
		FGridCombatTargetingResolver::IsDirectHostileTargetable(Effects));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG038PartyBatchSelectionTest, "Grimrock.RPG.RPG03.8.PartyBatchSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG038PartyBatchSelectionTest::RunTest(const FString&)
{
	FGridPartyInventoryState Party;
	Party.ActiveCharacters = {
		RPG038::MakeCharacter(10), RPG038::MakeCharacter(0), RPG038::MakeCharacter(8),
		RPG038::MakeCharacter(7), RPG038::MakeCharacter(0), RPG038::MakeCharacter(6)
	};

	TArray<int32> Targets;
	FGridCombatTargetingResolver::CollectPartyTargets(
		Party, EGridCombatTargetingPolicy::FrontRowParty, 3, INDEX_NONE, 3, Targets);
	TestTrue(TEXT("Front row contains only living slots 0 and 2"), Targets == TArray<int32>({ 0, 2 }));

	FGridCombatTargetingResolver::CollectPartyTargets(
		Party, EGridCombatTargetingPolicy::Party, 3, INDEX_NONE, 3, Targets);
	TestTrue(TEXT("Party batch is deterministic by slot"), Targets == TArray<int32>({ 0, 2, 3, 5 }));

	FGridCombatTargetingResolver::CollectPartyTargets(
		Party, EGridCombatTargetingPolicy::Ally, 0, TArray<int32>({ 5, 2, 5 }), 3, Targets);
	TestTrue(TEXT("Explicit ally selection preserves order and removes duplicates"), Targets == TArray<int32>({ 5, 2 }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG038StatusRemovalFilterTest, "Grimrock.RPG.RPG03.8.StatusRemovalFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG038StatusRemovalFilterTest::RunTest(const FString&)
{
	UGridStatusEffectDefinitionAsset* Poison = RPG038::MakeStatus(
		TEXT("Status_Poison"), EGridStatusEffectDisposition::Debuff, { TEXT("Toxin"), TEXT("Purifiable") });
	UGridStatusEffectDefinitionAsset* Venom = RPG038::MakeStatus(
		TEXT("Status_Toxin_Venom"), EGridStatusEffectDisposition::Debuff, { TEXT("Toxin") });
	UGridStatusEffectDefinitionAsset* Burning = RPG038::MakeStatus(
		TEXT("Status_Burning"), EGridStatusEffectDisposition::Debuff, { TEXT("Purifiable") });
	UGridStatusEffectDefinitionAsset* Blessed =
		RPG038::MakeStatus(TEXT("Status_Blessed"), EGridStatusEffectDisposition::Buff);

	const FGuid Source = FGuid::NewGuid();
	FGridStatusEffectCollection Effects;
	Effects.ActiveEffects = {
		RPG038::MakeState(Venom, Source), RPG038::MakeState(Blessed, Source),
		RPG038::MakeState(Poison, Source), RPG038::MakeState(Burning, Source)
	};

	FGridCombatStatusRemovalProfile PoisonExact;
	PoisonExact.EffectIds = { TEXT("Status_Poison") };
	PoisonExact.AllowedDispositions = { EGridStatusEffectDisposition::Debuff };
	PoisonExact.MaximumRemovals = 1;

	FGridCombatStatusRemovalProfile OneOtherToxin;
	OneOtherToxin.AnyStatusTags = { TEXT("Toxin") };
	OneOtherToxin.AllowedDispositions = { EGridStatusEffectDisposition::Debuff };
	OneOtherToxin.MaximumRemovals = 1;

	TArray<FName> Removed;
	FGridCombatTargetingResolver::CollectStatusRemovalIds(Effects, { PoisonExact, OneOtherToxin }, Removed);
	TestTrue(TEXT("Antidote removes Poison then one other toxin"),
		Removed == TArray<FName>({ TEXT("Status_Poison"), TEXT("Status_Toxin_Venom") }));

	FGridCombatStatusRemovalProfile Purify;
	Purify.AnyStatusTags = { TEXT("Purifiable") };
	Purify.AllowedDispositions = { EGridStatusEffectDisposition::Debuff };
	Purify.MaximumRemovals = 3;
	FGridCombatTargetingResolver::CollectStatusRemovalIds(Effects, { Purify }, Removed);
	TestTrue(TEXT("Purifiable removal is deterministic and excludes buffs"),
		Removed == TArray<FName>({ TEXT("Status_Burning"), TEXT("Status_Poison") }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG038TargetFilterTest, "Grimrock.RPG.RPG03.8.TargetFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG038TargetFilterTest::RunTest(const FString&)
{
	const FGuid RangerId = FGuid::NewGuid();
	UGridStatusEffectDefinitionAsset* Mark =
		RPG038::MakeStatus(TEXT("Status_MarkedByRanger"), EGridStatusEffectDisposition::Debuff, { TEXT("Marked") });
	FGridStatusEffectCollection Effects;
	Effects.ActiveEffects.Add(RPG038::MakeState(Mark, RangerId));

	FGridCombatTargetFilterProfile Filter;
	Filter.AllowedMonsterCategoryIds = { TEXT("Undead"), TEXT("Demon") };
	Filter.RequiredStatusEffectIds = { TEXT("Status_MarkedByRanger") };
	Filter.RequiredStatusTags = { TEXT("Marked") };
	Filter.bRequiredStatusesFromSource = true;

	TestTrue(TEXT("Matching category, mark and source are eligible"),
		FGridCombatTargetingResolver::MatchesTargetFilter(Filter, TEXT("Undead"), Effects, RangerId));
	TestFalse(TEXT("Another source cannot use the owner-specific mark"),
		FGridCombatTargetingResolver::MatchesTargetFilter(Filter, TEXT("Undead"), Effects, FGuid::NewGuid()));
	TestFalse(TEXT("Wrong monster category is rejected"),
		FGridCombatTargetingResolver::MatchesTargetFilter(Filter, TEXT("Beast"), Effects, RangerId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG038PrimaryTargetScopeTest, "Grimrock.RPG.RPG03.8.PrimaryTargetScope",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG038PrimaryTargetScopeTest::RunTest(const FString&)
{
	FGridCombatStatusApplicationProfile Profile;
	Profile.StatusEffectId = TEXT("Status_KnockedDown");
	Profile.TargetScope = EGridCombatResolvedTargetScope::PrimaryTargetOnly;

	TestTrue(TEXT("Primary target receives primary-only status"),
		FGridCombatTargetingResolver::ShouldApplyStatusApplication(Profile, 0));
	TestFalse(TEXT("Secondary target does not receive primary-only status"),
		FGridCombatTargetingResolver::ShouldApplyStatusApplication(Profile, 1));

	Profile.TargetScope = EGridCombatResolvedTargetScope::AllResolvedTargets;
	TestTrue(TEXT("All-target status reaches secondary targets"),
		FGridCombatTargetingResolver::ShouldApplyStatusApplication(Profile, 4));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG038ActionValidationTest, "Grimrock.RPG.RPG03.8.ActionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG038ActionValidationTest::RunTest(const FString&)
{
	FGridCombatActionDefinition Hostile = RPG038::MakeBaseEffect(EGridCombatTargetingPolicy::Hostile);
	FGridCombatArmorEffectProfile Acid;
	Acid.Pool = EGridCombatArmorPool::Physical;
	Acid.Operation = EGridCombatArmorEffectOperation::Damage;
	Acid.Magnitude = EGridCombatArmorEffectMagnitude::Flat;
	Acid.Trigger = EGridCombatArmorEffectTrigger::AfterResolution;
	Acid.Amount = 6;
	Hostile.ArmorEffects.Add(Acid);
	TestTrue(TEXT("Hostile direct armor Effect is valid"), Hostile.IsValid());

	FGridCombatActionDefinition Party = RPG038::MakeBaseEffect(EGridCombatTargetingPolicy::Party);
	FGridCombatStatusApplicationProfile Warlord;
	Warlord.StatusEffectId = TEXT("Status_Warlord");
	Party.StatusApplications.Add(Warlord);
	TestTrue(TEXT("Party batch status Effect is valid"), Party.IsValid());

	FGridCombatActionDefinition Rapid = RPG038::MakeAreaAttack();
	Rapid.ResolutionCount = 2;
	Rapid.SubsequentResolutionAccuracyModifier = -1;
	Rapid.bAffectsAlliesInArea = true;
	FGridCombatSurfaceEffectProfile Surface;
	Surface.SurfaceType = EGridCombatSurfaceType::Fire;
	Surface.DurationRounds = 2;
	Rapid.SurfaceEffects.Add(Surface);
	TestTrue(TEXT("Repeated Area attack can carry friendly-fire and surface payloads"), Rapid.IsValid());

	Rapid.ResolutionCount = 1;
	TestFalse(TEXT("Subsequent accuracy modifier requires repeated resolution"), Rapid.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG038DirectArmorDamageTest, "Grimrock.RPG.RPG03.8.DirectArmorDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG038DirectArmorDamageTest::RunTest(const FString&)
{
	FGridCombatArmorEffectProfile Acid;
	Acid.Pool = EGridCombatArmorPool::Physical;
	Acid.Operation = EGridCombatArmorEffectOperation::Damage;
	Acid.Magnitude = EGridCombatArmorEffectMagnitude::Flat;
	Acid.Trigger = EGridCombatArmorEffectTrigger::AfterResolution;
	Acid.Amount = 6;
	Acid.ScalingSkillId = TEXT("Skill_Alchemy");
	Acid.SkillRankScale = 2;

	FGridCombatArmorPoolSnapshot Snapshot;
	Snapshot.CurrentPhysicalArmor = 20;
	Snapshot.ReferencePhysicalArmor = 20;
	Snapshot.CurrentMagicalArmor = 9;
	Snapshot.ReferenceMagicalArmor = 9;

	FGridCombatArmorEffectSourceContext Source;
	FRPGSkillRank Rank;
	Rank.SkillId = TEXT("Skill_Alchemy");
	Rank.Rank = 3;
	Source.SkillRanks.Add(Rank);

	FGridResolvedCombatModifiers Modifiers;
	TestTrue(TEXT("Acid direct damage has an applicable armor mutation"),
		FGridCombatArmorEffectResolver::WouldAnyDirectDamage({ Acid }, Snapshot, Modifiers, &Source));
	TestEqual(TEXT("One armor pool mutation is applied"),
		FGridCombatArmorEffectResolver::ApplyDirectDamageEffects({ Acid }, Snapshot, Modifiers, &Source), 1);
	TestEqual(TEXT("6 + 2xAlchemy(3) removes twelve physical armor"), Snapshot.CurrentPhysicalArmor, 8);
	TestEqual(TEXT("Magical armor is untouched"), Snapshot.CurrentMagicalArmor, 9);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG038MonsterPartyTargetabilityTest, "Grimrock.RPG.RPG03.8.MonsterPartyTargetability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG038MonsterPartyTargetabilityTest::RunTest(const FString&)
{
	FGridPartyInventoryState Party;
	Party.ActiveCharacters = {
		RPG038::MakeCharacter(10), RPG038::MakeCharacter(10), RPG038::MakeCharacter(0), RPG038::MakeCharacter(10)
	};

	UGridStatusEffectDefinitionAsset* Hidden =
		RPG038::MakeStatus(TEXT("Status_RPG038_HiddenFront"), EGridStatusEffectDisposition::Buff, {}, true);
	Party.ActiveCharacters[0].StatusEffects.ActiveEffects.Add(RPG038::MakeState(Hidden, FGuid::NewGuid()));

	FRandomStream Random(42);
	TestEqual(TEXT("Monster skips untargetable front slot and selects the remaining living front target"),
		FGridPartyTargetSelector::SelectTarget(Party, Random, 3), 1);

	Party.ActiveCharacters[1].StatusEffects.ActiveEffects.Add(RPG038::MakeState(Hidden, FGuid::NewGuid()));
	Random.Initialize(42);
	TestEqual(TEXT("When every living front target is hidden the visible back row becomes eligible"),
		FGridPartyTargetSelector::SelectTarget(Party, Random, 3), 3);
	return true;
}

#endif

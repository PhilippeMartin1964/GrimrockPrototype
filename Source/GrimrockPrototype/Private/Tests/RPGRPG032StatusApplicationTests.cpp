#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "RPG/StatusEffects/GridCombatStatusApplicationResolver.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterTypes.h"

namespace RPG032
{
	UGridStatusEffectDefinitionAsset* MakeStatus(UObject* Outer, FName EffectId,
		EGridStatusEffectStackPolicy StackPolicy = EGridStatusEffectStackPolicy::NoStack, int32 MaxStacks = 1)
	{
		UGridStatusEffectDefinitionAsset* Definition = NewObject<UGridStatusEffectDefinitionAsset>(Outer);
		Definition->EffectId = EffectId;
		Definition->DisplayName = FText::FromName(EffectId);
		Definition->Disposition = EGridStatusEffectDisposition::Debuff;
		Definition->DurationUnit = EGridStatusEffectDurationUnit::Rounds;
		Definition->DefaultDuration = 2;
		Definition->StackPolicy = StackPolicy;
		Definition->MaxStacks = MaxStacks;
		return Definition;
	}

	FGridCombatStatusApplicationProfile MakeProfile(
		FName EffectId, EGridCombatStatusArmorGate Gate, EGridCombatStatusApplicationTrigger Trigger = EGridCombatStatusApplicationTrigger::AfterSuccessfulHit)
	{
		FGridCombatStatusApplicationProfile Profile;
		Profile.StatusEffectId = EffectId;
		Profile.Trigger = Trigger;
		Profile.ArmorGate = Gate;
		return Profile;
	}

	FGridAttackResult MakeHit(int32 PhysicalArmorDamage, int32 MagicalArmorDamage, int32 HealthAfter = 20)
	{
		FGridAttackResult Result;
		Result.bHit = true;
		Result.PhysicalArmorDamage = PhysicalArmorDamage;
		Result.MagicalArmorDamage = MagicalArmorDamage;
		Result.TargetHealthBefore = 20;
		Result.TargetHealthAfter = HealthAfter;
		return Result;
	}

	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
															.AllowAudioPlayback(false)
															.RequiresHitProxies(false)
															.CreatePhysicsScene(false)
															.CreateNavigation(false)
															.CreateAISystem(false)
															.ShouldSimulatePhysics(false)
															.SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("RPG032World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FTestWorld()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
			}
		}
	};

	struct FLifecycleFixture
	{
		FTestWorld TestWorld;
		AGrimrockPartyPawn* Party = nullptr;
		AGridMonsterActor* Monster = nullptr;
		AActor* Owner = nullptr;
		UGridTurnManagerComponent* TurnManager = nullptr;
		UGridStatusEffectLifecycleSubsystem* Lifecycle = nullptr;
		FGuid CharacterId = FGuid(3, 2, 1, 1);
		FGuid MonsterId = FGuid(3, 2, 2, 1);

		FLifecycleFixture()
		{
			if (!TestWorld.World)
			{
				return;
			}
			Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
			Monster = TestWorld.World->SpawnActor<AGridMonsterActor>();
			Owner = TestWorld.World->SpawnActor<AActor>();
			if (!Party || !Party->PartyInventoryComponent || !Monster || !Owner)
			{
				return;
			}

			FGridCharacterInventoryState Character;
			Character.CharacterId = CharacterId;
			Character.DisplayName = FText::FromString(TEXT("RPG03.2 Hero"));
			Character.Resources.CurrentHealth = 20;
			Character.Resources.CurrentPhysicalArmor = 0;
			Character.Resources.CurrentMagicalArmor = 0;
			Character.DerivedStats.MaxHealth = 20;
			Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { Character };
			Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment.SetNum(1);

			Monster->SpawnObjectId = MonsterId;
			Monster->PersistentMonsterId = MonsterId;

			TurnManager = NewObject<UGridTurnManagerComponent>(Owner, TEXT("RPG032TurnManager"));
			Lifecycle = NewObject<UGridStatusEffectLifecycleSubsystem>(TestWorld.World, TEXT("RPG032Lifecycle"));
			if (!TurnManager || !Lifecycle)
			{
				return;
			}
			TurnManager->PartyPawn = Party;
			TurnManager->CombatMonsters.Add(Monster);
			TurnManager->bCombatActive = true;
			TurnManager->CurrentPhase = EGridCombatPhase::PlayerPhase;
			TurnManager->RoundNumber = 1;
			Lifecycle->BindToTurnManager(TurnManager);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG032ProfileValidationTest, "Grimrock.RPG.RPG03.2.ProfileValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG032ProfileValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatStatusApplicationProfile Profile;
	TestFalse(TEXT("Missing status id is invalid"), Profile.IsValid());
	Profile.StatusEffectId = TEXT("Status_Test");
	TestTrue(TEXT("Minimal status application profile is valid"), Profile.IsValid());

	FGridCombatActionDefinition EffectAction;
	EffectAction.ActionId = TEXT("Action_StatusOnly");
	EffectAction.ActionType = EGridCombatActionType::Ability;
	EffectAction.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
	EffectAction.TargetingPolicy = EGridCombatTargetingPolicy::Self;
	EffectAction.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
	EffectAction.ActionPointCost = 1;
	EffectAction.StatusApplications = { Profile };
	TestTrue(TEXT("Status-only self Effect is a valid action"), EffectAction.IsValid());

	EffectAction.StatusApplications[0].Trigger = EGridCombatStatusApplicationTrigger::AfterSuccessfulHit;
	TestFalse(TEXT("Hit-triggered status requires an Attack action"), EffectAction.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG032PhysicalArmorGateTest, "Grimrock.RPG.RPG03.2.PhysicalArmorGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG032PhysicalArmorGateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGridCombatStatusApplicationProfile Profile =
		RPG032::MakeProfile(TEXT("Status_Stunned"), EGridCombatStatusArmorGate::PhysicalArmorDepleted);

	FGridAttackTargetStats Target;
	Target.CurrentHealth = 20;
	Target.PhysicalArmor = 5;

	const FGridAttackResult NotBroken = RPG032::MakeHit(4, 0);
	TestFalse(TEXT("Physical gate blocks while one armor remains"), FGridCombatStatusApplicationResolver::IsEligible(Profile, Target, &NotBroken));

	const FGridAttackResult Broken = RPG032::MakeHit(5, 0);
	TestTrue(TEXT("Physical gate opens when same hit depletes armor"), FGridCombatStatusApplicationResolver::IsEligible(Profile, Target, &Broken));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG032MagicalTriggerAndDeathTest, "Grimrock.RPG.RPG03.2.MagicalTriggerAndDeath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG032MagicalTriggerAndDeathTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGridCombatStatusApplicationProfile Profile =
		RPG032::MakeProfile(TEXT("Status_Silenced"), EGridCombatStatusArmorGate::MagicalArmorDepleted);

	FGridAttackTargetStats Target;
	Target.CurrentHealth = 20;
	Target.MagicalArmor = 3;

	FGridAttackResult Miss = RPG032::MakeHit(0, 3);
	Miss.bHit = false;
	TestFalse(TEXT("AfterSuccessfulHit rejects a miss even with depleted armor"), FGridCombatStatusApplicationResolver::IsEligible(Profile, Target, &Miss));

	const FGridAttackResult Broken = RPG032::MakeHit(0, 3);
	TestTrue(TEXT("Magical gate opens after a successful armor-breaking hit"), FGridCombatStatusApplicationResolver::IsEligible(Profile, Target, &Broken));

	const FGridAttackResult Lethal = RPG032::MakeHit(0, 3, 0);
	TestFalse(TEXT("C1 never applies a new status to a defeated target"), FGridCombatStatusApplicationResolver::IsEligible(Profile, Target, &Lethal));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG032StatusModifierStackProjectionTest, "Grimrock.RPG.RPG03.2.StatusModifierStackProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG032StatusModifierStackProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridStatusEffectDefinitionAsset* Definition =
		RPG032::MakeStatus(GetTransientPackage(), TEXT("RPG032_StackedModifier"), EGridStatusEffectStackPolicy::AddStacks, 2);
	FGridCombatModifierProfile Modifier;
	Modifier.EvasionModifier = 2;
	Modifier.IncomingDamagePercentModifier = -10;
	Definition->CombatModifiers.Add(Modifier);
	TestTrue(TEXT("Status definition accepts generic C2 profile"), Definition->IsValidDefinition());

	FGridStatusEffectCollection Collection;
	FGridStatusEffectApplyResult ApplyResult;
	FString Error;
	TestTrue(TEXT("Two status stacks apply"), Collection.TryApply(*Definition, FGuid::NewGuid(), 2, INDEX_NONE, INDEX_NONE, ApplyResult, Error));

	TArray<FGridCombatModifierProfile> Profiles;
	TestTrue(TEXT("Status C2 profiles project"), FGridCombatModifierResolver::CollectStatusModifiers(Collection, Profiles));
	TestEqual(TEXT("AddStacks duplicates modifier contribution"), Profiles.Num(), 2);

	FGridResolvedCombatModifiers Resolved;
	FGridCombatModifierResolver::Resolve(Profiles,
		FGridCombatModifierResolver::MakeAttackContext(TEXT("Attack"), NAME_None, EGridCombatActionSourcePolicy::Universal,
			EGridCombatActionType::MeleeAttack, EGridDamageType::Physical, EGridPhysicalDamageSubtype::Slashing),
		Resolved);
	TestEqual(TEXT("Evasion scales by stack"), Resolved.EvasionModifier, 4);
	TestEqual(TEXT("Incoming damage modifier scales by stack"), Resolved.IncomingDamagePercentModifier, -20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG032StatusOnlyCatalogTest, "Grimrock.RPG.RPG03.2.StatusOnlyCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG032StatusOnlyCatalogTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridStatusEffectDefinitionAsset* Definition = RPG032::MakeStatus(GetTransientPackage(), TEXT("RPG032_CatalogStatus"));

	FGridCombatActionDefinition Action;
	Action.ActionId = TEXT("RPG032_StatusAction");
	Action.DisplayName = FText::FromString(TEXT("Status Action"));
	Action.ActionType = EGridCombatActionType::Ability;
	Action.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
	Action.TargetingPolicy = EGridCombatTargetingPolicy::Self;
	Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
	Action.ActionPointCost = 1;
	Action.StatusApplications = {
		RPG032::MakeProfile(Definition->EffectId, EGridCombatStatusArmorGate::None, EGridCombatStatusApplicationTrigger::AfterResolution)
	};

	FGridCombatActionContribution Contribution;
	Contribution.Definition = Action;
	Contribution.SourceDefinitionId = TEXT("RPG032_Class");
	Contribution.AvailableSourceQuantity = 1;

	FGridCombatActionCatalogContext Context;
	Context.CharacterIndex = 0;
	Context.CharacterId = FGuid::NewGuid();
	Context.bCombatActive = true;
	Context.bActiveCombatant = true;
	Context.bEnableClassActionExecutors = true;
	Context.RemainingActionPoints = 4;
	Context.CurrentHealth = 10;
	Context.MaximumHealth = 10;
	Context.CurrentMana = 0;
	Context.MaximumMana = 0;

	TArray<FGridAvailableCombatAction> Actions;
	FGridCombatActionCatalog::Build(Context, { Contribution }, Actions);
	TestEqual(TEXT("Fresh status-only action is projected"), Actions.Num(), 1);
	TestTrue(TEXT("Fresh status-only action is enabled"), Actions.Num() == 1 && Actions[0].bEnabled);

	FGridStatusEffectApplyResult ApplyResult;
	FString Error;
	TestTrue(TEXT("NoStack status pre-exists"), Context.CurrentStatusEffects.TryApply(*Definition, Context.CharacterId, ApplyResult, Error));
	FGridCombatActionCatalog::Build(Context, { Contribution }, Actions);
	TestEqual(TEXT("Existing NoStack status keeps action visible"), Actions.Num(), 1);
	TestEqual(TEXT("Existing NoStack status reports no applicable effect"),
		Actions.Num() == 1 ? Actions[0].AvailabilityReason : EGridCombatActionAvailabilityReason::None,
		EGridCombatActionAvailabilityReason::NoApplicableEffect);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG032LifecycleBridgeTest, "Grimrock.RPG.RPG03.2.LifecycleBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG032LifecycleBridgeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	RPG032::FLifecycleFixture Fixture;
	if (!Fixture.Lifecycle || !Fixture.Party || !Fixture.Party->PartyInventoryComponent || !Fixture.Monster)
	{
		return false;
	}

	UGridStatusEffectDefinitionAsset* Definition = RPG032::MakeStatus(GetTransientPackage(), TEXT("RPG032_LifecycleStatus"));
	FGridCombatStatusApplicationProfile Profile =
		RPG032::MakeProfile(Definition->EffectId, EGridCombatStatusArmorGate::PhysicalArmorDepleted);

	FGridAttackTargetStats TargetBefore;
	TargetBefore.CurrentHealth = 20;
	TargetBefore.PhysicalArmor = 4;
	const FGridAttackResult Result = RPG032::MakeHit(4, 0);

	TestEqual(TEXT("Party bridge mutates one status"),
		Fixture.Lifecycle->ApplyCombatStatusApplicationsToPartyCharacter(0, { Profile }, Fixture.MonsterId, TargetBefore, &Result), 1);
	TestTrue(TEXT("Party character owns applied status"),
		Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].StatusEffects.Contains(Definition->EffectId));

	TestEqual(TEXT("Monster bridge mutates one status"),
		Fixture.Lifecycle->ApplyCombatStatusApplicationsToMonster(Fixture.Monster, { Profile }, Fixture.CharacterId, TargetBefore, &Result), 1);
	TestTrue(TEXT("Monster owns applied status"), Fixture.Monster->StatusEffects.Contains(Definition->EffectId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG032MonsterAttackValidationTest, "Grimrock.RPG.RPG03.2.MonsterAttackValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG032MonsterAttackValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMonsterAttackDefinition Attack;
	Attack.AttackId = TEXT("RPG032_Bite");
	Attack.MinDamage = 1;
	Attack.MaxDamage = 2;
	Attack.ExpectedDuration = 0.5f;
	Attack.ImpactTimeSeconds = 0.2f;
	Attack.StatusApplications = {
		RPG032::MakeProfile(TEXT("RPG032_Poison"), EGridCombatStatusArmorGate::MagicalArmorDepleted)
	};
	FString Error;
	TestTrue(TEXT("Monster hit-triggered status application is valid"), Attack.ValidateDefinition(Error));

	Attack.StatusApplications[0].Trigger = EGridCombatStatusApplicationTrigger::AfterResolution;
	TestFalse(TEXT("Monster attack rejects status-on-miss semantics"), Attack.ValidateDefinition(Error));
	TestTrue(TEXT("Validation explains trigger requirement"), Error.Contains(TEXT("AfterSuccessfulHit")));
	return true;
}

#endif

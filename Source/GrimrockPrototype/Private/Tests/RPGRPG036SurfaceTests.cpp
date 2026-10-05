#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "RPG/StatusEffects/GridCombatStatusApplicationResolver.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Combat/GridCombatSurfaceResolver.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "UObject/UnrealType.h"

namespace RPG036
{
	FGridCombatSurfaceEffectProfile MakeSurface(EGridCombatSurfaceType Type, int32 Duration, EGridDamageType DamageType = EGridDamageType::Physical,
		int32 Damage = 0)
	{
		FGridCombatSurfaceEffectProfile Profile;
		Profile.SurfaceType = Type;
		Profile.DurationRounds = Duration;
		Profile.PeriodicDamageType = DamageType;
		Profile.PeriodicDamagePerRound = Damage;
		return Profile;
	}

	FGridCombatSurfaceState MakeState(EGridCombatSurfaceType Type, int32 Duration = 3)
	{
		FGridCombatSurfaceState State;
		State.SurfaceType = Type;
		State.RemainingRounds = Duration;
		State.SourceCombatantId = FGuid(3, 6, 1, 1);
		State.SourceActionId = TEXT("Action_RPG036");
		return State;
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
				FName(*FString::Printf(TEXT("RPG036World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num, &Values);
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

	AGridLevelRuntimeActor* MakeRuntime(FTestWorld& TestWorld)
	{
		if (!TestWorld.World)
		{
			return nullptr;
		}
		AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
		if (!Runtime)
		{
			return nullptr;
		}
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
		Level->Width = 3;
		Level->Height = 3;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		Runtime->LevelAsset = Level;
		Runtime->CurrentDungeonLevelId = TEXT("RPG036_Level");
		return Runtime;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG036ProfileValidationTest, "Grimrock.RPG.RPG03.6.ProfileValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG036ProfileValidationTest::RunTest(const FString&)
{
	FGridCombatSurfaceEffectProfile Cloud =
		RPG036::MakeSurface(EGridCombatSurfaceType::PoisonCloud, 3, EGridDamageType::Poison, 2);
	FGridCombatStatusApplicationProfile Poison;
	Poison.StatusEffectId = TEXT("Status_Poison");
	Poison.Trigger = EGridCombatStatusApplicationTrigger::AfterResolution;
	Poison.ArmorGate = EGridCombatStatusArmorGate::MagicalArmorDepleted;
	Cloud.PeriodicStatusApplications.Add(Poison);
	TestTrue(TEXT("Corrosive-cloud style surface profile is valid"), Cloud.IsValid());

	Cloud.DurationRounds = 7;
	TestFalse(TEXT("Surface duration is capped by authoring contract"), Cloud.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG036StateModifierProjectionTest, "Grimrock.RPG.RPG03.6.StateModifierProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG036StateModifierProjectionTest::RunTest(const FString&)
{
	FGridCombatSurfaceEffectProfile Profile =
		RPG036::MakeSurface(EGridCombatSurfaceType::Fire, 3, EGridDamageType::Fire, 4);
	FGridResolvedCombatModifiers Modifiers;
	Modifiers.SurfaceDurationRoundsModifier = 2;
	Modifiers.SurfacePeriodicDamagePercentModifier = 25;

	FGridCombatSurfaceState State;
	TestTrue(TEXT("Surface state builds"), FGridCombatSurfaceResolver::BuildState(Profile, FGuid::NewGuid(), TEXT("Action_Surface"), Modifiers, State));
	TestEqual(TEXT("Persistent Surface adds two rounds"), State.RemainingRounds, 5);
	TestEqual(TEXT("Periodic surface damage gets plus twenty-five percent"), State.PeriodicDamagePerRound, 5);

	Profile.DurationRounds = 5;
	TestTrue(TEXT("Long surface state builds"), FGridCombatSurfaceResolver::BuildState(Profile, FGuid::NewGuid(), TEXT("Action_Surface"), Modifiers, State));
	TestEqual(TEXT("Modified duration clamps to six"), State.RemainingRounds, 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG036CanonicalReactionTableTest, "Grimrock.RPG.RPG03.6.CanonicalReactionTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG036CanonicalReactionTableTest::RunTest(const FString&)
{
	struct FCase
	{
		EGridCombatSurfaceType Input;
		EGridCombatSurfaceInteraction Interaction;
		EGridCombatSurfaceType Output;
		bool bExplosive;
	};
	const FCase Cases[] = {
		{ EGridCombatSurfaceType::Oil, EGridCombatSurfaceInteraction::Fire, EGridCombatSurfaceType::Fire, false },
		{ EGridCombatSurfaceType::Poison, EGridCombatSurfaceInteraction::Fire, EGridCombatSurfaceType::Fire, true },
		{ EGridCombatSurfaceType::Water, EGridCombatSurfaceInteraction::Ice, EGridCombatSurfaceType::Ice, false },
		{ EGridCombatSurfaceType::Water, EGridCombatSurfaceInteraction::Lightning, EGridCombatSurfaceType::ElectrifiedWater, false },
		{ EGridCombatSurfaceType::Ice, EGridCombatSurfaceInteraction::Fire, EGridCombatSurfaceType::Water, false },
		{ EGridCombatSurfaceType::PoisonCloud, EGridCombatSurfaceInteraction::Fire, EGridCombatSurfaceType::Fire, true },
		{ EGridCombatSurfaceType::Smoke, EGridCombatSurfaceInteraction::Wind, EGridCombatSurfaceType::None, false }
	};

	for (const FCase& Case : Cases)
	{
		FGridCombatSurfaceReactionResult Reaction;
		TestTrue(TEXT("Canonical surface reaction resolves"),
			FGridCombatSurfaceResolver::ResolveReaction(RPG036::MakeState(Case.Input), Case.Interaction, FGridResolvedCombatModifiers(), Reaction));
		TestEqual(TEXT("Canonical reaction output type"), Reaction.OutputSurfaceType, Case.Output);
		TestEqual(TEXT("Canonical explosion flag"), Reaction.bExplosive, Case.bExplosive);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG036ChainReactionModifiersTest, "Grimrock.RPG.RPG03.6.ChainReactionModifiers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG036ChainReactionModifiersTest::RunTest(const FString&)
{
	FGridCombatModifierProfile Profile;
	Profile.SurfaceReactionDamagePercentModifier = 25;
	Profile.SurfaceReactionAreaRadiusModifier = 1;
	FGridResolvedCombatModifiers Modifiers;
	FGridCombatModifierContext Context;
	Context.ActionId = TEXT("Action_Bomb");
	Context.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
	Context.ActionType = EGridCombatActionType::Ability;
	FGridCombatModifierResolver::Resolve({ Profile }, Context, Modifiers);

	FGridCombatSurfaceReactionResult Reaction;
	TestTrue(TEXT("Poison plus fire is explosive"),
		FGridCombatSurfaceResolver::ResolveReaction(
			RPG036::MakeState(EGridCombatSurfaceType::Poison), EGridCombatSurfaceInteraction::Fire, Modifiers, Reaction));
	TestTrue(TEXT("Reaction is marked explosive"), Reaction.bExplosive);
	TestEqual(TEXT("Reaction carries plus twenty-five damage modifier"), Reaction.ExplosionDamagePercentModifier, 25);
	TestEqual(TEXT("Reaction carries plus one area modifier"), Reaction.ExplosionAreaRadiusModifier, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG036RuntimePersistenceTest, "Grimrock.RPG.RPG03.6.RuntimePersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG036RuntimePersistenceTest::RunTest(const FString&)
{
	RPG036::FTestWorld World;
	AGridLevelRuntimeActor* Runtime = RPG036::MakeRuntime(World);
	if (!Runtime)
	{
		return false;
	}
	FGridResolvedCombatModifiers Modifiers;
	TestTrue(TEXT("Runtime accepts persistent Oil surface"),
		Runtime->ApplyCombatSurfaceAtCell(1, 1, RPG036::MakeSurface(EGridCombatSurfaceType::Oil, 4), FGuid::NewGuid(), TEXT("Action_Oil"), Modifiers));

	const FGridCombatSurfaceState* Surface = Runtime->FindCombatSurfaceAtCell(1, 1);
	TestTrue(TEXT("Surface is readable from current-level authority"), Surface && Surface->SurfaceType == EGridCombatSurfaceType::Oil);
	const FGridLevelRuntimeState* State = Runtime->FindRuntimeStateForCurrentLevel();
	TestTrue(TEXT("Surface is stored inside current-level runtime state"), State && State->Surfaces.Contains(FIntPoint(1, 1)));

	const FProperty* Property = FindFProperty<FProperty>(FGridLevelRuntimeState::StaticStruct(), GET_MEMBER_NAME_CHECKED(FGridLevelRuntimeState, Surfaces));
	TestTrue(TEXT("Surfaces field is SaveGame-persistent"), Property && Property->HasAnyPropertyFlags(CPF_SaveGame));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG036DurationAdvanceTest, "Grimrock.RPG.RPG03.6.DurationAdvance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG036DurationAdvanceTest::RunTest(const FString&)
{
	RPG036::FTestWorld World;
	AGridLevelRuntimeActor* Runtime = RPG036::MakeRuntime(World);
	if (!Runtime)
	{
		return false;
	}
	FGridResolvedCombatModifiers Modifiers;
	Runtime->ApplyCombatSurfaceAtCell(1, 1, RPG036::MakeSurface(EGridCombatSurfaceType::Water, 2), FGuid::NewGuid(), TEXT("Action_Water"), Modifiers);
	TestEqual(TEXT("First round boundary expires nothing"), Runtime->AdvanceCombatSurfaceRound(), 0);
	const FGridCombatSurfaceState* AfterOne = Runtime->FindCombatSurfaceAtCell(1, 1);
	TestEqual(TEXT("One round remains"), AfterOne ? AfterOne->RemainingRounds : -1, 1);
	TestEqual(TEXT("Second boundary expires surface"), Runtime->AdvanceCombatSurfaceRound(), 1);
	TestNull(TEXT("Expired surface is removed from cell"), Runtime->FindCombatSurfaceAtCell(1, 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG036RuntimeReactionMutationTest, "Grimrock.RPG.RPG03.6.RuntimeReactionMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG036RuntimeReactionMutationTest::RunTest(const FString&)
{
	RPG036::FTestWorld World;
	AGridLevelRuntimeActor* Runtime = RPG036::MakeRuntime(World);
	if (!Runtime)
	{
		return false;
	}
	FGridResolvedCombatModifiers Modifiers;
	Runtime->ApplyCombatSurfaceAtCell(
		1, 1, RPG036::MakeSurface(EGridCombatSurfaceType::PoisonCloud, 3, EGridDamageType::Poison, 2), FGuid::NewGuid(), TEXT("Action_Cloud"), Modifiers);

	FGridCombatSurfaceReactionResult Reaction;
	TestTrue(TEXT("Runtime resolves PoisonCloud plus Fire"),
		Runtime->InteractCombatSurfaceAtCell(1, 1, EGridCombatSurfaceInteraction::Fire, Modifiers, Reaction));
	const FGridCombatSurfaceState* Surface = Runtime->FindCombatSurfaceAtCell(1, 1);
	TestTrue(TEXT("Reaction converts cell to Fire"), Surface && Surface->SurfaceType == EGridCombatSurfaceType::Fire);
	TestEqual(TEXT("Reaction preserves remaining duration"), Surface ? Surface->RemainingRounds : -1, 3);
	TestEqual(TEXT("Reaction does not invent Fire periodic damage"), Surface ? Surface->PeriodicDamagePerRound : -1, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG036PeriodicArmorGateTest, "Grimrock.RPG.RPG03.6.PeriodicArmorGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG036PeriodicArmorGateTest::RunTest(const FString&)
{
	FGridAttackTargetStats Target;
	Target.CurrentHealth = 10;
	Target.MagicalArmor = 1;
	Target.DamageMultiplier = 1.0f;
	const FGridAttackResult Damage = FGridCombatResolver::ResolveDirectDamage(Target, EGridDamageType::Poison, 2);
	TestEqual(TEXT("Surface tick first consumes magical armor"), Damage.MagicalArmorDamage, 1);
	TestEqual(TEXT("Surface tick can overflow normally to health"), Damage.HealthDamage, 1);

	FGridCombatStatusApplicationProfile Poison;
	Poison.StatusEffectId = TEXT("Status_Poison");
	Poison.Trigger = EGridCombatStatusApplicationTrigger::AfterResolution;
	Poison.ArmorGate = EGridCombatStatusArmorGate::MagicalArmorDepleted;
	TestTrue(TEXT("C1 armor gate sees armor depleted by the same surface tick"),
		FGridCombatStatusApplicationResolver::IsEligible(Poison, Target, &Damage));
	return true;
}

#endif

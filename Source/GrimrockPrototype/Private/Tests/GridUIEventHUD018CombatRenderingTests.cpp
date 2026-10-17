#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridCombatEventFeedFormatter.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Combat/GridPlayerAttackPresentationTypes.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridEventFeedSubsystem.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace
{
	FGridAttackResult MakeDetailedResult(EGridDamageType DamageType, int32 NaturalRoll, int32 DamageRoll)
	{
		FGridAttackSourceStats Source;
		Source.Accuracy = 3;
		Source.DamageBonus = 2;
		Source.RawDamagePercent = 150;
		Source.DamageMultiplier = 1.25f;
		Source.CriticalChancePercent = 5;
		Source.CriticalDamagePercent = 200;

		FGridAttackTargetStats Target;
		Target.Evasion = 4;
		Target.CurrentHealth = 50;
		Target.PhysicalArmor = 5;
		Target.MagicalArmor = 7;
		Target.ResistancePercent = 20;
		Target.DamageMultiplier = 0.8f;

		FGridAttackDefinition Attack;
		Attack.DamageType = DamageType;
		Attack.PhysicalSubtype =
			DamageType == EGridDamageType::Physical ? EGridPhysicalDamageSubtype::Slashing : EGridPhysicalDamageSubtype::None;
		Attack.MinDamage = 1;
		Attack.MaxDamage = 8;
		Attack.AccuracyBonus = 2;
		return FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, NaturalRoll, DamageRoll);
	}

	FGridCombatLogEntry MakeAttackLog(const FGridAttackResult& Result)
	{
		FGridCombatLogEntry Entry;
		Entry.Type = Result.bHit ? EGridCombatLogEntryType::AttackHit : EGridCombatLogEntryType::AttackMiss;
		Entry.SourceId = TEXT("Source");
		Entry.SourceDisplayName = FText::FromString(TEXT("Elias"));
		Entry.TargetId = TEXT("Target");
		Entry.TargetDisplayName = FText::FromString(TEXT("Rat géant"));
		Entry.AttackResult = Result;
		Entry.Message = FText::FromString(TEXT("Legacy summary"));
		return Entry;
	}

	bool ContainsDetail(const TArray<FText>& Details, const FString& Needle)
	{
		return Details.ContainsByPredicate([&Needle](const FText& Line)
		{
			return Line.ToString().Contains(Needle);
		});
	}

	struct FEventHUD018WorldScope
	{
		UWorld* World = nullptr;

		FEventHUD018WorldScope()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);
			World = UWorld::CreateWorld(
				EWorldType::Game,
				false,
				FName(*FString::Printf(TEXT("EVENTHUD018_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr,
				true,
				ERHIFeatureLevel::Num,
				&Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FEventHUD018WorldScope()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
				World = nullptr;
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD018AttackProjectionTest, "Grimrock.UI.EventHUD01.CombatRendering.AttackProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD018AttackProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGridAttackResult Result = MakeDetailedResult(EGridDamageType::Fire, 15, 6);
	FGridHudEventEntry FeedEntry;
	TestTrue(TEXT("Resolved attack projects to Event Feed"), FGridCombatEventFeedFormatter::BuildEntry(MakeAttackLog(Result), FeedEntry));
	TestEqual(TEXT("Attack projection uses Combat category"), FeedEntry.Category, EGridHudEventCategory::Combat);
	TestEqual(TEXT("Fire attack uses Fire semantic"), FeedEntry.Semantic, EGridHudEventSemantic::Fire);
	TestEqual(TEXT("Hit event code is stable"), FeedEntry.EventCode, FName(TEXT("Combat.Attack.Hit")));
	TestTrue(TEXT("Header names both combatants"),
		FeedEntry.Header.ToString().Contains(TEXT("Elias")) && FeedEntry.Header.ToString().Contains(TEXT("Rat géant")));
	TestTrue(TEXT("Header reports hit outcome"), FeedEntry.Header.ToString().Contains(TEXT("TOUCHÉ")));
	TestTrue(TEXT("Attack roll is rendered"), ContainsDetail(FeedEntry.Details, TEXT("1d20")));
	TestTrue(TEXT("Attack total is rendered"), ContainsDetail(FeedEntry.Details, TEXT("Total attaque")));
	TestTrue(TEXT("Defense total is rendered"), ContainsDetail(FeedEntry.Details, TEXT("Total défense")));
	TestTrue(TEXT("Damage range is rendered honestly"), ContainsDetail(FeedEntry.Details, TEXT("Tirage 1..8")));
	TestTrue(TEXT("Raw damage is rendered"), ContainsDetail(FeedEntry.Details, TEXT("Dégâts bruts")));
	TestTrue(TEXT("Resistance is rendered"), ContainsDetail(FeedEntry.Details, TEXT("Résistance")));
	TestTrue(TEXT("Health transition is rendered"), ContainsDetail(FeedEntry.Details, TEXT("PV :")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD018MissProjectionTest, "Grimrock.UI.EventHUD01.CombatRendering.MissProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD018MissProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGridAttackResult Result = MakeDetailedResult(EGridDamageType::Physical, 1, 8);
	FGridHudEventEntry FeedEntry;
	TestTrue(TEXT("Miss projects to Event Feed"), FGridCombatEventFeedFormatter::BuildEntry(MakeAttackLog(Result), FeedEntry));
	TestEqual(TEXT("Miss uses neutral information semantic"), FeedEntry.Semantic, EGridHudEventSemantic::Information);
	TestEqual(TEXT("Miss event code is stable"), FeedEntry.EventCode, FName(TEXT("Combat.Attack.Miss")));
	TestTrue(TEXT("Miss still renders attack total"), ContainsDetail(FeedEntry.Details, TEXT("Total attaque")));
	TestTrue(TEXT("Miss still renders defense"), ContainsDetail(FeedEntry.Details, TEXT("Total défense")));
	TestFalse(TEXT("Miss does not invent a damage section"), ContainsDetail(FeedEntry.Details, TEXT("Dégâts bruts")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD018DamageSemanticTest, "Grimrock.UI.EventHUD01.CombatRendering.DamageSemantics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD018DamageSemanticTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestEqual(TEXT("Physical"), FGridCombatEventFeedFormatter::ResolveDamageSemantic(EGridDamageType::Physical), EGridHudEventSemantic::PhysicalDamage);
	TestEqual(TEXT("Fire"), FGridCombatEventFeedFormatter::ResolveDamageSemantic(EGridDamageType::Fire), EGridHudEventSemantic::Fire);
	TestEqual(TEXT("Ice"), FGridCombatEventFeedFormatter::ResolveDamageSemantic(EGridDamageType::Ice), EGridHudEventSemantic::Ice);
	TestEqual(TEXT("Lightning"), FGridCombatEventFeedFormatter::ResolveDamageSemantic(EGridDamageType::Lightning), EGridHudEventSemantic::Lightning);
	TestEqual(TEXT("Poison"), FGridCombatEventFeedFormatter::ResolveDamageSemantic(EGridDamageType::Poison), EGridHudEventSemantic::Poison);
	TestEqual(TEXT("Holy"), FGridCombatEventFeedFormatter::ResolveDamageSemantic(EGridDamageType::Holy), EGridHudEventSemantic::Holy);
	TestEqual(TEXT("Necrotic"), FGridCombatEventFeedFormatter::ResolveDamageSemantic(EGridDamageType::Necrotic), EGridHudEventSemantic::Necrotic);
	TestEqual(TEXT("Arcane"), FGridCombatEventFeedFormatter::ResolveDamageSemantic(EGridDamageType::Arcane), EGridHudEventSemantic::Arcane);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD018StatusTickProjectionTest, "Grimrock.UI.EventHUD01.CombatRendering.StatusTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD018StatusTickProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridAttackTargetStats Target;
	Target.CurrentHealth = 30;
	Target.MagicalArmor = 3;
	Target.ResistancePercent = 25;
	Target.DamageMultiplier = 0.5f;

	FGridCombatLogEntry StatusEntry;
	StatusEntry.Type = EGridCombatLogEntryType::StatusTicked;
	StatusEntry.SourceId = TEXT("PoisonSource");
	StatusEntry.TargetDisplayName = FText::FromString(TEXT("Rat géant"));
	StatusEntry.Message = FText::FromString(TEXT("Poison affecte Rat géant."));
	StatusEntry.AttackResult = FGridCombatResolver::ResolveDirectDamage(Target, EGridDamageType::Poison, 20);

	FGridHudEventEntry FeedEntry;
	TestTrue(TEXT("Status tick projects to Event Feed"), FGridCombatEventFeedFormatter::BuildEntry(StatusEntry, FeedEntry));
	TestEqual(TEXT("Status tick stays Combat"), FeedEntry.Category, EGridHudEventCategory::Combat);
	TestEqual(TEXT("Poison tick uses Poison semantic"), FeedEntry.Semantic, EGridHudEventSemantic::Poison);
	TestEqual(TEXT("Status tick has stable event code"), FeedEntry.EventCode, FName(TEXT("Combat.Status.Ticked")));
	TestFalse(TEXT("Direct damage does not invent an attack roll"), ContainsDetail(FeedEntry.Details, TEXT("Jet d'attaque")));
	TestTrue(TEXT("Direct raw damage is rendered"), ContainsDetail(FeedEntry.Details, TEXT("Dégâts bruts")));
	TestTrue(TEXT("Direct health transition is rendered"), ContainsDetail(FeedEntry.Details, TEXT("PV :")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD018CentralPublicationTest, "Grimrock.UI.EventHUD01.CombatRendering.CentralPublication",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD018CentralPublicationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FEventHUD018WorldScope Scope;
	if (!TestNotNull(TEXT("World exists"), Scope.World))
	{
		return false;
	}
	AGridLevelRuntimeActor* Runtime = Scope.World->SpawnActor<AGridLevelRuntimeActor>();
	UGridEventFeedSubsystem* Feed = Scope.World->GetSubsystem<UGridEventFeedSubsystem>();
	UGridTurnManagerComponent* TurnManager = Runtime ? Runtime->FindComponentByClass<UGridTurnManagerComponent>() : nullptr;
	if (Runtime && !TurnManager)
	{
		TurnManager = NewObject<UGridTurnManagerComponent>(Runtime);
		TurnManager->RegisterComponent();
	}
	if (!TestNotNull(TEXT("Runtime exists"), Runtime) || !TestNotNull(TEXT("Feed exists"), Feed) || !TestNotNull(TEXT("TurnManager exists"), TurnManager))
	{
		return false;
	}

	FGridCombatLogEntry AttackEntry = MakeAttackLog(MakeDetailedResult(EGridDamageType::Physical, 15, 6));
	TurnManager->AppendCombatLogEntry(AttackEntry);

	TestEqual(TEXT("One combat log attack creates one logical feed event"), Feed->GetEntries().Num(), 1);
	if (Feed->GetEntries().Num() == 1)
	{
		TestEqual(TEXT("Published entry is Combat"), Feed->GetEntries()[0].Category, EGridHudEventCategory::Combat);
		TestTrue(TEXT("Published entry keeps detailed lines"), Feed->GetEntries()[0].Details.Num() > 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD018PopupFacadeTest, "Grimrock.UI.EventHUD01.CombatRendering.PopupFacade",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD018PopupFacadeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FEventHUD018WorldScope Scope;
	if (!TestNotNull(TEXT("World exists"), Scope.World))
	{
		return false;
	}
	AGridLevelRuntimeActor* Runtime = Scope.World->SpawnActor<AGridLevelRuntimeActor>();
	UGridEventFeedSubsystem* Feed = Scope.World->GetSubsystem<UGridEventFeedSubsystem>();
	if (!TestNotNull(TEXT("Runtime exists"), Runtime) || !TestNotNull(TEXT("Feed exists"), Feed))
	{
		return false;
	}

	FGridPlayerAttackFeedbackRequest Accepted;
	Accepted.bAccepted = true;
	Accepted.PrimaryText = FText::FromString(TEXT("Accepted result already comes from combat log."));
	Runtime->ShowCombatFeedback(Accepted);
	TestEqual(TEXT("Accepted legacy feedback does not duplicate the combat log"), Feed->GetEntries().Num(), 0);

	FGridPlayerAttackFeedbackRequest Rejected;
	Rejected.bAccepted = false;
	Rejected.PrimaryText = FText::FromString(TEXT("L'attaque est impossible."));
	Runtime->ShowCombatFeedback(Rejected);
	TestEqual(TEXT("Rejected feedback is migrated once"), Feed->GetEntries().Num(), 1);
	if (Feed->GetEntries().Num() == 1)
	{
		TestEqual(TEXT("Rejected feedback uses stable event code"), Feed->GetEntries()[0].EventCode, FName(TEXT("Combat.Attack.Rejected")));
	}
	return true;
}

#endif

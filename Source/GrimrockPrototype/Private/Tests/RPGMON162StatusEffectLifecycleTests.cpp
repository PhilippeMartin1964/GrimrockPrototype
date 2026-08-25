#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"

namespace
{
	UGridStatusEffectDefinitionAsset* MON162MakeDefinition(UObject* Outer, FName EffectId,
		EGridStatusEffectDurationUnit DurationUnit = EGridStatusEffectDurationUnit::Rounds, int32 DefaultDuration = 3,
		EGridStatusEffectStackPolicy StackPolicy = EGridStatusEffectStackPolicy::NoStack, int32 MaxStacks = 1, int32 DefaultPotency = 0)
	{
		UGridStatusEffectDefinitionAsset* Definition = NewObject<UGridStatusEffectDefinitionAsset>(Outer);
		Definition->EffectId = EffectId;
		Definition->DisplayName = FText::FromName(EffectId);
		Definition->DurationUnit = DurationUnit;
		Definition->DefaultDuration = DefaultDuration;
		Definition->StackPolicy = StackPolicy;
		Definition->MaxStacks = MaxStacks;
		Definition->DefaultPotency = DefaultPotency;
		return Definition;
	}

	struct FGridMON162TestWorld
	{
		UWorld* World = nullptr;
		FGridMON162TestWorld()
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
				FName(*FString::Printf(TEXT("MON162TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}
		~FGridMON162TestWorld()
		{
			if (!World)
				return;
			World->DestroyWorld(false);
			if (GEngine)
				GEngine->DestroyWorldContext(World);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON162TurnDurationLifecycleTest, "Grimrock.RPG.MON16.2.TurnDurationLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON162TurnDurationLifecycleTest::RunTest(const FString& Parameters)
{
	UGridStatusEffectDefinitionAsset* TurnEffect = MON162MakeDefinition(GetTransientPackage(), TEXT("BurningTurn"), EGridStatusEffectDurationUnit::Turns, 2);
	UGridStatusEffectDefinitionAsset* RoundEffect = MON162MakeDefinition(GetTransientPackage(), TEXT("PoisonRound"), EGridStatusEffectDurationUnit::Rounds, 4);
	FGridStatusEffectCollection Collection;
	FString Error;
	TestTrue(TEXT("Turn effect is added"), Collection.TryAdd(*TurnEffect, FGuid::NewGuid(), Error));
	TestTrue(TEXT("Round effect is added"), Collection.TryAdd(*RoundEffect, FGuid::NewGuid(), Error));
	FGridStatusEffectAdvanceResult First;
	Collection.AdvanceDuration(EGridStatusEffectDurationUnit::Turns, First);
	const FGridStatusEffectRuntimeState* TurnState = Collection.FindByEffectId(TEXT("BurningTurn"));
	const FGridStatusEffectRuntimeState* RoundState = Collection.FindByEffectId(TEXT("PoisonRound"));
	TestEqual(TEXT("Turn decrements once"), TurnState ? TurnState->RemainingDuration : -1, 1);
	TestEqual(TEXT("Round untouched by turn"), RoundState ? RoundState->RemainingDuration : -1, 4);
	TestEqual(TEXT("One effect advanced"), First.AdvancedEffectIds.Num(), 1);
	FGridStatusEffectAdvanceResult Second;
	Collection.AdvanceDuration(EGridStatusEffectDurationUnit::Turns, Second);
	TestFalse(TEXT("Turn effect expires"), Collection.Contains(TEXT("BurningTurn")));
	TestTrue(TEXT("Round remains"), Collection.Contains(TEXT("PoisonRound")));
	TestTrue(TEXT("Expired id reported"), Second.ExpiredEffectIds.Num() == 1 && Second.ExpiredEffectIds[0] == FName(TEXT("BurningTurn")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON162RoundDurationLifecycleTest, "Grimrock.RPG.MON16.2.RoundDurationLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON162RoundDurationLifecycleTest::RunTest(const FString& Parameters)
{
	UGridStatusEffectDefinitionAsset* RoundEffect = MON162MakeDefinition(GetTransientPackage(), TEXT("Poison"), EGridStatusEffectDurationUnit::Rounds, 2);
	UGridStatusEffectDefinitionAsset* TurnEffect = MON162MakeDefinition(GetTransientPackage(), TEXT("Bleeding"), EGridStatusEffectDurationUnit::Turns, 5);
	FGridStatusEffectCollection Collection;
	FString Error;
	TestTrue(TEXT("Round effect is added"), Collection.TryAdd(*RoundEffect, FGuid::NewGuid(), Error));
	TestTrue(TEXT("Turn effect is added"), Collection.TryAdd(*TurnEffect, FGuid::NewGuid(), Error));
	FGridStatusEffectAdvanceResult Advance;
	Collection.AdvanceDuration(EGridStatusEffectDurationUnit::Rounds, Advance);
	const FGridStatusEffectRuntimeState* RoundState = Collection.FindByEffectId(TEXT("Poison"));
	const FGridStatusEffectRuntimeState* TurnState = Collection.FindByEffectId(TEXT("Bleeding"));
	TestEqual(TEXT("Round duration decrements"), RoundState ? RoundState->RemainingDuration : -1, 1);
	TestEqual(TEXT("Turn duration untouched"), TurnState ? TurnState->RemainingDuration : -1, 5);
	Collection.AdvanceDuration(EGridStatusEffectDurationUnit::Rounds, Advance);
	TestFalse(TEXT("Round effect expires"), Collection.Contains(TEXT("Poison")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON162PermanentDurationLifecycleTest, "Grimrock.RPG.MON16.2.PermanentDurationLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON162PermanentDurationLifecycleTest::RunTest(const FString& Parameters)
{
	UGridStatusEffectDefinitionAsset* Permanent =
		MON162MakeDefinition(GetTransientPackage(), TEXT("PermanentBlessing"), EGridStatusEffectDurationUnit::Permanent, 0);
	FGridStatusEffectCollection Collection;
	FString Error;
	TestTrue(TEXT("Permanent effect is added"), Collection.TryAdd(*Permanent, FGuid::NewGuid(), Error));
	FGridStatusEffectAdvanceResult TurnAdvance, RoundAdvance;
	Collection.AdvanceDuration(EGridStatusEffectDurationUnit::Turns, TurnAdvance);
	Collection.AdvanceDuration(EGridStatusEffectDurationUnit::Rounds, RoundAdvance);
	const FGridStatusEffectRuntimeState* State = Collection.FindByEffectId(TEXT("PermanentBlessing"));
	TestTrue(TEXT("Permanent remains"), State != nullptr);
	TestEqual(TEXT("Permanent stays zero"), State ? State->RemainingDuration : -1, 0);
	TestTrue(TEXT("Permanent never advances"), TurnAdvance.AdvancedEffectIds.IsEmpty() && RoundAdvance.AdvancedEffectIds.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON162NoStackAndRefreshTest, "Grimrock.RPG.MON16.2.NoStackAndRefresh", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON162NoStackAndRefreshTest::RunTest(const FString& Parameters)
{
	UGridStatusEffectDefinitionAsset* NoStack =
		MON162MakeDefinition(GetTransientPackage(), TEXT("Stunned"), EGridStatusEffectDurationUnit::Turns, 1, EGridStatusEffectStackPolicy::NoStack);
	UGridStatusEffectDefinitionAsset* Refresh = MON162MakeDefinition(
		GetTransientPackage(), TEXT("Haste"), EGridStatusEffectDurationUnit::Rounds, 3, EGridStatusEffectStackPolicy::RefreshDuration, 1, 2);
	FString Error;
	FGridStatusEffectApplyResult Result;
	FGridStatusEffectCollection NoStackCollection;
	const FGuid First = FGuid::NewGuid();
	TestTrue(TEXT("Initial NoStack succeeds"), NoStackCollection.TryApply(*NoStack, First, Result, Error));
	TestFalse(TEXT("NoStack duplicate rejected"), NoStackCollection.TryApply(*NoStack, FGuid::NewGuid(), Result, Error));
	TestTrue(TEXT("NoStack outcome"), Result.Outcome == EGridStatusEffectApplyOutcome::RejectedNoStack);
	TestTrue(TEXT("NoStack atomic"), NoStackCollection.FindByEffectId(TEXT("Stunned"))->SourceId == First);

	FGridStatusEffectCollection RefreshCollection;
	const FGuid Second = FGuid::NewGuid();
	TestTrue(TEXT("Initial refresh effect"), RefreshCollection.TryApply(*Refresh, First, 1, 2, 2, Result, Error));
	TestTrue(TEXT("Refresh succeeds"), RefreshCollection.TryApply(*Refresh, Second, 1, 5, 4, Result, Error));
	const FGridStatusEffectRuntimeState* Refreshed = RefreshCollection.FindByEffectId(TEXT("Haste"));
	TestTrue(TEXT("Refresh outcome"), Result.Outcome == EGridStatusEffectApplyOutcome::RefreshedDuration);
	TestTrue(TEXT("Refresh source"), Refreshed && Refreshed->SourceId == Second);
	TestEqual(TEXT("Refresh duration"), Refreshed ? Refreshed->RemainingDuration : -1, 5);
	TestEqual(TEXT("Refresh potency"), Refreshed ? Refreshed->Potency : -1, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON162AddStacksTest, "Grimrock.RPG.MON16.2.AddStacks", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON162AddStacksTest::RunTest(const FString& Parameters)
{
	UGridStatusEffectDefinitionAsset* Poison =
		MON162MakeDefinition(GetTransientPackage(), TEXT("Poison"), EGridStatusEffectDurationUnit::Rounds, 3, EGridStatusEffectStackPolicy::AddStacks, 3, 1);
	FGridStatusEffectCollection Collection;
	FGridStatusEffectApplyResult Result;
	FString Error;
	TestTrue(TEXT("Initial stacks"), Collection.TryApply(*Poison, FGuid::NewGuid(), 2, 2, 2, Result, Error));
	const FGuid Latest = FGuid::NewGuid();
	TestTrue(TEXT("Stack reapply"), Collection.TryApply(*Poison, Latest, 2, 4, 1, Result, Error));
	const FGridStatusEffectRuntimeState* State = Collection.FindByEffectId(TEXT("Poison"));
	TestTrue(TEXT("Stack outcome"), Result.Outcome == EGridStatusEffectApplyOutcome::AddedStacks);
	TestEqual(TEXT("Clamp MaxStacks"), State ? State->StackCount : -1, 3);
	TestEqual(TEXT("Refresh duration"), State ? State->RemainingDuration : -1, 4);
	TestEqual(TEXT("Keep strongest potency"), State ? State->Potency : -1, 2);
	TestTrue(TEXT("Latest source"), State && State->SourceId == Latest);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON162ReplaceIfStrongerTest, "Grimrock.RPG.MON16.2.ReplaceIfStronger", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON162ReplaceIfStrongerTest::RunTest(const FString& Parameters)
{
	UGridStatusEffectDefinitionAsset* Slow = MON162MakeDefinition(
		GetTransientPackage(), TEXT("Slow"), EGridStatusEffectDurationUnit::Rounds, 3, EGridStatusEffectStackPolicy::ReplaceIfStronger, 1, 5);
	FGridStatusEffectCollection Collection;
	FGridStatusEffectApplyResult Result;
	FString Error;
	const FGuid Original = FGuid::NewGuid();
	TestTrue(TEXT("Initial effect"), Collection.TryApply(*Slow, Original, 1, 3, 5, Result, Error));
	TestFalse(TEXT("Equal potency rejected"), Collection.TryApply(*Slow, FGuid::NewGuid(), 1, 6, 5, Result, Error));
	const FGridStatusEffectRuntimeState* Before = Collection.FindByEffectId(TEXT("Slow"));
	TestTrue(TEXT("Weak rejection atomic"), Before && Before->SourceId == Original && Before->RemainingDuration == 3 && Before->Potency == 5);
	const FGuid Strong = FGuid::NewGuid();
	TestTrue(TEXT("Stronger replaces"), Collection.TryApply(*Slow, Strong, 1, 2, 7, Result, Error));
	const FGridStatusEffectRuntimeState* After = Collection.FindByEffectId(TEXT("Slow"));
	TestTrue(TEXT("Replace outcome"), Result.Outcome == EGridStatusEffectApplyOutcome::ReplacedStronger);
	TestTrue(TEXT("Replacement state"), After && After->SourceId == Strong && After->RemainingDuration == 2 && After->Potency == 7);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON162AtomicFailureTest, "Grimrock.RPG.MON16.2.AtomicFailure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON162AtomicFailureTest::RunTest(const FString& Parameters)
{
	UGridStatusEffectDefinitionAsset* Rounds = MON162MakeDefinition(
		GetTransientPackage(), TEXT("ArmorBuff"), EGridStatusEffectDurationUnit::Rounds, 3, EGridStatusEffectStackPolicy::RefreshDuration, 1, 2);
	UGridStatusEffectDefinitionAsset* Turns = MON162MakeDefinition(
		GetTransientPackage(), TEXT("ArmorBuff"), EGridStatusEffectDurationUnit::Turns, 3, EGridStatusEffectStackPolicy::RefreshDuration, 1, 2);
	FGridStatusEffectCollection Collection;
	FGridStatusEffectApplyResult Result;
	FString Error;
	const FGuid Original = FGuid::NewGuid();
	TestTrue(TEXT("Initial state"), Collection.TryApply(*Rounds, Original, 1, 3, 2, Result, Error));
	TestFalse(TEXT("Negative potency rejected"), Collection.TryApply(*Rounds, FGuid::NewGuid(), 1, 4, -2, Result, Error));
	TestFalse(TEXT("DurationUnit mismatch rejected"), Collection.TryApply(*Turns, FGuid::NewGuid(), Result, Error));
	const FGridStatusEffectRuntimeState* State = Collection.FindByEffectId(TEXT("ArmorBuff"));
	TestTrue(
		TEXT("Failures preserve state"), State && State->SourceId == Original && State->RemainingDuration == 3 && State->Potency == 2 && Collection.Num() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON162DeterministicExpirationTest, "Grimrock.RPG.MON16.2.DeterministicExpiration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON162DeterministicExpirationTest::RunTest(const FString& Parameters)
{
	UGridStatusEffectDefinitionAsset* Poison = MON162MakeDefinition(GetTransientPackage(), TEXT("Poison"), EGridStatusEffectDurationUnit::Rounds, 1);
	UGridStatusEffectDefinitionAsset* Burning = MON162MakeDefinition(GetTransientPackage(), TEXT("Burning"), EGridStatusEffectDurationUnit::Rounds, 1);
	UGridStatusEffectDefinitionAsset* Haste = MON162MakeDefinition(GetTransientPackage(), TEXT("Haste"), EGridStatusEffectDurationUnit::Rounds, 2);
	FGridStatusEffectCollection Collection;
	FString Error;
	TestTrue(TEXT("Poison"), Collection.TryAdd(*Poison, FGuid::NewGuid(), Error));
	TestTrue(TEXT("Burning"), Collection.TryAdd(*Burning, FGuid::NewGuid(), Error));
	TestTrue(TEXT("Haste"), Collection.TryAdd(*Haste, FGuid::NewGuid(), Error));
	FGridStatusEffectAdvanceResult Advance;
	Collection.AdvanceDuration(EGridStatusEffectDurationUnit::Rounds, Advance);
	TestEqual(TEXT("Two expire"), Advance.ExpiredEffectIds.Num(), 2);
	TestTrue(TEXT("Deterministic order"), Advance.ExpiredEffectIds[0] == FName(TEXT("Burning")) && Advance.ExpiredEffectIds[1] == FName(TEXT("Poison")));
	TestTrue(TEXT("Only Haste remains"), Collection.Num() == 1 && Collection.Contains(TEXT("Haste")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON162TurnManagerEventIntegrationTest, "Grimrock.RPG.MON16.2.TurnManagerEventIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON162TurnManagerEventIntegrationTest::RunTest(const FString& Parameters)
{
	FGridMON162TestWorld TestWorld;
	TestNotNull(TEXT("World"), TestWorld.World);
	if (!TestWorld.World)
		return false;
	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	AActor* Owner = TestWorld.World->SpawnActor<AActor>();
	AGridMonsterActor* Monster = TestWorld.World->SpawnActor<AGridMonsterActor>();
	if (!Party || !Party->PartyInventoryComponent || !Owner || !Monster)
		return false;
	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>(Owner, TEXT("MON162TurnManager"));
	if (!TurnManager)
		return false;
	TurnManager->PartyPawn = Party;
	Monster->SpawnObjectId = FGuid::NewGuid();
	Monster->PersistentMonsterId = Monster->SpawnObjectId;
	TurnManager->CombatMonsters.Add(Monster);
	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.Add(Character);
	TArray<FGridCharacterInventoryState>& Characters = Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
	UGridStatusEffectDefinitionAsset* TurnEffect = MON162MakeDefinition(GetTransientPackage(), TEXT("TurnEffect"), EGridStatusEffectDurationUnit::Turns, 2);
	UGridStatusEffectDefinitionAsset* RoundEffect = MON162MakeDefinition(GetTransientPackage(), TEXT("RoundEffect"), EGridStatusEffectDurationUnit::Rounds, 2);
	FString Error;
	TestTrue(TEXT("Party turn"), Characters[0].StatusEffects.TryAdd(*TurnEffect, FGuid::NewGuid(), Error));
	TestTrue(TEXT("Party round"), Characters[0].StatusEffects.TryAdd(*RoundEffect, FGuid::NewGuid(), Error));
	TestTrue(TEXT("Monster turn"), Monster->StatusEffects.TryAdd(*TurnEffect, FGuid::NewGuid(), Error));
	TestTrue(TEXT("Monster round"), Monster->StatusEffects.TryAdd(*RoundEffect, FGuid::NewGuid(), Error));
	UGridStatusEffectLifecycleSubsystem* Lifecycle = NewObject<UGridStatusEffectLifecycleSubsystem>(TestWorld.World);
	if (!Lifecycle)
		return false;
	Lifecycle->BindToTurnManager(TurnManager);
	FGridCombatantInitiativeEntry PartyCompleted;
	PartyCompleted.Side = EGridCombatantSide::Party;
	PartyCompleted.CharacterIndex = 0;
	PartyCompleted.CombatantId = Character.CharacterId;
	PartyCompleted.State = EGridCombatantTurnState::Completed;
	TurnManager->OnCombatantStateChanged.Broadcast(PartyCompleted);
	FGridCombatantInitiativeEntry MonsterCompleted;
	MonsterCompleted.Side = EGridCombatantSide::Monster;
	MonsterCompleted.CombatantId = Monster->ResolvePersistenceId();
	MonsterCompleted.State = EGridCombatantTurnState::Completed;
	TurnManager->OnCombatantStateChanged.Broadcast(MonsterCompleted);
	TestEqual(TEXT("Party turn advances"), Characters[0].StatusEffects.FindByEffectId(TEXT("TurnEffect"))->RemainingDuration, 1);
	TestEqual(TEXT("Monster turn advances"), Monster->StatusEffects.FindByEffectId(TEXT("TurnEffect"))->RemainingDuration, 1);
	TurnManager->OnRoundStarted.Broadcast(1);
	TestEqual(TEXT("Round one baseline"), Characters[0].StatusEffects.FindByEffectId(TEXT("RoundEffect"))->RemainingDuration, 2);
	TurnManager->OnRoundStarted.Broadcast(2);
	TestEqual(TEXT("Party round advances"), Characters[0].StatusEffects.FindByEffectId(TEXT("RoundEffect"))->RemainingDuration, 1);
	TestEqual(TEXT("Monster round advances"), Monster->StatusEffects.FindByEffectId(TEXT("RoundEffect"))->RemainingDuration, 1);
	Lifecycle->UnbindFromTurnManager();
	TestNull(TEXT("Unbound"), Lifecycle->GetBoundTurnManager());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON162NoUIDependencyTest, "Grimrock.RPG.MON16.2.NoUIDependency", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON162NoUIDependencyTest::RunTest(const FString& Parameters)
{
	const TArray<FString> Paths = { TEXT("Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectTypes.h"),
		TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectTypes.cpp"),
		TEXT("Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"),
		TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectDefinitionAsset.cpp"),
		TEXT("Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"),
		TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.cpp") };
	for (const FString& RelativePath : Paths)
	{
		FString Source;
		const FString FullPath = FPaths::Combine(FPaths::ProjectDir(), RelativePath);
		TestTrue(*FString::Printf(TEXT("Load %s"), *RelativePath), FFileHelper::LoadFileToString(Source, *FullPath));
		TestFalse(*FString::Printf(TEXT("No UI %s"), *RelativePath), Source.Contains(TEXT("#include \"UI/")));
		TestFalse(*FString::Printf(TEXT("No UMG %s"), *RelativePath), Source.Contains(TEXT("UMG")));
		TestFalse(*FString::Printf(TEXT("No Widget %s"), *RelativePath), Source.Contains(TEXT("UUserWidget")));
	}
	return true;
}

#endif

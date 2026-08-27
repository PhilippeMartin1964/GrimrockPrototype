#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"
#include "RPG/StatusEffects/GridStatusEffectPeriodicDamageResolver.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

namespace
{
	UGridStatusEffectDefinitionAsset* MON163MakeDefinition(UObject* Outer, FName EffectId, EGridStatusEffectDurationUnit DurationUnit, int32 DefaultDuration,
		EGridDamageType DamageType, int32 DamagePerStack, EGridStatusEffectStackPolicy StackPolicy = EGridStatusEffectStackPolicy::NoStack, int32 MaxStacks = 1,
		int32 DefaultPotency = 0)
	{
		UGridStatusEffectDefinitionAsset* Definition = NewObject<UGridStatusEffectDefinitionAsset>(Outer);
		Definition->EffectId = EffectId;
		Definition->DisplayName = FText::FromName(EffectId);
		Definition->Disposition = EGridStatusEffectDisposition::Debuff;
		Definition->DurationUnit = DurationUnit;
		Definition->DefaultDuration = DefaultDuration;
		Definition->DefaultPotency = DefaultPotency;
		Definition->StackPolicy = StackPolicy;
		Definition->MaxStacks = MaxStacks;
		Definition->PeriodicDamage.DamageType = DamageType;
		Definition->PeriodicDamage.DamagePerStack = DamagePerStack;
		return Definition;
	}

	struct FGridMON163TestWorld
	{
		UWorld* World = nullptr;

		FGridMON163TestWorld()
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
				FName(*FString::Printf(TEXT("MON163TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridMON163TestWorld()
		{
			if (!World)
			{
				return;
			}
			World->DestroyWorld(false);
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
		}
	};

	UGridTurnManagerComponent* MON163CreateTurnManager(UWorld* World, AGrimrockPartyPawn*& OutParty, AActor*& OutOwner)
	{
		OutParty = World ? World->SpawnActor<AGrimrockPartyPawn>() : nullptr;
		OutOwner = World ? World->SpawnActor<AActor>() : nullptr;
		if (!OutParty || !OutOwner)
		{
			return nullptr;
		}
		UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>(OutOwner, TEXT("MON163TurnManager"));
		if (TurnManager)
		{
			TurnManager->PartyPawn = OutParty;
		}
		return TurnManager;
	}

	AGridMonsterActor* MON163CreateMonster(UWorld* World, UGridTurnManagerComponent* TurnManager, int32 Health, int32 PhysicalArmor = 0, int32 MagicalArmor = 0)
	{
		AGridMonsterActor* Monster = World ? World->SpawnActor<AGridMonsterActor>() : nullptr;
		if (!Monster)
		{
			return nullptr;
		}

		Monster->SpawnObjectId = FGuid::NewGuid();
		Monster->PersistentMonsterId = Monster->SpawnObjectId;
		Monster->CurrentHealth = FMath::Max(1, Health);
		Monster->CurrentPhysicalArmor = FMath::Max(0, PhysicalArmor);
		Monster->CurrentMagicalArmor = FMath::Max(0, MagicalArmor);
		Monster->bCombatStatsInitialized = true;
		if (TurnManager)
		{
			TurnManager->CombatMonsters.Add(Monster);
		}
		return Monster;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON163DefinitionValidationTest, "Grimrock.RPG.MON16.3.DefinitionValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON163DefinitionValidationTest::RunTest(const FString& Parameters)
{
	UGridStatusEffectDefinitionAsset* Poison = MON163MakeDefinition(GetTransientPackage(), TEXT("MON163_Poison"), EGridStatusEffectDurationUnit::Rounds, 3,
		EGridDamageType::Poison, 2, EGridStatusEffectStackPolicy::AddStacks, 4);
	FString Error;
	TestTrue(TEXT("Timed periodic definition is valid"), Poison->ValidateDefinition(Error));

	Poison->PeriodicDamage.DamagePerStack = -1;
	TestFalse(TEXT("Negative periodic damage is rejected"), Poison->ValidateDefinition(Error));

	UGridStatusEffectDefinitionAsset* PermanentDot =
		MON163MakeDefinition(GetTransientPackage(), TEXT("MON163_PermanentDot"), EGridStatusEffectDurationUnit::Permanent, 0, EGridDamageType::Fire, 1);
	TestFalse(TEXT("Permanent periodic damage is rejected in MON16.3"), PermanentDot->ValidateDefinition(Error));

	UGridStatusEffectDefinitionAsset* PermanentNonDot =
		MON163MakeDefinition(GetTransientPackage(), TEXT("MON163_PermanentNonDot"), EGridStatusEffectDurationUnit::Permanent, 0, EGridDamageType::Physical, 0);
	TestTrue(TEXT("Permanent non-periodic status remains valid"), PermanentNonDot->ValidateDefinition(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON163DirectDamagePipelineTest, "Grimrock.RPG.MON16.3.DirectDamagePipeline", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON163DirectDamagePipelineTest::RunTest(const FString& Parameters)
{
	FGridAttackTargetStats Target;
	Target.CurrentHealth = 10;
	Target.MagicalArmor = 3;
	Target.ResistancePercent = 20;
	Target.DamageMultiplier = 0.5f;

	const FGridAttackResult Result = FGridCombatResolver::ResolveDirectDamage(Target, EGridDamageType::Fire, 10);

	TestTrue(TEXT("Direct damage is a guaranteed hit"), Result.bHit);
	TestFalse(TEXT("Direct damage never crits"), Result.bCriticalHit);
	TestEqual(TEXT("Direct damage has no natural attack roll"), Result.NaturalAttackRoll, 0);
	TestEqual(TEXT("Raw damage is preserved"), Result.RawDamage, 10);
	TestEqual(TEXT("Multiplier then resistance are reused"), Result.DamageAfterModifiers, 4);
	TestEqual(TEXT("Fire drains magical armor"), Result.MagicalArmorDamage, 3);
	TestEqual(TEXT("Remaining damage reaches health"), Result.HealthDamage, 1);
	TestEqual(TEXT("Health snapshot is deterministic"), Result.TargetHealthAfter, 9);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON163DamageTypeRoutingTest, "Grimrock.RPG.MON16.3.DamageTypeRouting", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON163DamageTypeRoutingTest::RunTest(const FString& Parameters)
{
	FGridAttackTargetStats Target;
	Target.CurrentHealth = 10;
	Target.PhysicalArmor = 3;
	Target.MagicalArmor = 3;

	const FGridAttackResult Bleeding = FGridCombatResolver::ResolveDirectDamage(Target, EGridDamageType::Physical, 4);
	TestEqual(TEXT("Physical periodic damage uses physical armor"), Bleeding.PhysicalArmorDamage, 3);
	TestEqual(TEXT("Physical overflow reaches health"), Bleeding.HealthDamage, 1);

	const FGridAttackResult Burning = FGridCombatResolver::ResolveDirectDamage(Target, EGridDamageType::Fire, 4);
	TestEqual(TEXT("Fire periodic damage uses magical armor"), Burning.MagicalArmorDamage, 3);
	TestEqual(TEXT("Fire overflow reaches health"), Burning.HealthDamage, 1);

	Target.ResistancePercent = 50;
	const FGridAttackResult Poison = FGridCombatResolver::ResolveDirectDamage(Target, EGridDamageType::Poison, 8);
	TestEqual(TEXT("Poison resistance uses existing percentage mitigation"), Poison.DamageAfterModifiers, 4);
	TestEqual(TEXT("Poison uses magical armor"), Poison.MagicalArmorDamage, 3);
	TestEqual(TEXT("Poison overflow reaches health"), Poison.HealthDamage, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON163StackScalingTest, "Grimrock.RPG.MON16.3.StackScaling", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON163StackScalingTest::RunTest(const FString& Parameters)
{
	UGridStatusEffectDefinitionAsset* Poison = MON163MakeDefinition(GetTransientPackage(), TEXT("MON163_StackPoison"), EGridStatusEffectDurationUnit::Rounds, 3,
		EGridDamageType::Poison, 3, EGridStatusEffectStackPolicy::AddStacks, 4, 99);

	FGridStatusEffectRuntimeState State;
	FString Error;
	TestTrue(TEXT("Stacked runtime state builds"), Poison->BuildRuntimeState(FGuid::NewGuid(), 2, 3, State, Error));

	FGridAttackTargetStats Target;
	Target.CurrentHealth = 20;
	FGridStatusEffectPeriodicDamageResolution Resolution;
	TestTrue(TEXT("Periodic damage resolves"), FGridStatusEffectPeriodicDamageResolver::Resolve(State, Target, Resolution, Error));
	TestEqual(TEXT("Damage scales with stack count"), Resolution.RawDamage, 6);
	TestEqual(TEXT("Two stacks are reported"), Resolution.StackCount, 2);
	TestEqual(TEXT("Potency is not used as damage"), Resolution.DamageResult.HealthDamage, 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON163TurnLifecyclePartyTest, "Grimrock.RPG.MON16.3.TurnLifecycleParty", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON163TurnLifecyclePartyTest::RunTest(const FString& Parameters)
{
	FGridMON163TestWorld TestWorld;
	AGrimrockPartyPawn* Party = nullptr;
	AActor* Owner = nullptr;
	UGridTurnManagerComponent* TurnManager = MON163CreateTurnManager(TestWorld.World, Party, Owner);
	if (!TurnManager || !Party || !Party->PartyInventoryComponent)
	{
		return false;
	}

	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.Resources.CurrentHealth = 10;
	Character.DerivedStats.MaxHealth = 10;
	Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.Add(Character);
	FGridCharacterInventoryState& RuntimeCharacter = Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0];

	UGridStatusEffectDefinitionAsset* Burning =
		MON163MakeDefinition(GetTransientPackage(), TEXT("MON163_TurnBurning"), EGridStatusEffectDurationUnit::Turns, 1, EGridDamageType::Fire, 3);
	FString Error;
	TestTrue(TEXT("Turn DoT is applied"), RuntimeCharacter.StatusEffects.TryAdd(*Burning, FGuid::NewGuid(), Error));

	UGridStatusEffectLifecycleSubsystem* Lifecycle = NewObject<UGridStatusEffectLifecycleSubsystem>(TestWorld.World);
	Lifecycle->BindToTurnManager(TurnManager);

	FGridCombatantInitiativeEntry Completed;
	Completed.Side = EGridCombatantSide::Party;
	Completed.CharacterIndex = 0;
	Completed.CombatantId = RuntimeCharacter.CharacterId;
	Completed.State = EGridCombatantTurnState::Completed;
	TurnManager->OnCombatantStateChanged.Broadcast(Completed);

	TestEqual(TEXT("DoT damages party before turn expiration"), RuntimeCharacter.Resources.CurrentHealth, 7);
	TestFalse(TEXT("Final turn tick then status expires"), RuntimeCharacter.StatusEffects.Contains(TEXT("MON163_TurnBurning")));
	Lifecycle->UnbindFromTurnManager();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON163TurnLifecycleMonsterTest, "Grimrock.RPG.MON16.3.TurnLifecycleMonster", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON163TurnLifecycleMonsterTest::RunTest(const FString& Parameters)
{
	FGridMON163TestWorld TestWorld;
	AGrimrockPartyPawn* Party = nullptr;
	AActor* Owner = nullptr;
	UGridTurnManagerComponent* TurnManager = MON163CreateTurnManager(TestWorld.World, Party, Owner);
	AGridMonsterActor* Monster = MON163CreateMonster(TestWorld.World, TurnManager, 5, 0, 2);
	if (!TurnManager || !Monster)
	{
		return false;
	}

	UGridStatusEffectDefinitionAsset* Burning =
		MON163MakeDefinition(GetTransientPackage(), TEXT("MON163_MonsterBurning"), EGridStatusEffectDurationUnit::Turns, 1, EGridDamageType::Fire, 3);
	FString Error;
	TestTrue(TEXT("Monster DoT is applied"), Monster->StatusEffects.TryAdd(*Burning, FGuid::NewGuid(), Error));

	UGridStatusEffectLifecycleSubsystem* Lifecycle = NewObject<UGridStatusEffectLifecycleSubsystem>(TestWorld.World);
	Lifecycle->BindToTurnManager(TurnManager);

	FGridCombatantInitiativeEntry Completed;
	Completed.Side = EGridCombatantSide::Monster;
	Completed.CombatantId = Monster->ResolvePersistenceId();
	Completed.State = EGridCombatantTurnState::Completed;
	TurnManager->OnCombatantStateChanged.Broadcast(Completed);

	TestEqual(TEXT("Fire first consumes monster magical armor"), Monster->CurrentMagicalArmor, 0);
	TestEqual(TEXT("One point reaches monster health"), Monster->CurrentHealth, 4);
	TestFalse(TEXT("Monster final tick expires status"), Monster->StatusEffects.Contains(TEXT("MON163_MonsterBurning")));
	Lifecycle->UnbindFromTurnManager();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON163LethalMonsterDotTest, "Grimrock.RPG.MON16.3.LethalMonsterDot", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON163LethalMonsterDotTest::RunTest(const FString& Parameters)
{
	FGridMON163TestWorld TestWorld;
	AGrimrockPartyPawn* Party = nullptr;
	AActor* Owner = nullptr;
	UGridTurnManagerComponent* TurnManager = MON163CreateTurnManager(TestWorld.World, Party, Owner);
	AGridMonsterActor* Monster = MON163CreateMonster(TestWorld.World, TurnManager, 2);
	if (!TurnManager || !Monster)
	{
		return false;
	}

	UGridStatusEffectDefinitionAsset* Poison =
		MON163MakeDefinition(GetTransientPackage(), TEXT("MON163_LethalPoison"), EGridStatusEffectDurationUnit::Turns, 1, EGridDamageType::Poison, 2);
	FString Error;
	TestTrue(TEXT("Lethal DoT is applied"), Monster->StatusEffects.TryAdd(*Poison, FGuid::NewGuid(), Error));

	UGridStatusEffectLifecycleSubsystem* Lifecycle = NewObject<UGridStatusEffectLifecycleSubsystem>(TestWorld.World);
	Lifecycle->BindToTurnManager(TurnManager);
	FGridCombatantInitiativeEntry Completed;
	Completed.Side = EGridCombatantSide::Monster;
	Completed.CombatantId = Monster->ResolvePersistenceId();
	Completed.State = EGridCombatantTurnState::Completed;
	AddExpectedError(TEXT("Reason=MissingMonsterMovement; continuing death."), EAutomationExpectedErrorFlags::Contains, 1);
	TurnManager->OnCombatantStateChanged.Broadcast(Completed);

	TestTrue(TEXT("Periodic damage uses normal monster death path"), Monster->IsDead());
	TestEqual(TEXT("Lethal periodic damage reaches zero health"), Monster->CurrentHealth, 0);
	TestFalse(TEXT("Lethal final tick still expires status"), Monster->StatusEffects.Contains(TEXT("MON163_LethalPoison")));
	Lifecycle->UnbindFromTurnManager();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON163RoundLifecycleTest, "Grimrock.RPG.MON16.3.RoundLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON163RoundLifecycleTest::RunTest(const FString& Parameters)
{
	FGridMON163TestWorld TestWorld;
	AGrimrockPartyPawn* Party = nullptr;
	AActor* Owner = nullptr;
	UGridTurnManagerComponent* TurnManager = MON163CreateTurnManager(TestWorld.World, Party, Owner);
	if (!TurnManager || !Party || !Party->PartyInventoryComponent)
	{
		return false;
	}

	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.Resources.CurrentHealth = 10;
	Character.DerivedStats.MaxHealth = 10;
	Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.Add(Character);
	FGridCharacterInventoryState& RuntimeCharacter = Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0];

	UGridStatusEffectDefinitionAsset* Poison =
		MON163MakeDefinition(GetTransientPackage(), TEXT("MON163_RoundPoison"), EGridStatusEffectDurationUnit::Rounds, 2, EGridDamageType::Poison, 2);
	FString Error;
	TestTrue(TEXT("Round DoT is applied"), RuntimeCharacter.StatusEffects.TryAdd(*Poison, FGuid::NewGuid(), Error));

	UGridStatusEffectLifecycleSubsystem* Lifecycle = NewObject<UGridStatusEffectLifecycleSubsystem>(TestWorld.World);
	Lifecycle->BindToTurnManager(TurnManager);
	TurnManager->OnRoundStarted.Broadcast(1);
	TestEqual(TEXT("Round one is baseline and does not tick"), RuntimeCharacter.Resources.CurrentHealth, 10);
	TestEqual(TEXT("Round one does not decrement"), RuntimeCharacter.StatusEffects.FindByEffectId(TEXT("MON163_RoundPoison"))->RemainingDuration, 2);

	TurnManager->OnRoundStarted.Broadcast(2);
	TestEqual(TEXT("First round boundary ticks once"), RuntimeCharacter.Resources.CurrentHealth, 8);
	TestEqual(TEXT("First boundary decrements once"), RuntimeCharacter.StatusEffects.FindByEffectId(TEXT("MON163_RoundPoison"))->RemainingDuration, 1);

	TurnManager->OnRoundStarted.Broadcast(3);
	TestEqual(TEXT("Final round boundary ticks before expiry"), RuntimeCharacter.Resources.CurrentHealth, 6);
	TestFalse(TEXT("Round DoT expires after its second tick"), RuntimeCharacter.StatusEffects.Contains(TEXT("MON163_RoundPoison")));
	Lifecycle->UnbindFromTurnManager();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON163NonPeriodicIsolationTest, "Grimrock.RPG.MON16.3.NonPeriodicIsolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON163NonPeriodicIsolationTest::RunTest(const FString& Parameters)
{
	FGridMON163TestWorld TestWorld;
	AGrimrockPartyPawn* Party = nullptr;
	AActor* Owner = nullptr;
	UGridTurnManagerComponent* TurnManager = MON163CreateTurnManager(TestWorld.World, Party, Owner);
	if (!TurnManager || !Party || !Party->PartyInventoryComponent)
	{
		return false;
	}

	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.Resources.CurrentHealth = 10;
	Character.DerivedStats.MaxHealth = 10;
	Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.Add(Character);
	FGridCharacterInventoryState& RuntimeCharacter = Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0];

	UGridStatusEffectDefinitionAsset* NonPeriodic =
		MON163MakeDefinition(GetTransientPackage(), TEXT("MON163_NonPeriodic"), EGridStatusEffectDurationUnit::Rounds, 1, EGridDamageType::Physical, 0);
	FString Error;
	TestTrue(TEXT("Non-periodic status is applied"), RuntimeCharacter.StatusEffects.TryAdd(*NonPeriodic, FGuid::NewGuid(), Error));

	UGridStatusEffectLifecycleSubsystem* Lifecycle = NewObject<UGridStatusEffectLifecycleSubsystem>(TestWorld.World);
	Lifecycle->BindToTurnManager(TurnManager);
	TurnManager->OnRoundStarted.Broadcast(1);
	TurnManager->OnRoundStarted.Broadcast(2);

	TestEqual(TEXT("Non-periodic status never deals damage"), RuntimeCharacter.Resources.CurrentHealth, 10);
	TestFalse(TEXT("Non-periodic status still follows lifecycle"), RuntimeCharacter.StatusEffects.Contains(TEXT("MON163_NonPeriodic")));
	Lifecycle->UnbindFromTurnManager();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON163MonsterMultiplierTest, "Grimrock.RPG.MON16.3.MonsterDamageMultiplier", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON163MonsterMultiplierTest::RunTest(const FString& Parameters)
{
	FGridMON163TestWorld TestWorld;
	AGrimrockPartyPawn* Party = nullptr;
	AActor* Owner = nullptr;
	UGridTurnManagerComponent* TurnManager = MON163CreateTurnManager(TestWorld.World, Party, Owner);
	AGridMonsterActor* Monster = MON163CreateMonster(TestWorld.World, TurnManager, 10, 0, 1);
	if (!TurnManager || !Monster)
	{
		return false;
	}

	UGridMonsterDefinitionAsset* MonsterDefinition = NewObject<UGridMonsterDefinitionAsset>(GetTransientPackage());
	FGridMonsterDamageModifier Modifier;
	Modifier.DamageType = EGridDamageType::Poison;
	Modifier.PhysicalSubtype = EGridPhysicalDamageSubtype::None;
	Modifier.DamageMultiplier = 0.5f;
	MonsterDefinition->DamageModifiers.Add(Modifier);
	Monster->MonsterDefinition = MonsterDefinition;

	UGridStatusEffectDefinitionAsset* Poison =
		MON163MakeDefinition(GetTransientPackage(), TEXT("MON163_MultiplierPoison"), EGridStatusEffectDurationUnit::Turns, 1, EGridDamageType::Poison, 6);
	FString Error;
	TestTrue(TEXT("Poison is applied"), Monster->StatusEffects.TryAdd(*Poison, FGuid::NewGuid(), Error));

	UGridStatusEffectLifecycleSubsystem* Lifecycle = NewObject<UGridStatusEffectLifecycleSubsystem>(TestWorld.World);
	Lifecycle->BindToTurnManager(TurnManager);
	FGridCombatantInitiativeEntry Completed;
	Completed.Side = EGridCombatantSide::Monster;
	Completed.CombatantId = Monster->ResolvePersistenceId();
	Completed.State = EGridCombatantTurnState::Completed;
	TurnManager->OnCombatantStateChanged.Broadcast(Completed);

	TestEqual(TEXT("Existing monster multiplier halves six raw damage to three"), Monster->CurrentHealth, 8);
	TestEqual(TEXT("One point first drains magical armor"), Monster->CurrentMagicalArmor, 0);
	Lifecycle->UnbindFromTurnManager();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON163NoParallelSystemTest, "Grimrock.RPG.MON16.3.NoParallelSystem", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON163NoParallelSystemTest::RunTest(const FString& Parameters)
{
	const TArray<FString> Paths = { TEXT("Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"),
		TEXT("Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectPeriodicDamageResolver.h"),
		TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectPeriodicDamageResolver.cpp"),
		TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.cpp") };

	for (const FString& RelativePath : Paths)
	{
		FString Source;
		const FString FullPath = FPaths::Combine(FPaths::ProjectDir(), RelativePath);
		TestTrue(*FString::Printf(TEXT("Load %s"), *RelativePath), FFileHelper::LoadFileToString(Source, *FullPath));
		TestFalse(*FString::Printf(TEXT("No UI include %s"), *RelativePath), Source.Contains(TEXT("#include \"UI/")));
		TestFalse(*FString::Printf(TEXT("No UMG %s"), *RelativePath), Source.Contains(TEXT("UMG")));
		TestFalse(*FString::Printf(TEXT("No widget dependency %s"), *RelativePath), Source.Contains(TEXT("UUserWidget")));
	}

	FString ResolverSource;
	const FString ResolverPath =
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectPeriodicDamageResolver.cpp"));
	TestTrue(TEXT("Load periodic resolver for identity check"), FFileHelper::LoadFileToString(ResolverSource, *ResolverPath));
	TestFalse(TEXT("Periodic resolver does not hard-code Poison"), ResolverSource.Contains(TEXT("TEXT (\"Poison\")")));
	TestFalse(TEXT("Periodic resolver does not hard-code Bleeding"), ResolverSource.Contains(TEXT("TEXT (\"Bleeding\")")));
	TestFalse(TEXT("Periodic resolver does not hard-code Burning"), ResolverSource.Contains(TEXT("TEXT (\"Burning\")")));
	return true;
}

#endif

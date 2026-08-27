#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/StatusEffects/GridStatusEffectControlResolver.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"

namespace
{
	UGridStatusEffectDefinitionAsset* MON165MakeControlDefinition(UObject* Outer, FName EffectId, const FGridStatusEffectControlProfile& Control,
		EGridStatusEffectDurationUnit DurationUnit = EGridStatusEffectDurationUnit::Turns, int32 DefaultDuration = 1,
		EGridStatusEffectStackPolicy StackPolicy = EGridStatusEffectStackPolicy::NoStack, int32 MaxStacks = 1)
	{
		UGridStatusEffectDefinitionAsset* Definition = NewObject<UGridStatusEffectDefinitionAsset>(Outer);
		Definition->EffectId = EffectId;
		Definition->DisplayName = FText::FromName(EffectId);
		Definition->Disposition = EGridStatusEffectDisposition::Debuff;
		Definition->DurationUnit = DurationUnit;
		Definition->DefaultDuration = DefaultDuration;
		Definition->StackPolicy = StackPolicy;
		Definition->MaxStacks = MaxStacks;
		Definition->Control = Control;
		return Definition;
	}

	FGridCombatActionDefinition MON165MakeSelfClassAction(FName ActionId, EGridCombatActionSourcePolicy SourcePolicy, int32 RestoreHealth)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = ActionId;
		Action.DisplayName = FText::FromName(ActionId);
		Action.Description = FText::FromName(ActionId);
		Action.ActionType = EGridCombatActionType::Ability;
		Action.SourcePolicy = SourcePolicy;
		Action.TargetingPolicy = EGridCombatTargetingPolicy::Self;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Action.ActionPointCost = 1;
		Action.EffectProfile.RestoreHealth = RestoreHealth;
		return Action;
	}

	FGridCharacterInventoryState MON165MakeCharacter(const FGuid& CharacterId)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = CharacterId;
		Character.DisplayName = FText::FromString(TEXT("MON16.5 Hero"));
		Character.Resources.CurrentHealth = 5;
		Character.DerivedStats.MaxHealth = 10;
		Character.Resources.CurrentMana = 10;
		Character.DerivedStats.MaxMana = 10;
		Character.InventorySlots.SetNum(4);
		return Character;
	}

	FGridCombatantInitiativeEntry MON165MakePartyEntry(const FGuid& CharacterId)
	{
		FGridCombatantInitiativeEntry Entry;
		Entry.CombatantId = CharacterId;
		Entry.Side = EGridCombatantSide::Party;
		Entry.CharacterIndex = 0;
		Entry.DisplayName = FText::FromString(TEXT("MON16.5 Hero"));
		Entry.InitiativeBase = 20;
		Entry.InitiativeRoll = 10;
		Entry.InitiativeTotal = 30;
		Entry.CurrentHealth = 5;
		Entry.MaximumHealth = 10;
		Entry.State = EGridCombatantTurnState::Active;
		return Entry;
	}

	struct FGridMON165TestWorld
	{
		UWorld* World = nullptr;

		FGridMON165TestWorld()
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
				FName(*FString::Printf(TEXT("MON165TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridMON165TestWorld()
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

	struct FGridMON165PartyFixture
	{
		FGridMON165TestWorld TestWorld;
		AGridLevelRuntimeActor* Runtime = nullptr;
		AGrimrockPartyPawn* Party = nullptr;
		AActor* Owner = nullptr;
		UGridTurnManagerComponent* TurnManager = nullptr;
		FGuid CharacterId = FGuid(16, 5, 1, 1);

		FGridMON165PartyFixture()
		{
			if (!TestWorld.World)
			{
				return;
			}

			Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
			Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
			Owner = TestWorld.World->SpawnActor<AActor>();
			if (!Runtime || !Party || !Party->PartyInventoryComponent || !Owner)
			{
				return;
			}

			UGridLevelAsset* LevelAsset = NewObject<UGridLevelAsset>(Runtime);
			LevelAsset->Width = 3;
			LevelAsset->Height = 3;
			LevelAsset->EnsureCellCount();
			for (FGridLevelCellData& Cell : LevelAsset->Cells)
			{
				Cell.CellType = EGridCellType::Floor;
				Cell.bBlocksOccupancy = false;
			}
			Runtime->LevelAsset = LevelAsset;

			Party->LevelRuntimeActor = Runtime;
			Party->CurrentCellX = 1;
			Party->CurrentCellY = 1;
			Party->Facing = EGridEdge::North;
			Party->SnapToCurrentCell();
			Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { MON165MakeCharacter(CharacterId) };
			Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment.SetNum(1);

			TurnManager = NewObject<UGridTurnManagerComponent>(Owner, TEXT("MON165TurnManager"));
			if (!TurnManager || !TurnManager->InitializeTurnManager(Runtime, Party))
			{
				TurnManager = nullptr;
				return;
			}

			TurnManager->bCombatActive = true;
			TurnManager->CurrentPhase = EGridCombatPhase::PlayerPhase;
			TurnManager->RoundNumber = 1;
			TurnManager->InitiativeOrder = { MON165MakePartyEntry(CharacterId) };
			TurnManager->CurrentInitiativeIndex = 0;

			FGridPlayerCharacterTurnState TurnState;
			TurnState.CharacterIndex = 0;
			TurnState.CharacterId = CharacterId;
			TurnState.State = EGridCombatantTurnState::Active;
			TurnState.MaximumActionPoints = 4;
			TurnState.RemainingActionPoints = 4;
			TurnManager->PlayerCharacterTurnStates = { TurnState };
			TurnManager->PartyMobilityState.RoundNumber = 1;
			TurnManager->PartyMobilityState.MaximumMobilityActionPoints = 2;
			TurnManager->PartyMobilityState.RemainingMobilityActionPoints = 2;
		}

		FGridCharacterInventoryState* Character() const
		{
			return Party && Party->PartyInventoryComponent && Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex(0)
				? &Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0]
				: nullptr;
		}
	};

	const FGridAvailableCombatAction* MON165FindAction(const TArray<FGridAvailableCombatAction>& Actions, FName ActionId)
	{
		return Actions.FindByPredicate(
			[ActionId](const FGridAvailableCombatAction& Action)
			{
				return Action.Definition.ActionId == ActionId;
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON165ControlAggregationTest, "Grimrock.RPG.MON16.5.ControlAggregation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON165ControlAggregationTest::RunTest(const FString& Parameters)
{
	FGridStatusEffectControlProfile FirstControl;
	FirstControl.bSkipActivation = true;
	FGridStatusEffectControlProfile SecondControl;
	SecondControl.bBlockSpellActions = true;
	SecondControl.bBlockTranslation = true;

	UGridStatusEffectDefinitionAsset* First = MON165MakeControlDefinition(GetTransientPackage(), TEXT("MON165_ControlA"), FirstControl);
	UGridStatusEffectDefinitionAsset* Second = MON165MakeControlDefinition(GetTransientPackage(), TEXT("MON165_ControlB"), SecondControl);

	FGridStatusEffectCollection Collection;
	FString Error;
	TestTrue(TEXT("First arbitrary control effect applies"), Collection.TryAdd(*First, FGuid::NewGuid(), Error));
	TestTrue(TEXT("Second arbitrary control effect applies"), Collection.TryAdd(*Second, FGuid::NewGuid(), Error));

	const FGridStatusEffectControlProfile Result = FGridStatusEffectControlResolver::Resolve(Collection);
	TestTrue(TEXT("SkipActivation is OR-aggregated"), Result.bSkipActivation);
	TestTrue(TEXT("BlockSpellActions is OR-aggregated"), Result.bBlockSpellActions);
	TestTrue(TEXT("BlockTranslation is OR-aggregated"), Result.bBlockTranslation);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON165PermanentSkipActivationRejectedTest, "Grimrock.RPG.MON16.5.PermanentSkipActivationRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON165PermanentSkipActivationRejectedTest::RunTest(const FString& Parameters)
{
	FGridStatusEffectControlProfile Skip;
	Skip.bSkipActivation = true;
	UGridStatusEffectDefinitionAsset* PermanentSkip =
		MON165MakeControlDefinition(GetTransientPackage(), TEXT("MON165_PermanentSkip"), Skip, EGridStatusEffectDurationUnit::Permanent, 0);
	FString Error;
	TestFalse(TEXT("Permanent activation skipping is rejected"), PermanentSkip->ValidateDefinition(Error));
	TestTrue(TEXT("Validation explains the progress-safety rule"), Error.Contains(TEXT("SkipActivation")));

	FGridStatusEffectControlProfile PermanentControl;
	PermanentControl.bBlockSpellActions = true;
	PermanentControl.bBlockTranslation = true;
	UGridStatusEffectDefinitionAsset* NonSkip =
		MON165MakeControlDefinition(GetTransientPackage(), TEXT("MON165_PermanentControl"), PermanentControl, EGridStatusEffectDurationUnit::Permanent, 0);
	TestTrue(TEXT("Permanent silence/immobilize style control remains valid"), NonSkip->ValidateDefinition(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON165StackBooleanSemanticsTest, "Grimrock.RPG.MON16.5.StackBooleanSemantics", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON165StackBooleanSemanticsTest::RunTest(const FString& Parameters)
{
	FGridStatusEffectControlProfile Control;
	Control.bBlockTranslation = true;
	UGridStatusEffectDefinitionAsset* Definition = MON165MakeControlDefinition(
		GetTransientPackage(), TEXT("MON165_StackedControl"), Control, EGridStatusEffectDurationUnit::Rounds, 3, EGridStatusEffectStackPolicy::AddStacks, 3);

	FGridStatusEffectCollection Collection;
	FGridStatusEffectApplyResult Result;
	FString Error;
	TestTrue(TEXT("Three control stacks apply"), Collection.TryApply(*Definition, FGuid::NewGuid(), 3, INDEX_NONE, INDEX_NONE, Result, Error));
	TestEqual(TEXT("Stack count is retained by the common status model"), Collection.ActiveEffects[0].StackCount, 3);
	const FGridStatusEffectControlProfile Resolved = FGridStatusEffectControlResolver::Resolve(Collection);
	TestTrue(TEXT("Boolean control remains active regardless of stack count"), Resolved.bBlockTranslation);
	TestFalse(TEXT("Unconfigured capabilities remain false"), Resolved.bSkipActivation || Resolved.bBlockSpellActions);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON165TurnSkipLifecycleTest, "Grimrock.RPG.MON16.5.TurnSkipLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON165TurnSkipLifecycleTest::RunTest(const FString& Parameters)
{
	FGridMON165PartyFixture Fixture;
	FGridCharacterInventoryState* Character = Fixture.Character();
	if (!Fixture.TurnManager || !Character)
	{
		return false;
	}

	FGridStatusEffectControlProfile Control;
	Control.bSkipActivation = true;
	UGridStatusEffectDefinitionAsset* Definition =
		MON165MakeControlDefinition(GetTransientPackage(), TEXT("MON165_TurnSkip"), Control, EGridStatusEffectDurationUnit::Turns, 1);
	FString Error;
	TestTrue(TEXT("Turn skip status applies"), Character->StatusEffects.TryAdd(*Definition, FGuid::NewGuid(), Error));
	TestTrue(TEXT("Control resolver requests activation skipping"), FGridStatusEffectControlResolver::Resolve(Character->StatusEffects).bSkipActivation);

	UGridStatusEffectLifecycleSubsystem* Lifecycle = NewObject<UGridStatusEffectLifecycleSubsystem>(Fixture.TestWorld.World);
	Lifecycle->BindToTurnManager(Fixture.TurnManager);

	FGridCombatantInitiativeEntry Completed = Fixture.TurnManager->InitiativeOrder[0];
	Completed.State = EGridCombatantTurnState::Completed;
	Fixture.TurnManager->OnCombatantStateChanged.Broadcast(Completed);

	TestFalse(TEXT("A one-turn skipped activation expires the effect exactly once"), Character->StatusEffects.Contains(TEXT("MON165_TurnSkip")));
	Lifecycle->UnbindFromTurnManager();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON165RoundSkipLifecycleTest, "Grimrock.RPG.MON16.5.RoundSkipLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON165RoundSkipLifecycleTest::RunTest(const FString& Parameters)
{
	FGridMON165PartyFixture Fixture;
	FGridCharacterInventoryState* Character = Fixture.Character();
	if (!Fixture.TurnManager || !Character)
	{
		return false;
	}

	FGridStatusEffectControlProfile Control;
	Control.bSkipActivation = true;
	UGridStatusEffectDefinitionAsset* Definition =
		MON165MakeControlDefinition(GetTransientPackage(), TEXT("MON165_RoundSkip"), Control, EGridStatusEffectDurationUnit::Rounds, 1);
	FString Error;
	TestTrue(TEXT("Round skip status applies"), Character->StatusEffects.TryAdd(*Definition, FGuid::NewGuid(), Error));

	UGridStatusEffectLifecycleSubsystem* Lifecycle = NewObject<UGridStatusEffectLifecycleSubsystem>(Fixture.TestWorld.World);
	Fixture.TurnManager->RoundNumber = 1;
	Lifecycle->BindToTurnManager(Fixture.TurnManager);
	Fixture.TurnManager->OnRoundStarted.Broadcast(2);

	TestFalse(TEXT("One-round control expires on the next round boundary"), Character->StatusEffects.Contains(TEXT("MON165_RoundSkip")));
	Lifecycle->UnbindFromTurnManager();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON165SilenceCatalogIsolationTest, "Grimrock.RPG.MON16.5.SilenceCatalogIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON165SilenceCatalogIsolationTest::RunTest(const FString& Parameters)
{
	FGridMON165PartyFixture Fixture;
	FGridCharacterInventoryState* Character = Fixture.Character();
	if (!Fixture.TurnManager || !Character)
	{
		return false;
	}

	URPGClassAsset* Class = NewObject<URPGClassAsset>(GetTransientPackage());
	Class->ClassId = TEXT("MON165_Class");
	Class->DisplayName = FText::FromString(TEXT("MON16.5 Class"));
	Class->CombatActions = { MON165MakeSelfClassAction(TEXT("MON165_Spell"), EGridCombatActionSourcePolicy::Spell, 2),
		MON165MakeSelfClassAction(TEXT("MON165_Ability"), EGridCombatActionSourcePolicy::Ability, 1) };
	TestTrue(TEXT("Transient class fixture is valid"), Class->IsValidDefinition());
	Character->ClassId = Class->ClassId;
	Character->ClassDefinition = TSoftObjectPtr<URPGClassAsset>(Class);

	FGridStatusEffectControlProfile Control;
	Control.bBlockSpellActions = true;
	UGridStatusEffectDefinitionAsset* Definition = MON165MakeControlDefinition(GetTransientPackage(), TEXT("MON165_BlockMagic"), Control);
	FString Error;
	TestTrue(TEXT("Silence-style status applies"), Character->StatusEffects.TryAdd(*Definition, FGuid::NewGuid(), Error));

	TArray<FGridAvailableCombatAction> Actions;
	Fixture.TurnManager->GetAvailableCombatActions(0, Actions);
	const FGridAvailableCombatAction* Spell = MON165FindAction(Actions, TEXT("MON165_Spell"));
	const FGridAvailableCombatAction* Ability = MON165FindAction(Actions, TEXT("MON165_Ability"));

	TestNotNull(TEXT("Spell remains visible in the common catalogue"), Spell);
	TestNotNull(TEXT("Ability remains visible in the common catalogue"), Ability);
	TestFalse(TEXT("Spell is disabled by BlockSpellActions"), Spell && Spell->bEnabled);
	TestTrue(TEXT("Non-spell ability remains enabled"), Ability && Ability->bEnabled);
	TestEqual(TEXT("Silence uses the canonical missing-requirement availability"),
		Spell ? Spell->AvailabilityReason : EGridCombatActionAvailabilityReason::None, EGridCombatActionAvailabilityReason::MissingRequirement);
	TestTrue(TEXT("Disabled spell carries an explanatory reason"), Spell && !Spell->DisabledReason.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON165SilenceRequestAtomicTest, "Grimrock.RPG.MON16.5.SilenceRequestAtomic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON165SilenceRequestAtomicTest::RunTest(const FString& Parameters)
{
	FGridMON165PartyFixture Fixture;
	FGridCharacterInventoryState* Character = Fixture.Character();
	if (!Fixture.TurnManager || !Character)
	{
		return false;
	}

	URPGClassAsset* Class = NewObject<URPGClassAsset>(GetTransientPackage());
	Class->ClassId = TEXT("MON165_RequestClass");
	Class->CombatActions = { MON165MakeSelfClassAction(TEXT("MON165_RequestSpell"), EGridCombatActionSourcePolicy::Spell, 2) };
	Character->ClassId = Class->ClassId;
	Character->ClassDefinition = TSoftObjectPtr<URPGClassAsset>(Class);

	FGridStatusEffectControlProfile Control;
	Control.bBlockSpellActions = true;
	UGridStatusEffectDefinitionAsset* Definition = MON165MakeControlDefinition(GetTransientPackage(), TEXT("MON165_RequestBlock"), Control);
	FString Error;
	TestTrue(TEXT("Request-blocking status applies"), Character->StatusEffects.TryAdd(*Definition, FGuid::NewGuid(), Error));

	TArray<FGridAvailableCombatAction> Actions;
	Fixture.TurnManager->GetAvailableCombatActions(0, Actions);
	const FGridAvailableCombatAction* Spell = MON165FindAction(Actions, TEXT("MON165_RequestSpell"));
	TestNotNull(TEXT("Request fixture exposes the spell"), Spell);
	TestFalse(TEXT("Request fixture reaches Silence rather than PartyBusy"), Spell && Spell->bEnabled);
	TestEqual(TEXT("Request fixture is disabled specifically by the Silence gate"),
		Spell ? Spell->AvailabilityReason : EGridCombatActionAvailabilityReason::None, EGridCombatActionAvailabilityReason::MissingRequirement);

	FGridPlayerCharacterTurnState Before;
	Fixture.TurnManager->GetPlayerCharacterTurnState(0, Before);
	const int32 HealthBefore = Character->Resources.CurrentHealth;
	const int32 ManaBefore = Character->Resources.CurrentMana;

	FGridCombatActionRequestResult Result;
	const bool bAccepted = Fixture.TurnManager->RequestCharacterCombatAction(
		0, TEXT("MON165_RequestSpell"), EGridCombatActionSourcePolicy::Spell, Class->ClassId, EGridEquipmentSlot::None, Result);

	FGridPlayerCharacterTurnState After;
	Fixture.TurnManager->GetPlayerCharacterTurnState(0, After);
	TestFalse(TEXT("Silenced spell request is rejected"), bAccepted);
	TestEqual(TEXT("Canonical reject reason is ActionUnavailable"), Result.RejectReason, EGridCombatActionRequestRejectReason::ActionUnavailable);
	TestEqual(TEXT("Rejected spell spends no AP"), After.RemainingActionPoints, Before.RemainingActionPoints);
	TestEqual(TEXT("Rejected spell spends no mana"), Character->Resources.CurrentMana, ManaBefore);
	TestEqual(TEXT("Rejected spell applies no effect"), Character->Resources.CurrentHealth, HealthBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON165PartyImmobilizeTranslationTest, "Grimrock.RPG.MON16.5.PartyImmobilizeTranslation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON165PartyImmobilizeTranslationTest::RunTest(const FString& Parameters)
{
	FGridMON165PartyFixture Fixture;
	FGridCharacterInventoryState* Character = Fixture.Character();
	if (!Fixture.TurnManager || !Character)
	{
		return false;
	}

	FGridStatusEffectControlProfile Control;
	Control.bBlockTranslation = true;
	UGridStatusEffectDefinitionAsset* Definition = MON165MakeControlDefinition(GetTransientPackage(), TEXT("MON165_Root"), Control);
	FString Error;
	TestTrue(TEXT("Immobilize-style status applies"), Character->StatusEffects.TryAdd(*Definition, FGuid::NewGuid(), Error));

	FGridPlayerCharacterTurnState Before;
	Fixture.TurnManager->GetPlayerCharacterTurnState(0, Before);
	const FGridPartyMobilityState MobilityBefore = Fixture.TurnManager->GetPartyMobilityState();
	FIntPoint TargetCell;
	EGridPartyMovementRejectReason RejectReason = EGridPartyMovementRejectReason::None;
	const bool bAccepted = Fixture.TurnManager->RequestPartyTranslation(EGridEdge::North, TargetCell, RejectReason);

	FGridPlayerCharacterTurnState After;
	Fixture.TurnManager->GetPlayerCharacterTurnState(0, After);
	const FGridPartyMobilityState MobilityAfter = Fixture.TurnManager->GetPartyMobilityState();
	TestFalse(TEXT("Immobilized party translation is rejected"), bAccepted);
	TestEqual(TEXT("MON16.5 keeps the temporary public reject reason"), RejectReason, EGridPartyMovementRejectReason::PartyBusy);
	TestEqual(TEXT("Rejected translation spends no personal AP"), After.RemainingActionPoints, Before.RemainingActionPoints);
	TestEqual(TEXT("Rejected translation spends no shared PAM"), MobilityAfter.RemainingMobilityActionPoints, MobilityBefore.RemainingMobilityActionPoints);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON165PartyImmobilizeRotationTest, "Grimrock.RPG.MON16.5.PartyImmobilizeRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON165PartyImmobilizeRotationTest::RunTest(const FString& Parameters)
{
	FGridMON165PartyFixture Fixture;
	FGridCharacterInventoryState* Character = Fixture.Character();
	if (!Fixture.TurnManager || !Character || !Fixture.Party)
	{
		return false;
	}

	FGridStatusEffectControlProfile Control;
	Control.bBlockTranslation = true;
	UGridStatusEffectDefinitionAsset* Definition = MON165MakeControlDefinition(GetTransientPackage(), TEXT("MON165_RootRotate"), Control);
	FString Error;
	TestTrue(TEXT("Immobilize-style status applies"), Character->StatusEffects.TryAdd(*Definition, FGuid::NewGuid(), Error));

	EGridPartyMovementRejectReason RejectReason = EGridPartyMovementRejectReason::None;
	const bool bAccepted = Fixture.TurnManager->RequestPartyRotation(EGridEdge::West, RejectReason);
	TestTrue(TEXT("Immobilize does not block a legal 90-degree rotation"), bAccepted);
	TestEqual(TEXT("Accepted rotation has no reject reason"), RejectReason, EGridPartyMovementRejectReason::None);
	TestTrue(TEXT("Rotation is tracked as an accepted pending motion"), Fixture.TurnManager->IsPartyMotionInProgress());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON165TargetParityTest, "Grimrock.RPG.MON16.5.TargetParity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON165TargetParityTest::RunTest(const FString& Parameters)
{
	FGridMON165TestWorld TestWorld;
	AGridMonsterActor* Monster = TestWorld.World ? TestWorld.World->SpawnActor<AGridMonsterActor>() : nullptr;
	if (!Monster)
	{
		return false;
	}

	FGridStatusEffectControlProfile Control;
	Control.bSkipActivation = true;
	Control.bBlockTranslation = true;
	UGridStatusEffectDefinitionAsset* Definition = MON165MakeControlDefinition(GetTransientPackage(), TEXT("MON165_Parity"), Control);

	FGridStatusEffectCollection PartyCollection;
	FString Error;
	TestTrue(TEXT("Party-style collection accepts control"), PartyCollection.TryAdd(*Definition, FGuid::NewGuid(), Error));
	TestTrue(TEXT("Monster collection accepts the same control definition"), Monster->StatusEffects.TryAdd(*Definition, FGuid::NewGuid(), Error));

	const FGridStatusEffectControlProfile PartyControl = FGridStatusEffectControlResolver::Resolve(PartyCollection);
	const FGridStatusEffectControlProfile MonsterControl = FGridStatusEffectControlResolver::Resolve(Monster->StatusEffects);
	TestEqual(TEXT("SkipActivation has party/monster parity"), MonsterControl.bSkipActivation, PartyControl.bSkipActivation);
	TestEqual(TEXT("BlockTranslation has party/monster parity"), MonsterControl.bBlockTranslation, PartyControl.bBlockTranslation);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON165NoParallelSystemTest, "Grimrock.RPG.MON16.5.NoParallelSystem", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON165NoParallelSystemTest::RunTest(const FString& Parameters)
{
	struct FSourceExpectation
	{
		const TCHAR* RelativePath;
		const TCHAR* RequiredToken;
	};
	const FSourceExpectation Sources[] = { { TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectControlResolver.cpp"),
											   TEXT("DefinitionAsset->Control") },
		{ TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerInitiative.cpp"), TEXT("bSkipActivation") },
		{ TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerPartyMovement.cpp"), TEXT("bBlockTranslation") },
		{ TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerActions.cpp"), TEXT("bBlockTranslation") },
		{ TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerPlayerActionCatalog.cpp"), TEXT("bBlockSpellActions") } };

	for (const FSourceExpectation& Source : Sources)
	{
		FString Text;
		const FString Path = FPaths::Combine(FPaths::ProjectDir(), Source.RelativePath);
		TestTrue(*FString::Printf(TEXT("Production source loads: %s"), Source.RelativePath), FFileHelper::LoadFileToString(Text, *Path));
		TestTrue(*FString::Printf(TEXT("Production source uses generic capability: %s"), Source.RequiredToken), Text.Contains(Source.RequiredToken));
		TestFalse(*FString::Printf(TEXT("No EffectId equality branch in %s"), Source.RelativePath),
			Text.Contains(TEXT("EffectId ==")) || Text.Contains(TEXT("EffectId !=")));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

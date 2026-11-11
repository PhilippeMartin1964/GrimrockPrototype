#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/InputComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Core/GridDirectionUtils.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "RPG/RPGClassAsset.h"
#include "Runtime/Combat/GridPlayerAttackPresentationComponent.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridDualWieldPolicy.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Sound/SoundWave.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridEventFeedSubsystem.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridThrownItemActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"
#include "UI/GridCombatHudWidget.h"
#include "UI/GridEquippedWeaponActionIconPalette.h"
#include "UI/GridCombatHotbarDragDropOperation.h"
#include "UI/GridInventoryDragDropOperation.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	struct FGridCombatHudTestWorld
	{
		UWorld* World = nullptr;

		FGridCombatHudTestWorld()
		{
			const UWorld::InitializationValues InitializationValues = UWorld::InitializationValues()
																		  .AllowAudioPlayback(false)
																		  .RequiresHitProxies(false)
																		  .CreatePhysicsScene(false)
																		  .CreateNavigation(false)
																		  .CreateAISystem(false)
																		  .ShouldSimulatePhysics(false)
																		  .SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("MON12CombatHud_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridCombatHudTestWorld()
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

	FGridCharacterInventoryState MakeHudCharacter(const FGuid& CharacterId, const TCHAR* Name)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = CharacterId;
		Character.DisplayName = FText::FromString(Name);
		Character.Resources.CurrentHealth = 20;
		Character.DerivedStats.MaxHealth = 20;
		Character.Resources.CurrentMana = 8;
		Character.DerivedStats.MaxMana = 8;
		Character.InventorySlots.SetNum(4);
		return Character;
	}

	FGridCombatantInitiativeEntry MakeInitiativeEntry(
		const FGuid& Id, int32 CharacterIndex, EGridCombatantSide Side, EGridCombatantTurnState State, const TCHAR* Name)
	{
		FGridCombatantInitiativeEntry Entry;
		Entry.CombatantId = Id;
		Entry.CharacterIndex = CharacterIndex;
		Entry.Side = Side;
		Entry.State = State;
		Entry.DisplayName = FText::FromString(Name);
		Entry.CurrentHealth = 20;
		Entry.MaximumHealth = 20;
		Entry.InitiativeTotal = 30 - FMath::Max(0, CharacterIndex);
		return Entry;
	}

	FGridCombatActionDefinition MakeMON1285DirectSpell()
	{
		FGridCombatActionDefinition Spell;
		Spell.ActionId = TEXT("Spell_MON1285_ArcaneBolt");
		Spell.DisplayName = FText::FromString(TEXT("Trait arcanique"));
		Spell.Description = FText::FromString(TEXT("Un projectile magique contre la première cible axiale."));
		Spell.ActionType = EGridCombatActionType::RangedAttack;
		Spell.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
		Spell.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
		Spell.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
		Spell.ActionPointCost = 2;
		Spell.ResourceCosts.ManaCost = 3;
		Spell.RangeCells = 2;
		Spell.OffensiveProfile.AttackId = TEXT("Attack_MON1285_ArcaneBolt");
		Spell.OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Arcane;
		Spell.OffensiveProfile.AttackDefinition.MinDamage = 3;
		Spell.OffensiveProfile.AttackDefinition.MaxDamage = 3;
		Spell.OffensiveProfile.AttackDefinition.AccuracyBonus = 100;
		Spell.OffensiveProfile.DamageScalingAttribute = EGridAttackScalingAttribute::Intelligence;
		Spell.OffensiveProfile.RangeCells = 2;
		return Spell;
	}

	FGridCombatActionDefinition MakeMON1285SelfAbility()
	{
		FGridCombatActionDefinition Ability;
		Ability.ActionId = TEXT("Ability_MON1285_Recovery");
		Ability.DisplayName = FText::FromString(TEXT("Récupération"));
		Ability.Description = FText::FromString(TEXT("Restaure immédiatement une partie de la vitalité."));
		Ability.ActionType = EGridCombatActionType::Ability;
		Ability.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
		Ability.TargetingPolicy = EGridCombatTargetingPolicy::Self;
		Ability.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Ability.ActionPointCost = 1;
		Ability.ResourceCosts.ManaCost = 2;
		Ability.EffectProfile.RestoreHealth = 6;
		return Ability;
	}

	FGridCombatActionDefinition MakeMON1286CellSpell()
	{
		FGridCombatActionDefinition Spell;
		Spell.ActionId = TEXT("Spell_MON1286_CellStrike");
		Spell.DisplayName = FText::FromString(TEXT("Frappe ciblée"));
		Spell.Description = FText::FromString(TEXT("Frappe l'ennemi qui occupe la cellule sélectionnée."));
		Spell.ActionType = EGridCombatActionType::RangedAttack;
		Spell.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
		Spell.TargetingPolicy = EGridCombatTargetingPolicy::Cell;
		Spell.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
		Spell.ActionPointCost = 2;
		Spell.ResourceCosts.ManaCost = 3;
		Spell.RangeCells = 2;
		Spell.OffensiveProfile.AttackId = TEXT("Attack_MON1286_CellStrike");
		Spell.OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Arcane;
		Spell.OffensiveProfile.AttackDefinition.MinDamage = 3;
		Spell.OffensiveProfile.AttackDefinition.MaxDamage = 3;
		Spell.OffensiveProfile.AttackDefinition.AccuracyBonus = 100;
		Spell.OffensiveProfile.DamageScalingAttribute = EGridAttackScalingAttribute::Intelligence;
		Spell.OffensiveProfile.RangeCells = 2;
		return Spell;
	}

	FGridCombatActionDefinition MakeMON1286AreaSpell()
	{
		FGridCombatActionDefinition Spell = MakeMON1286CellSpell();
		Spell.ActionId = TEXT("Spell_MON1286_AreaBurst");
		Spell.DisplayName = FText::FromString(TEXT("Explosion arcanique"));
		Spell.TargetingPolicy = EGridCombatTargetingPolicy::Area;
		Spell.AreaRadiusCells = 1;
		Spell.OffensiveProfile.AttackId = TEXT("Attack_MON1286_AreaBurst");
		return Spell;
	}

	bool FindCatalogAction(const UGridTurnManagerComponent* TurnManager, int32 CharacterIndex, FName ActionId, FGridAvailableCombatAction& OutAction)
	{
		OutAction = FGridAvailableCombatAction();
		if (!IsValid(TurnManager))
		{
			return false;
		}
		TArray<FGridAvailableCombatAction> AvailableActions;
		TurnManager->GetAvailableCombatActions(CharacterIndex, AvailableActions);
		const FGridAvailableCombatAction* Match = AvailableActions.FindByPredicate(
			[ActionId](const FGridAvailableCombatAction& Candidate)
			{
				return Candidate.Definition.ActionId == ActionId;
			});
		if (!Match)
		{
			return false;
		}
		OutAction = *Match;
		return true;
	}

	struct FGridCombatHudFixture
	{
		FGridCombatHudTestWorld TestWorld;
		AGridLevelRuntimeActor* Runtime = nullptr;
		AGrimrockPartyPawn* Party = nullptr;
		AGridMonsterActor* Monster = nullptr;
		UGridTurnManagerComponent* TurnManager = nullptr;
		UGridCombatHudWidget* Hud = nullptr;
		FGuid CharacterIds[4] = { FGuid(12, 7, 1, 1), FGuid(12, 7, 1, 2), FGuid(12, 7, 1, 3), FGuid(12, 7, 1, 4) };

		FGridCombatHudFixture()
		{
			if (!TestWorld.World)
			{
				return;
			}

			Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
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
			Runtime->CurrentDungeonLevelId = TEXT("MON12_7_Test");

			Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
			Party->LevelRuntimeActor = Runtime;
			Party->CurrentCellX = 1;
			Party->CurrentCellY = 1;
			Party->Facing = EGridEdge::North;
			Party->SetActorLocation(Runtime->GetCellCenterWorld(1, 1, Party->EyeHeight));
			Party->SetActorRotation(FRotator(0.0f, GridDirectionUtils::ToYaw(Party->Facing), 0.0f));

			Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { MakeHudCharacter(CharacterIds[0], TEXT("Elias")),
				MakeHudCharacter(CharacterIds[1], TEXT("Mina")), MakeHudCharacter(CharacterIds[2], TEXT("Orin")),
				MakeHudCharacter(CharacterIds[3], TEXT("Sana")) };
			Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment.SetNum(4);
			Party->PartyInventoryComponent->InitializeDefaultPartyIfNeeded();

			FGridItemInstance Weapon;
			Weapon.RuntimeObjectId = FGuid(12, 7, 2, 1);
			Weapon.ItemDefinitionId = TEXT("MON12_7_Sword");
			Weapon.DisplayName = FText::FromString(TEXT("Épée de test"));
			Weapon.Quantity = 1;
			Weapon.OwnerType = EGridItemOwnerType::EquipmentSlot;
			Weapon.OwnerCharacterIndex = 0;
			Weapon.EquipmentSlot = EGridEquipmentSlot::MainHand;
			Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment[0].MainHand = Weapon;

			UGridItemDefinitionAsset* WeaponDefinition = NewObject<UGridItemDefinitionAsset>(Party);
			WeaponDefinition->ItemDefinitionId = Weapon.ItemDefinitionId;
			WeaponDefinition->DisplayName = Weapon.DisplayName;
			WeaponDefinition->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::MainHand);
			FGridCombatActionDefinition WeaponAction;
			WeaponAction.ActionId = TEXT("Attack_MON12_7_Sword");
			WeaponAction.DisplayName = WeaponDefinition->DisplayName;
			WeaponAction.ActionType = EGridCombatActionType::MeleeAttack;
			WeaponAction.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
			WeaponAction.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
			WeaponAction.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
			WeaponAction.ActionPointCost = 2;
			WeaponAction.RangeCells = 1;
			WeaponAction.PresentationProfileId = WeaponAction.ActionId;
			WeaponAction.OffensiveProfile.AttackId = WeaponAction.ActionId;
			WeaponAction.OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Physical;
			WeaponAction.OffensiveProfile.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
			WeaponAction.OffensiveProfile.AttackDefinition.MinDamage = 1;
			WeaponAction.OffensiveProfile.AttackDefinition.MaxDamage = 2;
			WeaponAction.OffensiveProfile.RangeCells = 1;
			WeaponDefinition->CombatActions.Add(WeaponAction);
			Party->PartyInventoryComponent->RegisterItemDefinition(WeaponDefinition);

			UGridMonsterDefinitionAsset* MonsterDefinition = NewObject<UGridMonsterDefinitionAsset>(Runtime);
			MonsterDefinition->MonsterId = TEXT("MON12_7_Rat");
			MonsterDefinition->DisplayName = FText::FromString(TEXT("Rat MON12.7"));
			MonsterDefinition->CategoryId = TEXT("Vermin");
			MonsterDefinition->MaxHealth = 1000;
			MonsterDefinition->Evasion = 0;
			MonsterDefinition->ActionPointsPerTurn = 2;
			Monster = TestWorld.World->SpawnActor<AGridMonsterActor>();
			UGridMonsterOccupancySubsystem* Occupancy = TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
			if (Monster && Occupancy)
			{
				Monster->InitializeMonster(MonsterDefinition, FGuid(12, 7, 3, 1), FIntPoint(1, 2), EGridEdge::South);
				Occupancy->RegisterMonster(Monster, FIntPoint(1, 2));

				UGridMonsterMovementComponent* Movement = NewObject<UGridMonsterMovementComponent>(Monster, TEXT("MON12_7_Movement"));
				Movement->bAutoInitialize = false;
				Movement->bInferCellFromActorLocation = false;
				Monster->AddInstanceComponent(Movement);
				Movement->RegisterComponent();

				UGridMonsterBehaviorComponent* Behavior = NewObject<UGridMonsterBehaviorComponent>(Monster, TEXT("MON12_7_Behavior"));
				Behavior->bAutoInitialize = false;
				Monster->AddInstanceComponent(Behavior);
				Behavior->RegisterComponent();
			}

			TurnManager = NewObject<UGridTurnManagerComponent>(Runtime, TEXT("MON12_7_TurnManager"));
			TurnManager->bAutoInitialize = false;
			Runtime->AddInstanceComponent(TurnManager);
			TurnManager->RegisterComponent();
			TurnManager->InitializeTurnManager(Runtime, Party);
			TurnManager->bCombatActive = true;
			TurnManager->CurrentPhase = EGridCombatPhase::PlayerPhase;
			TurnManager->RoundNumber = 1;
			TurnManager->CombatMonsters = { Monster };
			TurnManager->PartyMobilityState.RoundNumber = 1;
			TurnManager->PartyMobilityState.MaximumMobilityActionPoints = 2;
			TurnManager->PartyMobilityState.RemainingMobilityActionPoints = 2;

			for (int32 Index = 0; Index < 4; ++Index)
			{
				FGridPlayerCharacterTurnState State;
				State.CharacterIndex = Index;
				State.CharacterId = CharacterIds[Index];
				State.State = Index == 0 ? EGridCombatantTurnState::Active : EGridCombatantTurnState::Waiting;
				State.MaximumActionPoints = 4;
				State.RemainingActionPoints = Index == 0 ? 4 : 0;
				TurnManager->PlayerCharacterTurnStates.Add(State);
				TurnManager->InitiativeOrder.Add(MakeInitiativeEntry(CharacterIds[Index], Index, EGridCombatantSide::Party, State.State,
					*Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[Index].DisplayName.ToString()));
			}
			TurnManager->InitiativeOrder.Add(MakeInitiativeEntry(
				Monster->ResolvePersistenceId(), INDEX_NONE, EGridCombatantSide::Monster, EGridCombatantTurnState::Waiting, TEXT("Rat MON12.7")));
			TurnManager->CurrentInitiativeIndex = 0;

			Hud = CreateWidget<UGridCombatHudWidget>(TestWorld.World, UGridCombatHudWidget::StaticClass());
			if (Hud)
			{
				Hud->InitializeCombatHud(Party, TurnManager);
			}
		}

		bool IsReady() const
		{
			return TestWorld.World && Runtime && Party && Monster && TurnManager && Hud;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON12CombatHudViewModelTest, "Grimrock.Monsters.MON12.CombatHUD.ViewModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12CombatHudViewModelTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TArray<FGridPlayerCharacterTurnState> TurnStates;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		FGridPlayerCharacterTurnState& State = TurnStates.AddDefaulted_GetRef();
		State.CharacterIndex = Index;
		State.State = Index == 0 ? EGridCombatantTurnState::Active : EGridCombatantTurnState::Waiting;
		State.MaximumActionPoints = 4;
		State.RemainingActionPoints = 4;
	}
	TArray<FGridCombatHudPartyMemberView> Members;
	FGridCombatHudViewModelBuilder::BuildPartyMembers(4, TurnStates, Members);
	TestEqual(TEXT("Exactly four party panels are projected"), Members.Num(), 4);
	TestTrue(TEXT("The active member is identified"), Members[0].bActive);

	TArray<FGridAvailableCombatAction> Catalog;
	TArray<FGridCombatHotbarBinding> Bindings;
	Bindings.SetNum(FGridCombatHotbarBinding::MinimumSlotCount + 1);
	for (int32 SlotIndex = 0; SlotIndex < Bindings.Num(); ++SlotIndex)
	{
		Bindings[SlotIndex].Reset(SlotIndex);
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FGridAvailableCombatAction& Action = Catalog.AddDefaulted_GetRef();
		Action.Definition.ActionId = *FString::Printf(TEXT("Action_%d"), Index);
		Action.Definition.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
		Action.CurrentActionPointCost = Index + 1;
		Action.bEnabled = Index != 2;
		Action.DisabledReason = Action.bEnabled ? FText::GetEmpty() : FText::FromString(TEXT("PA insuffisants"));
		Bindings[Index].ActionId = Action.Definition.ActionId;
		Bindings[Index].SourcePolicy = EGridCombatActionSourcePolicy::Universal;
	}
	TArray<FGridCombatHudActionView> Actions;
	FGridCombatHudViewModelBuilder::BuildHotbarActions(Bindings, Catalog, Actions);
	TestEqual(TEXT("The view preserves the supplied dynamic action-bar width"), Actions.Num(), FGridCombatHotbarBinding::MinimumSlotCount + 1);
	TestEqual(TEXT("The first slot displays key 1"), Actions[0].ShortcutText.ToString(), FString(TEXT("1")));
	TestEqual(TEXT("Slot 10 displays key 0"), Actions[9].ShortcutText.ToString(), FString(TEXT("0")));
	TestEqual(TEXT("Slot 11 displays apostrophe"), Actions[10].ShortcutText.ToString(), FString(TEXT("'")));
	TestEqual(TEXT("Slot 12 displays caret"), Actions[11].ShortcutText.ToString(), FString(TEXT("^")));
	TestTrue(TEXT("Mouse-only slots have no keyboard label"), Actions[12].ShortcutText.IsEmpty());
	TestTrue(TEXT("An assigned action is resolved from the catalog"), Actions[0].bResolved);
	TestFalse(TEXT("An unassigned slot stays empty"), Actions[9].bHasBinding);
	TestEqual(TEXT("A disabled action keeps its reason"), Actions[2].DisabledReason.ToString(), FString(TEXT("PA insuffisants")));

	FGridPartyMobilityState MobilityState;
	MobilityState.MaximumMobilityActionPoints = 2;
	MobilityState.RemainingMobilityActionPoints = 1;
	const FGridCombatHudMobilityView Mobility = FGridCombatHudViewModelBuilder::BuildMobility(MobilityState);
	TestEqual(TEXT("The shared PAM snapshot is copied"), Mobility.RemainingMobilityActionPoints, 1);

	TArray<FGridInitiativePreviewEntry> Upcoming;
	for (int32 Index = 0; Index < 10; ++Index)
	{
		FGridInitiativePreviewEntry& Preview = Upcoming.AddDefaulted_GetRef();
		Preview.Combatant = MakeInitiativeEntry(FGuid(12, 7, 10, Index + 1), Index, Index % 2 == 0 ? EGridCombatantSide::Party : EGridCombatantSide::Monster,
			Index == 0 ? EGridCombatantTurnState::Active : EGridCombatantTurnState::Waiting, TEXT("Participant"));
		Preview.RoundNumber = Index < 5 ? 1 : 2;
		Preview.ActivationIndex = Index % 5;
		Preview.bIsActive = Index == 0;
		Preview.bStartsNewRound = Index == 5;
		Preview.Combatant.CurrentHealth = Index == 0 ? 7 : 20;
		Preview.Combatant.MaximumHealth = 20;
	}
	TArray<FGridCombatHudInitiativeView> Initiative;
	FGridCombatHudViewModelBuilder::BuildInitiative(Upcoming, Initiative);
	TestEqual(TEXT("At most eight initiative entries are visible"), Initiative.Num(), 8);
	TestTrue(TEXT("The first active combatant is emphasized"), Initiative[0].bActive);
	TestEqual(TEXT("The runtime initiative order is preserved"), Initiative[3].Combatant.CombatantId, Upcoming[3].Combatant.CombatantId);
	TestTrue(TEXT("The projected round boundary is preserved"), Initiative[5].bStartsNewRound);
	TestEqual(TEXT("The projected round number is preserved"), Initiative[5].RoundNumber, 2);
	TestEqual(TEXT("Initiative health percent reflects current health"), Initiative[0].HealthPercent, 0.35f);
	TestEqual(TEXT("Initiative health percent clamps overhealing"), FGridCombatHudViewModelBuilder::CalculateHealthPercent(25, 20), 1.0f);
	TestEqual(TEXT("Initiative health percent rejects invalid maximum"), FGridCombatHudViewModelBuilder::CalculateHealthPercent(7, 0), 0.0f);
	TestEqual(TEXT("Initiative health percent clamps negative health"), FGridCombatHudViewModelBuilder::CalculateHealthPercent(-2, 20), 0.0f);

	UGridCombatHudInitiativeSlotWidget* InitiativeSlot = NewObject<UGridCombatHudInitiativeSlotWidget>();
	InitiativeSlot->ProgressBar_Health = NewObject<UProgressBar>(InitiativeSlot);
	InitiativeSlot->Text_State = NewObject<UTextBlock>(InitiativeSlot);
	InitiativeSlot->InitializeInitiativeSlot(Initiative[0]);
	TestEqual(TEXT("The initiative health bar receives the view percent"), InitiativeSlot->ProgressBar_Health->GetPercent(), 0.35f);
	TestTrue(TEXT("The default health bar fill is visibly red"),
		InitiativeSlot->HealthBarFillColor.R > InitiativeSlot->HealthBarFillColor.G &&
			InitiativeSlot->HealthBarFillColor.R > InitiativeSlot->HealthBarFillColor.B);
	TestEqual(TEXT("The active initiative scale stays compact"), InitiativeSlot->ActiveScale, 1.12f);
	TestEqual(TEXT("Only the active state remains visible"), InitiativeSlot->Text_State->GetText().ToString(), FString(TEXT("ACTIF")));
	InitiativeSlot->InitializeInitiativeSlot(Initiative[1]);
	TestEqual(TEXT("Waiting state labels are hidden"), InitiativeSlot->Text_State->GetVisibility(), ESlateVisibility::Collapsed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1271InitiativePreviewTest, "Grimrock.Monsters.MON12.CombatHUD.SlidingInitiative",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1271InitiativePreviewTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.7.1 preview fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	TArray<FGridInitiativePreviewEntry> Preview;
	Fixture.TurnManager->GetInitiativePreview(Preview, 8);
	TestEqual(TEXT("The sliding timeline always predicts eight activations"), Preview.Num(), 8);
	TestEqual(TEXT("The active combatant remains the first activation"), Preview[0].Combatant.CombatantId, Fixture.CharacterIds[0]);
	TestTrue(TEXT("Only the first activation is active"), Preview[0].bIsActive);
	TestEqual(TEXT("Five current-round activations are retained"), Preview[4].RoundNumber, 1);
	TestTrue(TEXT("A separator precedes the first round-two activation"), Preview[5].bStartsNewRound);
	TestEqual(TEXT("The separator announces round two"), Preview[5].RoundNumber, 2);

	Fixture.Monster->CurrentHealth = 375;
	Fixture.TurnManager->GetInitiativePreview(Preview, 8);
	for (const FGridInitiativePreviewEntry& Entry : Preview)
	{
		if (Entry.Combatant.CombatantId == Fixture.Monster->ResolvePersistenceId())
		{
			TestEqual(TEXT("Every rat activation reads current runtime health"), Entry.Combatant.CurrentHealth, 375);
			TestEqual(TEXT("Every rat activation reads maximum definition health"), Entry.Combatant.MaximumHealth, 1000);
		}
	}

	Fixture.TurnManager->InitiativeOrder.Last().State = EGridCombatantTurnState::Defeated;
	Fixture.TurnManager->InitiativeOrder.Last().CurrentHealth = 0;
	Fixture.TurnManager->GetInitiativePreview(Preview, 8);
	TestEqual(TEXT("A defeated combatant does not reduce slot coverage"), Preview.Num(), 8);
	TestTrue(TEXT("Round two now starts after four living activations"), Preview[4].bStartsNewRound);
	TestEqual(TEXT("The updated separator still announces round two"), Preview[4].RoundNumber, 2);
	for (const FGridInitiativePreviewEntry& Entry : Preview)
	{
		TestNotEqual(TEXT("The defeated monster is absent from every projected round"), Entry.Combatant.CombatantId, Fixture.Monster->ResolvePersistenceId());
	}

	Fixture.TurnManager->GetInitiativePreview(Preview, 7);
	TestEqual(TEXT("Seven configured slots are supported"), Preview.Num(), 7);
	Fixture.TurnManager->GetInitiativePreview(Preview, 10);
	TestEqual(TEXT("Ten configured slots are supported"), Preview.Num(), 10);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1271DynamicInitiativeTest, "Grimrock.Monsters.MON12.CombatHUD.DynamicInitiative",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1271DynamicInitiativeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.7.1 dynamic fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	Fixture.TurnManager->InitiativeOrder[0].InitiativeTotal = 40;
	Fixture.TurnManager->InitiativeOrder[1].InitiativeTotal = 30;
	Fixture.TurnManager->InitiativeOrder[2].InitiativeTotal = 20;
	Fixture.TurnManager->InitiativeOrder[3].InitiativeTotal = 10;
	Fixture.TurnManager->InitiativeOrder[4].InitiativeTotal = 15;

	const FGuid ActiveId = Fixture.TurnManager->InitiativeOrder[0].CombatantId;
	const FGuid HastedId = Fixture.TurnManager->InitiativeOrder[3].CombatantId;
	TestTrue(TEXT("A haste modifier is accepted for a known combatant"),
		Fixture.TurnManager->SetCombatantInitiativeModifier(EGridCombatantSide::Party, HastedId, 100));
	TestEqual(TEXT("The already active combatant never moves retroactively"), Fixture.TurnManager->InitiativeOrder[0].CombatantId, ActiveId);
	TestEqual(TEXT("The hasted combatant becomes the next future activation"), Fixture.TurnManager->InitiativeOrder[1].CombatantId, HastedId);

	TArray<FGridInitiativePreviewEntry> Preview;
	Fixture.TurnManager->GetInitiativePreview(Preview, 8);
	TestEqual(TEXT("The active activation stays first in the prediction"), Preview[0].Combatant.CombatantId, ActiveId);
	TestEqual(TEXT("The current-round future order updates immediately"), Preview[1].Combatant.CombatantId, HastedId);
	TestEqual(TEXT("The next round uses the complete modified order"), Preview[5].Combatant.CombatantId, HastedId);

	Fixture.TurnManager->ResetInitiativeRound();
	TestEqual(TEXT("A new round starts with the modified initiative leader"), Fixture.TurnManager->InitiativeOrder[0].CombatantId, HastedId);
	TestFalse(TEXT("An unknown combatant cannot change initiative"),
		Fixture.TurnManager->SetCombatantInitiativeModifier(EGridCombatantSide::Party, FGuid(99, 99, 99, 99), 10));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON12CombatHudLifecycleTest, "Grimrock.Monsters.MON12.CombatHUD.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12CombatHudLifecycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.7 fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	TestEqual(TEXT("The live HUD exposes four party panels"), Fixture.Hud->View.PartyMembers.Num(), 4);
	TestEqual(TEXT("The first runtime combatant is active"), Fixture.Hud->View.ActiveCharacterIndex, 0);
	TestEqual(TEXT("The HUD reads the shared PAM authority"), Fixture.Hud->View.Mobility.RemainingMobilityActionPoints, 2);
	TestEqual(TEXT("The HUD exposes the canonical action-bar slot count"), Fixture.Hud->View.Actions.Num(), FGridCombatHotbarBinding::MinimumSlotCount);
	TestTrue(TEXT("Slot 1 is the protected PrimaryAttack binding"), Fixture.Hud->View.Actions[0].Binding.IsPrimaryAttackBinding());
	TestTrue(TEXT("PrimaryAttack resolves automatically"), Fixture.Hud->View.Actions[0].bResolved);
	TestEqual(TEXT("PrimaryAttack follows the equipped sword"), Fixture.Hud->View.Actions[0].Action.SourceDefinitionId, FName(TEXT("MON12_7_Sword")));

	FGridItemInstance EquippedSword;
	TestTrue(TEXT("The fixture exposes the equipped sword"),
		Fixture.Party->PartyInventoryComponent->GetEquippedItem(0, EGridEquipmentSlot::MainHand, EquippedSword));
	UGridInventoryDragDropOperation* SwordDrag = NewObject<UGridInventoryDragDropOperation>(Fixture.Hud);
	SwordDrag->InitializeFromSlot(EGridInventoryUiSlotType::MainHand, 0, EquippedSword);
	TestFalse(TEXT("PrimaryAttack rejects manual item drops"), Fixture.Hud->HandleHotbarDrop(0, SwordDrag));

	const FGridCombatHudActionView* SwordAction = Fixture.Hud->View.Actions.FindByPredicate(
		[](const FGridCombatHudActionView& Candidate)
		{
			return Candidate.bHasBinding && Candidate.Action.SourceDefinitionId == FName(TEXT("MON12_7_Sword"));
		});
	if (!TestNotNull(TEXT("The equipped sword contributes an action"), SwordAction))
	{
		return false;
	}

	FGridCombatActionRequestResult AcceptedResult;
	TestTrue(TEXT("HUD action routes through RequestCharacterCombatAction"), Fixture.Hud->RequestCombatAction(*SwordAction, AcceptedResult));
	TestTrue(TEXT("The TurnManager accepted the generic request"), AcceptedResult.bAccepted);
	TestEqual(TEXT("The accepted action spent the MON12.5/12.6 AP cost"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 2);
	TestEqual(TEXT("The attack cost remains two AP"), Fixture.TurnManager->PlayerAttackActionPointCost, 2);
	TestEqual(TEXT("The translation cost remains one AP"), Fixture.TurnManager->PartyTranslationActionPointCost, 1);
	TestEqual(TEXT("The shared translation cost remains one PAM"), Fixture.TurnManager->PartyTranslationMobilityActionPointCost, 1);

	UGridItemDefinitionAsset* SwordDefinition = Fixture.Party->PartyInventoryComponent->FindItemDefinition(FName(TEXT("MON12_7_Sword")));
	TestNotNull(TEXT("The sword definition remains registered"), SwordDefinition);
	if (SwordDefinition && !SwordDefinition->CombatActions.IsEmpty())
	{
		SwordDefinition->CombatActions[0].ActionPointCost = 3;
	}
	Fixture.Hud->RefreshFromSources();
	const FGridCombatHudActionView* DisabledSwordAction = Fixture.Hud->View.Actions.FindByPredicate(
		[](const FGridCombatHudActionView& Candidate)
		{
			return Candidate.Action.SourceDefinitionId == FName(TEXT("MON12_7_Sword"));
		});
	TestNotNull(TEXT("The unavailable action remains visible"), DisabledSwordAction);
	if (DisabledSwordAction)
	{
		TestFalse(TEXT("The unavailable action is disabled"), DisabledSwordAction->Action.bEnabled);
		TestFalse(TEXT("The unavailable action exposes a reason"), DisabledSwordAction->DisabledReason.IsEmpty());
		FGridCombatActionRequestResult RejectedResult;
		TestFalse(TEXT("The authoritative retry is refused"), Fixture.Hud->RequestHotbarSlot(0, RejectedResult));
		TestEqual(TEXT("A refused action consumes no AP"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 2);
	}

	Fixture.TurnManager->PlayerCharacterTurnStates[0].State = EGridCombatantTurnState::Completed;
	Fixture.TurnManager->PlayerCharacterTurnStates[1].State = EGridCombatantTurnState::Active;
	Fixture.TurnManager->PlayerCharacterTurnStates[1].RemainingActionPoints = 4;
	Fixture.TurnManager->InitiativeOrder[0].State = EGridCombatantTurnState::Completed;
	Fixture.TurnManager->InitiativeOrder[1].State = EGridCombatantTurnState::Active;
	Fixture.TurnManager->CurrentInitiativeIndex = 1;
	Fixture.TurnManager->OnActiveCombatantChanged.Broadcast(Fixture.TurnManager->InitiativeOrder[1]);
	TestEqual(TEXT("The active-combatant event refreshes the HUD"), Fixture.Hud->View.ActiveCharacterIndex, 1);
	TestTrue(TEXT("The second member is marked active"), Fixture.Hud->View.PartyMembers[1].bActive);

	Fixture.TurnManager->PendingPartyMotionType = EGridPendingPartyMotionType::Translation;
	Fixture.Hud->RefreshFromSources();
	TestFalse(TEXT("End turn is disabled during movement"), Fixture.Hud->View.bCanEndTurn);
	TestFalse(TEXT("The TurnManager refuses end turn during movement"), Fixture.Hud->RequestEndTurn());
	TestEqual(TEXT("The refused end-turn keeps the active member AP"), Fixture.Hud->View.PartyMembers[1].RemainingActionPoints, 4);
	Fixture.TurnManager->PendingPartyMotionType = EGridPendingPartyMotionType::None;
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1283HotbarClickExecutionTest, "Grimrock.Monsters.MON12.8.3.HotbarClickExecution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1283HotbarClickExecutionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.8.3 click fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	FGridItemInstance EquippedSword;
	if (!TestTrue(TEXT("The click fixture exposes its sword"),
			Fixture.Party->PartyInventoryComponent->GetEquippedItem(0, EGridEquipmentSlot::MainHand, EquippedSword)))
	{
		return false;
	}
	FGridCombatHotbarBinding PrimaryBinding;
	TestTrue(TEXT("Slot 1 keeps the protected PrimaryAttack alias"),
		Fixture.Party->PartyInventoryComponent->GetCharacterCombatHotbarBinding(0, 0, PrimaryBinding) && PrimaryBinding.IsPrimaryAttackBinding());

	Fixture.Hud->RefreshFromSources();
	UGridCombatHudActionWidget* ActionWidget = NewObject<UGridCombatHudActionWidget>(Fixture.Hud);
	if (!TestNotNull(TEXT("A persistent action widget can bind to the combat execution backend"), ActionWidget))
	{
		return false;
	}
	ActionWidget->InitializeAction(Fixture.Hud, Fixture.Hud->View.Actions[0]);

	TestTrue(TEXT("A short click executes the configured attack"), ActionWidget->TryExecuteAction());
	TestEqual(TEXT("The click pays exactly two action points"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 2);

	FGridCombatActionRequestResult EmptyResult;
	TestFalse(TEXT("An empty shortcut cannot execute"), Fixture.Hud->RequestHotbarSlot(9, EmptyResult));
	TestEqual(TEXT("The empty shortcut consumes no action points"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1283UnarmedHotbarExecutionTest, "Grimrock.Monsters.MON12.8.3.UnarmedHotbarExecution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1283UnarmedHotbarExecutionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.8.3 unarmed fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}
	Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment[0].MainHand = FGridItemInstance();
	FGridCombatHotbarBinding PrimaryBinding;
	TestTrue(TEXT("PrimaryAttack is readable"), Fixture.Party->PartyInventoryComponent->GetCharacterCombatHotbarBinding(0, 0, PrimaryBinding));
	TestTrue(TEXT("Slot 1 is the protected PrimaryAttack alias"), PrimaryBinding.IsPrimaryAttackBinding());
	FGridCombatActionRequestResult Result;
	TestTrue(TEXT("PrimaryAttack falls back to unarmed"), Fixture.Hud->RequestHotbarSlot(0, Result));
	TestTrue(TEXT("The generic result accepts the unarmed attack"), Result.bAccepted);
	TestEqual(TEXT("The unarmed fallback pays two action points"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 2);
	TestEqual(TEXT("The unarmed fallback uses no equipment slot"), Result.Action.SourceEquipmentSlot, EGridEquipmentSlot::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1283HotbarKeyboardGuardTest, "Grimrock.Monsters.MON12.8.3.HotbarKeyboardAndModalGuard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1283HotbarKeyboardGuardTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.8.3 keyboard fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	UInputComponent* InputComponent = NewObject<UInputComponent>(Fixture.Party, TEXT("MON12_8_3_Input"));
	Fixture.Party->SetupPlayerInputComponent(InputComponent);
	const FKey ExpectedKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine,
		EKeys::Zero };
	for (const FKey& ExpectedKey : ExpectedKeys)
	{
		const FInputKeyBinding* Binding = InputComponent->KeyBindings.FindByPredicate(
			[&ExpectedKey](const FInputKeyBinding& Candidate)
			{
				return Candidate.Chord.Key == ExpectedKey && Candidate.KeyEvent == IE_Pressed;
			});
		TestTrue(*FString::Printf(TEXT("Key %s is bound to the combat hotbar"), *ExpectedKey.ToString()),
			Binding && Binding->bConsumeInput && !Binding->bExecuteWhenPaused);
	}

	FGridItemInstance EquippedSword;
	if (!TestTrue(TEXT("The keyboard fixture exposes its sword"),
			Fixture.Party->PartyInventoryComponent->GetEquippedItem(0, EGridEquipmentSlot::MainHand, EquippedSword)))
	{
		return false;
	}
	FGridCombatHotbarBinding PrimaryBinding;
	TestTrue(TEXT("The keyboard fixture keeps PrimaryAttack in slot 1"),
		Fixture.Party->PartyInventoryComponent->GetCharacterCombatHotbarBinding(0, 0, PrimaryBinding) && PrimaryBinding.IsPrimaryAttackBinding());
	Fixture.Party->CombatHudWidgetInstance = Fixture.Hud;

	Fixture.Party->bInventoryWidgetVisible = true;
	TestFalse(TEXT("The inventory intercepts the numeric shortcut"), Fixture.Party->TryExecuteCombatHotbarSlot(0));
	FGridCombatActionRequestResult BlockedClick;
	TestFalse(TEXT("The inventory also intercepts a direct slot click"), Fixture.Hud->RequestHotbarSlot(0, BlockedClick));
	Fixture.Party->bInventoryWidgetVisible = false;
	Fixture.Party->bCharacterCreationModalActive = true;
	TestFalse(TEXT("A modal screen intercepts the numeric shortcut"), Fixture.Party->TryExecuteCombatHotbarSlot(0));
	Fixture.Party->bCharacterCreationModalActive = false;

	FGridPlayerCharacterTurnState TurnState;
	TestTrue(TEXT("The guarded character state remains readable"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, TurnState));
	TestEqual(TEXT("Blocked shortcuts consume no action points"), TurnState.RemainingActionPoints, 4);
	TestTrue(TEXT("The numeric shortcut executes after the modal closes"), Fixture.Party->TryExecuteCombatHotbarSlot(0));
	TestTrue(TEXT("The post-keyboard character state remains readable"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, TurnState));
	TestEqual(TEXT("The accepted numeric shortcut pays two action points"), TurnState.RemainingActionPoints, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1284QuickItemEffectTest, "Grimrock.Monsters.MON12.8.4.QuickItemEffectAndUnassignment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1284QuickItemEffectTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.8.4 effect fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	UGridPartyInventoryComponent* Inventory = Fixture.Party->PartyInventoryComponent;
	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
	Character.Resources.CurrentHealth = 5;
	Character.Resources.CurrentMana = 4;

	UGridItemDefinitionAsset* PotionDefinition = NewObject<UGridItemDefinitionAsset>(Fixture.Party);
	PotionDefinition->ItemDefinitionId = TEXT("Potion_MON1284_Health");
	PotionDefinition->DisplayName = FText::FromString(TEXT("Potion de soins MON12.8.4"));
	PotionDefinition->ItemType = EGridItemType::Potion;
	PotionDefinition->bStackable = true;
	PotionDefinition->MaxStackSize = 10;
	PotionDefinition->bProvidesQuickItemCombatAction = true;
	PotionDefinition->QuickItemCombatAction.ActionType = EGridCombatActionType::Ability;
	PotionDefinition->QuickItemCombatAction.TargetingPolicy = EGridCombatTargetingPolicy::Self;
	PotionDefinition->QuickItemCombatAction.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
	PotionDefinition->QuickItemCombatAction.ActionPointCost = 1;
	PotionDefinition->QuickItemCombatAction.EffectProfile.RestoreHealth = 7;
	PotionDefinition->QuickItemCombatAction.EffectProfile.RestoreMana = 3;
	if (!TestTrue(TEXT("The combat potion definition is registered"), Inventory->RegisterItemDefinition(PotionDefinition)))
	{
		return false;
	}

	FGridItemInstance Potion;
	Potion.RuntimeObjectId = FGuid(12, 8, 4, 1);
	Potion.ItemDefinitionId = PotionDefinition->ItemDefinitionId;
	Potion.DisplayName = PotionDefinition->DisplayName;
	Potion.Quantity = 2;
	TestTrue(TEXT("Two potion units enter the inventory"), Inventory->AddItemToCharacterInventory(0, Potion));
	TestTrue(TEXT("The potion configures shortcut two"), Inventory->SetCharacterCombatHotbarBindingFromItem(0, 1, Potion, EGridEquipmentSlot::None));

	Fixture.Hud->RefreshFromSources();
	TestTrue(TEXT("The configured potion resolves from the catalogue"), Fixture.Hud->View.Actions[1].bResolved);
	TestTrue(TEXT("The potion is initially usable"), Fixture.Hud->View.Actions[1].Action.bEnabled);
	TestEqual(TEXT("The catalogue aggregates both potion units"), Fixture.Hud->View.Actions[1].Action.CurrentSourceItemQuantity, 2);

	FGridCombatActionRequestResult FirstUse;
	TestTrue(TEXT("The first potion use is accepted"), Fixture.Hud->RequestHotbarSlot(1, FirstUse));
	TestEqual(TEXT("The potion restores seven health"), Character.Resources.CurrentHealth, 12);
	TestEqual(TEXT("The same potion restores three mana"), Character.Resources.CurrentMana, 7);
	TestEqual(TEXT("The potion spends one action point"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 3);
	TestEqual(TEXT("Exactly one potion remains"), Inventory->CountItemDefinitionInCharacterInventory(0, PotionDefinition->ItemDefinitionId), 1);
	TestEqual(TEXT("The result records the consumed unit"), FirstUse.QuickItemResult.SourceQuantityAfter, 1);
	FGridCombatHotbarBinding RemainingPotionBinding;
	TestTrue(TEXT("The partially consumed shortcut remains readable"), Inventory->GetCharacterCombatHotbarBinding(0, 1, RemainingPotionBinding));
	TestFalse(TEXT("A partial potion use keeps its shortcut"), RemainingPotionBinding.IsEmpty());
	TestTrue(TEXT("The remaining potion stays in the HUD slot"), Fixture.Hud->View.Actions[1].bHasBinding);
	TestEqual(TEXT("The HUD reports the remaining potion quantity"), Fixture.Hud->View.Actions[1].Action.CurrentSourceItemQuantity, 1);

	Character.Resources.CurrentHealth = 20;
	Character.Resources.CurrentMana = 8;
	Fixture.Hud->RefreshFromSources();
	TestFalse(TEXT("A full-health character cannot waste the potion"), Fixture.Hud->View.Actions[1].Action.bEnabled);
	TestEqual(TEXT("The disabled reason identifies a useless effect"), Fixture.Hud->View.Actions[1].Action.AvailabilityReason,
		EGridCombatActionAvailabilityReason::NoApplicableEffect);
	FGridCombatActionRequestResult RefusedUse;
	TestFalse(TEXT("The useless potion request is rejected"), Fixture.Hud->RequestHotbarSlot(1, RefusedUse));
	TestEqual(TEXT("A refused potion consumes no unit"), Inventory->CountItemDefinitionInCharacterInventory(0, PotionDefinition->ItemDefinitionId), 1);
	TestEqual(TEXT("A refused potion consumes no action point"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 3);
	TestTrue(TEXT("A refused potion keeps its shortcut"), Fixture.Hud->View.Actions[1].bHasBinding);

	Character.Resources.CurrentHealth = 10;
	FGridCombatActionRequestResult LastUse;
	TestTrue(TEXT("The last potion unit can be consumed"), Fixture.Hud->RequestHotbarSlot(1, LastUse));
	TestEqual(TEXT("The inventory quantity reaches zero"), Inventory->CountItemDefinitionInCharacterInventory(0, PotionDefinition->ItemDefinitionId), 0);
	FGridCombatHotbarBinding PersistentBinding;
	TestTrue(TEXT("The exhausted shortcut remains readable"), Inventory->GetCharacterCombatHotbarBinding(0, 1, PersistentBinding));
	TestTrue(TEXT("The last potion clears its shortcut"), PersistentBinding.IsEmpty());
	TestFalse(TEXT("The exhausted HUD slot has no binding"), Fixture.Hud->View.Actions[1].bHasBinding);
	TestFalse(TEXT("The exhausted HUD slot no longer resolves"), Fixture.Hud->View.Actions[1].bResolved);

	FGridItemInstance ReplacementPotion = Potion;
	ReplacementPotion.RuntimeObjectId = FGuid(12, 8, 4, 2);
	ReplacementPotion.Quantity = 3;
	TestTrue(TEXT("A replacement stack can be added"), Inventory->AddItemToCharacterInventory(0, ReplacementPotion));
	Fixture.Hud->RefreshFromSources();
	TestFalse(TEXT("A replacement stack does not restore the old shortcut"), Fixture.Hud->View.Actions[1].bHasBinding);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1284QuickItemScrollAttackTest, "Grimrock.Monsters.MON12.8.4.ScrollAttackConsumption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1284QuickItemScrollAttackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.8.4 scroll fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	UGridPartyInventoryComponent* Inventory = Fixture.Party->PartyInventoryComponent;
	UGridItemDefinitionAsset* ScrollDefinition = NewObject<UGridItemDefinitionAsset>(Fixture.Party);
	ScrollDefinition->ItemDefinitionId = TEXT("Scroll_MON1284_Fire");
	ScrollDefinition->DisplayName = FText::FromString(TEXT("Parchemin de feu MON12.8.4"));
	ScrollDefinition->ItemType = EGridItemType::Scroll;
	ScrollDefinition->bStackable = true;
	ScrollDefinition->MaxStackSize = 10;
	ScrollDefinition->bProvidesQuickItemCombatAction = true;
	FGridCombatActionDefinition& ScrollAction = ScrollDefinition->QuickItemCombatAction;
	ScrollAction.ActionType = EGridCombatActionType::RangedAttack;
	ScrollAction.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
	ScrollAction.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
	ScrollAction.ActionPointCost = 2;
	ScrollAction.RangeCells = 2;
	ScrollAction.OffensiveProfile.AttackId = TEXT("Attack_MON1284_FireScroll");
	ScrollAction.OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Fire;
	ScrollAction.OffensiveProfile.AttackDefinition.MinDamage = 2;
	ScrollAction.OffensiveProfile.AttackDefinition.MaxDamage = 2;
	ScrollAction.OffensiveProfile.AttackDefinition.AccuracyBonus = 100;
	ScrollAction.OffensiveProfile.DamageScalingAttribute = EGridAttackScalingAttribute::None;
	ScrollAction.OffensiveProfile.RangeCells = 2;
	if (!TestTrue(TEXT("The combat scroll definition is registered"), Inventory->RegisterItemDefinition(ScrollDefinition)))
	{
		return false;
	}

	FGridItemInstance Scroll;
	Scroll.RuntimeObjectId = FGuid(12, 8, 4, 3);
	Scroll.ItemDefinitionId = ScrollDefinition->ItemDefinitionId;
	Scroll.DisplayName = ScrollDefinition->DisplayName;
	Scroll.Quantity = 2;
	TestTrue(TEXT("Two scroll units enter the inventory"), Inventory->AddItemToCharacterInventory(0, Scroll));
	TestTrue(TEXT("The scroll configures shortcut two"), Inventory->SetCharacterCombatHotbarBindingFromItem(0, 1, Scroll, EGridEquipmentSlot::None));

	Fixture.Party->Facing = EGridEdge::South;
	Fixture.Hud->RefreshFromSources();
	FGridCombatActionRequestResult RejectedScroll;
	TestFalse(TEXT("A scroll attack without a target is rejected"), Fixture.Hud->RequestHotbarSlot(1, RejectedScroll));
	TestEqual(TEXT("A rejected scroll consumes no unit"), Inventory->CountItemDefinitionInCharacterInventory(0, ScrollDefinition->ItemDefinitionId), 2);
	TestEqual(TEXT("A rejected scroll consumes no action point"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 4);

	Fixture.Party->Facing = EGridEdge::North;
	FGridCombatActionRequestResult AcceptedScroll;
	TestTrue(TEXT("The axial scroll attack is accepted"), Fixture.Hud->RequestHotbarSlot(1, AcceptedScroll));
	TestTrue(TEXT("The generic result records acceptance"), AcceptedScroll.bAccepted);
	TestTrue(TEXT("The quick-item attack request is structurally valid"), AcceptedScroll.AttackRequest.IsValid());
	TestEqual(TEXT("The scroll source is recorded on the attack"), AcceptedScroll.AttackRequest.OffensiveItemDefinitionId, ScrollDefinition->ItemDefinitionId);
	TestEqual(TEXT("A scroll never claims an equipment slot"), AcceptedScroll.AttackRequest.OffensiveEquipmentSlot, EGridEquipmentSlot::None);
	TestEqual(TEXT("Exactly one accepted scroll is consumed"), Inventory->CountItemDefinitionInCharacterInventory(0, ScrollDefinition->ItemDefinitionId), 1);
	TestEqual(TEXT("The scroll result records the remaining unit"), AcceptedScroll.QuickItemResult.SourceQuantityAfter, 1);
	TestEqual(TEXT("The accepted scroll spends two action points"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 2);
	FGridCombatHotbarBinding ConsumedScrollBinding;
	TestTrue(TEXT("The partially consumed scroll shortcut remains readable"), Inventory->GetCharacterCombatHotbarBinding(0, 1, ConsumedScrollBinding));
	TestFalse(TEXT("A partial scroll use keeps its shortcut"), ConsumedScrollBinding.IsEmpty());
	TestTrue(TEXT("The remaining scroll stays in the HUD slot"), Fixture.Hud->View.Actions[1].bHasBinding);
	TestEqual(TEXT("The HUD reports the remaining scroll quantity"), Fixture.Hud->View.Actions[1].Action.CurrentSourceItemQuantity, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1285ActionPaletteBindingTest, "Grimrock.Monsters.MON12.8.5.ActionPaletteBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1285ActionPaletteBindingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.8.5 palette fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	URPGClassAsset* MageClass = NewObject<URPGClassAsset>(Fixture.Party);
	MageClass->ClassId = TEXT("Mage_MON1285");
	MageClass->DisplayName = FText::FromString(TEXT("Mage"));
	MageClass->HealthAtLevelOne = 6;
	MageClass->CombatActions = { MakeMON1285DirectSpell(), MakeMON1285SelfAbility() };
	TestTrue(TEXT("The class action source is valid"), MageClass->IsValidDefinition());

	FGridCharacterInventoryState& Character = Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0];
	Character.ClassId = MageClass->ClassId;
	Character.ClassDefinition = MageClass;
	Fixture.Hud->RefreshFromSources();

	FGridAvailableCombatAction SpellAction;
	if (!TestTrue(TEXT("The direct spell remains in the TurnManager catalogue"),
			FindCatalogAction(Fixture.TurnManager, 0, TEXT("Spell_MON1285_ArcaneBolt"), SpellAction)))
	{
		return false;
	}
	TestTrue(TEXT("A source UI can configure shortcut five directly from the catalogue"), Fixture.Hud->AssignCombatActionToHotbarSlot(4, SpellAction));

	FGridCombatHotbarBinding Binding;
	TestTrue(TEXT("The configured binding is persisted"), Fixture.Party->PartyInventoryComponent->GetCharacterCombatHotbarBinding(0, 4, Binding));
	TestEqual(TEXT("The binding keeps the spell identity"), Binding.ActionId, FName(TEXT("Spell_MON1285_ArcaneBolt")));
	TestEqual(TEXT("The binding keeps the Spell source policy"), Binding.SourcePolicy, EGridCombatActionSourcePolicy::Spell);
	TestEqual(TEXT("The binding identifies the contributing class"), Binding.SourceDefinitionId, MageClass->ClassId);
	TestFalse(TEXT("A class action never stores an item runtime id"), Binding.PreferredSourceRuntimeId.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1285DirectSpellManaTransactionTest, "Grimrock.Monsters.MON12.8.5.DirectSpellManaTransaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1285DirectSpellManaTransactionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.8.5 spell fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	URPGClassAsset* MageClass = NewObject<URPGClassAsset>(Fixture.Party);
	MageClass->ClassId = TEXT("Mage_MON1285_Spell");
	MageClass->HealthAtLevelOne = 6;
	MageClass->CombatActions = { MakeMON1285DirectSpell() };
	FGridCharacterInventoryState& Character = Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0];
	Character.ClassId = MageClass->ClassId;
	Character.ClassDefinition = MageClass;
	Fixture.Hud->RefreshFromSources();
	FGridAvailableCombatAction SpellAction;
	if (!TestTrue(TEXT("The spell is catalogued for the active mage"),
			FindCatalogAction(Fixture.TurnManager, 0, TEXT("Spell_MON1285_ArcaneBolt"), SpellAction)))
	{
		return false;
	}
	TestTrue(TEXT("The spell can be assigned without an item source"), Fixture.Hud->AssignCombatActionToHotbarSlot(2, SpellAction));

	Fixture.Party->Facing = EGridEdge::South;
	FGridCombatActionRequestResult Rejected;
	TestFalse(TEXT("A spell without an axial target is rejected"), Fixture.Hud->RequestHotbarSlot(2, Rejected));
	TestEqual(TEXT("The rejected spell restores its reserved mana"), Character.Resources.CurrentMana, 8);
	TestEqual(TEXT("The rejected spell consumes no action points"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 4);

	Fixture.Party->Facing = EGridEdge::North;
	FGridCombatActionRequestResult Accepted;
	TestTrue(TEXT("The direct axial spell is accepted"), Fixture.Hud->RequestHotbarSlot(2, Accepted));
	TestTrue(TEXT("The generic result reports spell acceptance"), Accepted.bAccepted);
	TestEqual(TEXT("The spell pays exactly three mana"), Character.Resources.CurrentMana, 5);
	TestEqual(TEXT("The spell pays exactly two action points"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 2);
	TestEqual(TEXT("The spell attack uses its offensive profile"), Accepted.AttackRequest.AttackId, FName(TEXT("Attack_MON1285_ArcaneBolt")));
	TestTrue(TEXT("The spell attack has no fake item source"), Accepted.AttackRequest.OffensiveItemDefinitionId.IsNone());
	TestEqual(TEXT("The class result records the mana transaction"), Accepted.ClassActionResult.ManaAfter, 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1285SelfAbilityEffectTest, "Grimrock.Monsters.MON12.8.5.SelfAbilityEffect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1285SelfAbilityEffectTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.8.5 ability fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	URPGClassAsset* PriestClass = NewObject<URPGClassAsset>(Fixture.Party);
	PriestClass->ClassId = TEXT("Priest_MON1285");
	PriestClass->HealthAtLevelOne = 8;
	PriestClass->CombatActions = { MakeMON1285SelfAbility() };
	FGridCharacterInventoryState& Character = Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0];
	Character.ClassId = PriestClass->ClassId;
	Character.ClassDefinition = PriestClass;
	Character.Resources.CurrentHealth = 10;
	Character.Resources.CurrentMana = 8;
	Fixture.Hud->RefreshFromSources();
	FGridAvailableCombatAction AbilityAction;
	if (!TestTrue(TEXT("The ability is catalogued for the active priest"),
			FindCatalogAction(Fixture.TurnManager, 0, TEXT("Ability_MON1285_Recovery"), AbilityAction)))
	{
		return false;
	}
	TestTrue(TEXT("The recovery ability can configure shortcut four"), Fixture.Hud->AssignCombatActionToHotbarSlot(3, AbilityAction));

	FGridCombatActionRequestResult Accepted;
	TestTrue(TEXT("The self-targeted ability is accepted"), Fixture.Hud->RequestHotbarSlot(3, Accepted));
	TestEqual(TEXT("The ability restores six health"), Character.Resources.CurrentHealth, 16);
	TestEqual(TEXT("The ability pays two mana"), Character.Resources.CurrentMana, 6);
	TestEqual(TEXT("The ability pays one action point"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 3);
	TestEqual(TEXT("The class result records restored health"), Accepted.ClassActionResult.HealthAfter, 16);

	Character.Resources.CurrentHealth = 20;
	Fixture.Hud->RefreshFromSources();
	TestEqual(TEXT("A useless recovery is disabled"), Fixture.Hud->View.Actions[3].Action.AvailabilityReason,
		EGridCombatActionAvailabilityReason::NoApplicableEffect);
	FGridCombatActionRequestResult Rejected;
	TestFalse(TEXT("A useless recovery cannot be spent"), Fixture.Hud->RequestHotbarSlot(3, Rejected));
	TestEqual(TEXT("The refused ability consumes no mana"), Character.Resources.CurrentMana, 6);
	TestEqual(TEXT("The refused ability consumes no action point"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1286CellTargetingLifecycleTest, "Grimrock.Monsters.MON12.8.6.CellTargetingLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1286CellTargetingLifecycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.8.6 cell fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	URPGClassAsset* MageClass = NewObject<URPGClassAsset>(Fixture.Party);
	MageClass->ClassId = TEXT("Mage_MON1286_Cell");
	MageClass->HealthAtLevelOne = 6;
	MageClass->CombatActions = { MakeMON1286CellSpell() };
	FGridCharacterInventoryState& Character = Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0];
	Character.ClassId = MageClass->ClassId;
	Character.ClassDefinition = MageClass;
	Fixture.Hud->RefreshFromSources();
	FGridAvailableCombatAction CellSpell;
	if (!TestTrue(TEXT("The cell spell is catalogued"),
			FindCatalogAction(Fixture.TurnManager, 0, TEXT("Spell_MON1286_CellStrike"), CellSpell)))
	{
		return false;
	}
	TestTrue(TEXT("The cell spell can configure shortcut six"), Fixture.Hud->AssignCombatActionToHotbarSlot(5, CellSpell));

	FGridCombatActionRequestResult Pending;
	TestTrue(TEXT("The shortcut opens explicit targeting"), Fixture.Hud->RequestHotbarSlot(5, Pending));
	TestTrue(TEXT("Targeting remains active until confirmation"), Fixture.Hud->IsCombatActionTargetingActive());
	TestFalse(TEXT("Opening targeting does not resolve the spell"), Pending.bAccepted);
	TestEqual(TEXT("Opening targeting spends no mana"), Character.Resources.CurrentMana, 8);
	TestEqual(TEXT("Opening targeting spends no action points"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 4);

	TestFalse(TEXT("An empty cell is not a valid attack target"), Fixture.Hud->UpdateCombatActionTargetingPreview(FIntPoint(0, 0)));
	TestTrue(TEXT("Invalid hover keeps targeting active"), Fixture.Hud->IsCombatActionTargetingActive());
	Fixture.Hud->CancelCombatActionTargeting();
	TestFalse(TEXT("Cancellation closes targeting"), Fixture.Hud->IsCombatActionTargetingActive());
	TestEqual(TEXT("Cancellation still spends no mana"), Character.Resources.CurrentMana, 8);

	TestTrue(TEXT("The shortcut can reopen targeting"), Fixture.Hud->RequestHotbarSlot(5, Pending));
	TestTrue(TEXT("The occupied monster cell previews as valid"), Fixture.Hud->UpdateCombatActionTargetingPreview(FIntPoint(1, 2)));
	TestEqual(TEXT("A cell spell covers exactly one cell"), Fixture.Hud->TargetingPreview.AffectedCells.Num(), 1);
	TestEqual(TEXT("A cell spell identifies exactly one monster"), Fixture.Hud->TargetingPreview.TargetMonsterIds.Num(), 1);

	FGridCombatActionRequestResult Accepted;
	TestTrue(TEXT("The selected monster cell is accepted"), Fixture.Hud->ConfirmCombatActionTarget(FIntPoint(1, 2), Accepted));
	TestFalse(TEXT("Accepted execution closes targeting"), Fixture.Hud->IsCombatActionTargetingActive());
	TestTrue(TEXT("The targeted result is accepted"), Accepted.bAccepted);
	TestEqual(TEXT("One targeted attack is resolved"), Accepted.TargetedActionResult.AttackResults.Num(), 1);
	TestEqual(TEXT("The cell spell pays exactly three mana"), Character.Resources.CurrentMana, 5);
	TestEqual(TEXT("The cell spell pays exactly two action points"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1286AreaTargetingTransactionTest, "Grimrock.Monsters.MON12.8.6.AreaTargetingTransaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1286AreaTargetingTransactionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.8.6 area fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	UGridMonsterOccupancySubsystem* Occupancy = Fixture.TestWorld.World->GetSubsystem<UGridMonsterOccupancySubsystem>();
	AGridMonsterActor* SecondMonster = Fixture.TestWorld.World->SpawnActor<AGridMonsterActor>();
	if (!TestNotNull(TEXT("The second area target is spawned"), SecondMonster) || !TestNotNull(TEXT("The occupancy subsystem is available"), Occupancy))
	{
		return false;
	}
	SecondMonster->InitializeMonster(Fixture.Monster->MonsterDefinition, FGuid(12, 8, 6, 2), FIntPoint(2, 2), EGridEdge::South);
	TestTrue(TEXT("The second area target occupies its cell"), Occupancy->RegisterMonster(SecondMonster, FIntPoint(2, 2)));
	Fixture.TurnManager->CombatMonsters.Add(SecondMonster);

	URPGClassAsset* MageClass = NewObject<URPGClassAsset>(Fixture.Party);
	MageClass->ClassId = TEXT("Mage_MON1286_Area");
	MageClass->HealthAtLevelOne = 6;
	MageClass->CombatActions = { MakeMON1286AreaSpell() };
	FGridCharacterInventoryState& Character = Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0];
	Character.ClassId = MageClass->ClassId;
	Character.ClassDefinition = MageClass;
	Fixture.Hud->RefreshFromSources();
	FGridAvailableCombatAction AreaSpell;
	if (!TestTrue(TEXT("The area spell is catalogued"),
			FindCatalogAction(Fixture.TurnManager, 0, TEXT("Spell_MON1286_AreaBurst"), AreaSpell)))
	{
		return false;
	}
	TestTrue(TEXT("The area spell can configure shortcut seven"), Fixture.Hud->AssignCombatActionToHotbarSlot(6, AreaSpell));

	FGridCombatActionRequestResult Pending;
	TestTrue(TEXT("The area shortcut opens targeting"), Fixture.Hud->RequestHotbarSlot(6, Pending));
	TestTrue(TEXT("The area centered on the first monster is valid"), Fixture.Hud->UpdateCombatActionTargetingPreview(FIntPoint(1, 2)));
	TestEqual(TEXT("The radius-one diamond contains four valid cells"), Fixture.Hud->TargetingPreview.AffectedCells.Num(), 4);
	TestEqual(TEXT("Both monsters are included in the preview"), Fixture.Hud->TargetingPreview.TargetMonsterIds.Num(), 2);

	FGridCombatActionRequestResult Accepted;
	TestTrue(TEXT("The area spell is accepted"), Fixture.Hud->ConfirmCombatActionTarget(FIntPoint(1, 2), Accepted));
	TestEqual(TEXT("Two attacks are resolved by one area action"), Accepted.TargetedActionResult.AttackResults.Num(), 2);
	TestEqual(TEXT("Both target identities are retained"), Accepted.TargetedActionResult.TargetMonsterIds.Num(), 2);
	TestEqual(TEXT("The area spell pays mana only once"), Character.Resources.CurrentMana, 5);
	TestEqual(TEXT("The area spell pays action points only once"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1286TargetRequiredNoSpendTest, "Grimrock.Monsters.MON12.8.6.TargetRequiredNoSpend",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1286TargetRequiredNoSpendTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.8.6 rejection fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	URPGClassAsset* MageClass = NewObject<URPGClassAsset>(Fixture.Party);
	MageClass->ClassId = TEXT("Mage_MON1286_Reject");
	MageClass->HealthAtLevelOne = 6;
	MageClass->CombatActions = { MakeMON1286AreaSpell() };
	FGridCharacterInventoryState& Character = Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0];
	Character.ClassId = MageClass->ClassId;
	Character.ClassDefinition = MageClass;
	Fixture.Hud->RefreshFromSources();
	FGridAvailableCombatAction AreaSpell;
	if (!TestTrue(TEXT("The rejected area spell is catalogued"),
			FindCatalogAction(Fixture.TurnManager, 0, TEXT("Spell_MON1286_AreaBurst"), AreaSpell)))
	{
		return false;
	}
	const FGridAvailableCombatAction& Action = AreaSpell;

	FGridCombatActionRequestResult MissingTarget;
	TestFalse(TEXT("A targeted action cannot execute without a cell"),
		Fixture.TurnManager->RequestCharacterCombatAction(
			0, Action.Definition.ActionId, Action.Definition.SourcePolicy, Action.SourceDefinitionId, Action.SourceEquipmentSlot, MissingTarget));
	TestEqual(TEXT("The generic request reports TargetRequired"), MissingTarget.RejectReason, EGridCombatActionRequestRejectReason::TargetRequired);

	FGridCombatActionRequestResult EmptyArea;
	TestFalse(TEXT("An area without an enemy is rejected"),
		Fixture.TurnManager->RequestCharacterCombatActionAtCell(
			0, Action.Definition.ActionId, Action.Definition.SourcePolicy, Action.SourceDefinitionId, Action.SourceEquipmentSlot, FIntPoint(0, 0), EmptyArea));
	TestEqual(TEXT("The empty area reports InvalidTarget"), EmptyArea.RejectReason, EGridCombatActionRequestRejectReason::InvalidTarget);
	TestEqual(TEXT("Rejected targeting spends no mana"), Character.Resources.CurrentMana, 8);
	FGridPlayerCharacterTurnState TurnState;
	TestTrue(TEXT("The active turn state remains available"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, TurnState));
	TestEqual(TEXT("Rejected targeting spends no action points"), TurnState.RemainingActionPoints, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1287PersistentHotbarTest, "Grimrock.Monsters.MON12.8.7.PersistentHotbarAndUnarmedPalette",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1287PersistentHotbarTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.8.7 HUD fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	UCanvasPanel* RootPanel = NewObject<UCanvasPanel>(Fixture.Hud, TEXT("Panel_CombatHud_1287"));
	Fixture.Hud->Panel_CombatHud = RootPanel;
	Fixture.Hud->Panel_Initiative = NewObject<UHorizontalBox>(Fixture.Hud, TEXT("Panel_Initiative_1287"));
	Fixture.Hud->Button_EndTurn = NewObject<UButton>(Fixture.Hud, TEXT("Button_EndTurn_1287"));

	Fixture.TurnManager->bCombatActive = false;
	Fixture.TurnManager->CurrentPhase = EGridCombatPhase::Exploration;
	TestTrue(TEXT("The third character can be selected out of combat"), Fixture.Party->PartyInventoryComponent->SetSelectedCharacterIndex(2));
	Fixture.Hud->RefreshFromSources();

	TestFalse(TEXT("The encounter is inactive"), Fixture.Hud->View.bCombatActive);
	TestEqual(TEXT("The selected character owns the out-of-combat bar"), Fixture.Hud->View.ActiveCharacterIndex, 2);
	TestEqual(TEXT("The minimum persistent action-bar slots remain projected out of combat"), Fixture.Hud->View.Actions.Num(), FGridCombatHotbarBinding::MinimumSlotCount);
	TestEqual(TEXT("Combat-only chrome is hidden out of combat"), RootPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Combat-only initiative is hidden out of combat"), Fixture.Hud->Panel_Initiative->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Combat-only end turn is hidden out of combat"), Fixture.Hud->Button_EndTurn->GetVisibility(), ESlateVisibility::Collapsed);
	TestTrue(TEXT("Out-of-combat slot 1 remains PrimaryAttack"), Fixture.Hud->View.Actions[0].Binding.IsPrimaryAttackBinding());
	TestTrue(TEXT("PrimaryAttack resolves out of combat"), Fixture.Hud->View.Actions[0].bResolved);
	TestEqual(TEXT("A character without a weapon falls back to unarmed"), Fixture.Hud->View.Actions[0].Action.Definition.ActionId, FName(TEXT("Attack_Unarmed")));
	TestFalse(TEXT("The resolved unarmed fallback cannot execute outside combat"), Fixture.Hud->View.Actions[0].Action.bEnabled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1287InventoryThrowableTest, "Grimrock.Monsters.MON12.8.7.InventoryThrowableHotbar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1287InventoryThrowableTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.8.7 throwable fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	UGridPartyInventoryComponent* Inventory = Fixture.Party->PartyInventoryComponent;
	UGridItemDefinitionAsset* ShurikenDefinition = NewObject<UGridItemDefinitionAsset>(Fixture.Party);
	ShurikenDefinition->ItemDefinitionId = TEXT("Shuriken_MON1287");
	ShurikenDefinition->DisplayName = FText::FromString(TEXT("Shuriken"));
	ShurikenDefinition->Description = FText::FromString(TEXT("Arme de jet rapide."));
	ShurikenDefinition->ItemType = EGridItemType::Weapon;
	ShurikenDefinition->bCombatThrowWeapon = true;
	ShurikenDefinition->bProvidesAttackPresentation = true;
	ShurikenDefinition->PlayerAttackPresentationProfile.MotionStyle = EGridPlayerAttackMotionStyle::Throw;
	ShurikenDefinition->PlayerAttackPresentationProfile.bAnimateHeldItem = true;
	ShurikenDefinition->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::MainHand);
	FGridCombatActionDefinition MisconfiguredThrow;
	MisconfiguredThrow.ActionId = TEXT("Attack_Shuriken_MON1287_Configured");
	MisconfiguredThrow.ActionType = EGridCombatActionType::RangedAttack;
	MisconfiguredThrow.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
	MisconfiguredThrow.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
	MisconfiguredThrow.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
	MisconfiguredThrow.ActionPointCost = 2;
	MisconfiguredThrow.ResourceCosts.SourceItemQuantityCost = 2;
	MisconfiguredThrow.RangeCells = 3;
	MisconfiguredThrow.PresentationProfileId = TEXT("Attack_Shuriken_MON1287");
	MisconfiguredThrow.OffensiveProfile.AttackId = TEXT("Attack_Shuriken_MON1287");
	MisconfiguredThrow.OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Physical;
	MisconfiguredThrow.OffensiveProfile.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
	MisconfiguredThrow.OffensiveProfile.AttackDefinition.MinDamage = 2;
	MisconfiguredThrow.OffensiveProfile.AttackDefinition.MaxDamage = 2;
	MisconfiguredThrow.OffensiveProfile.AttackDefinition.AccuracyBonus = 100;
	MisconfiguredThrow.OffensiveProfile.RangeCells = 3;
	ShurikenDefinition->CombatActions.Add(MisconfiguredThrow);
	TestTrue(TEXT("The throwable definition is registered"), Inventory->RegisterItemDefinition(ShurikenDefinition));

	FGridItemInstance Shuriken;
	Shuriken.RuntimeObjectId = FGuid::NewGuid();
	Shuriken.ItemDefinitionId = ShurikenDefinition->ItemDefinitionId;
	Shuriken.DisplayName = ShurikenDefinition->DisplayName;
	Shuriken.Quantity = 1;
	Shuriken.OwnerType = EGridItemOwnerType::CharacterInventory;
	Shuriken.OwnerCharacterIndex = 0;
	FGridInventorySlot& InventorySlot = Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[0];
	InventorySlot.bOccupied = true;
	InventorySlot.Item = Shuriken;

	UGridInventoryDragDropOperation* ShurikenDrag = NewObject<UGridInventoryDragDropOperation>(Fixture.Hud);
	ShurikenDrag->InitializeFromSlot(EGridInventoryUiSlotType::Inventory, 0, Shuriken);
	TestTrue(TEXT("The inventory shuriken configures slot seven"), Fixture.Hud->HandleHotbarDrop(6, ShurikenDrag));

	FGridCombatHotbarBinding Binding;
	TestTrue(TEXT("The shuriken binding is persisted"), Inventory->GetCharacterCombatHotbarBinding(0, 6, Binding));
	TestEqual(TEXT("The inventory throw uses a quick-item source"), Binding.SourcePolicy, EGridCombatActionSourcePolicy::QuickItem);
	TestEqual(TEXT("The throw keeps the stable item definition"), Binding.SourceDefinitionId, ShurikenDefinition->ItemDefinitionId);
	TestEqual(TEXT("The inventory source is not moved by assignment"),
		Inventory->CountItemDefinitionInCharacterInventory(0, ShurikenDefinition->ItemDefinitionId), 1);
	TestTrue(TEXT("The configured shuriken resolves immediately"), Fixture.Hud->View.Actions[6].bResolved);
	TestTrue(TEXT("The configured shuriken is usable in combat"), Fixture.Hud->View.Actions[6].Action.bEnabled);

	AGridThrownItemActor* PreviewProjectile =
		Fixture.Party->TryLaunchInventoryItemForAttack(0, ShurikenDefinition->ItemDefinitionId, Fixture.Monster->GetActorLocation(), FIntPoint(1, 1));
	TestNotNull(TEXT("The inventory throw can create a recoverable projectile"), PreviewProjectile);
	TestEqual(
		TEXT("Presentation alone never consumes the source"), Inventory->CountItemDefinitionInCharacterInventory(0, ShurikenDefinition->ItemDefinitionId), 1);
	if (PreviewProjectile)
	{
		PreviewProjectile->Destroy();
	}

	UGridPlayerAttackPresentationComponent* Presentation =
		NewObject<UGridPlayerAttackPresentationComponent>(Fixture.Runtime, TEXT("MON12_8_7_InventoryThrowPresentation"));
	Fixture.Runtime->AddInstanceComponent(Presentation);
	Presentation->RegisterComponent();
	Presentation->bNativeAudioPlaybackEnabled = false;
	Presentation->bNativeVFXSpawnEnabled = false;
	Presentation->bNativeFeedbackEnabled = false;
	TestTrue(TEXT("The inventory throw presentation is initialized"), Presentation->InitializePresentation(Fixture.TurnManager));

	FGridCombatActionRequestResult Result;
	TestTrue(TEXT("The shuriken shortcut executes from inventory"), Fixture.Hud->RequestHotbarSlot(6, Result));
	TestTrue(TEXT("The inventory throw is accepted"), Result.bAccepted);
	TestEqual(TEXT("The inventory throwable cost is normalized to one"), Result.Action.CurrentSourceItemQuantityCost, 1);
	TestEqual(TEXT("The attack uses the shuriken offensive profile"), Result.AttackRequest.AttackId, FName(TEXT("Attack_Shuriken_MON1287")));
	TestEqual(TEXT("The attack has no fake equipment slot"), Result.AttackRequest.OffensiveEquipmentSlot, EGridEquipmentSlot::None);
	TestEqual(TEXT("The accepted inventory throw requests one launch"), Presentation->ThrownItemLaunchRequestCount, 1);
	TestEqual(TEXT("The accepted inventory throw starts one projectile"), Presentation->ThrownItemLaunchStartedCount, 1);
	TestEqual(TEXT("Exactly one shuriken is consumed after acceptance"),
		Inventory->CountItemDefinitionInCharacterInventory(0, ShurikenDefinition->ItemDefinitionId), 0);
	FGridCombatHotbarBinding ExhaustedShurikenBinding;
	TestTrue(TEXT("The exhausted shuriken shortcut remains readable"), Inventory->GetCharacterCombatHotbarBinding(0, 6, ExhaustedShurikenBinding));
	TestTrue(TEXT("The last shuriken clears its hotbar slot"), ExhaustedShurikenBinding.IsEmpty());
	TestFalse(TEXT("The exhausted shuriken disappears from the HUD"), Fixture.Hud->View.Actions[6].bHasBinding);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1289WeaponDragDropUniquenessTest, "Grimrock.Monsters.MON12.8.9.WeaponDragDropMovesBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1289WeaponDragDropUniquenessTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.8.9 drag/drop fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	UGridPartyInventoryComponent* Inventory = Fixture.Party->PartyInventoryComponent;
	FGridItemInstance EquippedSword;
	if (!TestTrue(TEXT("The fixture exposes the equipped sword"), Inventory->GetEquippedItem(0, EGridEquipmentSlot::MainHand, EquippedSword)))
	{
		return false;
	}

	UGridInventoryDragDropOperation* SwordDrag = NewObject<UGridInventoryDragDropOperation>(Fixture.Hud);
	SwordDrag->InitializeFromSlot(EGridInventoryUiSlotType::MainHand, 0, EquippedSword);
	TestTrue(TEXT("The first drag configures slot three"), Fixture.Hud->HandleHotbarDrop(2, SwordDrag));
	TestTrue(TEXT("The repeated drag moves the sword to slot nine"), Fixture.Hud->HandleHotbarDrop(8, SwordDrag));

	FGridCombatHotbarBinding PreviousBinding;
	FGridCombatHotbarBinding CurrentBinding;
	Inventory->GetCharacterCombatHotbarBinding(0, 2, PreviousBinding);
	Inventory->GetCharacterCombatHotbarBinding(0, 8, CurrentBinding);
	TestTrue(TEXT("The previous drag/drop slot is empty"), PreviousBinding.IsEmpty());
	TestTrue(TEXT("The target slot keeps the exact weapon instance"), CurrentBinding.PreferredSourceRuntimeId == EquippedSword.RuntimeObjectId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON129ActionRolloverTest, "Grimrock.Monsters.MON12.9.ActionRollover", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON129ActionRolloverTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridAvailableCombatAction AvailableAction;
	AvailableAction.Definition.ActionId = TEXT("Attack_MON129_Shuriken");
	AvailableAction.Definition.DisplayName = FText::FromString(TEXT("Shuriken"));
	AvailableAction.Definition.Description = FText::FromString(TEXT("Lance un shuriken sur la cible."));
	AvailableAction.Definition.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
	AvailableAction.CurrentActionPointCost = 2;
	AvailableAction.CurrentSourceItemQuantityCost = 1;
	AvailableAction.CurrentSourceItemQuantity = 4;
	AvailableAction.bEnabled = true;

	FGridCombatHudActionView EmptyView;
	EmptyView.HotbarSlotIndex = 0;
	EmptyView.ShortcutText = FText::FromString(TEXT("1"));
	UGridCombatHudActionWidget* EmptyWidget = NewObject<UGridCombatHudActionWidget>();
	EmptyWidget->Button_Action = NewObject<UButton>(EmptyWidget);
	EmptyWidget->InitializeAction(nullptr, EmptyView);
	TestTrue(TEXT("The empty rollover explains assignment"), EmptyWidget->Button_Action->GetToolTipText().ToString().Contains(TEXT("Déposez ici un objet de l'inventaire")));

	FGridCombatHudActionView AssignedView;
	AssignedView.HotbarSlotIndex = 2;
	AssignedView.ShortcutText = FText::FromString(TEXT("3"));
	AssignedView.bHasBinding = true;
	AssignedView.bResolved = true;
	AssignedView.Action = AvailableAction;
	AssignedView.Binding.ActionId = AvailableAction.Definition.ActionId;
	AssignedView.Binding.SourcePolicy = AvailableAction.Definition.SourcePolicy;

	UGridCombatHudActionWidget* AssignedWidget = NewObject<UGridCombatHudActionWidget>();
	AssignedWidget->Button_Action = NewObject<UButton>(AssignedWidget);
	AssignedWidget->InitializeAction(nullptr, AssignedView);
	const FString AssignedToolTip = AssignedWidget->Button_Action->GetToolTipText().ToString();
	TestTrue(TEXT("The assigned rollover contains the action name"), AssignedToolTip.Contains(TEXT("Shuriken")));
	TestTrue(TEXT("The assigned rollover contains the complete cost"), AssignedToolTip.Contains(TEXT("Coût : 2 PA — quantité : 4")));
	TestTrue(TEXT("The assigned rollover contains the description"), AssignedToolTip.Contains(TEXT("Lance un shuriken sur la cible.")));
	TestTrue(TEXT("The assigned rollover explains removal"), AssignedToolTip.Contains(TEXT("Clic droit : retirer le raccourci.")));

	FGridCombatHudActionView DisabledView = AssignedView;
	DisabledView.Action.bEnabled = false;
	DisabledView.DisabledReason = FText::FromString(TEXT("aucun combat n’est actif."));
	UGridCombatHudActionWidget* DisabledWidget = NewObject<UGridCombatHudActionWidget>();
	DisabledWidget->Button_Action = NewObject<UButton>(DisabledWidget);
	DisabledWidget->InitializeAction(nullptr, DisabledView);
	const FString DisabledToolTip = DisabledWidget->Button_Action->GetToolTipText().ToString();
	TestTrue(TEXT("The disabled rollover keeps the description"), DisabledToolTip.Contains(TEXT("Lance un shuriken sur la cible.")));
	TestTrue(TEXT("The disabled rollover explains unavailability"), DisabledToolTip.Contains(TEXT("Indisponible : aucun combat n’est actif.")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1210HotbarTargetingTest, "Grimrock.Monsters.MON12.10.HotbarTargeting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1210HotbarTargetingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.10 fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	URPGClassAsset* MageClass = NewObject<URPGClassAsset>(Fixture.Party);
	MageClass->ClassId = TEXT("Mage_MON1210");
	MageClass->HealthAtLevelOne = 6;
	MageClass->CombatActions = { MakeMON1286AreaSpell() };
	FGridCharacterInventoryState& Character = Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0];
	Character.ClassId = MageClass->ClassId;
	Character.ClassDefinition = MageClass;
	Fixture.Hud->RefreshFromSources();

	FGridAvailableCombatAction AreaSpell;
	if (!TestTrue(TEXT("The synthetic area spell is catalogued"),
			FindCatalogAction(Fixture.TurnManager, 0, TEXT("Spell_MON1286_AreaBurst"), AreaSpell)))
	{
		return false;
	}
	TestTrue(TEXT("The area spell configures shortcut seven"), Fixture.Hud->AssignCombatActionToHotbarSlot(6, AreaSpell));
	TestFalse(TEXT("Targeting starts inactive"), Fixture.Hud->IsCombatActionTargetingActive());

	FGridCombatActionRequestResult Pending;
	TestTrue(TEXT("The shortcut opens area targeting"), Fixture.Hud->RequestHotbarSlot(6, Pending));
	TestTrue(TEXT("Targeting backend is active"), Fixture.Hud->IsCombatActionTargetingActive());
	TestEqual(TEXT("The pending preview keeps the selected action"), Fixture.Hud->TargetingPreview.Action.Definition.ActionId,
		FName(TEXT("Spell_MON1286_AreaBurst")));

	TestFalse(TEXT("An empty area is invalid"), Fixture.Hud->UpdateCombatActionTargetingPreview(FIntPoint(0, 0)));
	TestTrue(TEXT("Invalid preview keeps targeting active"), Fixture.Hud->IsCombatActionTargetingActive());

	TestTrue(TEXT("The monster area previews as valid"), Fixture.Hud->UpdateCombatActionTargetingPreview(FIntPoint(1, 2)));
	TestTrue(TEXT("Valid preview contains at least one monster"), Fixture.Hud->TargetingPreview.TargetMonsterIds.Num() > 0);

	Fixture.Hud->CancelCombatActionTargeting();
	TestFalse(TEXT("Cancellation closes targeting"), Fixture.Hud->IsCombatActionTargetingActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1211HotbarValidationTest, "Grimrock.Monsters.MON12.11.HotbarValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1211HotbarValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.11 fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}
	TestTrue(TEXT("The MON12.11 phase state starts combat"), Fixture.TurnManager->PhaseState.StartCombat());
	TestTrue(TEXT("The MON12.11 phase state starts round one"), Fixture.TurnManager->PhaseState.BeginRound());
	Fixture.TurnManager->CurrentPhase = Fixture.TurnManager->PhaseState.GetPhase();
	Fixture.TurnManager->RoundNumber = Fixture.TurnManager->PhaseState.GetRoundNumber();

	UGridPartyInventoryComponent* Inventory = Fixture.Party->PartyInventoryComponent;
	FGridItemInstance EquippedSword;
	if (!TestTrue(TEXT("The validation fixture exposes its sword"), Inventory->GetEquippedItem(0, EGridEquipmentSlot::MainHand, EquippedSword)))
	{
		return false;
	}
	FGridCombatHotbarBinding PrimaryBinding;
	TestTrue(TEXT("Slot 1 remains PrimaryAttack"), Inventory->GetCharacterCombatHotbarBinding(0, 0, PrimaryBinding) && PrimaryBinding.IsPrimaryAttackBinding());

	TestTrue(TEXT("The sword configures slot two"), Inventory->SetCharacterCombatHotbarBindingFromItem(0, 1, EquippedSword, EGridEquipmentSlot::MainHand));
	FGridCombatHotbarBinding UnarmedBinding;
	UnarmedBinding.Reset(2);
	UnarmedBinding.ActionId = TEXT("Attack_Unarmed");
	UnarmedBinding.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
	TestTrue(TEXT("Unarmed configures slot three"), Inventory->SetCharacterCombatHotbarBinding(0, 2, UnarmedBinding));

	FGridCombatHotbarBinding SwordBinding;
	TestTrue(TEXT("The sword binding is readable before the swap"), Inventory->GetCharacterCombatHotbarBinding(0, 1, SwordBinding));
	UGridCombatHotbarDragDropOperation* SwapOperation = NewObject<UGridCombatHotbarDragDropOperation>(Fixture.Hud);
	SwapOperation->InitializeFromHotbarSlot(0, 1, SwordBinding);
	TestTrue(TEXT("Dropping onto an occupied configurable slot swaps bindings"), Fixture.Hud->HandleHotbarDrop(2, SwapOperation));

	FGridCombatHotbarBinding FixedPrimary;
	FGridCombatHotbarBinding SwappedSecond;
	FGridCombatHotbarBinding SwappedThird;
	Inventory->GetCharacterCombatHotbarBinding(0, 0, FixedPrimary);
	Inventory->GetCharacterCombatHotbarBinding(0, 1, SwappedSecond);
	Inventory->GetCharacterCombatHotbarBinding(0, 2, SwappedThird);
	TestTrue(TEXT("PrimaryAttack is unaffected by configurable-slot swaps"), FixedPrimary.IsPrimaryAttackBinding());
	TestEqual(TEXT("Slot two now contains unarmed"), SwappedSecond.ActionId, FName(TEXT("Attack_Unarmed")));
	TestTrue(TEXT("Slot three keeps the exact sword instance"), SwappedThird.PreferredSourceRuntimeId == EquippedSword.RuntimeObjectId);

	UGridItemDefinitionAsset* SwordDefinition = Inventory->FindItemDefinition(EquippedSword.ItemDefinitionId);
	TestNotNull(TEXT("The hotbar sword definition remains registered"), SwordDefinition);
	if (SwordDefinition && !SwordDefinition->CombatActions.IsEmpty())
	{
		SwordDefinition->CombatActions[0].ActionPointCost = 5;
	}
	Fixture.Hud->RefreshFromSources();
	FGridPlayerCharacterTurnState TurnStateBeforeRefusal;
	Fixture.TurnManager->GetPlayerCharacterTurnState(0, TurnStateBeforeRefusal);
	FGridCombatActionRequestResult Rejected;
	TestFalse(TEXT("An unavailable sword shortcut is refused"), Fixture.Hud->RequestHotbarSlot(2, Rejected));
	FGridCombatHotbarBinding BindingAfterRefusal;
	Inventory->GetCharacterCombatHotbarBinding(0, 2, BindingAfterRefusal);
	TestTrue(TEXT("Refusal preserves the configured shortcut"), BindingAfterRefusal.PreferredSourceRuntimeId == EquippedSword.RuntimeObjectId);
	FGridPlayerCharacterTurnState TurnStateAfterRefusal;
	Fixture.TurnManager->GetPlayerCharacterTurnState(0, TurnStateAfterRefusal);
	TestEqual(TEXT("Refusal preserves action points"), TurnStateAfterRefusal.RemainingActionPoints, TurnStateBeforeRefusal.RemainingActionPoints);

	FGridPartyInventoryState SavedState = Inventory->PartyInventoryState;
	SavedState.bInitialCharacterCreationCompleted = true;
	UGridPartyInventoryComponent* RestoredInventory = NewObject<UGridPartyInventoryComponent>(Fixture.Party);
	FText RestoreError;
	TestTrue(TEXT("The saved hotbar state restores successfully"), RestoredInventory->RestorePartyInventoryState(SavedState, RestoreError));
	FGridCombatHotbarBinding RestoredSword;
	RestoredInventory->GetCharacterCombatHotbarBinding(0, 2, RestoredSword);
	TestTrue(TEXT("Restore keeps the exact sword identity"), RestoredSword.PreferredSourceRuntimeId == EquippedSword.RuntimeObjectId);

	TestTrue(TEXT("Right-click contract clears the configurable sword shortcut"), Fixture.Hud->ClearHotbarSlot(2));
	FGridCombatHotbarBinding ClearedBinding;
	Inventory->GetCharacterCombatHotbarBinding(0, 2, ClearedBinding);
	TestTrue(TEXT("The cleared configurable shortcut is empty"), ClearedBinding.IsEmpty());
	TestFalse(TEXT("PrimaryAttack cannot be cleared"), Fixture.Hud->ClearHotbarSlot(0));
	FGridItemInstance SwordAfterClear;
	TestTrue(TEXT("Clearing never removes the equipped source"), Inventory->GetEquippedItem(0, EGridEquipmentSlot::MainHand, SwordAfterClear));
	TestTrue(TEXT("The equipped source identity is unchanged"), SwordAfterClear.RuntimeObjectId == EquippedSword.RuntimeObjectId);

	TestTrue(TEXT("The real initiative flow ends character one's turn"), Fixture.TurnManager->EndActivePlayerTurn());
	TestEqual(TEXT("The HUD projects character two"), Fixture.Hud->View.ActiveCharacterIndex, 1);
	TestFalse(TEXT("Character two does not inherit character one's configurable slots"), Fixture.Hud->View.Actions[2].bHasBinding);
	TestTrue(TEXT("Character two still owns its own PrimaryAttack"), Fixture.Hud->View.Actions[0].Binding.IsPrimaryAttackBinding());

	Fixture.TurnManager->FinishCombat(EGridCombatPhase::Victory);
	Fixture.Hud->RefreshFromSources();
	FGridCombatHotbarBinding PersistentPrimary;
	Inventory->GetCharacterCombatHotbarBinding(0, 0, PersistentPrimary);
	TestTrue(TEXT("Combat finish preserves PrimaryAttack"), PersistentPrimary.IsPrimaryAttackBinding());
	TestFalse(TEXT("The primary action cannot execute after combat"), Fixture.Hud->View.Actions[0].Action.bEnabled);
	FGridCombatActionRequestResult AfterCombat;
	TestFalse(TEXT("The real action entry point refuses after combat"),
		Fixture.TurnManager->RequestCharacterCombatAction(
			0, TEXT("Attack_Unarmed"), EGridCombatActionSourcePolicy::Universal, NAME_None, EGridEquipmentSlot::None, AfterCombat));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON12ActionTransactionTest, "Grimrock.Monsters.MON12.Coherence.ActionTransactions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12ActionTransactionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12 transaction fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	UGridTurnManagerComponent* TurnManager = Fixture.TurnManager;
	UGridPartyInventoryComponent* Inventory = Fixture.Party->PartyInventoryComponent;
	TestTrue(TEXT("The real phase state enters combat"), TurnManager->PhaseState.StartCombat());
	TestTrue(TEXT("The real phase state enters round one"), TurnManager->PhaseState.BeginRound());
	TurnManager->CurrentPhase = TurnManager->PhaseState.GetPhase();
	TurnManager->RoundNumber = TurnManager->PhaseState.GetRoundNumber();

	UGridItemDefinitionAsset* ShurikenDefinition = NewObject<UGridItemDefinitionAsset>(Fixture.Party);
	ShurikenDefinition->ItemDefinitionId = TEXT("Shuriken_MON12_Transaction");
	ShurikenDefinition->DisplayName = FText::FromString(TEXT("Shuriken transactionnel"));
	ShurikenDefinition->ItemType = EGridItemType::Weapon;
	ShurikenDefinition->bStackable = true;
	ShurikenDefinition->MaxStackSize = 10;
	ShurikenDefinition->bCombatThrowWeapon = true;
	ShurikenDefinition->bProvidesAttackPresentation = true;
	ShurikenDefinition->PlayerAttackPresentationProfile.MotionStyle = EGridPlayerAttackMotionStyle::Throw;
	ShurikenDefinition->PlayerAttackPresentationProfile.bAnimateHeldItem = true;
	ShurikenDefinition->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::MainHand);

	FGridCombatActionDefinition ShurikenAction;
	ShurikenAction.ActionId = TEXT("Attack_MON12_Transaction");
	ShurikenAction.DisplayName = ShurikenDefinition->DisplayName;
	ShurikenAction.ActionType = EGridCombatActionType::RangedAttack;
	ShurikenAction.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
	ShurikenAction.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
	ShurikenAction.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
	ShurikenAction.ActionPointCost = 1;
	ShurikenAction.ResourceCosts.ManaCost = 2;
	// The runtime must normalize a misconfigured equipped throwable to one
	// consumed unit in both the catalogue and the authoritative transaction.
	ShurikenAction.ResourceCosts.SourceItemQuantityCost = 0;
	ShurikenAction.CooldownRounds = 1;
	ShurikenAction.RangeCells = 1;
	ShurikenAction.OffensiveProfile.AttackId = ShurikenAction.ActionId;
	ShurikenAction.OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Physical;
	ShurikenAction.OffensiveProfile.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
	ShurikenAction.OffensiveProfile.AttackDefinition.MinDamage = 2;
	ShurikenAction.OffensiveProfile.AttackDefinition.MaxDamage = 2;
	ShurikenAction.OffensiveProfile.AttackDefinition.AccuracyBonus = 100;
	ShurikenAction.OffensiveProfile.DamageScalingAttribute = EGridAttackScalingAttribute::None;
	ShurikenAction.OffensiveProfile.RangeCells = 1;
	ShurikenDefinition->CombatActions.Add(ShurikenAction);
	TestTrue(TEXT("The transactional weapon definition is valid"), ShurikenDefinition->IsValidDefinition());
	TestTrue(TEXT("The transactional weapon is registered"), Inventory->RegisterItemDefinition(ShurikenDefinition));

	FGridItemInstance EquippedShuriken;
	EquippedShuriken.RuntimeObjectId = FGuid(12, 12, 1, 1);
	EquippedShuriken.ItemDefinitionId = ShurikenDefinition->ItemDefinitionId;
	EquippedShuriken.DisplayName = ShurikenDefinition->DisplayName;
	EquippedShuriken.Quantity = 2;
	EquippedShuriken.OwnerType = EGridItemOwnerType::EquipmentSlot;
	EquippedShuriken.OwnerGuid = Fixture.CharacterIds[0];
	EquippedShuriken.OwnerCharacterIndex = 0;
	EquippedShuriken.EquipmentSlot = EGridEquipmentSlot::MainHand;
	Inventory->PartyInventoryState.ActiveEquipment[0].MainHand = EquippedShuriken;

	UGridPlayerAttackPresentationComponent* Presentation = Fixture.Runtime->GetPlayerAttackPresentationComponent();
	if (!TestNotNull(TEXT("The presentation component exists"), Presentation))
	{
		return false;
	}
	Presentation->bNativeThrownItemLaunchEnabled = false;
	Presentation->bNativeAudioPlaybackEnabled = false;
	Presentation->bNativeVFXSpawnEnabled = false;
	Presentation->bNativeFeedbackEnabled = false;
	TestTrue(TEXT("The presentation observes the authoritative manager"), Presentation->InitializePresentation(TurnManager));

	FGridCombatActionRequestResult Accepted;
	TestTrue(TEXT("The equipment action is accepted"),
		TurnManager->RequestCharacterCombatAction(0, ShurikenAction.ActionId, EGridCombatActionSourcePolicy::Equipment, ShurikenDefinition->ItemDefinitionId,
			EGridEquipmentSlot::MainHand, Accepted));
	TestEqual(TEXT("The catalogue normalizes the throwable source cost"), Accepted.Action.CurrentSourceItemQuantityCost, 1);
	TestTrue(TEXT("The accepted attack is structurally valid"), Accepted.AttackRequest.IsValid());
	TestNotNull(TEXT("Gameplay prepares a recoverable projectile"), Accepted.AttackRequest.PreparedThrownItemActor.Get());
	TestEqual(TEXT("Exactly one equipped source unit is paid"), Inventory->PartyInventoryState.ActiveEquipment[0].MainHand.Quantity, 1);
	TestEqual(TEXT("The equipment mana cost is paid"), Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentMana, 6);
	FGridPlayerCharacterTurnState TurnState;
	TurnManager->GetPlayerCharacterTurnState(0, TurnState);
	TestEqual(TEXT("The equipment AP cost is paid"), TurnState.RemainingActionPoints, 3);
	TestEqual(TEXT("Presentation does not perform a second extraction"), Inventory->PartyInventoryState.ActiveEquipment[0].MainHand.Quantity, 1);
	TestEqual(TEXT("Presentation observes exactly one committed launch"), Presentation->ThrownItemLaunchRequestCount, 1);
	TestEqual(TEXT("The committed launch does not depend on the visual fallback"), Presentation->ThrownItemLaunchStartedCount, 1);

	TArray<FGridAvailableCombatAction> Actions;
	TurnManager->GetAvailableCombatActions(0, Actions);
	const FGridAvailableCombatAction* CoolingAction = Actions.FindByPredicate(
		[&ShurikenAction](const FGridAvailableCombatAction& Candidate)
		{
			return Candidate.Definition.ActionId == ShurikenAction.ActionId;
		});
	TestTrue(TEXT("The used action remains in the catalogue"), CoolingAction != nullptr);
	TestTrue(
		TEXT("The used action enters cooldown"), CoolingAction && CoolingAction->AvailabilityReason == EGridCombatActionAvailabilityReason::CooldownActive);
	TestEqual(TEXT("Cooldown belongs only to the acting character"),
		TurnManager->GetRemainingCombatActionCooldown(Fixture.CharacterIds[1], ShurikenAction.ActionId), 0);

	TurnManager->RoundNumber = 2;
	TurnManager->GetAvailableCombatActions(0, Actions);
	CoolingAction = Actions.FindByPredicate(
		[&ShurikenAction](const FGridAvailableCombatAction& Candidate)
		{
			return Candidate.Definition.ActionId == ShurikenAction.ActionId;
		});
	TestTrue(TEXT("One complete following round remains blocked"),
		CoolingAction && !CoolingAction->bEnabled && CoolingAction->AvailabilityReason == EGridCombatActionAvailabilityReason::CooldownActive);

	TurnManager->RoundNumber = 3;
	TurnManager->GetAvailableCombatActions(0, Actions);
	CoolingAction = Actions.FindByPredicate(
		[&ShurikenAction](const FGridAvailableCombatAction& Candidate)
		{
			return Candidate.Definition.ActionId == ShurikenAction.ActionId;
		});
	TestTrue(TEXT("The action returns after one complete round"), CoolingAction && CoolingAction->bEnabled);
	ShurikenDefinition->ThrowSpeed = 0.0f;
	const int32 ManaBeforeRefusal = Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentMana;
	const int32 QuantityBeforeRefusal = Inventory->PartyInventoryState.ActiveEquipment[0].MainHand.Quantity;
	TurnManager->GetPlayerCharacterTurnState(0, TurnState);
	const int32 ActionPointsBeforeRefusal = TurnState.RemainingActionPoints;
	const int32 MonsterHealthBeforeRefusal = Fixture.Monster->CurrentHealth;
	FGridCombatActionRequestResult Rejected;
	TestFalse(TEXT("An unavailable projectile source is rejected"),
		TurnManager->RequestCharacterCombatAction(0, ShurikenAction.ActionId, EGridCombatActionSourcePolicy::Equipment, ShurikenDefinition->ItemDefinitionId,
			EGridEquipmentSlot::MainHand, Rejected));
	TurnManager->GetPlayerCharacterTurnState(0, TurnState);
	TestEqual(TEXT("A rejected action keeps AP"), TurnState.RemainingActionPoints, ActionPointsBeforeRefusal);
	TestEqual(TEXT("A rejected action keeps mana"), Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentMana, ManaBeforeRefusal);
	TestEqual(TEXT("A rejected action keeps its equipment source"), Inventory->PartyInventoryState.ActiveEquipment[0].MainHand.Quantity, QuantityBeforeRefusal);
	TestEqual(TEXT("A rejected action cannot damage its target"), Fixture.Monster->CurrentHealth, MonsterHealthBeforeRefusal);

	TurnManager->FinishCombat(EGridCombatPhase::Victory);
	TestFalse(TEXT("The real combat finish deactivates combat"), TurnManager->bCombatActive);
	TestEqual(
		TEXT("Combat finish clears transient cooldowns"), TurnManager->GetRemainingCombatActionCooldown(Fixture.CharacterIds[0], ShurikenAction.ActionId), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridHOTBAR011ShortcutQuantityBadgeTest, "Grimrock.Hotbar.HOTBAR01_1.ShortcutQuantityBadge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridHOTBAR011ShortcutQuantityBadgeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridCombatHudActionWidget* Widget = NewObject<UGridCombatHudActionWidget>();
	if (!TestNotNull(TEXT("The quantity-badge widget is created"), Widget))
	{
		return false;
	}
	Widget->Text_Quantity = NewObject<UTextBlock>(Widget);
	if (!TestNotNull(TEXT("The quantity text exists"), Widget->Text_Quantity.Get()))
	{
		return false;
	}

	FGridCombatHudActionView View;
	View.HotbarSlotIndex = 2;
	View.bHasBinding = true;
	View.bResolved = true;
	View.Binding.SlotIndex = 2;
	View.Binding.ActionId = TEXT("ThrowItem_Stone_HOTBAR011");
	View.Binding.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
	View.Binding.SourceDefinitionId = TEXT("Stone_HOTBAR011");
	View.Action.Definition.ActionId = View.Binding.ActionId;
	View.Action.Definition.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
	View.Action.SourceDefinitionId = View.Binding.SourceDefinitionId;
	View.Action.CurrentSourceItemQuantityCost = 1;
	View.Action.CurrentSourceItemQuantity = 7;
	View.Action.bEnabled = true;

	Widget->InitializeAction(nullptr, View);
	TestEqual(TEXT("A seven-unit inventory shortcut displays x7"), Widget->Text_Quantity->GetText().ToString(), FString(TEXT("x7")));
	TestEqual(TEXT("The quantity badge is visible"), Widget->Text_Quantity->GetVisibility(), ESlateVisibility::HitTestInvisible);

	View.Action.CurrentSourceItemQuantity = 1;
	Widget->InitializeAction(nullptr, View);
	TestEqual(TEXT("The last inventory unit is explicitly displayed as x1"), Widget->Text_Quantity->GetText().ToString(), FString(TEXT("x1")));

	View.Action.CurrentSourceItemQuantity = 0;
	Widget->InitializeAction(nullptr, View);
	TestEqual(TEXT("Zero inventory quantity hides the badge"), Widget->Text_Quantity->GetVisibility(), ESlateVisibility::Collapsed);

	View.Action.Definition.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
	View.Action.CurrentSourceItemQuantity = 9;
	Widget->InitializeAction(nullptr, View);
	TestEqual(TEXT("Equipment-backed actions do not display a quantity"), Widget->Text_Quantity->GetVisibility(), ESlateVisibility::Collapsed);

	View.Action.Definition.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
	View.Action.CurrentSourceItemQuantity = 4;
	View.Action.CurrentSourceItemQuantityCost = 0;
	Widget->InitializeAction(nullptr, View);
	TestEqual(TEXT("Non-consumable sources do not display a stock badge"), Widget->Text_Quantity->GetVisibility(), ESlateVisibility::Collapsed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT011714OilQuickItemRealPipeline,
	"Grimrock.RPG.ARMEMENT01.17.1.4.QuickItemToWorldProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT011714OilQuickItemRealPipeline::RunTest(const FString&)
{
	// Réutiliser la vraie fixture de combat avec action ciblée, personnage,
	// inventaire, hotbar, occupant du donjon et TurnManager initialisé.
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("La scène de combat et son gestionnaire sont prêts"), Fixture.IsReady()))
	{
		return false;
	}

	UGridItemDefinitionAsset* OilFlask = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Oil.DA_Item_Flask_Oil"));
	if (!TestNotNull(TEXT("Véritable DataAsset de la flasque d'huile"), OilFlask))
	{
		return false;
	}
	FGridCombatActionDefinition RealAction;
	if (!TestTrue(TEXT("La flasque définit une action QuickItem réelle"),
			OilFlask->BuildQuickItemCombatActionDefinition(RealAction)))
	{
		return false;
	}
	UGridPartyInventoryComponent* Inventory = Fixture.Party->PartyInventoryComponent.Get();
	if (!TestTrue(TEXT("Définition réelle enregistrée dans l'inventaire"),
			Inventory->RegisterItemDefinition(OilFlask))) return false;

	FGridItemInstance Flask;
	Flask.RuntimeObjectId = FGuid::NewGuid();
	Flask.ItemDefinitionId = OilFlask->ItemDefinitionId;
	Flask.DisplayName = OilFlask->DisplayName;
	Flask.Quantity = 2;
	Flask.OwnerType = EGridItemOwnerType::World;
	if (!TestTrue(TEXT("Deux flasques ajoutées au sac du personnage"),
			Inventory->AddItemToCharacterInventory(0, Flask))) return false;
	TestEqual(TEXT("Stock avant utilisation"),
		Inventory->CountItemDefinitionInCharacterInventory(0, Flask.ItemDefinitionId), 2);

	// Tester le vrai raccourci utilisateur, et pas seulement le TurnManager :
	// une flasque utilitaire ne doit pas être redirigée vers le jet physique.
	if (!TestTrue(TEXT("La flasque s'assigne au raccourci 2"),
			Inventory->SetCharacterCombatHotbarBindingFromItem(0, 1, Flask, EGridEquipmentSlot::None)))
	{
		return false;
	}
	FGridCombatHotbarBinding Binding;
	if (!TestTrue(TEXT("Le raccourci d'huile peut être relu"),
			Inventory->GetCharacterCombatHotbarBinding(0, 1, Binding)))
	{
		return false;
	}
	TestFalse(TEXT("La flasque utilise une action de surface, pas le jet physique"),
		Binding.IsPhysicalThrowItemBinding());
	TestEqual(TEXT("L'identité de l'action reste celle du DataAsset"),
		Binding.ActionId, RealAction.ActionId);
	Fixture.Hud->RefreshFromSources();
	if (!TestTrue(TEXT("Le raccourci HUD est visible et activé"),
			Fixture.Hud->View.Actions.IsValidIndex(1) &&
			Fixture.Hud->View.Actions[1].bResolved &&
			Fixture.Hud->View.Actions[1].Action.bEnabled))
	{
		return false;
	}

	FGridAvailableCombatAction Catalogued;
	if (!TestTrue(TEXT("La vraie action d'huile apparaît dans le catalogue de combat"),
			FindCatalogAction(Fixture.TurnManager, 0, RealAction.ActionId, Catalogued)))
	{
		return false;
	}
	if (!TestTrue(TEXT("L'action de flasque est autorisée durant ce tour"), Catalogued.bEnabled))
	{
		return false;
	}
	TestTrue(TEXT("Action de surface et non attaque physique"),
		!Catalogued.Definition.SurfaceEffects.IsEmpty() &&
		Catalogued.Definition.TargetingPolicy == EGridCombatTargetingPolicy::Area);

	const FIntPoint TargetCell(1, 2);
	FGridCombatActionRequestResult AimResult;
	UGridEventFeedSubsystem* FeedBefore = Fixture.TestWorld.World
		? Fixture.TestWorld.World->GetSubsystem<UGridEventFeedSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("Flux d'événements disponible avant ciblage"), FeedBefore)) return false;
	if (!TestTrue(TEXT("Le clic sur le raccourci active le ciblage"),
			Fixture.Hud->RequestHotbarSlot(1, AimResult)))
	{
		return false;
	}
	TestEqual(TEXT("Aucun splash pendant le ciblage"), Fixture.Runtime->GetOilSplashPresentationCount(), 0);
	TestFalse(TEXT("Aucun événement d'huile pendant le ciblage"),
		FeedBefore->GetEntries().ContainsByPredicate([](const FGridHudEventEntry& Entry)
		{
			return Entry.EventCode == FName(TEXT("Combat.Surface.OilSpread"));
		}));
	TestTrue(TEXT("L'étalement du splash est compris entre son rayon initial et final"),
		AGridLevelRuntimeActor::GetOilSplashSpreadScale(.5f) > .18f &&
		AGridLevelRuntimeActor::GetOilSplashSpreadScale(.5f) < 1.f);
	FGridCombatActionRequestResult Result;
	if (!TestTrue(TEXT("Le clic de confirmation déclenche la vraie action de flasque"),
			Fixture.Hud->ConfirmCombatActionTarget(TargetCell, Result)))
	{
		return false;
	}
	const FGridCombatSurfaceState* Oil = Fixture.Runtime->FindCombatSurfaceAtCell(TargetCell.X, TargetCell.Y);
	TestTrue(TEXT("L'effet d'huile RPG03.6 est présent sur la cellule visée"),
		Oil && Oil->IsValid() && Oil->SurfaceType == EGridCombatSurfaceType::Oil);
	TestEqual(TEXT("No visible oil puddle before the bottle lands"),
		Fixture.Runtime->GetActiveOilDecalCount(), 0);
	TestEqual(TEXT("Une flasque a été consommée"),
		Inventory->CountItemDefinitionInCharacterInventory(0, Flask.ItemDefinitionId), 1);
	// FLASK01: in-flight bottle defers the transient splash until physical impact.
	TestEqual(TEXT("A real oil bottle must fly before the splash"),
		Fixture.Runtime->GetActiveOilFlaskProjectileCount(), 1);
	if (Fixture.Runtime->GetActiveOilFlaskProjectileCount() > 0)
	{
		TestEqual(TEXT("Pas de splash avant impact"), Fixture.Runtime->GetOilSplashPresentationCount(), 0);
		for (int32 Step = 0; Step < 180 && Fixture.Runtime->GetActiveOilFlaskProjectileCount() > 0; ++Step)
		{
			Fixture.Runtime->UpdateOilFlaskFlight();
		}
	}
	TestEqual(TEXT("Une seule animation splash après impact"),
		Fixture.Runtime->GetOilSplashPresentationCount(), 1);
	TestEqual(TEXT("Splash temporaire actif après impact"),
		Fixture.Runtime->GetActiveOilSplashCount(), 1);
	TestTrue(TEXT("Persistent oil decals appear with the impact"),
		Fixture.Runtime->GetActiveOilDecalCount() > 0 &&
		Fixture.Runtime->GetActiveOilDecal(0) != nullptr);

	UGridEventFeedSubsystem* Feed = Fixture.TestWorld.World
		? Fixture.TestWorld.World->GetSubsystem<UGridEventFeedSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("Flux d'événements du monde"), Feed)) return false;
	int32 OilEntries = 0;
	for (const FGridHudEventEntry& Event : Feed->GetEntries())
	{
		if (Event.EventCode == FName(TEXT("Combat.Surface.OilSpread")))
		{
			++OilEntries;
			TestEqual(TEXT("Une entrée dans le filtre Combat"), Event.Category, EGridHudEventCategory::Combat);
			TestFalse(TEXT("Message d'huile non vide"), Event.Message.IsEmpty());
		}
	}
	TestEqual(TEXT("Une seule entrée par flasque, non une entrée par cellule"), OilEntries, 1);
	Fixture.Runtime->ClearCombatOilSplash();
	TestEqual(TEXT("Splash nettoyé sans supprimer la flaque persistante"),
		Fixture.Runtime->GetActiveOilSplashCount(), 0);
	TestTrue(TEXT("Le décalque durable reste présent après le splash"),
		Fixture.Runtime->GetActiveOilDecalCount() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT011717OilExplorationRealPipeline,
	"Grimrock.RPG.ARMEMENT01.17.1.7.ExplorationOilHotbar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT011717OilExplorationRealPipeline::RunTest(const FString&)
{
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("Real dungeon, party, inventory, HUD and TurnManager"), Fixture.IsReady()))
	{
		return false;
	}
	UGridItemDefinitionAsset* OilItem = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Oil.DA_Item_Flask_Oil"));
	if (!TestNotNull(TEXT("Authored oil flask"), OilItem)) return false;
	UGridPartyInventoryComponent* Inventory = Fixture.Party->PartyInventoryComponent.Get();
	if (!TestTrue(TEXT("Register real oil definition"), Inventory->RegisterItemDefinition(OilItem))) return false;
	FGridCombatActionDefinition Definition;
	if (!TestTrue(TEXT("Real oil action"), OilItem->BuildQuickItemCombatActionDefinition(Definition))) return false;
	TestTrue(TEXT("Only the authored oil action is admitted"), UGridTurnManagerComponent::IsExplorationOilSlickAction(Definition));
	FGridCombatActionDefinition Altered = Definition;
	Altered.SurfaceEffects.Reset();
	TestFalse(TEXT("A different surface payload is never admitted"),
		UGridTurnManagerComponent::IsExplorationOilSlickAction(Altered));

	FGridItemInstance Flask;
	Flask.RuntimeObjectId = FGuid::NewGuid();
	Flask.ItemDefinitionId = OilItem->ItemDefinitionId;
	Flask.DisplayName = OilItem->DisplayName;
	Flask.Quantity = 2;
	Flask.OwnerType = EGridItemOwnerType::World;
	if (!TestTrue(TEXT("Add two flasks"), Inventory->AddItemToCharacterInventory(0, Flask))) return false;
	if (!TestTrue(TEXT("Bind oil to hotbar"), Inventory->SetCharacterCombatHotbarBindingFromItem(
		0, 1, Flask, EGridEquipmentSlot::None))) return false;

	Fixture.TurnManager->bCombatActive = false;
	Fixture.TurnManager->CurrentPhase = EGridCombatPhase::Exploration;
	const int32 RoundBefore = Fixture.TurnManager->RoundNumber;
	const int32 ActionPointsBefore = Fixture.TurnManager->PlayerCharacterTurnStates[0].RemainingActionPoints;
	Fixture.Hud->RefreshFromSources();
	if (!TestTrue(TEXT("Oil hotbar resolves and enables outside combat"),
		Fixture.Hud->View.Actions.IsValidIndex(1) && Fixture.Hud->View.Actions[1].bResolved &&
		Fixture.Hud->View.Actions[1].Action.bEnabled)) return false;
	TestEqual(TEXT("No exploration AP cost"), Fixture.Hud->View.Actions[1].Action.CurrentActionPointCost, 0);
	TestEqual(TEXT("Correct authored action identity"), Fixture.Hud->View.Actions[1].Action.Definition.ActionId,
		FName(TEXT("Action_Alchemist_OilSlick")));
	FGridCombatActionRequestResult Aim;
	if (!TestTrue(TEXT("Enter existing cursor targeting"), Fixture.Hud->RequestHotbarSlot(1, Aim))) return false;
	TestTrue(TEXT("Targeting pending"), Fixture.Hud->IsCombatActionTargetingActive());
	TestEqual(TEXT("Aiming does not consume"), Inventory->CountItemDefinitionInCharacterInventory(0, Flask.ItemDefinitionId), 2);
	TestEqual(TEXT("Aiming does not play splash"), Fixture.Runtime->GetOilSplashPresentationCount(), 0);

	FGridCombatActionRequestResult Invalid;
	TestFalse(TEXT("Out-of-grid target rejected"), Fixture.Hud->ConfirmCombatActionTarget(FIntPoint(100, 100), Invalid));
	TestEqual(TEXT("Invalid target is free"), Inventory->CountItemDefinitionInCharacterInventory(0, Flask.ItemDefinitionId), 2);
	TestTrue(TEXT("Invalid target keeps targeting"), Fixture.Hud->IsCombatActionTargetingActive());

	const FIntPoint Target(1, 2);
	if (!TestTrue(TEXT("Preview for existing area succeeds"),
		Fixture.Hud->UpdateCombatActionTargetingPreview(Target))) return false;
	TestTrue(TEXT("Preview marked valid"), Fixture.Hud->TargetingPreview.bValid);
	TestEqual(TEXT("Three walkable cells in radius-one area"), Fixture.Hud->TargetingPreview.AffectedCells.Num(), 4);
	FGridCombatActionRequestResult Confirmed;
	if (!TestTrue(TEXT("Oil use resolves outside combat"),
		Fixture.Hud->ConfirmCombatActionTarget(Target, Confirmed))) return false;
	TestTrue(TEXT("Accepted result"), Confirmed.bAccepted);
	TestFalse(TEXT("Cursor targeting closes"), Fixture.Hud->IsCombatActionTargetingActive());
	TestEqual(TEXT("Exactly one flask spent"),
		Inventory->CountItemDefinitionInCharacterInventory(0, Flask.ItemDefinitionId), 1);
	TestEqual(TEXT("No combat turn started"), Fixture.TurnManager->bCombatActive, false);
	TestEqual(TEXT("Round unaffected"), Fixture.TurnManager->RoundNumber, RoundBefore);
	TestEqual(TEXT("AP unaffected"),
		Fixture.TurnManager->PlayerCharacterTurnStates[0].RemainingActionPoints, ActionPointsBefore);
	const FGridCombatSurfaceState* Applied = Fixture.Runtime->FindCombatSurfaceAtCell(Target.X, Target.Y);
	TestTrue(TEXT("RPG03.6 authoritative oil surface present"),
		Applied && Applied->SurfaceType == EGridCombatSurfaceType::Oil && Applied->RemainingRounds == 4);
	TestEqual(TEXT("Exploration oil flask must be visible in flight"),
		Fixture.Runtime->GetActiveOilFlaskProjectileCount(), 1);
	if (Fixture.Runtime->GetActiveOilFlaskProjectileCount() > 0)
	{
		TestEqual(TEXT("No splash in flight"), Fixture.Runtime->GetOilSplashPresentationCount(), 0);
		for (int32 Step = 0; Step < 180 && Fixture.Runtime->GetActiveOilFlaskProjectileCount() > 0; ++Step)
		{
			Fixture.Runtime->UpdateOilFlaskFlight();
		}
	}
	TestEqual(TEXT("One visible splash after impact"), Fixture.Runtime->GetOilSplashPresentationCount(), 1);
	TestTrue(TEXT("Oil decals on ground"), Fixture.Runtime->GetActiveOilDecalCount() > 0);

	UGridEventFeedSubsystem* Feed = Fixture.TestWorld.World->GetSubsystem<UGridEventFeedSubsystem>();
	if (!TestNotNull(TEXT("Event feed"), Feed)) return false;
	int32 Events = 0;
	for (const FGridHudEventEntry& Entry : Feed->GetEntries())
	{
		if (Entry.EventCode == FName(TEXT("Combat.Surface.OilSpread"))) ++Events;
	}
	TestEqual(TEXT("One event on confirmation"), Events, 1);
	FString OwnershipError;
	TestTrue(TEXT("Item ownership invariant"), Inventory->ValidateInventoryOwnership(OwnershipError));

	// Cancellation and a second target prove that no hidden combat turn is needed.
	FGridCombatActionRequestResult SecondAim;
	TestTrue(TEXT("Second flask can be aimed"), Fixture.Hud->RequestHotbarSlot(1, SecondAim));
	Fixture.Hud->CancelCombatActionTargeting();
	TestEqual(TEXT("Cancellation consumes nothing"),
		Inventory->CountItemDefinitionInCharacterInventory(0, Flask.ItemDefinitionId), 1);
	Fixture.Hud->RequestHotbarSlot(1, SecondAim);
	FGridCombatActionRequestResult SecondUse;
	TestTrue(TEXT("Second flask can be used"), Fixture.Hud->ConfirmCombatActionTarget(FIntPoint(1, 2), SecondUse));
	TestEqual(TEXT("Last flask spent"),
		Inventory->CountItemDefinitionInCharacterInventory(0, Flask.ItemDefinitionId), 0);
	TestFalse(TEXT("Empty stack removes its hotbar shortcut"),
		Fixture.Hud->View.Actions[1].bHasBinding);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01172BCombatCatalyst,
	"Grimrock.RPG.ARMEMENT01.17.2B.CatalystCombatHotbar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01172BCombatCatalyst::RunTest(const FString&)
{
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("Real combat fixture ready"), Fixture.IsReady())) return false;
	UGridItemDefinitionAsset* Catalyst = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_SurfaceCatalyst_01.DA_Item_SurfaceCatalyst_01"));
	if (!TestNotNull(TEXT("Native N1 Catalyst"), Catalyst)) return false;
	FGridCombatActionDefinition Definition;
	if (!TestTrue(TEXT("Catalyst QuickItem action exists"), Catalyst->BuildQuickItemCombatActionDefinition(Definition))) return false;
	TestEqual(TEXT("Fire reaction action"), Definition.SurfaceInteraction, EGridCombatSurfaceInteraction::Fire);
	TestEqual(TEXT("Square-area targeting"), Definition.TargetingPolicy, EGridCombatTargetingPolicy::Area);
	TestEqual(TEXT("Square radius one"), Definition.AreaRadiusCells, 1);
	TestTrue(TEXT("No-reaction targeting is an approved outcome"), Definition.bAllowNoSurfaceReaction);

	UGridPartyInventoryComponent* Inventory = Fixture.Party->PartyInventoryComponent.Get();
	if (!TestTrue(TEXT("Register exact catalyst definition"), Inventory->RegisterItemDefinition(Catalyst))) return false;
	FGridItemInstance Item;
	Item.RuntimeObjectId = FGuid::NewGuid();
	Item.ItemDefinitionId = Catalyst->ItemDefinitionId;
	Item.DisplayName = Catalyst->DisplayName;
	Item.OwnerType = EGridItemOwnerType::World;
	Item.Quantity = 2;
	if (!TestTrue(TEXT("Two catalyst units in inventory"), Inventory->AddItemToCharacterInventory(0, Item))) return false;
	if (!TestTrue(TEXT("Bind catalyst to hotbar"), Inventory->SetCharacterCombatHotbarBindingFromItem(
		0, 1, Item, EGridEquipmentSlot::None))) return false;
	Fixture.Hud->RefreshFromSources();

	const FIntPoint Target(1, 2);
	FGridCombatSurfaceEffectProfile Oil;
	Oil.SurfaceType = EGridCombatSurfaceType::Oil;
	Oil.DurationRounds = 4;
	Oil.TraversalCostModifier = 1;
	const FGridResolvedCombatModifiers Modifiers;
	if (!TestTrue(TEXT("Prepare existing RPG03.6 oil for catalyst"),
		Fixture.Runtime->ApplyCombatSurfaceAtCell(Target.X, Target.Y, Oil, Fixture.CharacterIds[0],
			FName(TEXT("Action_Alchemist_OilSlick")), Modifiers))) return false;
	FGridCombatSurfaceEffectProfile Poison;
	Poison.SurfaceType = EGridCombatSurfaceType::Poison;
	Poison.DurationRounds = 4;
	if (!TestTrue(TEXT("Prepare poison inside catalyst area"),
		Fixture.Runtime->ApplyCombatSurfaceAtCell(2, 2, Poison, Fixture.CharacterIds[0],
			FName(TEXT("PoisonTest")), Modifiers))) return false;
	FGridCombatSurfaceEffectProfile Ice;
	Ice.SurfaceType = EGridCombatSurfaceType::Ice;
	Ice.DurationRounds = 3;
	if (!TestTrue(TEXT("Prepare ice inside catalyst area"),
		Fixture.Runtime->ApplyCombatSurfaceAtCell(0, 2, Ice, Fixture.CharacterIds[0],
			FName(TEXT("IceTest")), Modifiers))) return false;

	if (!TestTrue(TEXT("Combat hotbar catalyst action resolves"), Fixture.Hud->View.Actions.IsValidIndex(1) &&
		Fixture.Hud->View.Actions[1].bResolved && Fixture.Hud->View.Actions[1].Action.bEnabled))
		return false;
	TestEqual(TEXT("Combat catalyst costs two AP"), Fixture.Hud->View.Actions[1].Action.CurrentActionPointCost, 2);
	const int32 ActionPointsBefore = Fixture.TurnManager->PlayerCharacterTurnStates[0].RemainingActionPoints;
	FGridCombatActionRequestResult Aim;
	if (!TestTrue(TEXT("Click enters canonical targeting"), Fixture.Hud->RequestHotbarSlot(1, Aim))) return false;
	TestEqual(TEXT("Aiming does not spend item"),
		Inventory->CountItemDefinitionInCharacterInventory(0, Catalyst->ItemDefinitionId), 2);
	FGridCombatActionRequestResult Invalid;
	TestFalse(TEXT("Invalid target rejected"), Fixture.Hud->ConfirmCombatActionTarget(FIntPoint(100,100), Invalid));
	TestEqual(TEXT("Rejection spends nothing"),
		Inventory->CountItemDefinitionInCharacterInventory(0, Catalyst->ItemDefinitionId), 2);

	FGridCombatActionTargetingPreview ZonePreview;
	if (!TestTrue(TEXT("Preview area around edge cell"),
		Fixture.TurnManager->BuildCombatActionTargetingPreview(0, Definition.ActionId,
			EGridCombatActionSourcePolicy::QuickItem, Catalyst->ItemDefinitionId, EGridEquipmentSlot::None,
			Target, ZonePreview))) return false;
	TestEqual(TEXT("Edge-clipped 3x3 has six traversable cells"), ZonePreview.AffectedCells.Num(), 6);
	FGridCombatActionRequestResult Confirmed;
	if (!TestTrue(TEXT("Apply canonical Fire reaction at valid cell"),
		Fixture.Hud->ConfirmCombatActionTarget(Target, Confirmed))) return false;
	TestTrue(TEXT("Combat action accepted"), Confirmed.bAccepted);
	const FGridCombatSurfaceState* Surface = Fixture.Runtime->FindCombatSurfaceAtCell(Target.X, Target.Y);
	TestTrue(TEXT("Oil changed to canonical Fire, no new surface type invented"),
		Surface && Surface->SurfaceType == EGridCombatSurfaceType::Fire);
	const FGridCombatSurfaceState* PoisonOutput = Fixture.Runtime->FindCombatSurfaceAtCell(2, 2);
	TestTrue(TEXT("Poison reacted within same catalyst throw"),
		PoisonOutput && PoisonOutput->SurfaceType == EGridCombatSurfaceType::Fire);
	const FGridCombatSurfaceState* IceOutput = Fixture.Runtime->FindCombatSurfaceAtCell(0, 2);
	TestTrue(TEXT("Ice melted within same catalyst throw"),
		IceOutput && IceOutput->SurfaceType == EGridCombatSurfaceType::Water);
	TestNull(TEXT("Dry part of catalyst area stays dry"),
		Fixture.Runtime->FindCombatSurfaceAtCell(0, 1));
	TestEqual(TEXT("Catalysed Fire has two periodic Fire damage per round"),
		Surface ? Surface->PeriodicDamagePerRound : -1, 2);
	const int32 MonsterHealthBeforeFireTick = Fixture.Monster->CurrentHealth;
	Fixture.TurnManager->ResolveCombatSurfaceRound();
	TestTrue(TEXT("Monster standing in ignited oil takes real periodic damage"),
		Fixture.Monster->CurrentHealth < MonsterHealthBeforeFireTick);
	TestEqual(TEXT("Exactly one catalyst spent"),
		Inventory->CountItemDefinitionInCharacterInventory(0, Catalyst->ItemDefinitionId), 1);
	TestEqual(TEXT("Exactly two AP spent"),
		Fixture.TurnManager->PlayerCharacterTurnStates[0].RemainingActionPoints, ActionPointsBefore - 2);
	TestEqual(TEXT("No oil flask splash for catalyst"), Fixture.Runtime->GetOilSplashPresentationCount(), 0);
	TestFalse(TEXT("Catalyst doesn't grant N2+ talent"), Definition.Requirements.Num() > 0);

	// No eligible reaction remains in the second clipped area: valid Fizzle, still paid.
	FGridCombatSurfaceEffectProfile Water;
	Water.SurfaceType = EGridCombatSurfaceType::Water;
	Water.DurationRounds = 3;
	if (!TestTrue(TEXT("Prepare non-reactive water"),
		Fixture.Runtime->ApplyCombatSurfaceAtCell(Target.X, Target.Y, Water, Fixture.CharacterIds[0],
			FName(TEXT("WaterTest")), Modifiers))) return false;
	FGridCombatActionRequestResult Aim2;
	if (TestTrue(TEXT("Remaining unit can enter targeting"), Fixture.Hud->RequestHotbarSlot(1, Aim2)))
	{
		FGridCombatActionRequestResult WaterResult;
		TestTrue(TEXT("Non-reactive water and dry ground still accept catalyst"), Fixture.Hud->ConfirmCombatActionTarget(Target, WaterResult));
		TestEqual(TEXT("One catalyst consumed even when no cell reacts"),
			Inventory->CountItemDefinitionInCharacterInventory(0, Catalyst->ItemDefinitionId), 0);
		TestEqual(TEXT("Two AP consumed for valid Fizzle"),
			Fixture.TurnManager->PlayerCharacterTurnStates[0].RemainingActionPoints, ActionPointsBefore - 4);
		TestTrue(TEXT("Water remains Water"),
			Fixture.Runtime->FindCombatSurfaceAtCell(Target.X, Target.Y) &&
			Fixture.Runtime->FindCombatSurfaceAtCell(Target.X, Target.Y)->SurfaceType == EGridCombatSurfaceType::Water);
		Fixture.Hud->CancelCombatActionTargeting();
	}
	FString Ownership;
	TestTrue(TEXT("No orphan inventory item"), Inventory->ValidateInventoryOwnership(Ownership));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01173BExplorationCatalyst,
	"Grimrock.RPG.ARMEMENT01.17.3B.ExplorationCatalyst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01173BExplorationCatalyst::RunTest(const FString&)
{
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("Real-world exploration fixture"), Fixture.IsReady())) return false;
	UGridItemDefinitionAsset* Catalyst = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_SurfaceCatalyst_01.DA_Item_SurfaceCatalyst_01"));
	if (!TestNotNull(TEXT("Artist-owned N1 catalyst"), Catalyst)) return false;
	UGridPartyInventoryComponent* Inventory = Fixture.Party->PartyInventoryComponent.Get();
	if (!TestTrue(TEXT("Register catalyst"), Inventory->RegisterItemDefinition(Catalyst))) return false;
	FGridItemInstance Item;
	Item.RuntimeObjectId = FGuid::NewGuid();
	Item.ItemDefinitionId = Catalyst->ItemDefinitionId;
	Item.DisplayName = Catalyst->DisplayName;
	Item.OwnerType = EGridItemOwnerType::World;
	Item.Quantity = 2;
	if (!TestTrue(TEXT("Own exactly two bottles"), Inventory->AddItemToCharacterInventory(0, Item)) ||
		!TestTrue(TEXT("Bind exploration catalyst"), Inventory->SetCharacterCombatHotbarBindingFromItem(
			0, 1, Item, EGridEquipmentSlot::None))) return false;

	// All authority executes without combat rounds or action points.
	Fixture.TurnManager->bCombatActive = false;
	FGridCombatSurfaceEffectProfile Poison;
	Poison.SurfaceType = EGridCombatSurfaceType::Poison;
	Poison.DurationRounds = 4;
	FGridCombatSurfaceEffectProfile Ice;
	Ice.SurfaceType = EGridCombatSurfaceType::Ice;
	Ice.DurationRounds = 3;
	const FGridResolvedCombatModifiers NoModifiers;
	if (!TestTrue(TEXT("Poison under monster"), Fixture.Runtime->ApplyCombatSurfaceAtCell(
			1, 2, Poison, Fixture.CharacterIds[0], TEXT("PoisonTest"), NoModifiers)) ||
		!TestTrue(TEXT("Ice inside catalyst area"), Fixture.Runtime->ApplyCombatSurfaceAtCell(
			2, 2, Ice, Fixture.CharacterIds[0], TEXT("IceTest"), NoModifiers))) return false;
	Fixture.Hud->RefreshFromSources();
	if (!TestTrue(TEXT("Exploration action available"), Fixture.Hud->View.Actions.IsValidIndex(1) &&
			Fixture.Hud->View.Actions[1].bResolved && Fixture.Hud->View.Actions[1].Action.bEnabled)) return false;
	TestEqual(TEXT("No AP exploration cost"), Fixture.Hud->View.Actions[1].Action.CurrentActionPointCost, 0);
	const int32 BeforeAP = Fixture.TurnManager->PlayerCharacterTurnStates[0].RemainingActionPoints;
	const int32 MonsterBefore = Fixture.Monster->CurrentHealth;
	FGridCombatActionRequestResult Begin;
	if (!TestTrue(TEXT("Click begins exploration targeting"), Fixture.Hud->RequestHotbarSlot(1, Begin))) return false;
	FGridCombatActionRequestResult Rejected;
	TestFalse(TEXT("Out of bounds rejected"), Fixture.Hud->ConfirmCombatActionTarget(FIntPoint(80, 80), Rejected));
	TestEqual(TEXT("Invalid preview does not consume bottle"),
		Inventory->CountItemDefinitionInCharacterInventory(0, Catalyst->ItemDefinitionId), 2);

	FGridCombatActionRequestResult Applied;
	if (!TestTrue(TEXT("Catalyst reacts outside combat"), Fixture.Hud->ConfirmCombatActionTarget(FIntPoint(1, 2), Applied))) return false;
	TestTrue(TEXT("Exploration result accepted"), Applied.bAccepted);
	TestEqual(TEXT("Exactly one bottle consumed"),
		Inventory->CountItemDefinitionInCharacterInventory(0, Catalyst->ItemDefinitionId), 1);
	TestEqual(TEXT("Exploration spends no AP"),
		Fixture.TurnManager->PlayerCharacterTurnStates[0].RemainingActionPoints, BeforeAP);
	const FGridCombatSurfaceState* Fire = Fixture.Runtime->FindCombatSurfaceAtCell(1, 2);
	const FGridCombatSurfaceState* Water = Fixture.Runtime->FindCombatSurfaceAtCell(2, 2);
	TestTrue(TEXT("Poison became damaging Fire"), Fire &&
		Fire->SurfaceType == EGridCombatSurfaceType::Fire && Fire->PeriodicDamagePerRound == 2);
	TestTrue(TEXT("Ice melted into Water"), Water && Water->SurfaceType == EGridCombatSurfaceType::Water);
	TestTrue(TEXT("Monster took immediate poisonous explosion damage"),
		Fixture.Monster->CurrentHealth < MonsterBefore);
	TestTrue(TEXT("Fire has exploration seconds"), Fire && FMath::IsNearlyEqual(Fire->RemainingSurfaceSeconds, 24.f));

	// A six-second exploration interval advances Fire and Water, without AP.
	TestEqual(TEXT("No surface expired after six seconds"), Fixture.Runtime->AdvanceExplorationOilTime(6.f), 0);
	Fire = Fixture.Runtime->FindCombatSurfaceAtCell(1, 2);
	TestTrue(TEXT("Fire has three rounds and 18 seconds"), Fire &&
		Fire->RemainingRounds == 3 && FMath::IsNearlyEqual(Fire->RemainingSurfaceSeconds, 18.f));
	Water = Fixture.Runtime->FindCombatSurfaceAtCell(2, 2);
	TestTrue(TEXT("Water also ages out of combat"), Water && Water->RemainingRounds == 2);

	Fixture.Hud->RefreshFromSources();
	FGridCombatActionRequestResult BeginFizzle;
	if (!TestTrue(TEXT("Second bottle can be aimed at nonreactive terrain"),
		Fixture.Hud->RequestHotbarSlot(1, BeginFizzle))) return false;
	FGridCombatActionRequestResult Fizzle;
	if (!TestTrue(TEXT("Fizzle is still a successful use"),
		Fixture.Hud->ConfirmCombatActionTarget(FIntPoint(1, 2), Fizzle))) return false;
	TestTrue(TEXT("Second result accepted"), Fizzle.bAccepted);
	TestEqual(TEXT("Fizzle consumes final bottle"),
		Inventory->CountItemDefinitionInCharacterInventory(0, Catalyst->ItemDefinitionId), 0);
	TestEqual(TEXT("Fizzle consumes zero exploration AP"),
		Fixture.TurnManager->PlayerCharacterTurnStates[0].RemainingActionPoints, BeforeAP);
	FString Ownership;
	TestTrue(TEXT("No orphan after exploration uses"), Inventory->ValidateInventoryOwnership(Ownership));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT02191HandProjection,
	"Grimrock.RPG.ARMEMENT.LOT02.19.1.HandAttackProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENTLOT02191HandProjection::RunTest(const FString&)
{
	const FGuid MainGuid(19,1,1,1), OffGuid(19,1,1,2);
	auto MakeAction = [](FName Id, FName SourceId, const FGuid& RuntimeId, EGridEquipmentSlot Hand)
	{
		FGridAvailableCombatAction Action;
		Action.Definition.ActionId = Id;
		Action.Definition.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
		Action.Definition.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
		Action.SourceDefinitionId = SourceId;
		Action.SourceRuntimeId = RuntimeId;
		Action.SourceEquipmentSlot = Hand;
		Action.bEnabled = true;
		return Action;
	};
	TArray<FGridAvailableCombatAction> Catalog;
	Catalog.Add(MakeAction(TEXT("Attack_One"), TEXT("Weapon_One"), MainGuid, EGridEquipmentSlot::MainHand));
	Catalog.Add(MakeAction(TEXT("Attack_Two"), TEXT("Weapon_Two"), OffGuid, EGridEquipmentSlot::OffHand));
	FGridAvailableCombatAction Unarmed;
	Unarmed.Definition.ActionId = TEXT("Attack_Unarmed");
	Unarmed.Definition.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
	Catalog.Add(Unarmed);

	TArray<FGridCombatHotbarBinding> Bindings;
	Bindings.SetNum(4);
	for (int32 Index = 0; Index < Bindings.Num(); ++Index) Bindings[Index].Reset(Index);
	Bindings[0].ActionId = FGridCombatHotbarBinding::MakePrimaryAttackActionId();
	Bindings[0].SourcePolicy = EGridCombatActionSourcePolicy::Universal;
	auto Project = [&Catalog](const TArray<FGridCombatHotbarBinding>& Source)
	{
		TArray<FGridCombatHudActionView> Views;
		FGridCombatHudViewModelBuilder::BuildHotbarActions(Source, Catalog, Views);
		FGridCombatHudViewModelBuilder::ProjectPrimaryEquippedAttack(Catalog, Views);
		return Views;
	};

	TArray<FGridCombatHudActionView> Views = Project(Bindings);
	if (!TestEqual(TEXT("MainHand alone is the one normal attack"), Views[0].Action.SourceEquipmentSlot, EGridEquipmentSlot::MainHand) ||
		!TestFalse(TEXT("No automatic OffHand button next to primary"), Views[1].bHasBinding) ||
		!TestFalse(TEXT("All other unused shortcuts stay empty"), Views[2].bHasBinding || Views[3].bHasBinding))
		return false;

	// A separate equipped OffHand source exists internally, but the normal
	// combat bar always projects just one basic attack: dual > main > off.
	Catalog.Add(MakeAction(FGridDualWieldPolicy::ActionId(), TEXT("Weapon_One"), MainGuid, EGridEquipmentSlot::MainHand));
	Views = Project(Bindings);
	if (!TestEqual(TEXT("Two weapons make one combined primary shortcut"),
			Views[0].Action.Definition.ActionId, FGridDualWieldPolicy::ActionId()) ||
		!TestFalse(TEXT("No automatic independent MainHand icon"), Views[1].bHasBinding) ||
		!TestFalse(TEXT("No automatic independent OffHand icon"), Views[2].bHasBinding) ||
		!TestTrue(TEXT("No saved binding was synthesized"), Bindings[1].IsEmpty() && Bindings[2].IsEmpty()))
		return false;

	FGridCombatHotbarBinding User;
	User.Reset(1);
	User.ActionId = TEXT("Attack_Unarmed");
	User.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
	Bindings[1] = User;
	Views = Project(Bindings);
	if (!TestTrue(TEXT("An explicitly saved action remains in shortcut 2"), Views[1].bResolved &&
			Views[1].Action.Definition.ActionId == User.ActionId) ||
		!TestFalse(TEXT("A user binding does not generate a substitute OffHand"), Views[2].bHasBinding))
		return false;

	// Existing manually saved weapon bindings are a deliberate customization,
	// not a projected duplicate. They are preserved for SaveGame compatibility.
	Bindings[3].ActionId = Catalog[1].Definition.ActionId;
	Bindings[3].SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
	Bindings[3].SourceDefinitionId = Catalog[1].SourceDefinitionId;
	Bindings[3].PreferredSourceRuntimeId = OffGuid;
	Bindings[3].PreferredEquipmentSlot = EGridEquipmentSlot::OffHand;
	Views = Project(Bindings);
	if (!TestTrue(TEXT("Explicit old OffHand binding is preserved"), Views[3].bResolved &&
			Views[3].Action.SourceRuntimeId == OffGuid) ||
		!TestFalse(TEXT("No automatic extra attack in free slot 3"), Views[2].bHasBinding))
		return false;

	Bindings[1].Reset(1);
	Bindings[3].Reset(3);
	Catalog.RemoveAt(3); // Remove the combined attack.
	Views = Project(Bindings);
	TestEqual(TEXT("Without dual mode the only normal action is MainHand"),
		Views[0].Action.SourceEquipmentSlot, EGridEquipmentSlot::MainHand);
	TestFalse(TEXT("OffHand is not automatically duplicated while MainHand is active"), Views[1].bHasBinding);
	Catalog.RemoveAt(0);
	Views = Project(Bindings);
	TestEqual(TEXT("Without MainHand, OffHand becomes the only normal attack"),
		Views[0].Action.SourceEquipmentSlot, EGridEquipmentSlot::OffHand);
	TestFalse(TEXT("OffHand primary is never duplicated"), Views[1].bHasBinding);
	Catalog.RemoveAt(0);
	Views = Project(Bindings);
	TestEqual(TEXT("Without weapons, the primary action falls back to unarmed"),
		Views[0].Action.Definition.ActionId, FName(TEXT("Attack_Unarmed")));
	TestFalse(TEXT("All other unassigned slots remain empty without weapons"), Views[1].bHasBinding);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT02191ShadowCombat,
	"Grimrock.RPG.ARMEMENT.LOT02.19.1.ShadowN1OffHandCombat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENTLOT02191ShadowCombat::RunTest(const FString&)
{
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("Combat world and HUD are ready"), Fixture.IsReady())) return false;
	UGridItemDefinitionAsset* Dagger = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Dagger_01.DA_Weapon_Dagger_01"));
	UGridItemDefinitionAsset* MainGauche = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_MainGauche_01.DA_Weapon_MainGauche_01"));
	if (!TestNotNull(TEXT("Real ARM001 Dagger"), Dagger) ||
		!TestNotNull(TEXT("Real ARM013 Main-gauche"), MainGauche) ||
		!TestTrue(TEXT("Real ARM013 explicitly allows an OffHand attack"),
			MainGauche->CanProvideAttackFromSlot(EGridEquipmentSlot::OffHand)))
		return false;

	UGridPartyInventoryComponent* Inventory = Fixture.Party->PartyInventoryComponent;
	auto ItemFor = [&Fixture](const UGridItemDefinitionAsset* Definition, const FGuid& Id, EGridEquipmentSlot Slot)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = Id;
		Item.ItemDefinitionId = Definition->ItemDefinitionId;
		Item.DisplayName = Definition->DisplayName;
		Item.ItemLevel = 1;
		Item.Quantity = 1;
		Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Item.OwnerGuid = Fixture.CharacterIds[0];
		Item.OwnerCharacterIndex = 0;
		Item.EquipmentSlot = Slot;
		return Item;
	};
	const FGridItemInstance Main = ItemFor(Dagger, FGuid(19, 1, 2, 1), EGridEquipmentSlot::MainHand);
	const FGridItemInstance Off = ItemFor(MainGauche, FGuid(19, 1, 2, 2), EGridEquipmentSlot::OffHand);
	Inventory->PartyInventoryState.ActiveEquipment[0].MainHand = Main;
	Inventory->PartyInventoryState.ActiveEquipment[0].OffHand = Off;
	if (!TestTrue(TEXT("Register real dagger"), Inventory->RegisterItemDefinition(Dagger)) ||
		!TestTrue(TEXT("Register real Main-gauche"), Inventory->RegisterItemDefinition(MainGauche))) return false;

	Fixture.Hud->RefreshFromSources();
	TArray<FGridAvailableCombatAction> Available;
	Fixture.TurnManager->GetAvailableCombatActions(0, Available);
	const FGridAvailableCombatAction* Secondary = Available.FindByPredicate(
		[&Off](const FGridAvailableCombatAction& A)
		{
			return A.Definition.SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
				A.Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack &&
				A.SourceEquipmentSlot == EGridEquipmentSlot::OffHand &&
				A.SourceRuntimeId == Off.RuntimeObjectId;
		});
	if (!TestNotNull(TEXT("TurnManager retains the actual physical OffHand profile"), Secondary) ||
		!TestEqual(TEXT("Independent underlying Main-gauche attack still costs 2 AP"),
			Secondary->CurrentActionPointCost, 2) ||
		!TestTrue(TEXT("Both equipped weapons select one combined basic attack in slot 1"),
			Fixture.Hud->View.Actions[0].Action.Definition.ActionId == FGridDualWieldPolicy::ActionId() &&
			Fixture.Hud->View.Actions[0].Action.SourceDefinitionId == Main.ItemDefinitionId) ||
		!TestFalse(TEXT("No automatically projected MainHand shortcut"), Fixture.Hud->View.Actions[1].bHasBinding) ||
		!TestFalse(TEXT("No automatically projected OffHand shortcut"), Fixture.Hud->View.Actions[2].bHasBinding) ||
		!TestTrue(TEXT("Empty customizable slots remain unmodified in SaveGame"),
			Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[1].IsEmpty() &&
			Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[2].IsEmpty()))
		return false;

	FGridCombatActionRequestResult Result;
	if (!TestTrue(TEXT("The one combined primary action executes"), Fixture.Hud->RequestHotbarSlot(0, Result)))
		return false;
	TestEqual(TEXT("The final physical strike is the OffHand weapon"),
		Result.AttackRequest.OffensiveEquipmentSlot, EGridEquipmentSlot::OffHand);
	TestEqual(TEXT("The second physical source is Main-gauche"),
		Result.AttackRequest.OffensiveItemDefinitionId, Off.ItemDefinitionId);
	TestEqual(TEXT("Only three AP spent for the entire two-weapon action"),
		Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 1);

	FGridCombatHotbarBinding User;
	User.Reset(1); User.ActionId = TEXT("Attack_Unarmed");
	User.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
	if (!TestTrue(TEXT("User customizes shortcut 2"), Inventory->SetCharacterCombatHotbarBinding(0, 1, User))) return false;
	Fixture.Hud->RefreshFromSources();
	TestTrue(TEXT("Saved custom shortcut remains assigned"),
		Fixture.Hud->View.Actions[1].bResolved &&
		Fixture.Hud->View.Actions[1].Action.Definition.ActionId == FName(TEXT("Attack_Unarmed")));
	TestFalse(TEXT("No auto hand attacks appear in otherwise free slots"),
		Fixture.Hud->View.Actions[2].bHasBinding || Fixture.Hud->View.Actions[3].bHasBinding);
	// Preserve an old intentionally pinned weapon action. This is not an
	// automatic shortcut and is not deleted from the player's SaveGame.
	if (!TestTrue(TEXT("Legacy explicit weapon shortcut remains save-compatible"),
		Inventory->SetCharacterCombatHotbarBindingFromItem(0, 4, Off, EGridEquipmentSlot::OffHand))) return false;
	Fixture.Hud->RefreshFromSources();
	TestTrue(TEXT("Only explicitly pinned OffHand remains in shortcut 5"),
		Fixture.Hud->View.Actions[4].bResolved &&
		Fixture.Hud->View.Actions[4].Action.SourceRuntimeId == Off.RuntimeObjectId);
	TestFalse(TEXT("Pinning equipment never generates automatic duplicates"),
		Fixture.Hud->View.Actions[2].bHasBinding || Fixture.Hud->View.Actions[3].bHasBinding);

	FGridCombatHotbarBinding Saved;
	Inventory->GetCharacterCombatHotbarBinding(0, 4, Saved);
	TestEqual(TEXT("Pinned item keeps its runtime GUID"), Saved.PreferredSourceRuntimeId, Off.RuntimeObjectId);

	Inventory->ClearCharacterCombatHotbarBinding(0, 4);
	Inventory->PartyInventoryState.ActiveEquipment[0].MainHand = FGridItemInstance();
	Fixture.Hud->RefreshFromSources();
	TestEqual(TEXT("Empty MainHand makes OffHand the primary attack"),
		Fixture.Hud->View.Actions[0].Action.SourceEquipmentSlot, EGridEquipmentSlot::OffHand);
	TestFalse(TEXT("No extra basic attack when OffHand is primary"),
		Fixture.Hud->View.Actions[2].bHasBinding);

	// Passive shield, incompatible MainHand-only dagger and a two-handed
	// weapon must never create a fabricated secondary strike.
	UGridItemDefinitionAsset* Shield = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RoundShield_01.DA_Weapon_RoundShield_01"));
	UGridItemDefinitionAsset* Assassin = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_AssassinDagger_01.DA_Weapon_AssassinDagger_01"));
	UGridItemDefinitionAsset* GreatAxe = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_GreatAxe_01.DA_Weapon_GreatAxe_01"));
	if (!TestNotNull(TEXT("Actual passive shield"), Shield) ||
		!TestNotNull(TEXT("Actual MainHand-only dagger"), Assassin) ||
		!TestNotNull(TEXT("Actual two-handed axe"), GreatAxe) ||
		!TestFalse(TEXT("Shield is not an offensive source"), Shield->CanProvideAttackFromSlot(EGridEquipmentSlot::OffHand)) ||
		!TestFalse(TEXT("Assassin dagger is not an authorized OffHand attack"),
			Assassin->CanProvideAttackFromSlot(EGridEquipmentSlot::OffHand)))
		return false;
	Inventory->RegisterItemDefinition(Shield);
	Inventory->RegisterItemDefinition(Assassin);
	Inventory->RegisterItemDefinition(GreatAxe);
	Inventory->PartyInventoryState.ActiveEquipment[0].MainHand = Main;
	Inventory->PartyInventoryState.ActiveEquipment[0].OffHand =
		ItemFor(Shield, FGuid(19, 1, 2, 3), EGridEquipmentSlot::OffHand);
	Fixture.Hud->RefreshFromSources();
	TestFalse(TEXT("Passive shield never projects an extra basic attack"),
		Fixture.Hud->View.Actions[2].bHasBinding);

	Inventory->PartyInventoryState.ActiveEquipment[0].OffHand =
		ItemFor(Assassin, FGuid(19, 1, 2, 4), EGridEquipmentSlot::OffHand);
	Fixture.TurnManager->GetAvailableCombatActions(0, Available);
	TestFalse(TEXT("Catalogue rejects unauthorized OffHand attack"),
		Available.ContainsByPredicate(
			[](const FGridAvailableCombatAction& A)
			{
				return A.SourceEquipmentSlot == EGridEquipmentSlot::OffHand &&
					A.Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack;
			}));
	Inventory->PartyInventoryState.ActiveEquipment[0].MainHand =
		ItemFor(GreatAxe, FGuid(19, 1, 2, 5), EGridEquipmentSlot::MainHand);
	Inventory->PartyInventoryState.ActiveEquipment[0].OffHand = FGridItemInstance();
	Fixture.Hud->RefreshFromSources();
	TestFalse(TEXT("Two-handed weapon never adds a second shortcut"),
		Fixture.Hud->View.Actions[2].bHasBinding);
	TestEqual(TEXT("Great axe is the only physical primary attack"),
		Fixture.Hud->View.Actions[0].Action.SourceDefinitionId, GreatAxe->ItemDefinitionId);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT02192DualWield,
	"Grimrock.RPG.ARMEMENT.LOT02.19.2.N1DualWieldTwoProfilesOneCost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENTLOT02192DualWield::RunTest(const FString&)
{
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("N1 dual strike has a live combat world"), Fixture.IsReady())) return false;
	UGridItemDefinitionAsset* Dagger = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Dagger_01.DA_Weapon_Dagger_01"));
	UGridItemDefinitionAsset* MainGauche = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_MainGauche_01.DA_Weapon_MainGauche_01"));
	UGridItemDefinitionAsset* Shield = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RoundShield_01.DA_Weapon_RoundShield_01"));
	UGridItemDefinitionAsset* GreatAxe = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_GreatAxe_01.DA_Weapon_GreatAxe_01"));
	if (!TestNotNull(TEXT("Real ARM001"), Dagger) ||
		!TestNotNull(TEXT("Real ARM013"), MainGauche) ||
		!TestNotNull(TEXT("Passive ARM075"), Shield) ||
		!TestNotNull(TEXT("Two-hand ARM031"), GreatAxe)) return false;

	// Do not alter shared production DataAssets in an automation test. Give
	// the *actual weapon profiles* separate, deterministic transient audio.
	// The previous test set bAudioEnabled=false and thus could never detect
	// a missing OffHand audio request even though combat delegates succeeded.
	Dagger = DuplicateObject<UGridItemDefinitionAsset>(Dagger, Fixture.Runtime);
	MainGauche = DuplicateObject<UGridItemDefinitionAsset>(MainGauche, Fixture.Runtime);
	if (!TestNotNull(TEXT("Runtime copy of ARM001"), Dagger) ||
		!TestNotNull(TEXT("Runtime copy of ARM013"), MainGauche)) return false;
	USoundWave* MainAttackSound = NewObject<USoundWave>(Fixture.Runtime, TEXT("Test_Dual_MainAttackSound"));
	USoundWave* OffAttackSound = NewObject<USoundWave>(Fixture.Runtime, TEXT("Test_Dual_OffAttackSound"));
	USoundWave* MainImpactSound = NewObject<USoundWave>(Fixture.Runtime, TEXT("Test_Dual_MainImpactSound"));
	USoundWave* OffImpactSound = NewObject<USoundWave>(Fixture.Runtime, TEXT("Test_Dual_OffImpactSound"));
	const auto ConfigureSourceAudio = [](UGridItemDefinitionAsset* Item, USoundWave* AttackSound, USoundWave* ImpactSound)
	{
		Item->bProvidesAttackPresentation = true;
		FGridPlayerAttackPresentationProfile& Profile = Item->PlayerAttackPresentationProfile;
		Profile.MotionStyle = EGridPlayerAttackMotionStyle::Swing;
		Profile.bAnimateHeldItem = false;
		Profile.AttackAudio.Sounds.Reset();
		Profile.AttackAudio.Sounds.Add(AttackSound);
		Profile.ImpactHitAudio.Sounds.Reset();
		Profile.ImpactHitAudio.Sounds.Add(ImpactSound);
		Profile.ImpactMissAudio.Sounds.Reset();
		Profile.ImpactMissAudio.Sounds.Add(ImpactSound);
		Profile.AttackAudio.VolumeMultiplier = 1.0f;
		Profile.ImpactHitAudio.VolumeMultiplier = 1.0f;
		Profile.ImpactMissAudio.VolumeMultiplier = 1.0f;
		Profile.ImpactDelaySeconds = 0.16f;
		Profile.RecoveryDelaySeconds = 0.18f;
	};
	ConfigureSourceAudio(Dagger, MainAttackSound, MainImpactSound);
	ConfigureSourceAudio(MainGauche, OffAttackSound, OffImpactSound);
	if (!TestTrue(TEXT("MainHand has a valid audible presentation profile"), Dagger->HasValidPlayerAttackPresentation()) ||
		!TestTrue(TEXT("OffHand has an independent audible presentation profile"), MainGauche->HasValidPlayerAttackPresentation()))
		return false;

	UGridPartyInventoryComponent* Inventory = Fixture.Party->PartyInventoryComponent;
	auto MakeEquipped = [&Fixture](const UGridItemDefinitionAsset* Definition, FGuid Id, EGridEquipmentSlot Hand)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = Id;
		Item.ItemDefinitionId = Definition->ItemDefinitionId;
		Item.DisplayName = Definition->DisplayName;
		Item.ItemLevel = 1;
		Item.Quantity = 1;
		Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Item.OwnerCharacterIndex = 0;
		Item.OwnerGuid = Fixture.CharacterIds[0];
		Item.EquipmentSlot = Hand;
		return Item;
	};
	const FGridItemInstance Main = MakeEquipped(Dagger, FGuid(19,2,1,1), EGridEquipmentSlot::MainHand);
	const FGridItemInstance Off = MakeEquipped(MainGauche, FGuid(19,2,1,2), EGridEquipmentSlot::OffHand);
	Inventory->PartyInventoryState.ActiveEquipment[0].MainHand = Main;
	Inventory->PartyInventoryState.ActiveEquipment[0].OffHand = Off;
	if (!TestTrue(TEXT("Register MainHand N1 asset"), Inventory->RegisterItemDefinition(Dagger)) ||
		!TestTrue(TEXT("Register OffHand N1 asset"), Inventory->RegisterItemDefinition(MainGauche)) ||
		!TestTrue(TEXT("Register passive N1 shield"), Inventory->RegisterItemDefinition(Shield)) ||
		!TestTrue(TEXT("Register two-handed N1 axe"), Inventory->RegisterItemDefinition(GreatAxe))) return false;
	FGridDualWieldPair Pair;
	if (!TestTrue(TEXT("Two different physical one-handed weapons are eligible"),
			FGridDualWieldPolicy::TryResolve(Inventory, 0, Pair)) ||
		!TestTrue(TEXT("Primary and secondary attack use distinct IDs"),
			Pair.Main.Profile.AttackId != Pair.Off.Profile.AttackId) ||
		!TestEqual(TEXT("Secondary damage coefficient is 50 percent"),
			FGridDualWieldPolicy::OffHandDamagePercent, 50))
		return false;

	TArray<FGridAvailableCombatAction> Available;
	Fixture.TurnManager->GetAvailableCombatActions(0, Available);
	const FGridAvailableCombatAction* Dual = Available.FindByPredicate(
		[](const FGridAvailableCombatAction& A) { return A.Definition.ActionId == FGridDualWieldPolicy::ActionId(); });
	if (!TestNotNull(TEXT("Dual action belongs to the authoritative catalogue"), Dual) ||
		!TestEqual(TEXT("Dual action costs 3 AP"), Dual->CurrentActionPointCost, 3) ||
		!TestEqual(TEXT("Dual action declares two attack resolutions"), Dual->Definition.ResolutionCount, 2) ||
		!TestTrue(TEXT("Dual action is sourced from the physical MainHand"),
			Dual->SourceEquipmentSlot == EGridEquipmentSlot::MainHand && Dual->SourceRuntimeId == Main.RuntimeObjectId))
		return false;

	Fixture.Hud->RefreshFromSources();
	if (!TestEqual(TEXT("Contextual slot 1 uses dual strike"), Fixture.Hud->View.Actions[0].Action.Definition.ActionId,
			FGridDualWieldPolicy::ActionId()) ||
		!TestFalse(TEXT("Slot 2 has no automatic MainHand action"), Fixture.Hud->View.Actions[1].bHasBinding) ||
		!TestFalse(TEXT("Slot 3 has no automatic OffHand action"), Fixture.Hud->View.Actions[2].bHasBinding) ||
		!TestTrue(TEXT("No spare action was written into the SaveGame"),
			Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[1].IsEmpty() &&
			Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[2].IsEmpty()))
		return false;

	// Bind the actual presentation component to the real TurnManager: this
	// asserts event-feed gating without changing the authoritative hit tests.
	// Use the registered runtime component, not an unregistered NewObject
	// that advances only when a test calls AdvanceSequencedPresentation.
	UGridPlayerAttackPresentationComponent* Presentation =
		Fixture.Runtime->GetPlayerAttackPresentationComponent();
	if (!TestNotNull(TEXT("Actual runtime presentation component"), Presentation))
		return false;
	Presentation->bAudioEnabled = true;
	Presentation->bNativeAudioPlaybackEnabled = false;
	Presentation->bNativeVFXSpawnEnabled = false;
	Presentation->bNativeFeedbackEnabled = false;
	if (!TestTrue(TEXT("Physical dual strike has a presentation subscriber"),
			Presentation->InitializePresentation(Fixture.TurnManager)))
		return false;

	const int32 CombatLogBefore = Fixture.TurnManager->GetCombatLogEntries().Num();
	const int32 HealthBefore = Fixture.Monster->CurrentHealth;
	FGridCombatActionRequestResult Result;
	if (!TestTrue(TEXT("Double strike executes as one accepted combat action"),
		Fixture.Hud->RequestHotbarSlot(0, Result))) return false;
	TestTrue(TEXT("Dual action is accepted"), Result.bAccepted);
	TestEqual(TEXT("Only three action points paid: 4 -> 1"),
		Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 1);
	TestEqual(TEXT("Two physical attack outcomes logged after a single action"),
		Fixture.TurnManager->GetCombatLogEntries().Num() - CombatLogBefore, 2);
	TestEqual(TEXT("Second resolved request uses OffHand"),
		Result.AttackRequest.OffensiveEquipmentSlot, EGridEquipmentSlot::OffHand);
	TestEqual(TEXT("Second resolved request identifies the physical Main-gauche"),
		Result.AttackRequest.OffensiveItemDefinitionId, Off.ItemDefinitionId);
	TestEqual(TEXT("Second hit uses the Main-gauche attack profile"),
		Result.AttackRequest.AttackId, Pair.Off.Profile.AttackId);
	TestEqual(TEXT("Second hit applies 50 percent raw damage BEFORE armor"),
		Result.AttackResult.Trace.Damage.RawDamagePercent, 50);
	const TArray<FGridCombatLogEntry>& LogEntries = Fixture.TurnManager->GetCombatLogEntries();
	TArray<const FGridCombatLogEntry*> DualStrikeEntries;
	for (const FGridCombatLogEntry& Entry : LogEntries)
	{
		if (Entry.AttackId == Pair.Main.Profile.AttackId || Entry.AttackId == Pair.Off.Profile.AttackId)
		{
			DualStrikeEntries.Add(&Entry);
		}
	}
	if (TestEqual(TEXT("Two distinct combat log attack lines"), DualStrikeEntries.Num(), 2))
	{
		TestEqual(TEXT("First log line names physical MainHand attack"),
			DualStrikeEntries[0]->AttackId, Pair.Main.Profile.AttackId);
		TestEqual(TEXT("First log line identifies MainHand"), DualStrikeEntries[0]->OffensiveEquipmentSlot,
			EGridEquipmentSlot::MainHand);
		TestEqual(TEXT("Second log line names physical OffHand attack"),
			DualStrikeEntries[1]->AttackId, Pair.Off.Profile.AttackId);
		TestEqual(TEXT("Second log line identifies OffHand"), DualStrikeEntries[1]->OffensiveEquipmentSlot,
			EGridEquipmentSlot::OffHand);
	}
	TestTrue(TEXT("Combat did not heal the monster"), Fixture.Monster->CurrentHealth <= HealthBefore);
	TestTrue(TEXT("The visual sequence gates a second player action"),
		Fixture.TurnManager->IsSequencedAttackFeedPending());
	TestEqual(TEXT("First strike presentation starts at the action request"), Presentation->PresentationAttackCount, 1);
	TestTrue(TEXT("Live component stays ticking until the second weapon is heard"), Presentation->IsComponentTickEnabled());
	TestEqual(TEXT("MainHand sound is requested in slot 1"), Presentation->AudioAttackRequestCount, 1);
	TestTrue(TEXT("MainHand plays its own configured whoosh"), Presentation->LastPresentationRequest.ResolvedSound.Get() == MainAttackSound);
	TestEqual(TEXT("Impacts wait for their own audible frame"), Presentation->PresentationImpactHitCount +
		Presentation->PresentationImpactMissCount, 0);
	Presentation->AdvanceSequencedPresentation(0.17f);
	TestEqual(TEXT("Only one combat impact is presented after first windup"),
		Presentation->PresentationImpactHitCount + Presentation->PresentationImpactMissCount, 1);
	TestTrue(TEXT("First impact plays its own contact sound"), Presentation->LastPresentationRequest.ResolvedSound.Get() == MainImpactSound);
	TestEqual(TEXT("OffHand still waits behind recovery"), Presentation->PresentationAttackCount, 1);
	Presentation->AdvanceSequencedPresentation(0.36f);
	TestEqual(TEXT("Second physical weapon starts after audible separation"), Presentation->PresentationAttackCount, 2);
	TestEqual(TEXT("Exactly TWO configured weapon whoosh sounds requested"), Presentation->AudioAttackRequestCount, 2);
	TestTrue(TEXT("OffHand must use its own sound, not the MainHand sample"),
		Presentation->LastPresentationRequest.ResolvedSound.Get() == OffAttackSound);
	Presentation->AdvanceSequencedPresentation(0.17f);
	TestEqual(TEXT("Second impact has its own presentation slot"),
		Presentation->PresentationImpactHitCount + Presentation->PresentationImpactMissCount, 2);
	TestTrue(TEXT("Second impact uses the OffHand contact sample"),
		Presentation->LastPresentationRequest.ResolvedSound.Get() == OffImpactSound);
	TestEqual(TEXT("Both separate impacts requested their sounds"),
		Presentation->AudioImpactHitRequestCount + Presentation->AudioImpactMissRequestCount, 2);
	TestEqual(TEXT("Two whooshes plus two impact audio requests, one per weapon"),
		Presentation->AudioPlaybackRequestCount, 4);
	Presentation->AdvanceSequencedPresentation(0.18f);
	TestFalse(TEXT("End of the dual sequence releases further combat actions"),
		Fixture.TurnManager->IsSequencedAttackFeedPending());

	const int32 LogBeforeRefusal = Fixture.TurnManager->GetCombatLogEntries().Num();
	FGridCombatActionRequestResult Refused;
	TestFalse(TEXT("Second double strike with only 1 AP is refused"), Fixture.Hud->RequestHotbarSlot(0, Refused));
	TestEqual(TEXT("Insufficient AP leaves remaining AP at one"),
		Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 1);
	TestEqual(TEXT("Refused attack records no additional hit"),
		Fixture.TurnManager->GetCombatLogEntries().Num(), LogBeforeRefusal);

	Inventory->PartyInventoryState.ActiveEquipment[0].OffHand =
		MakeEquipped(Shield, FGuid(19,2,1,3), EGridEquipmentSlot::OffHand);
	Fixture.Hud->RefreshFromSources();
	TestEqual(TEXT("Equipping shield falls back to standalone MainHand"),
		Fixture.Hud->View.Actions[0].Action.SourceDefinitionId, Main.ItemDefinitionId);
	TestTrue(TEXT("Dual action disappears when OffHand is passive"),
		Fixture.Hud->View.Actions[0].Action.Definition.ActionId != FGridDualWieldPolicy::ActionId());
	Inventory->PartyInventoryState.ActiveEquipment[0].MainHand =
		MakeEquipped(GreatAxe, FGuid(19,2,1,4), EGridEquipmentSlot::MainHand);
	Inventory->PartyInventoryState.ActiveEquipment[0].OffHand = FGridItemInstance();
	Fixture.Hud->RefreshFromSources();
	TestTrue(TEXT("Two-handed weapon never creates dual attack"),
		Fixture.Hud->View.Actions[0].Action.Definition.ActionId != FGridDualWieldPolicy::ActionId());
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT021921DualAttackIconViewTest,
	"Grimrock.RPG.ARMEMENT.LOT02.19.2.1.ContextualDualAttackIcons",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENTLOT021921DualAttackIconViewTest::RunTest(const FString&)
{
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("HUD and combat fixture are ready"), Fixture.IsReady())) return false;

	UGridItemDefinitionAsset* Dagger = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Dagger_01.DA_Weapon_Dagger_01"));
	UGridItemDefinitionAsset* MainGauche = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_MainGauche_01.DA_Weapon_MainGauche_01"));
	UGridItemDefinitionAsset* Shield = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RoundShield_01.DA_Weapon_RoundShield_01"));
	if (!TestNotNull(TEXT("Real ARM001 dagger"), Dagger) ||
		!TestNotNull(TEXT("Real ARM013 Main-gauche"), MainGauche) ||
		!TestNotNull(TEXT("Real ARM075 shield"), Shield)) return false;

	UGridPartyInventoryComponent* Inventory = Fixture.Party->PartyInventoryComponent;
	auto MakeEquipped = [&Fixture](const UGridItemDefinitionAsset* Definition, const FGuid& Id, EGridEquipmentSlot Slot)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = Id;
		Item.ItemDefinitionId = Definition->ItemDefinitionId;
		Item.DisplayName = Definition->DisplayName;
		Item.ItemLevel = 1;
		Item.Quantity = 1;
		Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Item.OwnerGuid = Fixture.CharacterIds[0];
		Item.OwnerCharacterIndex = 0;
		Item.EquipmentSlot = Slot;
		return Item;
	};
	Inventory->PartyInventoryState.ActiveEquipment[0].MainHand =
		MakeEquipped(Dagger, FGuid(19, 2, 11, 1), EGridEquipmentSlot::MainHand);
	Inventory->PartyInventoryState.ActiveEquipment[0].OffHand =
		MakeEquipped(MainGauche, FGuid(19, 2, 11, 2), EGridEquipmentSlot::OffHand);
	if (!TestTrue(TEXT("Register dagger"), Inventory->RegisterItemDefinition(Dagger)) ||
		!TestTrue(TEXT("Register Main-gauche"), Inventory->RegisterItemDefinition(MainGauche)) ||
		!TestTrue(TEXT("Register shield"), Inventory->RegisterItemDefinition(Shield))) return false;

	Fixture.Hud->RefreshFromSources();
	const FGridCombatHudActionView DualView = Fixture.Hud->View.Actions[0];
	if (!TestEqual(TEXT("Primary action remains the authoritative double strike"),
			DualView.Action.Definition.ActionId, FGridDualWieldPolicy::ActionId()) ||
		!TestTrue(TEXT("Composite retains an authored fallback icon"),
			!DualView.Action.Definition.Icon.IsNull()) ||
		!TestTrue(TEXT("PrimaryAttack binding remains unchanged"),
			Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[0].IsPrimaryAttackBinding())) return false;

	// A real alternative texture verifies that a single Designer Image
	// can show the system action icon without modifying the WBP hierarchy.
	const TSoftObjectPtr<UTexture2D> DedicatedIcon = !MainGauche->CombatActions.IsEmpty() &&
		!MainGauche->CombatActions[0].Icon.IsNull()
			? MainGauche->CombatActions[0].Icon : MainGauche->Icon;
	if (!TestTrue(TEXT("Test has a real alternate Texture2D"), !DedicatedIcon.IsNull())) return false;
	TestEqual(TEXT("Configured dual icon overrides the dagger-only image"),
		UGridCombatHudActionWidget::ResolveDisplayedIcon(DualView, DedicatedIcon).ToSoftObjectPath(),
		DedicatedIcon.ToSoftObjectPath());
	TestEqual(TEXT("No configured dual icon falls back to the existing action icon"),
		UGridCombatHudActionWidget::ResolveDisplayedIcon(DualView, TSoftObjectPtr<UTexture2D>()).ToSoftObjectPath(),
		DualView.Action.Definition.Icon.ToSoftObjectPath());

	UGridCombatHudActionWidget* Slot = CreateWidget<UGridCombatHudActionWidget>(
		Fixture.TestWorld.World, UGridCombatHudActionWidget::StaticClass());
	if (!TestNotNull(TEXT("One-icon action widget can be constructed"), Slot)) return false;
	Slot->Image_ActionIcon = NewObject<UImage>(Slot);
	Slot->Text_Quantity = NewObject<UTextBlock>(Slot);
	Slot->DualAttackIcon = DedicatedIcon;
	Slot->InitializeAction(Fixture.Hud, DualView);
	TestEqual(TEXT("The existing Image_ActionIcon remains the only visible action image"),
		Slot->Image_ActionIcon->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestTrue(TEXT("No strike-count x2 text is displayed"), Slot->Text_Quantity->GetText().IsEmpty());
	TestEqual(TEXT("Quantity badge is hidden for a dual action"),
		Slot->Text_Quantity->GetVisibility(), ESlateVisibility::Collapsed);

	Inventory->PartyInventoryState.ActiveEquipment[0].OffHand =
		MakeEquipped(Shield, FGuid(19, 2, 11, 3), EGridEquipmentSlot::OffHand);
	Fixture.Hud->RefreshFromSources();
	const FGridCombatHudActionView SimpleView = Fixture.Hud->View.Actions[0];
	TestTrue(TEXT("Shield changes primary attack back to simple"),
		SimpleView.Action.Definition.ActionId != FGridDualWieldPolicy::ActionId());
	TestEqual(TEXT("Dedicated dual icon never leaks onto normal attacks"),
		UGridCombatHudActionWidget::ResolveDisplayedIcon(SimpleView, DedicatedIcon).ToSoftObjectPath(),
		SimpleView.Action.Definition.Icon.ToSoftObjectPath());
	Slot->InitializeAction(Fixture.Hud, SimpleView);
	TestEqual(TEXT("Normal action still uses the same Image_ActionIcon"),
		Slot->Image_ActionIcon->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestTrue(TEXT("Single strike has no x2 indicator"), Slot->Text_Quantity->GetText().IsEmpty());
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT0220EquippedIconCategories,
	"Grimrock.RPG.ARMEMENT.LOT02.20.EquippedActionIconCategories",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENTLOT0220EquippedIconCategories::RunTest(const FString&)
{
	using Kind = EGridEquippedWeaponActionIconKind;
	UGridItemDefinitionAsset* Item = NewObject<UGridItemDefinitionAsset>();
	Item->ItemType = EGridItemType::Weapon;
	Item->bUseWeaponCatalogue = true;

	FGridAvailableCombatAction Action;
	Action.Definition.ActionId = FName(TEXT("Attack_Test"));
	Action.Definition.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
	Action.Definition.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
	Action.Definition.ActionType = EGridCombatActionType::MeleeAttack;
	Action.SourceEquipmentSlot = EGridEquipmentSlot::MainHand;
	Action.SourceRuntimeId = FGuid(20,1,1,1);
	Action.bEnabled = true;

	struct FExpectedIconFamily
	{
		int32 First;
		int32 Last;
		Kind Expected;
		EGridCombatActionType Type;
	};
	const FExpectedIconFamily Families[] = {
		{1,26,Kind::MeleeOneHand,EGridCombatActionType::MeleeAttack},
		{27,37,Kind::MeleeTwoHanded,EGridCombatActionType::MeleeAttack},
		{38,44,Kind::MeleePolearm,EGridCombatActionType::MeleeAttack},
		{45,48,Kind::RangedBow,EGridCombatActionType::RangedAttack},
		{49,51,Kind::RangedCrossbow,EGridCombatActionType::RangedAttack},
		{52,53,Kind::RangedOther,EGridCombatActionType::RangedAttack},
		{54,60,Kind::ThrownWeapon,EGridCombatActionType::RangedAttack},
		{61,65,Kind::MagicOneHand,EGridCombatActionType::RangedAttack},
		{66,68,Kind::MagicTwoHanded,EGridCombatActionType::RangedAttack},
		{69,69,Kind::MagicOneHand,EGridCombatActionType::RangedAttack}
	};
	for (const FExpectedIconFamily& Family : Families)
	{
		for (int32 Id = Family.First; Id <= Family.Last; ++Id)
		{
			Item->WeaponArchetypeId = FName(*FString::Printf(TEXT("ARM%03d"), Id));
			Action.Definition.ActionType = Family.Type;
			TestEqual(FString::Printf(TEXT("Visual family of %s"), *Item->WeaponArchetypeId.ToString()),
				FGridEquippedWeaponActionIconResolver::Classify(Action, Item), Family.Expected);
		}
	}
	// Complete the 82-archtype audit: passive focus objects (70-71)
	// and passive shields (72-82) MUST NOT manufacture offensive icons.
	Action.Definition.ActionType = EGridCombatActionType::RangedAttack;
	for (int32 Id = 70; Id <= 82; ++Id)
	{
		Item->ItemType = Id <= 71 ? EGridItemType::Weapon : EGridItemType::Shield;
		Item->WeaponArchetypeId = FName(*FString::Printf(TEXT("ARM%03d"), Id));
		TestEqual(FString::Printf(TEXT("Passive ARM%03d has no offensive icon"), Id),
			FGridEquippedWeaponActionIconResolver::Classify(Action, Item), Kind::None);
	}
	Item->ItemType = EGridItemType::Weapon;
	// A thrown weapon switched to an actually authored melee action takes
	// the one-handed melee icon, not its default thrown classification.
	Item->WeaponArchetypeId = TEXT("ARM054");
	Action.Definition.ActionType = EGridCombatActionType::MeleeAttack;
	TestEqual(TEXT("Thrown knife melee mode has an individual melee icon"),
		FGridEquippedWeaponActionIconResolver::Classify(Action, Item), Kind::MeleeOneHand);
	Item->WeaponArchetypeId = TEXT("ARM010");
	Item->bCombatThrowWeapon = true;
	Action.Definition.ActionType = EGridCombatActionType::RangedAttack;
	TestEqual(TEXT("Combat throwable hatchet uses throw icon for throw mode"),
		FGridEquippedWeaponActionIconResolver::Classify(Action, Item), Kind::ThrownWeapon);
	Item->bCombatThrowWeapon = false;
	Action.Definition.ActionType = EGridCombatActionType::MeleeAttack;
	Action.Definition.ActionId = FGridDualWieldPolicy::ActionId();
	TestEqual(TEXT("Two physical one-handed attacks share one dual icon"),
		FGridEquippedWeaponActionIconResolver::Classify(Action, Item), Kind::MeleeDualWield);

	Action.Definition.ActionId = TEXT("Action_Test");
	Item->ItemType = EGridItemType::Shield;
	Action.Definition.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
	Item->WeaponArchetypeId = TEXT("ARM075");
	TestEqual(TEXT("A passive shield must never manufacture an offensive icon"),
		FGridEquippedWeaponActionIconResolver::Classify(Action, Item), Kind::None);
	Action.Definition.ResolutionProfile = EGridCombatActionResolutionProfile::Defense;
	Action.Definition.ActionType = EGridCombatActionType::Defend;
	TestEqual(TEXT("Only a real shield defense action receives a block icon"),
		FGridEquippedWeaponActionIconResolver::Classify(Action, Item), Kind::DefenseBlock);
	Item->ItemType = EGridItemType::Weapon;
	Item->WeaponArchetypeId = TEXT("ARM013");
	TestEqual(TEXT("Future authored Main-gauche defense uses the parry icon"),
		FGridEquippedWeaponActionIconResolver::Classify(Action, Item), Kind::DefenseParry);

	Action.Definition.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
	TestEqual(TEXT("Spellbook actions are entirely outside ARMEMENT icon palette"),
		FGridEquippedWeaponActionIconResolver::Classify(Action, Item), Kind::None);
	Action.Definition.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
	TestEqual(TEXT("Consumables are entirely outside ARMEMENT icon palette"),
		FGridEquippedWeaponActionIconResolver::Classify(Action, Item), Kind::None);
	Action.Definition.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
	Item->bUseWeaponCatalogue = false;
	TestEqual(TEXT("Legacy equipment without ARM identity retains its icon"),
		FGridEquippedWeaponActionIconResolver::Classify(Action, Item), Kind::None);

	// Palette overrides only the selected class of action; missing entries
	// keep the existing DataAsset icon without touching hotbar or combat.
	UGridEquippedWeaponActionIconPalette* Palette = NewObject<UGridEquippedWeaponActionIconPalette>();
	UGridItemDefinitionAsset* Dagger = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Dagger_01.DA_Weapon_Dagger_01"));
	UGridItemDefinitionAsset* OffHand = LoadObject<UGridItemDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_MainGauche_01.DA_Weapon_MainGauche_01"));
	if (!TestNotNull(TEXT("Palette needs real production ARM001"), Dagger) ||
		!TestNotNull(TEXT("Palette needs real ARM013 alternate texture"), OffHand))
		return false;
	const TSoftObjectPtr<UTexture2D> Icon = OffHand->Icon;
	if (!TestTrue(TEXT("Alternate Texture2D is authored"), !Icon.IsNull()))
		return false;
	Palette->Icons.Add(Kind::MeleeDualWield, Icon);
	Item->bUseWeaponCatalogue = true;
	Item->ItemType = EGridItemType::Weapon;
	Item->WeaponArchetypeId = TEXT("ARM001");
	Action.Definition.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
	Action.Definition.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
	Action.Definition.ActionType = EGridCombatActionType::MeleeAttack;
	Action.Definition.ActionId = FGridDualWieldPolicy::ActionId();
	Action.Definition.Icon = Dagger->Icon;
	FGridCombatHudActionView View;
	View.bResolved = true;
	View.bHasBinding = true;
	View.Action = Action;
	TestEqual(TEXT("Central palette overrides previous one-off dual icon"),
		UGridCombatHudActionWidget::ResolveDisplayedIcon(View, Dagger->Icon, Palette, Item).ToSoftObjectPath(),
		Icon.ToSoftObjectPath());
	Palette->Icons.Add(Kind::None, Icon); // Deliberate bad configuration.
	FGridCombatHudActionView SpellView = View;
	SpellView.Action.Definition.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
	SpellView.Action.Definition.ActionId = TEXT("Spell_Test");
	TestEqual(TEXT("Even a misconfigured None entry cannot repaint a spell"),
		UGridCombatHudActionWidget::ResolveDisplayedIcon(
			SpellView, TSoftObjectPtr<UTexture2D>(), Palette, Item).ToSoftObjectPath(),
		Dagger->Icon.ToSoftObjectPath());
	Palette->Icons.Reset();
	TestEqual(TEXT("Unfilled palette preserves existing dual icon without reauthoring WBP"),
		UGridCombatHudActionWidget::ResolveDisplayedIcon(View, Dagger->Icon, Palette, Item).ToSoftObjectPath(),
		Dagger->Icon.ToSoftObjectPath());
	return true;
}


/**
 * ARMEMENT-LOT02.16.1: run every N1 melee weapon through the genuine
 * combat TurnManager/monster pipeline. Asset catalogue-only audits do not
 * prove that an attack can be requested, paid and resolved in combat.
 * The original DataAssets and the player's saves are never mutated.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT02161MeleeN1Execution,
	"Grimrock.RPG.ARMEMENT.LOT02.16.1.MeleeN1Execution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENTLOT02161MeleeN1Execution::RunTest(const FString& Parameters)
{
	(void)Parameters;
	struct FMeleeN1Case
	{
		const TCHAR* ArchetypeId;
		const TCHAR* AssetName;
		EGridEquipmentSlot Slot;
		bool bTwoHanded;
	};
	const FMeleeN1Case Cases[] = {
		{ TEXT("ARM001"), TEXT("Dagger"), EGridEquipmentSlot::MainHand, false },
		{ TEXT("ARM002"), TEXT("AssassinDagger"), EGridEquipmentSlot::MainHand, false },
		{ TEXT("ARM003"), TEXT("Stiletto"), EGridEquipmentSlot::MainHand, false },
		{ TEXT("ARM013"), TEXT("MainGauche"), EGridEquipmentSlot::OffHand, false },
		{ TEXT("ARM014"), TEXT("StraightSword"), EGridEquipmentSlot::MainHand, false },
		{ TEXT("ARM015"), TEXT("LongSword"), EGridEquipmentSlot::MainHand, false },
		{ TEXT("ARM020"), TEXT("Mace"), EGridEquipmentSlot::MainHand, false },
		{ TEXT("ARM025"), TEXT("WarPick"), EGridEquipmentSlot::MainHand, false },
		{ TEXT("ARM031"), TEXT("GreatAxe"), EGridEquipmentSlot::MainHand, true },
		{ TEXT("ARM038"), TEXT("ShortSpear"), EGridEquipmentSlot::MainHand, true }
	};

	int32 Executed = 0;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Cases); ++Index)
	{
		const FMeleeN1Case& Case = Cases[Index];
		const FString Context = FString::Printf(TEXT("N1 %s"), Case.ArchetypeId);
		FGridCombatHudFixture Fixture;
		if (!TestTrue(*FString::Printf(TEXT("%s: live combat fixture"), *Context), Fixture.IsReady()))
			return false;

		const FString Name(Case.AssetName);
		const FString AssetPath = FString::Printf(
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_%s_01.DA_Weapon_%s_01"),
			*Name, *Name);
		UGridItemDefinitionAsset* Definition = LoadObject<UGridItemDefinitionAsset>(nullptr, *AssetPath);
		if (!TestNotNull(*FString::Printf(TEXT("%s: real weapon DataAsset"), *Context), Definition))
			return false;
		if (!TestTrue(*FString::Printf(TEXT("%s: matching catalogue identity"), *Context),
				Definition->bUseWeaponCatalogue &&
				Definition->WeaponArchetypeId == FName(Case.ArchetypeId) &&
				Definition->DefaultWeaponItemLevel == 1) ||
			!TestTrue(*FString::Printf(TEXT("%s: authorized attack slot"), *Context),
				Definition->CanProvideAttackFromSlot(Case.Slot)) ||
			!TestEqual(*FString::Printf(TEXT("%s: physical two-hand contract"), *Context),
				Definition->GetEffectiveHandUsage() == EGridItemHandUsage::TwoHanded, Case.bTwoHanded))
			return false;

		UGridPartyInventoryComponent* Inventory = Fixture.Party->PartyInventoryComponent;
		FGridItemInstance Equipped;
		Equipped.RuntimeObjectId = FGuid(16, 1, 1, Index + 1);
		Equipped.ItemDefinitionId = Definition->ItemDefinitionId;
		Equipped.DisplayName = Definition->DisplayName;
		Equipped.ItemLevel = 1;
		Equipped.Quantity = 1;
		Equipped.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Equipped.OwnerCharacterIndex = 0;
		Equipped.OwnerGuid = Fixture.CharacterIds[0];
		Equipped.EquipmentSlot = Case.Slot;
		Inventory->PartyInventoryState.ActiveEquipment[0].MainHand = FGridItemInstance();
		Inventory->PartyInventoryState.ActiveEquipment[0].OffHand = FGridItemInstance();
		if (Case.Slot == EGridEquipmentSlot::MainHand)
			Inventory->PartyInventoryState.ActiveEquipment[0].MainHand = Equipped;
		else
			Inventory->PartyInventoryState.ActiveEquipment[0].OffHand = Equipped;
		if (!TestTrue(*FString::Printf(TEXT("%s: register actual weapon"), *Context),
				Inventory->RegisterItemDefinition(Definition)))
			return false;

		TArray<FGridAvailableCombatAction> Available;
		Fixture.TurnManager->GetAvailableCombatActions(0, Available);
		const FGridAvailableCombatAction* PhysicalAction = Available.FindByPredicate(
			[&Equipped, &Case](const FGridAvailableCombatAction& Action)
			{
				return Action.Definition.SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
					Action.Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack &&
					Action.SourceEquipmentSlot == Case.Slot &&
					Action.SourceRuntimeId == Equipped.RuntimeObjectId;
			});
		if (!TestNotNull(*FString::Printf(TEXT("%s: authoritative attack in catalogue"), *Context), PhysicalAction))
			return false;
		if (!TestTrue(*FString::Printf(TEXT("%s: N1 AP cost fits the active turn"), *Context),
				PhysicalAction->CurrentActionPointCost > 0 &&
				PhysicalAction->CurrentActionPointCost <= 4) ||
			!TestTrue(*FString::Printf(TEXT("%s: action enabled at adjacent monster"), *Context),
				PhysicalAction->bEnabled))
			return false;

		const int32 ExpectedCost = PhysicalAction->CurrentActionPointCost;
		const int32 LogBefore = Fixture.TurnManager->GetCombatLogEntries().Num();
		const int32 HealthBefore = Fixture.Monster->CurrentHealth;
		Fixture.Hud->RefreshFromSources();
		if (!TestTrue(*FString::Printf(TEXT("%s: primary hotbar uses equipped weapon"), *Context),
				Fixture.Hud->View.Actions[0].bResolved &&
				Fixture.Hud->View.Actions[0].Action.SourceRuntimeId == Equipped.RuntimeObjectId))
			return false;
		FGridCombatActionRequestResult Attack;
		if (!TestTrue(*FString::Printf(TEXT("%s: real N1 melee action accepted"), *Context),
				Fixture.Hud->RequestHotbarSlot(0, Attack)) ||
			!TestTrue(*FString::Printf(TEXT("%s: action result accepted"), *Context), Attack.bAccepted))
			return false;

		TestEqual(*FString::Printf(TEXT("%s: AP charged once"), *Context),
			Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 4 - ExpectedCost);
		TestEqual(*FString::Printf(TEXT("%s: one physical attack logged"), *Context),
			Fixture.TurnManager->GetCombatLogEntries().Num() - LogBefore, 1);
		TestEqual(*FString::Printf(TEXT("%s: correct weapon dealt the attack"), *Context),
			Attack.AttackRequest.OffensiveItemDefinitionId, Equipped.ItemDefinitionId);
		TestEqual(*FString::Printf(TEXT("%s: correct equipment hand used"), *Context),
			Attack.AttackRequest.OffensiveEquipmentSlot, Case.Slot);
		TestTrue(*FString::Printf(TEXT("%s: real attack roll preserved"), *Context),
			Attack.AttackResult.Trace.bValid && Attack.AttackResult.Trace.Attack.Roll.bUsed);
		TestTrue(*FString::Printf(TEXT("%s: no phantom healing"), *Context),
			Fixture.Monster->CurrentHealth <= HealthBefore);

		// Insufficient AP must not generate another hit or spend resources.
		Fixture.TurnManager->PlayerCharacterTurnStates[0].RemainingActionPoints = 0;
		Fixture.Hud->RefreshFromSources();
		const int32 LogsBeforeReject = Fixture.TurnManager->GetCombatLogEntries().Num();
		FGridCombatActionRequestResult Refused;
		TestFalse(*FString::Printf(TEXT("%s: action refused without AP"), *Context),
			Fixture.Hud->RequestHotbarSlot(0, Refused));
		TestEqual(*FString::Printf(TEXT("%s: no combat log entry on refusal"), *Context),
			Fixture.TurnManager->GetCombatLogEntries().Num(), LogsBeforeReject);
		++Executed;
	}
	TestEqual(TEXT("All ten real N1 melee profiles executed"), Executed, static_cast<int32>(UE_ARRAY_COUNT(Cases)));
	AddInfo(TEXT("[ARMEMENT-LOT02.16.1] N1 melee=10 actual monster attack transactions; PIE/GPU and Save/Continue remain separate."));
	return true;
}

#endif

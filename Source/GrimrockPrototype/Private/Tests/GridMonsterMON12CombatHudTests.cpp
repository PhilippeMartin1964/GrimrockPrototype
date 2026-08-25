#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/InputComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WrapBox.h"
#include "Core/GridDirectionUtils.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "RPG/RPGClassAsset.h"
#include "Runtime/Combat/GridPlayerAttackPresentationComponent.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridThrownItemActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"
#include "UI/GridCombatHudWidget.h"
#include "UI/GridCombatHotbarDragDropOperation.h"
#include "UI/GridInventoryDragDropOperation.h"

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
		Character.DerivedStats.CurrentHealth = 20;
		Character.DerivedStats.MaxHealth = 20;
		Character.DerivedStats.CurrentMana = 8;
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

	const FGridAvailableCombatAction* FindPaletteAction(const UGridCombatHudWidget* Hud, FName ActionId)
	{
		return IsValid(Hud) ? Hud->View.ActionPalette.FindByPredicate(
								  [ActionId](const FGridAvailableCombatAction& Candidate)
								  {
									  return Candidate.Definition.ActionId == ActionId;
								  })
							: nullptr;
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
			WeaponDefinition->bProvidesAttack = true;
			WeaponDefinition->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::MainHand);
			WeaponDefinition->OffensiveProfile.AttackId = TEXT("Attack_MON12_7_Sword");
			WeaponDefinition->OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Physical;
			WeaponDefinition->OffensiveProfile.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
			WeaponDefinition->OffensiveProfile.AttackDefinition.MinDamage = 1;
			WeaponDefinition->OffensiveProfile.AttackDefinition.MaxDamage = 2;
			WeaponDefinition->OffensiveProfile.RangeCells = 1;
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
	Bindings.SetNum(FGridCombatHotbarBinding::SlotCount);
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
	TestEqual(TEXT("The view always exposes ten hotbar slots"), Actions.Num(), 10);
	TestEqual(TEXT("The first slot displays key 1"), Actions[0].ShortcutText.ToString(), FString(TEXT("1")));
	TestEqual(TEXT("The last slot displays key 0"), Actions[9].ShortcutText.ToString(), FString(TEXT("0")));
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
	TestEqual(TEXT("The HUD exposes ten fixed hotbar slots"), Fixture.Hud->View.Actions.Num(), 10);
	TestFalse(TEXT("A new hotbar starts empty"), Fixture.Hud->View.Actions[0].bHasBinding);

	UWrapBox* LegacyWrapPanel = NewObject<UWrapBox>(Fixture.Hud, TEXT("Panel_Actions_Test"));
	Fixture.Hud->Panel_Actions = LegacyWrapPanel;
	Fixture.Hud->ActionWidgetClass = UGridCombatHudActionWidget::StaticClass();
	Fixture.Hud->RefreshFromSources();
	TestNotNull(TEXT("A horizontal hotbar row is created"), Fixture.Hud->HotbarRow.Get());
	TestEqual(TEXT("The legacy wrap panel owns one row only"), LegacyWrapPanel->GetChildrenCount(), 1);
	if (Fixture.Hud->HotbarRow)
	{
		TestEqual(TEXT("The HUD owns ten runtime shortcut widgets"), Fixture.Hud->HotbarActionWidgets.Num(), 10);
		TestEqual(TEXT("All ten shortcuts share the same row"), Fixture.Hud->HotbarRow->GetChildrenCount(), 10);
		if (Fixture.Hud->HotbarActionWidgets.IsValidIndex(0))
		{
			TestEqual(TEXT("An empty shortcut frame remains visible"), Fixture.Hud->HotbarActionWidgets[0]->GetRenderOpacity(), 0.8f);
		}
		for (const UGridCombatHudActionWidget* ActionWidget : Fixture.Hud->HotbarActionWidgets)
		{
			TestTrue(TEXT("Each shortcut belongs to the horizontal row"), IsValid(ActionWidget) && ActionWidget->GetParent() == Fixture.Hud->HotbarRow);
		}
	}

	FGridItemInstance EquippedSword;
	TestTrue(TEXT("The fixture exposes the equipped sword"),
		Fixture.Party->PartyInventoryComponent->GetEquippedItem(0, EGridEquipmentSlot::MainHand, EquippedSword));
	UGridInventoryDragDropOperation* SwordDrag = NewObject<UGridInventoryDragDropOperation>(Fixture.Hud);
	SwordDrag->InitializeFromSlot(EGridInventoryUiSlotType::MainHand, 0, EquippedSword);
	TestTrue(TEXT("Dragging the sword can configure slot zero"), Fixture.Hud->HandleHotbarDrop(0, SwordDrag));

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

	Fixture.TurnManager->PlayerAttackActionPointCost = 3;
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
	TestTrue(TEXT("The sword is assigned to keyboard slot 1"),
		Fixture.Party->PartyInventoryComponent->SetCharacterCombatHotbarBindingFromItem(0, 0, EquippedSword, EGridEquipmentSlot::MainHand));

	UWrapBox* LegacyWrapPanel = NewObject<UWrapBox>(Fixture.Hud, TEXT("Panel_Actions_1283_Click"));
	Fixture.Hud->Panel_Actions = LegacyWrapPanel;
	Fixture.Hud->ActionWidgetClass = UGridCombatHudActionWidget::StaticClass();
	Fixture.Hud->RefreshFromSources();
	if (!TestTrue(TEXT("The first clickable slot exists"), Fixture.Hud->HotbarActionWidgets.IsValidIndex(0)))
	{
		return false;
	}

	TestTrue(TEXT("A short click executes the configured attack"), Fixture.Hud->HotbarActionWidgets[0]->TryExecuteAction());
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
	FGridCombatHotbarBinding UnarmedBinding;
	UnarmedBinding.Reset(0);
	UnarmedBinding.ActionId = TEXT("Attack_Unarmed");
	UnarmedBinding.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
	TestTrue(
		TEXT("The unarmed action can be configured explicitly"), Fixture.Party->PartyInventoryComponent->SetCharacterCombatHotbarBinding(0, 0, UnarmedBinding));

	FGridCombatActionRequestResult Result;
	TestTrue(TEXT("The configured unarmed shortcut executes"), Fixture.Hud->RequestHotbarSlot(0, Result));
	TestTrue(TEXT("The generic result accepts the unarmed attack"), Result.bAccepted);
	TestEqual(TEXT("The unarmed shortcut pays two action points"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 2);
	TestEqual(TEXT("The unarmed shortcut uses no equipment slot"), Result.Action.SourceEquipmentSlot, EGridEquipmentSlot::None);
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
	TestTrue(TEXT("The sword is assigned before keyboard execution"),
		Fixture.Party->PartyInventoryComponent->SetCharacterCombatHotbarBindingFromItem(0, 0, EquippedSword, EGridEquipmentSlot::MainHand));
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
	Character.DerivedStats.CurrentHealth = 5;
	Character.DerivedStats.CurrentMana = 4;

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
	TestTrue(TEXT("The potion configures shortcut one"), Inventory->SetCharacterCombatHotbarBindingFromItem(0, 0, Potion, EGridEquipmentSlot::None));

	Fixture.Hud->RefreshFromSources();
	TestTrue(TEXT("The configured potion resolves from the catalogue"), Fixture.Hud->View.Actions[0].bResolved);
	TestTrue(TEXT("The potion is initially usable"), Fixture.Hud->View.Actions[0].Action.bEnabled);
	TestEqual(TEXT("The catalogue aggregates both potion units"), Fixture.Hud->View.Actions[0].Action.CurrentSourceItemQuantity, 2);

	FGridCombatActionRequestResult FirstUse;
	TestTrue(TEXT("The first potion use is accepted"), Fixture.Hud->RequestHotbarSlot(0, FirstUse));
	TestEqual(TEXT("The potion restores seven health"), Character.DerivedStats.CurrentHealth, 12);
	TestEqual(TEXT("The same potion restores three mana"), Character.DerivedStats.CurrentMana, 7);
	TestEqual(TEXT("The potion spends one action point"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 3);
	TestEqual(TEXT("Exactly one potion remains"), Inventory->CountItemDefinitionInCharacterInventory(0, PotionDefinition->ItemDefinitionId), 1);
	TestEqual(TEXT("The result records the consumed unit"), FirstUse.QuickItemResult.SourceQuantityAfter, 1);
	FGridCombatHotbarBinding RemainingPotionBinding;
	TestTrue(TEXT("The consumed shortcut remains readable"), Inventory->GetCharacterCombatHotbarBinding(0, 0, RemainingPotionBinding));
	TestTrue(TEXT("Every accepted potion clears its shortcut"), RemainingPotionBinding.IsEmpty());
	TestFalse(TEXT("The consumed potion disappears from the HUD slot"), Fixture.Hud->View.Actions[0].bHasBinding);

	TestTrue(TEXT("The remaining potion can be assigned again"), Inventory->SetCharacterCombatHotbarBindingFromItem(0, 0, Potion, EGridEquipmentSlot::None));

	Character.DerivedStats.CurrentHealth = 20;
	Character.DerivedStats.CurrentMana = 8;
	Fixture.Hud->RefreshFromSources();
	TestFalse(TEXT("A full-health character cannot waste the potion"), Fixture.Hud->View.Actions[0].Action.bEnabled);
	TestEqual(TEXT("The disabled reason identifies a useless effect"), Fixture.Hud->View.Actions[0].Action.AvailabilityReason,
		EGridCombatActionAvailabilityReason::NoApplicableEffect);
	FGridCombatActionRequestResult RefusedUse;
	TestFalse(TEXT("The useless potion request is rejected"), Fixture.Hud->RequestHotbarSlot(0, RefusedUse));
	TestEqual(TEXT("A refused potion consumes no unit"), Inventory->CountItemDefinitionInCharacterInventory(0, PotionDefinition->ItemDefinitionId), 1);
	TestEqual(TEXT("A refused potion consumes no action point"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 3);
	TestTrue(TEXT("A refused potion keeps its shortcut"), Fixture.Hud->View.Actions[0].bHasBinding);

	Character.DerivedStats.CurrentHealth = 10;
	FGridCombatActionRequestResult LastUse;
	TestTrue(TEXT("The last potion unit can be consumed"), Fixture.Hud->RequestHotbarSlot(0, LastUse));
	TestEqual(TEXT("The inventory quantity reaches zero"), Inventory->CountItemDefinitionInCharacterInventory(0, PotionDefinition->ItemDefinitionId), 0);
	FGridCombatHotbarBinding PersistentBinding;
	TestTrue(TEXT("The exhausted shortcut remains readable"), Inventory->GetCharacterCombatHotbarBinding(0, 0, PersistentBinding));
	TestTrue(TEXT("The last potion clears its shortcut"), PersistentBinding.IsEmpty());
	TestFalse(TEXT("The exhausted HUD slot has no binding"), Fixture.Hud->View.Actions[0].bHasBinding);
	TestFalse(TEXT("The exhausted HUD slot no longer resolves"), Fixture.Hud->View.Actions[0].bResolved);

	FGridItemInstance ReplacementPotion = Potion;
	ReplacementPotion.RuntimeObjectId = FGuid(12, 8, 4, 2);
	ReplacementPotion.Quantity = 3;
	TestTrue(TEXT("A replacement stack can be added"), Inventory->AddItemToCharacterInventory(0, ReplacementPotion));
	Fixture.Hud->RefreshFromSources();
	TestFalse(TEXT("A replacement stack does not restore the old shortcut"), Fixture.Hud->View.Actions[0].bHasBinding);
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
	TestTrue(TEXT("The consumed scroll shortcut remains readable"), Inventory->GetCharacterCombatHotbarBinding(0, 1, ConsumedScrollBinding));
	TestTrue(TEXT("An accepted scroll clears its shortcut"), ConsumedScrollBinding.IsEmpty());
	TestFalse(TEXT("The consumed scroll disappears from the HUD slot"), Fixture.Hud->View.Actions[1].bHasBinding);
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
	Fixture.Hud->ActionWidgetClass = UGridCombatHudActionWidget::StaticClass();
	UWrapBox* PalettePanel = NewObject<UWrapBox>(Fixture.Hud, TEXT("Panel_ActionPalette_Test"));
	Fixture.Hud->Panel_ActionPalette = PalettePanel;
	Fixture.Hud->RefreshFromSources();

	TestEqual(TEXT("Class actions and unarmed appear in the palette"), Fixture.Hud->View.ActionPalette.Num(), 3);
	TestEqual(TEXT("The palette creates one widget per assignable action"), Fixture.Hud->ActionPaletteWidgets.Num(), 3);
	TestEqual(TEXT("The optional palette panel owns all widgets"), PalettePanel->GetChildrenCount(), 3);
	const FGridAvailableCombatAction* SpellAction = FindPaletteAction(Fixture.Hud, TEXT("Spell_MON1285_ArcaneBolt"));
	if (!TestNotNull(TEXT("The direct spell is present"), SpellAction))
	{
		return false;
	}

	UGridCombatHotbarDragDropOperation* PaletteDrag = NewObject<UGridCombatHotbarDragDropOperation>(Fixture.Hud);
	PaletteDrag->InitializeFromActionPalette(0, *SpellAction);
	TestTrue(TEXT("A palette action can configure shortcut five"), Fixture.Hud->HandleHotbarDrop(4, PaletteDrag));

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
	const FGridAvailableCombatAction* SpellAction = FindPaletteAction(Fixture.Hud, TEXT("Spell_MON1285_ArcaneBolt"));
	if (!TestNotNull(TEXT("The spell is catalogued for the active mage"), SpellAction))
	{
		return false;
	}
	TestTrue(TEXT("The spell can be assigned without an item source"), Fixture.Hud->AssignCombatActionToHotbarSlot(2, *SpellAction));

	Fixture.Party->Facing = EGridEdge::South;
	FGridCombatActionRequestResult Rejected;
	TestFalse(TEXT("A spell without an axial target is rejected"), Fixture.Hud->RequestHotbarSlot(2, Rejected));
	TestEqual(TEXT("The rejected spell restores its reserved mana"), Character.DerivedStats.CurrentMana, 8);
	TestEqual(TEXT("The rejected spell consumes no action points"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 4);

	Fixture.Party->Facing = EGridEdge::North;
	FGridCombatActionRequestResult Accepted;
	TestTrue(TEXT("The direct axial spell is accepted"), Fixture.Hud->RequestHotbarSlot(2, Accepted));
	TestTrue(TEXT("The generic result reports spell acceptance"), Accepted.bAccepted);
	TestEqual(TEXT("The spell pays exactly three mana"), Character.DerivedStats.CurrentMana, 5);
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
	Character.DerivedStats.CurrentHealth = 10;
	Character.DerivedStats.CurrentMana = 8;
	Fixture.Hud->RefreshFromSources();
	const FGridAvailableCombatAction* AbilityAction = FindPaletteAction(Fixture.Hud, TEXT("Ability_MON1285_Recovery"));
	if (!TestNotNull(TEXT("The ability is catalogued for the active priest"), AbilityAction))
	{
		return false;
	}
	TestTrue(TEXT("The recovery ability can configure shortcut four"), Fixture.Hud->AssignCombatActionToHotbarSlot(3, *AbilityAction));

	FGridCombatActionRequestResult Accepted;
	TestTrue(TEXT("The self-targeted ability is accepted"), Fixture.Hud->RequestHotbarSlot(3, Accepted));
	TestEqual(TEXT("The ability restores six health"), Character.DerivedStats.CurrentHealth, 16);
	TestEqual(TEXT("The ability pays two mana"), Character.DerivedStats.CurrentMana, 6);
	TestEqual(TEXT("The ability pays one action point"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 3);
	TestEqual(TEXT("The class result records restored health"), Accepted.ClassActionResult.HealthAfter, 16);

	Character.DerivedStats.CurrentHealth = 20;
	Fixture.Hud->RefreshFromSources();
	TestEqual(TEXT("A useless recovery is disabled"), Fixture.Hud->View.Actions[3].Action.AvailabilityReason,
		EGridCombatActionAvailabilityReason::NoApplicableEffect);
	FGridCombatActionRequestResult Rejected;
	TestFalse(TEXT("A useless recovery cannot be spent"), Fixture.Hud->RequestHotbarSlot(3, Rejected));
	TestEqual(TEXT("The refused ability consumes no mana"), Character.DerivedStats.CurrentMana, 6);
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
	const FGridAvailableCombatAction* CellSpell = FindPaletteAction(Fixture.Hud, TEXT("Spell_MON1286_CellStrike"));
	if (!TestNotNull(TEXT("The cell spell is catalogued"), CellSpell))
	{
		return false;
	}
	TestTrue(TEXT("The cell spell can configure shortcut six"), Fixture.Hud->AssignCombatActionToHotbarSlot(5, *CellSpell));

	FGridCombatActionRequestResult Pending;
	TestTrue(TEXT("The shortcut opens explicit targeting"), Fixture.Hud->RequestHotbarSlot(5, Pending));
	TestTrue(TEXT("Targeting remains active until confirmation"), Fixture.Hud->IsCombatActionTargetingActive());
	TestFalse(TEXT("Opening targeting does not resolve the spell"), Pending.bAccepted);
	TestEqual(TEXT("Opening targeting spends no mana"), Character.DerivedStats.CurrentMana, 8);
	TestEqual(TEXT("Opening targeting spends no action points"), Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 4);

	TestFalse(TEXT("An empty cell is not a valid attack target"), Fixture.Hud->UpdateCombatActionTargetingPreview(FIntPoint(0, 0)));
	TestTrue(TEXT("Invalid hover keeps targeting active"), Fixture.Hud->IsCombatActionTargetingActive());
	Fixture.Hud->CancelCombatActionTargeting();
	TestFalse(TEXT("Cancellation closes targeting"), Fixture.Hud->IsCombatActionTargetingActive());
	TestEqual(TEXT("Cancellation still spends no mana"), Character.DerivedStats.CurrentMana, 8);

	TestTrue(TEXT("The shortcut can reopen targeting"), Fixture.Hud->RequestHotbarSlot(5, Pending));
	TestTrue(TEXT("The occupied monster cell previews as valid"), Fixture.Hud->UpdateCombatActionTargetingPreview(FIntPoint(1, 2)));
	TestEqual(TEXT("A cell spell covers exactly one cell"), Fixture.Hud->TargetingPreview.AffectedCells.Num(), 1);
	TestEqual(TEXT("A cell spell identifies exactly one monster"), Fixture.Hud->TargetingPreview.TargetMonsterIds.Num(), 1);

	FGridCombatActionRequestResult Accepted;
	TestTrue(TEXT("The selected monster cell is accepted"), Fixture.Hud->ConfirmCombatActionTarget(FIntPoint(1, 2), Accepted));
	TestFalse(TEXT("Accepted execution closes targeting"), Fixture.Hud->IsCombatActionTargetingActive());
	TestTrue(TEXT("The targeted result is accepted"), Accepted.bAccepted);
	TestEqual(TEXT("One targeted attack is resolved"), Accepted.TargetedActionResult.AttackResults.Num(), 1);
	TestEqual(TEXT("The cell spell pays exactly three mana"), Character.DerivedStats.CurrentMana, 5);
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
	const FGridAvailableCombatAction* AreaSpell = FindPaletteAction(Fixture.Hud, TEXT("Spell_MON1286_AreaBurst"));
	if (!TestNotNull(TEXT("The area spell is catalogued"), AreaSpell))
	{
		return false;
	}
	TestTrue(TEXT("The area spell can configure shortcut seven"), Fixture.Hud->AssignCombatActionToHotbarSlot(6, *AreaSpell));

	FGridCombatActionRequestResult Pending;
	TestTrue(TEXT("The area shortcut opens targeting"), Fixture.Hud->RequestHotbarSlot(6, Pending));
	TestTrue(TEXT("The area centered on the first monster is valid"), Fixture.Hud->UpdateCombatActionTargetingPreview(FIntPoint(1, 2)));
	TestEqual(TEXT("The radius-one diamond contains four valid cells"), Fixture.Hud->TargetingPreview.AffectedCells.Num(), 4);
	TestEqual(TEXT("Both monsters are included in the preview"), Fixture.Hud->TargetingPreview.TargetMonsterIds.Num(), 2);

	FGridCombatActionRequestResult Accepted;
	TestTrue(TEXT("The area spell is accepted"), Fixture.Hud->ConfirmCombatActionTarget(FIntPoint(1, 2), Accepted));
	TestEqual(TEXT("Two attacks are resolved by one area action"), Accepted.TargetedActionResult.AttackResults.Num(), 2);
	TestEqual(TEXT("Both target identities are retained"), Accepted.TargetedActionResult.TargetMonsterIds.Num(), 2);
	TestEqual(TEXT("The area spell pays mana only once"), Character.DerivedStats.CurrentMana, 5);
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
	const FGridAvailableCombatAction* AreaSpell = FindPaletteAction(Fixture.Hud, TEXT("Spell_MON1286_AreaBurst"));
	if (!TestNotNull(TEXT("The rejected area spell is catalogued"), AreaSpell))
	{
		return false;
	}
	const FGridAvailableCombatAction& Action = *AreaSpell;

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
	TestEqual(TEXT("Rejected targeting spends no mana"), Character.DerivedStats.CurrentMana, 8);
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
	UHorizontalBox* HotbarPanel = NewObject<UHorizontalBox>(Fixture.Hud, TEXT("Panel_Actions_1287"));
	UCanvasPanelSlot* HotbarCanvasSlot = RootPanel->AddChildToCanvas(HotbarPanel);
	HotbarCanvasSlot->SetAnchors(FAnchors(0.5f, 1.0f));
	HotbarCanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
	HotbarCanvasSlot->SetPosition(FVector2D(0.0f, -24.0f));
	HotbarCanvasSlot->SetSize(FVector2D(700.0f, 180.0f));
	Fixture.Hud->Panel_CombatHud = RootPanel;
	Fixture.Hud->Panel_Actions = HotbarPanel;
	Fixture.Hud->Panel_ActionPalette = nullptr;
	Fixture.Hud->Panel_Initiative = NewObject<UHorizontalBox>(Fixture.Hud, TEXT("Panel_Initiative_1287"));
	Fixture.Hud->Button_EndTurn = NewObject<UButton>(Fixture.Hud, TEXT("Button_EndTurn_1287"));
	Fixture.Hud->ActionWidgetClass = UGridCombatHudActionWidget::StaticClass();

	Fixture.TurnManager->bCombatActive = false;
	Fixture.TurnManager->CurrentPhase = EGridCombatPhase::Exploration;
	TestTrue(TEXT("The third character can be selected out of combat"), Fixture.Party->PartyInventoryComponent->SetSelectedCharacterIndex(2));
	Fixture.Hud->RefreshFromSources();

	TestFalse(TEXT("The encounter is inactive"), Fixture.Hud->View.bCombatActive);
	TestEqual(TEXT("The selected character owns the out-of-combat bar"), Fixture.Hud->View.ActiveCharacterIndex, 2);
	TestEqual(TEXT("All ten slots remain projected out of combat"), Fixture.Hud->View.Actions.Num(), FGridCombatHotbarBinding::SlotCount);
	TestEqual(TEXT("The HUD root remains visible out of combat"), RootPanel->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("The fixed hotbar remains visible out of combat"), HotbarPanel->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	TestEqual(TEXT("Combat-only initiative is hidden out of combat"), Fixture.Hud->Panel_Initiative->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Combat-only end turn is hidden out of combat"), Fixture.Hud->Button_EndTurn->GetVisibility(), ESlateVisibility::Collapsed);
	TestNotNull(TEXT("A missing WBP palette receives a runtime fallback"), Fixture.Hud->Panel_ActionPalette.Get());
	TestEqual(TEXT("The fallback palette owns its runtime entries"), Fixture.Hud->ActionPaletteWidgets.Num(), Fixture.Hud->View.ActionPalette.Num());

	const FGridAvailableCombatAction* UnarmedAction = Fixture.Hud->View.ActionPalette.FindByPredicate(
		[](const FGridAvailableCombatAction& Candidate)
		{
			return Candidate.Definition.ActionId == FName(TEXT("Attack_Unarmed")) &&
				Candidate.Definition.SourcePolicy == EGridCombatActionSourcePolicy::Universal;
		});
	if (!TestNotNull(TEXT("Unarmed remains available in the palette"), UnarmedAction))
	{
		return false;
	}
	TestTrue(TEXT("Unarmed can be assigned while combat is inactive"), Fixture.Hud->AssignCombatActionToHotbarSlot(4, *UnarmedAction));
	TestTrue(TEXT("The unarmed binding remains resolved out of combat"), Fixture.Hud->View.Actions[4].bResolved);
	TestFalse(TEXT("The resolved binding cannot execute outside combat"), Fixture.Hud->View.Actions[4].Action.bEnabled);
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
	ShurikenDefinition->bProvidesAttack = true;
	ShurikenDefinition->bThrowable = true;
	ShurikenDefinition->bProvidesAttackPresentation = true;
	ShurikenDefinition->PlayerAttackPresentationProfile.MotionStyle = EGridPlayerAttackMotionStyle::Throw;
	ShurikenDefinition->PlayerAttackPresentationProfile.bAnimateHeldItem = true;
	ShurikenDefinition->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::MainHand);
	ShurikenDefinition->OffensiveProfile.AttackId = TEXT("Attack_Shuriken_MON1287");
	ShurikenDefinition->OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Physical;
	ShurikenDefinition->OffensiveProfile.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
	ShurikenDefinition->OffensiveProfile.AttackDefinition.MinDamage = 2;
	ShurikenDefinition->OffensiveProfile.AttackDefinition.MaxDamage = 2;
	ShurikenDefinition->OffensiveProfile.AttackDefinition.AccuracyBonus = 100;
	ShurikenDefinition->OffensiveProfile.RangeCells = 3;
	FGridCombatActionDefinition MisconfiguredThrow;
	MisconfiguredThrow.ActionId = TEXT("Attack_Shuriken_MON1287_Configured");
	MisconfiguredThrow.ActionType = EGridCombatActionType::RangedAttack;
	MisconfiguredThrow.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
	MisconfiguredThrow.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
	MisconfiguredThrow.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
	MisconfiguredThrow.ActionPointCost = 2;
	MisconfiguredThrow.ResourceCosts.SourceItemQuantityCost = 2;
	MisconfiguredThrow.RangeCells = 3;
	MisconfiguredThrow.OffensiveProfile = ShurikenDefinition->OffensiveProfile;
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
	TestTrue(TEXT("The empty rollover explains assignment"), EmptyWidget->Button_Action->GetToolTipText().ToString().Contains(TEXT("Déposez ici une arme")));

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

	UGridCombatHudActionWidget* PaletteWidget = NewObject<UGridCombatHudActionWidget>();
	PaletteWidget->Button_Action = NewObject<UButton>(PaletteWidget);
	PaletteWidget->InitializePaletteAction(nullptr, AvailableAction);
	const FString PaletteToolTip = PaletteWidget->Button_Action->GetToolTipText().ToString();
	TestTrue(TEXT("The palette rollover contains the action name"), PaletteToolTip.Contains(TEXT("Shuriken")));
	TestTrue(TEXT("The palette rollover contains the complete cost"), PaletteToolTip.Contains(TEXT("Coût : 2 PA — quantité : 4")));
	TestTrue(TEXT("The palette rollover explains drag assignment"), PaletteToolTip.Contains(TEXT("Glissez cette action vers un raccourci.")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1210ActionPaletteTargetingTest, "Grimrock.Monsters.MON12.10.ActionPaletteTargeting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1210ActionPaletteTargetingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatHudFixture Fixture;
	if (!TestTrue(TEXT("The MON12.10 fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}

	Fixture.Hud->Panel_ActionPalette = NewObject<UHorizontalBox>(Fixture.Hud);
	Fixture.Hud->Panel_Targeting = NewObject<UVerticalBox>(Fixture.Hud);
	Fixture.Hud->Text_TargetingInstructions = NewObject<UTextBlock>(Fixture.Hud);
	Fixture.Hud->Text_TargetingCell = NewObject<UTextBlock>(Fixture.Hud);

	URPGClassAsset* MageClass = NewObject<URPGClassAsset>(Fixture.Party);
	MageClass->ClassId = TEXT("Mage_MON1210");
	MageClass->HealthAtLevelOne = 6;
	MageClass->CombatActions = { MakeMON1286AreaSpell() };
	FGridCharacterInventoryState& Character = Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0];
	Character.ClassId = MageClass->ClassId;
	Character.ClassDefinition = MageClass;
	Fixture.Hud->RefreshFromSources();

	const FGridAvailableCombatAction* AreaSpell = FindPaletteAction(Fixture.Hud, TEXT("Spell_MON1286_AreaBurst"));
	if (!TestNotNull(TEXT("The synthetic area spell is catalogued"), AreaSpell))
	{
		return false;
	}
	TestTrue(TEXT("The area spell configures shortcut seven"), Fixture.Hud->AssignCombatActionToHotbarSlot(6, *AreaSpell));
	TestEqual(TEXT("The palette is visible before targeting"), Fixture.Hud->Panel_ActionPalette->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("The targeting panel is initially hidden"), Fixture.Hud->Panel_Targeting->GetVisibility(), ESlateVisibility::Collapsed);

	FGridCombatActionRequestResult Pending;
	TestTrue(TEXT("The shortcut opens area targeting"), Fixture.Hud->RequestHotbarSlot(6, Pending));
	TestEqual(TEXT("Targeting replaces the palette"), Fixture.Hud->Panel_ActionPalette->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("The targeting panel is visible"), Fixture.Hud->Panel_Targeting->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	const FString Instructions = Fixture.Hud->Text_TargetingInstructions->GetText().ToString();
	TestTrue(TEXT("The instructions name the action"), Instructions.Contains(TEXT("Explosion arcanique")));
	TestTrue(TEXT("The instructions explain cancellation"), Instructions.Contains(TEXT("Échap : annuler")));

	TestFalse(TEXT("An empty area is invalid"), Fixture.Hud->UpdateCombatActionTargetingPreview(FIntPoint(0, 0)));
	const FString InvalidStatus = Fixture.Hud->Text_TargetingCell->GetText().ToString();
	TestFalse(TEXT("Invalid feedback exposes no cell coordinates"), InvalidStatus.Contains(TEXT("(0,0)")));

	TestTrue(TEXT("The monster area previews as valid"), Fixture.Hud->UpdateCombatActionTargetingPreview(FIntPoint(1, 2)));
	const FString ValidStatus = Fixture.Hud->Text_TargetingCell->GetText().ToString();
	TestTrue(TEXT("Valid feedback reports affected enemies"), ValidStatus.Contains(TEXT("Cible valide")));
	TestFalse(TEXT("Valid feedback exposes no cell coordinates"), ValidStatus.Contains(TEXT("(1,2)")));

	Fixture.Hud->CancelCombatActionTargeting();
	TestEqual(TEXT("Cancellation restores the palette"), Fixture.Hud->Panel_ActionPalette->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Cancellation hides the targeting panel"), Fixture.Hud->Panel_Targeting->GetVisibility(), ESlateVisibility::Collapsed);
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

	TestTrue(TEXT("The sword configures slot one"), Inventory->SetCharacterCombatHotbarBindingFromItem(0, 0, EquippedSword, EGridEquipmentSlot::MainHand));
	FGridCombatHotbarBinding UnarmedBinding;
	UnarmedBinding.Reset(1);
	UnarmedBinding.ActionId = TEXT("Attack_Unarmed");
	UnarmedBinding.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
	TestTrue(TEXT("Unarmed configures slot two"), Inventory->SetCharacterCombatHotbarBinding(0, 1, UnarmedBinding));

	FGridCombatHotbarBinding SwordBinding;
	TestTrue(TEXT("The sword binding is readable before the swap"), Inventory->GetCharacterCombatHotbarBinding(0, 0, SwordBinding));
	UGridCombatHotbarDragDropOperation* SwapOperation = NewObject<UGridCombatHotbarDragDropOperation>(Fixture.Hud);
	SwapOperation->InitializeFromHotbarSlot(0, 0, SwordBinding);
	TestTrue(TEXT("Dropping onto an occupied slot swaps bindings"), Fixture.Hud->HandleHotbarDrop(1, SwapOperation));

	FGridCombatHotbarBinding SwappedFirst;
	FGridCombatHotbarBinding SwappedSecond;
	Inventory->GetCharacterCombatHotbarBinding(0, 0, SwappedFirst);
	Inventory->GetCharacterCombatHotbarBinding(0, 1, SwappedSecond);
	TestEqual(TEXT("The first slot now contains unarmed"), SwappedFirst.ActionId, FName(TEXT("Attack_Unarmed")));
	TestTrue(TEXT("The second slot keeps the exact sword instance"), SwappedSecond.PreferredSourceRuntimeId == EquippedSword.RuntimeObjectId);

	Fixture.TurnManager->PlayerAttackActionPointCost = 5;
	Fixture.Hud->RefreshFromSources();
	FGridPlayerCharacterTurnState TurnStateBeforeRefusal;
	Fixture.TurnManager->GetPlayerCharacterTurnState(0, TurnStateBeforeRefusal);
	FGridCombatActionRequestResult Rejected;
	TestFalse(TEXT("An unavailable sword shortcut is refused"), Fixture.Hud->RequestHotbarSlot(1, Rejected));
	FGridCombatHotbarBinding BindingAfterRefusal;
	Inventory->GetCharacterCombatHotbarBinding(0, 1, BindingAfterRefusal);
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
	RestoredInventory->GetCharacterCombatHotbarBinding(0, 1, RestoredSword);
	TestTrue(TEXT("Restore keeps the exact sword identity"), RestoredSword.PreferredSourceRuntimeId == EquippedSword.RuntimeObjectId);

	TestTrue(TEXT("Right-click contract clears the shortcut"), Fixture.Hud->ClearHotbarSlot(1));
	FGridCombatHotbarBinding ClearedBinding;
	Inventory->GetCharacterCombatHotbarBinding(0, 1, ClearedBinding);
	TestTrue(TEXT("The cleared shortcut is empty"), ClearedBinding.IsEmpty());
	FGridItemInstance SwordAfterClear;
	TestTrue(TEXT("Clearing never removes the equipped source"), Inventory->GetEquippedItem(0, EGridEquipmentSlot::MainHand, SwordAfterClear));
	TestTrue(TEXT("The equipped source identity is unchanged"), SwordAfterClear.RuntimeObjectId == EquippedSword.RuntimeObjectId);

	TestTrue(TEXT("The real initiative flow ends character one's turn"), Fixture.TurnManager->EndActivePlayerTurn());
	TestEqual(TEXT("The HUD projects character two"), Fixture.Hud->View.ActiveCharacterIndex, 1);
	TestFalse(TEXT("Character two does not inherit character one's slots"), Fixture.Hud->View.Actions[1].bHasBinding);

	Fixture.TurnManager->FinishCombat(EGridCombatPhase::Victory);
	Fixture.Hud->RefreshFromSources();
	FGridCombatHotbarBinding PersistentUnarmed;
	Inventory->GetCharacterCombatHotbarBinding(0, 0, PersistentUnarmed);
	TestEqual(TEXT("Combat finish preserves configured assignments"), PersistentUnarmed.ActionId, FName(TEXT("Attack_Unarmed")));
	TestFalse(TEXT("The hotbar cannot execute after combat"), Fixture.Hud->View.Actions[0].Action.bEnabled);
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
	ShurikenDefinition->bProvidesAttack = true;
	ShurikenDefinition->bThrowable = true;
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
	ShurikenDefinition->OffensiveProfile = ShurikenAction.OffensiveProfile;
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
	TestEqual(TEXT("The equipment mana cost is paid"), Inventory->PartyInventoryState.ActiveCharacters[0].DerivedStats.CurrentMana, 6);
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
	const int32 ManaBeforeRefusal = Inventory->PartyInventoryState.ActiveCharacters[0].DerivedStats.CurrentMana;
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
	TestEqual(TEXT("A rejected action keeps mana"), Inventory->PartyInventoryState.ActiveCharacters[0].DerivedStats.CurrentMana, ManaBeforeRefusal);
	TestEqual(TEXT("A rejected action keeps its equipment source"), Inventory->PartyInventoryState.ActiveEquipment[0].MainHand.Quantity, QuantityBeforeRefusal);
	TestEqual(TEXT("A rejected action cannot damage its target"), Fixture.Monster->CurrentHealth, MonsterHealthBeforeRefusal);

	TurnManager->FinishCombat(EGridCombatPhase::Victory);
	TestFalse(TEXT("The real combat finish deactivates combat"), TurnManager->bCombatActive);
	TestEqual(
		TEXT("Combat finish clears transient cooldowns"), TurnManager->GetRemainingCombatActionCooldown(Fixture.CharacterIds[0], ShurikenAction.ActionId), 0);
	return true;
}

#endif

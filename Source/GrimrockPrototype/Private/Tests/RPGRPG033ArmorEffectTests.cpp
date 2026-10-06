#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/StatusEffects/GridCombatStatusApplicationResolver.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/Combat/GridCombatArmorEffectResolver.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace RPG033
{
	FGridCombatArmorEffectProfile MakeRestore(
		EGridCombatArmorPool Pool, EGridCombatArmorEffectMagnitude Magnitude, int32 Amount)
	{
		FGridCombatArmorEffectProfile Profile;
		Profile.Pool = Pool;
		Profile.Operation = EGridCombatArmorEffectOperation::Restore;
		Profile.Magnitude = Magnitude;
		Profile.Trigger = EGridCombatArmorEffectTrigger::AfterResolution;
		Profile.Amount = Amount;
		return Profile;
	}

	FGridCombatArmorEffectProfile MakeAttackDamage(
		EGridCombatArmorPool Pool, EGridCombatArmorEffectMagnitude Magnitude, int32 Amount)
	{
		FGridCombatArmorEffectProfile Profile;
		Profile.Pool = Pool;
		Profile.Operation = EGridCombatArmorEffectOperation::Damage;
		Profile.Magnitude = Magnitude;
		Profile.Trigger = EGridCombatArmorEffectTrigger::AfterSuccessfulHit;
		Profile.Amount = Amount;
		return Profile;
	}

	FGridCombatActionDefinition MakeSelfArmorAction(FName ActionId, int32 RestoreMagicalArmor)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = ActionId;
		Action.DisplayName = FText::FromName(ActionId);
		Action.Description = FText::FromName(ActionId);
		Action.ActionType = EGridCombatActionType::Ability;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
		Action.TargetingPolicy = EGridCombatTargetingPolicy::Self;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Action.ActionPointCost = 1;
		Action.ArmorEffects.Add(MakeRestore(EGridCombatArmorPool::Magical, EGridCombatArmorEffectMagnitude::Flat, RestoreMagicalArmor));
		return Action;
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
				FName(*FString::Printf(TEXT("RPG033World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num, &Values);
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

	struct FSelfActionFixture
	{
		FTestWorld TestWorld;
		AGridLevelRuntimeActor* Runtime = nullptr;
		AGrimrockPartyPawn* Party = nullptr;
		AActor* Owner = nullptr;
		UGridTurnManagerComponent* TurnManager = nullptr;
		URPGClassAsset* Class = nullptr;
		FGuid CharacterId = FGuid(3, 3, 1, 1);

		FSelfActionFixture()
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

			Class = NewObject<URPGClassAsset>(GetTransientPackage());
			Class->ClassId = TEXT("RPG033_ArmorClass");
			Class->HealthAtLevelOne = 10;
			Class->BasePhysicalArmor = 0;
			Class->BaseMagicalArmor = 10;
			Class->CombatActions = { MakeSelfArmorAction(TEXT("RPG033_ArcaneShield"), 4) };

			FGridCharacterInventoryState Character;
			Character.CharacterId = CharacterId;
			Character.DisplayName = FText::FromString(TEXT("RPG03.3 Hero"));
			Character.ClassId = Class->ClassId;
			Character.ClassDefinition = TSoftObjectPtr<URPGClassAsset>(Class);
			Character.DerivedStats.MaxHealth = 10;
			Character.Resources.CurrentHealth = 10;
			Character.Resources.CurrentMagicalArmor = 2;
			Character.InventorySlots.SetNum(4);
			Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { Character };
			Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment.SetNum(1);

			Party->LevelRuntimeActor = Runtime;
			Party->CurrentCellX = 1;
			Party->CurrentCellY = 1;
			Party->Facing = EGridEdge::North;
			Party->SnapToCurrentCell();

			TurnManager = NewObject<UGridTurnManagerComponent>(Owner, TEXT("RPG033TurnManager"));
			if (!TurnManager || !TurnManager->InitializeTurnManager(Runtime, Party))
			{
				TurnManager = nullptr;
				return;
			}

			TurnManager->bCombatActive = true;
			TurnManager->CurrentPhase = EGridCombatPhase::PlayerPhase;
			TurnManager->RoundNumber = 1;

			FGridCombatantInitiativeEntry Entry;
			Entry.CombatantId = CharacterId;
			Entry.Side = EGridCombatantSide::Party;
			Entry.CharacterIndex = 0;
			Entry.DisplayName = Character.DisplayName;
			Entry.InitiativeBase = 10;
			Entry.InitiativeRoll = 10;
			Entry.InitiativeTotal = 20;
			Entry.CurrentHealth = 10;
			Entry.MaximumHealth = 10;
			Entry.State = EGridCombatantTurnState::Active;
			TurnManager->InitiativeOrder = { Entry };
			TurnManager->CurrentInitiativeIndex = 0;

			FGridPlayerCharacterTurnState TurnState;
			TurnState.CharacterIndex = 0;
			TurnState.CharacterId = CharacterId;
			TurnState.State = EGridCombatantTurnState::Active;
			TurnState.MaximumActionPoints = 4;
			TurnState.RemainingActionPoints = 4;
			TurnManager->PlayerCharacterTurnStates = { TurnState };
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG033ProfileValidationTest, "Grimrock.RPG.RPG03.3.ProfileValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG033ProfileValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatArmorEffectProfile Restore =
		RPG033::MakeRestore(EGridCombatArmorPool::Magical, EGridCombatArmorEffectMagnitude::Flat, 6);
	TestTrue(TEXT("Flat restoration profile is valid"), Restore.IsValid());

	FGridCombatArmorEffectProfile RawDamage =
		RPG033::MakeAttackDamage(EGridCombatArmorPool::Physical, EGridCombatArmorEffectMagnitude::RawDamagePercent, 50);
	TestTrue(TEXT("Raw-damage armor damage profile is valid"), RawDamage.IsValid());

	RawDamage.Operation = EGridCombatArmorEffectOperation::Restore;
	TestFalse(TEXT("Raw-damage restoration is rejected"), RawDamage.IsValid());

	FGridCombatActionDefinition SelfAction = RPG033::MakeSelfArmorAction(TEXT("RPG033_Self"), 4);
	TestTrue(TEXT("Armor-only self Effect action is valid"), SelfAction.IsValid());

	SelfAction.ArmorEffects[0].Operation = EGridCombatArmorEffectOperation::Damage;
	TestTrue(TEXT("Effect action accepts direct Flat armor damage"), SelfAction.IsValid());
	SelfAction.ArmorEffects[0].Magnitude = EGridCombatArmorEffectMagnitude::RawDamagePercent;
	TestFalse(TEXT("Effect action rejects RawDamagePercent without an attack result"), SelfAction.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG033FlatRestoreClampTest, "Grimrock.RPG.RPG03.3.FlatRestoreClamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG033FlatRestoreClampTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatArmorPoolSnapshot Snapshot;
	Snapshot.CurrentMagicalArmor = 8;
	Snapshot.ReferenceMagicalArmor = 10;

	FGridResolvedCombatModifiers Modifiers;
	TArray<FGridCombatArmorEffectResult> Results;
	TestEqual(TEXT("One restore profile mutates"),
		FGridCombatArmorEffectResolver::ApplyRestoreEffects(
			{ RPG033::MakeRestore(EGridCombatArmorPool::Magical, EGridCombatArmorEffectMagnitude::Flat, 6) }, Snapshot, Modifiers, nullptr, &Results),
		1);
	TestEqual(TEXT("Restoration clamps to reference pool"), Snapshot.CurrentMagicalArmor, 10);
	TestEqual(TEXT("Only missing armor is applied"), Results.Num() == 1 ? Results[0].AppliedAmount : 0, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG033ReferenceAndRestorationModifiersTest, "Grimrock.RPG.RPG03.3.ReferenceAndRestorationModifiers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG033ReferenceAndRestorationModifiersTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatArmorPoolSnapshot Snapshot;
	Snapshot.ReferencePhysicalArmor = 20;

	FGridResolvedCombatModifiers Modifiers;
	Modifiers.PhysicalArmorReferencePercentModifier = 25;
	Modifiers.PhysicalArmorRestorationPercentModifier = -20;
	FGridCombatArmorEffectResolver::ApplyReferenceModifiers(Snapshot, Modifiers);
	TestEqual(TEXT("Reference pool receives +25 percent"), Snapshot.ReferencePhysicalArmor, 25);

	TArray<FGridCombatArmorEffectResult> Results;
	FGridCombatArmorEffectResolver::ApplyRestoreEffects(
		{ RPG033::MakeRestore(EGridCombatArmorPool::Physical, EGridCombatArmorEffectMagnitude::ReferencePercent, 40) }, Snapshot, Modifiers, nullptr, &Results);
	TestEqual(TEXT("Forty percent reference restore reduced by twenty percent applies eight"), Results.Num() == 1 ? Results[0].RequestedAmount : 0, 8);
	TestEqual(TEXT("Incoming restoration penalty is applied"), Snapshot.CurrentPhysicalArmor, 8);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG033RawDamageNoOverflowTest, "Grimrock.RPG.RPG03.3.RawDamageNoOverflow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG033RawDamageNoOverflowTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatArmorPoolSnapshot Snapshot;
	Snapshot.CurrentPhysicalArmor = 5;
	Snapshot.ReferencePhysicalArmor = 10;

	FGridAttackResult Attack;
	Attack.bHit = true;
	Attack.RawDamage = 10;
	Attack.PhysicalArmorDamage = 3;
	Attack.TargetHealthBefore = 20;
	Attack.TargetHealthAfter = 20;

	FGridResolvedCombatModifiers Modifiers;
	TArray<FGridCombatArmorEffectResult> Results;
	TestEqual(TEXT("Armor-break profile mutates once"),
		FGridCombatArmorEffectResolver::ApplyAttackDamageEffects(
			{ RPG033::MakeAttackDamage(EGridCombatArmorPool::Physical, EGridCombatArmorEffectMagnitude::RawDamagePercent, 50) }, Snapshot, Modifiers, Attack, nullptr,
			&Results),
		1);
	TestEqual(TEXT("Bonus can consume only armor remaining after primary damage"), Results.Num() == 1 ? Results[0].AppliedAmount : 0, 2);
	TestEqual(TEXT("Total physical armor damage reaches exactly the available pool"), Attack.PhysicalArmorDamage, 5);
	TestEqual(TEXT("C3 never creates HP overflow damage"), Attack.HealthDamage, 0);
	TestEqual(TEXT("Target HP is unchanged by direct armor damage"), Attack.TargetHealthAfter, 20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG033ScaledFlatMagnitudeTest, "Grimrock.RPG.RPG03.3.ScaledFlatMagnitude",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG033ScaledFlatMagnitudeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatArmorEffectProfile Profile =
		RPG033::MakeRestore(EGridCombatArmorPool::Magical, EGridCombatArmorEffectMagnitude::Flat, 6);
	Profile.ScalingAttribute = EGridAttackScalingAttribute::Intelligence;
	Profile.AttributeModifierScale = 1;
	Profile.ScalingSkillId = TEXT("Skill_Arcana");
	Profile.SkillRankScale = 1;
	TestTrue(TEXT("Arcane-shield style scaled profile is valid"), Profile.IsValid());

	FGridCombatArmorEffectSourceContext Source;
	Source.Attributes.Intelligence = 16; // +3
	FRPGSkillRank Arcana;
	Arcana.SkillId = TEXT("Skill_Arcana");
	Arcana.Rank = 4;
	Source.SkillRanks.Add(Arcana);

	FGridCombatArmorPoolSnapshot Snapshot;
	Snapshot.ReferenceMagicalArmor = 20;
	FGridResolvedCombatModifiers Modifiers;
	TArray<FGridCombatArmorEffectResult> Results;
	TestEqual(TEXT("Scaled restoration mutates once"),
		FGridCombatArmorEffectResolver::ApplyRestoreEffects({ Profile }, Snapshot, Modifiers, &Source, &Results), 1);
	TestEqual(TEXT("6 + INT mod 3 + Arcana rank 4 = 13"), Snapshot.CurrentMagicalArmor, 13);
	TestEqual(TEXT("Resolved requested amount is inspectable"), Results.Num() == 1 ? Results[0].RequestedAmount : 0, 13);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG033ArmorGateAfterC3Test, "Grimrock.RPG.RPG03.3.ArmorGateAfterC3",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG033ArmorGateAfterC3Test::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatArmorPoolSnapshot Snapshot;
	Snapshot.CurrentPhysicalArmor = 5;
	Snapshot.ReferencePhysicalArmor = 5;

	FGridAttackResult Attack;
	Attack.bHit = true;
	Attack.RawDamage = 4;
	Attack.PhysicalArmorDamage = 3;
	Attack.TargetHealthBefore = 20;
	Attack.TargetHealthAfter = 20;

	FGridResolvedCombatModifiers Modifiers;
	FGridCombatArmorEffectResolver::ApplyAttackDamageEffects(
		{ RPG033::MakeAttackDamage(EGridCombatArmorPool::Physical, EGridCombatArmorEffectMagnitude::Flat, 2) }, Snapshot, Modifiers, Attack, nullptr);

	FGridCombatStatusApplicationProfile Status;
	Status.StatusEffectId = TEXT("Status_RPG033_Stunned");
	Status.Trigger = EGridCombatStatusApplicationTrigger::AfterSuccessfulHit;
	Status.ArmorGate = EGridCombatStatusArmorGate::PhysicalArmorDepleted;

	FGridAttackTargetStats TargetBefore;
	TargetBefore.CurrentHealth = 20;
	TargetBefore.PhysicalArmor = 5;
	TestTrue(TEXT("C1 sees armor depleted by primary damage plus C3"),
		FGridCombatStatusApplicationResolver::IsEligible(Status, TargetBefore, &Attack));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG033ArmorOnlyCatalogTest, "Grimrock.RPG.RPG03.3.ArmorOnlyCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG033ArmorOnlyCatalogTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatActionDefinition Action = RPG033::MakeSelfArmorAction(TEXT("RPG033_CatalogArmor"), 4);
	FGridCombatActionContribution Contribution;
	Contribution.Definition = Action;
	Contribution.SourceDefinitionId = TEXT("RPG033_Class");
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
	Context.CurrentMagicalArmor = 2;
	Context.ReferenceMagicalArmor = 10;

	TArray<FGridAvailableCombatAction> Actions;
	FGridCombatActionCatalog::Build(Context, { Contribution }, Actions);
	TestEqual(TEXT("Armor-only action is projected"), Actions.Num(), 1);
	TestTrue(TEXT("Missing armor makes action applicable"), Actions.Num() == 1 && Actions[0].bEnabled);

	Context.CurrentMagicalArmor = 10;
	FGridCombatActionCatalog::Build(Context, { Contribution }, Actions);
	TestEqual(TEXT("Full armor keeps action visible"), Actions.Num(), 1);
	TestEqual(TEXT("Full reference pool reports NoApplicableEffect"),
		Actions.Num() == 1 ? Actions[0].AvailabilityReason : EGridCombatActionAvailabilityReason::None,
		EGridCombatActionAvailabilityReason::NoApplicableEffect);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG033SelfActionExecutionTest, "Grimrock.RPG.RPG03.3.SelfActionExecution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG033SelfActionExecutionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	RPG033::FSelfActionFixture Fixture;
	if (!Fixture.TurnManager || !Fixture.Party || !Fixture.Party->PartyInventoryComponent || !Fixture.Class)
	{
		return false;
	}

	FGridPlayerCharacterTurnState BeforeTurn;
	TestTrue(TEXT("Turn state exists before armor action"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, BeforeTurn));

	FGridCombatActionRequestResult Result;
	TestTrue(TEXT("Armor-only class action executes"),
		Fixture.TurnManager->RequestCharacterCombatAction(
			0, TEXT("RPG033_ArcaneShield"), EGridCombatActionSourcePolicy::Ability, Fixture.Class->ClassId, EGridEquipmentSlot::None, Result));
	TestTrue(TEXT("Generic action result is accepted"), Result.bAccepted);
	TestEqual(TEXT("Armor result exposes before value"), Result.ClassActionResult.MagicalArmorBefore, 2);
	TestEqual(TEXT("Armor result exposes after value"), Result.ClassActionResult.MagicalArmorAfter, 6);
	TestEqual(TEXT("Durable magical armor is restored"),
		Fixture.Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Resources.CurrentMagicalArmor, 6);

	FGridPlayerCharacterTurnState AfterTurn;
	TestTrue(TEXT("Turn state exists after armor action"), Fixture.TurnManager->GetPlayerCharacterTurnState(0, AfterTurn));
	TestEqual(TEXT("Armor action spends one AP"), AfterTurn.RemainingActionPoints, BeforeTurn.RemainingActionPoints - 1);
	return true;
}

#endif

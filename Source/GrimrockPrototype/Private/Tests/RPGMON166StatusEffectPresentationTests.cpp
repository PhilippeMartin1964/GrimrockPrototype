#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"
#include "RPG/StatusEffects/GridStatusEffectPresentation.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "UI/GridCombatActionPanelWidget.h"

namespace
{
	UGridStatusEffectDefinitionAsset* MON166MakeDefinition(UObject* Outer, FName EffectId, const TCHAR* DisplayName,
		EGridStatusEffectDurationUnit DurationUnit = EGridStatusEffectDurationUnit::Turns, int32 Duration = 2,
		EGridStatusEffectStackPolicy StackPolicy = EGridStatusEffectStackPolicy::NoStack, int32 MaxStacks = 1)
	{
		UGridStatusEffectDefinitionAsset* Definition = NewObject<UGridStatusEffectDefinitionAsset>(Outer);
		Definition->EffectId = EffectId;
		Definition->DisplayName = FText::FromString(DisplayName);
		Definition->Description = FText::FromString(FString::Printf(TEXT("Description %s"), DisplayName));
		Definition->Disposition = EGridStatusEffectDisposition::Debuff;
		Definition->DurationUnit = DurationUnit;
		Definition->DefaultDuration = Duration;
		Definition->StackPolicy = StackPolicy;
		Definition->MaxStacks = MaxStacks;
		return Definition;
	}

	FGridCharacterInventoryState MON166MakeCharacter(const FGuid& Id)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = Id;
		Character.DisplayName = FText::FromString(TEXT("MON16.6 Hero"));
		Character.DerivedStats.CurrentHealth = 10;
		Character.DerivedStats.MaxHealth = 10;
		Character.DerivedStats.CurrentMana = 8;
		Character.DerivedStats.MaxMana = 8;
		Character.InventorySlots.SetNum(4);
		return Character;
	}

	FGridCombatantInitiativeEntry MON166PartyEntry(const FGuid& Id)
	{
		FGridCombatantInitiativeEntry Entry;
		Entry.CombatantId = Id;
		Entry.Side = EGridCombatantSide::Party;
		Entry.CharacterIndex = 0;
		Entry.DisplayName = FText::FromString(TEXT("MON16.6 Hero"));
		Entry.State = EGridCombatantTurnState::Active;
		Entry.CurrentHealth = 10;
		Entry.MaximumHealth = 10;
		return Entry;
	}

	struct FGridMON166TestWorld
	{
		UWorld* World = nullptr;

		FGridMON166TestWorld()
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
				FName(*FString::Printf(TEXT("MON166TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridMON166TestWorld()
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

	struct FGridMON166Fixture
	{
		FGridMON166TestWorld TestWorld;
		AGridLevelRuntimeActor* Runtime = nullptr;
		AGrimrockPartyPawn* Party = nullptr;
		UGridTurnManagerComponent* TurnManager = nullptr;
		UGridStatusEffectLifecycleSubsystem* Lifecycle = nullptr;
		AGridMonsterActor* Monster = nullptr;
		UGridMonsterDefinitionAsset* MonsterDefinition = nullptr;
		FGuid CharacterId = FGuid(16, 6, 1, 1);
		FGuid MonsterId = FGuid(16, 6, 2, 1);

		FGridMON166Fixture()
		{
			if (!TestWorld.World)
			{
				return;
			}

			Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
			Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
			if (!Runtime || !Party || !Party->PartyInventoryComponent)
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
			Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { MON166MakeCharacter(CharacterId) };
			Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment.SetNum(1);

			MonsterDefinition = NewObject<UGridMonsterDefinitionAsset>(Runtime);
			MonsterDefinition->MonsterId = TEXT("MON166_Monster");
			MonsterDefinition->DisplayName = FText::FromString(TEXT("MON16.6 Monster"));
			MonsterDefinition->CategoryId = TEXT("MON166");
			MonsterDefinition->MaxHealth = 20;
			MonsterDefinition->ActionPointsPerTurn = 2;
			Monster = TestWorld.World->SpawnActor<AGridMonsterActor>();
			if (Monster)
			{
				Monster->InitializeMonster(MonsterDefinition, MonsterId, FIntPoint(1, 2), EGridEdge::South);
			}

			TurnManager = NewObject<UGridTurnManagerComponent>(Runtime, TEXT("MON166TurnManager"));
			Runtime->AddInstanceComponent(TurnManager);
			TurnManager->RegisterComponent();
			if (!TurnManager->InitializeTurnManager(Runtime, Party))
			{
				TurnManager = nullptr;
				return;
			}
			TurnManager->bCombatActive = true;
			TurnManager->CurrentPhase = EGridCombatPhase::PlayerPhase;
			TurnManager->RoundNumber = 1;
			TurnManager->InitiativeOrder = { MON166PartyEntry(CharacterId) };
			TurnManager->CurrentInitiativeIndex = 0;
			if (Monster)
			{
				TurnManager->CombatMonsters = { Monster };
			}

			FGridPlayerCharacterTurnState TurnState;
			TurnState.CharacterIndex = 0;
			TurnState.CharacterId = CharacterId;
			TurnState.State = EGridCombatantTurnState::Active;
			TurnState.MaximumActionPoints = 4;
			TurnState.RemainingActionPoints = 4;
			TurnManager->PlayerCharacterTurnStates = { TurnState };

			Lifecycle = NewObject<UGridStatusEffectLifecycleSubsystem>(TestWorld.World);
			Lifecycle->BindToTurnManager(TurnManager);
		}

		~FGridMON166Fixture()
		{
			if (Lifecycle)
			{
				Lifecycle->UnbindFromTurnManager();
			}
		}

		FGridCharacterInventoryState* Character() const
		{
			return Party && Party->PartyInventoryComponent && Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex(0)
				? &Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0]
				: nullptr;
		}

		bool IsReady() const
		{
			return TestWorld.World && Runtime && Party && TurnManager && Lifecycle && Character() && Monster && MonsterDefinition;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON166PresentationProjectionTest, "Grimrock.RPG.MON16.6.PresentationProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON166PresentationProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridStatusEffectDefinitionAsset* Definition = MON166MakeDefinition(
		GetTransientPackage(), TEXT("MON166_Status"), TEXT("Venin"), EGridStatusEffectDurationUnit::Turns, 3, EGridStatusEffectStackPolicy::AddStacks, 3);
	Definition->Disposition = EGridStatusEffectDisposition::Debuff;
	Definition->InitiativeModifier = -2;
	Definition->PeriodicDamage.DamagePerStack = 1;
	Definition->Control.bBlockTranslation = true;
	UTexture2D* Icon = NewObject<UTexture2D>(GetTransientPackage());
	Definition->Icon = Icon;

	FGridStatusEffectCollection Collection;
	FGridStatusEffectApplyResult ApplyResult;
	FString Error;
	TestTrue(TEXT("Status applies"), Collection.TryApply(*Definition, FGuid::NewGuid(), 2, INDEX_NONE, INDEX_NONE, ApplyResult, Error));

	TArray<FGridStatusEffectPresentationView> Views;
	FGridStatusEffectPresentationBuilder::Build(Collection, Views);
	TestEqual(TEXT("One presentation view"), Views.Num(), 1);
	if (Views.Num() != 1)
	{
		return false;
	}
	TestEqual(TEXT("Effect id projected"), Views[0].EffectId, Definition->EffectId);
	TestEqual(TEXT("Display name projected"), Views[0].DisplayName.ToString(), FString(TEXT("Venin")));
	TestEqual(TEXT("Stacks projected"), Views[0].StackCount, 2);
	TestEqual(TEXT("Initiative is per stack"), Views[0].InitiativeContribution, -4);
	TestTrue(TEXT("Periodic flag projected"), Views[0].bPeriodicDamage);
	TestTrue(TEXT("Control flag projected"), Views[0].bBlockTranslation);
	TestTrue(TEXT("Icon projected"), Views[0].Icon.Get() == Icon);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON166DurationFormattingTest, "Grimrock.RPG.MON16.6.DurationFormatting", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON166DurationFormattingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestEqual(TEXT("Turns compact"), FGridStatusEffectPresentationBuilder::FormatCompactDuration(EGridStatusEffectDurationUnit::Turns, 2).ToString(),
		FString(TEXT("T2")));
	TestEqual(TEXT("Rounds singular"), FGridStatusEffectPresentationBuilder::FormatDuration(EGridStatusEffectDurationUnit::Rounds, 1).ToString(),
		FString(TEXT("1 manche")));
	TestEqual(TEXT("Permanent compact"), FGridStatusEffectPresentationBuilder::FormatCompactDuration(EGridStatusEffectDurationUnit::Permanent, 0).ToString(),
		FString(TEXT("PERM")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON166DeterministicProjectionTest, "Grimrock.RPG.MON16.6.DeterministicProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON166DeterministicProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridStatusEffectDefinitionAsset* Z = MON166MakeDefinition(GetTransientPackage(), TEXT("Z_Effect"), TEXT("Z"));
	UGridStatusEffectDefinitionAsset* A = MON166MakeDefinition(GetTransientPackage(), TEXT("A_Effect"), TEXT("A"));
	FGridStatusEffectCollection Collection;
	FString Error;
	TestTrue(TEXT("Z applies"), Collection.TryAdd(*Z, FGuid::NewGuid(), Error));
	TestTrue(TEXT("A applies"), Collection.TryAdd(*A, FGuid::NewGuid(), Error));
	TArray<FGridStatusEffectPresentationView> Views;
	FGridStatusEffectPresentationBuilder::Build(Collection, Views);
	TestEqual(TEXT("Two views"), Views.Num(), 2);
	TestEqual(TEXT("Projection remains sorted"), Views[0].EffectId, FName(TEXT("A_Effect")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON166FallbackProjectionTest, "Grimrock.RPG.MON16.6.FallbackProjection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON166FallbackProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridStatusEffectRuntimeState State;
	State.EffectId = TEXT("MON166_MissingDefinition");
	State.StackCount = 1;
	State.DurationUnit = EGridStatusEffectDurationUnit::Rounds;
	State.RemainingDuration = 2;
	FGridStatusEffectPresentationView View;
	TestTrue(TEXT("Valid runtime state still projects"), FGridStatusEffectPresentationBuilder::BuildOne(State, View));
	TestEqual(TEXT("EffectId is fallback display name"), View.DisplayName.ToString(), FString(TEXT("MON166_MissingDefinition")));
	TestTrue(TEXT("Fallback icon remains null"), View.Icon.IsNull());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON166PartyApplyFeedbackTest, "Grimrock.RPG.MON16.6.PartyApplyFeedback", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON166PartyApplyFeedbackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON166Fixture Fixture;
	if (!TestTrue(TEXT("Fixture ready"), Fixture.IsReady()))
	{
		return false;
	}
	UGridStatusEffectDefinitionAsset* Definition = MON166MakeDefinition(GetTransientPackage(), TEXT("MON166_Apply"), TEXT("Silence"));
	FGridStatusEffectApplyResult Result;
	FString Error;
	TestTrue(TEXT("Lifecycle applies party status"), Fixture.Lifecycle->TryApplyStatusEffectToPartyCharacter(0, Definition, FGuid::NewGuid(), Result, Error));
	const FGridCombatLogEntry& Feedback = Fixture.Lifecycle->LastStatusEffectFeedback;
	TestEqual(TEXT("Feedback type is applied"), Feedback.Type, EGridCombatLogEntryType::StatusApplied);
	TestEqual(TEXT("Feedback targets character"), Feedback.TargetCharacterIndex, 0);
	TestEqual(TEXT("Feedback effect id"), Feedback.StatusEffectId, Definition->EffectId);
	TestFalse(TEXT("Feedback message is present"), Feedback.Message.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON166RefreshFeedbackTest, "Grimrock.RPG.MON16.6.RefreshFeedback", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON166RefreshFeedbackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON166Fixture Fixture;
	if (!Fixture.IsReady())
	{
		return false;
	}
	UGridStatusEffectDefinitionAsset* Definition = MON166MakeDefinition(GetTransientPackage(), TEXT("MON166_Refresh"), TEXT("Hâte"),
		EGridStatusEffectDurationUnit::Rounds, 2, EGridStatusEffectStackPolicy::RefreshDuration, 1);
	FGridStatusEffectApplyResult Result;
	FString Error;
	TestTrue(TEXT("Initial apply"), Fixture.Lifecycle->TryApplyStatusEffectToPartyCharacter(0, Definition, FGuid::NewGuid(), Result, Error));
	TestTrue(TEXT("Refresh apply"), Fixture.Lifecycle->TryApplyStatusEffectToPartyCharacter(0, Definition, FGuid::NewGuid(), Result, Error, 1, 4));
	TestEqual(TEXT("Feedback type is refreshed"), Fixture.Lifecycle->LastStatusEffectFeedback.Type, EGridCombatLogEntryType::StatusRefreshed);
	TestTrue(TEXT("Feedback exposes refreshed duration"), Fixture.Lifecycle->LastStatusEffectFeedback.StatusEffectDurationText.ToString().Contains(TEXT("4")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON166TickAndExpirationFeedbackTest, "Grimrock.RPG.MON16.6.TickAndExpirationFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON166TickAndExpirationFeedbackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON166Fixture Fixture;
	FGridCharacterInventoryState* Character = Fixture.Character();
	if (!Fixture.IsReady() || !Character)
	{
		return false;
	}
	UGridStatusEffectDefinitionAsset* Definition =
		MON166MakeDefinition(GetTransientPackage(), TEXT("MON166_Dot"), TEXT("Brûlure"), EGridStatusEffectDurationUnit::Turns, 2);
	Definition->PeriodicDamage.DamageType = EGridDamageType::Fire;
	Definition->PeriodicDamage.DamagePerStack = 2;
	FGridStatusEffectApplyResult Result;
	FString Error;
	TestTrue(TEXT("DoT applies"), Fixture.Lifecycle->TryApplyStatusEffectToPartyCharacter(0, Definition, FGuid::NewGuid(), Result, Error));

	FGridCombatantInitiativeEntry Completed = MON166PartyEntry(Fixture.CharacterId);
	Completed.State = EGridCombatantTurnState::Completed;
	Fixture.TurnManager->OnCombatantStateChanged.Broadcast(Completed);
	TestEqual(TEXT("First boundary leaves tick as latest feedback"), Fixture.Lifecycle->LastStatusEffectFeedback.Type, EGridCombatLogEntryType::StatusTicked);
	TestEqual(TEXT("Tick dealt two HP"), Character->DerivedStats.CurrentHealth, 8);

	Fixture.TurnManager->OnCombatantStateChanged.Broadcast(Completed);
	TestEqual(TEXT("Second boundary expires after final tick"), Fixture.Lifecycle->LastStatusEffectFeedback.Type, EGridCombatLogEntryType::StatusExpired);
	TestFalse(TEXT("Effect removed after final tick"), Character->StatusEffects.Contains(Definition->EffectId));
	TestEqual(TEXT("Second tick also dealt damage"), Character->DerivedStats.CurrentHealth, 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON166MonsterFeedbackParityTest, "Grimrock.RPG.MON16.6.MonsterFeedbackParity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON166MonsterFeedbackParityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON166Fixture Fixture;
	if (!Fixture.IsReady())
	{
		return false;
	}
	UGridStatusEffectDefinitionAsset* Definition = MON166MakeDefinition(GetTransientPackage(), TEXT("MON166_MonsterStatus"), TEXT("Lenteur"));
	FGridStatusEffectApplyResult Result;
	FString Error;
	TestTrue(TEXT("Lifecycle applies monster status"),
		Fixture.Lifecycle->TryApplyStatusEffectToMonster(Fixture.Monster, Definition, FGuid::NewGuid(), Result, Error));
	const FGridCombatLogEntry& Feedback = Fixture.Lifecycle->LastStatusEffectFeedback;
	TestEqual(TEXT("Monster uses same feedback type"), Feedback.Type, EGridCombatLogEntryType::StatusApplied);
	TestEqual(TEXT("Monster has no party character index"), Feedback.TargetCharacterIndex, INDEX_NONE);
	TestEqual(TEXT("Monster target id is stable"), Feedback.TargetId, FName(*Fixture.MonsterId.ToString(EGuidFormats::Digits)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON166PartyPanelProjectionTest, "Grimrock.RPG.MON16.6.PartyPanelProjection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON166PartyPanelProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON166Fixture Fixture;
	if (!Fixture.IsReady())
	{
		return false;
	}
	UGridStatusEffectDefinitionAsset* Definition =
		MON166MakeDefinition(GetTransientPackage(), TEXT("MON166_Panel"), TEXT("Immobilisé"), EGridStatusEffectDurationUnit::Turns, 2);
	Definition->Control.bBlockTranslation = true;
	FGridStatusEffectApplyResult Result;
	FString Error;
	TestTrue(TEXT("Panel status applies"), Fixture.Lifecycle->TryApplyStatusEffectToPartyCharacter(0, Definition, FGuid::NewGuid(), Result, Error));

	UGridCombatActionPanelWidget* Panel = NewObject<UGridCombatActionPanelWidget>(Fixture.Party);
	Panel->InitializeCombatActionPanel(Fixture.Party, 0, Fixture.TurnManager);
	TestEqual(TEXT("Panel exposes one status"), Panel->View.StatusEffects.Num(), 1);
	TestTrue(TEXT("Panel status summary is visible data"), Panel->View.StatusSummary.ToString().Contains(TEXT("Immobilisé")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON166NoParallelSystemTest, "Grimrock.RPG.MON16.6.NoParallelSystem", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRPGMON166NoParallelSystemTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	struct FExpectation
	{
		const TCHAR* Path;
		const TCHAR* Required;
	};
	const FExpectation Expectations[] = { { TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectPresentation.cpp"),
											  TEXT("DefinitionAsset") },
		{ TEXT("Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.cpp"), TEXT("OnStatusEffectFeedback.Broadcast") },
		{ TEXT("Source/GrimrockPrototype/Private/UI/GridCombatActionPanelWidget.cpp"), TEXT("BuildSummary") } };

	for (const FExpectation& Expectation : Expectations)
	{
		FString Text;
		const FString Path = FPaths::Combine(FPaths::ProjectDir(), Expectation.Path);
		TestTrue(*FString::Printf(TEXT("Source loads: %s"), Expectation.Path), FFileHelper::LoadFileToString(Text, *Path));
		TestTrue(*FString::Printf(TEXT("Source contains: %s"), Expectation.Required), Text.Contains(Expectation.Required));
		TestFalse(TEXT("No WBP dependency introduced"), Text.Contains(TEXT("WBP_")));
		TestFalse(TEXT("No hard-coded status identity branch introduced"), Text.Contains(TEXT("EffectId == TEXT")) || Text.Contains(TEXT("EffectId != TEXT")));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

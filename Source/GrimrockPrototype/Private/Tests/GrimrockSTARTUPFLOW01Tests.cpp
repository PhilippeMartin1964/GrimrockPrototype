#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GrimrockGameInstance.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockStartupModeComponent.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

namespace STARTUPFLOW01Canonical
{
	struct FWorld
	{
		UWorld* World = nullptr;
		UGrimrockGameInstance* GI = nullptr;
		UGridItemSubsystem* Registry = nullptr;

		FWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
				.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
				.SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("STARTUPFLOW01_Canonical_%s"),
					*FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (!World || !GEngine) return;
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			GI = NewObject<UGrimrockGameInstance>(GetTransientPackage());
			GI->Init();
			World->SetGameInstance(GI);
			Registry = GI->GetSubsystem<UGridItemSubsystem>();
		}

		~FWorld()
		{
			if (World) World->SetGameInstance(nullptr);
			if (GI) GI->Shutdown();
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine) GEngine->DestroyWorldContext(World);
			}
		}

		UGridPartyInventoryComponent* NewInventory() const
		{
			AGrimrockPartyPawn* Pawn = World ? World->SpawnActor<AGrimrockPartyPawn>() : nullptr;
			return Pawn ? Pawn->PartyInventoryComponent.Get() : nullptr;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGrimrockSTARTUPFLOW01PendingPartyTest,
	"Grimrock.StartupFlow.STARTUPFLOW01.PendingParty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGrimrockSTARTUPFLOW01PendingPartyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGrimrockGameInstance* GameInstance = NewObject<UGrimrockGameInstance>();
	TestNotNull(TEXT("GameInstance is created"), GameInstance);
	if (!GameInstance)
	{
		return false;
	}

	TestEqual(TEXT("The canonical gameplay map is L_Dungeon"), GameInstance->GetDungeonLevelName(),
		FName(TEXT("/Game/GrimrockPrototype/Maps/L_Dungeon")));

	FGridPartyInventoryState PartyState;
	PartyState.bInitialCharacterCreationCompleted = true;
	PartyState.MaxActiveCharacters = 6;
	PartyState.SelectedCharacterIndex = 0;

	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.DisplayName = FText::FromString(TEXT("Startup Hero"));
	PartyState.ActiveCharacters.Add(Character);
	PartyState.ActiveEquipment.SetNum(1);

	TestTrue(TEXT("A completed frontend party can be staged"), GameInstance->SetPendingNewPartyState(PartyState));
	TestTrue(TEXT("The staged party is visible"), GameInstance->HasPendingNewPartyState());

	FGridPartyInventoryState ConsumedState;
	TestTrue(TEXT("The staged party is consumed once"), GameInstance->ConsumePendingNewPartyState(ConsumedState));
	TestEqual(TEXT("The consumed party preserves its character"), ConsumedState.ActiveCharacters.Num(), 1);
	TestEqual(TEXT("The consumed character id is preserved"), ConsumedState.ActiveCharacters[0].CharacterId, Character.CharacterId);
	TestFalse(TEXT("No pending party remains after consume"), GameInstance->HasPendingNewPartyState());

	return true;
}



IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGrimrockSTARTUPFLOW01StarterVisualsHandoffTest,
	"Grimrock.StartupFlow.STARTUPFLOW01.NewGameStarterVisualsHandoff18",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGrimrockSTARTUPFLOW01StarterVisualsHandoffTest::RunTest(const FString&)
{
	URPGRaceAsset* Race = LoadObject<URPGRaceAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
	if (!TestNotNull(TEXT("Production Human race"), Race))
	{
		return false;
	}

	STARTUPFLOW01Canonical::FWorld Fixture;
	UGrimrockGameInstance* GameInstance = Fixture.GI;
	if (!TestNotNull(TEXT("Frontend / dungeon GameInstance"), GameInstance) ||
		!TestTrue(TEXT("Real configured ItemSystem"), Fixture.Registry && Fixture.Registry->IsConfigured()))
	{
		return false;
	}

	const TCHAR* const ClassIds[] = {
		TEXT("Warrior"), TEXT("Rogue"), TEXT("Ranger"),
		TEXT("Mage"), TEXT("Priest"), TEXT("Alchemist")
	};
	int32 VerifiedKits = 0;
	FGridPartyInventoryState RepresentativeState;
	for (const TCHAR* ClassId : ClassIds)
	{
		const FString ClassPath = FString::Printf(
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_%s.DA_Class_%s"),
			ClassId, ClassId);
		URPGClassAsset* Class = LoadObject<URPGClassAsset>(nullptr, *ClassPath);
		if (!TestNotNull(TEXT("Production class"), Class))
		{
			return false;
		}
		URPGClassAsset* Preview = DuplicateObject<URPGClassAsset>(Class, GetTransientPackage());
		FString KitError;
		if (!TestTrue(TEXT("Approved N1 kits materialize on transient class"),
			Preview && FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(
				*Preview, KitError, Fixture.Registry)))
		{
			AddError(KitError);
			return false;
		}
		if (!TestEqual(TEXT("Three kits for each production class"),
			Preview->StartingEquipmentKits.Num(), 3))
		{
			return false;
		}
		for (const FRPGStartingEquipmentKit& Kit : Preview->StartingEquipmentKits)
		{
			// Physical state crosses levels without a legacy definition map.
			UGridPartyInventoryComponent* Frontend = Fixture.NewInventory();
			if (!TestNotNull(TEXT("Real frontend inventory"), Frontend)) return false;
			Frontend->InitializeDefaultPartyIfNeeded();
			FRPGCharacterCreationRequest Request;
			Request.DisplayName = FText::FromString(TEXT("Handoff Test"));
			Request.RaceDefinition = Race;
			Request.ClassDefinition = Class;
			Request.CombatActionSourceClassDefinition = Class;
			Request.StartingEquipmentKitId = Kit.KitId;
			FText Error;
			if (!TestTrue(*FString::Printf(TEXT("%s: frontend creates hero"), *Kit.KitId.ToString()),
				Frontend->CreateInitialCharacter(Request, Error)))
			{
				AddError(Error.ToString());
				return false;
			}
			if (!TestTrue(TEXT("Frontend state can cross OpenLevel"),
				GameInstance->SetPendingNewPartyState(Frontend->PartyInventoryState)))
			{
				return false;
			}
			GameInstance->SetPendingStartupMode(EGrimrockPartyStartupMode::NewGame);
			FGridPartyInventoryState Pending;
			if (!TestTrue(TEXT("New Game consumes the pending state"),
				GameInstance->ConsumePendingNewPartyState(Pending)))
			{
				return false;
			}
			UGridPartyInventoryComponent* Dungeon = Fixture.NewInventory();
			if (!TestNotNull(TEXT("Real dungeon inventory"), Dungeon)) return false;
			Dungeon->InitializeDefaultPartyIfNeeded();
			const FName FirstId = Kit.Items[0].CanonicalItemId;
			if (!TestNull(TEXT("Dungeon initially has no frontend definition map"),
				Dungeon->FindItemDefinition(FirstId)))
			{
				return false;
			}
			if (!TestTrue(*FString::Printf(TEXT("%s: startup restores canonical physical identities"),
				*Kit.KitId.ToString()),
				UGrimrockStartupModeComponent::RestorePendingNewGameParty(
					Dungeon, Pending, Error)))
			{
				AddError(Error.ToString());
				return false;
			}
			if (!TestEqual(TEXT("Kit selection survives frontend -> dungeon"),
				Dungeon->PartyInventoryState.ActiveCharacters[0].StartingEquipmentKitId, Kit.KitId) ||
				!TestEqual(TEXT("Main hero GUID survives frontend -> dungeon"),
					Dungeon->PartyInventoryState.ActiveCharacters[0].CharacterId,
					Frontend->PartyInventoryState.ActiveCharacters[0].CharacterId))
			{
				return false;
			}
			const FGridCharacterInventoryState& Hero =
				Dungeon->PartyInventoryState.ActiveCharacters[0];
			const FGridCharacterEquipmentState& Equipment =
				Dungeon->PartyInventoryState.ActiveEquipment[0];
			for (const FRPGStartingEquipmentEntry& Entry : Kit.Items)
			{
				FGridResolvedItem Resolved;
				if (!TestNull(TEXT("No legacy definition in approved recipe"), Entry.ItemDefinition.Get()) ||
					!TestTrue(TEXT("Real recipe identity resolves"),
						Fixture.Registry->ResolveItem(Entry.CanonicalItemId, Entry.ItemLevel, Resolved)) ||
					!TestNull(TEXT("No old definition in dungeon registry"),
						Dungeon->FindItemDefinition(Entry.CanonicalItemId)))
				{
					return false;
				}
				const FGridItemInstance* Item = nullptr;
				if (Entry.TargetSlot != EGridEquipmentSlot::None)
				{
					Item = Equipment.GetSlot(Entry.TargetSlot);
				}
				else
				{
					for (const FGridInventorySlot& Slot : Hero.InventorySlots)
					{
						if (!Slot.IsEmpty() &&
							Slot.Item.ItemDefinitionId == Entry.CanonicalItemId)
						{
							Item = &Slot.Item;
							break;
						}
					}
				}
				const FString Label = FString::Printf(TEXT("%s / %s"),
					*Kit.KitId.ToString(), *Entry.CanonicalItemId.ToString());
				if (!TestTrue(*FString::Printf(TEXT("%s: correct item/slot/N1/quantity"),
					*Label), Item && Item->IsValid() &&
						Item->ItemDefinitionId == Entry.CanonicalItemId &&
						Item->Quantity == Entry.Quantity &&
						Item->ItemLevel == Entry.ItemLevel))
				{
					return false;
				}
				if (Resolved.bIsArmament)
				{
					if (!TestFalse(*FString::Printf(TEXT("%s: weapon icon reference"), *Label),
						Resolved.ItemIcon.IsNull()) ||
						!TestNotNull(*FString::Printf(TEXT("%s: icon loads on first New Game"), *Label),
							Resolved.ItemIcon.LoadSynchronous()))
					{
						return false;
					}
				}
			}
			FString OwnershipError;
			if (!TestTrue(TEXT("Handoff inventory ownership remains valid"),
				Dungeon->ValidateInventoryOwnership(OwnershipError)))
			{
				AddError(OwnershipError);
				return false;
			}
			if (RepresentativeState.ActiveCharacters.IsEmpty())
			{
				RepresentativeState = Pending;
			}
			++VerifiedKits;
		}
	}
	if (!TestEqual(TEXT("Every one of the 18 New Game starter kits rehydrates"), VerifiedKits, 18))
	{
		return false;
	}

	// Missing assets must not yield a finalized but unusable hero.
	RepresentativeState.ActiveEquipment[0].MainHand.ItemDefinitionId =
		TEXT("Weapon_Unknown_StartupHandoff_0218");
	UGridPartyInventoryComponent* InvalidDungeon = Fixture.NewInventory();
	if (!TestNotNull(TEXT("Invalid handoff inventory fixture"), InvalidDungeon)) return false;
	InvalidDungeon->InitializeDefaultPartyIfNeeded();
	FText Error;
	TestFalse(TEXT("Unknown starter item aborts dungeon startup"),
		UGrimrockStartupModeComponent::RestorePendingNewGameParty(
			InvalidDungeon, RepresentativeState, Error));
	TestFalse(TEXT("Failed handoff cannot leave a finalized hero"),
		InvalidDungeon->HasCompletedInitialCharacterCreation());
	TestFalse(TEXT("Missing definition error is reported"), Error.IsEmpty());
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGrimrockSTARTUPFLOW01FrontendControllerInventoryTest,
	"Grimrock.StartupFlow.STARTUPFLOW01.FrontendControllerCanonicalInventory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGrimrockSTARTUPFLOW01FrontendControllerInventoryTest::RunTest(const FString&)
{
	STARTUPFLOW01Canonical::FWorld Fixture;
	if (!TestNotNull(TEXT("Frontend world exists"), Fixture.World) ||
		!TestTrue(TEXT("Frontend GI configured canonical ItemSystem"),
			Fixture.Registry && Fixture.Registry->IsConfigured()))
	{
		return false;
	}

	APlayerController* MenuController = Fixture.World->SpawnActor<APlayerController>();
	if (!TestNotNull(TEXT("Real menu PlayerController exists"), MenuController))
	{
		return false;
	}

	// Same lifecycle as UGrimrockMainMenuWidget: the frontend inventory
	// must be an actual world-owned ActorComponent, never widget-outered.
	UGridPartyInventoryComponent* Frontend = NewObject<UGridPartyInventoryComponent>(MenuController);
	if (!TestNotNull(TEXT("Frontend inventory created"), Frontend))
	{
		return false;
	}
	MenuController->AddInstanceComponent(Frontend);
	Frontend->RegisterComponent();
	TestTrue(TEXT("Frontend inventory registered on menu controller"), Frontend->IsRegistered());
	TestTrue(TEXT("Frontend inventory resolves menu world"), Frontend->GetWorld() == Fixture.World);
	TestTrue(TEXT("Frontend inventory shares configured ItemSystem with menu"),
		Frontend->GetWorld() && Frontend->GetWorld()->GetGameInstance() == Fixture.GI &&
		Fixture.Registry->HasCanonicalItem(TEXT("Weapon_StraightSword_01")));

	URPGRaceAsset* Race = LoadObject<URPGRaceAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
	URPGClassAsset* Class = LoadObject<URPGClassAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Warrior.DA_Class_Warrior"));
	if (!TestNotNull(TEXT("Production race"), Race) ||
		!TestNotNull(TEXT("Production Warrior class"), Class))
	{
		Frontend->DestroyComponent();
		return false;
	}

	Frontend->ResetPartyForNewGame();
	FRPGCharacterCreationRequest Request;
	Request.DisplayName = FText::FromString(TEXT("Frontend Controller Hero"));
	Request.RaceDefinition = Race;
	Request.ClassDefinition = Class;
	Request.CombatActionSourceClassDefinition = Class;
	FText Error;
	const bool bCreated = Frontend->CreateInitialCharacter(Request, Error);
	if (!TestTrue(TEXT("Frontend UI inventory can create a hero with canonical starter kit"), bCreated))
	{
		AddError(Error.ToString());
	}
	if (bCreated)
	{
		TestTrue(TEXT("Frontend character creation committed"), Frontend->HasCompletedInitialCharacterCreation());
		TestTrue(TEXT("Starter kit identity persisted in prepared character"),
			Frontend->PartyInventoryState.ActiveCharacters.Num() == 1 &&
			!Frontend->PartyInventoryState.ActiveCharacters[0].StartingEquipmentKitId.IsNone());
	}
	Frontend->DestroyComponent();
	return !HasAnyErrors();
}

#endif

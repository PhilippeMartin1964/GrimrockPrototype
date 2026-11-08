#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GrimrockGameInstance.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockStartupModeComponent.h"
#include "UI/GridWeaponVisualUiResolver.h"

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

	UGrimrockGameInstance* GameInstance = NewObject<UGrimrockGameInstance>();
	if (!TestNotNull(TEXT("Frontend / dungeon GameInstance"), GameInstance))
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
				*Preview, KitError)))
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
			// The frontend component owns real item definitions; the pending
			// GameInstance state intentionally does not serialize that map.
			UGridPartyInventoryComponent* Frontend = NewObject<UGridPartyInventoryComponent>();
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
			UGridPartyInventoryComponent* Dungeon = NewObject<UGridPartyInventoryComponent>();
			Dungeon->InitializeDefaultPartyIfNeeded();
			const FName FirstId = Kit.Items[0].ItemDefinition->ItemDefinitionId;
			if (!TestNull(TEXT("Dungeon initially has no frontend definition map"),
				Dungeon->FindItemDefinition(FirstId)))
			{
				return false;
			}
			if (!TestTrue(*FString::Printf(TEXT("%s: startup restores items AND definitions"),
				*Kit.KitId.ToString()),
				UGrimrockStartupModeComponent::RestorePendingNewGameParty(
					Dungeon, Pending, nullptr, Error)))
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
				const UGridItemDefinitionAsset* Definition = Entry.ItemDefinition.Get();
				if (!TestNotNull(TEXT("N1 kit item definition"), Definition) ||
					!TestTrue(TEXT("Item definition immediately ready for inventory UI"),
						Dungeon->FindItemDefinition(Definition->ItemDefinitionId) == Definition))
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
							Slot.Item.ItemDefinitionId == Definition->ItemDefinitionId)
						{
							Item = &Slot.Item;
							break;
						}
					}
				}
				const FString Label = FString::Printf(TEXT("%s / %s"),
					*Kit.KitId.ToString(), *Definition->ItemDefinitionId.ToString());
				if (!TestTrue(*FString::Printf(TEXT("%s: correct item/slot/N1/quantity"),
					*Label), Item && Item->IsValid() &&
						Item->ItemDefinitionId == Definition->ItemDefinitionId &&
						Item->Quantity == Entry.Quantity &&
						Item->ItemLevel == (Definition->bUseWeaponCatalogue ? 1 : 0)))
				{
					return false;
				}
				if (Definition->bUseWeaponCatalogue)
				{
					const FGridResolvedWeaponVisual Visual =
						FGridWeaponVisualUiResolver::Resolve(*Definition, Item);
					if (!TestFalse(*FString::Printf(TEXT("%s: weapon icon reference"), *Label),
						Visual.Icon.IsNull()) ||
						!TestNotNull(*FString::Printf(TEXT("%s: icon loads on first New Game"), *Label),
							Visual.Icon.LoadSynchronous()))
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
	UGridPartyInventoryComponent* InvalidDungeon = NewObject<UGridPartyInventoryComponent>();
	InvalidDungeon->InitializeDefaultPartyIfNeeded();
	FText Error;
	TestFalse(TEXT("Unknown starter item aborts dungeon startup"),
		UGrimrockStartupModeComponent::RestorePendingNewGameParty(
			InvalidDungeon, RepresentativeState, nullptr, Error));
	TestFalse(TEXT("Failed handoff cannot leave a finalized hero"),
		InvalidDungeon->HasCompletedInitialCharacterCreation());
	TestFalse(TEXT("Missing definition error is reported"), Error.IsEmpty());
	return true;
}

#endif

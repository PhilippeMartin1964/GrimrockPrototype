#include "Runtime/GrimrockPartyPawn.h"

#include "Kismet/GameplayStatics.h"
#include "Magic/GridPartySpellbookComponent.h"
#include "Magic/GridSpellbookPersistence.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GridCombatSavePolicy.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UI/GrimrockMenuWidget.h"

namespace
{
	UGridPartySpellbookComponent* GridPartyPawnSaveGetSpellbookComponent(AGrimrockPartyPawn* PartyPawn)
	{
		return IsValid(PartyPawn) ? PartyPawn->FindComponentByClass<UGridPartySpellbookComponent>() : nullptr;
	}

	void GridPartyPawnSaveResetSpellbook(AGrimrockPartyPawn* PartyPawn)
	{
		if (UGridPartySpellbookComponent* SpellbookComponent = GridPartyPawnSaveGetSpellbookComponent(PartyPawn))
		{
			SpellbookComponent->ResetAllSpellbooks();
		}
	}
}

bool AGrimrockPartyPawn::HasCurrentSave() const
{
	return !PartySaveSlotName.IsEmpty() && UGameplayStatics::DoesSaveGameExist(PartySaveSlotName, PartySaveUserIndex);
}

bool AGrimrockPartyPawn::SaveCurrentGame(FText& OutError)
{
	OutError = FText::GetEmpty();

	if (PartySaveSlotName.IsEmpty())
	{
		OutError = FText::FromString(TEXT("Le nom du slot de sauvegarde est vide."));
		return false;
	}

	if (!PartyInventoryComponent || !PartyInventoryComponent->HasCompletedInitialCharacterCreation())
	{
		OutError = FText::FromString(TEXT("Aucun personnage finalisé ne peut être sauvegardé."));
		return false;
	}

	// MON18.9.1 primary gate: reject before any runtime capture or disk
	// write. Serialize() keeps a defense-in-depth backstop for direct
	// SaveGame writes that bypass this authoritative Pawn entry point.
	if (FGridCombatSavePolicy::IsSaveBlockedByCombatState(FindTurnManager()))
	{
		OutError = FText::FromString(TEXT("La sauvegarde est interdite pendant un combat ou après une défaite."));
		UE_LOG(LogTemp, Log, TEXT("PartySave SaveRejected Slot=%s Reason=CombatStateNotSaveable"), *PartySaveSlotName);
		return false;
	}

	UGridPartySpellbookComponent* SpellbookComponent = GridPartyPawnSaveGetSpellbookComponent(this);
	if (!SpellbookComponent)
	{
		OutError = FText::FromString(TEXT("Le composant Spellbook du groupe est indisponible."));
		return false;
	}

	FString OwnershipError;
	if (!PartyInventoryComponent->ValidateInventoryOwnership(OwnershipError))
	{
		OutError = FText::FromString(FString::Printf(TEXT("L'ownership du groupe est invalide : %s"), *OwnershipError));
		return false;
	}

	TArray<FGridCharacterSpellbookSaveState> SpellbookSnapshots;
	FString SpellbookCaptureError;
	if (!FGridSpellbookPersistence::CapturePartySpellbooks(
			PartyInventoryComponent->PartyInventoryState, SpellbookComponent->SpellbookState, SpellbookSnapshots, SpellbookCaptureError))
	{
		OutError = FText::FromString(FString::Printf(TEXT("Le Spellbook ne peut pas être capturé : %s"), *SpellbookCaptureError));
		return false;
	}

	if (!IsValid(LevelRuntimeActor) || !LevelRuntimeActor->CaptureCurrentLevelRuntimeState())
	{
		OutError = FText::FromString(TEXT("L'état runtime du niveau ne peut pas être capturé."));
		return false;
	}

	UGrimrockPartySaveGame* SaveGame = Cast<UGrimrockPartySaveGame>(UGameplayStatics::CreateSaveGameObject(UGrimrockPartySaveGame::StaticClass()));
	if (!SaveGame)
	{
		OutError = FText::FromString(TEXT("L'objet de sauvegarde ne peut pas être créé."));
		return false;
	}

	SaveGame->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion;
	SaveGame->PartyInventoryState = PartyInventoryComponent->PartyInventoryState;
	SaveGame->CharacterSpellbookStates = MoveTemp(SpellbookSnapshots);
	SaveGame->DungeonRuntimeState = LevelRuntimeActor->DungeonRuntimeState;
	SaveGame->CurrentDungeonLevelId = LevelRuntimeActor->CurrentDungeonLevelId;
	SaveGame->PartyCellX = CurrentCellX;
	SaveGame->PartyCellY = CurrentCellY;
	SaveGame->PartyFacing = Facing;

	if (!UGameplayStatics::SaveGameToSlot(SaveGame, PartySaveSlotName, PartySaveUserIndex))
	{
		OutError = FText::FromString(TEXT("L'écriture du fichier de sauvegarde a échoué."));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("PartySave Saved Slot=%s Version=%d Characters=%d Spellbooks=%d Cell=(%d,%d) Facing=%d"), *PartySaveSlotName,
		SaveGame->SaveVersion, SaveGame->PartyInventoryState.ActiveCharacters.Num(), SaveGame->CharacterSpellbookStates.Num(), CurrentCellX, CurrentCellY,
		static_cast<int32>(Facing));
	return true;
}

bool AGrimrockPartyPawn::RehydrateLoadedItemDefinitions(FText& OutError)
{
	if (!PartyInventoryComponent || !IsValid(LevelRuntimeActor))
	{
		OutError = FText::FromString(TEXT("Le groupe ou le niveau runtime est indisponible."));
		return false;
	}

	FName MissingDefinitionId = NAME_None;
	if (!PartyInventoryComponent->RehydrateOwnedItemDefinitions(
			[this](FName DefinitionId)
			{
				return LevelRuntimeActor->ResolveRuntimeItemDefinition(DefinitionId);
			},
			MissingDefinitionId))
	{
		OutError = FText::FromString(FString::Printf(TEXT("La définition d'objet sauvegardée '%s' est introuvable."), *MissingDefinitionId.ToString()));
		return false;
	}

	return true;
}

bool AGrimrockPartyPawn::LoadCurrentGameData(FText& OutError, bool bApplyDungeonState)
{
	OutError = FText::GetEmpty();

	if (!HasCurrentSave())
	{
		OutError = FText::FromString(TEXT("Aucune sauvegarde n'est disponible."));
		return false;
	}

	UGrimrockPartySaveGame* SaveGame = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromSlot(PartySaveSlotName, PartySaveUserIndex));
	if (!SaveGame)
	{
		OutError = FText::FromString(TEXT("Le fichier ne contient pas une sauvegarde de groupe valide."));
		return false;
	}

	if (!SaveGame->IsCompatible())
	{
		OutError = FText::FromString(FString::Printf(TEXT("Version de sauvegarde incompatible : %d, versions acceptées : %d à %d."), SaveGame->SaveVersion,
			UGrimrockPartySaveGame::MinimumCompatibleSaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion));
		return false;
	}

	UGridPartySpellbookComponent* SpellbookComponent = GridPartyPawnSaveGetSpellbookComponent(this);
	if (!PartyInventoryComponent || !SpellbookComponent || !IsValid(LevelRuntimeActor))
	{
		OutError = FText::FromString(TEXT("Le groupe, le Spellbook ou le niveau runtime est indisponible."));
		return false;
	}

	FGridPartySpellbookState RestoredSpellbookState;
	FString SpellbookRestoreError;
	if (!FGridSpellbookPersistence::RestorePartySpellbooks(
			SaveGame->PartyInventoryState, SaveGame->CharacterSpellbookStates, RestoredSpellbookState, SpellbookRestoreError))
	{
		OutError = FText::FromString(FString::Printf(TEXT("Le Spellbook sauvegardé est invalide : %s"), *SpellbookRestoreError));
		return false;
	}

	const FGridPartyInventoryState PreviousPartyState = PartyInventoryComponent->PartyInventoryState;
	const FGridDungeonRuntimeState PreviousDungeonState = LevelRuntimeActor->DungeonRuntimeState;
	const FName PreviousDungeonLevelId = LevelRuntimeActor->CurrentDungeonLevelId;
	UGridLevelAsset* PreviousLevelAsset = LevelRuntimeActor->LevelAsset;
	const int32 PreviousCellX = CurrentCellX;
	const int32 PreviousCellY = CurrentCellY;
	const EGridEdge PreviousFacing = Facing;

	auto RestorePreviousState =
		[this, &PreviousPartyState, &PreviousDungeonState, PreviousDungeonLevelId, PreviousLevelAsset, PreviousCellX, PreviousCellY, PreviousFacing]()
	{
		PartyInventoryComponent->PartyInventoryState = PreviousPartyState;
		LevelRuntimeActor->DungeonRuntimeState = PreviousDungeonState;
		LevelRuntimeActor->CurrentDungeonLevelId = PreviousDungeonLevelId;
		LevelRuntimeActor->LevelAsset = PreviousLevelAsset;
		CurrentCellX = PreviousCellX;
		CurrentCellY = PreviousCellY;
		Facing = PreviousFacing;
	};

	if (!PartyInventoryComponent->RestorePartyInventoryState(SaveGame->PartyInventoryState, OutError))
	{
		return false;
	}

	if (bApplyDungeonState)
	{
		LevelRuntimeActor->DungeonRuntimeState = SaveGame->DungeonRuntimeState;
		LevelRuntimeActor->CurrentDungeonLevelId = SaveGame->CurrentDungeonLevelId;
		if (LevelRuntimeActor->DungeonAsset && !SaveGame->CurrentDungeonLevelId.IsNone())
		{
			UGridLevelAsset* SavedLevelAsset = LevelRuntimeActor->DungeonAsset->GetLevelAssetById(SaveGame->CurrentDungeonLevelId);
			if (!SavedLevelAsset)
			{
				RestorePreviousState();
				OutError = FText::FromString(
					FString::Printf(TEXT("Le niveau sauvegardé '%s' est introuvable dans le donjon."), *SaveGame->CurrentDungeonLevelId.ToString()));
				return false;
			}
			LevelRuntimeActor->LevelAsset = SavedLevelAsset;
			LevelRuntimeActor->RebuildLevel();
		}
		CurrentCellX = SaveGame->PartyCellX;
		CurrentCellY = SaveGame->PartyCellY;
		Facing = SaveGame->PartyFacing == EGridEdge::None ? EGridEdge::North : SaveGame->PartyFacing;
	}

	if (!RehydrateLoadedItemDefinitions(OutError))
	{
		RestorePreviousState();
		return false;
	}

	if (bApplyDungeonState && !LevelRuntimeActor->ApplyCurrentLevelRuntimeState())
	{
		RestorePreviousState();
		FText RehydrateRollbackError;
		RehydrateLoadedItemDefinitions(RehydrateRollbackError);
		OutError = FText::FromString(TEXT("L'état runtime sauvegardé ne peut pas être appliqué au niveau."));
		return false;
	}

	SpellbookComponent->SpellbookState = MoveTemp(RestoredSpellbookState);
	SpellbookComponent->OnSpellbookChanged.Broadcast();
	return true;
}

bool AGrimrockPartyPawn::LoadCurrentGame(FText& OutError)
{
	if (!LoadCurrentGameData(OutError, true))
	{
		return false;
	}

	CloseCharacterCreationWidget();
	bCharacterCreationModalActive = false;
	ClearBufferedCommand();
	SnapToCurrentCell();

	if (LevelRuntimeActor)
	{
		LevelRuntimeActor->HandlePartyCellChanged(CurrentCellX, CurrentCellY, CurrentCellX, CurrentCellY);
	}

	ApplyCharacterCreationInputMode(false);
	SyncHeldVisualFromSelectedCharacterEquipment();

	if (MenuWidgetInstance)
	{
		MenuWidgetInstance->RefreshInventory();
		MenuWidgetInstance->RefreshSpellbook();
	}

	UE_LOG(LogTemp, Log, TEXT("PartySave Loaded Slot=%s"), *PartySaveSlotName);
	return true;
}

bool AGrimrockPartyPawn::StartNewGame(FText& OutError)
{
	OutError = FText::GetEmpty();

	if (HasCurrentSave() && !UGameplayStatics::DeleteGameInSlot(PartySaveSlotName, PartySaveUserIndex))
	{
		OutError = FText::FromString(TEXT("La sauvegarde existante ne peut pas être supprimée."));
		return false;
	}

	if (!PartyInventoryComponent)
	{
		OutError = FText::FromString(TEXT("Le composant de groupe est indisponible."));
		return false;
	}

	PartyInventoryComponent->ResetPartyForNewGame();
	GridPartyPawnSaveResetSpellbook(this);

	if (bInventoryWidgetVisible)
	{
		HideInventoryWidget();
	}

	CloseCharacterCreationWidget();
	bCharacterCreationModalActive = false;
	ClearBufferedCommand();
	ClearHeldItem();

	if (LevelRuntimeActor)
	{
		LevelRuntimeActor->DungeonRuntimeState = FGridDungeonRuntimeState();
		LevelRuntimeActor->RebuildRuntimeObjects();

		if (LevelRuntimeActor->LevelAsset && LevelRuntimeActor->LevelAsset->IsStartCellValid())
		{
			CurrentCellX = LevelRuntimeActor->LevelAsset->StartCellX;
			CurrentCellY = LevelRuntimeActor->LevelAsset->StartCellY;
			Facing = LevelRuntimeActor->LevelAsset->StartFacing == EGridEdge::None ? EGridEdge::North : LevelRuntimeActor->LevelAsset->StartFacing;
			SnapToCurrentCell();
		}
	}

	ShowInitialCharacterCreationWidget();
	UE_LOG(LogTemp, Log, TEXT("PartySave NewGame Slot=%s"), *PartySaveSlotName);
	return true;
}

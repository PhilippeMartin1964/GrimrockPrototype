#include "UI/RPGLevelUpWidget.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionService.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "Runtime/GridPartyInventoryComponent.h"

#define LOCTEXT_NAMESPACE "RPGLevelUpWidget"

namespace
{
	FRPGLevelUpPresentationView MakeTalentPresentationView()
	{
		FRPGLevelUpPresentationView Presentation;
		Presentation.Title = LOCTEXT("Title", "NIVEAU SUPÉRIEUR");
		Presentation.TalentSectionTitle = LOCTEXT("TalentSectionTitle", "TALENTS DE CLASSE");
		Presentation.TalentPointsLabel = LOCTEXT("TalentPointsLabel", "Points de talent");
		Presentation.EmptyTalentsMessage = LOCTEXT("EmptyTalentsMessage", "Aucun talent de classe à sélectionner pour ce niveau.");
		Presentation.SelectionPrompt = LOCTEXT("TalentSelectionPrompt", "Sélectionnez un talent, ou annulez pour le reporter.");
		return Presentation;
	}

	FText MakeCommitRejectText(ERPGClassProgressionCommitRejectReason Reason)
	{
		switch (Reason)
		{
			case ERPGClassProgressionCommitRejectReason::InvalidInventory:
			case ERPGClassProgressionCommitRejectReason::InvalidCharacter:
				return LOCTEXT("CommitInvalidCharacter", "Le personnage n'est plus disponible.");
			case ERPGClassProgressionCommitRejectReason::InvalidClassDefinition:
				return LOCTEXT("CommitInvalidClass", "La définition de classe n'est plus valide.");
			case ERPGClassProgressionCommitRejectReason::InvalidCurrentSelection:
				return LOCTEXT("CommitInvalidState", "L'état de progression courant est incohérent.");
			case ERPGClassProgressionCommitRejectReason::DuplicateRequest:
				return LOCTEXT("CommitDuplicate", "Un même talent apparaît plusieurs fois dans la transaction.");
			case ERPGClassProgressionCommitRejectReason::UnknownChoice:
				return LOCTEXT("CommitUnknown", "Un talent de classe n'existe plus.");
			case ERPGClassProgressionCommitRejectReason::AlreadySelected:
				return LOCTEXT("CommitAlreadySelected", "Un talent demandé est déjà acquis.");
			case ERPGClassProgressionCommitRejectReason::LevelTooLow:
				return LOCTEXT("CommitLevelTooLow", "Le niveau requis n'est pas atteint.");
			case ERPGClassProgressionCommitRejectReason::MissingPrerequisite:
				return LOCTEXT("CommitMissingPrerequisite", "Un prérequis de talent manque.");
			case ERPGClassProgressionCommitRejectReason::InsufficientChoicePoints:
				return LOCTEXT("CommitInsufficientPoints", "Il n'y a pas assez de points de talent.");
			case ERPGClassProgressionCommitRejectReason::EmptyRequest:
				return LOCTEXT("CommitEmpty", "Aucun talent n'est sélectionné.");
			case ERPGClassProgressionCommitRejectReason::None:
			default:
				return FText::GetEmpty();
		}
	}
}

bool URPGLevelUpWidget::InitializeLevelUpWidget(
	UGridPartyInventoryComponent* InInventoryComponent, int32 InCharacterIndex, int32 InPreviousLevel, int32 InNewLevel)
{
	InventoryComponent = InInventoryComponent;
	CharacterIndex = InCharacterIndex;
	PreviousLevel = InPreviousLevel;
	NewLevel = InNewLevel;
	PendingChoiceIds.Reset();

	if (!IsValid(InventoryComponent) || !InventoryComponent->IsValidCharacterIndex(CharacterIndex) ||
		PreviousLevel < URPGCharacterRulesLibrary::GetMinimumLevel() || NewLevel < PreviousLevel || NewLevel > URPGCharacterRulesLibrary::GetMaximumLevel() ||
		!ResolveClassDefinition())
	{
		View = FRPGLevelUpView();
		View.Presentation = MakeTalentPresentationView();
		View.ValidationMessage = LOCTEXT("InitializeInvalid", "Impossible d'ouvrir la progression de ce personnage.");
		RefreshNativeSlate();
		BP_OnLevelUpViewRefreshed();
		return false;
	}

	FRPGClassProgressionTransactionService::RefreshCharacterProjection(InventoryComponent, CharacterIndex);
	RefreshView();
	if (IsInViewport())
	{
		ApplyInputGuard();
	}
	return true;
}

bool URPGLevelUpWidget::StageOrUnstageChoice(FName ChoiceId)
{
	if (ChoiceId.IsNone() || !ResolveClassDefinition())
	{
		return false;
	}

	if (PendingChoiceIds.Contains(ChoiceId))
	{
		if (!CanRemovePendingChoice(ChoiceId))
		{
			View.ValidationMessage = LOCTEXT("CannotRemovePendingPrerequisite", "Ce talent est requis par une autre sélection en attente.");
			RefreshNativeSlate();
			BP_OnLevelUpViewRefreshed();
			return false;
		}
		PendingChoiceIds.Remove(ChoiceId);
		RefreshView();
		return true;
	}

	TSet<FName> CombinedSelection;
	if (!BuildCombinedSelection(CombinedSelection))
	{
		return false;
	}

	URPGClassAsset* ClassDefinition = ResolveClassDefinition();
	const FGridCharacterInventoryState& Character = InventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
	const ERPGClassProgressionChoiceAvailabilityReason Reason =
		FRPGClassProgressionService::GetChoiceAvailability(ClassDefinition, Character.Level, CombinedSelection, ChoiceId);
	if (Reason != ERPGClassProgressionChoiceAvailabilityReason::None)
	{
		View.ValidationMessage = GetChoiceStatusText(static_cast<int32>(Reason));
		RefreshNativeSlate();
		BP_OnLevelUpViewRefreshed();
		return false;
	}

	PendingChoiceIds.Add(ChoiceId);
	RefreshView();
	return true;
}

bool URPGLevelUpWidget::ConfirmSelection()
{
	if (!IsValid(InventoryComponent) || !InventoryComponent->IsValidCharacterIndex(CharacterIndex))
	{
		return false;
	}

	if (PendingChoiceIds.IsEmpty())
	{
		CloseModal();
		return true;
	}

	FRPGClassProgressionCommitResult Result;
	if (!FRPGClassProgressionTransactionService::TryCommitChoices(InventoryComponent, CharacterIndex, PendingChoiceIds, Result))
	{
		RefreshView();
		View.ValidationMessage = MakeCommitRejectText(Result.RejectReason);
		RefreshNativeSlate();
		BP_OnLevelUpViewRefreshed();
		return false;
	}

	PendingChoiceIds.Reset();
	RefreshView();
	CloseModal();
	return true;
}

void URPGLevelUpWidget::CancelSelection()
{
	PendingChoiceIds.Reset();
	CloseModal();
}

TArray<FName> URPGLevelUpWidget::GetPendingChoiceIds() const
{
	return PendingChoiceIds;
}

FRPGLevelUpWidgetClosedNativeSignature& URPGLevelUpWidget::OnClosed()
{
	return ClosedDelegate;
}

URPGClassAsset* URPGLevelUpWidget::ResolveClassDefinition() const
{
	if (!IsValid(InventoryComponent) || !InventoryComponent->IsValidCharacterIndex(CharacterIndex))
	{
		return nullptr;
	}

	FGridCharacterInventoryState& Character = InventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
	URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get();
	if (!ClassDefinition && !Character.ClassDefinition.IsNull())
	{
		ClassDefinition = Character.ClassDefinition.LoadSynchronous();
	}
	if (!IsValid(ClassDefinition) || !ClassDefinition->IsValidDefinition() || (!Character.ClassId.IsNone() && ClassDefinition->ClassId != Character.ClassId))
	{
		return nullptr;
	}
	return ClassDefinition;
}

bool URPGLevelUpWidget::BuildCombinedSelection(TSet<FName>& OutSelectedChoiceIds, TArray<FName>* OutCommittedChoiceIds) const
{
	OutSelectedChoiceIds.Reset();
	if (OutCommittedChoiceIds)
	{
		OutCommittedChoiceIds->Reset();
	}
	if (!IsValid(InventoryComponent))
	{
		return false;
	}

	TArray<FName> CommittedChoices;
	if (!FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds(InventoryComponent, CharacterIndex, CommittedChoices))
	{
		return false;
	}

	for (const FName ChoiceId : CommittedChoices)
	{
		if (ChoiceId.IsNone() || OutSelectedChoiceIds.Contains(ChoiceId))
		{
			return false;
		}
		OutSelectedChoiceIds.Add(ChoiceId);
	}
	if (OutCommittedChoiceIds)
	{
		*OutCommittedChoiceIds = CommittedChoices;
	}

	for (const FName ChoiceId : PendingChoiceIds)
	{
		if (ChoiceId.IsNone() || OutSelectedChoiceIds.Contains(ChoiceId))
		{
			return false;
		}
		OutSelectedChoiceIds.Add(ChoiceId);
	}
	return true;
}

bool URPGLevelUpWidget::CanRemovePendingChoice(FName ChoiceId) const
{
	URPGClassAsset* ClassDefinition = ResolveClassDefinition();
	if (!ClassDefinition || !PendingChoiceIds.Contains(ChoiceId))
	{
		return false;
	}

	TArray<FName> CommittedChoices;
	if (!FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds(InventoryComponent, CharacterIndex, CommittedChoices))
	{
		return false;
	}

	TSet<FName> Candidate;
	for (const FName CommittedChoiceId : CommittedChoices)
	{
		Candidate.Add(CommittedChoiceId);
	}
	for (const FName PendingChoiceId : PendingChoiceIds)
	{
		if (PendingChoiceId != ChoiceId)
		{
			Candidate.Add(PendingChoiceId);
		}
	}

	const FGridCharacterInventoryState& Character = InventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
	int32 Granted = 0;
	int32 Spent = 0;
	int32 Remaining = 0;
	return FRPGClassProgressionService::TryGetChoicePointBalance(ClassDefinition, Character.Level, Candidate, Granted, Spent, Remaining);
}

void URPGLevelUpWidget::RefreshView()
{
	FRPGLevelUpView NewView;
	NewView.Presentation = MakeTalentPresentationView();
	NewView.CharacterIndex = CharacterIndex;
	NewView.PreviousLevel = PreviousLevel;
	NewView.NewLevel = NewLevel;

	URPGClassAsset* ClassDefinition = ResolveClassDefinition();
	if (!ClassDefinition || !IsValid(InventoryComponent) || !InventoryComponent->IsValidCharacterIndex(CharacterIndex))
	{
		NewView.ValidationMessage = LOCTEXT("RefreshInvalid", "La progression du personnage n'est plus disponible.");
		View = MoveTemp(NewView);
		RefreshNativeSlate();
		BP_OnLevelUpViewRefreshed();
		return;
	}

	const FGridCharacterInventoryState& Character = InventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
	NewView.CharacterName = Character.DisplayName;
	NewView.ClassName = Character.ClassDisplayName.IsEmpty() ? ClassDefinition->DisplayName : Character.ClassDisplayName;

	const FRPGDerivedStats PreviousStats = URPGCharacterRulesLibrary::CalculateDerivedStats(Character.Attributes, ClassDefinition, PreviousLevel);
	const FRPGDerivedStats NewStats = URPGCharacterRulesLibrary::CalculateDerivedStats(Character.Attributes, ClassDefinition, NewLevel);
	NewView.PreviousMaxHealth = PreviousStats.MaxHealth;
	NewView.NewMaxHealth = NewStats.MaxHealth;
	NewView.PreviousMaxMana = PreviousStats.MaxMana;
	NewView.NewMaxMana = NewStats.MaxMana;
	NewView.PreviousPhysicalArmor = PreviousStats.PhysicalArmor;
	NewView.NewPhysicalArmor = NewStats.PhysicalArmor;
	NewView.PreviousMagicalArmor = PreviousStats.MagicalArmor;
	NewView.NewMagicalArmor = NewStats.MagicalArmor;

	TArray<FName> CommittedChoices;
	TSet<FName> CombinedSelection;
	if (!BuildCombinedSelection(CombinedSelection, &CommittedChoices))
	{
		NewView.ValidationMessage = LOCTEXT("InvalidCombinedSelection", "La sélection de talents est incohérente.");
		View = MoveTemp(NewView);
		RefreshNativeSlate();
		BP_OnLevelUpViewRefreshed();
		return;
	}

	if (!FRPGClassProgressionService::TryGetChoicePointBalance(
			ClassDefinition, Character.Level, CombinedSelection, NewView.GrantedChoicePoints, NewView.SpentChoicePoints, NewView.RemainingChoicePoints))
	{
		NewView.ValidationMessage = LOCTEXT("InvalidPointBalance", "Le budget de talents est invalide.");
		View = MoveTemp(NewView);
		RefreshNativeSlate();
		BP_OnLevelUpViewRefreshed();
		return;
	}

	TSet<FName> CommittedSet;
	for (const FName CommittedChoiceId : CommittedChoices)
	{
		CommittedSet.Add(CommittedChoiceId);
	}

	for (const FRPGClassProgressionChoiceDefinition& Choice : ClassDefinition->ProgressionChoices)
	{
		FRPGLevelUpChoiceView ChoiceView;
		ChoiceView.ChoiceId = Choice.ChoiceId;
		ChoiceView.DisplayName = Choice.DisplayName;
		ChoiceView.Description = Choice.Description;
		ChoiceView.PointCost = Choice.PointCost;
		ChoiceView.MinimumLevel = Choice.MinimumLevel;
		ChoiceView.bCommitted = CommittedSet.Contains(Choice.ChoiceId);
		ChoiceView.bPending = PendingChoiceIds.Contains(Choice.ChoiceId);

		if (ChoiceView.bCommitted)
		{
			ChoiceView.StatusText = LOCTEXT("ChoiceCommitted", "Acquis");
		}
		else if (ChoiceView.bPending)
		{
			ChoiceView.bAvailable = CanRemovePendingChoice(Choice.ChoiceId);
			ChoiceView.StatusText = LOCTEXT("ChoicePending", "Sélectionné — en attente de confirmation");
		}
		else
		{
			const ERPGClassProgressionChoiceAvailabilityReason Reason =
				FRPGClassProgressionService::GetChoiceAvailability(ClassDefinition, Character.Level, CombinedSelection, Choice.ChoiceId);
			ChoiceView.bAvailable = Reason == ERPGClassProgressionChoiceAvailabilityReason::None;
			ChoiceView.StatusText = ChoiceView.bAvailable ? LOCTEXT("ChoiceAvailable", "Disponible") : GetChoiceStatusText(static_cast<int32>(Reason));
			NewView.bHasSelectableChoices |= ChoiceView.bAvailable;
		}

		NewView.Choices.Add(MoveTemp(ChoiceView));
	}

	NewView.bCanConfirm = !PendingChoiceIds.IsEmpty() || NewView.RemainingChoicePoints <= 0 || !NewView.bHasSelectableChoices;

	if (PendingChoiceIds.IsEmpty() && NewView.RemainingChoicePoints > 0 && NewView.bHasSelectableChoices)
	{
		NewView.ValidationMessage = NewView.Presentation.SelectionPrompt;
	}

	View = MoveTemp(NewView);
	RefreshNativeSlate();
	BP_OnLevelUpViewRefreshed();
}

FText URPGLevelUpWidget::GetChoiceStatusText(int32 AvailabilityReasonValue) const
{
	const ERPGClassProgressionChoiceAvailabilityReason Reason = static_cast<ERPGClassProgressionChoiceAvailabilityReason>(AvailabilityReasonValue);
	switch (Reason)
	{
		case ERPGClassProgressionChoiceAvailabilityReason::InvalidClassDefinition:
			return LOCTEXT("ChoiceInvalidClass", "Classe invalide");
		case ERPGClassProgressionChoiceAvailabilityReason::InvalidLevel:
			return LOCTEXT("ChoiceInvalidLevel", "Niveau invalide");
		case ERPGClassProgressionChoiceAvailabilityReason::InvalidSelectionState:
			return LOCTEXT("ChoiceInvalidState", "Sélection incohérente");
		case ERPGClassProgressionChoiceAvailabilityReason::UnknownChoice:
			return LOCTEXT("ChoiceUnknown", "Talent inconnu");
		case ERPGClassProgressionChoiceAvailabilityReason::AlreadySelected:
			return LOCTEXT("ChoiceAlreadySelected", "Déjà acquis");
		case ERPGClassProgressionChoiceAvailabilityReason::LevelTooLow:
			return LOCTEXT("ChoiceLevelTooLow", "Niveau requis non atteint");
		case ERPGClassProgressionChoiceAvailabilityReason::MissingPrerequisite:
			return LOCTEXT("ChoiceMissingPrerequisite", "Prérequis manquant");
		case ERPGClassProgressionChoiceAvailabilityReason::InsufficientChoicePoints:
			return LOCTEXT("ChoiceInsufficientPoints", "Points de talent insuffisants");
		case ERPGClassProgressionChoiceAvailabilityReason::None:
		default:
			return LOCTEXT("ChoiceAvailable", "Disponible");
	}
}

#undef LOCTEXT_NAMESPACE

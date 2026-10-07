#include "UI/RPGProgressionFeedbackService.h"

#define LOCTEXT_NAMESPACE "RPGProgressionFeedbackService"

FText FRPGProgressionFeedbackService::GetCommitRejectMessage(ERPGClassProgressionCommitRejectReason Reason)
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
		case ERPGClassProgressionCommitRejectReason::EmptyRequest:
			return LOCTEXT("CommitEmpty", "Aucun talent n'est sélectionné.");
		case ERPGClassProgressionCommitRejectReason::DuplicateRequest:
			return LOCTEXT("CommitDuplicate", "Un même talent apparaît plusieurs fois dans la transaction.");
		case ERPGClassProgressionCommitRejectReason::UnknownChoice:
			return LOCTEXT("CommitUnknown", "Un talent de classe n'existe plus.");
		case ERPGClassProgressionCommitRejectReason::AlreadySelected:
			return LOCTEXT("CommitAlreadySelected", "Ce talent est déjà acquis.");
		case ERPGClassProgressionCommitRejectReason::LevelTooLow:
			return LOCTEXT("CommitLevelTooLow", "Le niveau requis n'est pas atteint.");
		case ERPGClassProgressionCommitRejectReason::MissingPrerequisite:
			return LOCTEXT("CommitMissingPrerequisite", "Un prérequis de talent manque.");
		case ERPGClassProgressionCommitRejectReason::InsufficientChoicePoints:
			return LOCTEXT("CommitInsufficientPoints", "Il n'y a pas assez de points de talent.");
		case ERPGClassProgressionCommitRejectReason::MutuallyExclusiveChoice:
			return LOCTEXT("CommitMutuallyExclusive", "Une autre variante exclusive de ce talent est déjà acquise.");
		case ERPGClassProgressionCommitRejectReason::None:
		default:
			return FText::GetEmpty();
	}
}

FRPGProgressionNotificationView FRPGProgressionFeedbackService::MakeTalentCommitNotification(
	const FRPGClassProgressionCommitResult& Result,
	const FText& TalentDisplayName)
{
	FRPGProgressionNotificationView Notification;
	if (Result.bCommitted)
	{
		Notification.Severity = ERPGProgressionNotificationSeverity::Success;
		Notification.Title = LOCTEXT("TalentAcquiredTitle", "Talent acquis");
		Notification.Message = TalentDisplayName.IsEmpty()
			? FText::Format(
				LOCTEXT("TalentAcquiredGeneric", "Acquisition réussie. Points de talent restants : {0}."),
				FText::AsNumber(Result.RemainingPoints))
			: FText::Format(
				LOCTEXT("TalentAcquiredNamed", "« {0} » a été acquis. Points de talent restants : {1}."),
				TalentDisplayName,
				FText::AsNumber(Result.RemainingPoints));
		return Notification;
	}

	Notification.Severity = ERPGProgressionNotificationSeverity::Error;
	Notification.Title = LOCTEXT("TalentRejectedTitle", "Acquisition refusée");
	Notification.Message = GetCommitRejectMessage(Result.RejectReason);
	if (Notification.Message.IsEmpty())
	{
		Notification.Message = LOCTEXT("TalentRejectedGeneric", "Le talent n'a pas pu être acquis.");
	}
	return Notification;
}

FRPGProgressionNotificationView FRPGProgressionFeedbackService::MakeLevelUpNotification(
	const FText& CharacterName,
	int32 PreviousLevel,
	int32 NewLevel,
	int32 TalentPointsGained)
{
	FRPGProgressionNotificationView Notification;
	Notification.Severity = ERPGProgressionNotificationSeverity::Success;
	Notification.Title = LOCTEXT("LevelUpTitle", "Niveau supérieur");

	if (TalentPointsGained > 0)
	{
		Notification.Message = FText::Format(
			LOCTEXT("LevelUpWithTalentPoint", "{0} passe du niveau {1} au niveau {2} et gagne {3} point(s) de talent."),
			CharacterName,
			FText::AsNumber(PreviousLevel),
			FText::AsNumber(NewLevel),
			FText::AsNumber(TalentPointsGained));
	}
	else
	{
		Notification.Message = FText::Format(
			LOCTEXT("LevelUpNoTalentPoint", "{0} passe du niveau {1} au niveau {2}."),
			CharacterName,
			FText::AsNumber(PreviousLevel),
			FText::AsNumber(NewLevel));
	}
	return Notification;
}

#undef LOCTEXT_NAMESPACE

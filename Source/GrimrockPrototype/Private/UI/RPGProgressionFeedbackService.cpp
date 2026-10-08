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

FText FRPGProgressionFeedbackService::GetSkillPurchaseRejectMessage(ERPGSkillPointPurchaseRejectReason Reason)
{
	switch (Reason)
	{
		case ERPGSkillPointPurchaseRejectReason::InvalidInventory:
		case ERPGSkillPointPurchaseRejectReason::InvalidCharacter:
			return LOCTEXT("SkillInvalidCharacter", "Le personnage n'est plus disponible.");
		case ERPGSkillPointPurchaseRejectReason::InvalidDefinition:
			return LOCTEXT("SkillInvalidDefinition", "La définition de cette compétence n'est plus valide.");
		case ERPGSkillPointPurchaseRejectReason::InvalidLevel:
			return LOCTEXT("SkillInvalidLevel", "Le niveau du personnage est invalide.");
		case ERPGSkillPointPurchaseRejectReason::InvalidSkillState:
		case ERPGSkillPointPurchaseRejectReason::InvalidPointBalance:
			return LOCTEXT("SkillInvalidState", "L'état des compétences du personnage est incohérent.");
		case ERPGSkillPointPurchaseRejectReason::NoSkillPoints:
			return LOCTEXT("SkillNoPoints", "Il ne reste aucun point de compétence.");
		case ERPGSkillPointPurchaseRejectReason::LevelRankCapReached:
			return LOCTEXT("SkillLevelCap", "Le plafond de rang actuel est atteint. Montez de niveau pour progresser davantage.");
		case ERPGSkillPointPurchaseRejectReason::SkillMaxRankReached:
			return LOCTEXT("SkillAbsoluteCap", "Cette compétence a atteint son rang maximal.");
		case ERPGSkillPointPurchaseRejectReason::MutationRejected:
			return LOCTEXT("SkillMutationRejected", "L'augmentation de rang a été refusée.");
		case ERPGSkillPointPurchaseRejectReason::None:
		default:
			return FText::GetEmpty();
	}
}

FRPGProgressionNotificationView FRPGProgressionFeedbackService::MakeSkillRankPurchaseNotification(
	const FRPGSkillPointPurchaseResult& Result,
	const FText& SkillDisplayName)
{
	FRPGProgressionNotificationView Notification;
	if (Result.bCommitted)
	{
		Notification.Severity = ERPGProgressionNotificationSeverity::Success;
		Notification.Title = LOCTEXT("SkillRankAcquiredTitle", "Compétence améliorée");
		Notification.Message = FText::Format(
			LOCTEXT("SkillRankAcquired", "« {0} » passe au rang {1}. Points de compétence restants : {2}."),
			SkillDisplayName,
			FText::AsNumber(Result.NewRank),
			FText::AsNumber(Result.RemainingPoints));
		return Notification;
	}

	Notification.Severity = ERPGProgressionNotificationSeverity::Warning;
	Notification.Title = LOCTEXT("SkillRankRejectedTitle", "Amélioration impossible");
	Notification.Message = GetSkillPurchaseRejectMessage(Result.RejectReason);
	if (Notification.Message.IsEmpty())
	{
		Notification.Message = LOCTEXT("SkillRankRejectedGeneric", "La compétence n'a pas pu être améliorée.");
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

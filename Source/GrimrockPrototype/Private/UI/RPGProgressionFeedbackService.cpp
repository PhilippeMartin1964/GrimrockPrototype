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

FText FRPGProgressionFeedbackService::GetSkillMutationRejectMessage(ERPGSkillPointMutationRejectReason Reason)
{
	switch (Reason)
	{
		case ERPGSkillPointMutationRejectReason::InvalidInventory:
		case ERPGSkillPointMutationRejectReason::InvalidCharacter:
			return LOCTEXT("SkillInvalidCharacter", "Le personnage n'est plus disponible.");
		case ERPGSkillPointMutationRejectReason::InvalidDefinition:
			return LOCTEXT("SkillInvalidDefinition", "La définition de cette compétence n'est plus valide.");
		case ERPGSkillPointMutationRejectReason::InvalidLevel:
			return LOCTEXT("SkillInvalidLevel", "Le niveau du personnage est invalide.");
		case ERPGSkillPointMutationRejectReason::InvalidSkillState:
		case ERPGSkillPointMutationRejectReason::InvalidPointBalance:
			return LOCTEXT("SkillInvalidState", "L'état des compétences du personnage est incohérent.");
		case ERPGSkillPointMutationRejectReason::NoSkillPoints:
			return LOCTEXT("SkillNoPoints", "Il ne reste aucun point de compétence.");
		case ERPGSkillPointMutationRejectReason::LevelRankCapReached:
			return LOCTEXT("SkillLevelCap", "Le plafond de rang actuel est atteint. Montez de niveau pour progresser davantage.");
		case ERPGSkillPointMutationRejectReason::SkillMaxRankReached:
			return LOCTEXT("SkillAbsoluteCap", "Cette compétence a atteint son rang maximal.");
		case ERPGSkillPointMutationRejectReason::InvalidSessionFloor:
			return LOCTEXT("SkillInvalidSessionFloor", "La limite d'annulation de cette session est incohérente.");
		case ERPGSkillPointMutationRejectReason::NoSessionPurchaseToUndo:
			return LOCTEXT("SkillNoSessionUndo", "Seuls les rangs attribués depuis l'ouverture actuelle de COMPÉTENCES peuvent être annulés.");
		case ERPGSkillPointMutationRejectReason::MutationRejected:
			return LOCTEXT("SkillMutationRejected", "L'augmentation de rang a été refusée.");
		case ERPGSkillPointMutationRejectReason::None:
		default:
			return FText::GetEmpty();
	}
}

FRPGProgressionNotificationView FRPGProgressionFeedbackService::MakeSkillRankPurchaseNotification(
	const FRPGSkillPointMutationResult& Result,
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
	Notification.Message = GetSkillMutationRejectMessage(Result.RejectReason);
	if (Notification.Message.IsEmpty())
	{
		Notification.Message = LOCTEXT("SkillRankRejectedGeneric", "La compétence n'a pas pu être améliorée.");
	}
	return Notification;
}

FRPGProgressionNotificationView FRPGProgressionFeedbackService::MakeSkillRankRefundNotification(
	const FRPGSkillPointMutationResult& Result,
	const FText& SkillDisplayName)
{
	FRPGProgressionNotificationView Notification;
	if (Result.bCommitted)
	{
		Notification.Severity = ERPGProgressionNotificationSeverity::Success;
		Notification.Title = LOCTEXT("SkillRankRefundedTitle", "Attribution annulée");
		Notification.Message = FText::Format(
			LOCTEXT("SkillRankRefunded", "« {0} » revient au rang {1}. Points de compétence disponibles : {2}."),
			SkillDisplayName,
			FText::AsNumber(Result.NewRank),
			FText::AsNumber(Result.RemainingPoints));
		return Notification;
	}

	Notification.Severity = ERPGProgressionNotificationSeverity::Warning;
	Notification.Title = LOCTEXT("SkillRankRefundRejectedTitle", "Annulation impossible");
	Notification.Message = GetSkillMutationRejectMessage(Result.RejectReason);
	if (Notification.Message.IsEmpty())
	{
		Notification.Message = LOCTEXT("SkillRankRefundRejectedGeneric", "Cette attribution ne peut pas être annulée.");
	}
	return Notification;
}

FText FRPGProgressionFeedbackService::GetAttributeMutationRejectMessage(
	ERPGAttributePointMutationRejectReason Reason)
{
	switch (Reason)
	{
		case ERPGAttributePointMutationRejectReason::InvalidInventory:
		case ERPGAttributePointMutationRejectReason::InvalidCharacter:
			return LOCTEXT("AttributeInvalidCharacter", "Le personnage n'est plus disponible.");
		case ERPGAttributePointMutationRejectReason::InvalidLevel:
			return LOCTEXT("AttributeInvalidLevel", "Le niveau du personnage est invalide.");
		case ERPGAttributePointMutationRejectReason::InvalidDefinition:
			return LOCTEXT("AttributeInvalidDefinition", "La classe ou la race du personnage n'est plus résolue.");
		case ERPGAttributePointMutationRejectReason::InvalidAttributeState:
		case ERPGAttributePointMutationRejectReason::InvalidPointBalance:
			return LOCTEXT("AttributeInvalidState", "L'état des caractéristiques du personnage est incohérent.");
		case ERPGAttributePointMutationRejectReason::NoAttributePoints:
			return LOCTEXT("AttributeNoPoints", "Il ne reste aucun point de caractéristique.");
		case ERPGAttributePointMutationRejectReason::AttributeCapReached:
			return LOCTEXT("AttributeCap", "Cette caractéristique a atteint sa valeur de base maximale de 20.");
		case ERPGAttributePointMutationRejectReason::InvalidSessionFloor:
			return LOCTEXT("AttributeInvalidSessionFloor", "La limite d'annulation de cette session est incohérente.");
		case ERPGAttributePointMutationRejectReason::NoSessionPurchaseToUndo:
			return LOCTEXT("AttributeNoSessionUndo", "Seuls les points attribués depuis l'ouverture actuelle de PERSONNAGE peuvent être annulés.");
		case ERPGAttributePointMutationRejectReason::MutationRejected:
			return LOCTEXT("AttributeMutationRejected", "La modification de caractéristique a été refusée.");
		case ERPGAttributePointMutationRejectReason::None:
		default:
			return FText::GetEmpty();
	}
}

FRPGProgressionNotificationView FRPGProgressionFeedbackService::MakeAttributePointPurchaseNotification(
	const FRPGAttributePointMutationResult& Result,
	const FText& AttributeDisplayName)
{
	FRPGProgressionNotificationView Notification;
	if (Result.bCommitted)
	{
		Notification.Severity = ERPGProgressionNotificationSeverity::Success;
		Notification.Title = LOCTEXT("AttributeImprovedTitle", "Caractéristique améliorée");
		Notification.Message = FText::Format(
			LOCTEXT("AttributeImproved", "« {0} » passe à {1}. Points de caractéristiques restants : {2}."),
			AttributeDisplayName,
			FText::AsNumber(Result.NewValue),
			FText::AsNumber(Result.RemainingPoints));
		return Notification;
	}

	Notification.Severity = ERPGProgressionNotificationSeverity::Warning;
	Notification.Title = LOCTEXT("AttributeImproveRejectedTitle", "Amélioration impossible");
	Notification.Message = GetAttributeMutationRejectMessage(Result.RejectReason);
	if (Notification.Message.IsEmpty())
	{
		Notification.Message = LOCTEXT("AttributeImproveRejectedGeneric", "La caractéristique n'a pas pu être améliorée.");
	}
	return Notification;
}

FRPGProgressionNotificationView FRPGProgressionFeedbackService::MakeAttributePointRefundNotification(
	const FRPGAttributePointMutationResult& Result,
	const FText& AttributeDisplayName)
{
	FRPGProgressionNotificationView Notification;
	if (Result.bCommitted)
	{
		Notification.Severity = ERPGProgressionNotificationSeverity::Success;
		Notification.Title = LOCTEXT("AttributeRefundedTitle", "Attribution annulée");
		Notification.Message = FText::Format(
			LOCTEXT("AttributeRefunded", "« {0} » revient à {1}. Points de caractéristiques disponibles : {2}."),
			AttributeDisplayName,
			FText::AsNumber(Result.NewValue),
			FText::AsNumber(Result.RemainingPoints));
		return Notification;
	}

	Notification.Severity = ERPGProgressionNotificationSeverity::Warning;
	Notification.Title = LOCTEXT("AttributeRefundRejectedTitle", "Annulation impossible");
	Notification.Message = GetAttributeMutationRejectMessage(Result.RejectReason);
	if (Notification.Message.IsEmpty())
	{
		Notification.Message = LOCTEXT("AttributeRefundRejectedGeneric", "Cette attribution ne peut pas être annulée.");
	}
	return Notification;
}

FRPGProgressionNotificationView FRPGProgressionFeedbackService::MakeLevelUpNotification(
	const FText& CharacterName,
	int32 PreviousLevel,
	int32 NewLevel,
	int32 SkillPointsGained,
	int32 TalentPointsGained,
	int32 AttributePointsGained,
	int32 UnlockedSkillRankCap)
{
	FRPGProgressionNotificationView Notification;
	Notification.Severity = ERPGProgressionNotificationSeverity::Success;
	Notification.Title = FText::Format(
		LOCTEXT("LevelUpTitle", "Niveau {0} atteint"),
		FText::AsNumber(NewLevel));
	Notification.DurationSeconds = 5.0f;

	FString Message = FText::Format(
		LOCTEXT("LevelUpBase", "{0} passe du niveau {1} au niveau {2}."),
		CharacterName,
		FText::AsNumber(PreviousLevel),
		FText::AsNumber(NewLevel)).ToString();

	if (SkillPointsGained > 0)
	{
		Message += TEXT(" ");
		Message += FText::Format(
			LOCTEXT("LevelUpSkillPoints", "+{0} point(s) de compétence."),
			FText::AsNumber(SkillPointsGained)).ToString();
	}

	if (TalentPointsGained > 0)
	{
		Message += TEXT(" ");
		Message += FText::Format(
			LOCTEXT("LevelUpTalentPoints", "+{0} point(s) de talent."),
			FText::AsNumber(TalentPointsGained)).ToString();
	}

	if (AttributePointsGained > 0)
	{
		Message += TEXT(" ");
		Message += FText::Format(
			LOCTEXT("LevelUpAttributePoints", "+{0} point(s) de caractéristique."),
			FText::AsNumber(AttributePointsGained)).ToString();
	}

	if (UnlockedSkillRankCap > 0)
	{
		Message += TEXT(" ");
		Message += FText::Format(
			LOCTEXT("LevelUpSkillRankCap", "Rang maximal des compétences : {0}."),
			FText::AsNumber(UnlockedSkillRankCap)).ToString();
	}

	Notification.Message = FText::FromString(MoveTemp(Message));
	return Notification;
}

#undef LOCTEXT_NAMESPACE

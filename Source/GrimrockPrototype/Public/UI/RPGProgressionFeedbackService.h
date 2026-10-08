#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGAttributePointService.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGSkillPointService.h"
#include "RPGProgressionFeedbackService.generated.h"

UENUM(BlueprintType)
enum class ERPGProgressionNotificationSeverity : uint8
{
	Info,
	Success,
	Warning,
	Error
};

USTRUCT(BlueprintType)
struct FRPGProgressionNotificationView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Progression|Notification")
	ERPGProgressionNotificationSeverity Severity = ERPGProgressionNotificationSeverity::Info;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Progression|Notification")
	FText Title;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Progression|Notification")
	FText Message;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Progression|Notification")
	float DurationSeconds = 3.5f;

	bool IsValid() const
	{
		return !Title.IsEmpty() || !Message.IsEmpty();
	}
};

/**
 * Autorité de présentation pour les retours de progression RPG.
 * Ne contient aucune règle métier : les décisions restent dans les services de progression.
 */
struct GRIMROCKPROTOTYPE_API FRPGProgressionFeedbackService
{
	/** Message français partagé pour une raison de rejet de transaction. */
	static FText GetCommitRejectMessage(ERPGClassProgressionCommitRejectReason Reason);

	/** Notification de résultat pour une acquisition de talent. */
	static FRPGProgressionNotificationView MakeTalentCommitNotification(
		const FRPGClassProgressionCommitResult& Result,
		const FText& TalentDisplayName);

	static FText GetSkillMutationRejectMessage(ERPGSkillPointMutationRejectReason Reason);

	static FRPGProgressionNotificationView MakeSkillRankPurchaseNotification(
		const FRPGSkillPointMutationResult& Result,
		const FText& SkillDisplayName);

	static FRPGProgressionNotificationView MakeSkillRankRefundNotification(
		const FRPGSkillPointMutationResult& Result,
		const FText& SkillDisplayName);

	static FText GetAttributeMutationRejectMessage(ERPGAttributePointMutationRejectReason Reason);

	static FRPGProgressionNotificationView MakeAttributePointPurchaseNotification(
		const FRPGAttributePointMutationResult& Result,
		const FText& AttributeDisplayName);

	static FRPGProgressionNotificationView MakeAttributePointRefundNotification(
		const FRPGAttributePointMutationResult& Result,
		const FText& AttributeDisplayName);

	/** Notification générique de montée de niveau, utilisable par les surfaces de progression. */
	static FRPGProgressionNotificationView MakeLevelUpNotification(
		const FText& CharacterName,
		int32 PreviousLevel,
		int32 NewLevel,
		int32 SkillPointsGained,
		int32 TalentPointsGained,
		int32 AttributePointsGained,
		int32 UnlockedSkillRankCap = 0);
};

#pragma once

#include "CoreMinimal.h"
#include "RPGSkillTypes.generated.h"

/** Attribute contributing to one skill check. */
UENUM(BlueprintType)
enum class ERPGSkillGoverningAttribute : uint8
{
	None UMETA(DisplayName = "Aucun"),
	Strength UMETA(DisplayName = "Force"),
	Dexterity UMETA(DisplayName = "Dextérité"),
	Constitution UMETA(DisplayName = "Constitution"),
	Intelligence UMETA(DisplayName = "Intelligence"),
	Wisdom UMETA(DisplayName = "Sagesse"),
	Charisma UMETA(DisplayName = "Charisme")
};

/** Sparse runtime rank for one skill. Rank zero is represented by no entry. */
USTRUCT(BlueprintType)
struct FRPGSkillRank
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Skills")
	FName SkillId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Skills", meta = (ClampMin = "0"))
	int32 Rank = 0;

	bool IsValid() const
	{
		return !SkillId.IsNone() && Rank > 0;
	}
};

/** Class/talent contribution to the existing Skill authority. */
USTRUCT(BlueprintType)
struct FRPGSkillProgressionModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills|Progression")
	FName SkillId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills|Progression", meta = (ClampMin = "-20", ClampMax = "20"))
	int32 CheckModifier = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills|Progression", meta = (ClampMin = "0", ClampMax = "5"))
	int32 RequirementGrantRankModifier = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills|Progression", meta = (ClampMin = "0", ClampMax = "20"))
	int32 SafeFailureMargin = 0;

	bool IsValid() const
	{
		return !SkillId.IsNone() && CheckModifier >= -20 && CheckModifier <= 20 &&
			RequirementGrantRankModifier >= 0 && RequirementGrantRankModifier <= 5 &&
			SafeFailureMargin >= 0 && SafeFailureMargin <= 20 &&
			(CheckModifier != 0 || RequirementGrantRankModifier != 0 || SafeFailureMargin != 0);
	}
};

/** Reason why one skill check could not be resolved. */
UENUM(BlueprintType)
enum class ERPGSkillCheckRejectReason : uint8
{
	None,
	InvalidDefinition,
	InvalidCharacterState,
	InvalidDifficulty,
	UntrainedNotAllowed
};

/** Fully inspectable result of one deterministic skill check. */
USTRUCT(BlueprintType)
struct FRPGSkillCheckResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Check")
	bool bResolved = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Check")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Check")
	ERPGSkillCheckRejectReason RejectReason = ERPGSkillCheckRejectReason::None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Check")
	FName SkillId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Check")
	int32 Rank = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Check")
	ERPGSkillGoverningAttribute GoverningAttribute = ERPGSkillGoverningAttribute::None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Check")
	int32 AttributeValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Check")
	int32 AttributeModifier = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Check")
	int32 ProgressionModifier = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Check")
	int32 SafeFailureMargin = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Check")
	int32 FailureMargin = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Check")
	bool bSafeFailure = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Check")
	int32 Roll = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Check")
	int32 Total = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|Check")
	int32 Difficulty = 0;
};

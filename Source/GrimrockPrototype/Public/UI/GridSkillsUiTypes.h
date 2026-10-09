#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGSkillTypes.h"
#include "RPG/RPGClassAsset.h"
#include "GridSkillsUiTypes.generated.h"

/** Read-only presentation of one canonical Skill for the selected character. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridSkillEntryView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	FName SkillId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	ERPGSkillGoverningAttribute GoverningAttribute = ERPGSkillGoverningAttribute::None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	int32 Rank = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	int32 MaxRank = 0;

	/** True when this Skill may be checked at rank zero. This is definition data, not progression state. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	bool bAllowUntrainedChecks = true;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	bool bTrained = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	int32 CurrentRankCap = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	bool bCanIncreaseRank = false;

	/** Session-only undo availability, supplied by UGridSkillsWidget. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	bool bCanDecreaseRank = false;
};

UENUM(BlueprintType)
enum class EGridTalentNodeState : uint8
{
	Acquired,
	Available,
	LockedLevel,
	LockedPrerequisite,
	LockedPoints,
	LockedExclusive
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridTalentDetailLineView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText Label;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText Value;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridTalentAcquisitionView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 MinimumLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 PointCost = 1;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	TArray<FText> PrerequisiteTalentNames;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText ExclusivityText;

	/** Populated only when a real recipe display-name authority exists. Raw Recipe_* ids are never shown. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	TArray<FText> GrantedRecipeNames;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridTalentVariantView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FName ChoiceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	EGridTalentNodeState State = EGridTalentNodeState::LockedPrerequisite;

	/** DESC01.14 structured projection. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	ERPGTalentPresentationType Type = ERPGTalentPresentationType::None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText TypeText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText StatusText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText Principle;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	TArray<FGridTalentDetailLineView> Effects;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	TArray<FGridTalentDetailLineView> Usage;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	bool bAcquired = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	bool bCanChoose = false;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridTalentNodeView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FName TalentNodeId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FName TalentBranchId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 Tier = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 MinimumLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 PointCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	EGridTalentNodeState State = EGridTalentNodeState::LockedPrerequisite;

	/** Presentation-only predecessor. Gameplay prerequisites remain authoritative in ProgressionChoice. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FName PreviousNodeId = NAME_None;

	/** Human-readable predecessor label for the detail panel; gameplay prerequisites stay authoritative in ProgressionChoice. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText PreviousNodeDisplayName;

	/** Concrete acquired ChoiceId when this conceptual node is acquired. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FName SelectedChoiceId = NAME_None;


	/** DESC01.14 canonical detail projection. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	ERPGTalentPresentationType Type = ERPGTalentPresentationType::None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText TypeText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText StatusText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText Principle;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	TArray<FGridTalentDetailLineView> Effects;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	TArray<FGridTalentDetailLineView> Usage;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FGridTalentAcquisitionView Acquisition;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FName SimpleChoiceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	bool bCanAcquireSimple = false;

	/** True only for one of the four exclusive multi-choice conceptual nodes. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	bool bHasExclusiveVariants = false;

	/** Concrete alternatives for true exclusive-variant nodes only. Empty for simple Talents. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	TArray<FGridTalentVariantView> Variants;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridTalentBranchView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FName TalentBranchId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	TArray<FGridTalentNodeView> Nodes;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 AcquiredNodeCount = 0;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridTalentTreeView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FName ClassId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	TArray<FGridTalentBranchView> Branches;
};

/** Temporary flat compatibility projection retained until UI-RPG03. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridTalentEntryView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FName ChoiceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 MinimumLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 PointCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	bool bSelected = false;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridSkillsPageView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	int32 CharacterIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	FGuid CharacterId;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	FText CharacterName;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	int32 CharacterLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	FName ClassId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	FText ClassDisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	TArray<FGridSkillEntryView> Skills;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	int32 GrantedSkillPoints = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	int32 SpentSkillPoints = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	int32 RemainingSkillPoints = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	int32 SkillRankCap = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	TArray<FGridTalentEntryView> Talents;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 GrantedTalentPoints = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 SpentTalentPoints = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 RemainingTalentPoints = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FGridTalentTreeView TalentTree;

	bool IsValid() const
	{
		return CharacterIndex != INDEX_NONE && CharacterId.IsValid();
	}
};

#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGSkillTypes.h"
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
struct GRIMROCKPROTOTYPE_API FGridTalentUnlockedActionView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FName ActionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 ActionPointCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 ManaCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 RangeCells = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 CooldownRounds = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 SourceItemQuantityCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText TargetSummary;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 AreaRadiusCells = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 MaximumResolvedTargets = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 ChainJumpRangeCells = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 ResolutionCount = 1;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 SubsequentResolutionAccuracyModifier = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	bool bRequiresLineOfSight = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	bool bAreaCenteredOnParty = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	bool bAffectsAlliesInArea = false;
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
	FText Description;

	/**
	 * Read-only projection of class CombatActions unlocked by this concrete
	 * ChoiceId or by one of its GrantedRequirementIds.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	TArray<FGridTalentUnlockedActionView> UnlockedActions;

	/** Read-only effect category inferred from authoritative progression profiles. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText EffectCategory;

	/** Structured readable summary projected from canonical modifier/reaction/skill/party data. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText MechanicsSummary;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	bool bSelected = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	bool bAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	EGridTalentNodeState State = EGridTalentNodeState::LockedPrerequisite;
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

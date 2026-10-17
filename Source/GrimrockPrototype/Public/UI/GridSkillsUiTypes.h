#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGSkillTypes.h"
#include "RPG/RPGClassAsset.h"
#include "UI/RPGSkillPresentationAsset.h"
#include "GridSkillsUiTypes.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class EGridSkillSortColumn : uint8
{
	Name,
	Attribute,
	Affinity,
	Rank
};

UENUM(BlueprintType)
enum class EGridSkillSortDirection : uint8
{
	Ascending,
	Descending
};

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
	ERPGSkillGoverningAttribute GoverningAttribute = ERPGSkillGoverningAttribute::None;

	/** Presentation-only relation between this Skill and the selected character class. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	EGridSkillClassAffinity ClassAffinity = EGridSkillClassAffinity::Neutral;

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


/** Stable UI semantic for the current training state of one Skill. */
UENUM(BlueprintType)
enum class EGridSkillTrainingState : uint8
{
	UntrainedAllowed,
	TrainingRequired,
	Trained
};

/** Resolved class label used by the Skill detail read model. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridSkillClassAffinityView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	FName ClassId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	FText DisplayName;

	bool IsValid() const
	{
		return !ClassId.IsNone() && !DisplayName.IsEmpty();
	}
};

/** Resolved related-Skill label used by the Skill detail read model. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridSkillSynergyView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	FName SkillId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	FText DisplayName;

	bool IsValid() const
	{
		return !SkillId.IsNone() && !DisplayName.IsEmpty();
	}
};

/**
 * Presentation-ready detail projection for one selected Skill.
 *
 * This structure owns no gameplay state. Dynamic values are projected from the
 * selected character and existing Skill services; authored guidance comes from
 * URPGSkillPresentationAsset.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridSkillDetailView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	FName SkillId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	ERPGSkillGoverningAttribute GoverningAttribute = ERPGSkillGoverningAttribute::None;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	int32 GoverningAttributeValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	int32 GoverningAttributeModifier = 0;

	/** Rank + governing attribute modifier only; excludes Talent, party and contextual modifiers. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	int32 BaseCheckBonus = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	int32 Rank = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	int32 MaxRank = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	int32 CurrentRankCap = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	int32 RemainingSkillPoints = 0;

	/** Zero only when the Skill is already at MaxRank. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	int32 NextRank = 0;

	/** Cost of NextRank; zero only when no next rank exists. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	int32 NextRankCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	bool bAllowUntrainedChecks = true;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	EGridSkillTrainingState TrainingState = EGridSkillTrainingState::UntrainedAllowed;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	bool bCanIncreaseRank = false;

	/** Session-only undo availability; never persistent Skill authority. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	bool bCanDecreaseRank = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	bool bAtCurrentRankCap = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	bool bAtSkillMaxRank = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	FText UsageDescription;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	FText BuildAdvice;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	TArray<FGridSkillClassAffinityView> RecommendedClasses;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	TArray<FGridSkillClassAffinityView> UsefulClasses;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	TArray<FGridSkillClassAffinityView> DiscouragedClasses;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI|Detail")
	TArray<FGridSkillSynergyView> Synergies;

	bool IsValid() const
	{
		if (SkillId.IsNone() ||
			MaxRank < 1 ||
			Rank < 0 ||
			Rank > MaxRank ||
			CurrentRankCap < 1 ||
			CurrentRankCap > MaxRank ||
			RemainingSkillPoints < 0)
		{
			return false;
		}

		if (GoverningAttribute == ERPGSkillGoverningAttribute::None &&
			(GoverningAttributeValue != 0 || GoverningAttributeModifier != 0))
		{
			return false;
		}

		if (BaseCheckBonus != Rank + GoverningAttributeModifier)
		{
			return false;
		}

		const EGridSkillTrainingState ExpectedTrainingState =
			Rank > 0
				? EGridSkillTrainingState::Trained
				: (bAllowUntrainedChecks
					? EGridSkillTrainingState::UntrainedAllowed
					: EGridSkillTrainingState::TrainingRequired);
		if (TrainingState != ExpectedTrainingState)
		{
			return false;
		}

		const bool bExpectedAtSkillMaxRank = Rank >= MaxRank;
		const bool bExpectedAtCurrentRankCap = Rank >= CurrentRankCap;
		if (bAtSkillMaxRank != bExpectedAtSkillMaxRank ||
			bAtCurrentRankCap != bExpectedAtCurrentRankCap)
		{
			return false;
		}

		const int32 ExpectedNextRank = Rank < MaxRank ? Rank + 1 : 0;
		if (NextRank != ExpectedNextRank ||
			(ExpectedNextRank == 0 && NextRankCost != 0) ||
			(ExpectedNextRank > 0 && NextRankCost < 1))
		{
			return false;
		}

		if (bCanIncreaseRank &&
			(bAtSkillMaxRank ||
			 bAtCurrentRankCap ||
			 RemainingSkillPoints < NextRankCost))
		{
			return false;
		}

		if (bCanDecreaseRank && Rank <= 0)
		{
			return false;
		}

		TSet<FName> RecommendedIds;
		for (const FGridSkillClassAffinityView& ClassView : RecommendedClasses)
		{
			if (!ClassView.IsValid() || RecommendedIds.Contains(ClassView.ClassId))
			{
				return false;
			}
			RecommendedIds.Add(ClassView.ClassId);
		}

		TSet<FName> UsefulIds;
		for (const FGridSkillClassAffinityView& ClassView : UsefulClasses)
		{
			if (!ClassView.IsValid() ||
				UsefulIds.Contains(ClassView.ClassId) ||
				RecommendedIds.Contains(ClassView.ClassId))
			{
				return false;
			}
			UsefulIds.Add(ClassView.ClassId);
		}

		TSet<FName> DiscouragedIds;
		for (const FGridSkillClassAffinityView& ClassView : DiscouragedClasses)
		{
			if (!ClassView.IsValid() ||
				DiscouragedIds.Contains(ClassView.ClassId) ||
				RecommendedIds.Contains(ClassView.ClassId) ||
				UsefulIds.Contains(ClassView.ClassId))
			{
				return false;
			}
			DiscouragedIds.Add(ClassView.ClassId);
		}

		TSet<FName> SynergyIds;
		for (const FGridSkillSynergyView& Synergy : Synergies)
		{
			if (!Synergy.IsValid() ||
				Synergy.SkillId == SkillId ||
				SynergyIds.Contains(Synergy.SkillId))
			{
				return false;
			}
			SynergyIds.Add(Synergy.SkillId);
		}

		return true;
	}
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

UENUM(BlueprintType)
enum class EGridTalentDetailLineSemantic : uint8
{
	None,
	Context,
	Action,
	ActionPointCost,
	ManaCost,
	ItemCost,
	Target,
	Range,
	Area,
	LineOfSight,
	ResolutionCount,
	Cooldown,
	LevelRequirement,
	TalentPointCost,
	Prerequisite,
	Exclusivity,
	Recipe
};

UENUM(BlueprintType)
enum class EGridTalentDetailValueStyle : uint8
{
	Text,
	Integer,
	SignedInteger,
	Percent,
	SignedPercent
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridTalentDetailLineView
{
	GENERATED_BODY()

	/** Stable presentation meaning. Never derive glyphs from localized Label text. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	EGridTalentDetailLineSemantic Semantic = EGridTalentDetailLineSemantic::None;

	/** Stable UI vocabulary key. Player-facing wording resolves from ST_UI; Recipe content uses the Recipe catalog. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FName LabelTextId = NAME_None;

	/** Optional data-driven value key. When set, it is authoritative over Value. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FName ValueTextId = NAME_None;

	/** Dynamic/authored text value. Empty for numeric rows and when ValueTextId supplies the complete value. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FText Value;

	/** Raw mechanic value for numeric rows. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 NumericValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	bool bHasNumericValue = false;

	/**
	 * Universal mechanic notation for NumericValue.
	 * C++ renders signs and '%' from this semantic; only linguistic wording is localized.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	EGridTalentDetailValueStyle ValueStyle = EGridTalentDetailValueStyle::Text;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridRecipeDisplayView
{
	GENERATED_BODY()

	/** Stable gameplay/content identity. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Crafting|UI")
	FName RecipeId = NAME_None;

	/** Player-facing authored content from DA_RPGRecipeCatalog. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Crafting|UI")
	FText DisplayName;
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

	/** Stable localizable text key for the exclusivity rule, empty when not applicable. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	FName ExclusivityTextId = NAME_None;

	/** Granted Recipe content, resolved from the canonical Recipe catalog. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	TArray<FGridRecipeDisplayView> GrantedRecipes;
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

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

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Skills|UI")
	bool bTrained = false;
};

/** Read-only presentation alias of one acquired MON15 ProgressionChoice talent. */
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

/** Complete read-only model consumed by WBP_GridSkills. */
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
	TArray<FGridSkillEntryView> Skills;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	TArray<FGridTalentEntryView> Talents;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 GrantedTalentPoints = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 SpentTalentPoints = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|UI")
	int32 RemainingTalentPoints = 0;

	bool IsValid() const
	{
		return CharacterIndex != INDEX_NONE && CharacterId.IsValid();
	}
};

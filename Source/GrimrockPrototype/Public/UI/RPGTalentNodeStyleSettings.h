#pragma once

#include "CoreMinimal.h"
#include "UI/GridSkillsUiTypes.h"
#include "RPGTalentNodeStyleSettings.generated.h"

/**
 * Pure presentation style for one semantic Talent node state.
 *
 * Runtime semantics remain EGridTalentNodeState. RGB/tint values are supplied
 * by Config/DefaultGame.ini rather than hard-coded by the widget.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FRPGTalentNodeStateVisualStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Style")
	FLinearColor TextColor = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Style")
	FLinearColor NodeTint = FLinearColor::Transparent;

	bool IsValid() const
	{
		return TextColor.A > 0.0f && NodeTint.A > 0.0f;
	}
};

/**
 * Data-driven visual state styling for WBP_RPGTalentNode.
 *
 * This object owns no gameplay state. It maps the semantic read-model state to
 * configurable presentation values loaded from DefaultGame.ini.
 */
UCLASS(Config=Game, DefaultConfig)
class GRIMROCKPROTOTYPE_API URPGTalentNodeStyleSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Style")
	FRPGTalentNodeStateVisualStyle AcquiredStyle;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Style")
	FRPGTalentNodeStateVisualStyle AvailableStyle;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Style")
	FRPGTalentNodeStateVisualStyle LockedLevelStyle;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Style")
	FRPGTalentNodeStateVisualStyle LockedPrerequisiteStyle;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Style")
	FRPGTalentNodeStateVisualStyle LockedPointsStyle;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RPG|Talents|Style")
	FRPGTalentNodeStateVisualStyle LockedExclusiveStyle;

	bool IsValidDefinition() const;

	bool TryGetStyle(EGridTalentNodeState State, FRPGTalentNodeStateVisualStyle& OutStyle) const;
};

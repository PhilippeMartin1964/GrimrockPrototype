#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RPG/RPGCharacterTypes.h"
#include "RPGStoryCompanionAsset.generated.h"

class URPGClassAsset;
class URPGRaceAsset;
class UTexture2D;

/** Data-driven identity and starting state for one narrative companion. */
UCLASS (BlueprintType)
class GRIMROCKPROTOTYPE_API URPGStoryCompanionAsset : public UPrimaryDataAsset
{
    GENERATED_BODY ()

public:
    /** Stable human-readable content identity. */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Companion")
    FName CompanionId = NAME_None;

    /**
     * Stable runtime/save identity copied into FGridCharacterInventoryState.
     * Generate it once when authoring the companion and never change it after
     * a production save has referenced this asset.
     */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Companion")
    FGuid CharacterId;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Companion")
    FText DisplayName;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "RPG|Companion",
        meta = (MultiLine = "true"))
    FText ShortDescription;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Companion")
    TObjectPtr<URPGRaceAsset> RaceDefinition = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Companion")
    TObjectPtr<URPGClassAsset> ClassDefinition = nullptr;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "RPG|Companion",
        meta = (ClampMin = "1", ClampMax = "20"))
    int32 Level = 1;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Companion|Visual")
    ERPGCharacterPortraitGender PortraitGender =
        ERPGCharacterPortraitGender::Male;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Companion|Visual")
    FName PortraitVariantId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Companion|Visual")
    TSoftObjectPtr<UTexture2D> Portrait;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Companion|Visual")
    TSoftObjectPtr<UTexture2D> FullBody;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Companion|Visual")
    TSoftObjectPtr<UTexture2D> ClassIcon;

    /** Content contract for the later equipment-materialization tranche. */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Companion|Equipment")
    TArray<FName> StartingEquipmentIds;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "RPG|Companion|Recruitment",
        meta = (MultiLine = "true"))
    FText RecruitmentConditionText;

    UFUNCTION (BlueprintPure, Category = "RPG|Companion")
    bool IsValidDefinition () const;

    /** Generates the immutable CharacterId only when it is currently invalid. */
    UFUNCTION (BlueprintCallable, CallInEditor, Category = "RPG|Companion")
    void GenerateCharacterId ();
};

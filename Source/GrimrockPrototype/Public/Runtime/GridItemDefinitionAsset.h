#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridItemDefinitionAsset.generated.h"

class UStaticMesh;
class UTexture2D;

UENUM (BlueprintType)
enum class EGridItemType : uint8
{
    None,
    Torch,
    Weapon,
    Shield,
    Armor,
    Jewelry,
    Key,
    Gem,
    Potion,
    Scroll,
    Book,
    Food,
    Component,
    Quest,
    Misc
};

UCLASS (BlueprintType)
class GRIMROCKPROTOTYPE_API UGridItemDefinitionAsset : public UPrimaryDataAsset
{
    GENERATED_BODY ()

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Item")
    FName ItemDefinitionId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Item")
    FText DisplayName;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (MultiLine = "true"))
    FText Description;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Reading", meta = (MultiLine = "true"))
    FText ReadText;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Item")
    EGridItemType ItemType = EGridItemType::None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Item")
    float Weight = 0.0f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Item")
    bool bStackable = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (EditCondition = "bStackable", ClampMin = "1"))
    int32 MaxStackSize = 1;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Equipment")
    TArray<EGridEquipmentSlot> CompatibleEquipmentSlots;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Equipment|Stats")
    FGridEquipmentStatBonus EquipmentStatBonus;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Equipment|Resistances")
    FGridDamageResistanceSet EquipmentResistanceBonus;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Equipment|Offense")
    bool bProvidesAttack = false;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Equipment|Offense",
        meta = (EditCondition = "bProvidesAttack"))
    FGridOffensiveEquipmentProfile OffensiveProfile;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Visual")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Visual")
    TSoftObjectPtr<UStaticMesh> WorldMesh;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Visual")
    TSoftObjectPtr<UStaticMesh> EquippedMesh;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Throw")
    bool bThrowable = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Throw", meta = (EditCondition = "bThrowable", ClampMin = "0.0"))
    float ThrowSpeed = 1800.0f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Throw", meta = (EditCondition = "bThrowable", ClampMin = "0.0"))
    float ThrowArc = 0.08f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Throw", meta = (EditCondition = "bThrowable", ClampMin = "0.0"))
    float ThrowLifeSeconds = 5.0f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Throw", meta = (EditCondition = "bThrowable", ClampMin = "0.0"))
    float ThrowImpactDropOffset = 12.0f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Torch")
    bool bCanEmitLight = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Torch", meta = (EditCondition = "bCanEmitLight"))
    bool bDefaultLightEnabled = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Torch", meta = (EditCondition = "bCanEmitLight"))
    float LightRadius = 600.0f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Tags")
    TArray<FName> ItemTags;

    UFUNCTION (BlueprintCallable, Category = "Item")
    bool IsValidDefinition () const;

    UFUNCTION (BlueprintCallable, Category = "Item")
    bool CanEquipToSlot (EGridEquipmentSlot Slot) const;

    UFUNCTION (BlueprintPure, Category = "Equipment|Offense")
    bool HasValidOffensiveProfile () const;

    UFUNCTION (BlueprintPure, Category = "Equipment|Offense")
    bool CanProvideAttackFromSlot (EGridEquipmentSlot Slot) const;
};

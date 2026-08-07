#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/Combat/GridPlayerAttackPresentationTypes.h"
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

    /**
     * Generic actions granted while this item is equipped. An empty array
     * keeps the legacy bProvidesAttack / OffensiveProfile adapter active.
     */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Equipment|Combat Actions")
    TArray<FGridCombatActionDefinition> CombatActions;

    /**
     * Enables one inventory-backed combat action for a potion or scroll.
     * Runtime identity, source policy and a minimum source cost of one are
     * normalized from ItemDefinitionId by BuildQuickItemCombatActionDefinition.
     */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|Quick Item")
    bool bProvidesQuickItemCombatAction = false;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Combat|Quick Item",
        meta = (EditCondition = "bProvidesQuickItemCombatAction"))
    FGridCombatActionDefinition QuickItemCombatAction;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Equipment|Offense|Presentation")
    bool bProvidesAttackPresentation = false;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Equipment|Offense|Presentation",
        meta = (EditCondition = "bProvidesAttackPresentation"))
    FGridPlayerAttackPresentationProfile
        PlayerAttackPresentationProfile;

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

    /**
     * Local projectile-mesh transform. The default turns a flat XY mesh
     * toward the source while the actor X axis follows its velocity.
     */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Throw|Visual", meta = (EditCondition = "bThrowable"))
    FRotator ThrowVisualRelativeRotation = FRotator (-90.0f, 0.0f, 0.0f);

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Throw|Visual", meta = (EditCondition = "bThrowable"))
    FVector ThrowVisualRelativeScale = FVector (1.5f);

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Throw|Visual", meta = (EditCondition = "bThrowable", ClampMin = "0.0"))
    float ThrowVisualSpinDegreesPerSecond = 1080.0f;

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

    UFUNCTION (BlueprintPure, Category = "Equipment|Combat Actions")
    bool HasValidCombatActions () const;

    /** Builds the normalized Use_<ItemDefinitionId> catalogue definition. */
    bool BuildQuickItemCombatActionDefinition (
        FGridCombatActionDefinition& OutDefinition) const;

    UFUNCTION (BlueprintPure, Category = "Equipment|Offense|Presentation")
    bool HasValidPlayerAttackPresentation () const;

    /** Uses the dedicated equipped mesh when assigned, otherwise WorldMesh. */
    UStaticMesh* LoadHeldMesh () const;
};

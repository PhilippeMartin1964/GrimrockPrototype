#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "GridDungeonTrapTypes.generated.h"

class UMaterialInterface;

/** TRAP01.1: authored dungeon hazards are opt-in Trigger definitions.
 * They are not RPG03.9 temporary combat traps or visible pickup actors.
 */
UENUM(BlueprintType)
enum class EGridDungeonTrapDetectionSense : uint8
{
    Visual UMETA(DisplayName = "Visual"),
    Audible UMETA(DisplayName = "Audible"),
    Both UMETA(DisplayName = "Visual or audible")
};

UENUM(BlueprintType)
enum class EGridDungeonTrapActivationPolicy : uint8
{
    OnEnterCell UMETA(DisplayName = "Enter cell"),
    OnCrossBoundary UMETA(DisplayName = "Cross boundary"),
    OnInteract UMETA(DisplayName = "Interact")
};

UENUM(BlueprintType)
enum class EGridDungeonTrapState : uint8
{
    Unknown UMETA(DisplayName = "Not a dungeon trap"),
    Hidden UMETA(DisplayName = "Hidden"),
    Detected UMETA(DisplayName = "Detected"),
    Triggered UMETA(DisplayName = "Triggered"),
    Disarmed UMETA(DisplayName = "Disarmed"),
    Disabled UMETA(DisplayName = "Disabled")
};

/** Shared immutable authoring defaults. Placement is owned by
 * UGridWorldObjectDefinitionAsset::PlacementSurface (Floor/Wall/Ceiling).
 * TRAP01.2 implements detection and TRAP01.3 implements damage and triggers.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridDungeonTrapDefinitionParams
{
    GENERATED_BODY()

    /** False on legacy Triggers: opt-in guarantees no change to existing puzzles. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Trap")
    bool bIsDungeonTrap = false;

    /** Authoritative v0.1 invariant: a trap is never revealed merely by its mesh. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Trap",
        meta = (EditCondition = "bIsDungeonTrap", EditConditionHides))
    bool bInitiallyHidden = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Trap|Perception",
        meta = (EditCondition = "bIsDungeonTrap", EditConditionHides))
    EGridDungeonTrapDetectionSense DetectionSense = EGridDungeonTrapDetectionSense::Visual;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Trap|Perception",
        meta = (ClampMin = "1", ClampMax = "40", EditCondition = "bIsDungeonTrap", EditConditionHides))
    int32 DetectionDifficulty = 10;

    /** Upper cap only; actual range comes from the strongest party observer. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Trap|Perception",
        meta = (ClampMin = "1", ClampMax = "4", EditCondition = "bIsDungeonTrap", EditConditionHides))
    int32 MaxDetectionRangeCells = 4;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Trap|Activation",
        meta = (EditCondition = "bIsDungeonTrap", EditConditionHides))
    EGridDungeonTrapActivationPolicy ActivationPolicy = EGridDungeonTrapActivationPolicy::OnEnterCell;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Trap|Activation",
        meta = (EditCondition = "bIsDungeonTrap", EditConditionHides))
    bool bOneShot = true;

    /** TRAP01.3: one damage roll, each living party member resolves defenses. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Trap|Damage",
        meta = (EditCondition = "bIsDungeonTrap", EditConditionHides))
    EGridDamageType DamageType = EGridDamageType::Physical;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Trap|Damage",
        meta = (EditCondition = "bIsDungeonTrap", EditConditionHides))
    EGridPhysicalDamageSubtype PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Trap|Damage",
        meta = (ClampMin = "0", EditCondition = "bIsDungeonTrap", EditConditionHides))
    int32 MinimumDamage = 4;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Trap|Damage",
        meta = (ClampMin = "0", EditCondition = "bIsDungeonTrap", EditConditionHides))
    int32 MaximumDamage = 8;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Trap|Damage",
        meta = (EditCondition = "bIsDungeonTrap", EditConditionHides))
    bool bAffectsEntireParty = true;

    /** No tool consumed on a successful disarm unless a later approved rule says so. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Trap|Disarm",
        meta = (EditCondition = "bIsDungeonTrap", EditConditionHides))
    FName RequiredToolTag = TEXT("Tool.Disarm");

    /** Presentation only: never changes detection, damage, collision or interaction. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Trap|Reveal",
        meta = (EditCondition = "bIsDungeonTrap", EditConditionHides))
    bool bEnableDangerHighlight = true;

    /** Optional danger overlay. Author color/intensity/opacity in a Material Instance.
     * If unset, base materials remain; CustomDepth can still drive an authored postprocess.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Trap|Reveal",
        meta = (EditCondition = "bIsDungeonTrap && bEnableDangerHighlight", EditConditionHides))
    TObjectPtr<UMaterialInterface> RevealedOverlayMaterial = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Trap|Reveal",
        meta = (EditCondition = "bIsDungeonTrap && bEnableDangerHighlight", EditConditionHides))
    bool bUseDangerCustomDepth = true;

    /** 0 is unmarked; 1/2 are editor hover/selection. Runtime clamps to 3..255. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Trap|Reveal",
        meta = (ClampMin = "3", ClampMax = "255",
            EditCondition = "bIsDungeonTrap && bEnableDangerHighlight && bUseDangerCustomDepth", EditConditionHides))
    int32 DangerStencilValue = 252;

    bool IsValidContract() const
    {
        return !bIsDungeonTrap ||
            (bInitiallyHidden &&
             DetectionDifficulty >= 1 && DetectionDifficulty <= 40 &&
             MaxDetectionRangeCells >= 1 && MaxDetectionRangeCells <= 4 &&
             MinimumDamage >= 0 && MaximumDamage >= MinimumDamage &&
             (DamageType == EGridDamageType::Physical ||
              PhysicalSubtype == EGridPhysicalDamageSubtype::None));
    }
};

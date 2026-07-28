#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "GridMonsterVFXTypes.generated.h"

class UNiagaraSystem;
class USceneComponent;

UENUM (BlueprintType)
enum class EGridMonsterVFXEvent : uint8
{
    Alert,
    Attack,
    ImpactHit,
    ImpactMiss,
    Hurt,
    Death
};

USTRUCT (BlueprintType)
struct FGridMonsterVFXEventDefinition
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|VFX")
    TArray<TSoftObjectPtr<UNiagaraSystem>> Systems;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|VFX")
    bool bAttachToSource = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|VFX")
    FName SocketName = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|VFX")
    FVector LocationOffset = FVector::ZeroVector;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|VFX")
    FRotator RotationOffset = FRotator::ZeroRotator;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Monster|VFX")
    FVector Scale = FVector::OneVector;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Monster|VFX",
        meta = (ClampMin = "0.0"))
    float CooldownSeconds = 0.0f;

    bool IsValidDefinition () const
    {
        if (LocationOffset.ContainsNaN () ||
            RotationOffset.ContainsNaN () ||
            Scale.ContainsNaN () ||
            Scale.X <= 0.0 ||
            Scale.Y <= 0.0 ||
            Scale.Z <= 0.0 ||
            !FMath::IsFinite (CooldownSeconds) ||
            CooldownSeconds < 0.0f)
        {
            return false;
        }

        for (const TSoftObjectPtr<UNiagaraSystem>& System : Systems)
        {
            if (System.IsNull ())
            {
                return false;
            }
        }
        return true;
    }

    bool HasConfiguredSystem () const
    {
        return !Systems.IsEmpty ();
    }
};

USTRUCT (BlueprintType)
struct FGridMonsterVFXSpawnRequest
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|VFX")
    int32 SequenceNumber = 0;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|VFX")
    EGridMonsterVFXEvent Event = EGridMonsterVFXEvent::ImpactHit;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|VFX")
    FName MonsterId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|VFX")
    FName AttackId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|VFX")
    TObjectPtr<UNiagaraSystem> System = nullptr;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|VFX")
    FTransform WorldTransform = FTransform::Identity;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|VFX")
    bool bAttachToSource = false;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|VFX")
    TObjectPtr<USceneComponent> AttachComponent = nullptr;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|VFX")
    FName SocketName = NAME_None;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|VFX")
    int32 TargetCharacterIndex = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|VFX")
    EGridDamageType DamageType = EGridDamageType::Physical;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|VFX")
    EGridPhysicalDamageSubtype PhysicalSubtype =
        EGridPhysicalDamageSubtype::None;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|VFX")
    bool bHasAttackResult = false;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Monster|VFX")
    FGridAttackResult AttackResult;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (
    FGridMonsterVFXSpawnRequestedSignature,
    FGridMonsterVFXSpawnRequest,
    Request);

/** Pure presentation-only selection helpers. They never read gameplay RNG. */
class GRIMROCKPROTOTYPE_API FGridMonsterVFXSelector
{
public:
    static int32 BuildPresentationSeed (
        const FGuid& PersistenceId,
        FName MonsterId,
        EGridMonsterVFXEvent Event,
        int32 OccurrenceNumber);

    static int32 SelectVariationIndex (
        int32 PresentationSeed,
        int32 VariationCount);
};

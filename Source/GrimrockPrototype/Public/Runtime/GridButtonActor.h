#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridTypes.h"
#include "GridButtonActor.generated.h"

class UStaticMeshComponent;

UCLASS ()
class GRIMROCKPROTOTYPE_API AGridButtonActor : public AActor
{
    GENERATED_BODY ()

public:
    AGridButtonActor ();

    virtual void BeginPlay () override;
    virtual void Tick (float DeltaSeconds) override;

public:
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* SceneRoot;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* ButtonMeshComponent;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Button")
    float PressDistance = 6.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Button")
    float PressDuration = 0.08f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Button")
    float ReleaseDuration = 0.10f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Button")
    float HoldTime = 0.15f;

    UPROPERTY (BlueprintReadOnly, Category = "Button")
    int32 CellX = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Button")
    int32 CellY = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Button")
    EGridEdge Edge = EGridEdge::None;

    UFUNCTION (BlueprintCallable, Category = "Button")
    void InitializeButton (
        UStaticMesh* InButtonMesh,
        UMaterialInterface* InMaterial,
        const FVector& InWorldLocation,
        const FRotator& InWorldRotation,
        int32 InCellX,
        int32 InCellY,
        EGridEdge InEdge);

    UFUNCTION (BlueprintCallable, Category = "Button")
    void TriggerPress ();

    UFUNCTION (BlueprintCallable, Category = "Button")
    bool MatchesEdge (int32 InCellX, int32 InCellY, EGridEdge InEdge) const;

protected:
    void UpdateAnimation (float DeltaSeconds);
    FVector GetPressAxis () const;

private:
    enum class EButtonAnimState : uint8
    {
        Idle,
        Pressing,
        Holding,
        Releasing
    };

    FVector ReleasedLocation = FVector::ZeroVector;
    FVector PressedLocation = FVector::ZeroVector;

    EButtonAnimState AnimState = EButtonAnimState::Idle;
    float StateElapsed = 0.f;
};
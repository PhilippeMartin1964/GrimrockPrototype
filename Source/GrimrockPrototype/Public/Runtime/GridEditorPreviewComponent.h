#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GridEditorPreviewComponent.generated.h"

class AGridLevelRuntimeActor;
class AGridEditorPreviewObjectActor;
struct FGridLevelObjectData;

UCLASS (ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridEditorPreviewComponent : public UActorComponent
{
    GENERATED_BODY ()

public:
    UGridEditorPreviewComponent ();

    void Initialize (AGridLevelRuntimeActor* InRuntimeActor);

    void ClearPreviewObjects ();
    void RebuildPreviewObjects ();

    void SetHoveredObject (FGuid ObjectId);
    void SetSelectedObject (FGuid ObjectId);
    void CleanupOrphanPreviewObjects ();

private:
    UPROPERTY (Transient)
    TObjectPtr<AGridLevelRuntimeActor> RuntimeActor;

    UPROPERTY (Transient)
    TArray<TObjectPtr<AGridEditorPreviewObjectActor>> SpawnedPreviewObjects;

    UPROPERTY (Transient)
    FGuid CurrentHoveredObjectId;

    UPROPERTY (Transient)
    FGuid CurrentSelectedObjectId;

private:
    bool IsPreviewableObject (const FGridLevelObjectData& ObjectData) const;
    void AddPreviewObject (const FGridLevelObjectData& ObjectData);
};
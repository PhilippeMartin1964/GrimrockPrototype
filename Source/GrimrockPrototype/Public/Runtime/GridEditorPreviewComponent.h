#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GridEditorPreviewComponent.generated.h"

class AGridLevelRuntimeActor;
class AGridEditorPreviewObjectActor;
struct FGridLooseItemInstance;
struct FGridMonsterSpawnInstance;
struct FGridWorldObjectInstance;

UCLASS(ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridEditorPreviewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGridEditorPreviewComponent();

	void Initialize(AGridLevelRuntimeActor* InRuntimeActor);

	void ClearPreviewObjects();
	void RebuildPreviewObjects();

	void SetHoveredObject(FGuid ObjectId);
	void SetSelectedObject(FGuid ObjectId);
	void CleanupOrphanPreviewObjects();

private:
	UPROPERTY(Transient)
	TObjectPtr<AGridLevelRuntimeActor> RuntimeActor;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGridEditorPreviewObjectActor>> SpawnedPreviewObjects;

	UPROPERTY(Transient)
	FGuid CurrentHoveredObjectId;

	UPROPERTY(Transient)
	FGuid CurrentSelectedObjectId;

private:
	bool IsPreviewableWorldObject(const FGridWorldObjectInstance& Instance) const;
	bool IsPreviewableLooseItem(const FGridLooseItemInstance& Instance) const;
	bool IsPreviewableMonsterSpawn(const FGridMonsterSpawnInstance& SpawnData) const;
	void AddWorldObjectPreview(const FGridWorldObjectInstance& Instance);
	void AddLooseItemPreview(const FGridLooseItemInstance& Instance);
	void AddMonsterPreviewObject(const FGridMonsterSpawnInstance& SpawnData);
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridCombatTargetingMarkers.generated.h"

class AGridLevelRuntimeActor;
class UInstancedStaticMeshComponent;
struct FGridCombatActionTargetingPreview;

UCLASS(Transient, NotPlaceable)
class AGridCombatTargetingMarkers : public AActor
{
	GENERATED_BODY()

public:
	AGridCombatTargetingMarkers();

	void Show(const AGridLevelRuntimeActor& Level, const FGridCombatActionTargetingPreview& Preview);
	void Clear();

private:
	enum EMarker : uint8
	{
		Center,
		Area,
		Invalid,
		MarkerCount
	};

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Markers[MarkerCount];

	TArray<FIntPoint> DisplayedCells;
	FIntPoint DisplayedTarget = FIntPoint::NoneValue;
	double DisplayedScale = 0.0;
	bool bDisplayedValid = false;
};

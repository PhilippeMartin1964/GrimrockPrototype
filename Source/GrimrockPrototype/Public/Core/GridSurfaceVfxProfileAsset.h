#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/GridVfxProfileTypes.h"
#include "GridSurfaceVfxProfileAsset.generated.h"

/** Reusable data-only presentation of persistent ground fire; no gameplay authority. */
UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridSurfaceVfxProfileAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ground Fire", meta=(ShowOnlyInnerProperties))
	FGridFireVfxSettings Settings;

	FGridFireVfxSettings GetSettings() const { return Settings.Sanitized(); }
};

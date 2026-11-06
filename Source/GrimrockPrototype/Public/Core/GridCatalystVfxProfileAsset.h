#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/GridVfxProfileTypes.h"
#include "GridCatalystVfxProfileAsset.generated.h"

/** Reusable data-only catalyst projectile and impact presentation. */
UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridCatalystVfxProfileAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Catalyst VFX", meta=(ShowOnlyInnerProperties))
	FGridCatalystVfxSettings Settings;

	FGridCatalystVfxSettings GetSettings() const { return Settings.Sanitized(); }
};

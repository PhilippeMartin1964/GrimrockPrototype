#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/GridLevelAsset.h"
#include "GridSurfaceVfxProfileAsset.generated.h"

/**
 * ARMEMENT01.17.4C reusable data-driven surface presentation.
 * Never stores authoritative RPG03.6 damage or duration.
 */
UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridSurfaceVfxProfileAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Niagara selection, user-parameter bindings, lights and catalyst visuals. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Surface VFX", meta=(ShowOnlyInnerProperties))
	FGridLevelSurfaceVfxSettings Settings;

	/** A selected profile always overrides old actor-level presentation defaults. */
	FGridLevelSurfaceVfxSettings GetRuntimeSettings() const
	{
		FGridLevelSurfaceVfxSettings Effective = Settings.Sanitized();
		Effective.bOverrideRuntimeVisuals = true;
		return Effective;
	}
};

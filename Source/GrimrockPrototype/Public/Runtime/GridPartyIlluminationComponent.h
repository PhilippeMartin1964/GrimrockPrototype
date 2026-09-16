#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "GridPartyIlluminationComponent.generated.h"

/**
 * PARTY-LIGHT01: ergonomic first-person illumination owned by the party.
 * A source (equipment today, magic later) provides an FGridLightEmitterConfig;
 * the party keeps only the point-light presentation and owns its placement.
 */
UCLASS(ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridPartyIlluminationComponent : public UGridLightEmitterComponent
{
	GENERATED_BODY()

public:
	UGridPartyIlluminationComponent();

	/** Global ergonomic tuning applied after copying the source light definition. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party Illumination", meta = (ClampMin = "0.0"))
	float IntensityMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party Illumination", meta = (ClampMin = "0.0"))
	float RadiusMultiplier = 1.0f;

	/** First-person party light defaults to shadowless to avoid giant held-item shadows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party Illumination")
	bool bCastShadows = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Party Illumination")
	FName ActiveSourceId = NAME_None;

	UFUNCTION(BlueprintCallable, Category = "Party Illumination")
	void ApplyIlluminationSource(const FGridLightEmitterConfig& SourceConfig, FName SourceId);

	UFUNCTION(BlueprintCallable, Category = "Party Illumination")
	void ClearIlluminationSource();

	UFUNCTION(BlueprintPure, Category = "Party Illumination")
	bool HasActiveIlluminationSource() const { return !ActiveSourceId.IsNone() && IsLightEnabled(); }

protected:
	virtual void BeginPlay() override;

private:
	FGridLightEmitterConfig BuildPartyConfig(const FGridLightEmitterConfig& SourceConfig) const;
};

#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "GridPartyIlluminationComponent.generated.h"

class AGridLevelRuntimeActor;
class UGridPartyInventoryComponent;

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

	/** PARTY-LIGHT03: party illumination casts real-time dungeon shadows by default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party Illumination")
	bool bCastShadows = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Party Illumination")
	FName ActiveSourceId = NAME_None;

	UFUNCTION(BlueprintCallable, Category = "Party Illumination")
	void ApplyIlluminationSource(const FGridLightEmitterConfig& SourceConfig, FName SourceId);

	UFUNCTION(BlueprintCallable, Category = "Party Illumination")
	void ClearIlluminationSource();

	/** Recomputes the party-wide equipment light without touching the selected held visual. */
	void RefreshFromEquipment(UGridPartyInventoryComponent* Inventory, AGridLevelRuntimeActor* LevelRuntimeActor);

	UFUNCTION(BlueprintPure, Category = "Party Illumination")
	bool HasActiveIlluminationSource() const { return !ActiveSourceId.IsNone() && IsLightEnabled(); }

protected:
	virtual void BeginPlay() override;

private:
	FGridLightEmitterConfig BuildPartyConfig(const FGridLightEmitterConfig& SourceConfig) const;
};

#pragma once

#include "CoreMinimal.h"
#include "GridVfxProfileTypes.generated.h"

class UNiagaraSystem;
class USoundBase;
class UMaterialInterface;
class UStaticMesh;

/** A per-component Niagara user override. The type must match the actual
 * parameter type exposed by the selected Niagara system.
 */
UENUM(BlueprintType)
enum class EGridNiagaraOverrideValueType : uint8
{
	Float,
	Integer,
	Boolean,
	Vector3,
	Color
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridNiagaraUserOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Niagara")
	FName ParameterName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Niagara")
	EGridNiagaraOverrideValueType ValueType = EGridNiagaraOverrideValueType::Float;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Niagara")
	float FloatValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Niagara")
	int32 IntValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Niagara")
	bool bBoolValue = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Niagara")
	FVector VectorValue = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Niagara")
	FLinearColor ColorValue = FLinearColor::White;
};

/** One reusable Niagara ground-fire presentation; not a level or gameplay component. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridFireVfxSettings
{
	GENERATED_BODY()

	/** Explicitly selected Niagara system. None means no ground-fire Niagara. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Niagara")
	TSoftObjectPtr<UNiagaraSystem> GroundFireSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Niagara", meta=(ClampMin="0.01", ClampMax="5.0"))
	float GroundFireScale = 1.f;

	/** Width of the visual effect relative to a cell. Applied to Niagara XYZ transform WITHOUT User Parameters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Niagara", meta=(ClampMin="0.1", ClampMax="1.0"))
	float GroundFireCoverage = .9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Niagara", meta=(ClampMin="5.0", ClampMax="300.0"))
	float FlameHeightCm = 75.f;

	/** Calibration: approximate horizontal extent of the unscaled Niagara at GroundFireScale=1 (in cm).
	 * E.g. 180 cm for a system authored for a nearly full 200 cm tile.
	 * This is a reference, not a vendor-specific User parameter.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Calibration", meta=(ClampMin="10.0", ClampMax="2000.0"))
	float NiagaraReferenceWidthCm = 180.f;

	/** Calibration: apparent flame height of the unscaled Niagara (in cm).
	 * Set this once after observing the vendor effect in PIE. FlameHeightCm is then a reliable Z-scale control.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Calibration", meta=(ClampMin="5.0", ClampMax="2000.0"))
	float NiagaraReferenceHeightCm = 75.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Niagara", meta=(ClampMin="0.0", ClampMax="4.0"))
	float FlameDensity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Niagara")
	bool bEnableSmoke = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Niagara", meta=(ClampMin="0.0", ClampMax="4.0"))
	float SmokeDensity = .5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Niagara")
	FLinearColor FireTint = FLinearColor(1.f, .43f, .08f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Budget", meta=(ClampMin="0", ClampMax="128"))
	int32 MaxFireCells = 48;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Budget", meta=(ClampMin="0", ClampMax="32"))
	int32 MaxFireLights = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Light", meta=(ClampMin="0.0", ClampMax="50000.0"))
	float FireLightIntensity = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Light", meta=(ClampMin="0.1", ClampMax="5.0"))
	float FireLightRadiusCells = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Light")
	FLinearColor FireLightColor = FLinearColor(1.f, .3f, .06f);

	/** FIREFX01: authored looping spatial sound, never a hardcoded vendor path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Audio")
	TSoftObjectPtr<USoundBase> FireCrackleLoopSound;

	/** At most one voice per contiguous fire region. Zero disables fire audio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Audio", meta=(ClampMin="0", ClampMax="8"))
	int32 MaxFireCrackleSources = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Audio", meta=(ClampMin="0.0", ClampMax="2.0"))
	float FireCrackleVolume = .65f;

	/** Recognized FireFX User parameters (SpawnBox, FlameRate, SmokeRate, Tint)
	 * can be bound automatically by exact name + Niagara type. No package path is hardcoded.
	 * The native spawn/rate defaults are preserved and reused as references.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Niagara User Parameters")
	bool bAutoBindKnownFireControls = true;

	/** FIREFX09: per-profile, explicitly authored overrides applied LAST.
	 * Supports all matching Niagara User Float/Int/Bool/Vector3/Color values,
	 * including vendor-specific controls not represented by dedicated fields.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Niagara User Parameters",
		meta=(TitleProperty="ParameterName"))
	TArray<FGridNiagaraUserOverride> UserParameterOverrides;

	/** Optional names of actual Niagara User Parameters (including User. prefix). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Niagara User Parameters")
	FName SpawnAreaParameter = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Niagara User Parameters")
	FName FlameHeightParameter = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Niagara User Parameters")
	FName FlameDensityParameter = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Niagara User Parameters")
	FName SmokeDensityParameter = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fire|Niagara User Parameters")
	FName FireTintParameter = NAME_None;

	/** Bounds user-authored values, without modifying the asset. */
	FGridFireVfxSettings Sanitized() const
	{
		FGridFireVfxSettings Result = *this;
		const auto Safe = [](float Value, float Fallback, float Min, float Max)
		{
			return FMath::Clamp(FMath::IsFinite(Value) ? Value : Fallback, Min, Max);
		};
		Result.GroundFireScale = Safe(GroundFireScale, 1.f, .01f, 5.f);
		Result.GroundFireCoverage = Safe(GroundFireCoverage, .9f, .1f, 1.f);
		Result.FlameHeightCm = Safe(FlameHeightCm, 75.f, 5.f, 300.f);
		Result.NiagaraReferenceWidthCm = Safe(NiagaraReferenceWidthCm, 180.f, 10.f, 2000.f);
		Result.NiagaraReferenceHeightCm = Safe(NiagaraReferenceHeightCm, 75.f, 5.f, 2000.f);
		Result.FlameDensity = Safe(FlameDensity, 1.f, 0.f, 4.f);
		Result.SmokeDensity = Safe(SmokeDensity, .5f, 0.f, 4.f);
		Result.MaxFireCells = FMath::Clamp(MaxFireCells, 0, 128);
		Result.MaxFireLights = FMath::Clamp(MaxFireLights, 0, 32);
		Result.FireLightIntensity = Safe(FireLightIntensity, 650.f, 0.f, 50000.f);
		Result.FireLightRadiusCells = Safe(FireLightRadiusCells, 1.2f, .1f, 5.f);
		Result.MaxFireCrackleSources = FMath::Clamp(MaxFireCrackleSources, 0, 8);
		Result.FireCrackleVolume = Safe(FireCrackleVolume, .65f, 0.f, 2.f);
		Result.FireTint = FireTint.GetClamped(0.f, 8.f);
		Result.FireLightColor = FireLightColor.GetClamped(0.f, 8.f);
		return Result;
	}
};

/** Catalyst's distinct transient projectile, ring and spark presentation. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCatalystVfxSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Catalyst|Assets")
	TSoftObjectPtr<UStaticMesh> ProjectileMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Catalyst|Assets")
	TSoftObjectPtr<UMaterialInterface> RunePulseMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Catalyst|Assets")
	TSoftObjectPtr<UMaterialInterface> EmberMaterial;

	/** Optional mesh used for transient sparks; omit for no spark geometry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Catalyst|Assets")
	TSoftObjectPtr<UStaticMesh> SparkMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Catalyst|Animation", meta=(ClampMin="0.1", ClampMax="1.5"))
	float FlightDurationSeconds = .42f;

	/** FLASK01: physical launch speed; flight duration comes from gravity and target distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Catalyst|Ballistics", meta=(ClampMin="100.0", ClampMax="5000.0"))
	float LaunchSpeedCmPerSec = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Catalyst|Animation", meta=(ClampMin="0.3", ClampMax="2.0"))
	float PulseDurationSeconds = .95f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Catalyst|Animation", meta=(ClampMin="0.0", ClampMax="0.3"))
	float WaveDelayPerCellSeconds = .085f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Catalyst|Animation", meta=(ClampMin="0.0", ClampMax="300.0"))
	float FlightArcHeightCm = 82.f;

	FGridCatalystVfxSettings Sanitized() const
	{
		FGridCatalystVfxSettings Result = *this;
		const auto Safe = [](float Value, float Fallback, float Min, float Max)
		{
			return FMath::Clamp(FMath::IsFinite(Value) ? Value : Fallback, Min, Max);
		};
		Result.FlightDurationSeconds = Safe(FlightDurationSeconds, .42f, .1f, 1.5f);
		Result.LaunchSpeedCmPerSec = Safe(LaunchSpeedCmPerSec, 900.f, 100.f, 5000.f);
		Result.PulseDurationSeconds = Safe(PulseDurationSeconds, .95f, .3f, 2.f);
		Result.WaveDelayPerCellSeconds = Safe(WaveDelayPerCellSeconds, .085f, 0.f, .3f);
		Result.FlightArcHeightCm = Safe(FlightArcHeightCm, 82.f, 0.f, 300.f);
		return Result;
	}
};

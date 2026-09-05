#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridTypes.h"
#include "GridObjectAudio.h"
#include "GridObjectBehavior.h"
#include "GridWorldObjectVisual.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "GridObjectArchetypeAsset.generated.h"

class AGridRuntimeObjectActor;
class AGridItemActor;
class USoundBase;
class USoundAttenuation;
struct FPropertyChangedEvent;

UENUM(BlueprintType)
enum class EGridArchetypeValidationSeverity : uint8
{
	Info UMETA(DisplayName = "Info"),
	Warning UMETA(DisplayName = "Warning"),
	Error UMETA(DisplayName = "Error")
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridSurfaceLocalPosition
{
	GENERATED_BODY()

	/** Horizontal tangent coordinate on the selected placement surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (DisplayName = "U"))
	float U = 0.0f;

	/**
	 * Second tangent coordinate on the selected surface.
	 * On Wall this is vertical height. On Floor/Ceiling this is the second in-plane coordinate.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (DisplayName = "V"))
	float V = 0.0f;

	/**
	 * Normal coordinate to the selected surface.
	 * Floor: height above floor. Wall: inset into the cell. Ceiling: distance below ceiling.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (DisplayName = "N"))
	float N = 0.0f;

	bool IsFinite() const
	{
		return FMath::IsFinite(U) && FMath::IsFinite(V) && FMath::IsFinite(N);
	}
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridArchetypeValidationMessage
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Validation")
	EGridArchetypeValidationSeverity Severity = EGridArchetypeValidationSeverity::Info;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Validation")
	FString Message;

	FGridArchetypeValidationMessage() = default;

	FGridArchetypeValidationMessage(EGridArchetypeValidationSeverity InSeverity, const FString& InMessage)
		: Severity(InSeverity)
		, Message(InMessage)
	{
	}
};

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridObjectArchetypeAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype")
	FName ArchetypeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype",
		meta = (DisplayName = "Gameplay Type",
			ToolTip = "Gameplay behavior family supported by this archetype. Concrete variants stay in ArchetypeId and DisplayName."))
	EGridLevelObjectType SupportedType = EGridLevelObjectType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
	bool bDefaultInitiallyEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
	bool bDefaultInitiallyActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults")
	FName DefaultTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults",
		meta = (ToolTip = "Default behavior copied to placed object instances. Currently contains teleporter, receptacle and mechanism parameters."))
	FGridObjectBehaviorParams DefaultBehavior;

	/** Single 3D attenuation used by every audio event emitted by this object archetype. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio",
		meta = (DisplayName = "Attenuation",
			ToolTip = "Single spatial attenuation used by all audio events of this archetype."))
	TObjectPtr<USoundAttenuation> DefaultAudioAttenuation = nullptr;

	/** Data-driven semantic audio events such as Open, Close, Press, Activate or Interact. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio",
		meta = (DisplayName = "Audio Events",
			ToolTip = "Data-driven audio events for this archetype. Keys are semantic names such as Open, Close, Press, Release or custom names."))
	TMap<FName, FGridObjectAudioEvent> AudioEvents;

	// Existing audio migration is intentionally untouched by WORLDOBJ-MIG03.
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use AudioEvents[Open].Sounds."))
	TArray<TObjectPtr<USoundBase>> DoorOpenSounds;

	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use AudioEvents[Close].Sounds."))
	TArray<TObjectPtr<USoundBase>> DoorCloseSounds;

	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use AudioEvents event Volume."))
	float DoorAudioVolume = 1.0f;

	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use AudioEvents event PitchVariation."))
	float DoorAudioPitchVariation = 0.0f;

	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use the archetype Audio > Attenuation field."))
	TObjectPtr<USoundAttenuation> DoorAudioAttenuation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette",
		meta = (DisplayName = "Palette Category",
			ToolTip =
				"Editor palette grouping only. Does not affect gameplay. Examples: Doors, Mechanisms, Wall Decorations, Floor Decorations, Receptacles, Lights, Spawns."))
	FName Category = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype",
		meta = (DisplayName = "Functional Category",
			ToolTip =
				"Editor/validation functional category. Does not directly drive runtime gameplay. SupportedType remains the gameplay type and Category remains the palette grouping."))
	EGridObjectCategory ObjectCategory = EGridObjectCategory::Decoration;

	/**
	 * WORLDOBJ-MIG01 placement authority.
	 * Only Floor, Wall and Ceiling are valid authoring surfaces.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement",
		meta = (DisplayName = "Placement Surface",
			ToolTip = "Current source of truth for editor/runtime placement. Valid authoring surfaces are Floor, Wall and Ceiling.",
			ValidEnumValues = "Floor,Wall,Ceiling"))
	EGridObjectPlacementKind PlacementSurface = EGridObjectPlacementKind::Floor;

	/**
	 * Local coordinates relative to Placement Surface.
	 * Floor: U/V in the floor plane, N above floor.
	 * Wall: U along wall, V vertical, N inset into the cell.
	 * Ceiling: U/V in the ceiling plane, N below ceiling.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (DisplayName = "Default Local Position"))
	FGridSurfaceLocalPosition DefaultLocalPosition;

	/**
	 * WORLDOBJ-MIG02 spatial behavior contract.
	 * The authoring surface now exposes exactly three independent spatial semantics.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spatial Behavior",
		meta = (DisplayName = "Blocks Cell Movement",
			ToolTip = "Statically blocks grid traversal through this object's cell. Door open/closed passage blocking is handled by the door runtime state instead."))
	bool bBlocksMovement = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spatial Behavior|Boundary",
		meta = (DisplayName = "Occupies Boundary",
			EditCondition = "PlacementSurface == EGridObjectPlacementKind::Wall", EditConditionHides,
			ToolTip = "Owns the topological boundary between two adjacent cells. Wall-mounted decorations and buttons normally leave this disabled."))
	bool bOccupiesBoundary = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spatial Behavior|Boundary",
		meta = (DisplayName = "Suppress Base Wall",
			EditCondition = "PlacementSurface == EGridObjectPlacementKind::Wall && bOccupiesBoundary", EditConditionHides,
			ToolTip = "Suppresses the generated structural wall mesh on the occupied boundary. This is visual/construction behavior and is independent of runtime passage state."))
	bool bReplacesStandardWall = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering|Cell Override",
		meta = (DisplayName = "Hide Cell Floor",
			ToolTip = "Prevents the standard floor mesh from being generated for the cell containing this object. This does not change cell walkability."))
	bool bHideCellFloor = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction",
		meta = (DisplayName = "Runtime Interactable",
			ToolTip = "Controls whether the runtime object can respond to direct player interaction when the runtime actor path supports it."))
	bool bIsInteractable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction",
		meta = (DisplayName = "Runtime Readable",
			ToolTip = "Controls whether the object behaves as readable at runtime. Functional Category = Readable is only classification."))
	bool bIsReadable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (MultiLine = "true", EditCondition = "bIsReadable", EditConditionHides))
	FText ReadableText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (EditCondition = "bIsReadable", EditConditionHides))
	bool bShowReadableOnlyOnce = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light",
		meta = (DisplayName = "Runtime Light Source",
			ToolTip =
				"Controls whether the object creates/configures a runtime light. Gameplay Type or Functional Category may also classify the object as Light."))
	bool bIsLightSource = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light", meta = (EditCondition = "bIsLightSource", EditConditionHides))
	FLinearColor LightColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light", meta = (ClampMin = "0.0", EditCondition = "bIsLightSource", EditConditionHides))
	float LightIntensity = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light", meta = (ClampMin = "0.0", EditCondition = "bIsLightSource", EditConditionHides))
	float LightRadius = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Light",
		meta = (DisplayName = "Use Light Flicker (if supported)", EditCondition = "bIsLightSource", EditConditionHides,
			ToolTip = "Currently displayed/configured at archetype level; actual flicker support depends on the runtime light component path."))
	bool bUseLightFlicker = false;

	/**
	 * WORLDOBJ-MIG03 target visual authoring contract.
	 * StaticPart is optional. MovingParts contains exactly two optional slots, therefore an object can have 0, 1 or 2 moving parts and never a third.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Composition", meta = (DisplayName = "Static Part"))
	FGridWorldObjectStaticPart StaticPart;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Composition", meta = (DisplayName = "Moving Parts"))
	FGridWorldObjectMovingParts MovingParts;

	/**
	 * WORLDOBJ-MIG03.4C compile-only bridge for the few C++ consumers not yet deleted.
	 * These names are deliberately Transient and non-editable: they are no longer authoring data,
	 * are never serialized, and are ignored by the target runtime presentation path.
	 * WORLDOBJ-MIG03.4D removes the symbols completely after the remaining callers are migrated.
	 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Visual|Legacy Runtime Bridge")
	TObjectPtr<UStaticMesh> PreviewMesh = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Visual|Legacy Runtime Bridge")
	TObjectPtr<UStaticMesh> FixedMesh = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Visual|Legacy Runtime Bridge")
	TObjectPtr<UStaticMesh> MovingMesh = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Visual|Legacy Runtime Bridge")
	TObjectPtr<UStaticMesh> PitLeftLeafMesh = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Visual|Legacy Runtime Bridge")
	TObjectPtr<UStaticMesh> PitRightLeafMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime",
		meta = (DisplayName = "Runtime Actor Class",
			ToolTip =
				"Runtime actor class used to spawn this archetype. Gameplay Type defines what the object is; Runtime Actor Class defines how it is instantiated."))
	TSubclassOf<AGridRuntimeObjectActor> RuntimeActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime",
		meta = (DisplayName = "Item Actor Class", ToolTip = "Runtime item actor class used when this archetype represents a spawned or carried item."))
	TSubclassOf<AGridItemActor> ItemActorClass;

	/**
	 * Internal implementation bridges only. They are not authoring parameters and are never serialized.
	 * Placement bridges remain until the current transform consumers are collapsed onto PlacementSurface/U/V/N.
	 * Sharing bridges remain only so untouched editor code compiles during WORLDOBJ-MIG02; target sharing is permissive by default.
	 */
	UPROPERTY(Transient)
	EGridObjectPlacementKind PlacementKind = EGridObjectPlacementKind::Floor;

	UPROPERTY(Transient)
	float PlacementZOffset = 0.0f;

	UPROPERTY(Transient)
	float WallInset = 0.0f;

	UPROPERTY(Transient)
	float LocalOffsetAlongWall = 0.0f;

	UPROPERTY(Transient)
	float LocalOffsetVertical = 0.0f;

	UPROPERTY(Transient)
	bool bCanShareCell = true;

	UPROPERTY(Transient)
	bool bCanShareAnchor = true;

	bool HasValidPlacementSurface() const
	{
		return PlacementSurface == EGridObjectPlacementKind::Floor || PlacementSurface == EGridObjectPlacementKind::Wall ||
			PlacementSurface == EGridObjectPlacementKind::Ceiling;
	}

	bool IsEdgePlaced() const
	{
		return PlacementKind == EGridObjectPlacementKind::Wall;
	}

	bool IsCenterPlaced() const
	{
		return PlacementKind == EGridObjectPlacementKind::Floor || PlacementKind == EGridObjectPlacementKind::Ceiling;
	}

	bool IsWallPlaced() const
	{
		return PlacementKind == EGridObjectPlacementKind::Wall;
	}

	bool IsCeilingPlaced() const
	{
		return PlacementKind == EGridObjectPlacementKind::Ceiling;
	}

	bool BlocksCellMovement() const
	{
		return bBlocksMovement;
	}

	bool OccupiesBoundary() const
	{
		return bOccupiesBoundary;
	}

	bool SuppressesBaseWall() const
	{
		return bReplacesStandardWall;
	}

	bool IsReadable() const
	{
		return bIsReadable;
	}

	bool IsLightSource() const
	{
		return bIsLightSource;
	}

	int32 GetDefinedMovingPartCount() const
	{
		return MovingParts.NumDefined();
	}

	/** Recomputes the non-serialized projection consumed by current transform call sites. */
	void RefreshPlacementRuntimeProjection();

	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Resolves a generic event, including the pre-existing audio migration path. */
	bool ResolveAudioEvent(FName EventName, FGridObjectAudioEvent& OutEvent) const;

	bool ValidateArchetype(TArray<FGridArchetypeValidationMessage>& OutMessages) const;
	bool IsValidArchetype() const;
	FString GetValidationSummary() const;

	// Existing helper API retained for callers; semantics now resolve against Floor/Wall/Ceiling only.
	bool RequiresEdgePlacement() const;
	bool SupportsCenterPlacement() const;
	bool SupportsWallPlacement() const;
	bool RequiresRuntimeActorClass() const;
	bool AllowsInvisibleRuntimeObject() const;
	bool UsesWallPlacementParams() const;
	bool UsesCenterPlacementParams() const;
	bool UsesReadableParams() const;
	bool UsesLightParams() const;
	bool UsesItemParams() const;
	bool UsesReceptacleParams() const;
	bool UsesTeleporterParams() const;
	bool UsesButtonAnimationParams() const;
	bool UsesTriggerParams() const;
	bool UsesMovingMeshParams() const;
	bool UsesFixedMeshParams() const;
	bool UsesRuntimeActorClass() const;

	bool HasCompletePitTrapdoorCover() const
	{
		return PitLeftLeafMesh != nullptr && PitRightLeafMesh != nullptr;
	}
};
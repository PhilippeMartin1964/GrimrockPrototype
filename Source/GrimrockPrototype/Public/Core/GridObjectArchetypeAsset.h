#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridTypes.h"
#include "GridObjectAudio.h"
#include "GridObjectBehavior.h"
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

	// Existing audio migration is intentionally untouched by WORLDOBJ-MIG01.
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
	 * Center and Edge remain enum symbols temporarily because the shared enum is used outside world-object definitions,
	 * but they are not valid WorldObject definition values and validation rejects them.
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement",
		meta = (ToolTip = "Editor placement rule: allows this object to share a cell with other objects. WORLDOBJ-MIG02 will replace this legacy rule."))
	bool bCanShareCell = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement",
		meta = (ToolTip = "Editor placement rule: allows this object to share the same edge/anchor with another object. WORLDOBJ-MIG02 will replace this legacy rule."))
	bool bCanShareAnchor = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement|Wall",
		meta = (DisplayName = "Replaces Standard Wall",
			EditCondition = "PlacementSurface == EGridObjectPlacementKind::Wall", EditConditionHides,
			ToolTip = "Hides the standard wall mesh on this object's solid wall boundary. WORLDOBJ-MIG02 will rename this to SuppressBaseWall."))
	bool bReplacesStandardWall = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement",
		meta = (DisplayName = "Blocks Movement (Generic Object)",
			ToolTip = "Door passage blocking is handled by the door system. WORLDOBJ-MIG02 will rename this to BlocksCellMovement."))
	bool bBlocksMovement = false;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual",
		meta = (DisplayName = "Main Mesh / Preview Mesh",
			ToolTip = "Primary mesh used for simple visible objects and editor preview. WORLDOBJ-MIG03 will replace this visual contract."))
	TObjectPtr<UStaticMesh> PreviewMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Visual",
		meta = (DisplayName = "Fixed Mesh",
			ToolTip = "Static part of a composite object. WORLDOBJ-MIG03 will replace this visual contract."))
	TObjectPtr<UStaticMesh> FixedMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Visual",
		meta = (DisplayName = "Moving Mesh",
			EditCondition = "SupportedType != EGridLevelObjectType::Pit", EditConditionHides,
			ToolTip = "Animated part of a composite object. WORLDOBJ-MIG03 will replace this visual contract."))
	TObjectPtr<UStaticMesh> MovingMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Pit Trapdoor",
		meta = (DisplayName = "Left Leaf Mesh",
			EditCondition = "SupportedType == EGridLevelObjectType::Pit", EditConditionHides,
			ToolTip = "Left trapdoor leaf. WORLDOBJ-MIG03 will replace this specialized field."))
	TObjectPtr<UStaticMesh> PitLeftLeafMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Pit Trapdoor",
		meta = (DisplayName = "Right Leaf Mesh",
			EditCondition = "SupportedType == EGridLevelObjectType::Pit", EditConditionHides,
			ToolTip = "Right trapdoor leaf. WORLDOBJ-MIG03 will replace this specialized field."))
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
	 * Internal WORLDOBJ-MIG01 projection used only by current runtime/editor transform call sites.
	 * These transient fields are reflected only so existing C++/diagnostic call sites keep compiling;
	 * they are not editable, are never serialized, and provide no backward-compatible DataAsset path.
	 * Direct call sites will be collapsed onto PlacementSurface/DefaultLocalPosition in a later cleanup.
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

	bool IsReadable() const
	{
		return bIsReadable;
	}

	bool IsLightSource() const
	{
		return bIsLightSource;
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

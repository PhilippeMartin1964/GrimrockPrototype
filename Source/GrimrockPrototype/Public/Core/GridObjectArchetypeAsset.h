#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridTypes.h"
#include "GridObjectBehavior.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "GridObjectArchetypeAsset.generated.h"

class AGridRuntimeObjectActor;
class AGridItemActor;

UENUM(BlueprintType)
enum class EGridArchetypeValidationSeverity : uint8
{
	Info UMETA(DisplayName = "Info"),
	Warning UMETA(DisplayName = "Warning"),
	Error UMETA(DisplayName = "Error")
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
		meta = (ToolTip = "Default behavior copied to placed object instances. Currently contains teleporter, receptacle and button parameters."))
	FGridObjectBehaviorParams DefaultBehavior;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement",
		meta = (DisplayName = "Placement Kind", ToolTip = "Current source of truth for editor/runtime placement."))
	EGridObjectPlacementKind PlacementKind = EGridObjectPlacementKind::Center;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement",
		meta = (ToolTip = "Editor placement rule: allows this object to share a cell with other objects."))
	bool bCanShareCell = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement",
		meta = (ToolTip = "Editor placement rule: allows this object to share the same edge/anchor with another object."))
	bool bCanShareAnchor = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement|Wall",
		meta = (DisplayName = "Replaces Standard Wall",
			EditCondition = "PlacementKind == EGridObjectPlacementKind::Wall || PlacementKind == EGridObjectPlacementKind::Edge", EditConditionHides,
			ToolTip = "Hides the standard wall mesh on this object's solid wall edge."))
	bool bReplacesStandardWall = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement",
		meta = (DisplayName = "Blocks Movement (Generic Object)",
			ToolTip = "Door passage blocking is handled by the door system. This flag is mainly for generic non-door objects."))
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
			ToolTip = "Primary mesh used for simple visible objects and editor preview. For composite/animated objects, use FixedMesh and/or MovingMesh."))
	TObjectPtr<UStaticMesh> PreviewMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual",
		meta = (DisplayName = "Main Material / Preview Material",
			ToolTip =
				"Primary material used for simple visible objects and editor preview. For composite/animated objects, use FixedMaterial and/or MovingMaterial."))
	TObjectPtr<UMaterialInterface> PreviewMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Visual",
		meta = (DisplayName = "Fixed Mesh",
			ToolTip = "Fixed Mesh - static part of a composite object, such as secret doors, doors, buttons, levers or receptacles with visible item parts."))
	TObjectPtr<UStaticMesh> FixedMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Visual",
		meta = (DisplayName = "Moving Mesh",
			ToolTip = "Moving Mesh - animated or movable part of a composite object, such as doors, buttons, levers or receptacles with visible item parts."))
	TObjectPtr<UStaticMesh> MovingMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Visual",
		meta = (DisplayName = "Fixed Material", ToolTip = "Material for the fixed/static part of a composite object."))
	TObjectPtr<UMaterialInterface> FixedMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = "Visual",
		meta = (DisplayName = "Moving Material", ToolTip = "Material for the moving/animated part of a composite object."))
	TObjectPtr<UMaterialInterface> MovingMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime",
		meta = (DisplayName = "Runtime Actor Class",
			ToolTip =
				"Runtime actor class used to spawn this archetype. Gameplay Type defines what the object is; Runtime Actor Class defines how it is instantiated."))
	TSubclassOf<AGridRuntimeObjectActor> RuntimeActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Runtime",
		meta = (DisplayName = "Item Actor Class", ToolTip = "Runtime item actor class used when this archetype represents a spawned or carried item."))
	TSubclassOf<AGridItemActor> ItemActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0.0"))
	float PlacementZOffset = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement|Wall",
		meta = (ClampMin = "0.0", EditCondition = "PlacementKind == EGridObjectPlacementKind::Wall || PlacementKind == EGridObjectPlacementKind::Edge",
			EditConditionHides, ToolTip = "Used only when PlacementKind is Wall or Edge."))
	float WallInset = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement|Wall",
		meta = (EditCondition = "PlacementKind == EGridObjectPlacementKind::Wall || PlacementKind == EGridObjectPlacementKind::Edge", EditConditionHides,
			ToolTip = "Used only when PlacementKind is Wall or Edge."))
	float LocalOffsetAlongWall = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement|Wall",
		meta = (EditCondition = "PlacementKind == EGridObjectPlacementKind::Wall || PlacementKind == EGridObjectPlacementKind::Edge", EditConditionHides,
			ToolTip = "Used only when PlacementKind is Wall or Edge."))
	float LocalOffsetVertical = 0.f;

	bool IsEdgePlaced() const
	{
		return PlacementKind == EGridObjectPlacementKind::Edge || PlacementKind == EGridObjectPlacementKind::Wall;
	}

	bool IsCenterPlaced() const
	{
		return PlacementKind == EGridObjectPlacementKind::Center || PlacementKind == EGridObjectPlacementKind::Floor ||
			PlacementKind == EGridObjectPlacementKind::Ceiling;
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

	bool ValidateArchetype(TArray<FGridArchetypeValidationMessage>& OutMessages) const;
	bool IsValidArchetype() const;
	FString GetValidationSummary() const;
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
};

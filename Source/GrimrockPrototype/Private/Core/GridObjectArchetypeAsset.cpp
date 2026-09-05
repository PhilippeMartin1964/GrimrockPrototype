#include "Core/GridObjectArchetypeAsset.h"

#include "Runtime/GridDoorActor.h"
#include "Runtime/GridPitTrapdoorActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridWallLockActor.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

namespace
{
	constexpr float CurrentCeilingPlaneHeight = 200.0f;

	const TCHAR* ToValidationSeverityText(EGridArchetypeValidationSeverity Severity)
	{
		switch (Severity)
		{
			case EGridArchetypeValidationSeverity::Info:
				return TEXT("Info");
			case EGridArchetypeValidationSeverity::Warning:
				return TEXT("Warning");
			case EGridArchetypeValidationSeverity::Error:
				return TEXT("Error");
			default:
				return TEXT("Unknown");
		}
	}

	void AddValidationMessage(TArray<FGridArchetypeValidationMessage>& Messages, EGridArchetypeValidationSeverity Severity, const TCHAR* Message)
	{
		Messages.Emplace(Severity, FString(Message));
	}

	bool HasAnyMesh(const UGridObjectArchetypeAsset& Archetype)
	{
		return Archetype.PreviewMesh || Archetype.FixedMesh || Archetype.MovingMesh;
	}

	bool HasPreviewOrMovingMesh(const UGridObjectArchetypeAsset& Archetype)
	{
		return Archetype.PreviewMesh || Archetype.MovingMesh;
	}

	bool IsFloorPlacement(EGridObjectPlacementKind PlacementKind)
	{
		return PlacementKind == EGridObjectPlacementKind::Floor;
	}

	bool IsWallPlacement(EGridObjectPlacementKind PlacementKind)
	{
		return PlacementKind == EGridObjectPlacementKind::Wall;
	}

	const TCHAR* ToSupportedTypeText(EGridLevelObjectType SupportedType)
	{
		switch (SupportedType)
		{
			case EGridLevelObjectType::Door:
				return TEXT("Door");
			case EGridLevelObjectType::Button:
				return TEXT("Button");
			case EGridLevelObjectType::PressurePlate:
				return TEXT("PressurePlate");
			case EGridLevelObjectType::Lever:
				return TEXT("Lever");
			case EGridLevelObjectType::Decoration:
				return TEXT("Decoration");
			case EGridLevelObjectType::MonsterSpawn:
				return TEXT("MonsterSpawn");
			case EGridLevelObjectType::ItemSpawn:
				return TEXT("ItemSpawn");
			case EGridLevelObjectType::Light:
				return TEXT("Light");
			case EGridLevelObjectType::Teleporter:
				return TEXT("Teleporter");
			case EGridLevelObjectType::Trigger:
				return TEXT("Trigger");
			case EGridLevelObjectType::Receptacle:
				return TEXT("Receptacle");
			case EGridLevelObjectType::Item:
				return TEXT("Item");
			case EGridLevelObjectType::Logic:
				return TEXT("Logic");
			case EGridLevelObjectType::StoryCompanion:
				return TEXT("StoryCompanion");
			case EGridLevelObjectType::CustomRecruiter:
				return TEXT("CustomRecruiter");
			case EGridLevelObjectType::Pit:
				return TEXT("Pit");
			case EGridLevelObjectType::None:
			default:
				return TEXT("None");
		}
	}

	const TCHAR* ToObjectCategoryText(EGridObjectCategory Category)
	{
		switch (Category)
		{
			case EGridObjectCategory::Mechanism:
				return TEXT("Mechanism");
			case EGridObjectCategory::Decoration:
				return TEXT("Decoration");
			case EGridObjectCategory::Prop:
				return TEXT("Prop");
			case EGridObjectCategory::Receptacle:
				return TEXT("Receptacle");
			case EGridObjectCategory::Light:
				return TEXT("Light");
			case EGridObjectCategory::Readable:
				return TEXT("Readable");
			case EGridObjectCategory::Spawn:
				return TEXT("Spawn");
			case EGridObjectCategory::Teleporter:
				return TEXT("Teleporter");
			case EGridObjectCategory::Item:
				return TEXT("Item");
			default:
				return TEXT("Unknown");
		}
	}

	EGridObjectCategory GetRecommendedObjectCategory(EGridLevelObjectType SupportedType, bool bIsReadable)
	{
		switch (SupportedType)
		{
			case EGridLevelObjectType::Door:
			case EGridLevelObjectType::Button:
			case EGridLevelObjectType::PressurePlate:
			case EGridLevelObjectType::Lever:
			case EGridLevelObjectType::Trigger:
			case EGridLevelObjectType::Pit:
				return EGridObjectCategory::Mechanism;

			case EGridLevelObjectType::Receptacle:
				return EGridObjectCategory::Receptacle;

			case EGridLevelObjectType::Decoration:
				return bIsReadable ? EGridObjectCategory::Readable : EGridObjectCategory::Decoration;

			case EGridLevelObjectType::Light:
				return EGridObjectCategory::Light;

			case EGridLevelObjectType::Teleporter:
				return EGridObjectCategory::Teleporter;

			case EGridLevelObjectType::MonsterSpawn:
			case EGridLevelObjectType::ItemSpawn:
				return EGridObjectCategory::Spawn;

			case EGridLevelObjectType::Item:
				return EGridObjectCategory::Item;

			case EGridLevelObjectType::None:
			default:
				return EGridObjectCategory::Decoration;
		}
	}

	bool IsObjectCategoryCompatible(EGridLevelObjectType SupportedType, EGridObjectCategory ObjectCategory, bool bIsReadable)
	{
		if (SupportedType == EGridLevelObjectType::None || SupportedType == EGridLevelObjectType::Logic ||
			SupportedType == EGridLevelObjectType::StoryCompanion || SupportedType == EGridLevelObjectType::CustomRecruiter)
		{
			return true;
		}

		if (SupportedType == EGridLevelObjectType::Decoration)
		{
			if (bIsReadable)
			{
				return ObjectCategory == EGridObjectCategory::Readable;
			}

			return ObjectCategory == EGridObjectCategory::Decoration || ObjectCategory == EGridObjectCategory::Readable ||
				ObjectCategory == EGridObjectCategory::Prop;
		}

		return ObjectCategory == GetRecommendedObjectCategory(SupportedType, bIsReadable);
	}

	bool IsDefaultLightParams(const UGridObjectArchetypeAsset& Archetype)
	{
		return Archetype.LightColor.Equals(FLinearColor::White) && FMath::IsNearlyEqual(Archetype.LightIntensity, 500.f) &&
			FMath::IsNearlyEqual(Archetype.LightRadius, 500.f) && !Archetype.bUseLightFlicker;
	}

	bool HasReceptacleBehaviorParams(const FGridObjectBehaviorParams& Behavior)
	{
		return !Behavior.Receptacle.bAcceptAnyItem || Behavior.Receptacle.AcceptedItems.Num() > 0 || Behavior.Receptacle.InitialContent.Num() > 0 ||
			Behavior.Receptacle.MaxContainedItems != 1 || Behavior.Receptacle.VisualPlacementMode != EGridReceptacleVisualPlacementMode::AttachedSocket ||
			Behavior.Receptacle.bSimulatePhysicsWhenPlaced || !FMath::IsNearlyEqual(Behavior.Receptacle.PhysicalPlacementSurfaceOffset, 10.f) ||
			!Behavior.Receptacle.PhysicalPlacementInitialRotationOffset.IsNearlyZero();
	}

	bool IsWallLockArchetype(const UGridObjectArchetypeAsset& Archetype)
	{
		return (Archetype.RuntimeActorClass && Archetype.RuntimeActorClass->IsChildOf(AGridWallLockActor::StaticClass())) ||
			Archetype.DefaultBehavior.Lock.AcceptedKeyIds.Num() > 0 || Archetype.DefaultBehavior.Lock.AcceptedKeyItems.Num() > 0;
	}

	bool HasTeleporterBehaviorParams(const FGridObjectBehaviorParams& Behavior)
	{
		return Behavior.Teleporter.TargetCellX != INDEX_NONE || Behavior.Teleporter.TargetCellY != INDEX_NONE;
	}

	bool HasCustomButtonAnimationParams(const FGridObjectBehaviorParams& Behavior)
	{
		return !FMath::IsNearlyEqual(Behavior.ButtonAnimation.ButtonPressDistance, 6.f) ||
			!FMath::IsNearlyEqual(Behavior.ButtonAnimation.ButtonPressDuration, 0.08f) ||
			!FMath::IsNearlyEqual(Behavior.ButtonAnimation.ButtonReleaseDuration, 0.10f) ||
			!FMath::IsNearlyEqual(Behavior.ButtonAnimation.ButtonHoldTime, 0.15f);
	}

	bool IsPaletteCategory(const UGridObjectArchetypeAsset& Archetype, const TCHAR* ExpectedCategory)
	{
		return Archetype.Category == FName(ExpectedCategory);
	}

	bool IsExpectedConcreteReceptacleArchetype(FName ArchetypeId)
	{
		static const FName ReceptacleAlcoveId(TEXT("Receptacle_Alcove"));
		static const FName ReceptacleStoneAlcoveId(TEXT("Receptacle_Alcove_Stone"));
		static const FName ReceptacleTorchHolderId(TEXT("Receptacle_TorchHolder"));
		static const FName ReceptacleAltarId(TEXT("Receptacle_Altar"));
		static const FName ReceptacleOfferingBowlId(TEXT("Receptacle_OfferingBowl"));

		return ArchetypeId == ReceptacleAlcoveId || ArchetypeId == ReceptacleStoneAlcoveId || ArchetypeId == ReceptacleTorchHolderId ||
			ArchetypeId == ReceptacleAltarId || ArchetypeId == ReceptacleOfferingBowlId;
	}
}

void UGridObjectArchetypeAsset::RefreshPlacementRuntimeProjection()
{
	PlacementKind = PlacementSurface;
	LocalOffsetAlongWall = DefaultLocalPosition.U;
	LocalOffsetVertical = 0.0f;
	WallInset = DefaultLocalPosition.N;

	switch (PlacementSurface)
	{
		case EGridObjectPlacementKind::Wall:
			// The current wall transform consumes ZOffset + LocalOffsetVertical.
			// V is now the single vertical coordinate, therefore the secondary offset is always zero.
			PlacementZOffset = DefaultLocalPosition.V;
			break;

		case EGridObjectPlacementKind::Ceiling:
			// Current dungeon geometry places the ceiling plane at Z=200 cm.
			// Target N is measured downward from that surface.
			PlacementZOffset = CurrentCeilingPlaneHeight - DefaultLocalPosition.N;
			break;

		case EGridObjectPlacementKind::Floor:
			PlacementZOffset = DefaultLocalPosition.N;
			break;

		case EGridObjectPlacementKind::Center:
		case EGridObjectPlacementKind::Edge:
		default:
			// Center and Edge are intentionally invalid authoring values after MIG01.
			// Do not synthesize a compatibility transform from them.
			PlacementZOffset = 0.0f;
			WallInset = 0.0f;
			LocalOffsetAlongWall = 0.0f;
			LocalOffsetVertical = 0.0f;
			break;
	}
}

void UGridObjectArchetypeAsset::PostLoad()
{
	Super::PostLoad();

	// WORLDOBJ-MIG01 is a clean prototype cut: serialized PlacementZOffset/WallInset/
	// LocalOffsetAlongWall/LocalOffsetVertical no longer exist. Only the new local
	// position is projected into the current transform implementation.
	RefreshPlacementRuntimeProjection();

	if (SupportedType != EGridLevelObjectType::Door)
	{
		return;
	}

	// Pre-existing audio migration is unrelated to WORLDOBJ-MIG01 and remains unchanged.
	if (!DefaultAudioAttenuation && DoorAudioAttenuation)
	{
		DefaultAudioAttenuation = DoorAudioAttenuation;
	}

	auto MigrateLegacyEvent = [this](FName EventName, const TArray<TObjectPtr<USoundBase>>& LegacySounds)
	{
		if (AudioEvents.Contains(EventName) || LegacySounds.IsEmpty())
		{
			return;
		}

		FGridObjectAudioEvent Event;
		Event.Sounds = LegacySounds;
		Event.Volume = DoorAudioVolume;
		Event.PitchVariation = DoorAudioPitchVariation;
		AudioEvents.Add(EventName, MoveTemp(Event));
	};

	MigrateLegacyEvent(TEXT("Open"), DoorOpenSounds);
	MigrateLegacyEvent(TEXT("Close"), DoorCloseSounds);
}

#if WITH_EDITOR
void UGridObjectArchetypeAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshPlacementRuntimeProjection();
}
#endif

bool UGridObjectArchetypeAsset::ResolveAudioEvent(FName EventName, FGridObjectAudioEvent& OutEvent) const
{
	if (const FGridObjectAudioEvent* Event = AudioEvents.Find(EventName))
	{
		OutEvent = *Event;
		return true;
	}

	if (SupportedType == EGridLevelObjectType::Door)
	{
		const TArray<TObjectPtr<USoundBase>>* LegacySounds = nullptr;
		if (EventName == FName(TEXT("Open")))
		{
			LegacySounds = &DoorOpenSounds;
		}
		else if (EventName == FName(TEXT("Close")))
		{
			LegacySounds = &DoorCloseSounds;
		}

		if (LegacySounds && !LegacySounds->IsEmpty())
		{
			OutEvent.Sounds = *LegacySounds;
			OutEvent.Volume = DoorAudioVolume;
			OutEvent.PitchVariation = DoorAudioPitchVariation;
			return true;
		}
	}

	return false;
}

bool UGridObjectArchetypeAsset::ValidateArchetype(TArray<FGridArchetypeValidationMessage>& OutMessages) const
{
	OutMessages.Reset();

	if (ArchetypeId.IsNone())
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("ArchetypeId is not set."));
	}

	if (SupportedType == EGridLevelObjectType::None)
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("SupportedType must not be None."));
	}

	if (!HasValidPlacementSurface())
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error,
			TEXT("Placement Surface must be Floor, Wall or Ceiling. Center and Edge are no longer valid authoring values."));
	}

	if (!DefaultLocalPosition.IsFinite())
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error,
			TEXT("Default Local Position U/V/N must contain finite values."));
	}

	if (ArchetypeId == FName(TEXT("Door_Secret")) && SupportedType != EGridLevelObjectType::Door)
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error,
			TEXT("Door_Secret must use SupportedType=Door. Visual variants must stay archetypes, not EGridLevelObjectType values."));
	}

	if (IsExpectedConcreteReceptacleArchetype(ArchetypeId) && SupportedType != EGridLevelObjectType::Receptacle)
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error,
			TEXT("Concrete receptacle archetypes must use SupportedType=Receptacle. Visual variants must stay archetypes, not EGridLevelObjectType values."));
	}

	if ((ArchetypeId == FName(TEXT("Receptacle_Alcove")) || ArchetypeId == FName(TEXT("Receptacle_Alcove_Stone"))) &&
		DefaultBehavior.Receptacle.MaxContainedItems == 1)
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Receptacle alcoves must use MaxContainedItems > 1 or <= 0."));
	}

	if ((ArchetypeId == FName(TEXT("Receptacle_Alcove")) || ArchetypeId == FName(TEXT("Receptacle_Alcove_Stone"))) &&
		DefaultBehavior.Receptacle.VisualPlacementMode != EGridReceptacleVisualPlacementMode::PhysicalAtHit)
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Receptacle alcoves must use VisualPlacementMode=PhysicalAtHit."));
	}

	if (RequiresRuntimeActorClass() && !RuntimeActorClass)
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("RuntimeActorClass is required for this SupportedType."));
	}

	for (const TPair<FName, FGridObjectAudioEvent>& Pair : AudioEvents)
	{
		if (Pair.Key.IsNone())
		{
			AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Audio event key must not be None."));
		}
		if (!FMath::IsFinite(Pair.Value.Volume) || Pair.Value.Volume < 0.f)
		{
			AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error,
				*FString::Printf(TEXT("Audio event %s Volume must be finite and >= 0."), *Pair.Key.ToString()));
		}
		if (!FMath::IsFinite(Pair.Value.PitchVariation) || Pair.Value.PitchVariation < 0.f || Pair.Value.PitchVariation > 0.25f)
		{
			AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error,
				*FString::Printf(TEXT("Audio event %s PitchVariation must be finite and between 0.0 and 0.25."), *Pair.Key.ToString()));
		}
	}

	if (Category.IsNone())
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Info,
			TEXT("Palette Category is not set. This does not affect runtime, but the object may be harder to organize in the editor palette."));
	}

	if (bReplacesStandardWall && !IsWallPlacement(PlacementSurface))
	{
		AddValidationMessage(
			OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("Replaces Standard Wall is enabled but Placement Surface is not Wall."));
	}

	if (bReplacesStandardWall && bCanShareAnchor)
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Warning,
			TEXT("Replaces Standard Wall is enabled while bCanShareAnchor=true. Multiple wall replacements can overlap on the same boundary."));
	}

	if (!IsObjectCategoryCompatible(SupportedType, ObjectCategory, bIsReadable))
	{
		if (SupportedType == EGridLevelObjectType::Decoration && bIsReadable)
		{
			AddValidationMessage(
				OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("Readable Decoration should generally use ObjectCategory=Readable."));
		}
		else
		{
			const EGridObjectCategory RecommendedCategory = GetRecommendedObjectCategory(SupportedType, bIsReadable);
			OutMessages.Emplace(EGridArchetypeValidationSeverity::Warning,
				FString::Printf(TEXT("%s should generally use ObjectCategory=%s, but currently uses %s."), ToSupportedTypeText(SupportedType),
					ToObjectCategoryText(RecommendedCategory), ToObjectCategoryText(ObjectCategory)));
		}
	}

	if (!bIsReadable && !ReadableText.IsEmpty())
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("ReadableText is set but bIsReadable=false."));
	}

	if (!bIsReadable && bShowReadableOnlyOnce)
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Info, TEXT("bShowReadableOnlyOnce is enabled but bIsReadable=false."));
	}

	if (!UsesLightParams() && !bIsLightSource && !IsDefaultLightParams(*this))
	{
		AddValidationMessage(
			OutMessages, EGridArchetypeValidationSeverity::Info, TEXT("Light parameters are customized but this archetype is not a light source."));
	}

	if (bUseLightFlicker && !bIsLightSource)
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("Light flicker is enabled but bIsLightSource=false."));
	}

	if (!UsesReceptacleParams() && HasReceptacleBehaviorParams(DefaultBehavior))
	{
		AddValidationMessage(
			OutMessages, EGridArchetypeValidationSeverity::Info, TEXT("Receptacle behavior parameters are set but SupportedType is not Receptacle."));
	}

	if (!UsesTeleporterParams() && HasTeleporterBehaviorParams(DefaultBehavior))
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Info, TEXT("Teleporter target cell is set but SupportedType is not Teleporter."));
	}

	if (!UsesButtonAnimationParams() && HasCustomButtonAnimationParams(DefaultBehavior))
	{
		AddValidationMessage(
			OutMessages, EGridArchetypeValidationSeverity::Info, TEXT("Button animation parameters are customized but SupportedType is not Button."));
	}

	if (!UsesMovingMeshParams() && MovingMesh)
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Info,
			TEXT("MovingMesh is set but this archetype type does not normally use moving mesh."));
	}

	if (!IsWallLockArchetype(*this) && !UsesFixedMeshParams() && FixedMesh)
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Info,
			TEXT("FixedMesh is set but this archetype type does not normally use fixed mesh."));
	}

	switch (SupportedType)
	{
		case EGridLevelObjectType::Door:
		{
			if (!IsWallPlacement(PlacementSurface))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Door Placement Surface must be Wall."));
			}
			if (!HasPreviewOrMovingMesh(*this))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Door requires PreviewMesh or MovingMesh."));
			}
			if (RuntimeActorClass && !RuntimeActorClass->IsChildOf(AGridDoorActor::StaticClass()))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error,
					TEXT("Door RuntimeActorClass must derive from AGridDoorActor."));
			}
			if (bCanShareAnchor)
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("Door should generally have bCanShareAnchor set to false."));
			}
			break;
		}

		case EGridLevelObjectType::Button:
		case EGridLevelObjectType::Lever:
		{
			if (!IsWallPlacement(PlacementSurface))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Button and Lever Placement Surface must be Wall."));
			}
			if (!bIsInteractable)
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("Button and Lever should generally be interactable."));
			}
			if (!HasPreviewOrMovingMesh(*this))
			{
				AddValidationMessage(
					OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("Button and Lever should generally define PreviewMesh or MovingMesh."));
			}
			break;
		}

		case EGridLevelObjectType::Pit:
		{
			if (!IsFloorPlacement(PlacementSurface))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Pit Placement Surface must be Floor."));
			}
			if (!PreviewMesh)
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Pit requires a PreviewMesh."));
			}
			if (bBlocksMovement)
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Pit must not block movement; entering its cell triggers the fall."));
			}
			if (!bHideCellFloor)
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("Pit should hide the standard cell floor."));
			}
			if (MovingMesh)
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error,
					TEXT("Pit no longer supports the legacy single MovingMesh cover. Use Left Leaf Mesh and Right Leaf Mesh."));
			}
			const bool bHasLeftLeaf = PitLeftLeafMesh != nullptr;
			const bool bHasRightLeaf = PitRightLeafMesh != nullptr;
			if (bHasLeftLeaf != bHasRightLeaf)
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error,
					TEXT("Pit trapdoor cover is incomplete: both Left Leaf Mesh and Right Leaf Mesh are required."));
			}
			if (!bHasLeftLeaf && !bHasRightLeaf && !DefaultBehavior.Pit.bInitiallyOpen)
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Warning,
					TEXT("Pit has no dual-leaf cover, so it is a static open hole regardless of Initially Open=false."));
			}
			if (HasCompletePitTrapdoorCover() && RuntimeActorClass && !RuntimeActorClass->IsChildOf(AGridPitTrapdoorActor::StaticClass()))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error,
					TEXT("A dual-leaf Pit trapdoor requires GridPitTrapdoorActor (or a derived Blueprint) as Runtime Actor Class."));
			}
			if (!FMath::IsFinite(DefaultBehavior.PitAnimation.OpenAngleDegrees) ||
				DefaultBehavior.PitAnimation.OpenAngleDegrees < 0.0f || DefaultBehavior.PitAnimation.OpenAngleDegrees > 120.0f)
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error,
					TEXT("Pit Open Angle must be finite and between 0 and 120 degrees."));
			}
			if (DefaultBehavior.PitAnimation.LeftHingeLocation.ContainsNaN() || DefaultBehavior.PitAnimation.RightHingeLocation.ContainsNaN())
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error,
					TEXT("Pit hinge locations must contain finite coordinates."));
			}
			break;
		}

		case EGridLevelObjectType::PressurePlate:
		{
			if (!IsFloorPlacement(PlacementSurface))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("PressurePlate Placement Surface must be Floor."));
			}
			if (bIsInteractable)
			{
				AddValidationMessage(
					OutMessages, EGridArchetypeValidationSeverity::Info, TEXT("PressurePlate is marked interactable, which is usually unnecessary."));
			}
			if (!HasPreviewOrMovingMesh(*this))
			{
				AddValidationMessage(
					OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("PressurePlate should generally define PreviewMesh or MovingMesh."));
			}
			break;
		}

		case EGridLevelObjectType::Trigger:
		{
			if (!IsFloorPlacement(PlacementSurface))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Trigger Placement Surface must be Floor."));
			}
			break;
		}

		case EGridLevelObjectType::Decoration:
		{
			if (!HasAnyMesh(*this))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("Decoration should generally define a mesh."));
			}
			if (bIsReadable && !bIsInteractable)
			{
				AddValidationMessage(
					OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("Readable decoration should generally also be interactable."));
			}
			if (bIsReadable && ReadableText.IsEmpty())
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Info,
					TEXT("ReadableText is empty; this is acceptable when OverrideReadableText is defined on level object data."));
			}
			break;
		}

		case EGridLevelObjectType::Light:
		{
			if (!bIsLightSource)
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("Light should generally have bIsLightSource set to true."));
			}
			if (bIsLightSource && LightIntensity <= 0.f)
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("LightIntensity must be greater than 0 when bIsLightSource is true."));
			}
			if (bIsLightSource && LightRadius <= 0.f)
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("LightRadius must be greater than 0 when bIsLightSource is true."));
			}
			break;
		}

		case EGridLevelObjectType::Teleporter:
		{
			if (!IsFloorPlacement(PlacementSurface))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Teleporter Placement Surface must be Floor."));
			}
			if (DefaultBehavior.Teleporter.TargetCellX == INDEX_NONE || DefaultBehavior.Teleporter.TargetCellY == INDEX_NONE)
			{
				AddValidationMessage(
					OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("Teleporter should define DefaultBehavior TargetCellX and TargetCellY."));
			}
			break;
		}

		case EGridLevelObjectType::Receptacle:
		{
			if (!bIsInteractable)
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("Receptacle should generally be interactable."));
			}
			if (RuntimeActorClass && !RuntimeActorClass->IsChildOf(AGridReceptacleActor::StaticClass()))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error,
					TEXT("Receptacle RuntimeActorClass must derive from AGridReceptacleActor."));
			}
			if (!IsWallLockArchetype(*this) && !DefaultBehavior.Receptacle.bAcceptAnyItem && DefaultBehavior.Receptacle.AcceptedItems.Num() == 0)
			{
				AddValidationMessage(
					OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("Receptacle does not accept any item because AcceptedItems is empty."));
			}
			for (const FGridReceptacleAcceptedItemConfig& AcceptedItem : DefaultBehavior.Receptacle.AcceptedItems)
			{
				if (!AcceptedItem.ItemDefinition)
				{
					AddValidationMessage(
						OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("Receptacle AcceptedItems contains an entry without an ItemDefinition."));
				}
			}
			for (const FGridReceptacleInitialItemConfig& InitialItem : DefaultBehavior.Receptacle.InitialContent)
			{
				if (!InitialItem.ItemDefinition)
				{
					AddValidationMessage(
						OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("Receptacle InitialContent contains an entry without an ItemDefinition."));
				}
			}
			break;
		}

		case EGridLevelObjectType::MonsterSpawn:
		case EGridLevelObjectType::ItemSpawn:
		{
			if (!IsFloorPlacement(PlacementSurface))
			{
				AddValidationMessage(
					OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("MonsterSpawn and ItemSpawn Placement Surface must be Floor."));
			}
			if (SupportedType == EGridLevelObjectType::ItemSpawn && !Category.IsNone() && !IsPaletteCategory(*this, TEXT("Spawns")))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Info, TEXT("ItemSpawn palette category should generally be Spawns."));
			}
			break;
		}

		case EGridLevelObjectType::Item:
		{
			if (!Category.IsNone() && !IsPaletteCategory(*this, TEXT("Items")))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Info, TEXT("Item palette category should generally be Items."));
			}
			if (!IsFloorPlacement(PlacementSurface))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Warning,
					TEXT("Item Placement Surface should be Floor while items still use the world-object placement path."));
			}
			const bool bHasDefaultItemDefinition = DefaultBehavior.Item.ItemDefinitionAsset || !DefaultBehavior.Item.ItemDefinitionId.IsNone();
			if (!ItemActorClass && !bHasDefaultItemDefinition)
			{
				AddValidationMessage(
					OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("Item should generally define ItemActorClass or DefaultBehavior.Item."));
			}
			break;
		}

		case EGridLevelObjectType::None:
		default:
			break;
	}

	for (const FGridArchetypeValidationMessage& Message : OutMessages)
	{
		if (Message.Severity == EGridArchetypeValidationSeverity::Error)
		{
			return false;
		}
	}
	return true;
}

bool UGridObjectArchetypeAsset::IsValidArchetype() const
{
	TArray<FGridArchetypeValidationMessage> Messages;
	return ValidateArchetype(Messages);
}

FString UGridObjectArchetypeAsset::GetValidationSummary() const
{
	TArray<FGridArchetypeValidationMessage> Messages;
	ValidateArchetype(Messages);

	int32 ErrorCount = 0;
	int32 WarningCount = 0;
	int32 InfoCount = 0;

	for (const FGridArchetypeValidationMessage& Message : Messages)
	{
		switch (Message.Severity)
		{
			case EGridArchetypeValidationSeverity::Error:
				++ErrorCount;
				break;
			case EGridArchetypeValidationSeverity::Warning:
				++WarningCount;
				break;
			case EGridArchetypeValidationSeverity::Info:
				++InfoCount;
				break;
		}
	}

	const FString ArchetypeName = ArchetypeId.IsNone() ? GetName() : ArchetypeId.ToString();
	FString Summary =
		FString::Printf(TEXT("Grid archetype validation: %s | Errors=%d Warnings=%d Info=%d"), *ArchetypeName, ErrorCount, WarningCount, InfoCount);

	for (const FGridArchetypeValidationMessage& Message : Messages)
	{
		Summary += FString::Printf(TEXT("\n- [%s] %s"), ToValidationSeverityText(Message.Severity), *Message.Message);
	}

	return Summary;
}

bool UGridObjectArchetypeAsset::RequiresEdgePlacement() const
{
	// Historical API name retained for current editor call sites.
	// After WORLDOBJ-MIG01 this means "requires a Wall surface / WallSide".
	switch (SupportedType)
	{
		case EGridLevelObjectType::Door:
		case EGridLevelObjectType::Button:
		case EGridLevelObjectType::Lever:
			return true;
		default:
			return false;
	}
}

bool UGridObjectArchetypeAsset::SupportsCenterPlacement() const
{
	// Historical API name retained for current callers; Center is not a valid surface.
	// The helper now means "supports cell/floor placement".
	switch (SupportedType)
	{
		case EGridLevelObjectType::PressurePlate:
		case EGridLevelObjectType::Trigger:
		case EGridLevelObjectType::Decoration:
		case EGridLevelObjectType::Light:
		case EGridLevelObjectType::Teleporter:
		case EGridLevelObjectType::MonsterSpawn:
		case EGridLevelObjectType::ItemSpawn:
		case EGridLevelObjectType::Item:
		case EGridLevelObjectType::Pit:
		case EGridLevelObjectType::StoryCompanion:
		case EGridLevelObjectType::CustomRecruiter:
			return true;
		default:
			return false;
	}
}

bool UGridObjectArchetypeAsset::SupportsWallPlacement() const
{
	switch (SupportedType)
	{
		case EGridLevelObjectType::Door:
		case EGridLevelObjectType::Button:
		case EGridLevelObjectType::Lever:
		case EGridLevelObjectType::Decoration:
		case EGridLevelObjectType::Light:
		case EGridLevelObjectType::Receptacle:
			return true;
		default:
			return false;
	}
}

bool UGridObjectArchetypeAsset::RequiresRuntimeActorClass() const
{
	switch (SupportedType)
	{
		case EGridLevelObjectType::Door:
		case EGridLevelObjectType::Button:
		case EGridLevelObjectType::Lever:
		case EGridLevelObjectType::PressurePlate:
		case EGridLevelObjectType::Teleporter:
		case EGridLevelObjectType::Receptacle:
		case EGridLevelObjectType::Pit:
			return true;
		default:
			return false;
	}
}

bool UGridObjectArchetypeAsset::AllowsInvisibleRuntimeObject() const
{
	switch (SupportedType)
	{
		case EGridLevelObjectType::Trigger:
		case EGridLevelObjectType::MonsterSpawn:
		case EGridLevelObjectType::ItemSpawn:
			return true;
		default:
			return false;
	}
}

bool UGridObjectArchetypeAsset::UsesWallPlacementParams() const
{
	return PlacementSurface == EGridObjectPlacementKind::Wall;
}

bool UGridObjectArchetypeAsset::UsesCenterPlacementParams() const
{
	return PlacementSurface == EGridObjectPlacementKind::Floor || PlacementSurface == EGridObjectPlacementKind::Ceiling;
}

bool UGridObjectArchetypeAsset::UsesReadableParams() const
{
	return bIsReadable || ObjectCategory == EGridObjectCategory::Readable || (SupportedType == EGridLevelObjectType::Decoration && bIsReadable);
}

bool UGridObjectArchetypeAsset::UsesLightParams() const
{
	return bIsLightSource || SupportedType == EGridLevelObjectType::Light || ObjectCategory == EGridObjectCategory::Light;
}

bool UGridObjectArchetypeAsset::UsesItemParams() const
{
	return SupportedType == EGridLevelObjectType::Item || SupportedType == EGridLevelObjectType::ItemSpawn || ItemActorClass != nullptr;
}

bool UGridObjectArchetypeAsset::UsesReceptacleParams() const
{
	return SupportedType == EGridLevelObjectType::Receptacle;
}

bool UGridObjectArchetypeAsset::UsesTeleporterParams() const
{
	return SupportedType == EGridLevelObjectType::Teleporter;
}

bool UGridObjectArchetypeAsset::UsesButtonAnimationParams() const
{
	return SupportedType == EGridLevelObjectType::Button;
}

bool UGridObjectArchetypeAsset::UsesTriggerParams() const
{
	switch (SupportedType)
	{
		case EGridLevelObjectType::Trigger:
		case EGridLevelObjectType::PressurePlate:
		case EGridLevelObjectType::Button:
		case EGridLevelObjectType::Lever:
		case EGridLevelObjectType::Receptacle:
		case EGridLevelObjectType::Teleporter:
			return true;
		default:
			return false;
	}
}

bool UGridObjectArchetypeAsset::UsesMovingMeshParams() const
{
	if (UsesItemParams())
	{
		return true;
	}

	switch (SupportedType)
	{
		case EGridLevelObjectType::Door:
		case EGridLevelObjectType::Button:
		case EGridLevelObjectType::Lever:
		case EGridLevelObjectType::Receptacle:
			return true;
		default:
			return false;
	}
}

bool UGridObjectArchetypeAsset::UsesFixedMeshParams() const
{
	return SupportedType == EGridLevelObjectType::Door || SupportedType == EGridLevelObjectType::Pit || UsesItemParams();
}

bool UGridObjectArchetypeAsset::UsesRuntimeActorClass() const
{
	return RequiresRuntimeActorClass() || RuntimeActorClass != nullptr;
}

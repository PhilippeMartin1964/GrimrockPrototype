#include "Core/GridObjectArchetypeAsset.h"

#include "Runtime/GridDoorActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridWallLockActor.h"

namespace
{
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

	bool IsFloorOrCenterPlacement(EGridObjectPlacementKind PlacementKind)
	{
		return PlacementKind == EGridObjectPlacementKind::Floor || PlacementKind == EGridObjectPlacementKind::Center;
	}

	bool IsCenterFloorOrCeilingPlacement(EGridObjectPlacementKind PlacementKind)
	{
		return PlacementKind == EGridObjectPlacementKind::Center || PlacementKind == EGridObjectPlacementKind::Floor ||
			PlacementKind == EGridObjectPlacementKind::Ceiling;
	}

	bool IsWallOrEdgePlacement(EGridObjectPlacementKind PlacementKind)
	{
		return PlacementKind == EGridObjectPlacementKind::Wall || PlacementKind == EGridObjectPlacementKind::Edge;
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
		if (SupportedType == EGridLevelObjectType::None)
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

	bool IsDefaultWallPlacementParams(const UGridObjectArchetypeAsset& Archetype)
	{
		return FMath::IsNearlyEqual(Archetype.WallInset, 6.f) && FMath::IsNearlyZero(Archetype.LocalOffsetAlongWall) &&
			FMath::IsNearlyZero(Archetype.LocalOffsetVertical);
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

void UGridObjectArchetypeAsset::PostLoad()
{
	Super::PostLoad();

	if (SupportedType != EGridLevelObjectType::Door)
	{
		return;
	}

	// Historical Door attenuation becomes the single archetype-wide attenuation.
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

bool UGridObjectArchetypeAsset::ResolveAudioEvent(FName EventName, FGridObjectAudioEvent& OutEvent) const
{
	if (const FGridObjectAudioEvent* Event = AudioEvents.Find(EventName))
	{
		OutEvent = *Event;
		return true;
	}

	// Compatibility for already-saved door archetypes that still carry the
	// pre-generic serialized fields and have not yet been resaved after migration.
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

	if (bReplacesStandardWall && !IsWallOrEdgePlacement(PlacementKind))
	{
		AddValidationMessage(
			OutMessages, EGridArchetypeValidationSeverity::Warning, TEXT("Replaces Standard Wall is enabled but PlacementKind is not Wall or Edge."));
	}

	if (bReplacesStandardWall && bCanShareAnchor)
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Warning,
			TEXT("Replaces Standard Wall is enabled while bCanShareAnchor=true. Multiple wall replacements can overlap on the same edge."));
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

	if (!UsesWallPlacementParams() && !IsDefaultWallPlacementParams(*this))
	{
		AddValidationMessage(
			OutMessages, EGridArchetypeValidationSeverity::Info, TEXT("Wall placement parameters are set but this archetype is not wall/edge placed."));
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

	if (!UsesMovingMeshParams() && (MovingMesh || MovingMaterial))
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Info,
			TEXT("MovingMesh or MovingMaterial is set but this archetype type does not normally use moving mesh."));
	}

	if (!IsWallLockArchetype(*this) && !UsesFixedMeshParams() && (FixedMesh || FixedMaterial))
	{
		AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Info,
			TEXT("FixedMesh or FixedMaterial is set but this archetype type does not normally use fixed mesh."));
	}

	switch (SupportedType)
	{
		case EGridLevelObjectType::Door:
		{
			if (!IsWallOrEdgePlacement(PlacementKind))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Door PlacementKind must be Edge or Wall."));
			}
			if (!HasPreviewOrMovingMesh(*this))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Door requires PreviewMesh or MovingMesh."));
			}
			if (RuntimeActorClass && !RuntimeActorClass->IsChildOf(AGridDoorActor::StaticClass()))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error,
					TEXT(
						"Door RuntimeActorClass must derive from AGridDoorActor. Door_Secret can use AGridSecretDoorActor or a Blueprint derived from AGridDoorActor."));
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
			if (!IsWallOrEdgePlacement(PlacementKind))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Button and Lever PlacementKind must be Wall or Edge."));
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
			if (!IsFloorOrCenterPlacement(PlacementKind))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Pit PlacementKind must be Floor or Center."));
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
			if (!DefaultBehavior.Transition.bIsTransition)
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Pit requires Transition.bIsTransition=true."));
			}
			if (DefaultBehavior.Transition.bRequireUseAction)
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Pit transition cannot require the Use action."));
			}
			break;
		}

		case EGridLevelObjectType::PressurePlate:
		{
			if (!IsFloorOrCenterPlacement(PlacementKind))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("PressurePlate PlacementKind must be Floor or Center."));
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
			if (!IsFloorOrCenterPlacement(PlacementKind))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Trigger PlacementKind must be Floor or Center."));
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
				AddValidationMessage(
					OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("LightIntensity must be greater than 0 when bIsLightSource is true."));
			}
			if (bIsLightSource && LightRadius <= 0.f)
			{
				AddValidationMessage(
					OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("LightRadius must be greater than 0 when bIsLightSource is true."));
			}
			break;
		}

		case EGridLevelObjectType::Teleporter:
		{
			if (!IsFloorOrCenterPlacement(PlacementKind))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("Teleporter PlacementKind must be Floor or Center."));
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
					TEXT(
						"Receptacle RuntimeActorClass must derive from AGridReceptacleActor. Concrete Receptacle_* variants can use AGridReceptacleActor or a Blueprint derived from it."));
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
			if (!IsFloorOrCenterPlacement(PlacementKind))
			{
				AddValidationMessage(
					OutMessages, EGridArchetypeValidationSeverity::Error, TEXT("MonsterSpawn and ItemSpawn PlacementKind must be Floor or Center."));
			}
			if (SupportedType == EGridLevelObjectType::ItemSpawn)
			{
				if (!Category.IsNone() && !IsPaletteCategory(*this, TEXT("Spawns")))
				{
					AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Info, TEXT("ItemSpawn palette category should generally be Spawns."));
				}
			}
			break;
		}

		case EGridLevelObjectType::Item:
		{
			if (!Category.IsNone() && !IsPaletteCategory(*this, TEXT("Items")))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Info, TEXT("Item palette category should generally be Items."));
			}
			if (!IsFloorOrCenterPlacement(PlacementKind) && ArchetypeId != FName(TEXT("Item_Torch")))
			{
				AddValidationMessage(OutMessages, EGridArchetypeValidationSeverity::Warning,
					TEXT("Item PlacementKind should generally be Floor or Center when placed in the level."));
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
	return PlacementKind == EGridObjectPlacementKind::Wall || PlacementKind == EGridObjectPlacementKind::Edge;
}

bool UGridObjectArchetypeAsset::UsesCenterPlacementParams() const
{
	return PlacementKind == EGridObjectPlacementKind::Center || PlacementKind == EGridObjectPlacementKind::Floor ||
		PlacementKind == EGridObjectPlacementKind::Ceiling;
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
	return SupportedType == EGridLevelObjectType::Door || UsesItemParams();
}

bool UGridObjectArchetypeAsset::UsesRuntimeActorClass() const
{
	return RequiresRuntimeActorClass() || RuntimeActorClass != nullptr;
}

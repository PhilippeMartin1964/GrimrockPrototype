#include "Core/GridWorldObjectDefinitionAsset.h"

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

	const TCHAR* ToValidationSeverityText(EGridWorldObjectDefinitionValidationSeverity Severity)
	{
		switch (Severity)
		{
			case EGridWorldObjectDefinitionValidationSeverity::Info:
				return TEXT("Info");
			case EGridWorldObjectDefinitionValidationSeverity::Warning:
				return TEXT("Warning");
			case EGridWorldObjectDefinitionValidationSeverity::Error:
				return TEXT("Error");
			default:
				return TEXT("Unknown");
		}
	}

	void AddValidationMessage(TArray<FGridWorldObjectDefinitionValidationMessage>& Messages, EGridWorldObjectDefinitionValidationSeverity Severity, const TCHAR* Message)
	{
		Messages.Emplace(Severity, FString(Message));
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
			case EGridLevelObjectType::Door: return TEXT("Door");
			case EGridLevelObjectType::Button: return TEXT("Button");
			case EGridLevelObjectType::PressurePlate: return TEXT("PressurePlate");
			case EGridLevelObjectType::Lever: return TEXT("Lever");
			case EGridLevelObjectType::Decoration: return TEXT("Decoration");
			case EGridLevelObjectType::MonsterSpawn: return TEXT("MonsterSpawn");
			case EGridLevelObjectType::ItemSpawn: return TEXT("ItemSpawn");
			case EGridLevelObjectType::Light: return TEXT("Light");
			case EGridLevelObjectType::Teleporter: return TEXT("Teleporter");
			case EGridLevelObjectType::Trigger: return TEXT("Trigger");
			case EGridLevelObjectType::Receptacle: return TEXT("Receptacle");
			case EGridLevelObjectType::Item: return TEXT("Item");
			case EGridLevelObjectType::Logic: return TEXT("Logic");
			case EGridLevelObjectType::StoryCompanion: return TEXT("StoryCompanion");
			case EGridLevelObjectType::CustomRecruiter: return TEXT("CustomRecruiter");
			case EGridLevelObjectType::Pit: return TEXT("Pit");
			case EGridLevelObjectType::None:
			default: return TEXT("None");
		}
	}

	const TCHAR* ToObjectCategoryText(EGridObjectCategory Category)
	{
		switch (Category)
		{
			case EGridObjectCategory::Mechanism: return TEXT("Mechanism");
			case EGridObjectCategory::Decoration: return TEXT("Decoration");
			case EGridObjectCategory::Prop: return TEXT("Prop");
			case EGridObjectCategory::Receptacle: return TEXT("Receptacle");
			case EGridObjectCategory::Light: return TEXT("Light");
			case EGridObjectCategory::Readable: return TEXT("Readable");
			case EGridObjectCategory::Spawn: return TEXT("Spawn");
			case EGridObjectCategory::Teleporter: return TEXT("Teleporter");
			case EGridObjectCategory::Item: return TEXT("Item");
			default: return TEXT("Unknown");
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

	bool IsDefaultLightParams(const UGridWorldObjectDefinitionAsset& Definition)
	{
		return Definition.LightColor.Equals(FLinearColor::White) && FMath::IsNearlyEqual(Definition.LightIntensity, 500.f) &&
			FMath::IsNearlyEqual(Definition.LightRadius, 500.f) && !Definition.bUseLightFlicker;
	}

	bool HasReceptacleBehaviorParams(const FGridObjectBehaviorParams& Behavior)
	{
		return !Behavior.Receptacle.bAcceptAnyItem || Behavior.Receptacle.AcceptedItems.Num() > 0 || Behavior.Receptacle.InitialContent.Num() > 0 ||
			Behavior.Receptacle.MaxContainedItems != 1 || Behavior.Receptacle.VisualPlacementMode != EGridReceptacleVisualPlacementMode::AttachedSocket ||
			Behavior.Receptacle.bSimulatePhysicsWhenPlaced || !FMath::IsNearlyEqual(Behavior.Receptacle.PhysicalPlacementSurfaceOffset, 10.f) ||
			!Behavior.Receptacle.PhysicalPlacementInitialRotationOffset.IsNearlyZero();
	}

	bool IsWallLockDefinition(const UGridWorldObjectDefinitionAsset& Definition)
	{
		return (Definition.RuntimeActorClass && Definition.RuntimeActorClass->IsChildOf(AGridWallLockActor::StaticClass())) ||
			Definition.DefaultBehavior.Lock.AcceptedKeyIds.Num() > 0 || Definition.DefaultBehavior.Lock.AcceptedKeyItems.Num() > 0;
	}

	bool HasTeleporterBehaviorParams(const FGridObjectBehaviorParams& Behavior)
	{
		return Behavior.Teleporter.TargetCellX != INDEX_NONE || Behavior.Teleporter.TargetCellY != INDEX_NONE;
	}

	bool HasCustomButtonBehaviorParams(const FGridObjectBehaviorParams& Behavior)
	{
		return !FMath::IsNearlyEqual(Behavior.ButtonAnimation.ButtonHoldTime, 0.15f);
	}

	bool IsPaletteCategory(const UGridWorldObjectDefinitionAsset& Definition, const TCHAR* ExpectedCategory)
	{
		return Definition.Category == FName(ExpectedCategory);
	}

	bool IsExpectedConcreteReceptacleDefinition(FName DefinitionId)
	{
		static const FName ReceptacleAlcoveId(TEXT("Receptacle_Alcove"));
		static const FName ReceptacleStoneAlcoveId(TEXT("Receptacle_Alcove_Stone"));
		static const FName ReceptacleTorchHolderId(TEXT("Receptacle_TorchHolder"));
		static const FName ReceptacleAltarId(TEXT("Receptacle_Altar"));
		static const FName ReceptacleOfferingBowlId(TEXT("Receptacle_OfferingBowl"));
		return DefinitionId == ReceptacleAlcoveId || DefinitionId == ReceptacleStoneAlcoveId || DefinitionId == ReceptacleTorchHolderId ||
			DefinitionId == ReceptacleAltarId || DefinitionId == ReceptacleOfferingBowlId;
	}
}

void UGridWorldObjectDefinitionAsset::RefreshPlacementRuntimeProjection()
{
	PlacementKind = PlacementSurface;
	LocalOffsetAlongWall = DefaultLocalPosition.U;
	LocalOffsetVertical = 0.0f;
	WallInset = DefaultLocalPosition.N;

	switch (PlacementSurface)
	{
		case EGridObjectPlacementKind::Wall:
			PlacementZOffset = DefaultLocalPosition.V;
			break;
		case EGridObjectPlacementKind::Ceiling:
			PlacementZOffset = CurrentCeilingPlaneHeight - DefaultLocalPosition.N;
			break;
		case EGridObjectPlacementKind::Floor:
			PlacementZOffset = DefaultLocalPosition.N;
			break;
		case EGridObjectPlacementKind::Center:
		case EGridObjectPlacementKind::Edge:
		default:
			PlacementZOffset = 0.0f;
			WallInset = 0.0f;
			LocalOffsetAlongWall = 0.0f;
			LocalOffsetVertical = 0.0f;
			break;
	}
}

void UGridWorldObjectDefinitionAsset::PostLoad()
{
	Super::PostLoad();
	RefreshPlacementRuntimeProjection();

	if (SupportedType != EGridLevelObjectType::Door)
	{
		return;
	}

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
void UGridWorldObjectDefinitionAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshPlacementRuntimeProjection();
}
#endif

bool UGridWorldObjectDefinitionAsset::ResolveAudioEvent(FName EventName, FGridObjectAudioEvent& OutEvent) const
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

bool UGridWorldObjectDefinitionAsset::ValidateDefinition(TArray<FGridWorldObjectDefinitionValidationMessage>& OutMessages) const
{
	OutMessages.Reset();

	if (DefinitionId.IsNone()) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("DefinitionId is not set."));
	if (SupportedType == EGridLevelObjectType::None) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("SupportedType must not be None."));
	if (!HasValidPlacementSurface())
	{
		AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error,
			TEXT("Placement Surface must be Floor, Wall or Ceiling. Center and Edge are no longer valid authoring values."));
	}
	if (!DefaultLocalPosition.IsFinite())
	{
		AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error,
			TEXT("Default Local Position U/V/N must contain finite values."));
	}
	if (DefinitionId == FName(TEXT("Door_Secret")) && SupportedType != EGridLevelObjectType::Door)
	{
		AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error,
			TEXT("Door_Secret must use SupportedType=Door. Visual variants must stay definitions, not EGridLevelObjectType values."));
	}
	if (IsExpectedConcreteReceptacleDefinition(DefinitionId) && SupportedType != EGridLevelObjectType::Receptacle)
	{
		AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error,
			TEXT("Concrete receptacle definitions must use SupportedType=Receptacle. Visual variants must stay definitions, not EGridLevelObjectType values."));
	}
	if ((DefinitionId == FName(TEXT("Receptacle_Alcove")) || DefinitionId == FName(TEXT("Receptacle_Alcove_Stone"))) &&
		DefaultBehavior.Receptacle.MaxContainedItems == 1)
	{
		AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("Receptacle alcoves must use MaxContainedItems > 1 or <= 0."));
	}
	if ((DefinitionId == FName(TEXT("Receptacle_Alcove")) || DefinitionId == FName(TEXT("Receptacle_Alcove_Stone"))) &&
		DefaultBehavior.Receptacle.VisualPlacementMode != EGridReceptacleVisualPlacementMode::PhysicalAtHit)
	{
		AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("Receptacle alcoves must use VisualPlacementMode=PhysicalAtHit."));
	}
	if (RequiresRuntimeActorClass() && !RuntimeActorClass)
	{
		AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("RuntimeActorClass is required for this SupportedType."));
	}

	for (const TPair<FName, FGridObjectAudioEvent>& Pair : AudioEvents)
	{
		if (Pair.Key.IsNone()) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("Audio event key must not be None."));
		if (!FMath::IsFinite(Pair.Value.Volume) || Pair.Value.Volume < 0.f)
		{
			AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error,
				*FString::Printf(TEXT("Audio event %s Volume must be finite and >= 0."), *Pair.Key.ToString()));
		}
		if (!FMath::IsFinite(Pair.Value.PitchVariation) || Pair.Value.PitchVariation < 0.f || Pair.Value.PitchVariation > 0.25f)
		{
			AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error,
				*FString::Printf(TEXT("Audio event %s PitchVariation must be finite and between 0.0 and 0.25."), *Pair.Key.ToString()));
		}
	}

	if (Category.IsNone())
	{
		AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Info,
			TEXT("Palette Category is not set. This does not affect runtime, but the object may be harder to organize in the editor palette."));
	}
	if (bReplacesStandardWall && !IsWallPlacement(PlacementSurface))
	{
		AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("Replaces Standard Wall is enabled but Placement Surface is not Wall."));
	}
	if (bReplacesStandardWall && bCanShareAnchor)
	{
		AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning,
			TEXT("Replaces Standard Wall is enabled while bCanShareAnchor=true. Multiple wall replacements can overlap on the same boundary."));
	}
	if (!IsObjectCategoryCompatible(SupportedType, ObjectCategory, bIsReadable))
	{
		if (SupportedType == EGridLevelObjectType::Decoration && bIsReadable)
		{
			AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("Readable Decoration should generally use ObjectCategory=Readable."));
		}
		else
		{
			const EGridObjectCategory RecommendedCategory = GetRecommendedObjectCategory(SupportedType, bIsReadable);
			OutMessages.Emplace(EGridWorldObjectDefinitionValidationSeverity::Warning,
				FString::Printf(TEXT("%s should generally use ObjectCategory=%s, but currently uses %s."), ToSupportedTypeText(SupportedType),
					ToObjectCategoryText(RecommendedCategory), ToObjectCategoryText(ObjectCategory)));
		}
	}
	if (!bIsReadable && !ReadableText.IsEmpty()) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("ReadableText is set but bIsReadable=false."));
	if (!bIsReadable && bShowReadableOnlyOnce) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Info, TEXT("bShowReadableOnlyOnce is enabled but bIsReadable=false."));
	if (!UsesLightParams() && !bIsLightSource && !IsDefaultLightParams(*this))
	{
		AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Info, TEXT("Light parameters are customized but this definition is not a light source."));
	}
	if (bUseLightFlicker && !bIsLightSource) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("Light flicker is enabled but bIsLightSource=false."));
	if (!UsesReceptacleParams() && HasReceptacleBehaviorParams(DefaultBehavior))
	{
		AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Info, TEXT("Receptacle behavior parameters are set but SupportedType is not Receptacle."));
	}
	if (!UsesTeleporterParams() && HasTeleporterBehaviorParams(DefaultBehavior))
	{
		AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Info, TEXT("Teleporter target cell is set but SupportedType is not Teleporter."));
	}
	if (!UsesButtonAnimationParams() && HasCustomButtonBehaviorParams(DefaultBehavior))
	{
		AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Info, TEXT("Button behavior parameters are customized but SupportedType is not Button."));
	}

	switch (SupportedType)
	{
		case EGridLevelObjectType::Door:
		{
			if (!IsWallPlacement(PlacementSurface)) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("Door Placement Surface must be Wall."));
			if (!HasMovingVisualPart()) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("Door requires at least one Moving Part."));
			if (RuntimeActorClass && !RuntimeActorClass->IsChildOf(AGridDoorActor::StaticClass()))
			{
				AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("Door RuntimeActorClass must derive from AGridDoorActor."));
			}
			if (bCanShareAnchor) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("Door should generally have bCanShareAnchor set to false."));
			break;
		}
		case EGridLevelObjectType::Button:
		case EGridLevelObjectType::Lever:
		{
			if (!IsWallPlacement(PlacementSurface)) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("Button and Lever Placement Surface must be Wall."));
			if (!bIsInteractable) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("Button and Lever should generally be interactable."));
			if (!HasMovingVisualPart()) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("Button and Lever should generally define at least one Moving Part."));
			break;
		}
		case EGridLevelObjectType::Pit:
		{
			if (!IsFloorPlacement(PlacementSurface)) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("Pit Placement Surface must be Floor."));
			if (!StaticPart.IsDefined()) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("Pit requires a Static Part for the permanent pit geometry."));
			if (bBlocksMovement) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("Pit must not block movement; entering its cell triggers the fall."));
			if (!bHideCellFloor) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("Pit should hide the standard cell floor."));
			const bool bHasPart0 = MovingParts.Part0.IsDefined();
			const bool bHasPart1 = MovingParts.Part1.IsDefined();
			if (bHasPart0 != bHasPart1)
			{
				AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error,
					TEXT("Pit trapdoor cover is incomplete: Moving Part 0 and Moving Part 1 must both be defined."));
			}
			if (!bHasPart0 && !bHasPart1 && !DefaultBehavior.Pit.bInitiallyOpen)
			{
				AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning,
					TEXT("Pit has no moving cover, so it is a static open hole regardless of Initially Open=false."));
			}
			if (HasCompletePitTrapdoorCover() && RuntimeActorClass && !RuntimeActorClass->IsChildOf(AGridPitTrapdoorActor::StaticClass()))
			{
				AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error,
					TEXT("A dual-part Pit trapdoor requires GridPitTrapdoorActor (or a derived Blueprint) as Runtime Actor Class."));
			}
			if (bHasPart0)
			{
				const FGridWorldObjectMotion& Motion0 = MovingParts.Part0.Motion;
				const FGridWorldObjectMotion& Motion1 = MovingParts.Part1.Motion;
				if (!FMath::IsFinite(Motion0.Amount) || !FMath::IsFinite(Motion1.Amount) || FMath::Abs(Motion0.Amount) > 120.0f || FMath::Abs(Motion1.Amount) > 120.0f)
				{
					AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error,
						TEXT("Pit trapdoor Motion.Amount must be finite and within +/-120 degrees."));
				}
				if (Motion0.Pivot.ContainsNaN() || Motion1.Pivot.ContainsNaN())
				{
					AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error,
						TEXT("Pit trapdoor Motion.Pivot coordinates must be finite."));
				}
				if (!FMath::IsFinite(Motion0.Duration) || !FMath::IsFinite(Motion1.Duration) || Motion0.Duration < 0.0f || Motion1.Duration < 0.0f)
				{
					AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error,
						TEXT("Pit trapdoor Motion.Duration must be finite and >= 0."));
				}
			}
			break;
		}
		case EGridLevelObjectType::PressurePlate:
		{
			if (!IsFloorPlacement(PlacementSurface)) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("PressurePlate Placement Surface must be Floor."));
			if (bIsInteractable) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Info, TEXT("PressurePlate is marked interactable, which is usually unnecessary."));
			if (!HasMovingVisualPart()) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("PressurePlate should generally define at least one Moving Part."));
			break;
		}
		case EGridLevelObjectType::Trigger:
			if (!IsFloorPlacement(PlacementSurface)) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("Trigger Placement Surface must be Floor."));
			break;
		case EGridLevelObjectType::Decoration:
		{
			if (!HasAnyVisualPart()) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("Decoration should generally define a visual part."));
			if (bIsReadable && !bIsInteractable) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("Readable decoration should generally also be interactable."));
			if (bIsReadable && ReadableText.IsEmpty()) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Info, TEXT("ReadableText is empty; this is acceptable when OverrideReadableText is defined on level object data."));
			break;
		}
		case EGridLevelObjectType::Light:
		{
			if (!bIsLightSource) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("Light should generally have bIsLightSource set to true."));
			if (bIsLightSource && LightIntensity <= 0.f) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("LightIntensity must be greater than 0 when bIsLightSource is true."));
			if (bIsLightSource && LightRadius <= 0.f) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("LightRadius must be greater than 0 when bIsLightSource is true."));
			break;
		}
		case EGridLevelObjectType::Teleporter:
		{
			if (!IsFloorPlacement(PlacementSurface)) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("Teleporter Placement Surface must be Floor."));
			if (DefaultBehavior.Teleporter.TargetCellX == INDEX_NONE || DefaultBehavior.Teleporter.TargetCellY == INDEX_NONE)
			{
				AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("Teleporter should define DefaultBehavior TargetCellX and TargetCellY."));
			}
			break;
		}
		case EGridLevelObjectType::Receptacle:
		{
			if (!bIsInteractable) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("Receptacle should generally be interactable."));
			if (RuntimeActorClass && !RuntimeActorClass->IsChildOf(AGridReceptacleActor::StaticClass()))
			{
				AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("Receptacle RuntimeActorClass must derive from AGridReceptacleActor."));
			}
			if (!IsWallLockDefinition(*this) && !DefaultBehavior.Receptacle.bAcceptAnyItem && DefaultBehavior.Receptacle.AcceptedItems.Num() == 0)
			{
				AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("Receptacle does not accept any item because AcceptedItems is empty."));
			}
			for (const FGridReceptacleAcceptedItemConfig& AcceptedItem : DefaultBehavior.Receptacle.AcceptedItems)
			{
				if (!AcceptedItem.ItemDefinition) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("Receptacle AcceptedItems contains an entry without an ItemDefinition."));
			}
			for (const FGridReceptacleInitialItemConfig& InitialItem : DefaultBehavior.Receptacle.InitialContent)
			{
				if (!InitialItem.ItemDefinition) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("Receptacle InitialContent contains an entry without an ItemDefinition."));
			}
			break;
		}
		case EGridLevelObjectType::MonsterSpawn:
		case EGridLevelObjectType::ItemSpawn:
		{
			if (!IsFloorPlacement(PlacementSurface)) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Error, TEXT("MonsterSpawn and ItemSpawn Placement Surface must be Floor."));
			if (SupportedType == EGridLevelObjectType::ItemSpawn && !Category.IsNone() && !IsPaletteCategory(*this, TEXT("Spawns")))
			{
				AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Info, TEXT("ItemSpawn palette category should generally be Spawns."));
			}
			break;
		}
		case EGridLevelObjectType::Item:
		{
			if (!Category.IsNone() && !IsPaletteCategory(*this, TEXT("Items"))) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Info, TEXT("Item palette category should generally be Items."));
			if (!IsFloorPlacement(PlacementSurface)) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("Item Placement Surface should be Floor while items still use the world-object placement path."));
			const bool bHasDefaultItemDefinition = DefaultBehavior.Item.ItemDefinitionAsset || !DefaultBehavior.Item.ItemDefinitionId.IsNone();
			if (!ItemActorClass && !bHasDefaultItemDefinition) AddValidationMessage(OutMessages, EGridWorldObjectDefinitionValidationSeverity::Warning, TEXT("Item should generally define ItemActorClass or DefaultBehavior.Item."));
			break;
		}
		case EGridLevelObjectType::None:
		default:
			break;
	}

	for (const FGridWorldObjectDefinitionValidationMessage& Message : OutMessages)
	{
		if (Message.Severity == EGridWorldObjectDefinitionValidationSeverity::Error) return false;
	}
	return true;
}

bool UGridWorldObjectDefinitionAsset::IsValidDefinition() const
{
	TArray<FGridWorldObjectDefinitionValidationMessage> Messages;
	return ValidateDefinition(Messages);
}

FString UGridWorldObjectDefinitionAsset::GetValidationSummary() const
{
	TArray<FGridWorldObjectDefinitionValidationMessage> Messages;
	ValidateDefinition(Messages);
	int32 ErrorCount = 0;
	int32 WarningCount = 0;
	int32 InfoCount = 0;
	for (const FGridWorldObjectDefinitionValidationMessage& Message : Messages)
	{
		switch (Message.Severity)
		{
			case EGridWorldObjectDefinitionValidationSeverity::Error: ++ErrorCount; break;
			case EGridWorldObjectDefinitionValidationSeverity::Warning: ++WarningCount; break;
			case EGridWorldObjectDefinitionValidationSeverity::Info: ++InfoCount; break;
		}
	}
	const FString DefinitionName = DefinitionId.IsNone() ? GetName() : DefinitionId.ToString();
	FString Summary = FString::Printf(TEXT("Grid definition validation: %s | Errors=%d Warnings=%d Info=%d"), *DefinitionName, ErrorCount, WarningCount, InfoCount);
	for (const FGridWorldObjectDefinitionValidationMessage& Message : Messages)
	{
		Summary += FString::Printf(TEXT("\n- [%s] %s"), ToValidationSeverityText(Message.Severity), *Message.Message);
	}
	return Summary;
}

bool UGridWorldObjectDefinitionAsset::RequiresEdgePlacement() const
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

bool UGridWorldObjectDefinitionAsset::SupportsCenterPlacement() const
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
		case EGridLevelObjectType::StoryCompanion:
		case EGridLevelObjectType::CustomRecruiter:
			return true;
		default:
			return false;
	}
}

bool UGridWorldObjectDefinitionAsset::SupportsWallPlacement() const
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

bool UGridWorldObjectDefinitionAsset::RequiresRuntimeActorClass() const
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

bool UGridWorldObjectDefinitionAsset::AllowsInvisibleRuntimeObject() const
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

bool UGridWorldObjectDefinitionAsset::UsesWallPlacementParams() const { return PlacementSurface == EGridObjectPlacementKind::Wall; }
bool UGridWorldObjectDefinitionAsset::UsesCenterPlacementParams() const { return PlacementSurface == EGridObjectPlacementKind::Floor || PlacementSurface == EGridObjectPlacementKind::Ceiling; }
bool UGridWorldObjectDefinitionAsset::UsesReadableParams() const { return bIsReadable || ObjectCategory == EGridObjectCategory::Readable || (SupportedType == EGridLevelObjectType::Decoration && bIsReadable); }
bool UGridWorldObjectDefinitionAsset::UsesLightParams() const { return bIsLightSource || SupportedType == EGridLevelObjectType::Light || ObjectCategory == EGridObjectCategory::Light; }
bool UGridWorldObjectDefinitionAsset::UsesItemParams() const { return SupportedType == EGridLevelObjectType::Item || SupportedType == EGridLevelObjectType::ItemSpawn || ItemActorClass != nullptr; }
bool UGridWorldObjectDefinitionAsset::UsesReceptacleParams() const { return SupportedType == EGridLevelObjectType::Receptacle; }
bool UGridWorldObjectDefinitionAsset::UsesTeleporterParams() const { return SupportedType == EGridLevelObjectType::Teleporter; }
bool UGridWorldObjectDefinitionAsset::UsesButtonAnimationParams() const { return SupportedType == EGridLevelObjectType::Button; }

bool UGridWorldObjectDefinitionAsset::UsesTriggerParams() const
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

bool UGridWorldObjectDefinitionAsset::UsesMovingMeshParams() const
{
	if (UsesItemParams()) return true;
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

bool UGridWorldObjectDefinitionAsset::UsesFixedMeshParams() const
{
	return SupportedType == EGridLevelObjectType::Door || SupportedType == EGridLevelObjectType::Pit || UsesItemParams();
}

bool UGridWorldObjectDefinitionAsset::UsesRuntimeActorClass() const
{
	return RequiresRuntimeActorClass() || RuntimeActorClass != nullptr;
}

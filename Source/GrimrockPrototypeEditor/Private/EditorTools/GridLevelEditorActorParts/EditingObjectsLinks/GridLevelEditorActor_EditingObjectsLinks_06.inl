#include "Core/GridObjectInstanceBehavior.h"

const FGridLevelObjectData* AGridLevelEditorActor::GetSelectedObjectData() const
{
	return FindObjectById(LastSelectedObjectId);
}

bool AGridLevelEditorActor::SelectObjectById(FGuid ObjectId)
{
	if (!HasValidLevelAsset() || !ObjectId.IsValid())
	{
		ClearSelectedObjectState();
		return false;
	}

	const FGridLevelObjectData* Obj = FindObjectById(ObjectId);
	if (!Obj)
	{
		ClearSelectedObjectState();
		return false;
	}

	LastSelectedObjectId = Obj->ObjectId;

	SelectedCellX = Obj->CellX;
	SelectedCellY = Obj->CellY;
	SelectedEdge = Obj->Edge;

	PaintObjectType = Obj->Type;
	ObjectArchetypeId = Obj->ArchetypeId;
	SelectedArchetypeId = Obj->ArchetypeId;
	SelectedPaletteEntryId = Obj->PaletteEntryId;

	bObjectInitiallyEnabled = Obj->bInitiallyEnabled;
	bObjectInitiallyActive = Obj->bInitiallyActive;
	ObjectTag = Obj->Tag;
	ObjectNotes = Obj->Notes;

	const UGridObjectArchetypeAsset* Archetype = FindObjectArchetypeById(Obj->ArchetypeId);
	ObjectBehavior = GridObjectInstanceBehavior::Resolve(LevelAsset, *Obj, Archetype);
	if (Obj->Type == EGridLevelObjectType::Item)
	{
		ObjectBehavior.Item.ItemDefinitionAsset = Obj->ItemDefinitionAsset;
		ObjectBehavior.Item.ItemDefinitionId = Obj->ItemDefinitionId;
		ObjectBehavior.Item.DefaultReadableContentAsset = Obj->ReadableContentAsset;
		ObjectBehavior.Item.DefaultReadableContentId = Obj->ReadableContentId;
		ObjectBehavior.Item.DefaultReadTitleOverride = Obj->ReadTitleOverride;
		ObjectBehavior.Item.DefaultReadTextOverride = Obj->ReadTextOverride;
	}

	ResolvePreviewRuntimeActor();

	if (PreviewRuntimeActor)
	{
		PreviewRuntimeActor->SetEditorSelectedObject(LastSelectedObjectId);
	}
	return true;
}

bool AGridLevelEditorActor::TryGetObjectWorldLocationById(FGuid ObjectId, FVector& OutWorldLocation) const
{
	const FGridLevelObjectData* Obj = FindObjectById(ObjectId);
	return Obj ? TryGetObjectWorldLocation(*Obj, OutWorldLocation) : false;
}

bool AGridLevelEditorActor::GetObjectEditorWorldCenter(const FGridLevelObjectData& Obj, FVector& OutWorldCenter) const
{
	if (!HasValidLevelAsset())
	{
		return false;
	}

	const float CellSize = LevelAsset ? LevelAsset->CellSize : 200.f;
	if (CellSize <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	constexpr float FallbackCellHeight = 300.f;
	constexpr float DoorCenterHeight = FallbackCellHeight * 0.5f;
	constexpr float CeilingObjectInset = 32.f;

	auto ApplyDoorCenterHeight = [DoorCenterHeight](const FGridLevelObjectData& ObjectData, FVector& InOutLocation)
	{
		if (ObjectData.Type == EGridLevelObjectType::Door)
		{
			InOutLocation.Z += DoorCenterHeight;
		}
	};

	if (PreviewRuntimeActor)
	{
		FTransform PlacementTransform = FTransform::Identity;
		if (PreviewRuntimeActor->GetObjectPlacementTransform(Obj, PlacementTransform))
		{
			OutWorldCenter = PlacementTransform.GetLocation();
			ApplyDoorCenterHeight(Obj, OutWorldCenter);
			return true;
		}
	}

	FVector GridWorldOrigin = GetActorLocation();
	const FVector CellBase = GridWorldOrigin + FVector(Obj.CellX * CellSize, Obj.CellY * CellSize, 0.f);

	const UGridObjectArchetypeAsset* Archetype = FindObjectArchetypeById(Obj.ArchetypeId);
	const EGridObjectPlacementKind PlacementKind =
		Archetype ? Archetype->PlacementKind : (IsEdgePlacedObject(Obj) ? EGridObjectPlacementKind::Edge : EGridObjectPlacementKind::Center);

	if (Obj.Type == EGridLevelObjectType::Door)
	{
		switch (Obj.Edge)
		{
			case EGridEdge::North:
				OutWorldCenter = CellBase + FVector(CellSize * 0.5f, CellSize, DoorCenterHeight);
				return true;

			case EGridEdge::East:
				OutWorldCenter = CellBase + FVector(CellSize, CellSize * 0.5f, DoorCenterHeight);
				return true;

			case EGridEdge::South:
				OutWorldCenter = CellBase + FVector(CellSize * 0.5f, 0.f, DoorCenterHeight);
				return true;

			case EGridEdge::West:
				OutWorldCenter = CellBase + FVector(0.f, CellSize * 0.5f, DoorCenterHeight);
				return true;

			default:
				return false;
		}
	}
	if (Obj.Type == EGridLevelObjectType::Item && Obj.Edge != EGridEdge::None)
	{
		const float PlacementZOffset = Archetype ? Archetype->PlacementZOffset : 12.f;
		const float EdgeInset = Archetype ? FMath::Max(Archetype->WallInset, 18.f) : 18.f;

		switch (Obj.Edge)
		{
			case EGridEdge::North:
				OutWorldCenter = CellBase + FVector(CellSize * 0.5f, CellSize - EdgeInset, PlacementZOffset);
				return true;

			case EGridEdge::South:
				OutWorldCenter = CellBase + FVector(CellSize * 0.5f, EdgeInset, PlacementZOffset);
				return true;

			case EGridEdge::East:
				OutWorldCenter = CellBase + FVector(CellSize - EdgeInset, CellSize * 0.5f, PlacementZOffset);
				return true;

			case EGridEdge::West:
				OutWorldCenter = CellBase + FVector(EdgeInset, CellSize * 0.5f, PlacementZOffset);
				return true;

			default:
				return false;
		}
	}

	switch (PlacementKind)
	{
		case EGridObjectPlacementKind::Wall:
		case EGridObjectPlacementKind::Edge:
			if (Obj.Edge == EGridEdge::None)
			{
				return false;
			}
			{
				const float PlacementZOffset = Archetype ? Archetype->PlacementZOffset : 12.f;
				const float WallInset = Archetype ? Archetype->WallInset : 6.f;
				const float LocalOffsetAlongWall = Archetype ? Archetype->LocalOffsetAlongWall : 0.f;
				const float LocalOffsetVertical = Archetype ? Archetype->LocalOffsetVertical : 0.f;
				const float FinalZ = PlacementZOffset + LocalOffsetVertical;

				switch (Obj.Edge)
				{
					case EGridEdge::North:
						OutWorldCenter = CellBase + FVector((CellSize * 0.5f) + LocalOffsetAlongWall, CellSize - WallInset, FinalZ);
						return true;

					case EGridEdge::South:
						OutWorldCenter = CellBase + FVector((CellSize * 0.5f) - LocalOffsetAlongWall, WallInset, FinalZ);
						return true;

					case EGridEdge::East:
						OutWorldCenter = CellBase + FVector(CellSize - WallInset, (CellSize * 0.5f) - LocalOffsetAlongWall, FinalZ);
						return true;

					case EGridEdge::West:
						OutWorldCenter = CellBase + FVector(WallInset, (CellSize * 0.5f) + LocalOffsetAlongWall, FinalZ);
						return true;

					default:
						return false;
				}
			}

		case EGridObjectPlacementKind::Ceiling:
		{
			const float PlacementZOffset = Archetype ? Archetype->PlacementZOffset : FallbackCellHeight - CeilingObjectInset;
			OutWorldCenter = CellBase + FVector(CellSize * 0.5f, CellSize * 0.5f, PlacementZOffset);
			return true;
		}

		case EGridObjectPlacementKind::Center:
		case EGridObjectPlacementKind::Floor:
		default:
		{
			const float PlacementZOffset = Archetype ? Archetype->PlacementZOffset : 12.f;
			OutWorldCenter = CellBase + FVector(CellSize * 0.5f, CellSize * 0.5f, PlacementZOffset);
			return true;
		}
	}
}

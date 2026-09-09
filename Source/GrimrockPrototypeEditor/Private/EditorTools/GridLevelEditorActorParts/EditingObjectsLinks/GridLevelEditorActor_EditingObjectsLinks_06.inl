#include "Core/GridObjectInstanceBehavior.h"
#include "Runtime/GridPlacementTransformResolver.h"


bool AGridLevelEditorActor::SelectObjectById(FGuid ObjectId)
{
	if (!HasValidLevelAsset() || !ObjectId.IsValid())
	{
		ClearSelectedObjectState();
		return false;
	}

	int32 CellX, CellY;
	EGridEdge Edge;
	if (!LevelAsset->TryGetTypedPlacementLocation(ObjectId, CellX, CellY, Edge))
	{
		ClearSelectedObjectState();
		return false;
	}

	LastSelectedObjectId = ObjectId;
	SelectedCellX = CellX;
	SelectedCellY = CellY;
	SelectedEdge = Edge;
	PaintObjectType = LevelAsset->GetTypedPlacementType(ObjectId);
	ObjectArchetypeId = NAME_None;
	SelectedArchetypeId = NAME_None;
	bObjectInitiallyActive = false;
	ObjectBehavior = FGridObjectBehaviorParams();
	const auto ReadAuthoring = [this](const auto& Placement)
	{
		SelectedPaletteEntryId = Placement.PaletteEntryId;
		bObjectInitiallyEnabled = Placement.bInitiallyEnabled;
		ObjectTag = Placement.Tag;
		ObjectNotes = Placement.Notes;
	};
	if (const FGridWorldObjectInstance* WorldObjectInstance = LevelAsset->FindWorldObjectInstanceById(ObjectId))
	{
		ReadAuthoring(*WorldObjectInstance);
		ObjectArchetypeId = WorldObjectInstance->WorldObjectDefinitionId;
		SelectedArchetypeId = ObjectArchetypeId;
		bObjectInitiallyActive = WorldObjectInstance->bInitiallyActive;
		ObjectBehavior = GridObjectInstanceBehavior::Resolve(*WorldObjectInstance, FindObjectArchetypeById(ObjectArchetypeId));
	}
	else if (const FGridLooseItemInstance* LooseItemInstance = LevelAsset->FindLooseItemInstanceById(ObjectId))
	{
		ReadAuthoring(*LooseItemInstance);
		ObjectBehavior.Item.ItemDefinitionAsset = LooseItemInstance->ItemDefinition;
		ObjectBehavior.Item.DefaultReadableContentAsset = LooseItemInstance->ReadableContentAsset;
		ObjectBehavior.Item.DefaultReadableContentId = LooseItemInstance->ReadableContentId;
		ObjectBehavior.Item.DefaultReadTitleOverride = LooseItemInstance->ReadTitleOverride;
		ObjectBehavior.Item.DefaultReadTextOverride = LooseItemInstance->ReadTextOverride;
	}
	else if (const FGridMonsterSpawnInstance* MonsterSpawn = LevelAsset->FindMonsterSpawnInstanceById(ObjectId))
	{
		ReadAuthoring(*MonsterSpawn);
	}
	else if (const FGridItemSpawnInstance* ItemSpawn = LevelAsset->FindItemSpawnInstanceById(ObjectId))
	{
		ReadAuthoring(*ItemSpawn);
		ObjectBehavior.Item.ItemDefinitionAsset = ItemSpawn->ItemDefinition;
	}
	else if (const FGridLogicObjectInstance* LogicInstance = LevelAsset->FindLogicObjectInstanceById(ObjectId))
	{
		ReadAuthoring(*LogicInstance);
		bObjectInitiallyActive = LogicInstance->bInitiallyActive;
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
	if (!HasValidLevelAsset() || !ObjectId.IsValid())
	{
		return false;
	}

	int32 CellX = INDEX_NONE;
	int32 CellY = INDEX_NONE;
	EGridEdge Edge = EGridEdge::None;
	if (!LevelAsset->TryGetTypedPlacementLocation(ObjectId, CellX, CellY, Edge))
	{
		return false;
	}

	const float CellSize = LevelAsset->CellSize;
	const FVector GridWorldOrigin = PreviewRuntimeActor
		? PreviewRuntimeActor->GetActorLocation() + PreviewRuntimeActor->GridOrigin
		: GetActorLocation();
	const FVector CellCenter =
		GridWorldOrigin + FVector((CellX * CellSize) + (CellSize * 0.5f), (CellY * CellSize) + (CellSize * 0.5f), 12.f);

	bool bUsesEdge = false;
	if (const FGridWorldObjectInstance* WorldObjectInstance = LevelAsset->FindWorldObjectInstanceById(ObjectId))
	{
		bUsesEdge = IsEdgePlacedObject(WorldObjectInstance->Type, WorldObjectInstance->WorldObjectDefinitionId);
	}
	else if (const FGridLooseItemInstance* LooseItemInstance = LevelAsset->FindLooseItemInstanceById(ObjectId))
	{
		bUsesEdge = LooseItemInstance->SurfaceSide != EGridEdge::None;
	}

	if (bUsesEdge)
	{
		switch (Edge)
		{
			case EGridEdge::North:
				OutWorldLocation = CellCenter + FVector(0.f, CellSize * 0.5f, 0.f);
				return true;
			case EGridEdge::East:
				OutWorldLocation = CellCenter + FVector(CellSize * 0.5f, 0.f, 0.f);
				return true;
			case EGridEdge::South:
				OutWorldLocation = CellCenter + FVector(0.f, -CellSize * 0.5f, 0.f);
				return true;
			case EGridEdge::West:
				OutWorldLocation = CellCenter + FVector(-CellSize * 0.5f, 0.f, 0.f);
				return true;
			case EGridEdge::None:
			default:
				break;
		}
	}

	OutWorldLocation = CellCenter;
	return true;
}

bool AGridLevelEditorActor::GetObjectEditorWorldCenter(const FGuid& ObjectId, FVector& OutWorldCenter) const
{
	if (!HasValidLevelAsset() || !ObjectId.IsValid())
	{
		return false;
	}

	constexpr float FallbackCellHeight = 300.f;
	constexpr float DoorCenterHeight = FallbackCellHeight * 0.5f;
	constexpr float CeilingObjectInset = 32.f;

	if (const FGridWorldObjectInstance* Instance = LevelAsset->FindWorldObjectInstanceById(ObjectId))
	{
		if (PreviewRuntimeActor)
		{
			FTransform PlacementTransform = FTransform::Identity;
			if (GridPlacementTransformResolver::ResolveWorldObject(*PreviewRuntimeActor, *Instance, PlacementTransform))
			{
				OutWorldCenter = PlacementTransform.GetLocation();
				if (Instance->Type == EGridLevelObjectType::Door)
				{
					OutWorldCenter.Z += DoorCenterHeight;
				}
				return true;
			}
		}

		const float CellSize = LevelAsset->CellSize;
		const FVector GridWorldOrigin = PreviewRuntimeActor
			? PreviewRuntimeActor->GetActorLocation() + PreviewRuntimeActor->GridOrigin
			: GetActorLocation();
		const FVector CellBase = GridWorldOrigin + FVector(Instance->CellX * CellSize, Instance->CellY * CellSize, 0.f);
		const UGridObjectArchetypeAsset* Archetype = FindObjectArchetypeById(Instance->WorldObjectDefinitionId);
		const EGridObjectPlacementKind PlacementKind =
			Archetype ? Archetype->PlacementKind
					  : (IsEdgePlacedObject(Instance->Type, Instance->WorldObjectDefinitionId)
								? EGridObjectPlacementKind::Edge
								: EGridObjectPlacementKind::Center);

		if (Instance->Type == EGridLevelObjectType::Door)
		{
			switch (Instance->WallSide)
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
				case EGridEdge::None:
				default:
					return false;
			}
		}

		switch (PlacementKind)
		{
			case EGridObjectPlacementKind::Wall:
			case EGridObjectPlacementKind::Edge:
			{
				if (Instance->WallSide == EGridEdge::None)
				{
					return false;
				}

				const float PlacementZOffset = Archetype ? Archetype->PlacementZOffset : 12.f;
				const float WallInset = Archetype ? Archetype->WallInset : 6.f;
				const float LocalOffsetAlongWall = Archetype ? Archetype->LocalOffsetAlongWall : 0.f;
				const float LocalOffsetVertical = Archetype ? Archetype->LocalOffsetVertical : 0.f;
				const float FinalZ = PlacementZOffset + LocalOffsetVertical;

				switch (Instance->WallSide)
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
					case EGridEdge::None:
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

	if (const FGridLooseItemInstance* Instance = LevelAsset->FindLooseItemInstanceById(ObjectId))
	{
		if (PreviewRuntimeActor)
		{
			FTransform PlacementTransform = FTransform::Identity;
			if (GridPlacementTransformResolver::ResolveLooseItem(*PreviewRuntimeActor, *Instance, PlacementTransform))
			{
				OutWorldCenter = PlacementTransform.GetLocation();
				return true;
			}
		}

		const float CellSize = LevelAsset->CellSize;
		const FVector GridWorldOrigin = PreviewRuntimeActor
			? PreviewRuntimeActor->GetActorLocation() + PreviewRuntimeActor->GridOrigin
			: GetActorLocation();
		const FVector CellBase = GridWorldOrigin + FVector(Instance->CellX * CellSize, Instance->CellY * CellSize, 0.f);
		const float PlacementZOffset = 12.f;
		const float EdgeInset = 18.f;

		switch (Instance->SurfaceSide)
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
			case EGridEdge::None:
			default:
				OutWorldCenter = CellBase + FVector(CellSize * 0.5f, CellSize * 0.5f, PlacementZOffset);
				return true;
		}
	}

	return TryGetObjectWorldLocationById(ObjectId, OutWorldCenter);
}

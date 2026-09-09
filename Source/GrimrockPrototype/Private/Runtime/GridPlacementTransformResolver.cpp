#include "Runtime/GridPlacementTransformResolver.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace
{
	constexpr float CurrentWorldObjectCeilingPlaneHeight = 200.0f;

	FVector GetCellOrigin(const AGridLevelRuntimeActor& RuntimeActor, int32 CellX, int32 CellY, float ZOffset)
	{
		const float CellSize = RuntimeActor.LevelAsset ? RuntimeActor.LevelAsset->CellSize : 200.0f;
		return RuntimeActor.GetCellCenterWorld(CellX, CellY, ZOffset) - FVector(CellSize * 0.5f, CellSize * 0.5f, 0.0f);
	}

	bool ResolveDoorEdge(const AGridLevelRuntimeActor& RuntimeActor, int32 CellX, int32 CellY, EGridEdge Edge, FTransform& OutTransform)
	{
		if (!RuntimeActor.LevelAsset || Edge == EGridEdge::None)
		{
			return false;
		}

		const float CellSize = RuntimeActor.LevelAsset->CellSize;
		const FVector Base = GetCellOrigin(RuntimeActor, CellX, CellY, 0.0f);
		FVector Position = Base;
		FRotator Rotation = FRotator::ZeroRotator;
		switch (Edge)
		{
			case EGridEdge::North:
				Position = Base + FVector(CellSize * 0.5f, CellSize, 0.0f);
				Rotation = FRotator(0.0f, 0.0f, 0.0f);
				break;
			case EGridEdge::East:
				Position = Base + FVector(CellSize, CellSize * 0.5f, 0.0f);
				Rotation = FRotator(0.0f, -90.0f, 0.0f);
				break;
			case EGridEdge::South:
				Position = Base + FVector(CellSize * 0.5f, 0.0f, 0.0f);
				Rotation = FRotator(0.0f, 180.0f, 0.0f);
				break;
			case EGridEdge::West:
				Position = Base + FVector(0.0f, CellSize * 0.5f, 0.0f);
				Rotation = FRotator(0.0f, 90.0f, 0.0f);
				break;
			default:
				return false;
		}

		OutTransform = FTransform(Rotation, Position, FVector::OneVector);
		return true;
	}

	bool ResolveFloorEdge(const AGridLevelRuntimeActor& RuntimeActor, int32 CellX, int32 CellY, EGridEdge Edge, float LocalYaw, float ZOffset,
		float EdgeInset, FTransform& OutTransform)
	{
		if (!RuntimeActor.LevelAsset || Edge == EGridEdge::None)
		{
			return false;
		}

		const float CellSize = RuntimeActor.LevelAsset->CellSize;
		const FVector Base = GetCellOrigin(RuntimeActor, CellX, CellY, ZOffset);
		FVector Position = Base + FVector(CellSize * 0.5f, CellSize * 0.5f, 0.0f);
		FRotator Rotation = FRotator::ZeroRotator;
		switch (Edge)
		{
			case EGridEdge::North:
				Position = Base + FVector(CellSize * 0.5f, CellSize - EdgeInset, 0.0f);
				Rotation = FRotator(0.0f, 0.0f, 0.0f);
				break;
			case EGridEdge::South:
				Position = Base + FVector(CellSize * 0.5f, EdgeInset, 0.0f);
				Rotation = FRotator(0.0f, 180.0f, 0.0f);
				break;
			case EGridEdge::East:
				Position = Base + FVector(CellSize - EdgeInset, CellSize * 0.5f, 0.0f);
				Rotation = FRotator(0.0f, 90.0f, 0.0f);
				break;
			case EGridEdge::West:
				Position = Base + FVector(EdgeInset, CellSize * 0.5f, 0.0f);
				Rotation = FRotator(0.0f, -90.0f, 0.0f);
				break;
			default:
				return false;
		}

		Rotation.Yaw += LocalYaw;
		OutTransform = FTransform(Rotation, Position, FVector::OneVector);
		return true;
	}

	bool ResolveWallMounted(const AGridLevelRuntimeActor& RuntimeActor, int32 CellX, int32 CellY, EGridEdge Edge, float ZOffset, float WallInset,
		float LocalOffsetAlongWall, float LocalOffsetVertical, FTransform& OutTransform)
	{
		if (!RuntimeActor.LevelAsset || Edge == EGridEdge::None)
		{
			return false;
		}

		const float CellSize = RuntimeActor.LevelAsset->CellSize;
		const FVector Base = GetCellOrigin(RuntimeActor, CellX, CellY, ZOffset + LocalOffsetVertical);
		FVector Position = Base;
		FRotator Rotation = FRotator::ZeroRotator;
		switch (Edge)
		{
			case EGridEdge::North:
				Position = Base + FVector((CellSize * 0.5f) + LocalOffsetAlongWall, CellSize - WallInset, 0.0f);
				Rotation = FRotator(0.0f, 90.0f, 0.0f);
				break;
			case EGridEdge::South:
				Position = Base + FVector((CellSize * 0.5f) - LocalOffsetAlongWall, WallInset, 0.0f);
				Rotation = FRotator(0.0f, -90.0f, 0.0f);
				break;
			case EGridEdge::East:
				Position = Base + FVector(CellSize - WallInset, (CellSize * 0.5f) - LocalOffsetAlongWall, 0.0f);
				Rotation = FRotator(0.0f, 0.0f, 0.0f);
				break;
			case EGridEdge::West:
				Position = Base + FVector(WallInset, (CellSize * 0.5f) + LocalOffsetAlongWall, 0.0f);
				Rotation = FRotator(0.0f, 180.0f, 0.0f);
				break;
			default:
				return false;
		}

		OutTransform = FTransform(Rotation, Position, FVector::OneVector);
		return true;
	}

	bool ResolveCentered(const AGridLevelRuntimeActor& RuntimeActor, int32 CellX, int32 CellY, float LocalYaw, float ZOffset, FTransform& OutTransform)
	{
		if (!RuntimeActor.LevelAsset)
		{
			return false;
		}

		OutTransform = FTransform(FRotator(0.0f, LocalYaw, 0.0f), RuntimeActor.GetCellCenterWorld(CellX, CellY, ZOffset), FVector::OneVector);
		return true;
	}
}

namespace GridPlacementTransformResolver
{
	bool ResolveWorldObject(const AGridLevelRuntimeActor& RuntimeActor, const FGridWorldObjectInstance& Instance, FTransform& OutTransform)
	{
		if (!RuntimeActor.LevelAsset || !RuntimeActor.LevelAsset->IsValidCoord(Instance.CellX, Instance.CellY))
		{
			return false;
		}

		const UGridWorldObjectDefinitionAsset* Definition = RuntimeActor.FindWorldObjectDefinition(Instance.WorldObjectDefinitionId);
		if (!Definition)
		{
			return false;
		}
		if (Instance.Type == EGridLevelObjectType::Door)
		{
			return ResolveDoorEdge(RuntimeActor, Instance.CellX, Instance.CellY, Instance.WallSide, OutTransform);
		}
		if (Definition->PlacementSurface == EGridObjectPlacementKind::Wall)
		{
			return ResolveWallMounted(RuntimeActor, Instance.CellX, Instance.CellY, Instance.WallSide, Definition->DefaultLocalPosition.V,
				Definition->DefaultLocalPosition.N, Definition->DefaultLocalPosition.U, 0.0f, OutTransform);
		}
		if (Definition->PlacementSurface == EGridObjectPlacementKind::Floor || Definition->PlacementSurface == EGridObjectPlacementKind::Ceiling)
		{
			const float LocalYaw = Instance.bHasLocalTransformOverride ? Instance.LocalTransformOverride.Rotator().Yaw : 0.0f;
			// Preserve the current centered U/V behavior and the fixed 200 cm ceiling plane.
			const float ZOffset = Definition->PlacementSurface == EGridObjectPlacementKind::Ceiling
				? CurrentWorldObjectCeilingPlaneHeight - Definition->DefaultLocalPosition.N
				: Definition->DefaultLocalPosition.N;
			return ResolveCentered(RuntimeActor, Instance.CellX, Instance.CellY, LocalYaw, ZOffset, OutTransform);
		}
		return false;
	}

	bool ResolveLooseItem(const AGridLevelRuntimeActor& RuntimeActor, const FGridLooseItemInstance& Instance, FTransform& OutTransform)
	{
		if (!RuntimeActor.LevelAsset || !RuntimeActor.LevelAsset->IsValidCoord(Instance.CellX, Instance.CellY))
		{
			return false;
		}
		if (Instance.SurfaceSide != EGridEdge::None)
		{
			return ResolveFloorEdge(RuntimeActor, Instance.CellX, Instance.CellY, Instance.SurfaceSide, Instance.LocalYaw, 12.0f, 18.0f, OutTransform);
		}
		return ResolveCentered(RuntimeActor, Instance.CellX, Instance.CellY, Instance.LocalYaw, 12.0f, OutTransform);
	}
}

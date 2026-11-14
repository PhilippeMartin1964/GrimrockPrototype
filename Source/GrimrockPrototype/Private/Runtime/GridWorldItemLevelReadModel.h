#pragma once

#include "GridReceptacleItemMetadataReadModel.h"

/**
 * Read-only world item level bridge, reusing 06.3AI identity validation.
 * The two named entry points prohibit an accidental default-level promotion
 * during world drops, projectile landings and pending pit transfers.
 */
struct FGridWorldItemLevelReadModel
{
	/** Unmaterialized LevelAsset placement: authored ARM N0 may inherit its
	 * author's DefaultWeaponItemLevel exactly once. */
	static FGridReceptacleItemMetadataReadView ResolveAuthoredPlacement(
		const UGridItemSubsystem* Registry,
		const UGridItemDefinitionAsset* Definition, FName ItemId, int32 AuthoredLevel)
	{
		return FGridReceptacleItemMetadataReadModel::Resolve(
			Registry, Definition, ItemId, AuthoredLevel, true);
	}

	/** Already-existing item: registered ARM N0 is invalid, ammo N0 is real. */
	static FGridReceptacleItemMetadataReadView ResolvePersistedInstance(
		const UGridItemSubsystem* Registry,
		const UGridItemDefinitionAsset* Definition, FName ItemId, int32 FixedLevel)
	{
		return FGridReceptacleItemMetadataReadModel::Resolve(
			Registry, Definition, ItemId, FixedLevel, false);
	}
};

#pragma once

#include "GridReceptacleItemMetadataReadModel.h"

/**
 * Read-only world item level bridge. Both authoring and persistence now
 * require exact physical levels for registered IDs (5G.6); the two entry
 * points remain until the historical reader signatures are retired.
 */
struct FGridWorldItemLevelReadModel
{
	/** LevelAsset placement with a materialized physical level. */
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

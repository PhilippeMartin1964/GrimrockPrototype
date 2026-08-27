#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"

class URPGCharacterPortraitSetAsset;
class URPGClassAsset;
class URPGClassVisualAsset;
class URPGRaceAsset;

/**
 * TD07.3.4.2 canonical resolver for RPG authoring definitions.
 * Business IDs are the only persistent identities; UObject paths/names are authoring details.
 */
struct GRIMROCKPROTOTYPE_API FRPGAuthoringIdentityResolver
{
	static const FPrimaryAssetType ClassPrimaryAssetType;
	static const FPrimaryAssetType RacePrimaryAssetType;
	static const FPrimaryAssetType ClassVisualPrimaryAssetType;
	static const FPrimaryAssetType PortraitSetPrimaryAssetType;

	static FPrimaryAssetId MakeClassPrimaryAssetId(FName ClassId);
	static FPrimaryAssetId MakeRacePrimaryAssetId(FName RaceId);
	static FPrimaryAssetId MakeClassVisualPrimaryAssetId(FName ClassId);
	static FPrimaryAssetId MakePortraitSetPrimaryAssetId(FName RaceId);

	static bool IsMatchingClassDefinition(FName ClassId, const URPGClassAsset* Definition);
	static bool IsMatchingRaceDefinition(FName RaceId, const URPGRaceAsset* Definition);
	static bool IsMatchingClassVisual(FName ClassId, const URPGClassVisualAsset* Definition);
	static bool IsMatchingPortraitSet(FName RaceId, const URPGCharacterPortraitSetAsset* Definition);

	/** TD07.3.4.3 transient weak caches; never serialized and never authoritative. */
	static bool RememberClassDefinition(URPGClassAsset* Definition);
	static bool RememberRaceDefinition(URPGRaceAsset* Definition);
	static bool RememberClassVisual(URPGClassVisualAsset* Definition);
	static bool RememberPortraitSet(URPGCharacterPortraitSetAsset* Definition);
	static void ResetRuntimeCache();

	static URPGClassAsset* ResolveClassById(FName ClassId);
	static URPGRaceAsset* ResolveRaceById(FName RaceId);
	static URPGClassVisualAsset* ResolveClassVisualByClassId(FName ClassId);
	static URPGCharacterPortraitSetAsset* ResolvePortraitSetByRaceId(FName RaceId);
};

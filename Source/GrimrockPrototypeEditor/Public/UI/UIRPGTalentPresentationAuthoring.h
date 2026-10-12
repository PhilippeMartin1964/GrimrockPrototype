#pragma once

#include "CoreMinimal.h"

class URPGTalentPresentationAsset;

/** Canonical editor-only authoring for the UI-RPG02 production presentation catalog. */
struct GRIMROCKPROTOTYPEEDITOR_API FUIRPGTalentPresentationAuthoring
{
	static const TCHAR* PackageName();
	static const TCHAR* AssetName();
	static const TCHAR* ObjectPath();

	/** Rebuilds canonical structure/colors while preserving serialized branch/node text by stable ID. */
	static void ConfigureCatalog(URPGTalentPresentationAsset& Catalog);
};

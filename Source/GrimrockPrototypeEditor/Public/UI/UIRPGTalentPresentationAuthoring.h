#pragma once

#include "CoreMinimal.h"

class URPGTalentPresentationAsset;

/** Canonical editor-only authoring for the UI-RPG02 production presentation catalog. */
struct GRIMROCKPROTOTYPEEDITOR_API FUIRPGTalentPresentationAuthoring
{
	static const TCHAR* PackageName();
	static const TCHAR* AssetName();
	static const TCHAR* ObjectPath();

	/** Replaces presentation data with the canonical six-class / eighteen-branch catalog. */
	static void ConfigureCatalog(URPGTalentPresentationAsset& Catalog);
};

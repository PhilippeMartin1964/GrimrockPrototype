#pragma once

#include "CoreMinimal.h"

class URPGClassAsset;

// Storage only: callers retain authoring validation, Modify(), transactions and
// permission to apply changes. These two entry points preserve their diagnostics.
namespace GridEditorAssetSave
{
	bool SaveAuthoredAsset(UObject* Asset, FString& OutError);
	bool SaveClassAsset(URPGClassAsset* Asset, FString& OutError);
}

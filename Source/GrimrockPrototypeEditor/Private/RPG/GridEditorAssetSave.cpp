#include "RPG/GridEditorAssetSave.h"

#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "RPG/RPGClassAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
	bool SaveAsset(UObject* Asset, FString& OutError, bool bClassDiagnostics)
	{
		if (!IsValid(Asset))
		{
			OutError = bClassDiagnostics ? TEXT("Cannot save a null class asset.") : TEXT("Cannot save a null authored asset.");
			return false;
		}
		UPackage* Package = Asset->GetOutermost();
		if (!Package)
		{
			OutError = FString::Printf(bClassDiagnostics ? TEXT("Class '%s' has no package.") : TEXT("Asset '%s' has no package."), *GetNameSafe(Asset));
			return false;
		}
		Package->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		if (!UPackage::SavePackage(Package, Asset, *Filename, SaveArgs))
		{
			OutError = bClassDiagnostics ? FString::Printf(TEXT("Failed to save '%s'."), *Asset->GetPathName())
										 : FString::Printf(TEXT("Failed to save '%s' to '%s'."), *Asset->GetPathName(), *Filename);
			return false;
		}
		return true;
	}
}

bool GridEditorAssetSave::SaveAuthoredAsset(UObject* Asset, FString& OutError)
{
	return SaveAsset(Asset, OutError, false);
}

bool GridEditorAssetSave::SaveClassAsset(URPGClassAsset* Asset, FString& OutError)
{
	return SaveAsset(Asset, OutError, true);
}

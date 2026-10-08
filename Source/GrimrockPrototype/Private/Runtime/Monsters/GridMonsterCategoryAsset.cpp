#include "Runtime/Monsters/GridMonsterCategoryAsset.h"

#include "Engine/AssetManager.h"

namespace GridMonsterCategoryAssetPrivate
{
	const FPrimaryAssetType PrimaryAssetType(TEXT("GridMonsterCategory"));
	const TCHAR* ProductionFolderPath = TEXT("/Game/GrimrockPrototype/Monsters/Categories");
}

FPrimaryAssetId UGridMonsterCategoryAsset::GetPrimaryAssetId() const
{
	if (CategoryId.IsNone())
	{
		return Super::GetPrimaryAssetId();
	}
	return FPrimaryAssetId(GridMonsterCategoryAssetPrivate::PrimaryAssetType, CategoryId);
}

bool UGridMonsterCategoryAsset::IsValidDefinition() const
{
	FString Error;
	return ValidateDefinition(Error);
}

bool UGridMonsterCategoryAsset::ValidateDefinition(FString& OutError) const
{
	TArray<FString> Errors;
	if (CategoryId.IsNone())
	{
		Errors.Add(TEXT("CategoryId must not be None."));
	}
	if (DisplayName.IsEmpty())
	{
		Errors.Add(TEXT("DisplayName must not be empty."));
	}
	OutError = FString::Join(Errors, TEXT("\n"));
	return Errors.IsEmpty();
}

const UGridMonsterCategoryAsset* UGridMonsterCategoryAsset::ResolveByCategoryId(FName CategoryId)
{
	if (CategoryId.IsNone())
	{
		return nullptr;
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	TArray<FString> SearchPaths;
	SearchPaths.Add(GridMonsterCategoryAssetPrivate::ProductionFolderPath);
	AssetManager.ScanPathsForPrimaryAssets(
		GridMonsterCategoryAssetPrivate::PrimaryAssetType,
		SearchPaths,
		UGridMonsterCategoryAsset::StaticClass(),
		false,
		false,
		true);

	const FPrimaryAssetId AssetId(GridMonsterCategoryAssetPrivate::PrimaryAssetType, CategoryId);
	UGridMonsterCategoryAsset* Definition = AssetManager.GetPrimaryAssetObject<UGridMonsterCategoryAsset>(AssetId);
	if (!IsValid(Definition))
	{
		const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(AssetId);
		if (AssetPath.IsValid())
		{
			Definition = Cast<UGridMonsterCategoryAsset>(AssetPath.TryLoad());
		}
	}

	return IsValid(Definition) && Definition->IsValidDefinition() && Definition->CategoryId == CategoryId
		? Definition
		: nullptr;
}

const TCHAR* UGridMonsterCategoryAsset::ProductionFolder()
{
	return GridMonsterCategoryAssetPrivate::ProductionFolderPath;
}

#include "RPG/RPGAuthoringIdentityResolver.h"

#include "Engine/AssetManager.h"
#include "RPG/RPGCharacterPortraitSetAsset.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassVisualAsset.h"
#include "RPG/RPGRaceAsset.h"

const FPrimaryAssetType FRPGAuthoringIdentityResolver::ClassPrimaryAssetType(TEXT("RPGClass"));
const FPrimaryAssetType FRPGAuthoringIdentityResolver::RacePrimaryAssetType(TEXT("RPGRace"));
const FPrimaryAssetType FRPGAuthoringIdentityResolver::ClassVisualPrimaryAssetType(TEXT("RPGClassVisual"));
const FPrimaryAssetType FRPGAuthoringIdentityResolver::PortraitSetPrimaryAssetType(TEXT("RPGPortraitSet"));

namespace RPGAuthoringIdentityResolverPrivate
{
	template <typename T>
	T* ResolveCanonicalAsset(const FPrimaryAssetId& PrimaryAssetId, UClass* AssetClass)
	{
		if (!PrimaryAssetId.IsValid() || !AssetClass)
		{
			return nullptr;
		}

		UAssetManager& AssetManager = UAssetManager::Get();
		T* Definition = AssetManager.GetPrimaryAssetObject<T>(PrimaryAssetId);
		if (!IsValid(Definition))
		{
			FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(PrimaryAssetId);
			if (!AssetPath.IsValid())
			{
				TArray<FString> SearchPaths;
				SearchPaths.Add(TEXT("/Game"));
				AssetManager.ScanPathsForPrimaryAssets(
					PrimaryAssetId.PrimaryAssetType, SearchPaths, AssetClass, false, false, true);
				AssetPath = AssetManager.GetPrimaryAssetPath(PrimaryAssetId);
			}
			if (AssetPath.IsValid())
			{
				Definition = Cast<T>(AssetPath.TryLoad());
			}
		}

		return IsValid(Definition) ? Definition : nullptr;
	}
}

FPrimaryAssetId FRPGAuthoringIdentityResolver::MakeClassPrimaryAssetId(FName ClassId)
{
	return ClassId.IsNone() ? FPrimaryAssetId() : FPrimaryAssetId(ClassPrimaryAssetType, ClassId);
}

FPrimaryAssetId FRPGAuthoringIdentityResolver::MakeRacePrimaryAssetId(FName RaceId)
{
	return RaceId.IsNone() ? FPrimaryAssetId() : FPrimaryAssetId(RacePrimaryAssetType, RaceId);
}

FPrimaryAssetId FRPGAuthoringIdentityResolver::MakeClassVisualPrimaryAssetId(FName ClassId)
{
	return ClassId.IsNone() ? FPrimaryAssetId() : FPrimaryAssetId(ClassVisualPrimaryAssetType, ClassId);
}

FPrimaryAssetId FRPGAuthoringIdentityResolver::MakePortraitSetPrimaryAssetId(FName RaceId)
{
	return RaceId.IsNone() ? FPrimaryAssetId() : FPrimaryAssetId(PortraitSetPrimaryAssetType, RaceId);
}

bool FRPGAuthoringIdentityResolver::IsMatchingClassDefinition(FName ClassId, const URPGClassAsset* Definition)
{
	const FPrimaryAssetId ExpectedId = MakeClassPrimaryAssetId(ClassId);
	return ExpectedId.IsValid() && IsValid(Definition) && Definition->IsValidDefinition() &&
		Definition->ClassId == ClassId && Definition->GetPrimaryAssetId() == ExpectedId;
}

bool FRPGAuthoringIdentityResolver::IsMatchingRaceDefinition(FName RaceId, const URPGRaceAsset* Definition)
{
	const FPrimaryAssetId ExpectedId = MakeRacePrimaryAssetId(RaceId);
	return ExpectedId.IsValid() && IsValid(Definition) && Definition->IsValidDefinition() &&
		Definition->RaceId == RaceId && Definition->GetPrimaryAssetId() == ExpectedId;
}

bool FRPGAuthoringIdentityResolver::IsMatchingClassVisual(FName ClassId, const URPGClassVisualAsset* Definition)
{
	const FPrimaryAssetId ExpectedId = MakeClassVisualPrimaryAssetId(ClassId);
	return ExpectedId.IsValid() && IsValid(Definition) && Definition->IsValidDefinition() &&
		Definition->ClassId == ClassId && Definition->GetPrimaryAssetId() == ExpectedId;
}

bool FRPGAuthoringIdentityResolver::IsMatchingPortraitSet(FName RaceId, const URPGCharacterPortraitSetAsset* Definition)
{
	const FPrimaryAssetId ExpectedId = MakePortraitSetPrimaryAssetId(RaceId);
	return ExpectedId.IsValid() && IsValid(Definition) && Definition->IsValidDefinition() &&
		Definition->RaceId == RaceId && Definition->GetPrimaryAssetId() == ExpectedId;
}

URPGClassAsset* FRPGAuthoringIdentityResolver::ResolveClassById(FName ClassId)
{
	URPGClassAsset* Definition =
		RPGAuthoringIdentityResolverPrivate::ResolveCanonicalAsset<URPGClassAsset>(MakeClassPrimaryAssetId(ClassId), URPGClassAsset::StaticClass());
	return IsMatchingClassDefinition(ClassId, Definition) ? Definition : nullptr;
}

URPGRaceAsset* FRPGAuthoringIdentityResolver::ResolveRaceById(FName RaceId)
{
	URPGRaceAsset* Definition =
		RPGAuthoringIdentityResolverPrivate::ResolveCanonicalAsset<URPGRaceAsset>(MakeRacePrimaryAssetId(RaceId), URPGRaceAsset::StaticClass());
	return IsMatchingRaceDefinition(RaceId, Definition) ? Definition : nullptr;
}

URPGClassVisualAsset* FRPGAuthoringIdentityResolver::ResolveClassVisualByClassId(FName ClassId)
{
	URPGClassVisualAsset* Definition =
		RPGAuthoringIdentityResolverPrivate::ResolveCanonicalAsset<URPGClassVisualAsset>(
			MakeClassVisualPrimaryAssetId(ClassId), URPGClassVisualAsset::StaticClass());
	return IsMatchingClassVisual(ClassId, Definition) ? Definition : nullptr;
}

URPGCharacterPortraitSetAsset* FRPGAuthoringIdentityResolver::ResolvePortraitSetByRaceId(FName RaceId)
{
	URPGCharacterPortraitSetAsset* Definition =
		RPGAuthoringIdentityResolverPrivate::ResolveCanonicalAsset<URPGCharacterPortraitSetAsset>(
			MakePortraitSetPrimaryAssetId(RaceId), URPGCharacterPortraitSetAsset::StaticClass());
	return IsMatchingPortraitSet(RaceId, Definition) ? Definition : nullptr;
}

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
	TMap<FName, TWeakObjectPtr<URPGClassAsset>> ClassCache;
	TMap<FName, TWeakObjectPtr<URPGRaceAsset>> RaceCache;
	TMap<FName, TWeakObjectPtr<URPGClassVisualAsset>> ClassVisualCache;
	TMap<FName, TWeakObjectPtr<URPGCharacterPortraitSetAsset>> PortraitSetCache;

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

bool FRPGAuthoringIdentityResolver::RememberClassDefinition(URPGClassAsset* Definition)
{
	if (!IsValid(Definition) || !IsMatchingClassDefinition(Definition->ClassId, Definition))
	{
		return false;
	}
	RPGAuthoringIdentityResolverPrivate::ClassCache.Add(Definition->ClassId, Definition);
	return true;
}

bool FRPGAuthoringIdentityResolver::RememberRaceDefinition(URPGRaceAsset* Definition)
{
	if (!IsValid(Definition) || !IsMatchingRaceDefinition(Definition->RaceId, Definition))
	{
		return false;
	}
	RPGAuthoringIdentityResolverPrivate::RaceCache.Add(Definition->RaceId, Definition);
	return true;
}

bool FRPGAuthoringIdentityResolver::RememberClassVisual(URPGClassVisualAsset* Definition)
{
	if (!IsValid(Definition) || !IsMatchingClassVisual(Definition->ClassId, Definition))
	{
		return false;
	}
	RPGAuthoringIdentityResolverPrivate::ClassVisualCache.Add(Definition->ClassId, Definition);
	return true;
}

bool FRPGAuthoringIdentityResolver::RememberPortraitSet(URPGCharacterPortraitSetAsset* Definition)
{
	if (!IsValid(Definition) || !IsMatchingPortraitSet(Definition->RaceId, Definition))
	{
		return false;
	}
	RPGAuthoringIdentityResolverPrivate::PortraitSetCache.Add(Definition->RaceId, Definition);
	return true;
}

void FRPGAuthoringIdentityResolver::ResetRuntimeCache()
{
	RPGAuthoringIdentityResolverPrivate::ClassCache.Reset();
	RPGAuthoringIdentityResolverPrivate::RaceCache.Reset();
	RPGAuthoringIdentityResolverPrivate::ClassVisualCache.Reset();
	RPGAuthoringIdentityResolverPrivate::PortraitSetCache.Reset();
}

URPGClassAsset* FRPGAuthoringIdentityResolver::ResolveClassById(FName ClassId)
{
	if (const TWeakObjectPtr<URPGClassAsset>* Cached = RPGAuthoringIdentityResolverPrivate::ClassCache.Find(ClassId))
	{
		if (URPGClassAsset* Definition = Cached->Get(); IsMatchingClassDefinition(ClassId, Definition))
		{
			return Definition;
		}
		RPGAuthoringIdentityResolverPrivate::ClassCache.Remove(ClassId);
	}

	URPGClassAsset* Definition =
		RPGAuthoringIdentityResolverPrivate::ResolveCanonicalAsset<URPGClassAsset>(MakeClassPrimaryAssetId(ClassId), URPGClassAsset::StaticClass());
	if (!IsMatchingClassDefinition(ClassId, Definition))
	{
		return nullptr;
	}
	RememberClassDefinition(Definition);
	return Definition;
}

URPGRaceAsset* FRPGAuthoringIdentityResolver::ResolveRaceById(FName RaceId)
{
	if (const TWeakObjectPtr<URPGRaceAsset>* Cached = RPGAuthoringIdentityResolverPrivate::RaceCache.Find(RaceId))
	{
		if (URPGRaceAsset* Definition = Cached->Get(); IsMatchingRaceDefinition(RaceId, Definition))
		{
			return Definition;
		}
		RPGAuthoringIdentityResolverPrivate::RaceCache.Remove(RaceId);
	}

	URPGRaceAsset* Definition =
		RPGAuthoringIdentityResolverPrivate::ResolveCanonicalAsset<URPGRaceAsset>(MakeRacePrimaryAssetId(RaceId), URPGRaceAsset::StaticClass());
	if (!IsMatchingRaceDefinition(RaceId, Definition))
	{
		return nullptr;
	}
	RememberRaceDefinition(Definition);
	return Definition;
}

URPGClassVisualAsset* FRPGAuthoringIdentityResolver::ResolveClassVisualByClassId(FName ClassId)
{
	if (const TWeakObjectPtr<URPGClassVisualAsset>* Cached = RPGAuthoringIdentityResolverPrivate::ClassVisualCache.Find(ClassId))
	{
		if (URPGClassVisualAsset* Definition = Cached->Get(); IsMatchingClassVisual(ClassId, Definition))
		{
			return Definition;
		}
		RPGAuthoringIdentityResolverPrivate::ClassVisualCache.Remove(ClassId);
	}

	URPGClassVisualAsset* Definition =
		RPGAuthoringIdentityResolverPrivate::ResolveCanonicalAsset<URPGClassVisualAsset>(
			MakeClassVisualPrimaryAssetId(ClassId), URPGClassVisualAsset::StaticClass());
	if (!IsMatchingClassVisual(ClassId, Definition))
	{
		return nullptr;
	}
	RememberClassVisual(Definition);
	return Definition;
}

URPGCharacterPortraitSetAsset* FRPGAuthoringIdentityResolver::ResolvePortraitSetByRaceId(FName RaceId)
{
	if (const TWeakObjectPtr<URPGCharacterPortraitSetAsset>* Cached = RPGAuthoringIdentityResolverPrivate::PortraitSetCache.Find(RaceId))
	{
		if (URPGCharacterPortraitSetAsset* Definition = Cached->Get(); IsMatchingPortraitSet(RaceId, Definition))
		{
			return Definition;
		}
		RPGAuthoringIdentityResolverPrivate::PortraitSetCache.Remove(RaceId);
	}

	URPGCharacterPortraitSetAsset* Definition =
		RPGAuthoringIdentityResolverPrivate::ResolveCanonicalAsset<URPGCharacterPortraitSetAsset>(
			MakePortraitSetPrimaryAssetId(RaceId), URPGCharacterPortraitSetAsset::StaticClass());
	if (!IsMatchingPortraitSet(RaceId, Definition))
	{
		return nullptr;
	}
	RememberPortraitSet(Definition);
	return Definition;
}

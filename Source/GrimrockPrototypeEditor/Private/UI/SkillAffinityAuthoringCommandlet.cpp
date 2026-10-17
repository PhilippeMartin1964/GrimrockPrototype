#include "UI/SkillAffinityAuthoringCommandlet.h"

#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "UI/RPGSkillPresentationAsset.h"
#include "UI/SkillAffinityProductionAuthoring.h"
#include "UObject/SavePackage.h"

namespace SkillAffinityAuthoringCommandlet
{
	const TCHAR* SkillPresentationPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/UI/RPG/DA_RPGSkillPresentation.DA_RPGSkillPresentation");

	const FName CanonicalClassIds[] =
	{
		TEXT("Warrior"),
		TEXT("Rogue"),
		TEXT("Ranger"),
		TEXT("Mage"),
		TEXT("Priest"),
		TEXT("Alchemist")
	};

	bool SaveCatalog(URPGSkillPresentationAsset& Catalog, FString& OutError)
	{
		UPackage* Package = Catalog.GetOutermost();
		if (!Package)
		{
			OutError = TEXT("DA_RPGSkillPresentation has no package.");
			return false;
		}

		Package->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(),
			FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		if (!UPackage::SavePackage(Package, &Catalog, *Filename, SaveArgs))
		{
			OutError = FString::Printf(TEXT("Failed to save %s."), *Catalog.GetPathName());
			return false;
		}
		return true;
	}
}

USkillAffinityAuthoringCommandlet::USkillAffinityAuthoringCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 USkillAffinityAuthoringCommandlet::Main(const FString& Params)
{
	(void)Params;
	FRPGAuthoringIdentityResolver::ResetRuntimeCache();

	for (const FName ClassId : SkillAffinityAuthoringCommandlet::CanonicalClassIds)
	{
		if (!FRPGAuthoringIdentityResolver::ResolveClassById(ClassId))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[UI-SKILLS-AFFINITY01.3C] Canonical class %s does not resolve."),
				*ClassId.ToString());
			return 1;
		}
	}

	URPGSkillPresentationAsset* Catalog =
		LoadObject<URPGSkillPresentationAsset>(nullptr, SkillAffinityAuthoringCommandlet::SkillPresentationPath);
	if (!IsValid(Catalog))
	{
		UE_LOG(LogTemp, Error, TEXT("[UI-SKILLS-AFFINITY01.3C] Missing production Skill presentation catalog."));
		return 1;
	}

	Catalog->Modify();
	int32 ChangedSkillCount = 0;
	FString Error;
	if (!FSkillAffinityProductionAuthoring::ApplyCanonicalDiscouragedAffinities(
		*Catalog, ChangedSkillCount, Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[UI-SKILLS-AFFINITY01.3C] %s"), *Error);
		return 1;
	}

	if (ChangedSkillCount == 0)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[UI-SKILLS-AFFINITY01.3C] DA_RPGSkillPresentation already matches the canonical discouraged matrix."));
		FRPGAuthoringIdentityResolver::ResetRuntimeCache();
		return 0;
	}

	if (!SkillAffinityAuthoringCommandlet::SaveCatalog(*Catalog, Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[UI-SKILLS-AFFINITY01.3C] %s"), *Error);
		return 1;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[UI-SKILLS-AFFINITY01.3C] Materialized canonical DiscouragedClassIds on %d Skill rows."),
		ChangedSkillCount);
	UE_LOG(LogTemp, Display,
		TEXT("[UI-SKILLS-AFFINITY01.3C] Saved: %s"),
		*Catalog->GetPathName());

	FRPGAuthoringIdentityResolver::ResetRuntimeCache();
	return 0;
}

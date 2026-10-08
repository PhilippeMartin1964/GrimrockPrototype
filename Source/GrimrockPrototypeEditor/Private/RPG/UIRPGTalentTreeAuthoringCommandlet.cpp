#include "RPG/UIRPGTalentTreeAuthoringCommandlet.h"

#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "RPG/RPGAlchemistAuthoring.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGMageAuthoring.h"
#include "RPG/RPGPriestAuthoring.h"
#include "RPG/RPGRangerAuthoring.h"
#include "RPG/RPGRogueAuthoring.h"
#include "RPG/RPGWarriorAuthoring.h"
#include "UObject/SavePackage.h"

namespace UIRPGTalentTreeAuthoring
{
	struct FClassSpec
	{
		FName ClassId = NAME_None;
		const TCHAR* ObjectPath = nullptr;
	};

	bool SaveClassAsset(URPGClassAsset* ClassAsset, FString& OutError)
	{
		if (!IsValid(ClassAsset))
		{
			OutError = TEXT("Cannot save a null class asset.");
			return false;
		}

		UPackage* Package = ClassAsset->GetOutermost();
		if (!Package)
		{
			OutError = FString::Printf(TEXT("Class '%s' has no package."), *GetNameSafe(ClassAsset));
			return false;
		}

		Package->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		if (!UPackage::SavePackage(Package, ClassAsset, *Filename, SaveArgs))
		{
			OutError = FString::Printf(TEXT("Failed to save '%s'."), *ClassAsset->GetPathName());
			return false;
		}
		return true;
	}

	bool ConfigureClass(FName ClassId, URPGClassAsset& ClassAsset, FString& OutError)
	{
		if (ClassId == TEXT("Warrior"))
		{
			FRPGWarriorAuthoring::ConfigureClass(ClassAsset);
			return true;
		}
		if (ClassId == TEXT("Rogue"))
		{
			FRPGRogueAuthoring::ConfigureClass(ClassAsset);
			return true;
		}
		if (ClassId == TEXT("Ranger"))
		{
			TArray<FRPGRangerFavoredEnemyCategoryDefinition> FavoredEnemyCategories;
			if (!FRPGRangerAuthoring::CollectProductionFavoredEnemyCategories(FavoredEnemyCategories, OutError))
			{
				return false;
			}
			FRPGRangerAuthoring::ConfigureClass(ClassAsset, FavoredEnemyCategories);
			return true;
		}
		if (ClassId == TEXT("Mage"))
		{
			FRPGMageAuthoring::ConfigureClass(ClassAsset);
			return true;
		}
		if (ClassId == TEXT("Priest"))
		{
			FRPGPriestAuthoring::ConfigureClass(ClassAsset);
			return true;
		}
		if (ClassId == TEXT("Alchemist"))
		{
			FRPGAlchemistAuthoring::ConfigureClass(ClassAsset);
			return true;
		}

		OutError = FString::Printf(TEXT("Unsupported UI-RPG production class '%s'."), *ClassId.ToString());
		return false;
	}

	bool HasCompleteStructuralMetadata(const URPGClassAsset& ClassAsset, FString& OutError)
	{
		if (ClassAsset.ProgressionChoices.IsEmpty())
		{
			OutError = FString::Printf(TEXT("%s exposes no progression choices."), *ClassAsset.ClassId.ToString());
			return false;
		}

		for (const FRPGClassProgressionChoiceDefinition& Choice : ClassAsset.ProgressionChoices)
		{
			if (Choice.TalentBranchId.IsNone() || Choice.TalentNodeId.IsNone())
			{
				OutError = FString::Printf(
					TEXT("%s choice %s is missing TalentBranchId or TalentNodeId."),
					*ClassAsset.ClassId.ToString(), *Choice.ChoiceId.ToString());
				return false;
			}
		}
		return true;
	}
}

UUIRPGTalentTreeAuthoringCommandlet::UUIRPGTalentTreeAuthoringCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UUIRPGTalentTreeAuthoringCommandlet::Main(const FString& Params)
{
	(void)Params;
	using namespace UIRPGTalentTreeAuthoring;

	const FClassSpec Specs[] = {
		{ TEXT("Warrior"), FRPGWarriorAuthoring::WarriorAssetPath() },
		{ TEXT("Rogue"), FRPGRogueAuthoring::RogueAssetPath() },
		{ TEXT("Ranger"), FRPGRangerAuthoring::RangerAssetPath() },
		{ TEXT("Mage"), FRPGMageAuthoring::MageAssetPath() },
		{ TEXT("Priest"), FRPGPriestAuthoring::PriestAssetPath() },
		{ TEXT("Alchemist"), FRPGAlchemistAuthoring::AlchemistAssetPath() }
	};

	for (const FClassSpec& Spec : Specs)
	{
		URPGClassAsset* ClassAsset = LoadObject<URPGClassAsset>(nullptr, Spec.ObjectPath);
		if (!IsValid(ClassAsset))
		{
			UE_LOG(LogTemp, Error, TEXT("[UI-RPG01.4] Missing production class asset: %s"), Spec.ObjectPath);
			return 1;
		}
		if (ClassAsset->ClassId != Spec.ClassId)
		{
			UE_LOG(LogTemp, Error, TEXT("[UI-RPG01.4] ClassId mismatch for %s: got %s."),
				Spec.ObjectPath, *ClassAsset->ClassId.ToString());
			return 1;
		}

		ClassAsset->Modify();
		FString Error;
		if (!ConfigureClass(Spec.ClassId, *ClassAsset, Error))
		{
			UE_LOG(LogTemp, Error, TEXT("[UI-RPG01.4] %s authoring failed: %s"), *Spec.ClassId.ToString(), *Error);
			return 1;
		}
		if (!ClassAsset->IsValidDefinition())
		{
			UE_LOG(LogTemp, Error, TEXT("[UI-RPG01.4] %s became structurally invalid after canonical authoring."),
				*Spec.ClassId.ToString());
			return 1;
		}
		if (!HasCompleteStructuralMetadata(*ClassAsset, Error))
		{
			UE_LOG(LogTemp, Error, TEXT("[UI-RPG01.4] %s"), *Error);
			return 1;
		}
		if (!SaveClassAsset(ClassAsset, Error))
		{
			UE_LOG(LogTemp, Error, TEXT("[UI-RPG01.4] %s"), *Error);
			return 1;
		}

		UE_LOG(LogTemp, Display, TEXT("[UI-RPG01.4] Materialized structural Talent metadata on %s."),
			*Spec.ClassId.ToString());
	}

	UE_LOG(LogTemp, Display,
		TEXT("[UI-RPG01.4] Six production DA_Class_* assets materialized. No status or item asset was authored."));
	return 0;
}

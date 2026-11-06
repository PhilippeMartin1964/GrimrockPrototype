#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "HAL/FileManager.h"
#include "Misc/AssetRegistryInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

namespace EffectsOrg011
{
	constexpr TCHAR ProjectRoot[] = TEXT("/Game/GrimrockPrototype");
	constexpr TCHAR VendorRoot[] = TEXT("/Game/FireFX");

	// Enumerate every first-party effect folder, plus the art dependencies
	// suspected from repository filenames. Do not classify ordinary item
	// PBR textures or UI icons as VFX assets just because their name contains "Fire".
	const TCHAR* const EffectFolders[] =
	{
		TEXT("/Game/GrimrockPrototype/Effects/"),
		TEXT("/Game/GrimrockPrototype/Art/FX/"),
		TEXT("/Game/GrimrockPrototype/Art/Materials/Catalyst/"),
		TEXT("/Game/GrimrockPrototype/Art/Materials/Surfaces/"),
		TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Flame/"),
		TEXT("/Game/GrimrockPrototype/Art/Textures/Final/Flame/")
	};

	const TCHAR* const AdditionalAssets[] =
	{
		TEXT("/Game/GrimrockPrototype/Art/Materials/Masters/M_Master_Flame"),
		TEXT("/Game/GrimrockPrototype/Art/Materials/Item_Materials/M_Potion_LiquidFlow"),
		TEXT("/Game/GrimrockPrototype/Art/Materials/Item_Materials/M_Item_WorldSparkle"),
		TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Items/MI_Potion_LiquidFlow")
	};

	bool IsEffectCandidate(const FAssetData& Data)
	{
		const FString Package = Data.PackageName.ToString();
		for (const TCHAR* Prefix : EffectFolders)
		{
			if (Package.StartsWith(Prefix, ESearchCase::CaseSensitive))
			{
				return true;
			}
		}
		for (const TCHAR* Name : AdditionalAssets)
		{
			if (Package.Equals(Name, ESearchCase::CaseSensitive))
			{
				return true;
			}
		}
		// Catch Niagara effects created later, regardless of directory.
		return Data.AssetName.ToString().StartsWith(TEXT("NS_"));
	}

	FString CsvCell(FString Text)
	{
		Text.ReplaceInline(TEXT("\""), TEXT("\"\""));
		return FString::Printf(TEXT("\"%s\""), *Text);
	}

	FString FormatPackages(TArray<FName> Packages)
	{
		Packages.Sort([](const FName& A, const FName& B)
		{
			return A.ToString() < B.ToString();
		});
		TArray<FString> Values;
		Values.Reserve(Packages.Num());
		for (const FName& Package : Packages)
		{
			Values.Add(Package.ToString());
		}
		return FString::Join(Values, TEXT("; "));
	}

	FString OwnerHint(const FString& Package)
	{
		if (Package.EndsWith(TEXT("/M_Teleporter_Spark_02")))
		{
			return TEXT("ALCHIMIE_POTION_RENAME_M_Potion_Spark");
		}
		if (Package.StartsWith(TEXT("/Game/GrimrockPrototype/Effects/Alchimie/")))
		{
			return TEXT("ALCHIMIE_KEEP");
		}
		if (Package.StartsWith(TEXT("/Game/GrimrockPrototype/Effects/Teleporter/")))
		{
			return TEXT("TELEPORTER_KEEP");
		}
		if (Package.StartsWith(TEXT("/Game/GrimrockPrototype/Art/FX/")) ||
			Package.Contains(TEXT("/Flame/")) ||
			Package.Contains(TEXT("M_Master_Flame")))
		{
			return TEXT("FIRE_REVIEW_DEPENDENCIES");
		}
		if (Package.Contains(TEXT("Catalyst")))
		{
			return TEXT("CATALYST_REVIEW_DEPENDENCIES");
		}
		if (Package.Contains(TEXT("Surface_Oil")))
		{
			return TEXT("SURFACE_OIL_REVIEW_DEPENDENCIES");
		}
		return TEXT("REVIEW_OWNER_NO_MOVE");
	}

	bool HasPackage(IAssetRegistry& Registry, const TCHAR* Name)
	{
		TArray<FAssetData> Found;
		return Registry.GetAssetsByPackageName(FName(Name), Found, true, false) && !Found.IsEmpty();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorEffectsOrg011InventoryTest,
	"Grimrock.Editor.EffectsOrg01.Inventory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorEffectsOrg011InventoryTest::RunTest(const FString&)
{
	using namespace EffectsOrg011;
	IAssetRegistry& Registry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	// Blocks until the on-disk registry is ready. No content package is modified.
	Registry.SearchAllAssets(true);

	TArray<FAssetData> ProjectAssets;
	Registry.GetAssetsByPath(FName(ProjectRoot), ProjectAssets, true, false);
	TArray<FAssetData> VendorAssets;
	Registry.GetAssetsByPath(FName(VendorRoot), VendorAssets, true, false);

	TArray<FAssetData> Candidates;
	for (const FAssetData& Asset : ProjectAssets)
	{
		if (IsEffectCandidate(Asset))
		{
			Candidates.Add(Asset);
		}
	}
	Candidates.Sort([](const FAssetData& A, const FAssetData& B)
	{
		return A.PackageName.ToString() < B.PackageName.ToString();
	});

	TArray<FString> Csv;
	Csv.Add(TEXT("Package,AssetName,AssetClass,OwnerHint,RefQueryOK,ReferencerCount,Referencers,DepQueryOK,DependencyCount,Dependencies,Attention"));
	int32 NoRegistryReferencers = 0;
	int32 CrossFolderDependencies = 0;
	int32 AssetQueryFailures = 0;
	int32 ExternalVendorLinks = 0;

	for (const FAssetData& Asset : Candidates)
	{
		TArray<FName> References;
		TArray<FName> Dependencies;
		const bool bRefsOk = Registry.GetReferencers(
			Asset.PackageName, References,
			UE::AssetRegistry::EDependencyCategory::All,
			UE::AssetRegistry::FDependencyQuery());
		const bool bDepsOk = Registry.GetDependencies(
			Asset.PackageName, Dependencies,
			UE::AssetRegistry::EDependencyCategory::All,
			UE::AssetRegistry::FDependencyQuery());

		if (!bRefsOk || !bDepsOk)
		{
			++AssetQueryFailures;
		}
		if (bRefsOk && References.IsEmpty())
		{
			++NoRegistryReferencers;
		}

		bool bCrossFolder = false;
		bool bVendorLink = false;
		const FString OwnFolder = FPaths::GetPath(Asset.PackageName.ToString());
		for (const FName& Dep : Dependencies)
		{
			const FString DepPath = Dep.ToString();
			if (DepPath.StartsWith(VendorRoot))
			{
				bVendorLink = true;
			}
			if (DepPath.StartsWith(ProjectRoot) &&
				FPaths::GetPath(DepPath) != OwnFolder)
			{
				bCrossFolder = true;
			}
		}
		CrossFolderDependencies += bCrossFolder ? 1 : 0;
		ExternalVendorLinks += bVendorLink ? 1 : 0;

		// Zero registry referrers DOES NOT prove the asset unused:
		// C++ LoadObject, soft references and authoring scripts may rely on it.
		FString Attention = TEXT("REVIEW");
		if (!bRefsOk || !bDepsOk)
		{
			Attention = TEXT("REGISTRY_QUERY_INCOMPLETE");
		}
		else if (References.IsEmpty())
		{
			Attention = TEXT("ZERO_REGISTRY_REFERENCERS_NOT_PROOF_OF_DEAD_ASSET");
		}
		else if (bVendorLink)
		{
			Attention = TEXT("PRIVATE_VENDOR_DEPENDENCY_CHECK_PACKAGE");
		}
		else if (bCrossFolder)
		{
			Attention = TEXT("CROSS_FOLDER_DEPENDENCIES_REVIEW");
		}

		const TArray<FString> Columns =
		{
			CsvCell(Asset.PackageName.ToString()),
			CsvCell(Asset.AssetName.ToString()),
			CsvCell(Asset.AssetClassPath.ToString()),
			CsvCell(OwnerHint(Asset.PackageName.ToString())),
			CsvCell(bRefsOk ? TEXT("true") : TEXT("false")),
			CsvCell(FString::FromInt(References.Num())),
			CsvCell(bRefsOk ? FormatPackages(References) : FString()),
			CsvCell(bDepsOk ? TEXT("true") : TEXT("false")),
			CsvCell(FString::FromInt(Dependencies.Num())),
			CsvCell(bDepsOk ? FormatPackages(Dependencies) : FString()),
			CsvCell(Attention)
		};
		Csv.Add(FString::Join(Columns, TEXT(",")));
	}

	const bool bPotionOld = HasPackage(Registry,
		TEXT("/Game/GrimrockPrototype/Effects/Alchimie/M_Teleporter_Spark_02"));
	const bool bPotionTarget = HasPackage(Registry,
		TEXT("/Game/GrimrockPrototype/Effects/Alchimie/M_Potion_Spark"));

	TArray<FString> Summary;
	Summary.Add(TEXT("EFFECTS-ORG01.1 - Audit Asset Registry (lecture seule)"));
	Summary.Add(FString::Printf(TEXT("First-party effect candidates: %d"), Candidates.Num()));
	Summary.Add(FString::Printf(TEXT("FireFX vendor assets visible locally: %d (optional, ignored by Git)"),
		VendorAssets.Num()));
	Summary.Add(FString::Printf(TEXT("Assets with cross-folder dependencies: %d"), CrossFolderDependencies));
	Summary.Add(FString::Printf(TEXT("Assets referencing /Game/FireFX: %d"), ExternalVendorLinks));
	Summary.Add(FString::Printf(TEXT("No registry referencers: %d (NOT proof of dead content)"),
		NoRegistryReferencers));
	Summary.Add(FString::Printf(TEXT("Registry query incomplete: %d"), AssetQueryFailures));
	Summary.Add(FString::Printf(TEXT("Potion spark old path exists: %s"),
		bPotionOld ? TEXT("yes") : TEXT("no")));
	Summary.Add(FString::Printf(TEXT("Potion spark target M_Potion_Spark exists: %s"),
		bPotionTarget ? TEXT("yes") : TEXT("no")));
	Summary.Add(TEXT("Decision: M_Teleporter_Spark_02 IS the healing potion bubble material; rename later in Unreal, keep in Alchimie."));
	Summary.Add(TEXT("No asset renaming, deletion, save, redirector fix-up or vendor content publication was performed."));

	const FString Directory = FPaths::Combine(
		FPaths::ProjectSavedDir(), TEXT("Audits/EFFECTS-ORG01"));
	IFileManager::Get().MakeDirectory(*Directory, true);
	const FString CsvPath = FPaths::Combine(Directory, TEXT("EffectsAssetRegistry.csv"));
	const FString SummaryPath = FPaths::Combine(Directory, TEXT("EffectsSummary.txt"));
	const bool bCsvSaved = FFileHelper::SaveStringArrayToFile(
		Csv, *CsvPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	const bool bSummarySaved = FFileHelper::SaveStringArrayToFile(
		Summary, *SummaryPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

	TestTrue(TEXT("Inventory CSV written under Saved (never Content)"), bCsvSaved);
	TestTrue(TEXT("Inventory summary written under Saved"), bSummarySaved);

	TestTrue(TEXT("Canonical potion Niagara in Alchimie"),
		HasPackage(Registry, TEXT("/Game/GrimrockPrototype/Effects/Alchimie/NS_LiquideSparkle")));
	TestTrue(TEXT("Canonical teleporter Niagara"),
		HasPackage(Registry, TEXT("/Game/GrimrockPrototype/Effects/Teleporter/NS_Teleporter_Blue")));
	TestTrue(TEXT("Canonical torch Niagara"),
		HasPackage(Registry, TEXT("/Game/GrimrockPrototype/Art/FX/NS_Flame_8_Torch")));
	TestTrue(TEXT("Potion bubble material remains in Alchimie before/after rename"),
		bPotionOld || bPotionTarget);
	TestFalse(TEXT("Do not retain old and renamed potion material simultaneously"),
		bPotionOld && bPotionTarget);

	for (const FString& Line : Summary)
	{
		AddInfo(Line);
	}
	AddInfo(FString::Printf(TEXT("[EFFECTS-ORG01.1] CSV=%s"), *CsvPath));
	return bCsvSaved && bSummarySaved;
}

#endif // WITH_DEV_AUTOMATION_TESTS

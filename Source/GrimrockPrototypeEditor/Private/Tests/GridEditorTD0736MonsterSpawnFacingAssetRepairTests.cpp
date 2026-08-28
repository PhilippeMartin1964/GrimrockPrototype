#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Core/GridLevelAsset.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace GridTD0736FacingRepair
{
	bool IsCardinalFacing(EGridEdge Facing)
	{
		return Facing == EGridEdge::North || Facing == EGridEdge::East ||
			Facing == EGridEdge::South || Facing == EGridEdge::West;
	}

	bool SaveAssetPackage(UGridLevelAsset* Asset)
	{
		if (!IsValid(Asset))
		{
			return false;
		}

		UPackage* Package = Asset->GetOutermost();
		if (!Package)
		{
			return false;
		}

		const FString Filename =
			FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		SaveArgs.Error = GError;
		return UPackage::SavePackage(Package, Asset, *Filename, SaveArgs);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0736MonsterSpawnFacingAssetRepairTest,
	"Grimrock.TechnicalDebt.TD07_3_6.AssetRepair.MonsterSpawnFacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0736MonsterSpawnFacingAssetRepairTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0736FacingRepair;

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game")));
	Filter.ClassPaths.Add(UGridLevelAsset::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);
	Assets.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.PackageName.LexicalLess(Right.PackageName);
	});

	int32 LoadedCount = 0;
	int32 MonsterLevelCount = 0;
	int32 SavedCount = 0;
	int32 MonsterSpawnCount = 0;

	for (const FAssetData& AssetData : Assets)
	{
		UGridLevelAsset* Level = Cast<UGridLevelAsset>(AssetData.GetAsset());
		TestNotNull(*FString::Printf(TEXT("%s loads"), *AssetData.PackageName.ToString()), Level);
		if (!Level)
		{
			continue;
		}
		++LoadedCount;

		bool bHasMonsterSpawn = false;
		for (const FGridLevelObjectData& Object : Level->Objects)
		{
			if (Object.Type != EGridLevelObjectType::MonsterSpawn)
			{
				continue;
			}

			bHasMonsterSpawn = true;
			++MonsterSpawnCount;
			TestTrue(*FString::Printf(TEXT("%s MonsterSpawn has cardinal InitialFacing"), *AssetData.PackageName.ToString()),
				IsCardinalFacing(Object.InitialFacing));
		}

		if (!bHasMonsterSpawn)
		{
			continue;
		}

		++MonsterLevelCount;
		TArray<FString> SpawnErrors;
		const bool bValidSpawns = Level->ValidateMonsterSpawns(SpawnErrors);
		TestTrue(*FString::Printf(TEXT("%s MonsterSpawn data validates"), *AssetData.PackageName.ToString()), bValidSpawns);
		for (const FString& Error : SpawnErrors)
		{
			AddError(FString::Printf(TEXT("%s: %s"), *AssetData.PackageName.ToString(), *Error));
		}

		Level->Modify();
		Level->MarkPackageDirty();
		TestTrue(*FString::Printf(TEXT("%s resaves current MonsterSpawn facing"), *AssetData.PackageName.ToString()),
			SaveAssetPackage(Level));
		++SavedCount;
	}

	TestEqual(TEXT("Every discovered GridLevelAsset loads"), LoadedCount, Assets.Num());
	TestEqual(TEXT("Every GridLevelAsset containing MonsterSpawn is resaved"), SavedCount, MonsterLevelCount);
	AddInfo(FString::Printf(TEXT("TD07.3.6 scanned %d GridLevelAsset(s), resaved %d monster level(s), validated %d MonsterSpawn(s)."),
		LoadedCount, SavedCount, MonsterSpawnCount));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

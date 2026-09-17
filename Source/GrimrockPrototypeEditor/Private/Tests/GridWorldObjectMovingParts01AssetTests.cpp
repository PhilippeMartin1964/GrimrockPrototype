#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMovingParts01RealAssetTest,
	"Grimrock.WorldObjects.MOVINGPARTS01.RealAssetMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMovingParts01RealAssetTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = AssetRegistryModule.Get();
	Registry.SearchAllAssets(true);

	FARFilter Filter;
	Filter.ClassPaths.Add(UGridWorldObjectDefinitionAsset::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(FName(TEXT("/Game")));
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	TArray<FAssetData> Assets;
	Registry.GetAssets(Filter, Assets);
	TestEqual(TEXT("All 38 migrated definition assets remain discoverable"), Assets.Num(), 38);

	const TMap<FName, int32> ExpectedMovingPartCounts = {
		{TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Button_Normal"), 1},
		{TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Button_Secret"), 1},
		{TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Door_Grating"), 1},
		{TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Door_Iron"), 2},
		{TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Door_Secret"), 1},
		{TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Door_Wood"), 1},
		{TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lever"), 1},
		{TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_PressurePlate"), 1},
		{TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Pit_Stone"), 2},
	};

	for (const FAssetData& AssetData : Assets)
	{
		UGridWorldObjectDefinitionAsset* Definition = Cast<UGridWorldObjectDefinitionAsset>(AssetData.GetAsset());
		if (!TestNotNull(*FString::Printf(TEXT("Load final asset %s without migration redirect"), *AssetData.PackageName.ToString()), Definition))
		{
			continue;
		}
		const int32 ExpectedCount = ExpectedMovingPartCounts.FindRef(AssetData.PackageName);
		TestEqual(*FString::Printf(TEXT("Final MovingParts count for %s"), *AssetData.PackageName.ToString()), Definition->MovingParts.Num(), ExpectedCount);
		for (int32 Index = 0; Index < Definition->MovingParts.Num(); ++Index)
		{
			TestTrue(*FString::Printf(TEXT("Migrated part %d remains defined for %s"), Index, *AssetData.PackageName.ToString()),
				Definition->MovingParts[Index].IsDefined());
		}
	}
	return true;
}

#endif

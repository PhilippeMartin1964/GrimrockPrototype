#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "EdGraph/EdGraph.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridEditorWorldObjectMIG09ItemLegacyBlueprintReferenceAuditTest,
	"Grimrock.WorldObjects.MIG09.ItemLegacyBlueprintReferences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorWorldObjectMIG09ItemLegacyBlueprintReferenceAuditTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game/GrimrockPrototype")));
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> BlueprintAssets;
	AssetRegistry.GetAssets(Filter, BlueprintAssets);
	BlueprintAssets.Sort(
		[](const FAssetData& Left, const FAssetData& Right)
		{
			return Left.PackageName.ToString() < Right.PackageName.ToString();
		});

	const TSet<FName> LegacyFunctionNames = {
		FName(TEXT("InitializeItem")),
		FName(TEXT("GetItemArchetypeId"))
	};

	TArray<FString> LegacyReferences;
	int32 LoadedBlueprintCount = 0;

	for (const FAssetData& BlueprintAssetData : BlueprintAssets)
	{
		UBlueprint* Blueprint = Cast<UBlueprint>(BlueprintAssetData.GetAsset());
		if (!Blueprint)
		{
			AddError(FString::Printf(
				TEXT("MIG09-B2C Blueprint audit could not load %s."), *BlueprintAssetData.PackageName.ToString()));
			continue;
		}

		++LoadedBlueprintCount;
		TArray<UEdGraph*> Graphs;
		FBlueprintEditorUtils::GetAllGraphs(Blueprint, Graphs);

		for (const UEdGraph* Graph : Graphs)
		{
			if (!Graph)
			{
				continue;
			}

			for (const UEdGraphNode* Node : Graph->Nodes)
			{
				const UK2Node_CallFunction* CallFunction = Cast<UK2Node_CallFunction>(Node);
				if (!CallFunction)
				{
					continue;
				}

				const FName FunctionName = CallFunction->FunctionReference.GetMemberName();
				if (!LegacyFunctionNames.Contains(FunctionName))
				{
					continue;
				}

				LegacyReferences.Add(FString::Printf(
					TEXT("%s :: Graph=%s :: Function=%s :: Node=%s"),
					*BlueprintAssetData.PackageName.ToString(),
					*Graph->GetName(),
					*FunctionName.ToString(),
					*GetNameSafe(Node)));
			}
		}
	}

	TestEqual(TEXT("Every discovered Blueprint can be loaded"), LoadedBlueprintCount, BlueprintAssets.Num());

	for (const FString& Reference : LegacyReferences)
	{
		AddError(FString::Printf(TEXT("MIG09-B2C legacy item Blueprint reference remains: %s"), *Reference));
	}

	TestEqual(TEXT("No Blueprint calls InitializeItem or GetItemArchetypeId"), LegacyReferences.Num(), 0);
	return LegacyReferences.IsEmpty() && LoadedBlueprintCount == BlueprintAssets.Num();
}

#endif // WITH_DEV_AUTOMATION_TESTS

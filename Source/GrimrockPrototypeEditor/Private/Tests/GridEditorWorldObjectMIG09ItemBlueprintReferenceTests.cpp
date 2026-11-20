#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/Blueprint.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridReceptacleActor.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
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
	Filter.PackagePaths.Add(FName(TEXT("/Game")));
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
		FName(TEXT("GetItemArchetypeId")),
		FName(TEXT("SetSelectedObjectItemDefinitionAsset")),
		FName(TEXT("SyncSelectedItemDefinitionIdFromAsset")),
		FName(TEXT("RegisterItemDefinition")),
		FName(TEXT("FindItemDefinition")),
		FName(TEXT("TryInsertItem"))
	};

	TArray<FString> LegacyReferences;
	TArray<FString> ContainedStructReferences;
	const UScriptStruct* ContainedStruct = FGridContainedReceptacleItem::StaticStruct();
	const auto ReferencesContainedStruct = [ContainedStruct](const FEdGraphPinType& PinType)
	{
		return PinType.PinSubCategoryObject.Get() == ContainedStruct ||
			PinType.PinValueType.TerminalSubCategoryObject.Get() == ContainedStruct;
	};
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
		if (Blueprint->GeneratedClass &&
			Blueprint->GeneratedClass->IsChildOf(AGridReceptacleActor::StaticClass()))
		{
			const auto* CDO = Cast<AGridReceptacleActor>(Blueprint->GeneratedClass->GetDefaultObject());
			if (!CDO || !CDO->ContainedItems.IsEmpty())
				ContainedStructReferences.Add(BlueprintAssetData.PackageName.ToString() +
					TEXT(" :: authored runtime container is not empty or unavailable"));
		}

		// Conservative preflight before removing a reflected field from the
		// contained-item struct. Inspect the whole typed graph surface,
		// including split/Make/Break pins, not just connected old fields.
		for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
		{
			if (ReferencesContainedStruct(Variable.VarType))
				ContainedStructReferences.Add(BlueprintAssetData.PackageName.ToString() +
					TEXT(" :: Variable=") + Variable.VarName.ToString());
		}
		TArray<UEdGraphNode*> Nodes;
		FBlueprintEditorUtils::GetAllNodesOfClass<UEdGraphNode>(Blueprint, Nodes);
		for (const UEdGraphNode* Node : Nodes)
		{
			if (!Node) continue;
			for (const UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && ReferencesContainedStruct(Pin->PinType))
					ContainedStructReferences.Add(FString::Printf(TEXT("%s :: Node=%s :: Pin=%s"),
						*BlueprintAssetData.PackageName.ToString(), *GetNameSafe(Node), *Pin->PinName.ToString()));
			}
		}
		TArray<UK2Node_CallFunction*> CallFunctions;
		FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_CallFunction>(Blueprint, CallFunctions);

		for (const UK2Node_CallFunction* CallFunction : CallFunctions)
		{
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
				*GetNameSafe(CallFunction->GetGraph()),
				*FunctionName.ToString(),
				*GetNameSafe(CallFunction)));
		}
	}

	TestEqual(TEXT("Every discovered Blueprint can be loaded"), LoadedBlueprintCount, BlueprintAssets.Num());

	for (const FString& Reference : LegacyReferences)
	{
		AddError(FString::Printf(TEXT("MIG09-B2C legacy item Blueprint reference remains: %s"), *Reference));
	}

	for (const FString& Reference : ContainedStructReferences)
	{
		AddError(FString::Printf(TEXT("5G77 contained-item struct usage requires review: %s"), *Reference));
	}
	TestEqual(TEXT("No native Blueprint variable or graph pin uses contained-item struct"),
		ContainedStructReferences.Num(), 0);
	TestTrue(TEXT("Native Blueprint inventory is not empty"), BlueprintAssets.Num() > 0);
	TestEqual(TEXT("No Blueprint calls retired item identity APIs or historical inventory registry"), LegacyReferences.Num(), 0);
	TestNull(TEXT("Retired setter is no longer reflected"), AGridLevelEditorActor::StaticClass()->FindFunctionByName(TEXT("SetSelectedObjectItemDefinitionAsset")));
	TestNull(TEXT("Retired sync is no longer reflected"), AGridLevelEditorActor::StaticClass()->FindFunctionByName(TEXT("SyncSelectedItemDefinitionIdFromAsset")));
	TestNull(TEXT("Historical inventory registration is no longer reflected"), UGridPartyInventoryComponent::StaticClass()->FindFunctionByName(TEXT("RegisterItemDefinition")));
	TestNull(TEXT("Historical inventory lookup is no longer reflected"), UGridPartyInventoryComponent::StaticClass()->FindFunctionByName(TEXT("FindItemDefinition")));
	TestNotNull(TEXT("Inventory instance projection remains reflected"), UGridPartyInventoryComponent::StaticClass()->FindFunctionByName(TEXT("ApplyItemDefinitionToInstance")));
	return BlueprintAssets.Num() > 0 && LegacyReferences.IsEmpty() &&
		ContainedStructReferences.IsEmpty() && LoadedBlueprintCount == BlueprintAssets.Num();
}

#endif // WITH_DEV_AUTOMATION_TESTS

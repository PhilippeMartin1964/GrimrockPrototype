#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/Blueprint.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridReceptacleActor.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Modules/ModuleManager.h"
#include "UObject/UnrealType.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"

namespace
{
    bool IsInstancePin(const UEdGraphPin* Pin)
    {
        return Pin && Pin->PinType.PinSubCategoryObject.Get() == FGridItemInstance::StaticStruct();
    }

    // Inventory instances are physical identities. Only these cache members
    // are targeted, not whole-struct references.
    FString InstanceCacheField(const UEdGraphNode* Node, const UEdGraphPin* Pin)
    {
        if (!Node || !Pin) return FString();
        const FString Name = Pin->PinName.ToString();
        bool bStructMember = false;
        for (const UEdGraphPin* Parent = Pin->ParentPin; Parent; Parent = Parent->ParentPin)
            if (IsInstancePin(Parent)) { bStructMember = true; break; }
        if (!bStructMember)
        {
            if (!Node->GetClass()->GetName().Contains(TEXT("Struct"))) return FString();
            if (const FObjectPropertyBase* Type = FindFProperty<FObjectPropertyBase>(Node->GetClass(), TEXT("StructType")))
                bStructMember = Type->GetObjectPropertyValue_InContainer(Node) == FGridItemInstance::StaticStruct();
            if (!bStructMember)
                for (const UEdGraphPin* Other : Node->Pins)
                    if (IsInstancePin(Other)) { bStructMember = true; break; }
        }
        if (!bStructMember) return FString();
        for (const TCHAR* Field : { TEXT("DisplayName"), TEXT("Weight") })
            if (Name.Equals(Field, ESearchCase::CaseSensitive) ||
                Name.EndsWith(FString(TEXT("_")) + Field, ESearchCase::CaseSensitive))
                return Field;
        return FString();
    }

    bool WriteInstanceCacheEvidence(int32 BlueprintCount, const TArray<TSharedPtr<FJsonValue>>& Rows)
    {
        FString Directory;
        if (!FParse::Value(FCommandLine::Get(), TEXT("ReportExportPath="), Directory) || Directory.IsEmpty())
            return true;
        auto Report = MakeShared<FJsonObject>();
        Report->SetNumberField(TEXT("schema"), 1);
        Report->SetNumberField(TEXT("blueprints_scanned"), BlueprintCount);
        Report->SetArrayField(TEXT("rows"), Rows);
        FString Json;
        if (!FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json))) return false;
        IFileManager::Get().MakeDirectory(*Directory, true);
        return FFileHelper::SaveStringToFile(Json, *FPaths::Combine(Directory, TEXT("InstanceCacheBlueprintEvidence.json")),
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }
}

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
		FName(TEXT("TryInsertItem")),
		FName(TEXT("AddRuntimeItemToSelectedCharacterInventory"))
	};

	TArray<FString> LegacyReferences;
	TArray<FString> ContainedStructReferences;
	const UScriptStruct* ContainedStruct = FGridContainedReceptacleItem::StaticStruct();
	const auto ReferencesContainedStruct = [ContainedStruct](const FEdGraphPinType& PinType)
	{
		return PinType.PinSubCategoryObject.Get() == ContainedStruct ||
			PinType.PinValueType.TerminalSubCategoryObject.Get() == ContainedStruct;
	};
	TArray<FString> WorldSaveStructReferences;
	const auto ReferencesWorldSaveStruct = [](const FEdGraphPinType& PinType)
	{
		const auto IsTarget = [](const UObject* Type)
		{
			return Type == FGridRuntimeItemState::StaticStruct() ||
				Type == FGridPendingWorldItemState::StaticStruct();
		};
		return IsTarget(PinType.PinSubCategoryObject.Get()) ||
			IsTarget(PinType.PinValueType.TerminalSubCategoryObject.Get());
	};
	int32 LoadedBlueprintCount = 0;
	TArray<TSharedPtr<FJsonValue>> InstanceCacheRows;

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
			if (ReferencesWorldSaveStruct(Variable.VarType))
				WorldSaveStructReferences.Add(BlueprintAssetData.PackageName.ToString() +
					TEXT(" :: Variable=") + Variable.VarName.ToString());
		}
		TArray<UEdGraphNode*> Nodes;
		FBlueprintEditorUtils::GetAllNodesOfClass<UEdGraphNode>(Blueprint, Nodes);
		for (const UEdGraphNode* Node : Nodes)
		{
			if (!Node) continue;
			for (const UEdGraphPin* Pin : Node->Pins)
			{
				const FString CacheField = InstanceCacheField(Node, Pin);
				if (!CacheField.IsEmpty())
				{
					const bool bConnected = !Pin->LinkedTo.IsEmpty();
					const bool bAuthored = Pin->Direction == EGPD_Input && !bConnected &&
						(!Pin->DefaultTextValue.IsEmpty() || Pin->DefaultValue != Pin->AutogeneratedDefaultValue);
					auto Row = MakeShared<FJsonObject>();
					Row->SetStringField(TEXT("package"), BlueprintAssetData.PackageName.ToString());
					Row->SetStringField(TEXT("field"), CacheField);
					Row->SetStringField(TEXT("kind"), bConnected ? TEXT("Connected") : bAuthored ? TEXT("AuthoredDefault") : TEXT("Surface"));
					InstanceCacheRows.Add(MakeShared<FJsonValueObject>(Row));
				}
				if (Pin && ReferencesContainedStruct(Pin->PinType))
					ContainedStructReferences.Add(FString::Printf(TEXT("%s :: Node=%s :: Pin=%s"),
						*BlueprintAssetData.PackageName.ToString(), *GetNameSafe(Node), *Pin->PinName.ToString()));
				if (Pin && ReferencesWorldSaveStruct(Pin->PinType))
					WorldSaveStructReferences.Add(FString::Printf(TEXT("%s :: Node=%s :: Pin=%s"),
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
	for (const FString& Reference : WorldSaveStructReferences)
		AddError(FString::Printf(TEXT("5G85 serialized world struct usage requires review: %s"), *Reference));
	TestEqual(TEXT("No native Blueprint directly uses the two serialized world structs"), WorldSaveStructReferences.Num(), 0);
	TestTrue(TEXT("Native Blueprint inventory is not empty"), BlueprintAssets.Num() > 0);
	TestEqual(TEXT("No Blueprint calls retired item identity APIs, historical inventory registry or weight-bearing pawn grant"), LegacyReferences.Num(), 0);
	TestNull(TEXT("Retired setter is no longer reflected"), AGridLevelEditorActor::StaticClass()->FindFunctionByName(TEXT("SetSelectedObjectItemDefinitionAsset")));
	TestNull(TEXT("Retired sync is no longer reflected"), AGridLevelEditorActor::StaticClass()->FindFunctionByName(TEXT("SyncSelectedItemDefinitionIdFromAsset")));
	TestNull(TEXT("Historical inventory registration is no longer reflected"), UGridPartyInventoryComponent::StaticClass()->FindFunctionByName(TEXT("RegisterItemDefinition")));
	TestNull(TEXT("Historical inventory lookup is no longer reflected"), UGridPartyInventoryComponent::StaticClass()->FindFunctionByName(TEXT("FindItemDefinition")));
	TestNull(TEXT("Obsolete weight-bearing pawn grant is no longer reflected"),
		AGrimrockPartyPawn::StaticClass()->FindFunctionByName(TEXT("AddRuntimeItemToSelectedCharacterInventory")));
	TestNotNull(TEXT("Physical pawn grant remains reflected"),
		AGrimrockPartyPawn::StaticClass()->FindFunctionByName(TEXT("AddItemInstanceToSelectedCharacterInventory")));
	TestNull(TEXT("Historical ID-only receptacle insertion is no longer reflected"),
		AGridReceptacleActor::StaticClass()->FindFunctionByName(TEXT("TryInsertItem")));
	TestNull(TEXT("Historical contained definition pointer no longer reflected"),
		FGridContainedReceptacleItem::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition")));
	TestNotNull(TEXT("Inventory instance projection remains reflected"), UGridPartyInventoryComponent::StaticClass()->FindFunctionByName(TEXT("ApplyItemDefinitionToInstance")));
	TestTrue(TEXT("Instance cache Blueprint evidence exported"), WriteInstanceCacheEvidence(LoadedBlueprintCount, InstanceCacheRows));
	return !HasAnyErrors() && BlueprintAssets.Num() > 0 && LegacyReferences.IsEmpty() &&
		ContainedStructReferences.IsEmpty() && WorldSaveStructReferences.IsEmpty() && LoadedBlueprintCount == BlueprintAssets.Num();
}

#endif // WITH_DEV_AUTOMATION_TESTS

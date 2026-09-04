#if WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/AssetRegistryModule.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMaterialOwnershipAuditTest,
	"Grimrock.Architecture.MaterialOwnership.Audit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMaterialOwnershipAuditTest::RunTest(const FString& Parameters)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game/GrimrockPrototype")));
	Filter.ClassPaths.Add(UGridObjectArchetypeAsset::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> AssetDatas;
	AssetRegistry.GetAssets(Filter, AssetDatas);

	if (AssetDatas.IsEmpty())
	{
		AddError(TEXT("No UGridObjectArchetypeAsset was found under /Game/GrimrockPrototype; the material ownership audit cannot be considered valid."));
		return false;
	}

	int32 ArchetypesWithOverrides = 0;
	for (const FAssetData& AssetData : AssetDatas)
	{
		const UGridObjectArchetypeAsset* Archetype = Cast<UGridObjectArchetypeAsset>(AssetData.GetAsset());
		if (!Archetype)
		{
			AddError(FString::Printf(TEXT("Failed to load archetype asset %s."), *AssetData.GetObjectPathString()));
			continue;
		}

		TArray<FString> OverrideFields;
		if (Archetype->PreviewMaterial)
		{
			OverrideFields.Add(TEXT("PreviewMaterial"));
		}
		if (Archetype->FixedMaterial)
		{
			OverrideFields.Add(TEXT("FixedMaterial"));
		}
		if (Archetype->MovingMaterial)
		{
			OverrideFields.Add(TEXT("MovingMaterial"));
		}
		if (Archetype->PitLeftLeafMaterial)
		{
			OverrideFields.Add(TEXT("PitLeftLeafMaterial"));
		}
		if (Archetype->PitRightLeafMaterial)
		{
			OverrideFields.Add(TEXT("PitRightLeafMaterial"));
		}

		if (!OverrideFields.IsEmpty())
		{
			++ArchetypesWithOverrides;
			AddError(FString::Printf(
				TEXT("%s uses archetype material override(s): %s. Move the material(s) to the corresponding Static Mesh Material Slots before removing the legacy fields."),
				*AssetData.GetObjectPathString(), *FString::Join(OverrideFields, TEXT(", "))));
		}
	}

	AddInfo(FString::Printf(TEXT("Material ownership audit: %d archetype asset(s) inspected, %d with material override(s)."), AssetDatas.Num(), ArchetypesWithOverrides));
	return ArchetypesWithOverrides == 0;
}

#endif // WITH_DEV_AUTOMATION_TESTS

#if WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/AssetRegistryModule.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"

namespace
{
	FString DescribeMaterialSlots(const UStaticMesh* Mesh)
	{
		if (!Mesh)
		{
			return TEXT("<no mesh>");
		}

		const TArray<FStaticMaterial>& StaticMaterials = Mesh->GetStaticMaterials();
		if (StaticMaterials.IsEmpty())
		{
			return TEXT("<no material slots>");
		}

		TArray<FString> SlotDescriptions;
		SlotDescriptions.Reserve(StaticMaterials.Num());
		for (int32 Index = 0; Index < StaticMaterials.Num(); ++Index)
		{
			const UMaterialInterface* SlotMaterial = StaticMaterials[Index].MaterialInterface;
			SlotDescriptions.Add(FString::Printf(TEXT("%d:%s"), Index, SlotMaterial ? *SlotMaterial->GetPathName() : TEXT("<None>")));
		}

		return FString::Join(SlotDescriptions, TEXT(" | "));
	}

	bool MeshContainsMaterial(const UStaticMesh* Mesh, const UMaterialInterface* Material)
	{
		if (!Mesh || !Material)
		{
			return false;
		}

		for (const FStaticMaterial& StaticMaterial : Mesh->GetStaticMaterials())
		{
			if (StaticMaterial.MaterialInterface == Material)
			{
				return true;
			}
		}

		return false;
	}
}

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

	auto ReportOverride = [this](const FAssetData& AssetData, const TCHAR* FieldName, const UStaticMesh* Mesh, const UMaterialInterface* Material)
	{
		const FString MeshPath = Mesh ? Mesh->GetPathName() : TEXT("<None>");
		const FString MaterialPath = Material ? Material->GetPathName() : TEXT("<None>");
		const bool bAlreadyInMeshSlots = MeshContainsMaterial(Mesh, Material);
		const FString SlotDescription = DescribeMaterialSlots(Mesh);

		AddError(FString::Printf(
			TEXT("%s Field=%s Mesh=%s OverrideMaterial=%s AlreadyInMeshSlots=%s MeshSlots=[%s]"),
			*AssetData.GetObjectPathString(), FieldName, *MeshPath, *MaterialPath, bAlreadyInMeshSlots ? TEXT("YES") : TEXT("NO"), *SlotDescription));
	};

	int32 ArchetypesWithOverrides = 0;
	for (const FAssetData& AssetData : AssetDatas)
	{
		const UGridObjectArchetypeAsset* Archetype = Cast<UGridObjectArchetypeAsset>(AssetData.GetAsset());
		if (!Archetype)
		{
			AddError(FString::Printf(TEXT("Failed to load archetype asset %s."), *AssetData.GetObjectPathString()));
			continue;
		}

		bool bHasOverride = false;
		if (Archetype->PreviewMaterial)
		{
			bHasOverride = true;
			ReportOverride(AssetData, TEXT("PreviewMaterial"), Archetype->PreviewMesh.Get(), Archetype->PreviewMaterial.Get());
		}
		if (Archetype->FixedMaterial)
		{
			bHasOverride = true;
			ReportOverride(AssetData, TEXT("FixedMaterial"), Archetype->FixedMesh.Get(), Archetype->FixedMaterial.Get());
		}
		if (Archetype->MovingMaterial)
		{
			bHasOverride = true;
			const UStaticMesh* EffectiveMovingMesh = Archetype->MovingMesh ? Archetype->MovingMesh.Get() : Archetype->PreviewMesh.Get();
			ReportOverride(AssetData, TEXT("MovingMaterial"), EffectiveMovingMesh, Archetype->MovingMaterial.Get());
		}
		if (Archetype->PitLeftLeafMaterial)
		{
			bHasOverride = true;
			ReportOverride(AssetData, TEXT("PitLeftLeafMaterial"), Archetype->PitLeftLeafMesh.Get(), Archetype->PitLeftLeafMaterial.Get());
		}
		if (Archetype->PitRightLeafMaterial)
		{
			bHasOverride = true;
			ReportOverride(AssetData, TEXT("PitRightLeafMaterial"), Archetype->PitRightLeafMesh.Get(), Archetype->PitRightLeafMaterial.Get());
		}

		if (bHasOverride)
		{
			++ArchetypesWithOverrides;
		}
	}

	AddInfo(FString::Printf(TEXT("Material ownership audit: %d archetype asset(s) inspected, %d with material override(s)."), AssetDatas.Num(), ArchetypesWithOverrides));
	return ArchetypesWithOverrides == 0;
}

#endif // WITH_DEV_AUTOMATION_TESTS

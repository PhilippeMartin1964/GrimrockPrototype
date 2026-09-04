#if WITH_DEV_AUTOMATION_TESTS

#include "Core/GridObjectArchetypeAsset.h"
#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "Subsystems/EditorAssetSubsystem.h"

namespace
{
	struct FMaterialOwnershipMigrationEntry
	{
		const TCHAR* ArchetypeAssetPath;
		const TCHAR* ExpectedMaterialPath;
		const TCHAR* DedicatedMeshAssetPath;
	};

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMaterialOwnershipMigrationTest,
	"Grimrock.Architecture.MaterialOwnership.MigrateFloorDecorations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMaterialOwnershipMigrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	if (!GEditor)
	{
		AddError(TEXT("GEditor is unavailable; floor-decoration material migration requires the Unreal Editor."));
		return false;
	}

	UEditorAssetSubsystem* AssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	if (!AssetSubsystem)
	{
		AddError(TEXT("UEditorAssetSubsystem is unavailable."));
		return false;
	}

	static const FMaterialOwnershipMigrationEntry Entries[] = {
		{ TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_FloorRuneCircle"),
			TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Decorations/MI_FloorRuneCircle_Blue"), nullptr },
		{ TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_FloorRubble"),
			TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Wall/MI_Wall_Stone_02"), nullptr },
		{ TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_FloorRoots"),
			TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Decorations/MI_FloorRoots_01"),
			TEXT("/Game/GrimrockPrototype/Meshes/Decorations/Floor/SM_FloorRoots_01") },
		{ TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_FloorMoss"),
			TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Decorations/MI_FloorMoss_01"), nullptr },
		{ TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_FloorDust"),
			TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Decorations/MI_FloorDust_01"),
			TEXT("/Game/GrimrockPrototype/Meshes/Decorations/Floor/SM_FloorDust_01") },
		{ TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_FloorDebris"),
			TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Wall/MI_Wall_Stone_02"), nullptr },
		{ TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_FloorCarpet"),
			TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Decorations/MI_FloorCarpet_01"), nullptr },
		{ TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_FloorBloodStain"),
			TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Decorations/MI_FloorBloodStain_01"),
			TEXT("/Game/GrimrockPrototype/Meshes/Decorations/Floor/SM_FloorBloodStain_01") }
	};

	static const FString SharedDecalMeshPath(TEXT("/Game/GrimrockPrototype/Meshes/Decorations/Floor/SM_FloorDecalPlane_01"));
	const int32 ExpectedArchetypeCount = static_cast<int32>(UE_ARRAY_COUNT(Entries));

	int32 MigratedArchetypeCount = 0;
	int32 CreatedDedicatedMeshCount = 0;

	for (const FMaterialOwnershipMigrationEntry& Entry : Entries)
	{
		UGridObjectArchetypeAsset* Archetype = Cast<UGridObjectArchetypeAsset>(AssetSubsystem->LoadAsset(Entry.ArchetypeAssetPath));
		UMaterialInterface* ExpectedMaterial = Cast<UMaterialInterface>(AssetSubsystem->LoadAsset(Entry.ExpectedMaterialPath));
		if (!Archetype || !ExpectedMaterial)
		{
			AddError(FString::Printf(TEXT("Unable to load migration input Archetype=%s Material=%s."), Entry.ArchetypeAssetPath, Entry.ExpectedMaterialPath));
			continue;
		}

		UStaticMesh* TargetMesh = Archetype->PreviewMesh.Get();

		if (Entry.DedicatedMeshAssetPath)
		{
			if (!AssetSubsystem->DoesAssetExist(Entry.DedicatedMeshAssetPath))
			{
				UObject* Duplicate = AssetSubsystem->DuplicateAsset(SharedDecalMeshPath, Entry.DedicatedMeshAssetPath);
				if (!Duplicate)
				{
					AddError(FString::Printf(TEXT("Failed to duplicate %s to %s."), *SharedDecalMeshPath, Entry.DedicatedMeshAssetPath));
					continue;
				}
				++CreatedDedicatedMeshCount;
			}

			TargetMesh = Cast<UStaticMesh>(AssetSubsystem->LoadAsset(Entry.DedicatedMeshAssetPath));
			if (!TargetMesh)
			{
				AddError(FString::Printf(TEXT("Dedicated mesh %s could not be loaded as UStaticMesh."), Entry.DedicatedMeshAssetPath));
				continue;
			}

			TargetMesh->Modify();
			TargetMesh->SetMaterial(0, ExpectedMaterial);
			TargetMesh->MarkPackageDirty();
			if (!AssetSubsystem->SaveLoadedAsset(TargetMesh, false))
			{
				AddError(FString::Printf(TEXT("Failed to save dedicated mesh %s."), Entry.DedicatedMeshAssetPath));
				continue;
			}
		}
		else if (!MeshContainsMaterial(TargetMesh, ExpectedMaterial))
		{
			AddError(FString::Printf(
				TEXT("Archetype %s was expected to already carry %s in its Static Mesh Material Slots, but it does not. Migration aborted for this asset."),
				Entry.ArchetypeAssetPath, Entry.ExpectedMaterialPath));
			continue;
		}

		if (!TargetMesh || !MeshContainsMaterial(TargetMesh, ExpectedMaterial))
		{
			AddError(FString::Printf(TEXT("Target mesh for %s does not contain expected material %s after migration."), Entry.ArchetypeAssetPath, Entry.ExpectedMaterialPath));
			continue;
		}

		Archetype->Modify();
		Archetype->PreviewMesh = TargetMesh;
		Archetype->PreviewMaterial = nullptr;
		Archetype->MarkPackageDirty();
		if (!AssetSubsystem->SaveLoadedAsset(Archetype, false))
		{
			AddError(FString::Printf(TEXT("Failed to save migrated archetype %s."), Entry.ArchetypeAssetPath));
			continue;
		}

		++MigratedArchetypeCount;
		AddInfo(FString::Printf(TEXT("Migrated %s -> Mesh=%s MaterialSlot0=%s"), Entry.ArchetypeAssetPath, *TargetMesh->GetPathName(), *ExpectedMaterial->GetPathName()));
	}

	TestEqual(TEXT("All eight floor-decoration archetypes migrated"), MigratedArchetypeCount, ExpectedArchetypeCount);
	AddInfo(FString::Printf(TEXT("Material ownership migration complete: %d archetype(s), %d dedicated mesh asset(s) created."),
		MigratedArchetypeCount, CreatedDedicatedMeshCount));
	return MigratedArchetypeCount == ExpectedArchetypeCount;
}

#endif // WITH_DEV_AUTOMATION_TESTS

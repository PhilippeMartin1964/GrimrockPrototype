#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/Combat/GridWeaponVisualResolver.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInterface.h"

namespace ARMEMENT0163
{
	const TCHAR* CataloguePath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue");

	struct FFinalPilot
	{
		const TCHAR* ArchetypeId;
		const TCHAR* VisualId;
		const TCHAR* Family;
		const TCHAR* MeshName;
		const TCHAR* IconName;
		const TCHAR* MaterialInstanceName;
		const TCHAR* TextureFolder;
		const TCHAR* TextureStem;
	};

	const FFinalPilot Pilots[] =
	{
		{TEXT("ARM001"), TEXT("VIS_Dagger_01"), TEXT("Light"), TEXT("SM_Weapon_Dagger_01"),
			TEXT("T_Weapon_Dagger_01"), TEXT("MI_Dagger"), TEXT("Dagger"), TEXT("Dagger")},
		{TEXT("ARM045"), TEXT("VIS_ShortBow_01"), TEXT("Ranged"), TEXT("SM_Weapon_ShortBow_01"),
			TEXT("T_Weapon_ShortBow_01"), TEXT("MI_Bow"), TEXT("Bow"), TEXT("Bow")},
		{TEXT("ARM075"), TEXT("VIS_RoundShield_01"), TEXT("Shield"), TEXT("SM_Weapon_RoundShield_01"),
			TEXT("T_Weapon_RoundShield_01"), TEXT("MI_Shield"), TEXT("Shield"), TEXT("Shield")}
	};

	FString IconPath(const FFinalPilot& Pilot)
	{
		return FString::Printf(TEXT("/Game/GrimrockPrototype/UI/Icons/Weapons/%s.%s"),
			Pilot.IconName, Pilot.IconName);
	}

	FString MeshPath(const FFinalPilot& Pilot)
	{
		return FString::Printf(TEXT("/Game/GrimrockPrototype/Meshes/Weapons/%s.%s"),
			Pilot.MeshName, Pilot.MeshName);
	}

	FString MaterialPath(const FFinalPilot& Pilot)
	{
		return FString::Printf(TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Weapons/%s.%s"),
			Pilot.MaterialInstanceName, Pilot.MaterialInstanceName);
	}

	FString MapPath(const FFinalPilot& Pilot, const TCHAR* Suffix)
	{
		return FString::Printf(
			TEXT("/Game/GrimrockPrototype/Art/Textures/Final/Weapons/%s/T_%s_01_%s.T_%s_01_%s"),
			Pilot.TextureFolder, Pilot.TextureStem, Suffix, Pilot.TextureStem, Suffix);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0163MaterializedAssetTest,
	"Grimrock.RPG.ARMEMENT01.6.Pilot.MaterializedAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0163MaterializedAssetTest::RunTest(const FString&)
{
	const UGridWeaponVisualCatalogueAsset* Catalogue =
		LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, ARMEMENT0163::CataloguePath);
	if (!TestNotNull(TEXT("Realistic visual catalogue exists"), Catalogue)) { return false; }
	TestTrue(TEXT("Final catalogue is structurally valid"), Catalogue->IsValidCatalogue());
	TestEqual(TEXT("Exactly three approved visual profiles"), Catalogue->VisualProfiles.Num(), 3);
	TestEqual(TEXT("Exactly three approved archetype bindings"), Catalogue->ArchetypeBindings.Num(), 3);
	for (const ARMEMENT0163::FFinalPilot& Pilot : ARMEMENT0163::Pilots)
	{
		const FGridWeaponVisualProfile* Profile =
			Catalogue->FindProfileForArchetype(FName(Pilot.ArchetypeId));
		if (!TestNotNull(*FString::Printf(TEXT("Profile %s"), Pilot.ArchetypeId), Profile)) { continue; }
		TestEqual(TEXT("Approved numeric VisualId"), Profile->VisualId, FName(Pilot.VisualId));
		TestEqual(TEXT("Art family preserved"), Profile->FamilyTag, FName(Pilot.Family));
		TestEqual(TEXT("Resolved icon path"), Profile->BaseIcon.ToSoftObjectPath().ToString(),
			ARMEMENT0163::IconPath(Pilot));
		TestEqual(TEXT("Resolved mesh path"), Profile->WorldMesh.ToSoftObjectPath().ToString(),
			ARMEMENT0163::MeshPath(Pilot));

		const UTexture2D* Icon = Profile->BaseIcon.LoadSynchronous();
		const UStaticMesh* Mesh = Profile->WorldMesh.LoadSynchronous();
		if (TestNotNull(TEXT("Art icon loads"), Icon))
		{
			const FIntPoint Size = Icon->GetImportedSize();
			TestTrue(TEXT("Art icon is square with a positive imported resolution"),
				Size.X > 0 && Size.X == Size.Y);
		}
		if (TestNotNull(TEXT("Realistic mesh loads"), Mesh))
		{
			TestTrue(TEXT("Realistic mesh has render geometry"), Mesh->GetNumLODs() > 0);
			TestTrue(TEXT("Realistic mesh bounds are not degenerate"),
				Mesh->GetBounds().SphereRadius > 0.5f);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0163ResolverTest,
	"Grimrock.RPG.ARMEMENT01.6.Pilot.ProductionResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0163ResolverTest::RunTest(const FString&)
{
	UGridWeaponVisualCatalogueAsset* Catalogue =
		LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, ARMEMENT0163::CataloguePath);
	if (!TestNotNull(TEXT("Approved visual catalogue exists"), Catalogue)) { return false; }
	for (const ARMEMENT0163::FFinalPilot& Pilot : ARMEMENT0163::Pilots)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>();
		Definition->ItemDefinitionId = TEXT("ARM0163_FinalReadOnly");
		Definition->WeaponArchetypeId = FName(Pilot.ArchetypeId);
		Definition->bUseWeaponVisualCatalogue = true;
		Definition->WeaponVisualCatalogue = Catalogue;

		FGridResolvedWeaponVisual Visual;
		EGridWeaponVisualResolutionError Error = EGridWeaponVisualResolutionError::CatalogueUnavailable;
		TestTrue(TEXT("Realistic profile resolves"), FGridWeaponVisualResolver::TryResolve(
			*Definition, nullptr, Visual, Error));
		TestTrue(TEXT("No resolver errors"), Error == EGridWeaponVisualResolutionError::None);
		TestEqual(TEXT("Approved numeric VisualId preserved"), Visual.VisualId, FName(Pilot.VisualId));
		TestFalse(TEXT("No fabricated item level for editor preview"), Visual.bHasItemLevel);
		TestFalse(TEXT("No Legacy fallback for approved profile"), Visual.bLegacyFallback);
		TestTrue(TEXT("Equipped presentation has a valid visual mesh reference"),
			!Visual.EquippedMesh.IsNull());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0163PassiveTest,
	"Grimrock.RPG.ARMEMENT01.6.Pilot.PassiveShieldNoAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0163PassiveTest::RunTest(const FString&)
{
	UGridWeaponVisualCatalogueAsset* Catalogue =
		LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, ARMEMENT0163::CataloguePath);
	if (!TestNotNull(TEXT("Approved visual catalogue exists"), Catalogue)) { return false; }
	TestNotNull(TEXT("ARM075 shield has its own visual profile"),
		Catalogue->FindProfileForArchetype(TEXT("ARM075")));

	UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>();
	Definition->ItemDefinitionId = TEXT("ARM0163_FinalRoundShield");
	Definition->ItemType = EGridItemType::Shield;
	Definition->WeaponArchetypeId = TEXT("ARM075");
	Definition->bUseWeaponVisualCatalogue = true;
	Definition->WeaponVisualCatalogue = Catalogue;
	TestTrue(TEXT("Shield definition is valid"), Definition->IsValidDefinition());
	TestFalse(TEXT("Appearance does not enable RPG catalogue"), Definition->bUseWeaponCatalogue);
	TestFalse(TEXT("Appearance does not add a main-hand attack"),
		Definition->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("Appearance does not add combat actions"), Definition->CombatActions.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0163MaterialDepsTest,
	"Grimrock.RPG.ARMEMENT01.6.Pilot.MaterialDependencies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0163MaterialDepsTest::RunTest(const FString&)
{
	for (const ARMEMENT0163::FFinalPilot& Pilot : ARMEMENT0163::Pilots)
	{
		UMaterialInstance* Instance = LoadObject<UMaterialInstance>(
			nullptr, *ARMEMENT0163::MaterialPath(Pilot));
		if (!TestNotNull(*FString::Printf(TEXT("PBR instance for %s"), Pilot.ArchetypeId),
			Instance)) { continue; }
		TestNotNull(TEXT("Material instance has a parent"), Instance->Parent);

		const TCHAR* Suffixes[] = {TEXT("BC"), TEXT("N"), TEXT("ORM")};
		for (const TCHAR* Suffix : Suffixes)
		{
			UTexture2D* Texture = LoadObject<UTexture2D>(
				nullptr, *ARMEMENT0163::MapPath(Pilot, Suffix));
			if (!TestNotNull(
				*FString::Printf(TEXT("%s PBR map for %s"), Suffix, Pilot.ArchetypeId), Texture))
			{
				continue;
			}
			const FIntPoint ImportedSize = Texture->GetImportedSize();
			TestTrue(TEXT("PBR map has positive imported dimensions"),
				ImportedSize.X > 0 && ImportedSize.Y > 0);
			if (FCString::Strcmp(Suffix, TEXT("ORM")) == 0)
			{
				TestFalse(TEXT("ORM is linear data: sRGB disabled"), Texture->SRGB);
			}
			else if (FCString::Strcmp(Suffix, TEXT("N")) == 0)
			{
				TestFalse(TEXT("Normal map is not sRGB"), Texture->SRGB);
			}
		}

		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *ARMEMENT0163::MeshPath(Pilot));
		if (!TestNotNull(TEXT("Material owner mesh exists"), Mesh)) { continue; }
		const TArray<FStaticMaterial>& Slots = Mesh->GetStaticMaterials();
		TestTrue(TEXT("Mesh owns nonempty Material Slots"), !Slots.IsEmpty());
		bool bUsesApprovedMaterial = false;
		for (const FStaticMaterial& Slot : Slots)
		{
			if (TestNotNull(TEXT("Every Material Slot resolves"), Slot.MaterialInterface) &&
				Slot.MaterialInterface == Instance)
			{
				bUsesApprovedMaterial = true;
			}
		}
		TestTrue(*FString::Printf(TEXT("%s uses its approved material instance"),
			Pilot.ArchetypeId), bUsesApprovedMaterial);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

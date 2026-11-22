#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInterface.h"

namespace ARMEMENT0163
{
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
    FName FFinalPilotId(const FFinalPilot& Pilot)
    {
        if (FCString::Strcmp(Pilot.ArchetypeId, TEXT("ARM001")) == 0)
            return TEXT("Weapon_Dagger_01");
        if (FCString::Strcmp(Pilot.ArchetypeId, TEXT("ARM045")) == 0)
            return TEXT("Weapon_ShortBow_01");
        return TEXT("Weapon_RoundShield_01");
    }
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0163MaterializedAssetTest,
    "Grimrock.RPG.ARMEMENT01.6.Pilot.MaterializedAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT0163MaterializedAssetTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Production canonical visual authority configured"), Fixture.bReady))
        return false;
    UGridItemSystemAsset* Root = GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Single installed native visual catalogue"), Root) ||
        !TestNotNull(TEXT("Real canonical visual source"), Root->WeaponVisualCatalogue.Get()))
        return false;
    for (const ARMEMENT0163::FFinalPilot& Pilot : ARMEMENT0163::Pilots)
    {
        const FName ArmId(Pilot.ArchetypeId);
        const FGridWeaponVisual* Profile =
            Root->WeaponVisualCatalogue->FindForArchetype(ArmId);
        if (!TestNotNull(TEXT("Actual production pilot profile"), Profile)) continue;
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("Pilot N1 resolves from sole ItemSystem"),
            Fixture.Resolve(ARMEMENT0163::FFinalPilotId(Pilot), 1, Item))) continue;
        TestEqual(TEXT("Exact visual identity"), Profile->VisualId, FName(Pilot.VisualId));
        TestEqual(TEXT("Exact art family"), Profile->FamilyTag, FName(Pilot.Family));
        TestEqual(TEXT("Author-approved icon path"),
            Item.ItemIcon.ToSoftObjectPath().ToString(), ARMEMENT0163::IconPath(Pilot));
        TestEqual(TEXT("Author-approved mesh path"),
            Item.WorldMesh.ToSoftObjectPath().ToString(), ARMEMENT0163::MeshPath(Pilot));
        const UTexture2D* Icon = Item.ItemIcon.LoadSynchronous();
        const UStaticMesh* Mesh = Item.WorldMesh.LoadSynchronous();
        if (TestNotNull(TEXT("Real icon loads"), Icon))
        {
            const FIntPoint Size = Icon->GetImportedSize();
            TestTrue(TEXT("RGBA icon is square"), Size.X > 0 && Size.X == Size.Y);
        }
        if (TestNotNull(TEXT("Real world mesh loads"), Mesh))
        {
            TestTrue(TEXT("Real geometry present"), Mesh->GetNumLODs() > 0);
            TestTrue(TEXT("Mesh bounding radius physical"), Mesh->GetBounds().SphereRadius > 0.5f);
        }
    }
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0163ResolverTest,
    "Grimrock.RPG.ARMEMENT01.6.Pilot.ProductionResolver",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT0163ResolverTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Canonical visual registry configured"), Fixture.bReady)) return false;
    for (const ARMEMENT0163::FFinalPilot& Pilot : ARMEMENT0163::Pilots)
    {
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("Canonical N1 visually resolves"),
            Fixture.Resolve(ARMEMENT0163::FFinalPilotId(Pilot), 1, Item))) continue;
        TestEqual(TEXT("Stable visual identity"), Item.VisualId, FName(Pilot.VisualId));
        TestTrue(TEXT("One equipped mesh is authored"),
            Item.EquippedMesh.LoadSynchronous() != nullptr);
        TestTrue(TEXT("World offset contains no invalid data"),
            !Item.WorldVisualTransform.ContainsNaN());
        FGridResolvedItem N20;
        TestTrue(TEXT("Same art asset from N1 to N20"),
            Fixture.Resolve(Item.ItemId, 20, N20) &&
            N20.VisualId == Item.VisualId &&
            N20.WorldMesh.ToSoftObjectPath() == Item.WorldMesh.ToSoftObjectPath());
    }
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0163PassiveTest,
    "Grimrock.RPG.ARMEMENT01.6.Pilot.PassiveShieldNoAttack",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT0163PassiveTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Registered ItemSystem"), Fixture.bReady)) return false;
    FGridResolvedItem Shield;
    if (!TestTrue(TEXT("True N1 RoundShield resolves"),
        Fixture.Resolve(TEXT("Weapon_RoundShield_01"), 1, Shield))) return false;
    TestEqual(TEXT("Shield visual is unique"), Shield.VisualId, FName(TEXT("VIS_RoundShield_01")));
    TestEqual(TEXT("Actual passive shield kind"),
        Shield.Weapon.Kind, EGridArmamentKind::Shield);
    TestTrue(TEXT("No attacking bindings"), Shield.AvailableActions.IsEmpty());
    TestFalse(TEXT("No MainHand attack"), Shield.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::MainHand));
    TestFalse(TEXT("No OffHand attack"), Shield.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::OffHand));
    TestTrue(TEXT("Equippable OffHand"), Shield.Weapon.CanEquipInHand(EGridEquipmentSlot::OffHand));
    return !HasAnyErrors();
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
		TestNotNull(TEXT("Material instance has a parent"), Instance->Parent.Get());

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
			if (TestNotNull(TEXT("Every Material Slot resolves"), Slot.MaterialInterface.Get()) &&
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

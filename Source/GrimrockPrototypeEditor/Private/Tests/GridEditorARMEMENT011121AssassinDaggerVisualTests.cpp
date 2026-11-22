#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridAssassinDaggerVisualSpecs.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstance.h"

namespace ARMEMENT011121Tests
{
    bool CheckMaterialOwnership(const ARMEMENT011121::FAssassinVisualSpec& Spec, UStaticMesh* Mesh)
    {
        UMaterialInstance* MI =
            LoadObject<UMaterialInstance>(nullptr, *ARMEMENT011121::MaterialPath(Spec));
        if (!MI || !IsValid(MI->Parent) || !Mesh) return false;
        bool bFound = false;
        for (const FStaticMaterial& Slot : Mesh->GetStaticMaterials())
        {
            if (!IsValid(Slot.MaterialInterface)) return false;
            bFound |= Slot.MaterialInterface == MI;
        }
        return bFound;
    }
}

// N1 combat, visual binding and PBR now come exclusively from ItemSystem.
// The original artist files remain unchanged; never regenerate placeholders.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT011121VisualCatalogueIntegrity,
    "Grimrock.RPG.ARMEMENT01.11.2.Visual.AssassinDaggerAssetContracts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT011121VisualCatalogueIntegrity::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Native installed ItemSystem"), Fixture.bReady)) return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Real ItemSystem root"), Root) ||
        !TestNotNull(TEXT("Canonical weapon number catalogue"), Root->WeaponCatalogue.Get()) ||
        !TestNotNull(TEXT("Canonical visual catalogue"), Root->WeaponVisualCatalogue.Get()))
        return false;
    const bool bNumericDirty = Root->WeaponCatalogue->GetOutermost()->IsDirty();
    const bool bVisualDirty = Root->WeaponVisualCatalogue->GetOutermost()->IsDirty();
    const struct {const TCHAR* Arm;const TCHAR* Item;const TCHAR* Visual;} Pilots[] =
    {
        {TEXT("ARM001"),TEXT("Weapon_Dagger_01"),TEXT("VIS_Dagger_01")},
        {TEXT("ARM045"),TEXT("Weapon_ShortBow_01"),TEXT("VIS_ShortBow_01")},
        {TEXT("ARM075"),TEXT("Weapon_RoundShield_01"),TEXT("VIS_RoundShield_01")}
    };
    for (const auto& Pilot : Pilots)
    {
        FGridResolvedItem Item;
        TestTrue(TEXT("Pilot concrete source and immutable visual binding"),
            Fixture.Resolve(FName(Pilot.Item), 1, Item) &&
            Item.ArchetypeId == FName(Pilot.Arm) &&
            Item.VisualId == FName(Pilot.Visual));
    }
    TSet<FName> SeenArms, SeenVisuals;
    int32 Checked = 0;
    for (const auto& Spec : ARMEMENT011121::Specs)
    {
        const FName Archetype(Spec.ArchetypeId), VisualId(Spec.VisualId);
        TestFalse(TEXT("No duplicate Assassin Dagger ARM"), SeenArms.Contains(Archetype));
        TestFalse(TEXT("No duplicate Assassin Dagger VisualId"), SeenVisuals.Contains(VisualId));
        SeenArms.Add(Archetype);
        SeenVisuals.Add(VisualId);
        const FGridWeaponVisual* Visual =
            Root->WeaponVisualCatalogue->FindForArchetype(Archetype);
        if (!TestNotNull(TEXT("Approved final native AssassinDagger visual"), Visual))
            continue;
        FGridResolvedItem N1;
        if (!TestTrue(TEXT("ARM002 N1 resolves from unique ItemSystem"),
            Fixture.Resolve(TEXT("Weapon_AssassinDagger_01"), 1, N1)))
            continue;
        ++Checked;
        TestTrue(TEXT("One canonical physical ARM and exact visual ID"),
            N1.bIsArmament && N1.ArchetypeId == Archetype &&
            N1.VisualId == VisualId && Visual->VisualId == VisualId &&
            Visual->FamilyTag == FName(Spec.FamilyTag));
        TestEqual(TEXT("Actual approved icon path"),
            N1.ItemIcon.ToSoftObjectPath().ToString(), ARMEMENT011121::IconPath(Spec));
        TestEqual(TEXT("Actual approved mesh path"),
            N1.WorldMesh.ToSoftObjectPath().ToString(), ARMEMENT011121::MeshPath(Spec));
        UTexture2D* Icon = N1.ItemIcon.LoadSynchronous();
        UStaticMesh* Mesh = N1.WorldMesh.LoadSynchronous();
        if (!TestNotNull(TEXT("Approved AssassinDagger icon loads"), Icon) ||
            !TestNotNull(TEXT("Approved AssassinDagger mesh loads"), Mesh))
            continue;
        const FIntPoint Size = Icon->GetImportedSize();
        TestTrue(TEXT("Square artist icon >=256"), Size.X >= 256 && Size.Y == Size.X);
        TestTrue(TEXT("Mesh has real render geometry"), Mesh->GetNumLODs() > 0);
        TestTrue(TEXT("Approved MaterialInstance belongs to StaticMesh slots"),
            ARMEMENT011121Tests::CheckMaterialOwnership(Spec, Mesh));
        for (const TCHAR* Suffix : {TEXT("BC"),TEXT("N"),TEXT("ORM")})
        {
            UTexture2D* Map =
                LoadObject<UTexture2D>(nullptr, *ARMEMENT011121::PbrPath(Spec, Suffix));
            if (!TestNotNull(TEXT("Approved PBR texture loads"), Map)) continue;
            TestEqual(TEXT("Correct PBR sRGB policy"), Map->SRGB,
                FCString::Strcmp(Suffix, TEXT("BC")) == 0);
        }
        FGridResolvedItem N20;
        TestTrue(TEXT("N20 retains same artist silhouettes"),
            Fixture.Resolve(N1.ItemId, 20, N20) &&
            N20.WorldMesh.ToSoftObjectPath() == N1.WorldMesh.ToSoftObjectPath() &&
            N20.VisualId == N1.VisualId);
    }
    TestEqual(TEXT("Exactly one authored AssassinDagger N1 visual"), Checked, 1);
    TestEqual(TEXT("Exactly one AssassinDagger visual ID"), SeenVisuals.Num(), 1);
    TestEqual(TEXT("Numeric catalog remains clean"),
        Root->WeaponCatalogue->GetOutermost()->IsDirty(), bNumericDirty);
    TestEqual(TEXT("Visual catalog remains clean"),
        Root->WeaponVisualCatalogue->GetOutermost()->IsDirty(), bVisualDirty);
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS

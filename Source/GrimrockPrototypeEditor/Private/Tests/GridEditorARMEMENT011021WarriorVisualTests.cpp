#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridWarriorN1VisualSpecs.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "ARMEMENT/GridWarriorN1ItemAuthoringSpecs.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstance.h"

namespace ARMEMENT011021Tests
{
    bool CheckMaterialOwnership(const ARMEMENT011021::FWarriorVisualSpec& Spec, UStaticMesh* Mesh)
    {
        UMaterialInstance* MI =
            LoadObject<UMaterialInstance>(nullptr, *ARMEMENT011021::MaterialPath(Spec));
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT011021VisualCatalogueIntegrity,
    "Grimrock.RPG.ARMEMENT01.10.Visual.WarriorAssetContracts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT011021VisualCatalogueIntegrity::RunTest(const FString&)
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
    for (const auto& Spec : ARMEMENT011021::Specs)
    {
        const FName Archetype(Spec.ArchetypeId), VisualId(Spec.VisualId);
        TestFalse(TEXT("No duplicate Warrior ARM"), SeenArms.Contains(Archetype));
        TestFalse(TEXT("No duplicate Warrior VisualId"), SeenVisuals.Contains(VisualId));
        SeenArms.Add(Archetype);
        SeenVisuals.Add(VisualId);
        const FGridCanonicalWeaponVisual* Visual =
            Root->WeaponVisualCatalogue->FindForArchetype(Archetype);
        if (!TestNotNull(TEXT("Approved final native Warrior visual"), Visual))
            continue;
        const ARMEMENT01102::FWarriorItemSpec* WeaponSpec = nullptr;
        for (const auto& Candidate : ARMEMENT01102::WarriorItems)
            if (FName(Candidate.ArchetypeId) == Archetype)
            {
                WeaponSpec = &Candidate;
                break;
            }
        if (!TestNotNull(TEXT("Approved Warrior canonical ItemId"), WeaponSpec))
            continue;
        FGridResolvedItem N1;
        if (!TestTrue(TEXT("One of four Warrior N1 items resolves canonically"),
            Fixture.Resolve(FName(WeaponSpec->ItemDefinitionId), 1, N1)))
            continue;
        ++Checked;
        TestTrue(TEXT("One canonical physical ARM and exact visual ID"),
            N1.bIsArmament && N1.ArchetypeId == Archetype &&
            N1.VisualId == VisualId && Visual->VisualId == VisualId &&
            Visual->FamilyTag == FName(Spec.FamilyTag));
        TestEqual(TEXT("Actual approved icon path"),
            N1.ItemIcon.ToSoftObjectPath().ToString(), ARMEMENT011021::IconPath(Spec));
        TestEqual(TEXT("Actual approved mesh path"),
            N1.WorldMesh.ToSoftObjectPath().ToString(), ARMEMENT011021::MeshPath(Spec));
        UTexture2D* Icon = N1.ItemIcon.LoadSynchronous();
        UStaticMesh* Mesh = N1.WorldMesh.LoadSynchronous();
        if (!TestNotNull(TEXT("Approved Warrior icon loads"), Icon) ||
            !TestNotNull(TEXT("Approved Warrior mesh loads"), Mesh))
            continue;
        const FIntPoint Size = Icon->GetImportedSize();
        TestTrue(TEXT("Square artist icon >=256"), Size.X >= 256 && Size.Y == Size.X);
        TestTrue(TEXT("Mesh has real render geometry"), Mesh->GetNumLODs() > 0);
        TestTrue(TEXT("Approved MaterialInstance belongs to StaticMesh slots"),
            ARMEMENT011021Tests::CheckMaterialOwnership(Spec, Mesh));
        for (const TCHAR* Suffix : {TEXT("BC"),TEXT("N"),TEXT("ORM")})
        {
            UTexture2D* Map =
                LoadObject<UTexture2D>(nullptr, *ARMEMENT011021::PbrPath(Spec, Suffix));
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
    TestEqual(TEXT("Exactly four authored Warrior N1 visuals"), Checked, 4);
    TestEqual(TEXT("Four distinct approved Warrior visual IDs"), SeenVisuals.Num(), 4);
    TestEqual(TEXT("Numeric catalog remains clean"),
        Root->WeaponCatalogue->GetOutermost()->IsDirty(), bNumericDirty);
    TestEqual(TEXT("Visual catalog remains clean"),
        Root->WeaponVisualCatalogue->GetOutermost()->IsDirty(), bVisualDirty);
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS

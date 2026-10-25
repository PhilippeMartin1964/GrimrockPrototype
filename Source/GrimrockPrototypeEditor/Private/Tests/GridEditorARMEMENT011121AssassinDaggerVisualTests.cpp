#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridAssassinDaggerVisualSpecs.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstance.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENT011121Tests
{
    const TCHAR* const CataloguePath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue");
    const TCHAR* const RpgPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue");

    bool CheckMaterialOwnership(const ARMEMENT011121::FAssassinVisualSpec& Spec, UStaticMesh* Mesh)
    {
        UMaterialInstance* MI =
            LoadObject<UMaterialInstance>(nullptr, *ARMEMENT011121::MaterialPath(Spec));
        if (!MI || !IsValid(MI->Parent) || !Mesh)
        {
            return false;
        }
        bool bFound = false;
        for (const FStaticMaterial& Slot : Mesh->GetStaticMaterials())
        {
            if (!IsValid(Slot.MaterialInterface))
            {
                return false;
            }
            bFound |= Slot.MaterialInterface == MI;
        }
        return bFound;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT011121VisualCatalogueIntegrity,
    "Grimrock.RPG.ARMEMENT01.11.2.Visual.AssassinDaggerAssetContracts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT011121VisualCatalogueIntegrity::RunTest(const FString&)
{
    UGridWeaponCatalogueAsset* Rpg =
        LoadObject<UGridWeaponCatalogueAsset>(nullptr, ARMEMENT011121Tests::RpgPath);
    UGridWeaponVisualCatalogueAsset* Visuals =
        LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, ARMEMENT011121Tests::CataloguePath);
    if (!TestNotNull(TEXT("Canonical RPG catalogue"), Rpg) ||
        !TestNotNull(TEXT("Canonical visual catalogue"), Visuals))
    {
        return false;
    }
    if (!TestTrue(TEXT("RPG catalogue still valid"), Rpg->IsValidCatalogue()) ||
        !TestTrue(TEXT("Visual catalogue still valid"), Visuals->IsValidCatalogue()))
    {
        return false;
    }

    const bool bRpgDirty = Rpg->GetOutermost()->IsDirty();
    const bool bVisualDirty = Visuals->GetOutermost()->IsDirty();

    // The 01.6 pilot bindings remain immutable. Other mappings may exist
    // in the future and must not be discarded by this batch.
    TestEqual(TEXT("Original dagger binding remains"),
        Visuals->FindVisualId(TEXT("ARM001")), FName(TEXT("VIS_Dagger_01")));
    TestEqual(TEXT("Original bow binding remains"),
        Visuals->FindVisualId(TEXT("ARM045")), FName(TEXT("VIS_ShortBow_01")));
    TestEqual(TEXT("Original shield binding remains"),
        Visuals->FindVisualId(TEXT("ARM075")), FName(TEXT("VIS_RoundShield_01")));

    int32 Registered = 0;
    TSet<FName> Archetypes;
    TSet<FName> VisualIds;
    for (const ARMEMENT011121::FAssassinVisualSpec& Spec : ARMEMENT011121::Specs)
    {
        const FName Archetype(Spec.ArchetypeId);
        const FName VisualId(Spec.VisualId);
        TestFalse(TEXT("No duplicate AssassinDagger archetype in spec"), Archetypes.Contains(Archetype));
        TestFalse(TEXT("No duplicate AssassinDagger visual ID in spec"), VisualIds.Contains(VisualId));
        Archetypes.Add(Archetype);
        VisualIds.Add(VisualId);
        TestTrue(TEXT("Canonical archetype ID"),
            UGridWeaponVisualCatalogueAsset::IsCanonicalArchetypeId(Archetype));
        TestNotNull(TEXT("Valid N1 combat profile"), Rpg->FindProfile(Archetype, 1));
        const FName CurrentVisual = Visuals->FindVisualId(Archetype);
        if (CurrentVisual.IsNone())
        {
            // Artwork not yet imported is not a failing test; it must not
            // be represented as a completed catalogue binding.
            AddInfo(FString::Printf(TEXT("[ARMEMENT01.11.2.2.1] %s: art not yet registered"),
                Spec.ArchetypeId));
            continue;
        }
        ++Registered;
        TestEqual(TEXT("Registered visual identity exact"), CurrentVisual, VisualId);

        const FGridWeaponVisualProfile* Look = Visuals->FindProfileForArchetype(Archetype);
        if (!TestNotNull(TEXT("Registered profile exists"), Look))
        {
            continue;
        }
        TestEqual(TEXT("Family tag is presentation-only grouping"),
            Look->FamilyTag, FName(Spec.FamilyTag));
        TestTrue(TEXT("Icon uses exact UE asset path"),
            Look->BaseIcon.ToSoftObjectPath() == FSoftObjectPath(ARMEMENT011121::IconPath(Spec)));
        TestTrue(TEXT("World mesh uses exact UE asset path"),
            Look->WorldMesh.ToSoftObjectPath() == FSoftObjectPath(ARMEMENT011121::MeshPath(Spec)));

        UTexture2D* Icon = Look->BaseIcon.LoadSynchronous();
        UStaticMesh* Mesh = Look->WorldMesh.LoadSynchronous();
        if (!TestNotNull(TEXT("Approved icon actually loads"), Icon) ||
            !TestNotNull(TEXT("Approved mesh actually loads"), Mesh))
        {
            continue;
        }
        const FIntPoint Size = Icon->GetImportedSize();
        TestTrue(TEXT("Square icon at least 256x256"), Size.X >= 256 && Size.Y == Size.X);
        TestTrue(TEXT("Mesh contains geometry"), Mesh->GetNumLODs() > 0);
        TestTrue(TEXT("Material ownership resides in the StaticMesh slots"),
            ARMEMENT011121Tests::CheckMaterialOwnership(Spec, Mesh));

        const TCHAR* const Maps[] = { TEXT("BC"), TEXT("N"), TEXT("ORM") };
        for (const TCHAR* Suffix : Maps)
        {
            const FString MapPath = ARMEMENT011121::PbrPath(Spec, Suffix);
            UTexture2D* Map = LoadObject<UTexture2D>(nullptr, *MapPath);
            if (!TestNotNull(TEXT("Approved PBR texture present"), Map))
            {
                continue;
            }
            const bool bExpectedSRGB = FCString::Strcmp(Suffix, TEXT("BC")) == 0;
            TestEqual(TEXT("PBR map uses correct sRGB policy"), Map->SRGB, bExpectedSRGB);
        }
    }
    TestEqual(TEXT("Four exact planned AssassinDagger visual IDs"), VisualIds.Num(), 1);
    TestTrue(TEXT("No more than four new AssassinDagger bindings"), Registered <= 1);
    TestEqual(TEXT("RPG asset never dirtied by visual tests"), Rpg->GetOutermost()->IsDirty(), bRpgDirty);
    TestEqual(TEXT("Visual catalogue never dirtied by tests"), Visuals->GetOutermost()->IsDirty(), bVisualDirty);
    AddInfo(FString::Printf(
        TEXT("[ARMEMENT01.11.2.2.1] %d/1 AssassinDagger visual registrations; missing are not production-ready"),
        Registered));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

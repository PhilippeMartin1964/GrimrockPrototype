#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "Materials/Material.h"
#include "MaterialDomain.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"

// Final-state visual checks must not reload historical ItemDefinitionAsset.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01172CProductionVisuals,
    "Grimrock.RPG.ARMEMENT01.17.2C.ProductionVisualAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01172CProductionVisuals::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Actual canonical ItemSystem configured"), Fixture.bReady))
        return false;
    FGridResolvedItem Catalyst;
    if (!TestTrue(TEXT("Real authored N0 catalyst resolves from ItemSystem"),
        Fixture.Resolve(TEXT("Item_SurfaceCatalyst_01"), 0, Catalyst)))
        return false;
    TestFalse(TEXT("Catalyst is not an ARM"), Catalyst.bIsArmament);
    const TCHAR* MeshPath = TEXT("/Game/GrimrockPrototype/Meshes/Items/SM_Catalyst_01.SM_Catalyst_01");
    const TCHAR* IconPath = TEXT("/Game/GrimrockPrototype/UI/Icons/Items/T_Catalyst_Icon.T_Catalyst_Icon");
    UStaticMesh* Mesh = Catalyst.WorldMesh.LoadSynchronous();
    UTexture2D* Icon = Catalyst.ItemIcon.LoadSynchronous();
    UMaterial* Ember = LoadObject<UMaterial>(nullptr,
        TEXT("/Game/GrimrockPrototype/Art/Materials/Catalyst/M_SurfaceCatalyst_Ember.M_SurfaceCatalyst_Ember"));
    UMaterial* Ring = LoadObject<UMaterial>(nullptr,
        TEXT("/Game/GrimrockPrototype/Art/Materials/Catalyst/M_SurfaceCatalyst_RunePulse.M_SurfaceCatalyst_RunePulse"));
    if (!TestNotNull(TEXT("Approved artist catalyst mesh"), Mesh) ||
        !TestNotNull(TEXT("Approved artist catalyst icon"), Icon) ||
        !TestNotNull(TEXT("Approved Ember material"), Ember) ||
        !TestNotNull(TEXT("Approved Ring material"), Ring))
        return false;
    TestEqual(TEXT("Canonical mesh path matches approved artwork"),
        Catalyst.WorldMesh.ToSoftObjectPath().ToString(), FString(MeshPath));
    TestEqual(TEXT("Canonical icon path matches approved artwork"),
        Catalyst.ItemIcon.ToSoftObjectPath().ToString(), FString(IconPath));
    TestTrue(TEXT("Mesh keeps its native material"),
        Mesh->GetStaticMaterials().Num() > 0 && IsValid(Mesh->GetMaterial(0)));
    TestTrue(TEXT("Ember material is a translucent surface"),
        Ember->MaterialDomain == MD_Surface && Ember->BlendMode == BLEND_Translucent);
    TestTrue(TEXT("Rune material is a translucent decal"),
        Ring->MaterialDomain == MD_DeferredDecal && Ring->BlendMode == BLEND_Translucent);
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS

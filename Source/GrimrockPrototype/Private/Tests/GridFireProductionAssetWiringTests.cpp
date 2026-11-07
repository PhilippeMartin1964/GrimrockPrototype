#include "Misc/AutomationTest.h"
#include "Core/GridSurfaceVfxProfileAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridFireProductionAssetWiring,
    "Grimrock.RPG.FIREFX05.ProductionProfileWiring",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridFireProductionAssetWiring::RunTest(const FString&)
{
    const TCHAR* ProfileObjectPath =
        TEXT("/Game/GrimrockPrototype/Effects/Fire/Local/DA_GroundFire.DA_GroundFire");
    const UGridSurfaceVfxProfileAsset* Profile =
        LoadObject<UGridSurfaceVfxProfileAsset>(nullptr, ProfileObjectPath);
    if (!TestNotNull(TEXT("Committed fire presentation profile exists"), Profile))
    {
        return false;
    }
    // The actual FireFX system is excluded from Git, so inspect the authored
    // soft reference without loading the licensed package on the Windows agent.
    TestFalse(TEXT("Authored profile selects a Niagara ground-fire system"),
        Profile->Settings.GroundFireSystem.IsNull());

    UClass* RuntimeClass = LoadClass<AGridLevelRuntimeActor>(nullptr,
        TEXT("/Game/GrimrockPrototype/Blueprints/Runtime/BP_GridLevelRuntimeActor.BP_GridLevelRuntimeActor_C"));
    if (!TestNotNull(TEXT("Committed runtime Blueprint class exists"), RuntimeClass))
    {
        return false;
    }
    const AGridLevelRuntimeActor* Defaults = RuntimeClass->GetDefaultObject<AGridLevelRuntimeActor>();
    if (!TestNotNull(TEXT("Runtime Blueprint class defaults"), Defaults))
    {
        return false;
    }
    TestEqual(TEXT("Blueprint uses the committed fire profile, no private override"),
        Defaults->FireVfxProfile.ToSoftObjectPath().ToString(), FString(ProfileObjectPath));
    TestFalse(TEXT("Direct profile is not empty"), Defaults->FireVfxProfile.IsNull());
    return true;
}

#endif

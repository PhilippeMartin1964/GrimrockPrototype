#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GridEditorTD073ApprovedCanonicalLockAcceptance.h"
#include "Runtime/GridItemDefinitionAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X841TD073LockSchemaWhitelist,
    "Grimrock.RPG.ARM.CLEAN06_4X841.TD073LockCanonicalSchemaWhitelist",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X841TD073LockSchemaWhitelist::RunTest(const FString&)
{
    using namespace GridTD073ApprovedCanonicalLockAcceptance;
    const FString CopperPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall.DA_Lock_CopperWall");
    const FString IronPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall.DA_Lock_IronWall");

    UGridItemDefinitionAsset* Copper = NewObject<UGridItemDefinitionAsset>();
    Copper->ItemDefinitionId = TEXT("Key_Copper");
    UGridItemDefinitionAsset* Iron = NewObject<UGridItemDefinitionAsset>();
    Iron->ItemDefinitionId = TEXT("Key_Iron");

    FGridLockBehaviorParams L;
    L.AcceptedKeyItems.Add(Copper);
    if (!TestTrue(TEXT("Historical pointer-only copper remains allowed"),
        IsHistoricalOrApproved(CopperPath, L))) return false;
    L.AcceptedKeyIds.Add(TEXT("Key_Copper"));
    if (!TestTrue(TEXT("Only exact copper pair is approved"), IsExactApprovedPair(CopperPath, L)) ||
        !TestFalse(TEXT("Copper pair invalid at iron package"), IsExactApprovedPair(IronPath, L)) ||
        !TestFalse(TEXT("Copper pair invalid at arbitrary package"),
            IsExactApprovedPair(TEXT("/Game/GrimrockPrototype/Other.Other"), L)))
        return false;
    L.AcceptedKeyIds.Add(TEXT("Key_Copper"));
    if (!TestFalse(TEXT("Duplicate explicit IDs rejected"), IsHistoricalOrApproved(CopperPath, L)))
        return false;
    L.AcceptedKeyIds.SetNum(1);
    L.AcceptedKeyIds[0] = TEXT("Key_Iron");
    if (!TestFalse(TEXT("Contradictory explicit ID rejected"), IsHistoricalOrApproved(CopperPath, L)))
        return false;
    L.AcceptedKeyIds[0] = TEXT("Key_Copper");
    L.AcceptedKeyItems[0] = Iron;
    if (!TestFalse(TEXT("Contradictory legacy pointer rejected"), IsHistoricalOrApproved(CopperPath, L)))
        return false;
    L.AcceptedKeyItems[0] = Copper;
    L.AcceptedKeyItems.Add(Copper);
    if (!TestFalse(TEXT("Extra legacy pointer rejected"), IsHistoricalOrApproved(CopperPath, L)))
        return false;
    L.AcceptedKeyItems.SetNum(1);

    FGridLockBehaviorParams R;
    R.AcceptedKeyItems.Add(Iron);
    R.AcceptedKeyIds.Add(TEXT("Key_Iron"));
    if (!TestTrue(TEXT("Exact iron pair approved"), IsExactApprovedPair(IronPath, R)) ||
        !TestFalse(TEXT("Iron pair cannot be applied to copper"), IsExactApprovedPair(CopperPath, R)))
        return false;
    R.AcceptedKeyItems[0] = nullptr;
    if (!TestFalse(TEXT("A present but null historical pointer is not canonical-only"),
        IsHistoricalOrApproved(IronPath, R))) return false;
    R.AcceptedKeyItems.Reset();
    if (!TestTrue(TEXT("Exact iron ID-only final state accepted"),
        IsExactApprovedPair(IronPath, R)) ||
        !TestFalse(TEXT("Iron ID-only state not accepted for copper"),
            IsExactApprovedPair(CopperPath, R))) return false;
    L.AcceptedKeyItems.Reset();
    if (!TestTrue(TEXT("Exact copper ID-only final state accepted"),
        IsExactApprovedPair(CopperPath, L)) ||
        !TestFalse(TEXT("Copper ID-only state not accepted for iron"),
            IsExactApprovedPair(IronPath, L))) return false;
    L.AcceptedKeyIds.Add(TEXT("Key_Copper"));
    return TestFalse(TEXT("Duplicate ID-only key set rejected"),
        IsHistoricalOrApproved(CopperPath, L));
}
#endif

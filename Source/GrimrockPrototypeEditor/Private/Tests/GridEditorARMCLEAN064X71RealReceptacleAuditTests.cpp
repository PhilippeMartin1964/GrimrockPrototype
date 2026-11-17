#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

/** X.52 replaces the obsolete X.71 -Audit commandlet dependency.
 * It opens the real native rules read-only and accepts only a matching
 * old source OR a pointerless canonical rule; never calls old -Apply.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X71RealReceptacleAudit,
    "Grimrock.RPG.ARM.CLEAN06_4X71.RealReceptacleMigrationAuditOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X71RealReceptacleAudit::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/"
                      "DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real item system"), Root.Get())) return false;
    const bool bSystemDirty = Root->GetOutermost()->IsDirty();
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Real item registry configured"),
        Registry->RegisterItemSystem(Root.Get(), Errors))) return false;

    const TCHAR* Paths[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Receptacle_TorchHolder.DA_Receptacle_TorchHolder"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Guardian.DA_Guardian")
    };
    const FName ExpectedIds[] = {TEXT("Item_Torch"), TEXT("Gem_Blue")};
    int32 Old = 0;
    for (int32 Index = 0; Index < 2; ++Index)
    {
        TStrongObjectPtr<UGridWorldObjectDefinitionAsset> Definition(
            LoadObject<UGridWorldObjectDefinitionAsset>(nullptr, Paths[Index]));
        if (!TestNotNull(TEXT("One real shared receptacle"), Definition.Get())) return false;
        const bool bDirty = Definition->GetOutermost()->IsDirty();
        const FGridReceptacleBehaviorParams& R = Definition->DefaultBehavior.Receptacle;
        if (!TestEqual(TEXT("One exact native accepted-item rule"), R.AcceptedItems.Num(), 1) ||
            !TestEqual(TEXT("Canonical accepted ItemId unchanged"),
                R.AcceptedItems[0].CanonicalItemId, ExpectedIds[Index]) ||
            !TestTrue(TEXT("Registered physical item is accepted"),
                Registry->HasCanonicalItem(ExpectedIds[Index])))
            return false;
        const UGridItemDefinitionAsset* Legacy = R.AcceptedItems[0].ItemDefinition.Get();
        if (R.AcceptedItems[0].ItemDefinition)
        {
            if (!TestTrue(TEXT("Optional old source has the exact ItemId"),
                IsValid(Legacy) && Legacy->ItemDefinitionId == ExpectedIds[Index]))
                return false;
            ++Old;
        }
        if (!TestEqual(TEXT("Real receptacle package unchanged"), 
            Definition->GetOutermost()->IsDirty(), bDirty)) return false;
    }

    AddInfo(FString::Printf(TEXT(
        "[ARM-CLEAN06.4X52] read-only shared receptacle rules=2 "
        "oldPointers=%d canonical Torch/Gem preserved; no SavePackage."),
        Old));
    return TestTrue(TEXT("Two shared rules are both legacy-backed or both pointerless"),
        Old == 2 || Old == 0) &&
        TestEqual(TEXT("Real ItemSystem remains read-only"),
            Root->GetOutermost()->IsDirty(), bSystemDirty);
}
#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

/** X.52 replaces the obsolete X.71 -Audit commandlet dependency.
 * The real shared rules are read-only and must contain their exact IDs.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X71RealReceptacleAudit,
    "Grimrock.RPG.ARM.CLEAN06_4X71.RealReceptacleMigrationAuditOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X71RealReceptacleAudit::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
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
    TestNull(TEXT("No historical acceptance pointer property"), FGridReceptacleAcceptedItemConfig::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition")));
    for (int32 Index = 0; Index < 2; ++Index)
    {
        TStrongObjectPtr<UGridWorldObjectDefinitionAsset> Definition(
            LoadObject<UGridWorldObjectDefinitionAsset>(nullptr, Paths[Index]));
        if (!TestNotNull(TEXT("One real shared receptacle"), Definition.Get())) return false;
        const bool bDirty = Definition->GetOutermost()->IsDirty();
        const FGridReceptacleBehaviorParams& R = Definition->DefaultBehavior.Receptacle;
        if (!TestEqual(TEXT("One exact native accepted-item rule"), R.AcceptedItems.Num(), 1) ||
            !TestEqual(TEXT("Canonical accepted ItemId unchanged"),
                R.AcceptedItems[0].ItemId, ExpectedIds[Index]) ||
            !TestTrue(TEXT("Registered physical item is accepted"),
                Registry->ContainsItem(ExpectedIds[Index])))
            return false;
        if (!TestEqual(TEXT("Real receptacle package unchanged"), 
            Definition->GetOutermost()->IsDirty(), bDirty)) return false;
    }

    return TestEqual(TEXT("Real ItemSystem remains read-only"),
            Root->GetOutermost()->IsDirty(), bSystemDirty);
}
#endif

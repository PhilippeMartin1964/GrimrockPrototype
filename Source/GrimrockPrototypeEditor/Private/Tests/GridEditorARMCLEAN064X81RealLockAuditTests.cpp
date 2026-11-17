#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

/** X.52 replaces the obsolete X.81 -Audit commandlet dependency.
 * The two real shared locks have one exact canonical ID each in
 * either their approved pre-native or post-native serialized form.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X81RealLockAudit,
    "Grimrock.RPG.ARM.CLEAN06_4X81.RealLockKeyMigrationAuditOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X81RealLockAudit::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/"
                      "DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real item system"), Root.Get())) return false;
    const bool bSystemDirty = Root->GetOutermost()->IsDirty();
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Real canonical item registry ready"),
        Registry->RegisterItemSystem(Root.Get(), Errors))) return false;

    const TCHAR* Paths[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall.DA_Lock_CopperWall"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall.DA_Lock_IronWall")
    };
    const FName ExpectedIds[] = {TEXT("Key_Copper"), TEXT("Key_Iron")};
    int32 OldPointers = 0;
    for (int32 Index = 0; Index < 2; ++Index)
    {
        TStrongObjectPtr<UGridWorldObjectDefinitionAsset> Definition(
            LoadObject<UGridWorldObjectDefinitionAsset>(nullptr, Paths[Index]));
        if (!TestNotNull(TEXT("One native Copper/Iron lock definition"), Definition.Get()))
            return false;
        const bool bDirty = Definition->GetOutermost()->IsDirty();
        const FGridLockBehaviorParams& Rule = Definition->DefaultBehavior.Lock;
        if (!TestEqual(TEXT("Exact canonical key count"), Rule.AcceptedKeyIds.Num(), 1) ||
            !TestEqual(TEXT("Exact Copper/Iron key ID"),
                Rule.AcceptedKeyIds[0], ExpectedIds[Index]) ||
            !TestTrue(TEXT("Lock key registered in canonical system"),
                Registry->HasCanonicalItem(ExpectedIds[Index])) ||
            !TestTrue(TEXT("No additional key asset sources"),
                Rule.AcceptedKeyItems.Num() <= 1))
            return false;

        if (!Rule.AcceptedKeyItems.IsEmpty())
        {
            const UGridItemDefinitionAsset* Source = Rule.AcceptedKeyItems[0].Get();
            if (!TestTrue(TEXT("Old lock source agrees with approved key"),
                IsValid(Source) && Source->ItemDefinitionId == ExpectedIds[Index]))
                return false;
            ++OldPointers;
        }
        if (!TestEqual(TEXT("Read-only lock package"),
            Definition->GetOutermost()->IsDirty(), bDirty)) return false;
    }

    AddInfo(FString::Printf(TEXT(
        "[ARM-CLEAN06.4X52] read-only Copper/Iron lock rules=2 "
        "historicalPointers=%d; old -Apply never called; no SavePackage."),
        OldPointers));
    return TestTrue(TEXT("Both real keys use the same native migration state"),
            OldPointers == 2 || OldPointers == 0) &&
        TestEqual(TEXT("Canonical item system stays read-only"),
            Root->GetOutermost()->IsDirty(), bSystemDirty);
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Tests/GridCanonicalRegistryFixture.h"

namespace ARMEMENT01101
{
    bool IsArmId(FName Id)
    {
        const FString Text = Id.ToString();
        return Text.Len() == 6 && Text.StartsWith(TEXT("ARM")) &&
            Text.Mid(3).IsNumeric();
    }
    bool ReadRequiredArchetypes(TSet<FName>& OutRequired, FString& OutError)
    {
        OutRequired.Reset();
        OutError.Reset();

        const FString SourcePath = FPaths::Combine(
            FPaths::ProjectDir(), TEXT("docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv"));
        FString Csv;
        if (!FFileHelper::LoadFileToString(Csv, *SourcePath))
        {
            OutError = TEXT("Approved starter-kit recipe manifest not found");
            return false;
        }

        FCsvParser Parser(Csv);
        const auto& Rows = Parser.GetRows();
        if (Rows.Num() < 2 || Rows[0].Num() != 15 ||
            FCString::Strcmp(Rows[0][3], TEXT("Weapon1ArchetypeId")) != 0 ||
            FCString::Strcmp(Rows[0][6], TEXT("Weapon2ArchetypeId")) != 0 ||
            FCString::Strcmp(Rows[0][13], TEXT("ApprovalState")) != 0)
        {
            OutError = TEXT("Unexpected recipe manifest schema");
            return false;
        }

        int32 RecipeCount = 0;
        for (int32 Index = 1; Index < Rows.Num(); ++Index)
        {
            const TArray<const TCHAR*>& Row = Rows[Index];
            if (Row.IsEmpty() || !Row[0] || !*Row[0])
            {
                continue;
            }
            if (Row.Num() != 15 || FCString::Strcmp(Row[13], TEXT("ApprovedDesign")) != 0)
            {
                OutError = FString::Printf(TEXT("Recipe row %d is malformed or not approved"), Index + 1);
                return false;
            }

            const FName First(Row[3]);
            if (!IsArmId(First))
            {
                OutError = FString::Printf(TEXT("Invalid first archetype at row %d"), Index + 1);
                return false;
            }
            OutRequired.Add(First);

            if (FCString::Strcmp(Row[6], TEXT("None")) != 0)
            {
                const FName Second(Row[6]);
                if (!IsArmId(Second))
                {
                    OutError = FString::Printf(TEXT("Invalid second archetype at row %d"), Index + 1);
                    return false;
                }
                OutRequired.Add(Second);
            }
            ++RecipeCount;
        }

        if (RecipeCount != 18 || OutRequired.Num() != 22)
        {
            OutError = FString::Printf(TEXT("Expected 18 approved recipes and 22 archetypes; got %d and %d"),
                RecipeCount, OutRequired.Num());
            return false;
        }
        return true;
    }

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01101ReadinessAudit,
    "Grimrock.RPG.ARMEMENT01.10.Readiness.Audit22N1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01101ReadinessAudit::RunTest(const FString&)
{
    TSet<FName> Required;
    FString Error;
    if (!TestTrue(TEXT("18 approved recipes exactly cover 22 N1 identities"),
        ARMEMENT01101::ReadRequiredArchetypes(Required, Error)))
    {
        AddError(Error);
        return false;
    }
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Production DA_ItemSystem registry configured"), Fixture.bReady))
        return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Real production root"), Root) ||
        !TestNotNull(TEXT("Native weapon numbers"), Root->WeaponCatalogue.Get()) ||
        !TestNotNull(TEXT("Native visuals"), Root->WeaponVisualCatalogue.Get()))
        return false;
    const bool bNumbersDirty = Root->WeaponCatalogue->GetOutermost()->IsDirty();
    const bool bVisualsDirty = Root->WeaponVisualCatalogue->GetOutermost()->IsDirty();
    TArray<FGridWeaponPipelineRow> Rows;
    if (!TestTrue(TEXT("22 authored physical ARM identities"),
        FGridWeaponPipelineManifest::Load(
            FGridWeaponPipelineManifest::DefaultFile(), Rows, Error)))
    {
        AddError(Error);
        return false;
    }
    FString Report = TEXT(
        "ArchetypeId,ItemLevel,VisualMapped,IconLoaded,WorldMeshLoaded,ItemCount,ItemId,DefinitionValid,ActionChecked,Status\n");
    TSet<FName> SeenItems, SeenArms;
    int32 Ready = 0, VerifiedPilots = 0;
    for (const FGridWeaponPipelineRow& Row : Rows)
    {
        if (Row.ProductionState != TEXT("Active")) continue;
        const FName ArmId(Row.ArchetypeId), ItemId(Row.ItemDefinitionId);
        TestTrue(TEXT("Every recipe archetype belongs to physical ItemSystem"),
            Required.Contains(ArmId));
        TestFalse(TEXT("No duplicate production ItemId"), SeenItems.Contains(ItemId));
        TestFalse(TEXT("No duplicate production ARM"), SeenArms.Contains(ArmId));
        SeenItems.Add(ItemId);
        SeenArms.Add(ArmId);
        FGridResolvedItem N1;
        if (!TestTrue(TEXT("Actual native weapon N1 available"),
            Fixture.Resolve(ItemId, 1, N1))) continue;
        const FGridCanonicalWeaponVisual* Visual =
            Root->WeaponVisualCatalogue->FindForArchetype(ArmId);
        const bool bMapped = Visual && Visual->VisualId == FName(Row.VisualId);
        const bool bIcon = N1.ItemIcon.LoadSynchronous() != nullptr;
        const bool bMesh = N1.WorldMesh.LoadSynchronous() != nullptr;
        const bool bDefinitionValid = N1.bIsArmament &&
            N1.ArchetypeId == ArmId && N1.MaxStack == 1 &&
            N1.ItemLevel == 1 && FMath::IsFinite(N1.WeightKg);
        const bool bAttack = Row.IsPassive()
            ? N1.AvailableActions.IsEmpty()
            : N1.AvailableActions.Num() == 1 &&
              N1.AvailableActions[0].ActionId == FName(Row.AttackId);
        const bool bGood = bMapped && bIcon && bMesh &&
            bDefinitionValid && bAttack;
        TestTrue(TEXT("Native production candidate is complete, not legacy or missing"), bGood);
        if (bGood) ++Ready;
        if (ArmId == FName(TEXT("ARM001")) || ArmId == FName(TEXT("ARM045")) ||
            ArmId == FName(TEXT("ARM075")))
        {
            TestTrue(TEXT("Original three pilot silhouettes remain valid"), bGood);
            if (bGood) ++VerifiedPilots;
        }
        Report += FString::Printf(TEXT("%s,1,%d,%d,%d,1,%s,%d,%d,%s\n"),
            *ArmId.ToString(), bMapped ? 1 : 0, bIcon ? 1 : 0,
            bMesh ? 1 : 0, *ItemId.ToString(), bDefinitionValid ? 1 : 0,
            bAttack ? 1 : 0, bGood ? TEXT("CanonicalReady") : TEXT("BLOCKED"));
    }
    TestEqual(TEXT("All 22 concrete ARM N1 are production-ready"), Ready, 22);
    TestEqual(TEXT("All 22 unique ItemIds"), SeenItems.Num(), 22);
    TestEqual(TEXT("Exactly 3 original visual pilots"), VerifiedPilots, 3);
    const FString ReportDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ARMEMENT01"));
    IFileManager::Get().MakeDirectory(*ReportDir, true);
    const FString ReportPath = FPaths::Combine(
        ReportDir, TEXT("ARMEMENT01_10_ProductionReadiness.csv"));
    TestTrue(TEXT("Read-only report stored under Saved"),
        FFileHelper::SaveStringToFile(
            Report, *ReportPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
    TestEqual(TEXT("Native numeric source never dirtied"),
        Root->WeaponCatalogue->GetOutermost()->IsDirty(), bNumbersDirty);
    TestEqual(TEXT("Native visual source never dirtied"),
        Root->WeaponVisualCatalogue->GetOutermost()->IsDirty(), bVisualsDirty);
    AddInfo(FString::Printf(TEXT("ARMEMENT01.10 native production readiness: %d/22 N1, report=%s"),
        Ready, *ReportPath));
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS

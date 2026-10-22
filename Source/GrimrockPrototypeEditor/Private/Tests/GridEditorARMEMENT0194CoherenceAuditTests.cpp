#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridStartingKitCoherenceAudit.h"
#include "RPG/RPGClassAsset.h"
#include "UObject/Package.h"

namespace ARMEMENT0194
{
    int32 CsvRecordCount(const FString& Contents)
    {
        TArray<FString> Rows;
        Contents.ParseIntoArray(Rows, TEXT("\n"), true);
        return Rows.Num() > 0 ? Rows.Num() - 1 : 0; // exclude CSV header
    }

    URPGClassAsset* LoadRanger()
    {
        return LoadObject<URPGClassAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.DA_Class_Ranger"));
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0194AuthoritativeCoverage,
    "Grimrock.RPG.ARMEMENT01.9.CoherenceAudit.ProductionCoverage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0194AuthoritativeCoverage::RunTest(const FString&)
{
    FGridStartingKitCoherenceAuditReport Report;
    FString Error;
    if (!TestTrue(TEXT("Production report builds"),
        FGridStartingKitCoherenceAudit::BuildProductionReport(Report, Error))) return false;
    TestEqual(TEXT("Six production class assets inspected"), Report.ClassCount, 6);
    TestEqual(TEXT("Exactly twelve authored or preset kits"), Report.KitCount, 12);
    TestEqual(TEXT("Three real talent branches per class"), Report.BranchCount, 18);
    TestEqual(TEXT("Production skill catalog contains 25 skills"), Report.SkillCount, 25);
    TestEqual(TEXT("Every class-skill affinity evaluated"), Report.AffinityCount, 150);
    TestEqual(TEXT("Exactly four weak kit-to-branch associations flagged"), Report.KitsWithoutProposedBranch, 4);
    TestEqual(TEXT("Kit CSV has 12 records"), ARMEMENT0194::CsvRecordCount(Report.KitMatrixCsv), 12);
    TestEqual(TEXT("Talent CSV has 18 records"), ARMEMENT0194::CsvRecordCount(Report.TalentBranchesCsv), 18);
    TestEqual(TEXT("Affinity CSV has 150 records"), ARMEMENT0194::CsvRecordCount(Report.SkillAffinitiesCsv), 150);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0194SemanticGaps,
    "Grimrock.RPG.ARMEMENT01.9.CoherenceAudit.RecommendationsAreAdvisory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0194SemanticGaps::RunTest(const FString&)
{
    FGridStartingKitCoherenceAuditReport Report;
    FString Error;
    if (!FGridStartingKitCoherenceAudit::BuildProductionReport(Report, Error))
    {
        AddError(Error);
        return false;
    }
    TestTrue(TEXT("Ranger Archer links to an existing Marksman branch"),
        Report.KitMatrixCsv.Contains(TEXT("\"Ranger_Archer\",\"Yes\",\"2\",")) &&
        Report.KitMatrixCsv.Contains(TEXT("\"Marksman\",\"Yes\"")));
    TestTrue(TEXT("Ranger related skill uses canonical RangedWeapons ID"),
        Report.KitMatrixCsv.Contains(TEXT("\"Skill_RangedWeapons\"")));
    TestTrue(TEXT("Warrior Skirmisher lacks substantiated talent branch"),
        Report.KitMatrixCsv.Contains(TEXT("\"Warrior_Skirmisher\"")) &&
        Report.KitMatrixCsv.Contains(TEXT("\"NotProposed\"")));
    TestTrue(TEXT("Generic kits explicitly need design review"),
        Report.KitMatrixCsv.Contains(TEXT("\"NeedsDesignReview\"")));
    TestTrue(TEXT("A proposed link still requires game design approval"),
        Report.KitMatrixCsv.Contains(TEXT("\"ProposedMatchNeedsReview\"")));
    TestTrue(TEXT("All individual class affinities come from the Skill presentation catalog"),
        Report.SkillAffinitiesCsv.Contains(TEXT("\"Alchemist\",\"Skill_Alchemy\"")) &&
        Report.SkillAffinitiesCsv.Contains(TEXT("\"Priest\",\"Skill_Religion\"")));
    TestTrue(TEXT("Early talent branch is inspected rather than granted"),
        Report.TalentBranchesCsv.Contains(TEXT("\"Ranger\",\"Marksman\"")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0194NoMutation,
    "Grimrock.RPG.ARMEMENT01.9.CoherenceAudit.ReadOnlyAndDeterministic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0194NoMutation::RunTest(const FString&)
{
    URPGClassAsset* Ranger = ARMEMENT0194::LoadRanger();
    if (!TestNotNull(TEXT("Production ranger loads"), Ranger)) return false;
    const int32 OriginalKitCount = Ranger->StartingEquipmentKits.Num();
    const FName OriginalDefault = Ranger->DefaultStartingEquipmentKitId;
    const bool bWasDirty = Ranger->GetOutermost()->IsDirty();

    FGridStartingKitCoherenceAuditReport First;
    FGridStartingKitCoherenceAuditReport Second;
    FString Error;
    if (!TestTrue(TEXT("First report"), FGridStartingKitCoherenceAudit::BuildProductionReport(First, Error)) ||
        !TestTrue(TEXT("Second report"), FGridStartingKitCoherenceAudit::BuildProductionReport(Second, Error)))
    {
        AddError(Error);
        return false;
    }
    TestEqual(TEXT("Kit report output deterministic"), First.KitMatrixCsv, Second.KitMatrixCsv);
    TestEqual(TEXT("Talent report output deterministic"), First.TalentBranchesCsv, Second.TalentBranchesCsv);
    TestEqual(TEXT("Skill-affinity report output deterministic"), First.SkillAffinitiesCsv, Second.SkillAffinitiesCsv);
    TestEqual(TEXT("Production DataAsset kit count unchanged"), Ranger->StartingEquipmentKits.Num(), OriginalKitCount);
    TestEqual(TEXT("Production DataAsset default unchanged"), Ranger->DefaultStartingEquipmentKitId, OriginalDefault);
    TestEqual(TEXT("Report must not dirty production class package"), Ranger->GetOutermost()->IsDirty(), bWasDirty);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

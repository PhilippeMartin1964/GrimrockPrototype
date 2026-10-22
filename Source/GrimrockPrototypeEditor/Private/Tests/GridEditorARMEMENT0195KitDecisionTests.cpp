#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"
#include "ARMEMENT/GridStartingKitCoherenceAudit.h"
#include "RPG/RPGClassAsset.h"

namespace ARMEMENT0195
{
    struct FRow
    {
        FString ClassId;
        FString KitId;
        FString Disposition;
        FString BranchId;
        FString Level2TalentChoiceId;
        FString RelatedSkillId;
        FString ExpectedAffinity;
        FString MechanicReview;
        FString NextMilestone;
        FString ApprovalState;
    };

    bool ReadManifest(TArray<FRow>& OutRows, FString& OutError)
    {
        OutRows.Reset();
        OutError.Reset();
        FString Csv;
        const FString Filename = FPaths::Combine(
            FPaths::ProjectDir(), TEXT("docs/Design/ARMEMENT01_9_5_KIT_DECISIONS.csv"));
        if (!FFileHelper::LoadFileToString(Csv, *Filename))
        {
            OutError = TEXT("Cannot read canonical ARMEMENT01.9.5 review matrix.");
            return false;
        }
        FCsvParser Parser(Csv);
        const auto& Lines = Parser.GetRows();
        if (Lines.IsEmpty() || Lines[0].Num() != 10 ||
            FCString::Strcmp(Lines[0][0], TEXT("ClassId")) != 0 ||
            FCString::Strcmp(Lines[0][1], TEXT("KitId")) != 0 ||
            FCString::Strcmp(Lines[0][9], TEXT("ApprovalState")) != 0)
        {
            OutError = TEXT("Invalid ARMEMENT01.9.5 manifest header.");
            return false;
        }
        TSet<FString> UniqueIds;
        for (int32 Index = 1; Index < Lines.Num(); ++Index)
        {
            const TArray<const TCHAR*>& Cells = Lines[Index];
            if (Cells.IsEmpty() || !Cells[0] || FCString::Strlen(Cells[0]) == 0)
            {
                continue;
            }
            if (Cells.Num() != 10)
            {
                OutError = TEXT("Incorrect number of review fields.");
                return false;
            }
            FRow Entry;
            Entry.ClassId = Cells[0];
            Entry.KitId = Cells[1];
            Entry.Disposition = Cells[2];
            Entry.BranchId = Cells[3];
            Entry.Level2TalentChoiceId = Cells[4];
            Entry.RelatedSkillId = Cells[5];
            Entry.ExpectedAffinity = Cells[6];
            Entry.MechanicReview = Cells[7];
            Entry.NextMilestone = Cells[8];
            Entry.ApprovalState = Cells[9];
            const bool bKnownDisposition =
                Entry.Disposition == TEXT("Retain") ||
                Entry.Disposition == TEXT("RetainConditional") ||
                Entry.Disposition == TEXT("RetainUtility") ||
                Entry.Disposition == TEXT("Revise");
            if (UniqueIds.Contains(Entry.KitId) || Entry.KitId.IsEmpty() ||
                Entry.ClassId.IsEmpty() || !bKnownDisposition ||
                Entry.MechanicReview.IsEmpty() || Entry.NextMilestone.IsEmpty() ||
                Entry.ApprovalState != TEXT("PendingApproval"))
            {
                OutError = FString::Printf(TEXT("Invalid or duplicated design-review entry: %s."),
                    *Entry.KitId);
                return false;
            }
            UniqueIds.Add(Entry.KitId);
            OutRows.Add(MoveTemp(Entry));
        }
        if (OutRows.Num() != 12)
        {
            OutError = TEXT("ARMEMENT01.9.5 must have exactly 12 decisions.");
            return false;
        }
        return true;
    }

    bool ExtractActualRows(const FString& Csv, TMap<FString, TArray<FString>>& OutRows)
    {
        OutRows.Reset();
        FCsvParser Parser(Csv);
        const auto& Lines = Parser.GetRows();
        if (Lines.IsEmpty() || Lines[0].Num() != 11)
        {
            return false;
        }
        for (int32 Index = 1; Index < Lines.Num(); ++Index)
        {
            const TArray<const TCHAR*>& Cells = Lines[Index];
            if (Cells.IsEmpty() || !Cells[0] || !*Cells[0]) continue;
            if (Cells.Num() != 11) return false;
            TArray<FString> Values;
            for (const TCHAR* Cell : Cells) Values.Add(Cell ? FString(Cell) : FString());
            if (OutRows.Contains(Values[1])) return false;
            OutRows.Add(Values[1], MoveTemp(Values));
        }
        return OutRows.Num() == 12;
    }

    URPGClassAsset* LoadClass(const FString& ClassId)
    {
        const FString Path = FString::Printf(
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_%s.DA_Class_%s"),
            *ClassId, *ClassId);
        return LoadObject<URPGClassAsset>(nullptr, *Path);
    }

    bool LoadEvidence(TArray<FRow>& Manifest, TMap<FString, TArray<FString>>& Actual, FString& Error)
    {
        if (!ReadManifest(Manifest, Error)) return false;
        FGridStartingKitCoherenceAuditReport Report;
        if (!FGridStartingKitCoherenceAudit::BuildProductionReport(Report, Error))
        {
            return false;
        }
        if (!ExtractActualRows(Report.KitMatrixCsv, Actual))
        {
            Error = TEXT("Cannot parse authoritative ARMEMENT01.9.4 kit audit.");
            return false;
        }
        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0195AllKitsCovered,
    "Grimrock.RPG.ARMEMENT01.9.DesignReview.TwelveKitsAndDisposition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0195AllKitsCovered::RunTest(const FString&)
{
    TArray<ARMEMENT0195::FRow> Manifest;
    TMap<FString, TArray<FString>> Actual;
    FString Error;
    if (!TestTrue(TEXT("Review matrix and authoritative kit audit load"),
        ARMEMENT0195::LoadEvidence(Manifest, Actual, Error)))
    {
        AddInfo(Error);
        return false;
    }
    TMap<FString, int32> Dispositions;
    for (const ARMEMENT0195::FRow& Entry : Manifest)
    {
        const TArray<FString>* ProductionKit = Actual.Find(Entry.KitId);
        if (!TestNotNull(TEXT("Every reviewed kit exists in production"), ProductionKit))
        {
            return false;
        }
        TestEqual(TEXT("Reviewed class ID matches actual kit"), Entry.ClassId, (*ProductionKit)[0]);
        TestEqual(TEXT("Review does not invent a skill ID"), Entry.RelatedSkillId, (*ProductionKit)[8]);
        TestEqual(TEXT("Actual affinity is never guessed"), Entry.ExpectedAffinity, (*ProductionKit)[9]);
        Dispositions.FindOrAdd(Entry.Disposition) += 1;
    }
    TestEqual(TEXT("One strongly retained kit"), Dispositions.FindRef(TEXT("Retain")), 1);
    TestEqual(TEXT("Five retained with conditions"), Dispositions.FindRef(TEXT("RetainConditional")), 5);
    TestEqual(TEXT("One branch-independent utility kit"), Dispositions.FindRef(TEXT("RetainUtility")), 1);
    TestEqual(TEXT("Five kits need an equipment review"), Dispositions.FindRef(TEXT("Revise")), 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0195ActualTalentChecks,
    "Grimrock.RPG.ARMEMENT01.9.DesignReview.RealLevel2TalentChoices",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0195ActualTalentChecks::RunTest(const FString&)
{
    TArray<ARMEMENT0195::FRow> Manifest;
    TMap<FString, TArray<FString>> Actual;
    FString Error;
    if (!TestTrue(TEXT("Manifest available"), ARMEMENT0195::LoadEvidence(Manifest, Actual, Error)))
    {
        AddInfo(Error);
        return false;
    }
    int32 Linked = 0;
    int32 Independent = 0;
    for (const ARMEMENT0195::FRow& Entry : Manifest)
    {
        const TArray<FString>& ProductionKit = Actual.FindChecked(Entry.KitId);
        TestEqual(TEXT("Talent branch suggestion matches 01.9.4 evidence"), Entry.BranchId, ProductionKit[5]);
        if (Entry.BranchId == TEXT("None"))
        {
            TestEqual(TEXT("No invented level-2 talent on branch-independent kit"),
                Entry.Level2TalentChoiceId, FString(TEXT("None")));
            ++Independent;
            continue;
        }

        URPGClassAsset* Class = ARMEMENT0195::LoadClass(Entry.ClassId);
        if (!TestNotNull(TEXT("Production class exists for proposed talent"), Class)) return false;
        const FRPGClassProgressionChoiceDefinition* Choice = Class->ProgressionChoices.FindByPredicate(
            [&Entry](const FRPGClassProgressionChoiceDefinition& Candidate)
            {
                return Candidate.ChoiceId == FName(*Entry.Level2TalentChoiceId);
            });
        if (!TestNotNull(TEXT("Actual level-two talent exists in class progression"), Choice)) return false;
        TestEqual(TEXT("Talent belongs to the claimed branch"), Choice->TalentBranchId, FName(*Entry.BranchId));
        TestEqual(TEXT("Talent first becomes available at level 2"), Choice->MinimumLevel, 2);

        TArray<FString> ActualFirstLevel2;
        ProductionKit[7].ParseIntoArray(ActualFirstLevel2, TEXT(";"), true);
        TestTrue(TEXT("Reference talent is one of the actual first-tier choices"),
            ActualFirstLevel2.Contains(Entry.Level2TalentChoiceId));
        ++Linked;
    }
    TestEqual(TEXT("Eight conditional talent links are documented"), Linked, 8);
    TestEqual(TEXT("Four entries do not claim a specific talent"), Independent, 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0195AdvisoryOnly,
    "Grimrock.RPG.ARMEMENT01.9.DesignReview.NoPrematureApproval",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0195AdvisoryOnly::RunTest(const FString&)
{
    TArray<ARMEMENT0195::FRow> Manifest;
    FString Error;
    if (!TestTrue(TEXT("Design proposals can be read"),
        ARMEMENT0195::ReadManifest(Manifest, Error)))
    {
        AddInfo(Error);
        return false;
    }
    TSet<FString> Actions;
    for (const ARMEMENT0195::FRow& Entry : Manifest)
    {
        TestEqual(TEXT("Every kit awaits design approval"), Entry.ApprovalState, FString(TEXT("PendingApproval")));
        TestFalse(TEXT("No silent kit-to-talent lock at level one"), Entry.NextMilestone.Contains(TEXT("Lock")));
        TestFalse(TEXT("A review outcome is not an equipment restriction"), Entry.MechanicReview.Contains(TEXT("BlockEquip")));
        TestFalse(TEXT("Follow-up actions are individually identified"), Actions.Contains(Entry.NextMilestone));
        Actions.Add(Entry.NextMilestone);
    }
    TestEqual(TEXT("Exactly twelve distinct follow-up actions"), Actions.Num(), 12);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridWeaponNarrativeImport.h"

namespace
{
    FString MakeNarrativeCsv()
    {
        FString Csv = TEXT("SchemaVersion,ArchetypeId,DescriptionCourteFR,LoreFR,ParticulariteFR,EtatParticularite,NotesConception,EtatValidation\n");
        for (int32 I = 1; I <= 82; ++I)
        {
            const FString Id = FString::Printf(TEXT("ARM%03d"), I);
            const FString Description = I == 75 ? TEXT("\"Bouclier rond, forgé pour la garde.\"")
                : TEXT("\"Arme de forge médiévale.\"");
            Csv += FString::Printf(TEXT("NarrativeV1,%s,%s,\"Lore privé\",,NOT_APPLICABLE,\"Relecture\",%s\n"),
                *Id, *Description, I == 75 ? TEXT("APPROVED") : TEXT("DRAFT"));
        }
        return Csv;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FINDUS044NarrativeParserTest,
    "Grimrock.RPG.ARMEMENT.INDUS04.4.NarrativeV1StrictParser",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINDUS044NarrativeParserTest::RunTest(const FString&)
{
    TArray<FGridWeaponNarrativeRow> Rows;
    FString Error;
    const FString Source = MakeNarrativeCsv();
    TestTrue(TEXT("82 NarrativeV1 rows with UTF-8 French short text"), FGridWeaponNarrativeImport::Parse(Source, Rows, Error));
    TestEqual(TEXT("82 archetype descriptions"), Rows.Num(), 82);
    if (Rows.Num() != 82) return false;
    TestEqual(TEXT("True ARM075 identity"), Rows[74].ArchetypeId, FString(TEXT("ARM075")));
    TestEqual(TEXT("Accented, quoted French text survives parser"), Rows[74].ShortDescriptionFR,
        FString(TEXT("Bouclier rond, forgé pour la garde.")));
    TestEqual(TEXT("Only explicitly reviewed row approved"), Rows[74].ValidationState, FString(TEXT("APPROVED")));

    FString Duplicate = Source.Replace(TEXT("NarrativeV1,ARM082,"), TEXT("NarrativeV1,ARM075,"));
    TestFalse(TEXT("Duplicate ID rejected"), FGridWeaponNarrativeImport::Parse(Duplicate, Rows, Error));
    TestTrue(TEXT("Parser failure exposes error"), !Error.IsEmpty());
    TestTrue(TEXT("Failure resets partial output"), Rows.IsEmpty());

    TestFalse(TEXT("Incomplete 81-row export rejected"),
        FGridWeaponNarrativeImport::Parse(Source.LeftChop(Source.Len() - Source.Find(TEXT("NarrativeV1,ARM082,"))), Rows, Error));

    FString BadHeader = Source.Replace(TEXT("DescriptionCourteFR"), TEXT("Description"));
    TestFalse(TEXT("Unexpected header rejected"), FGridWeaponNarrativeImport::Parse(BadHeader, Rows, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FINDUS044NarrativeApprovalTest,
    "Grimrock.RPG.ARMEMENT.INDUS04.4.OnlyApprovedNonDestructive",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FINDUS044NarrativeApprovalTest::RunTest(const FString&)
{
    TArray<FGridWeaponNarrativeRow> Rows;
    FString Error;
    if (!TestTrue(TEXT("Canonical synthetic narrative CSV parsed"),
        FGridWeaponNarrativeImport::Parse(MakeNarrativeCsv(), Rows, Error))) return false;
    const FGridWeaponNarrativeRow& Approved = Rows[74];
    const FGridWeaponNarrativeRow& Draft = Rows[0];
    TestEqual(TEXT("Approved text fills only empty description"),
        FGridWeaponNarrativeImport::Decide(Approved, FString()), EGridWeaponNarrativeDecision::Ready);
    TestEqual(TEXT("Already authored same description is idempotent"),
        FGridWeaponNarrativeImport::Decide(Approved, Approved.ShortDescriptionFR), EGridWeaponNarrativeDecision::PresentSame);
    TestEqual(TEXT("Different existing legacy description cannot be overwritten"),
        FGridWeaponNarrativeImport::Decide(Approved, TEXT("Texte manuel")), EGridWeaponNarrativeDecision::Conflict);
    TestEqual(TEXT("DRAFT cannot publish even with empty asset description"),
        FGridWeaponNarrativeImport::Decide(Draft, FString()), EGridWeaponNarrativeDecision::Blocked);

    FGridWeaponNarrativeRow Review = Approved;
    Review.ValidationState = TEXT("NEEDS_REVIEW");
    TestEqual(TEXT("NEEDS_REVIEW cannot publish"),
        FGridWeaponNarrativeImport::Decide(Review, FString()), EGridWeaponNarrativeDecision::Blocked);
    Review.ValidationState = TEXT("APPROVED");
    Review.ShortDescriptionFR = TEXT("   ");
    TestEqual(TEXT("Blank approved text never overwrites"),
        FGridWeaponNarrativeImport::Decide(Review, FString()), EGridWeaponNarrativeDecision::Blocked);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

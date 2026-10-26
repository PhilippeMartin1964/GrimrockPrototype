#include "ARMEMENT/GridWeaponNarrativeImport.h"

#include "Misc/FileHelper.h"
#include "Serialization/Csv/CsvParser.h"

namespace
{
    constexpr int32 ExpectedWidth = 8;
    const TCHAR* const Header[ExpectedWidth] =
    {
        TEXT("SchemaVersion"), TEXT("ArchetypeId"), TEXT("DescriptionCourteFR"),
        TEXT("LoreFR"), TEXT("ParticulariteFR"), TEXT("EtatParticularite"),
        TEXT("NotesConception"), TEXT("EtatValidation")
    };

    FString Cell(const TArray<const TCHAR*>& Cells, int32 Index)
    {
        return Cells.IsValidIndex(Index) && Cells[Index] ? FString(Cells[Index]) : FString();
    }

    bool IsCanonicalArchetype(const FString& Id)
    {
        if (Id.Len() != 6 || !Id.StartsWith(TEXT("ARM")))
        {
            return false;
        }
        for (int32 I = 3; I < 6; ++I)
        {
            if (Id[I] < TEXT('0') || Id[I] > TEXT('9'))
            {
                return false;
            }
        }
        const int32 Number = FCString::Atoi(*Id.Right(3));
        return Number >= 1 && Number <= 82;
    }
}

bool FGridWeaponNarrativeImport::Load(const FString& CsvPath,
    TArray<FGridWeaponNarrativeRow>& OutRows, FString& OutError)
{
    FString Content;
    if (!FFileHelper::LoadFileToString(Content, *CsvPath))
    {
        OutRows.Reset();
        OutError = FString::Printf(TEXT("Narrative CSV missing: %s"), *CsvPath);
        return false;
    }
    return Parse(Content, OutRows, OutError);
}

bool FGridWeaponNarrativeImport::Parse(const FString& CsvContent,
    TArray<FGridWeaponNarrativeRow>& OutRows, FString& OutError)
{
    OutRows.Reset();
    OutError.Reset();
    FCsvParser Parser(CsvContent);
    const auto& Data = Parser.GetRows();
    if (Data.Num() < 2 || Data[0].Num() != ExpectedWidth)
    {
        OutError = TEXT("NarrativeV1 header missing or wrong width");
        return false;
    }
    for (int32 I = 0; I < ExpectedWidth; ++I)
    {
        FString Actual = Cell(Data[0], I);
        if (I == 0 && !Actual.IsEmpty() && Actual[0] == TCHAR(0xFEFF))
        {
            Actual.RightChopInline(1);
        }
        if (Actual != Header[I])
        {
            OutError = FString::Printf(TEXT("NarrativeV1 column %d must be %s"), I, Header[I]);
            return false;
        }
    }
    TSet<FString> Seen;
    for (int32 I = 1; I < Data.Num(); ++I)
    {
        const auto& Cells = Data[I];
        if (Cells.IsEmpty() || (Cells.Num() == 1 && Cell(Cells, 0).IsEmpty()))
        {
            continue;
        }
        if (Cells.Num() != ExpectedWidth || Cell(Cells, 0) != TEXT("NarrativeV1"))
        {
            OutError = FString::Printf(TEXT("NarrativeV1 invalid schema or width on row %d"), I + 1);
            OutRows.Reset();
            return false;
        }
        FGridWeaponNarrativeRow Row;
        Row.ArchetypeId = Cell(Cells, 1);
        Row.ShortDescriptionFR = Cell(Cells, 2);
        Row.ValidationState = Cell(Cells, 7);

        if (!IsCanonicalArchetype(Row.ArchetypeId) || Seen.Contains(Row.ArchetypeId) ||
            Row.ShortDescriptionFR.Len() > 300 || Cell(Cells, 3).Len() > 4000 ||
            Cell(Cells, 4).Len() > 300 || Cell(Cells, 6).Len() > 2000 ||
            (Cell(Cells, 5) != TEXT("UNDOCUMENTED") &&
             Cell(Cells, 5) != TEXT("PLANNED") &&
             Cell(Cells, 5) != TEXT("IMPLEMENTED") &&
             Cell(Cells, 5) != TEXT("NOT_APPLICABLE")) ||
            (Row.ValidationState != TEXT("DRAFT") &&
             Row.ValidationState != TEXT("NEEDS_REVIEW") &&
             Row.ValidationState != TEXT("APPROVED")) ||
            (Row.ValidationState == TEXT("APPROVED") &&
             Row.ShortDescriptionFR.TrimStartAndEnd().IsEmpty()) ||
            (Cell(Cells, 5) == TEXT("IMPLEMENTED") && Cell(Cells, 4).TrimStartAndEnd().IsEmpty()))
        {
            OutError = FString::Printf(TEXT("NarrativeV1 invalid identity/text/review on row %d"), I + 1);
            OutRows.Reset();
            return false;
        }
        Seen.Add(Row.ArchetypeId);
        OutRows.Add(MoveTemp(Row));
    }
    if (OutRows.Num() != 82)
    {
        OutError = FString::Printf(TEXT("NarrativeV1 requires 82 unique archetypes, found %d"), OutRows.Num());
        OutRows.Reset();
        return false;
    }
    return true;
}

EGridWeaponNarrativeDecision FGridWeaponNarrativeImport::Decide(
    const FGridWeaponNarrativeRow& Row, const FString& ExistingDescription)
{
    if (Row.ValidationState != TEXT("APPROVED") ||
        Row.ShortDescriptionFR.TrimStartAndEnd().IsEmpty() ||
        Row.ShortDescriptionFR.Len() > 300)
    {
        return EGridWeaponNarrativeDecision::Blocked;
    }
    if (ExistingDescription == Row.ShortDescriptionFR)
    {
        return EGridWeaponNarrativeDecision::PresentSame;
    }
    return ExistingDescription.TrimStartAndEnd().IsEmpty()
        ? EGridWeaponNarrativeDecision::Ready
        : EGridWeaponNarrativeDecision::Conflict;
}

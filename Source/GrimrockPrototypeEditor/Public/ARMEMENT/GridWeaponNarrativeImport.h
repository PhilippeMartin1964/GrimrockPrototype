#pragma once

#include "CoreMinimal.h"

/** INDUS04.4: editorial-only short descriptions. Never combat stats or lore. */
struct FGridWeaponNarrativeRow
{
    FString ArchetypeId;
    FString ShortDescriptionFR;
    FString ValidationState;
};

enum class EGridWeaponNarrativeDecision : uint8
{
    Blocked,
    Ready,
    PresentSame,
    Conflict
};

/** Strict, read-only parser for the NarrativeV1 CSV emitted by the canonical XLSX exporter. */
class FGridWeaponNarrativeImport
{
public:
    static bool Parse(const FString& CsvContent, TArray<FGridWeaponNarrativeRow>& OutRows, FString& OutError);
    static bool Load(const FString& CsvPath, TArray<FGridWeaponNarrativeRow>& OutRows, FString& OutError);

    /** A DRAFT or NEEDS_REVIEW row is never publishable, including when -Apply is supplied. */
    static EGridWeaponNarrativeDecision Decide(
        const FGridWeaponNarrativeRow& Row, const FString& ExistingDescription);
};

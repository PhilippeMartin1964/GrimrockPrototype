#pragma once

#include "CoreMinimal.h"

/** ARMEMENT01.9.4: read-only evidence generated from production class,
 * starter-item, talent and skill presentation DataAssets. No gameplay authority.
 */
struct GRIMROCKPROTOTYPEEDITOR_API FGridStartingKitCoherenceAuditReport
{
    FString KitMatrixCsv;
    FString TalentBranchesCsv;
    FString SkillAffinitiesCsv;
    int32 ClassCount = 0;
    int32 KitCount = 0;
    int32 BranchCount = 0;
    int32 SkillCount = 0;
    int32 AffinityCount = 0;
    int32 KitsWithoutProposedBranch = 0;
};

class GRIMROCKPROTOTYPEEDITOR_API FGridStartingKitCoherenceAudit
{
public:
    /** Proposed links are advisory ONLY and never affect kit eligibility. */
    static bool BuildProductionReport(FGridStartingKitCoherenceAuditReport& OutReport, FString& OutError);
};

#include "RPG/RPGClassProgressionService.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"

namespace
{
    bool IsMON154LevelValid (int32 CharacterLevel)
    {
        return CharacterLevel >=
                URPGCharacterRulesLibrary::GetMinimumLevel () &&
            CharacterLevel <=
                URPGCharacterRulesLibrary::GetMaximumLevel ();
    }

    bool IsMON154ClassValid (const URPGClassAsset* ClassDefinition)
    {
        return IsValid (ClassDefinition) &&
            ClassDefinition->IsValidDefinition ();
    }
}

int32 FRPGClassProgressionService::GetTotalChoicePointsGranted (
    const URPGClassAsset* ClassDefinition,
    int32 CharacterLevel)
{
    if (!IsMON154ClassValid (ClassDefinition) ||
        !IsMON154LevelValid (CharacterLevel))
    {
        return 0;
    }

    int32 TotalPoints = 0;
    for (const FRPGClassProgressionLevelGrant& Grant :
        ClassDefinition->ProgressionLevelGrants)
    {
        if (Grant.Level <= CharacterLevel)
        {
            TotalPoints += Grant.ChoicePointsGranted;
        }
    }
    return FMath::Max (0, TotalPoints);
}

bool FRPGClassProgressionService::TryGetChoicePointBalance (
    const URPGClassAsset* ClassDefinition,
    int32 CharacterLevel,
    const TSet<FName>& SelectedChoiceIds,
    int32& OutGrantedPoints,
    int32& OutSpentPoints,
    int32& OutRemainingPoints)
{
    OutGrantedPoints = 0;
    OutSpentPoints = 0;
    OutRemainingPoints = 0;

    if (!IsMON154ClassValid (ClassDefinition) ||
        !IsMON154LevelValid (CharacterLevel))
    {
        return false;
    }

    const int32 GrantedPoints =
        GetTotalChoicePointsGranted (
            ClassDefinition,
            CharacterLevel);
    int32 SpentPoints = 0;

    for (const FName ChoiceId : SelectedChoiceIds)
    {
        const FRPGClassProgressionChoiceDefinition* Choice =
            ClassDefinition->FindProgressionChoice (ChoiceId);
        if (!Choice || Choice->MinimumLevel > CharacterLevel)
        {
            return false;
        }
        for (const FName PrerequisiteId :
            Choice->PrerequisiteChoiceIds)
        {
            if (!SelectedChoiceIds.Contains (PrerequisiteId))
            {
                return false;
            }
        }
        SpentPoints += Choice->PointCost;
        if (SpentPoints > GrantedPoints)
        {
            return false;
        }
    }

    OutGrantedPoints = GrantedPoints;
    OutSpentPoints = SpentPoints;
    OutRemainingPoints = GrantedPoints - SpentPoints;
    return true;
}

ERPGClassProgressionChoiceAvailabilityReason
FRPGClassProgressionService::GetChoiceAvailability (
    const URPGClassAsset* ClassDefinition,
    int32 CharacterLevel,
    const TSet<FName>& SelectedChoiceIds,
    FName ChoiceId)
{
    if (!IsMON154ClassValid (ClassDefinition))
    {
        return ERPGClassProgressionChoiceAvailabilityReason::
            InvalidClassDefinition;
    }
    if (!IsMON154LevelValid (CharacterLevel))
    {
        return ERPGClassProgressionChoiceAvailabilityReason::InvalidLevel;
    }

    int32 GrantedPoints = 0;
    int32 SpentPoints = 0;
    int32 RemainingPoints = 0;
    if (!TryGetChoicePointBalance (
            ClassDefinition,
            CharacterLevel,
            SelectedChoiceIds,
            GrantedPoints,
            SpentPoints,
            RemainingPoints))
    {
        return ERPGClassProgressionChoiceAvailabilityReason::
            InvalidSelectionState;
    }

    const FRPGClassProgressionChoiceDefinition* Choice =
        ClassDefinition->FindProgressionChoice (ChoiceId);
    if (!Choice)
    {
        return ERPGClassProgressionChoiceAvailabilityReason::UnknownChoice;
    }
    if (SelectedChoiceIds.Contains (ChoiceId))
    {
        return ERPGClassProgressionChoiceAvailabilityReason::
            AlreadySelected;
    }
    if (CharacterLevel < Choice->MinimumLevel)
    {
        return ERPGClassProgressionChoiceAvailabilityReason::LevelTooLow;
    }
    for (const FName PrerequisiteId : Choice->PrerequisiteChoiceIds)
    {
        if (!SelectedChoiceIds.Contains (PrerequisiteId))
        {
            return ERPGClassProgressionChoiceAvailabilityReason::
                MissingPrerequisite;
        }
    }
    if (RemainingPoints < Choice->PointCost)
    {
        return ERPGClassProgressionChoiceAvailabilityReason::
            InsufficientChoicePoints;
    }
    return ERPGClassProgressionChoiceAvailabilityReason::None;
}

bool FRPGClassProgressionService::CollectSatisfiedRequirements (
    const URPGClassAsset* ClassDefinition,
    int32 CharacterLevel,
    const TSet<FName>& SelectedChoiceIds,
    TSet<FName>& OutSatisfiedRequirements)
{
    OutSatisfiedRequirements.Reset ();

    int32 GrantedPoints = 0;
    int32 SpentPoints = 0;
    int32 RemainingPoints = 0;
    if (!TryGetChoicePointBalance (
            ClassDefinition,
            CharacterLevel,
            SelectedChoiceIds,
            GrantedPoints,
            SpentPoints,
            RemainingPoints))
    {
        return false;
    }

    OutSatisfiedRequirements.Add (ClassDefinition->ClassId);
    for (const FRPGClassProgressionLevelGrant& Grant :
        ClassDefinition->ProgressionLevelGrants)
    {
        if (Grant.Level > CharacterLevel)
        {
            continue;
        }
        for (const FName RequirementId : Grant.GrantedRequirementIds)
        {
            OutSatisfiedRequirements.Add (RequirementId);
        }
    }

    for (const FName ChoiceId : SelectedChoiceIds)
    {
        const FRPGClassProgressionChoiceDefinition* Choice =
            ClassDefinition->FindProgressionChoice (ChoiceId);
        if (!Choice)
        {
            OutSatisfiedRequirements.Reset ();
            return false;
        }

        // ChoiceId doubles as a stable feature/requirement id.
        OutSatisfiedRequirements.Add (Choice->ChoiceId);
        for (const FName RequirementId : Choice->GrantedRequirementIds)
        {
            OutSatisfiedRequirements.Add (RequirementId);
        }
    }
    return true;
}

bool FRPGClassProgressionService::CollectAutomaticSatisfiedRequirements (
    const URPGClassAsset* ClassDefinition,
    int32 CharacterLevel,
    TSet<FName>& OutSatisfiedRequirements)
{
    const TSet<FName> NoSelectedChoices;
    return CollectSatisfiedRequirements (
        ClassDefinition,
        CharacterLevel,
        NoSelectedChoices,
        OutSatisfiedRequirements);
}

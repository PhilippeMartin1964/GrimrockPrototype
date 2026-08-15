#include "RPG/RPGClassAsset.h"

#include "RPG/RPGCharacterRulesLibrary.h"

namespace
{
    bool AreMON154RequirementIdsValid (const TArray<FName>& RequirementIds)
    {
        TSet<FName> Seen;
        for (const FName RequirementId : RequirementIds)
        {
            if (RequirementId.IsNone () || Seen.Contains (RequirementId))
            {
                return false;
            }
            Seen.Add (RequirementId);
        }
        return true;
    }

    bool HasMON154ChoiceDependencyCycle (
        const TMap<FName, TArray<FName>>& Dependencies)
    {
        TSet<FName> Visiting;
        TSet<FName> Visited;

        TFunction<bool (FName)> Visit =
            [&] (FName ChoiceId) -> bool
            {
                if (Visited.Contains (ChoiceId))
                {
                    return false;
                }
                if (Visiting.Contains (ChoiceId))
                {
                    return true;
                }

                Visiting.Add (ChoiceId);
                if (const TArray<FName>* Prerequisites =
                    Dependencies.Find (ChoiceId))
                {
                    for (const FName PrerequisiteId : *Prerequisites)
                    {
                        if (Visit (PrerequisiteId))
                        {
                            return true;
                        }
                    }
                }
                Visiting.Remove (ChoiceId);
                Visited.Add (ChoiceId);
                return false;
            };

        for (const TPair<FName, TArray<FName>>& Entry : Dependencies)
        {
            if (Visit (Entry.Key))
            {
                return true;
            }
        }
        return false;
    }
}

bool URPGClassAsset::IsValidDefinition () const
{
    if (ClassId.IsNone () || HealthAtLevelOne <= 0)
    {
        return false;
    }

    TSet<FName> ActionIds;
    for (const FGridCombatActionDefinition& Action : CombatActions)
    {
        if (!Action.IsValid () ||
            (Action.SourcePolicy !=
                    EGridCombatActionSourcePolicy::Ability &&
                Action.SourcePolicy !=
                    EGridCombatActionSourcePolicy::Spell &&
                Action.SourcePolicy !=
                    EGridCombatActionSourcePolicy::Universal) ||
            Action.ResourceCosts.SourceItemQuantityCost != 0 ||
            (Action.SourcePolicy ==
                    EGridCombatActionSourcePolicy::Universal &&
                Action.ResourceCosts.ManaCost != 0) ||
            ActionIds.Contains (Action.ActionId))
        {
            return false;
        }
        ActionIds.Add (Action.ActionId);
    }

    const int32 MinimumLevel =
        URPGCharacterRulesLibrary::GetMinimumLevel ();
    const int32 MaximumLevel =
        URPGCharacterRulesLibrary::GetMaximumLevel ();

    TSet<int32> GrantLevels;
    TSet<FName> AutomaticallyGrantedRequirementIds;
    for (const FRPGClassProgressionLevelGrant& Grant :
        ProgressionLevelGrants)
    {
        if (Grant.Level < MinimumLevel ||
            Grant.Level > MaximumLevel ||
            Grant.ChoicePointsGranted < 0 ||
            (Grant.ChoicePointsGranted == 0 &&
                Grant.GrantedRequirementIds.IsEmpty ()) ||
            GrantLevels.Contains (Grant.Level) ||
            !AreMON154RequirementIdsValid (
                Grant.GrantedRequirementIds))
        {
            return false;
        }
        GrantLevels.Add (Grant.Level);

        for (const FName RequirementId : Grant.GrantedRequirementIds)
        {
            if (AutomaticallyGrantedRequirementIds.Contains (
                    RequirementId))
            {
                return false;
            }
            AutomaticallyGrantedRequirementIds.Add (RequirementId);
        }
    }

    TSet<FName> ChoiceIds;
    TMap<FName, TArray<FName>> ChoiceDependencies;
    for (const FRPGClassProgressionChoiceDefinition& Choice :
        ProgressionChoices)
    {
        if (Choice.ChoiceId.IsNone () ||
            Choice.MinimumLevel < MinimumLevel ||
            Choice.MinimumLevel > MaximumLevel ||
            Choice.PointCost <= 0 ||
            ChoiceIds.Contains (Choice.ChoiceId) ||
            !AreMON154RequirementIdsValid (
                Choice.PrerequisiteChoiceIds) ||
            !AreMON154RequirementIdsValid (
                Choice.GrantedRequirementIds) ||
            Choice.PrerequisiteChoiceIds.Contains (Choice.ChoiceId))
        {
            return false;
        }
        ChoiceIds.Add (Choice.ChoiceId);
        ChoiceDependencies.Add (
            Choice.ChoiceId,
            Choice.PrerequisiteChoiceIds);
    }

    for (const FRPGClassProgressionChoiceDefinition& Choice :
        ProgressionChoices)
    {
        for (const FName PrerequisiteId : Choice.PrerequisiteChoiceIds)
        {
            if (!ChoiceIds.Contains (PrerequisiteId))
            {
                return false;
            }
        }
    }

    if (HasMON154ChoiceDependencyCycle (ChoiceDependencies))
    {
        return false;
    }

    return true;
}

const FRPGClassProgressionChoiceDefinition*
URPGClassAsset::FindProgressionChoice (FName ChoiceId) const
{
    if (ChoiceId.IsNone ())
    {
        return nullptr;
    }

    return ProgressionChoices.FindByPredicate (
        [ChoiceId] (const FRPGClassProgressionChoiceDefinition& Choice)
        {
            return Choice.ChoiceId == ChoiceId;
        });
}

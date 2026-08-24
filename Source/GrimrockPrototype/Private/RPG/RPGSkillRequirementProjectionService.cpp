#include "RPG/RPGSkillRequirementProjectionService.h"

#include "Engine/AssetManager.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/GridInventoryTypes.h"

namespace
{
    const FPrimaryAssetType RPGSkillPrimaryAssetType (TEXT ("RPGSkill"));
}

bool FRPGSkillRequirementProjectionService::AppendSatisfiedRequirements (
    const FGridCharacterInventoryState& CharacterState,
    TSet<FName>& OutRequirements,
    FString& OutError)
{
    return AppendSatisfiedRequirements (
        CharacterState,
        [] (FName SkillId)
        {
            return FRPGSkillRequirementProjectionService::
                ResolveDefinitionBySkillId (SkillId);
        },
        OutRequirements,
        OutError);
}

bool FRPGSkillRequirementProjectionService::AppendSatisfiedRequirements (
    const FGridCharacterInventoryState& CharacterState,
    TFunctionRef<const URPGSkillAsset* (FName)> DefinitionResolver,
    TSet<FName>& OutRequirements,
    FString& OutError)
{
    if (!FRPGSkillService::ValidateSkillState (CharacterState))
    {
        OutError = TEXT ("Character skill state is structurally invalid.");
        return false;
    }

    TSet<FName> CandidateRequirements = OutRequirements;
    for (const FRPGSkillRank& SkillRank : CharacterState.SkillRanks)
    {
        const URPGSkillAsset* Definition =
            DefinitionResolver (SkillRank.SkillId);
        if (!IsValid (Definition) ||
            !Definition->IsValidDefinition () ||
            Definition->SkillId != SkillRank.SkillId)
        {
            OutError = FString::Printf (
                TEXT ("Skill '%s' cannot resolve a matching valid definition."),
                *SkillRank.SkillId.ToString ());
            return false;
        }
        if (SkillRank.Rank > Definition->MaxRank)
        {
            OutError = FString::Printf (
                TEXT ("Skill '%s' rank %d exceeds definition MaxRank %d."),
                *SkillRank.SkillId.ToString (),
                SkillRank.Rank,
                Definition->MaxRank);
            return false;
        }

        // Any trained skill satisfies its own stable SkillId requirement.
        CandidateRequirements.Add (SkillRank.SkillId);
        for (const FRPGSkillRequirementGrant& Grant :
            Definition->RequirementGrants)
        {
            if (SkillRank.Rank < Grant.MinimumRank)
            {
                continue;
            }
            for (const FName RequirementId : Grant.GrantedRequirementIds)
            {
                CandidateRequirements.Add (RequirementId);
            }
        }
    }

    OutRequirements = MoveTemp (CandidateRequirements);
    OutError.Reset ();
    return true;
}

const URPGSkillAsset*
FRPGSkillRequirementProjectionService::ResolveDefinitionBySkillId (
    FName SkillId)
{
    if (SkillId.IsNone ())
    {
        return nullptr;
    }

    UAssetManager& AssetManager = UAssetManager::Get ();
    const FPrimaryAssetId PrimaryAssetId (
        RPGSkillPrimaryAssetType,
        SkillId);

    URPGSkillAsset* Definition =
        AssetManager.GetPrimaryAssetObject<URPGSkillAsset> (PrimaryAssetId);
    if (!IsValid (Definition))
    {
        FSoftObjectPath AssetPath =
            AssetManager.GetPrimaryAssetPath (PrimaryAssetId);
        if (!AssetPath.IsValid ())
        {
            TArray<FString> SearchPaths;
            SearchPaths.Add (TEXT ("/Game"));
            AssetManager.ScanPathsForPrimaryAssets (
                RPGSkillPrimaryAssetType,
                SearchPaths,
                URPGSkillAsset::StaticClass (),
                false,
                false,
                true);
            AssetPath = AssetManager.GetPrimaryAssetPath (PrimaryAssetId);
        }
        if (AssetPath.IsValid ())
        {
            Definition = Cast<URPGSkillAsset> (AssetPath.TryLoad ());
        }
    }

    if (!IsValid (Definition) ||
        !Definition->IsValidDefinition () ||
        Definition->GetPrimaryAssetId () != PrimaryAssetId)
    {
        return nullptr;
    }
    return Definition;
}

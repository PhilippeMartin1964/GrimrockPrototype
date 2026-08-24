#include "UI/GridSkillsPageService.h"

#include "Engine/AssetManager.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillService.h"
#include "RPG/RPGTalentRuntimeService.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace
{
    const FPrimaryAssetType RPGSkillPrimaryAssetType (TEXT ("RPGSkill"));

    bool ValidateAndSortDefinitions (
        const TArray<const URPGSkillAsset*>& SkillDefinitions,
        TArray<const URPGSkillAsset*>& OutSortedDefinitions)
    {
        OutSortedDefinitions.Reset (SkillDefinitions.Num ());
        TSet<FName> SeenSkillIds;
        for (const URPGSkillAsset* Definition : SkillDefinitions)
        {
            if (!IsValid (Definition) ||
                !Definition->IsValidDefinition () ||
                SeenSkillIds.Contains (Definition->SkillId))
            {
                OutSortedDefinitions.Reset ();
                return false;
            }
            SeenSkillIds.Add (Definition->SkillId);
            OutSortedDefinitions.Add (Definition);
        }

        OutSortedDefinitions.Sort (
            [] (const URPGSkillAsset& Left, const URPGSkillAsset& Right)
            {
                return Left.SkillId.ToString ().Compare (
                    Right.SkillId.ToString (),
                    ESearchCase::CaseSensitive) < 0;
            });
        return true;
    }
}

bool FGridSkillsPageService::TryBuildCharacterView (
    UGridPartyInventoryComponent* PartyInventoryComponent,
    int32 CharacterIndex,
    const TArray<const URPGSkillAsset*>& SkillDefinitions,
    FGridSkillsPageView& OutView)
{
    OutView = FGridSkillsPageView ();
    if (!IsValid (PartyInventoryComponent) ||
        !PartyInventoryComponent->IsValidCharacterIndex (CharacterIndex))
    {
        return false;
    }

    const FGridCharacterInventoryState& Character =
        PartyInventoryComponent->PartyInventoryState.ActiveCharacters[
            CharacterIndex];
    if (!Character.CharacterId.IsValid () ||
        !FRPGSkillService::ValidateSkillState (Character))
    {
        return false;
    }

    TArray<const URPGSkillAsset*> SortedDefinitions;
    if (!ValidateAndSortDefinitions (
            SkillDefinitions,
            SortedDefinitions))
    {
        return false;
    }

    TSet<FName> DefinitionSkillIds;
    for (const URPGSkillAsset* Definition : SortedDefinitions)
    {
        DefinitionSkillIds.Add (Definition->SkillId);
    }
    for (const FRPGSkillRank& SkillRank : Character.SkillRanks)
    {
        if (!DefinitionSkillIds.Contains (SkillRank.SkillId))
        {
            return false;
        }
    }

    FGridSkillsPageView Candidate;
    Candidate.CharacterIndex = CharacterIndex;
    Candidate.CharacterId = Character.CharacterId;
    Candidate.CharacterName = Character.DisplayName;

    Candidate.Skills.Reserve (SortedDefinitions.Num ());
    for (const URPGSkillAsset* Definition : SortedDefinitions)
    {
        const int32 Rank = FRPGSkillService::GetSkillRank (
            Character,
            Definition->SkillId);
        if (Rank < 0 || Rank > Definition->MaxRank)
        {
            return false;
        }

        FGridSkillEntryView SkillView;
        SkillView.SkillId = Definition->SkillId;
        SkillView.DisplayName = Definition->DisplayName;
        SkillView.Description = Definition->Description;
        SkillView.GoverningAttribute = Definition->GoverningAttribute;
        SkillView.Rank = Rank;
        SkillView.MaxRank = Definition->MaxRank;
        SkillView.bTrained = Rank > 0;
        Candidate.Skills.Add (MoveTemp (SkillView));
    }

    TArray<FRPGTalentRuntimeView> TalentViews;
    if (!FRPGTalentRuntimeService::TryGetSelectedTalents (
            PartyInventoryComponent,
            CharacterIndex,
            TalentViews))
    {
        return false;
    }
    TalentViews.Sort (
        [] (const FRPGTalentRuntimeView& Left,
            const FRPGTalentRuntimeView& Right)
        {
            return Left.ChoiceId.ToString ().Compare (
                Right.ChoiceId.ToString (),
                ESearchCase::CaseSensitive) < 0;
        });

    Candidate.Talents.Reserve (TalentViews.Num ());
    for (const FRPGTalentRuntimeView& Talent : TalentViews)
    {
        FGridTalentEntryView TalentView;
        TalentView.ChoiceId = Talent.ChoiceId;
        TalentView.DisplayName = Talent.DisplayName;
        TalentView.Description = Talent.Description;
        TalentView.MinimumLevel = Talent.MinimumLevel;
        TalentView.PointCost = Talent.PointCost;
        TalentView.bSelected = Talent.bSelected;
        Candidate.Talents.Add (MoveTemp (TalentView));
    }

    FRPGTalentPointBalance Balance;
    if (!FRPGTalentRuntimeService::TryGetTalentPointBalance (
            PartyInventoryComponent,
            CharacterIndex,
            Balance))
    {
        return false;
    }
    Candidate.GrantedTalentPoints = Balance.GrantedPoints;
    Candidate.SpentTalentPoints = Balance.SpentPoints;
    Candidate.RemainingTalentPoints = Balance.RemainingPoints;

    OutView = MoveTemp (Candidate);
    return true;
}

bool FGridSkillsPageService::TryBuildSelectedCharacterView (
    UGridPartyInventoryComponent* PartyInventoryComponent,
    const TArray<const URPGSkillAsset*>& SkillDefinitions,
    FGridSkillsPageView& OutView)
{
    OutView = FGridSkillsPageView ();
    if (!IsValid (PartyInventoryComponent))
    {
        return false;
    }

    return TryBuildCharacterView (
        PartyInventoryComponent,
        PartyInventoryComponent->GetSelectedCharacterIndex (),
        SkillDefinitions,
        OutView);
}

void FGridSkillsPageService::ResolveCanonicalSkillDefinitions (
    TArray<const URPGSkillAsset*>& OutDefinitions)
{
    OutDefinitions.Reset ();

    UAssetManager& AssetManager = UAssetManager::Get ();
    TArray<FString> SearchPaths;
    SearchPaths.Add (TEXT ("/Game"));
    AssetManager.ScanPathsForPrimaryAssets (
        RPGSkillPrimaryAssetType,
        SearchPaths,
        URPGSkillAsset::StaticClass (),
        false,
        false,
        true);

    TArray<FPrimaryAssetId> AssetIds;
    AssetManager.GetPrimaryAssetIdList (
        RPGSkillPrimaryAssetType,
        AssetIds);
    AssetIds.Sort (
        [] (const FPrimaryAssetId& Left, const FPrimaryAssetId& Right)
        {
            return Left.PrimaryAssetName.ToString ().Compare (
                Right.PrimaryAssetName.ToString (),
                ESearchCase::CaseSensitive) < 0;
        });

    TSet<FName> SeenSkillIds;
    for (const FPrimaryAssetId& AssetId : AssetIds)
    {
        URPGSkillAsset* Definition =
            AssetManager.GetPrimaryAssetObject<URPGSkillAsset> (AssetId);
        if (!IsValid (Definition))
        {
            const FSoftObjectPath AssetPath =
                AssetManager.GetPrimaryAssetPath (AssetId);
            if (AssetPath.IsValid ())
            {
                Definition = Cast<URPGSkillAsset> (AssetPath.TryLoad ());
            }
        }

        if (!IsValid (Definition) ||
            !Definition->IsValidDefinition () ||
            Definition->GetPrimaryAssetId () != AssetId ||
            SeenSkillIds.Contains (Definition->SkillId))
        {
            continue;
        }
        SeenSkillIds.Add (Definition->SkillId);
        OutDefinitions.Add (Definition);
    }
}

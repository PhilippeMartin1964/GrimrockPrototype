#include "RPG/RPGSkillPersistence.h"

#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillRequirementProjectionService.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/GridInventoryTypes.h"

namespace RPGSkillPersistencePrivate
{
    bool CollectPartyCharacters (
        const FGridPartyInventoryState& PartyState,
        TArray<const FGridCharacterInventoryState*>& OutCharacters,
        TSet<FGuid>& OutCharacterIds,
        FString& OutError)
    {
        OutCharacters.Reset ();
        OutCharacterIds.Reset ();
        OutError.Reset ();

        const auto AppendCharacters =
            [&OutCharacters, &OutCharacterIds, &OutError] (
                const TArray<FGridCharacterInventoryState>& Characters,
                const TCHAR* Location) -> bool
        {
            for (int32 Index = 0; Index < Characters.Num (); ++Index)
            {
                const FGridCharacterInventoryState& Character = Characters[Index];
                if (!Character.CharacterId.IsValid ())
                {
                    OutError = FString::Printf (
                        TEXT ("%s[%d] has an invalid CharacterId."),
                        Location,
                        Index);
                    return false;
                }
                if (OutCharacterIds.Contains (Character.CharacterId))
                {
                    OutError = FString::Printf (
                        TEXT ("CharacterId %s is duplicated or ambiguous in the party snapshot."),
                        *Character.CharacterId.ToString (EGuidFormats::Digits));
                    return false;
                }

                OutCharacterIds.Add (Character.CharacterId);
                OutCharacters.Add (&Character);
            }
            return true;
        };

        return AppendCharacters (
                PartyState.ActiveCharacters,
                TEXT ("ActiveCharacter")) &&
            AppendCharacters (
                PartyState.CharacterPool,
                TEXT ("CharacterPool"));
    }

    bool ValidateDefinitionAndRank (
        FName SkillId,
        int32 Rank,
        TFunctionRef<const URPGSkillAsset* (FName)> DefinitionResolver,
        FString& OutError)
    {
        if (SkillId.IsNone () || Rank <= 0)
        {
            OutError = TEXT ("A persisted Skill rank contains an invalid SkillId or non-positive Rank.");
            return false;
        }

        const URPGSkillAsset* Definition = DefinitionResolver (SkillId);
        if (!IsValid (Definition) ||
            !Definition->IsValidDefinition () ||
            Definition->SkillId != SkillId)
        {
            OutError = FString::Printf (
                TEXT ("Skill '%s' cannot resolve a matching valid canonical definition."),
                *SkillId.ToString ());
            return false;
        }
        if (Rank > Definition->MaxRank)
        {
            OutError = FString::Printf (
                TEXT ("Skill '%s' persisted rank %d exceeds definition MaxRank %d."),
                *SkillId.ToString (),
                Rank,
                Definition->MaxRank);
            return false;
        }
        return true;
    }

    void SortSavedRanks (TArray<FRPGSkillRankSaveState>& SkillRanks)
    {
        SkillRanks.Sort ([] (
            const FRPGSkillRankSaveState& Left,
            const FRPGSkillRankSaveState& Right)
        {
            return Left.SkillId.ToString () < Right.SkillId.ToString ();
        });
    }

    void ResetRuntimeSkills (FGridPartyInventoryState& PartyState)
    {
        for (FGridCharacterInventoryState& Character : PartyState.ActiveCharacters)
        {
            Character.SkillRanks.Reset ();
        }
        for (FGridCharacterInventoryState& Character : PartyState.CharacterPool)
        {
            Character.SkillRanks.Reset ();
        }
    }

    FGridCharacterInventoryState* FindCharacterById (
        FGridPartyInventoryState& PartyState,
        const FGuid& CharacterId)
    {
        if (FGridCharacterInventoryState* Active =
            PartyState.ActiveCharacters.FindByPredicate (
                [&CharacterId] (const FGridCharacterInventoryState& Character)
                {
                    return Character.CharacterId == CharacterId;
                }))
        {
            return Active;
        }

        return PartyState.CharacterPool.FindByPredicate (
            [&CharacterId] (const FGridCharacterInventoryState& Character)
            {
                return Character.CharacterId == CharacterId;
            });
    }
}

using namespace RPGSkillPersistencePrivate;

bool FRPGSkillPersistence::CapturePartySkills (
    const FGridPartyInventoryState& PartyState,
    TArray<FRPGCharacterSkillSaveState>& OutSavedStates,
    FString& OutError)
{
    return CapturePartySkills (
        PartyState,
        [] (FName SkillId)
        {
            return FRPGSkillPersistence::ResolveDefinitionBySkillId (SkillId);
        },
        OutSavedStates,
        OutError);
}

bool FRPGSkillPersistence::CapturePartySkills (
    const FGridPartyInventoryState& PartyState,
    TFunctionRef<const URPGSkillAsset* (FName)> DefinitionResolver,
    TArray<FRPGCharacterSkillSaveState>& OutSavedStates,
    FString& OutError)
{
    TArray<const FGridCharacterInventoryState*> OrderedCharacters;
    TSet<FGuid> CharacterIds;
    if (!CollectPartyCharacters (
            PartyState,
            OrderedCharacters,
            CharacterIds,
            OutError))
    {
        return false;
    }

    TArray<FRPGCharacterSkillSaveState> Candidate;
    Candidate.Reserve (OrderedCharacters.Num ());
    for (const FGridCharacterInventoryState* Character : OrderedCharacters)
    {
        if (!Character || !FRPGSkillService::ValidateSkillState (*Character))
        {
            OutError = TEXT ("A party character has a structurally invalid runtime Skill state.");
            return false;
        }
        if (Character->SkillRanks.IsEmpty ())
        {
            continue;
        }

        FRPGCharacterSkillSaveState SavedCharacter;
        SavedCharacter.CharacterId = Character->CharacterId;
        SavedCharacter.SkillRanks.Reserve (Character->SkillRanks.Num ());
        for (const FRPGSkillRank& RuntimeRank : Character->SkillRanks)
        {
            if (!ValidateDefinitionAndRank (
                    RuntimeRank.SkillId,
                    RuntimeRank.Rank,
                    DefinitionResolver,
                    OutError))
            {
                return false;
            }

            FRPGSkillRankSaveState SavedRank;
            SavedRank.SkillId = RuntimeRank.SkillId;
            SavedRank.Rank = RuntimeRank.Rank;
            SavedCharacter.SkillRanks.Add (SavedRank);
        }
        SortSavedRanks (SavedCharacter.SkillRanks);
        Candidate.Add (MoveTemp (SavedCharacter));
    }

    Candidate.Sort ([] (
        const FRPGCharacterSkillSaveState& Left,
        const FRPGCharacterSkillSaveState& Right)
    {
        return Left.CharacterId.ToString (EGuidFormats::Digits) <
            Right.CharacterId.ToString (EGuidFormats::Digits);
    });

    OutSavedStates = MoveTemp (Candidate);
    OutError.Reset ();
    return true;
}

bool FRPGSkillPersistence::ValidateSavedPartySkills (
    const FGridPartyInventoryState& PartyState,
    const TArray<FRPGCharacterSkillSaveState>& SavedStates,
    FString& OutError)
{
    return ValidateSavedPartySkills (
        PartyState,
        SavedStates,
        [] (FName SkillId)
        {
            return FRPGSkillPersistence::ResolveDefinitionBySkillId (SkillId);
        },
        OutError);
}

bool FRPGSkillPersistence::ValidateSavedPartySkills (
    const FGridPartyInventoryState& PartyState,
    const TArray<FRPGCharacterSkillSaveState>& SavedStates,
    TFunctionRef<const URPGSkillAsset* (FName)> DefinitionResolver,
    FString& OutError)
{
    TArray<const FGridCharacterInventoryState*> OrderedCharacters;
    TSet<FGuid> PartyCharacterIds;
    if (!CollectPartyCharacters (
            PartyState,
            OrderedCharacters,
            PartyCharacterIds,
            OutError))
    {
        return false;
    }

    TSet<FGuid> SeenCharacterIds;
    for (const FRPGCharacterSkillSaveState& SavedCharacter : SavedStates)
    {
        if (!SavedCharacter.CharacterId.IsValid ())
        {
            OutError = TEXT ("Saved Skill state contains an invalid CharacterId.");
            return false;
        }
        if (SeenCharacterIds.Contains (SavedCharacter.CharacterId))
        {
            OutError = FString::Printf (
                TEXT ("Saved Skill state duplicates CharacterId %s."),
                *SavedCharacter.CharacterId.ToString (EGuidFormats::Digits));
            return false;
        }
        if (!PartyCharacterIds.Contains (SavedCharacter.CharacterId))
        {
            OutError = FString::Printf (
                TEXT ("Saved Skill state references orphan CharacterId %s."),
                *SavedCharacter.CharacterId.ToString (EGuidFormats::Digits));
            return false;
        }
        if (SavedCharacter.SkillRanks.IsEmpty ())
        {
            OutError = FString::Printf (
                TEXT ("Saved Skill state %s is empty and must be omitted from the sparse snapshot."),
                *SavedCharacter.CharacterId.ToString (EGuidFormats::Digits));
            return false;
        }

        TSet<FName> SeenSkillIds;
        for (const FRPGSkillRankSaveState& SavedRank : SavedCharacter.SkillRanks)
        {
            if (!SavedRank.IsValid () || SeenSkillIds.Contains (SavedRank.SkillId))
            {
                OutError = FString::Printf (
                    TEXT ("Saved Skill state %s contains an invalid or duplicate SkillId."),
                    *SavedCharacter.CharacterId.ToString (EGuidFormats::Digits));
                return false;
            }
            if (!ValidateDefinitionAndRank (
                    SavedRank.SkillId,
                    SavedRank.Rank,
                    DefinitionResolver,
                    OutError))
            {
                return false;
            }
            SeenSkillIds.Add (SavedRank.SkillId);
        }

        SeenCharacterIds.Add (SavedCharacter.CharacterId);
    }

    OutError.Reset ();
    return true;
}

bool FRPGSkillPersistence::RestorePartySkills (
    FGridPartyInventoryState& InOutPartyState,
    const TArray<FRPGCharacterSkillSaveState>& SavedStates,
    FString& OutError)
{
    return RestorePartySkills (
        InOutPartyState,
        SavedStates,
        [] (FName SkillId)
        {
            return FRPGSkillPersistence::ResolveDefinitionBySkillId (SkillId);
        },
        OutError);
}

bool FRPGSkillPersistence::RestorePartySkills (
    FGridPartyInventoryState& InOutPartyState,
    const TArray<FRPGCharacterSkillSaveState>& SavedStates,
    TFunctionRef<const URPGSkillAsset* (FName)> DefinitionResolver,
    FString& OutError)
{
    if (!ValidateSavedPartySkills (
            InOutPartyState,
            SavedStates,
            DefinitionResolver,
            OutError))
    {
        return false;
    }

    FGridPartyInventoryState Candidate = InOutPartyState;
    ResetRuntimeSkills (Candidate);

    for (const FRPGCharacterSkillSaveState& SavedCharacter : SavedStates)
    {
        FGridCharacterInventoryState* TargetCharacter =
            FindCharacterById (Candidate, SavedCharacter.CharacterId);
        if (!TargetCharacter)
        {
            OutError = FString::Printf (
                TEXT ("Saved Skill state cannot resolve CharacterId %s."),
                *SavedCharacter.CharacterId.ToString (EGuidFormats::Digits));
            return false;
        }

        TArray<FRPGSkillRankSaveState> OrderedRanks = SavedCharacter.SkillRanks;
        SortSavedRanks (OrderedRanks);
        for (const FRPGSkillRankSaveState& SavedRank : OrderedRanks)
        {
            const URPGSkillAsset* Definition = DefinitionResolver (SavedRank.SkillId);
            FRPGSkillMutationResult MutationResult;
            if (!FRPGSkillService::TrySetSkillRank (
                    *TargetCharacter,
                    Definition,
                    SavedRank.Rank,
                    MutationResult))
            {
                OutError = FString::Printf (
                    TEXT ("Skill '%s' could not be restored for CharacterId %s."),
                    *SavedRank.SkillId.ToString (),
                    *SavedCharacter.CharacterId.ToString (EGuidFormats::Digits));
                return false;
            }
        }
    }

    InOutPartyState = MoveTemp (Candidate);
    OutError.Reset ();
    return true;
}

const URPGSkillAsset* FRPGSkillPersistence::ResolveDefinitionBySkillId (
    FName SkillId)
{
    return FRPGSkillRequirementProjectionService::ResolveDefinitionBySkillId (
        SkillId);
}

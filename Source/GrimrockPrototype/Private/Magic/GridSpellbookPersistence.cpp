#include "Magic/GridSpellbookPersistence.h"

namespace GridSpellbookPersistencePrivate
{
    bool CollectPartyCharacterIds (
        const FGridPartyInventoryState& PartyState,
        TArray<FGuid>& OutOrderedIds,
        TSet<FGuid>& OutUniqueIds,
        FString& OutError)
    {
        OutOrderedIds.Reset ();
        OutUniqueIds.Reset ();
        OutError.Reset ();

        const auto AppendCharacters =
            [&OutOrderedIds, &OutUniqueIds, &OutError] (
                const TArray<FGridCharacterInventoryState>& Characters,
                const TCHAR* Location) -> bool
        {
            for (int32 Index = 0; Index < Characters.Num (); ++Index)
            {
                const FGuid CharacterId = Characters[Index].CharacterId;
                if (!CharacterId.IsValid ())
                {
                    OutError = FString::Printf (
                        TEXT ("%s[%d] has an invalid CharacterId."),
                        Location,
                        Index);
                    return false;
                }
                if (OutUniqueIds.Contains (CharacterId))
                {
                    OutError = FString::Printf (
                        TEXT ("CharacterId %s is duplicated or ambiguous in the party snapshot."),
                        *CharacterId.ToString (EGuidFormats::Digits));
                    return false;
                }

                OutUniqueIds.Add (CharacterId);
                OutOrderedIds.Add (CharacterId);
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

    bool ValidateSpellIds (
        const TArray<FName>& SpellIds,
        const FGuid& CharacterId,
        FString& OutError)
    {
        TSet<FName> SeenSpellIds;
        for (const FName SpellId : SpellIds)
        {
            if (SpellId.IsNone ())
            {
                OutError = FString::Printf (
                    TEXT ("Spellbook %s contains NAME_None."),
                    *CharacterId.ToString (EGuidFormats::Digits));
                return false;
            }
            if (SeenSpellIds.Contains (SpellId))
            {
                OutError = FString::Printf (
                    TEXT ("Spellbook %s contains duplicate SpellId '%s'."),
                    *CharacterId.ToString (EGuidFormats::Digits),
                    *SpellId.ToString ());
                return false;
            }
            SeenSpellIds.Add (SpellId);
        }
        return true;
    }

    void SortSpellIds (TArray<FName>& SpellIds)
    {
        SpellIds.Sort ([] (const FName Left, const FName Right)
        {
            return Left.ToString () < Right.ToString ();
        });
    }
}

using namespace GridSpellbookPersistencePrivate;

bool FGridSpellbookPersistence::CapturePartySpellbooks (
    const FGridPartyInventoryState& PartyState,
    const FGridPartySpellbookState& RuntimeState,
    TArray<FGridCharacterSpellbookSaveState>& OutSavedStates,
    FString& OutError)
{
    TArray<FGuid> OrderedPartyIds;
    TSet<FGuid> PartyIds;
    if (!CollectPartyCharacterIds (
            PartyState,
            OrderedPartyIds,
            PartyIds,
            OutError))
    {
        return false;
    }

    if (!RuntimeState.IsValid ())
    {
        OutError = TEXT ("Runtime spellbook state is structurally invalid.");
        return false;
    }

    TArray<FGridCharacterSpellbookSaveState> Candidate;
    Candidate.Reserve (RuntimeState.CharacterSpellbooks.Num ());
    for (const FGridCharacterSpellbookState& RuntimeSpellbook :
        RuntimeState.CharacterSpellbooks)
    {
        if (!PartyIds.Contains (RuntimeSpellbook.CharacterId))
        {
            OutError = FString::Printf (
                TEXT ("Runtime spellbook references orphan CharacterId %s."),
                *RuntimeSpellbook.CharacterId.ToString (EGuidFormats::Digits));
            return false;
        }
        if (!ValidateSpellIds (
                RuntimeSpellbook.KnownSpellIds,
                RuntimeSpellbook.CharacterId,
                OutError))
        {
            return false;
        }
        if (RuntimeSpellbook.KnownSpellIds.IsEmpty ())
        {
            continue;
        }

        FGridCharacterSpellbookSaveState Saved;
        Saved.CharacterId = RuntimeSpellbook.CharacterId;
        Saved.KnownSpellIds = RuntimeSpellbook.KnownSpellIds;
        SortSpellIds (Saved.KnownSpellIds);
        Candidate.Add (MoveTemp (Saved));
    }

    Candidate.Sort ([] (
        const FGridCharacterSpellbookSaveState& Left,
        const FGridCharacterSpellbookSaveState& Right)
    {
        return Left.CharacterId.ToString (EGuidFormats::Digits) <
            Right.CharacterId.ToString (EGuidFormats::Digits);
    });

    OutSavedStates = MoveTemp (Candidate);
    OutError.Reset ();
    return true;
}

bool FGridSpellbookPersistence::ValidateSavedPartySpellbooks (
    const FGridPartyInventoryState& PartyState,
    const TArray<FGridCharacterSpellbookSaveState>& SavedStates,
    FString& OutError)
{
    TArray<FGuid> OrderedPartyIds;
    TSet<FGuid> PartyIds;
    if (!CollectPartyCharacterIds (
            PartyState,
            OrderedPartyIds,
            PartyIds,
            OutError))
    {
        return false;
    }

    TSet<FGuid> SeenCharacterIds;
    for (const FGridCharacterSpellbookSaveState& Saved : SavedStates)
    {
        if (!Saved.CharacterId.IsValid ())
        {
            OutError = TEXT ("Saved spellbook contains an invalid CharacterId.");
            return false;
        }
        if (SeenCharacterIds.Contains (Saved.CharacterId))
        {
            OutError = FString::Printf (
                TEXT ("Saved spellbook duplicates CharacterId %s."),
                *Saved.CharacterId.ToString (EGuidFormats::Digits));
            return false;
        }
        if (!PartyIds.Contains (Saved.CharacterId))
        {
            OutError = FString::Printf (
                TEXT ("Saved spellbook references orphan CharacterId %s."),
                *Saved.CharacterId.ToString (EGuidFormats::Digits));
            return false;
        }
        if (!ValidateSpellIds (
                Saved.KnownSpellIds,
                Saved.CharacterId,
                OutError))
        {
            return false;
        }

        SeenCharacterIds.Add (Saved.CharacterId);
    }

    OutError.Reset ();
    return true;
}

bool FGridSpellbookPersistence::RestorePartySpellbooks (
    const FGridPartyInventoryState& PartyState,
    const TArray<FGridCharacterSpellbookSaveState>& SavedStates,
    FGridPartySpellbookState& OutRuntimeState,
    FString& OutError)
{
    if (!ValidateSavedPartySpellbooks (
            PartyState,
            SavedStates,
            OutError))
    {
        return false;
    }

    TArray<FGuid> OrderedPartyIds;
    TSet<FGuid> PartyIds;
    if (!CollectPartyCharacterIds (
            PartyState,
            OrderedPartyIds,
            PartyIds,
            OutError))
    {
        return false;
    }

    FGridPartySpellbookState Candidate;
    for (const FGuid& CharacterId : OrderedPartyIds)
    {
        if (!Candidate.EnsureCharacter (CharacterId))
        {
            OutError = FString::Printf (
                TEXT ("Could not create runtime spellbook for CharacterId %s."),
                *CharacterId.ToString (EGuidFormats::Digits));
            return false;
        }
    }

    for (const FGridCharacterSpellbookSaveState& Saved : SavedStates)
    {
        FGridCharacterSpellbookState* RuntimeSpellbook =
            Candidate.FindMutableSpellbook (Saved.CharacterId);
        if (!RuntimeSpellbook)
        {
            OutError = FString::Printf (
                TEXT ("Saved spellbook cannot resolve CharacterId %s."),
                *Saved.CharacterId.ToString (EGuidFormats::Digits));
            return false;
        }

        RuntimeSpellbook->KnownSpellIds = Saved.KnownSpellIds;
        SortSpellIds (RuntimeSpellbook->KnownSpellIds);
    }

    if (!Candidate.IsValid ())
    {
        OutError = TEXT ("Restored runtime spellbook candidate is invalid.");
        return false;
    }

    OutRuntimeState = MoveTemp (Candidate);
    OutError.Reset ();
    return true;
}

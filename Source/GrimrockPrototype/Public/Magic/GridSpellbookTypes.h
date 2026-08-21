#pragma once

#include "CoreMinimal.h"
#include "GridSpellbookTypes.generated.h"

UENUM (BlueprintType)
enum class EGridSpellbookMutationResult : uint8
{
    Success          UMETA (DisplayName = "Success"),
    InvalidCharacter UMETA (DisplayName = "Invalid Character"),
    InvalidSpell     UMETA (DisplayName = "Invalid Spell"),
    AlreadyKnown     UMETA (DisplayName = "Already Known"),
    NotKnown         UMETA (DisplayName = "Not Known")
};

/** Runtime spell knowledge owned by one stable CharacterId. */
USTRUCT (BlueprintType)
struct FGridCharacterSpellbookState
{
    GENERATED_BODY ()

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Magic|Spellbook")
    FGuid CharacterId;

    /** Stable MON18.1 identities only. Definitions are resolved elsewhere. */
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Magic|Spellbook")
    TArray<FName> KnownSpellIds;

    bool KnowsSpell (FName SpellId) const
    {
        return !SpellId.IsNone () && KnownSpellIds.Contains (SpellId);
    }

    EGridSpellbookMutationResult LearnSpell (FName SpellId)
    {
        if (!CharacterId.IsValid ())
        {
            return EGridSpellbookMutationResult::InvalidCharacter;
        }
        if (SpellId.IsNone ())
        {
            return EGridSpellbookMutationResult::InvalidSpell;
        }
        if (KnownSpellIds.Contains (SpellId))
        {
            return EGridSpellbookMutationResult::AlreadyKnown;
        }

        KnownSpellIds.Add (SpellId);
        return EGridSpellbookMutationResult::Success;
    }

    EGridSpellbookMutationResult ForgetSpell (FName SpellId)
    {
        if (!CharacterId.IsValid ())
        {
            return EGridSpellbookMutationResult::InvalidCharacter;
        }
        if (SpellId.IsNone ())
        {
            return EGridSpellbookMutationResult::InvalidSpell;
        }

        return KnownSpellIds.RemoveSingle (SpellId) > 0
            ? EGridSpellbookMutationResult::Success
            : EGridSpellbookMutationResult::NotKnown;
    }

    void Sanitize ()
    {
        TSet<FName> SeenSpellIds;
        for (int32 Index = KnownSpellIds.Num () - 1; Index >= 0; --Index)
        {
            const FName SpellId = KnownSpellIds[Index];
            if (SpellId.IsNone () || SeenSpellIds.Contains (SpellId))
            {
                KnownSpellIds.RemoveAt (Index);
                continue;
            }
            SeenSpellIds.Add (SpellId);
        }
    }

    bool IsValid () const
    {
        if (!CharacterId.IsValid ())
        {
            return false;
        }

        TSet<FName> SeenSpellIds;
        for (const FName SpellId : KnownSpellIds)
        {
            if (SpellId.IsNone () || SeenSpellIds.Contains (SpellId))
            {
                return false;
            }
            SeenSpellIds.Add (SpellId);
        }
        return true;
    }
};

/** Party-wide runtime collection. One spellbook is allowed per CharacterId. */
USTRUCT (BlueprintType)
struct FGridPartySpellbookState
{
    GENERATED_BODY ()

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Magic|Spellbook")
    TArray<FGridCharacterSpellbookState> CharacterSpellbooks;

    const FGridCharacterSpellbookState* FindSpellbook (const FGuid& CharacterId) const
    {
        return CharacterSpellbooks.FindByPredicate (
            [&CharacterId] (const FGridCharacterSpellbookState& Spellbook)
            {
                return Spellbook.CharacterId == CharacterId;
            });
    }

    FGridCharacterSpellbookState* FindMutableSpellbook (const FGuid& CharacterId)
    {
        return CharacterSpellbooks.FindByPredicate (
            [&CharacterId] (FGridCharacterSpellbookState& Spellbook)
            {
                return Spellbook.CharacterId == CharacterId;
            });
    }

    bool EnsureCharacter (const FGuid& CharacterId)
    {
        if (!CharacterId.IsValid ())
        {
            return false;
        }
        if (FindSpellbook (CharacterId))
        {
            return true;
        }

        FGridCharacterSpellbookState& NewSpellbook = CharacterSpellbooks.AddDefaulted_GetRef ();
        NewSpellbook.CharacterId = CharacterId;
        return true;
    }

    bool RemoveCharacter (const FGuid& CharacterId)
    {
        return CharacterId.IsValid () &&
            CharacterSpellbooks.RemoveAll (
                [&CharacterId] (const FGridCharacterSpellbookState& Spellbook)
                {
                    return Spellbook.CharacterId == CharacterId;
                }) > 0;
    }

    bool KnowsSpell (const FGuid& CharacterId, FName SpellId) const
    {
        const FGridCharacterSpellbookState* Spellbook = FindSpellbook (CharacterId);
        return Spellbook && Spellbook->KnowsSpell (SpellId);
    }

    EGridSpellbookMutationResult LearnSpell (const FGuid& CharacterId, FName SpellId)
    {
        FGridCharacterSpellbookState* Spellbook = FindMutableSpellbook (CharacterId);
        return Spellbook
            ? Spellbook->LearnSpell (SpellId)
            : EGridSpellbookMutationResult::InvalidCharacter;
    }

    EGridSpellbookMutationResult ForgetSpell (const FGuid& CharacterId, FName SpellId)
    {
        FGridCharacterSpellbookState* Spellbook = FindMutableSpellbook (CharacterId);
        return Spellbook
            ? Spellbook->ForgetSpell (SpellId)
            : EGridSpellbookMutationResult::InvalidCharacter;
    }

    bool IsValid () const
    {
        TSet<FGuid> SeenCharacterIds;
        for (const FGridCharacterSpellbookState& Spellbook : CharacterSpellbooks)
        {
            if (!Spellbook.IsValid () || SeenCharacterIds.Contains (Spellbook.CharacterId))
            {
                return false;
            }
            SeenCharacterIds.Add (Spellbook.CharacterId);
        }
        return true;
    }

    void Reset ()
    {
        CharacterSpellbooks.Reset ();
    }
};

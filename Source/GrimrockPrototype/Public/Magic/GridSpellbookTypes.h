#pragma once

#include "CoreMinimal.h"
#include "GridSpellbookTypes.generated.h"

UENUM(BlueprintType)
enum class EGridSpellbookMutationResult : uint8
{
	Success UMETA(DisplayName = "Success"),
	InvalidCharacter UMETA(DisplayName = "Invalid Character"),
	InvalidSpell UMETA(DisplayName = "Invalid Spell"),
	AlreadyKnown UMETA(DisplayName = "Already Known"),
	NotKnown UMETA(DisplayName = "Not Known")
};

/**
 * Ephemeral read/transaction view for one character Spellbook.
 * TD07.3.3.7 durable authority lives in FGridCharacterInventoryState::KnownSpellIds.
 * This struct is never stored as party or SaveGame authority.
 */
USTRUCT(BlueprintType)
struct FGridCharacterSpellbookState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Magic|Spellbook")
	FGuid CharacterId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Magic|Spellbook")
	TArray<FName> KnownSpellIds;

	bool KnowsSpell(FName SpellId) const
	{
		return !SpellId.IsNone() && KnownSpellIds.Contains(SpellId);
	}

	EGridSpellbookMutationResult LearnSpell(FName SpellId)
	{
		if (!CharacterId.IsValid())
		{
			return EGridSpellbookMutationResult::InvalidCharacter;
		}
		if (SpellId.IsNone())
		{
			return EGridSpellbookMutationResult::InvalidSpell;
		}
		if (KnownSpellIds.Contains(SpellId))
		{
			return EGridSpellbookMutationResult::AlreadyKnown;
		}

		KnownSpellIds.Add(SpellId);
		KnownSpellIds.Sort(
			[](const FName Left, const FName Right)
			{
				return Left.ToString() < Right.ToString();
			});
		return EGridSpellbookMutationResult::Success;
	}

	EGridSpellbookMutationResult ForgetSpell(FName SpellId)
	{
		if (!CharacterId.IsValid())
		{
			return EGridSpellbookMutationResult::InvalidCharacter;
		}
		if (SpellId.IsNone())
		{
			return EGridSpellbookMutationResult::InvalidSpell;
		}

		return KnownSpellIds.RemoveSingle(SpellId) > 0 ? EGridSpellbookMutationResult::Success : EGridSpellbookMutationResult::NotKnown;
	}

	void Sanitize()
	{
		TSet<FName> SeenSpellIds;
		for (int32 Index = KnownSpellIds.Num() - 1; Index >= 0; --Index)
		{
			const FName SpellId = KnownSpellIds[Index];
			if (SpellId.IsNone() || SeenSpellIds.Contains(SpellId))
			{
				KnownSpellIds.RemoveAt(Index);
				continue;
			}
			SeenSpellIds.Add(SpellId);
		}
		KnownSpellIds.Sort(
			[](const FName Left, const FName Right)
			{
				return Left.ToString() < Right.ToString();
			});
	}

	bool IsValid() const
	{
		if (!CharacterId.IsValid())
		{
			return false;
		}

		TSet<FName> SeenSpellIds;
		for (const FName SpellId : KnownSpellIds)
		{
			if (SpellId.IsNone() || SeenSpellIds.Contains(SpellId))
			{
				return false;
			}
			SeenSpellIds.Add(SpellId);
		}
		return true;
	}
};

#include "RPG/RPGCharacterIdentityPersistence.h"

#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridInventoryTypes.h"

namespace RPGCharacterIdentityPersistencePrivate
{
	URPGClassAsset* ResolveCharacterClass(const FGridCharacterInventoryState& Character)
	{
		URPGClassAsset* Definition = Character.ClassDefinition.Get();
		if (FRPGAuthoringIdentityResolver::IsMatchingClassDefinition(Character.ClassId, Definition))
		{
			FRPGAuthoringIdentityResolver::RememberClassDefinition(Definition);
			return Definition;
		}
		return FRPGAuthoringIdentityResolver::ResolveClassById(Character.ClassId);
	}

	bool RehydrateCharacter(FGridCharacterInventoryState& Character, const TCHAR* Location, FString& OutError)
	{
		if (Character.ClassId.IsNone())
		{
			Character.ClassDefinition.Reset();
			Character.ClassDisplayName = FText::GetEmpty();
		}
		else
		{
			URPGClassAsset* ClassDefinition = ResolveCharacterClass(Character);
			if (!ClassDefinition)
			{
				OutError = FString::Printf(TEXT("%s references unresolved ClassId=%s."), Location, *Character.ClassId.ToString());
				return false;
			}
			Character.ClassDefinition = ClassDefinition;
			Character.ClassDisplayName = ClassDefinition->DisplayName;
		}

		if (Character.RaceId.IsNone())
		{
			Character.RaceDisplayName = FText::GetEmpty();
		}
		else
		{
			if (URPGRaceAsset* RaceDefinition = FRPGAuthoringIdentityResolver::ResolveRaceById(Character.RaceId))
			{
				Character.RaceDisplayName = RaceDefinition->DisplayName;
			}
			else
			{
				// Race metadata is presentation-only in TD07.3.4.3. Keep the save usable
				// even if a race authoring asset is temporarily unavailable.
				Character.RaceDisplayName = FText::FromName(Character.RaceId);
			}
		}

		return true;
	}

	bool ValidateCharacter(const FGridCharacterInventoryState& Character, const TCHAR* Location, FString& OutError)
	{
		if (!Character.ClassId.IsNone())
		{
			URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get();
			if (!FRPGAuthoringIdentityResolver::IsMatchingClassDefinition(Character.ClassId, ClassDefinition))
			{
				OutError = FString::Printf(TEXT("%s has a ClassDefinition cache inconsistent with ClassId=%s."), Location, *Character.ClassId.ToString());
				return false;
			}
			if (!Character.ClassDisplayName.EqualTo(ClassDefinition->DisplayName))
			{
				OutError = FString::Printf(TEXT("%s has a ClassDisplayName cache inconsistent with ClassId=%s."), Location, *Character.ClassId.ToString());
				return false;
			}
		}
		else if (!Character.ClassDefinition.IsNull() || !Character.ClassDisplayName.IsEmpty())
		{
			OutError = FString::Printf(TEXT("%s has class caches without a ClassId."), Location);
			return false;
		}

		if (!Character.RaceId.IsNone())
		{
			if (URPGRaceAsset* RaceDefinition = FRPGAuthoringIdentityResolver::ResolveRaceById(Character.RaceId))
			{
				if (!Character.RaceDisplayName.EqualTo(RaceDefinition->DisplayName))
				{
					OutError = FString::Printf(TEXT("%s has RaceDisplayName inconsistent with RaceId=%s."), Location, *Character.RaceId.ToString());
					return false;
				}
			}
			else if (!Character.RaceDisplayName.EqualTo(FText::FromName(Character.RaceId)))
			{
				OutError = FString::Printf(TEXT("%s has an unresolved RaceId with a non-canonical fallback label."), Location);
				return false;
			}
		}
		else if (!Character.RaceDisplayName.IsEmpty())
		{
			OutError = FString::Printf(TEXT("%s has a RaceDisplayName cache without a RaceId."), Location);
			return false;
		}

		return true;
	}

	template <typename CharacterArray>
	bool RehydrateArray(CharacterArray& Characters, const TCHAR* Prefix, FString& OutError)
	{
		for (int32 Index = 0; Index < Characters.Num(); ++Index)
		{
			const FString Location = FString::Printf(TEXT("%s[%d]"), Prefix, Index);
			if (!RehydrateCharacter(Characters[Index], *Location, OutError))
			{
				return false;
			}
		}
		return true;
	}

	template <typename CharacterArray>
	bool ValidateArray(const CharacterArray& Characters, const TCHAR* Prefix, FString& OutError)
	{
		for (int32 Index = 0; Index < Characters.Num(); ++Index)
		{
			const FString Location = FString::Printf(TEXT("%s[%d]"), Prefix, Index);
			if (!ValidateCharacter(Characters[Index], *Location, OutError))
			{
				return false;
			}
		}
		return true;
	}
}

void FRPGCharacterIdentityPersistence::RememberRuntimeCaches(const FGridPartyInventoryState& PartyState)
{
	auto Remember = [](const FGridCharacterInventoryState& Character)
	{
		if (URPGClassAsset* Definition = Character.ClassDefinition.Get())
		{
			FRPGAuthoringIdentityResolver::RememberClassDefinition(Definition);
		}
	};

	for (const FGridCharacterInventoryState& Character : PartyState.ActiveCharacters)
	{
		Remember(Character);
	}
	for (const FGridCharacterInventoryState& Character : PartyState.CharacterPool)
	{
		Remember(Character);
	}
}

bool FRPGCharacterIdentityPersistence::RehydratePartyIdentity(FGridPartyInventoryState& PartyState, FString& OutError)
{
	using namespace RPGCharacterIdentityPersistencePrivate;
	OutError.Reset();

	FGridPartyInventoryState Candidate = PartyState;
	if (!RehydrateArray(Candidate.ActiveCharacters, TEXT("ActiveCharacter"), OutError) ||
		!RehydrateArray(Candidate.CharacterPool, TEXT("CharacterPool"), OutError))
	{
		return false;
	}

	PartyState = MoveTemp(Candidate);
	return true;
}

bool FRPGCharacterIdentityPersistence::ValidateRuntimePartyIdentity(const FGridPartyInventoryState& PartyState, FString& OutError)
{
	using namespace RPGCharacterIdentityPersistencePrivate;
	OutError.Reset();
	return ValidateArray(PartyState.ActiveCharacters, TEXT("ActiveCharacter"), OutError) &&
		ValidateArray(PartyState.CharacterPool, TEXT("CharacterPool"), OutError);
}

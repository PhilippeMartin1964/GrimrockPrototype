#include "RPG/RPGCharacterPortraitSetAsset.h"

namespace
{
	bool HasValidPortrait(const TArray<FRPGCharacterPortraitVariant>& Portraits)
	{
		for (const FRPGCharacterPortraitVariant& Portrait : Portraits)
		{
			if (Portrait.IsValidDefinition())
			{
				return true;
			}
		}
		return false;
	}
}

bool URPGCharacterPortraitSetAsset::IsValidDefinition() const
{
	return !RaceId.IsNone() && (HasValidPortrait(MalePortraits) || HasValidPortrait(FemalePortraits));
}

bool URPGCharacterPortraitSetAsset::IsValidForRace(FName InRaceId) const
{
	return IsValidDefinition() && !InRaceId.IsNone() && RaceId == InRaceId;
}

void URPGCharacterPortraitSetAsset::GetPortraitsForGender(ERPGCharacterPortraitGender Gender, TArray<FRPGCharacterPortraitVariant>& OutPortraits) const
{
	OutPortraits = GetPortraitsForGenderRef(Gender);
}

bool URPGCharacterPortraitSetAsset::FindPortraitVariant(ERPGCharacterPortraitGender Gender, FName VariantId, FRPGCharacterPortraitVariant& OutVariant) const
{
	OutVariant = FRPGCharacterPortraitVariant();
	if (VariantId.IsNone())
	{
		return false;
	}

	for (const FRPGCharacterPortraitVariant& Portrait : GetPortraitsForGenderRef(Gender))
	{
		if (Portrait.IsValidDefinition() && Portrait.VariantId == VariantId)
		{
			OutVariant = Portrait;
			return true;
		}
	}

	return false;
}

bool URPGCharacterPortraitSetAsset::GetFirstValidPortrait(ERPGCharacterPortraitGender Gender, FRPGCharacterPortraitVariant& OutVariant) const
{
	OutVariant = FRPGCharacterPortraitVariant();
	for (const FRPGCharacterPortraitVariant& Portrait : GetPortraitsForGenderRef(Gender))
	{
		if (Portrait.IsValidDefinition())
		{
			OutVariant = Portrait;
			return true;
		}
	}

	return false;
}

const TArray<FRPGCharacterPortraitVariant>& URPGCharacterPortraitSetAsset::GetPortraitsForGenderRef(ERPGCharacterPortraitGender Gender) const
{
	return Gender == ERPGCharacterPortraitGender::Female ? FemalePortraits : MalePortraits;
}

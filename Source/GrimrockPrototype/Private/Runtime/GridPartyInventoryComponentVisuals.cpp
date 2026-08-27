#include "Runtime/GridPartyInventoryComponent.h"

#include "Engine/Texture2D.h"
#include "RPG/RPGAuthoringIdentityResolver.h"

bool UGridPartyInventoryComponent::SetCharacterVisualSelection(int32 CharacterIndex, ERPGCharacterPortraitGender PortraitGender, FName PortraitVariantId,
	TSoftObjectPtr<UTexture2D> Portrait, TSoftObjectPtr<UTexture2D> ClassIcon)
{
	if (!IsValidCharacterIndex(CharacterIndex))
	{
		return false;
	}

	FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (!Portrait.IsNull() && PortraitVariantId.IsNone())
	{
		return false;
	}

	CharacterState.PortraitGender = PortraitGender;
	CharacterState.PortraitVariantId = PortraitVariantId;
	FRPGAuthoringIdentityResolver::RememberPortraitVisual(
		CharacterState.RaceId, PortraitGender, PortraitVariantId, Portrait);
	FRPGAuthoringIdentityResolver::RememberClassIcon(CharacterState.ClassId, ClassIcon);
	CharacterState.Portrait = FRPGAuthoringIdentityResolver::ResolvePortraitVisual(
		CharacterState.RaceId, PortraitGender, PortraitVariantId);
	CharacterState.ClassIcon = FRPGAuthoringIdentityResolver::ResolveClassIcon(CharacterState.ClassId);
	NotifyPartyInventoryChanged(CharacterIndex);
	return true;
}

bool UGridPartyInventoryComponent::GetCharacterVisualSelection(int32 CharacterIndex, FRPGCharacterVisualSelection& OutSelection) const
{
	OutSelection = FRPGCharacterVisualSelection();
	if (!IsValidCharacterIndex(CharacterIndex))
	{
		return false;
	}

	const FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
	OutSelection.RaceId = CharacterState.RaceId;
	OutSelection.Gender = CharacterState.PortraitGender;
	OutSelection.PortraitVariantId = CharacterState.PortraitVariantId;
	OutSelection.Portrait = FRPGAuthoringIdentityResolver::ResolvePortraitVisual(
		CharacterState.RaceId, CharacterState.PortraitGender, CharacterState.PortraitVariantId);
	OutSelection.ClassId = CharacterState.ClassId;
	OutSelection.ClassIcon = FRPGAuthoringIdentityResolver::ResolveClassIcon(CharacterState.ClassId);
	return true;
}

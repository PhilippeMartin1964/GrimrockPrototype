#include "Runtime/GridPartyInventoryComponent.h"

#include "Engine/Texture2D.h"

bool UGridPartyInventoryComponent::SetCharacterVisualSelection (
    int32 CharacterIndex,
    ERPGCharacterPortraitGender PortraitGender,
    FName PortraitVariantId,
    TSoftObjectPtr<UTexture2D> Portrait,
    TSoftObjectPtr<UTexture2D> ClassIcon)
{
    if (!IsValidCharacterIndex (CharacterIndex))
    {
        return false;
    }

    FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    CharacterState.PortraitGender = PortraitGender;
    CharacterState.PortraitVariantId = PortraitVariantId;
    CharacterState.Portrait = Portrait;
    CharacterState.ClassIcon = ClassIcon;
    NotifyPartyInventoryChanged (CharacterIndex);
    return true;
}

bool UGridPartyInventoryComponent::GetCharacterVisualSelection (
    int32 CharacterIndex,
    FRPGCharacterVisualSelection& OutSelection) const
{
    OutSelection = FRPGCharacterVisualSelection ();
    if (!IsValidCharacterIndex (CharacterIndex))
    {
        return false;
    }

    const FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    OutSelection.RaceId = CharacterState.RaceId;
    OutSelection.Gender = CharacterState.PortraitGender;
    OutSelection.PortraitVariantId = CharacterState.PortraitVariantId;
    OutSelection.Portrait = CharacterState.Portrait;
    OutSelection.ClassId = CharacterState.ClassId;
    OutSelection.ClassIcon = CharacterState.ClassIcon;
    return true;
}

#include "Runtime/GridPartyInventoryComponent.h"

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
    return true;
}

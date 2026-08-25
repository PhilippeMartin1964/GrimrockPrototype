#include "Runtime/GrimrockPartyPawn.h"

#include "Runtime/GridPartyInventoryComponent.h"

void AGrimrockPartyPawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (!PartyInventoryComponent)
	{
		return;
	}

	PartyInventoryComponent->OnPartyInventoryChanged.RemoveDynamic(this, &AGrimrockPartyPawn::HandlePartyInventoryChanged);
	PartyInventoryComponent->OnPartyInventoryChanged.AddDynamic(this, &AGrimrockPartyPawn::HandlePartyInventoryChanged);
}

void AGrimrockPartyPawn::HandlePartyInventoryChanged(int32 CharacterIndex)
{
	if (!PartyInventoryComponent)
	{
		ClearHeldItem();
		return;
	}

	const int32 SelectedCharacterIndex = PartyInventoryComponent->GetSelectedCharacterIndex();
	if (CharacterIndex != INDEX_NONE && CharacterIndex != SelectedCharacterIndex)
	{
		return;
	}

	SyncHeldVisualFromSelectedCharacterEquipment();
}

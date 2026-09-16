#include "Runtime/GrimrockPartyPawn.h"

#include "Runtime/GridPartyIlluminationComponent.h"
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
		SyncHeldVisualFromSelectedCharacterEquipment();
		return;
	}

	const int32 SelectedCharacterIndex = PartyInventoryComponent->GetSelectedCharacterIndex();
	if (CharacterIndex != INDEX_NONE && CharacterIndex != SelectedCharacterIndex)
	{
		// Party illumination is party-wide, so another character can still change
		// the light. The selected first-person held visual must not be resynced.
		if (UGridPartyIlluminationComponent* PartyIllumination = FindComponentByClass<UGridPartyIlluminationComponent>())
		{
			PartyIllumination->RefreshFromEquipment(PartyInventoryComponent, LevelRuntimeActor);
		}
		return;
	}

	SyncHeldVisualFromSelectedCharacterEquipment();
}

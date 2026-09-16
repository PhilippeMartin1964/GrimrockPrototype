#include "Runtime/GrimrockPartyPawn.h"

#include "Camera/CameraComponent.h"
#include "Runtime/GridPartyIlluminationComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"

void AGrimrockPartyPawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// A Blueprint-authored component is preferred because its transform and
	// ergonomic multipliers are directly configurable on BP_GrimrockPartyPawn.
	// Bare C++ pawns still receive a functional camera-attached default.
	if (!FindComponentByClass<UGridPartyIlluminationComponent>())
	{
		UGridPartyIlluminationComponent* PartyIllumination = NewObject<UGridPartyIlluminationComponent>(this, TEXT("PartyIlluminationRuntime"));
		if (PartyIllumination)
		{
			USceneComponent* AttachParent = Camera ? Cast<USceneComponent>(Camera) : GetRootComponent();
			PartyIllumination->SetupAttachment(AttachParent);
			PartyIllumination->SetRelativeLocation(FVector(25.0f, 0.0f, 0.0f));
			PartyIllumination->SetRelativeRotation(FRotator::ZeroRotator);
			AddInstanceComponent(PartyIllumination);
			PartyIllumination->RegisterComponent();
		}
	}

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

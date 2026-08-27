#include "Runtime/GrimrockPartyPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Core/GridDirectionUtils.h"
#include "InputCoreTypes.h"
#include "Magic/GridPartySpellbookComponent.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPIEPlaytestRequest.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockGameInstance.h"
#include "Runtime/GrimrockPlayerController.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridThrownItemActor.h"
#include "Save/GridCombatSavePolicy.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UI/GridCombatHudWidget.h"
#include "UI/GridInventoryWidget.h"
#include "UI/GrimrockMenuWidget.h"
#include "UI/RPGCharacterCreationWidget.h"

namespace
{
	bool IsPawnHandEquipmentSlot(EGridEquipmentSlot Slot)
	{
		return Slot == EGridEquipmentSlot::MainHand || Slot == EGridEquipmentSlot::OffHand;
	}

	const TCHAR* GetPawnEquipmentSlotName(EGridEquipmentSlot Slot)
	{
		switch (Slot)
		{
			case EGridEquipmentSlot::None:
				return TEXT("None");
			case EGridEquipmentSlot::MainHand:
				return TEXT("MainHand");
			case EGridEquipmentSlot::OffHand:
				return TEXT("OffHand");
			case EGridEquipmentSlot::Head:
				return TEXT("Head");
			case EGridEquipmentSlot::Chest:
				return TEXT("Chest");
			case EGridEquipmentSlot::Legs:
				return TEXT("Legs");
			case EGridEquipmentSlot::Feet:
				return TEXT("Feet");
			case EGridEquipmentSlot::Amulet:
				return TEXT("Amulet");
			case EGridEquipmentSlot::Ring1:
				return TEXT("Ring1");
			case EGridEquipmentSlot::Ring2:
				return TEXT("Ring2");
			case EGridEquipmentSlot::Shoulders:
				return TEXT("Shoulders");
			case EGridEquipmentSlot::Gloves:
				return TEXT("Gloves");
			case EGridEquipmentSlot::Belt:
				return TEXT("Belt");
			case EGridEquipmentSlot::Cloak:
				return TEXT("Cloak");
			case EGridEquipmentSlot::Talisman:
				return TEXT("Talisman");
			case EGridEquipmentSlot::QuickSlot1:
				return TEXT("QuickSlot1");
			case EGridEquipmentSlot::QuickSlot2:
				return TEXT("QuickSlot2");
			case EGridEquipmentSlot::Face:
				return TEXT("Visage");
			case EGridEquipmentSlot::Shirt:
				return TEXT("Chemise");
			case EGridEquipmentSlot::Bracers:
				return TEXT("Brassards");
			case EGridEquipmentSlot::Earring1:
				return TEXT("Bijou d'oreille I");
			case EGridEquipmentSlot::Earring2:
				return TEXT("Bijou d'oreille II");
			default:
				return TEXT("Unsupported");
		}
	}
}

AGrimrockPartyPawn::AGrimrockPartyPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(SceneRoot);
	SpringArm->TargetArmLength = 0.f;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = false;
	SpringArm->SetRelativeLocation(FVector::ZeroVector);
	SpringArm->SetRelativeRotation(FRotator::ZeroRotator);
	SpringArmBaseRelativeLocation = SpringArm->GetRelativeLocation();

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	Camera->bUsePawnControlRotation = false;

	HeldItemRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HeldItemRoot"));
	HeldItemRoot->SetupAttachment(Camera ? Cast<USceneComponent>(Camera) : SceneRoot);

	PartyInventoryComponent = CreateDefaultSubobject<UGridPartyInventoryComponent>(TEXT("PartyInventoryComponent"));
	UGridPartySpellbookComponent* PartySpellbookComponent = CreateDefaultSubobject<UGridPartySpellbookComponent>(TEXT("PartySpellbookComponent"));
	PartySpellbookComponent->InitializeSpellbookComponent(PartyInventoryComponent);

	AutoPossessPlayer = EAutoReceiveInput::Player0;
	Camera->SetRelativeLocation(CameraLocalOffset);
}

void AGrimrockPartyPawn::BeginPlay()
{
	Super::BeginPlay();

	const bool bFreshPIERequestForWorld = GridPIEPlaytestRequest::IsActiveForWorld(GetWorld());
	if (bFreshPIERequestForWorld)
	{
		LevelRuntimeActor = GridPIEPlaytestRequest::ResolveMatchingRuntimeActor(GetWorld());
	}
	else if (!LevelRuntimeActor)
	{
		LevelRuntimeActor = Cast<AGridLevelRuntimeActor>(UGameplayStatics::GetActorOfClass(GetWorld(), AGridLevelRuntimeActor::StaticClass()));
	}

	if (!LevelRuntimeActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("GrimrockPartyPawn: no AGridLevelRuntimeActor found."));
	}
	else if (LevelRuntimeActor->bApplyLevelStartOnBeginPlay && LevelRuntimeActor->LevelAsset)
	{
		if (LevelRuntimeActor->LevelAsset->IsStartCellValid())
		{
			CurrentCellX = LevelRuntimeActor->LevelAsset->StartCellX;
			CurrentCellY = LevelRuntimeActor->LevelAsset->StartCellY;
			Facing = LevelRuntimeActor->LevelAsset->StartFacing == EGridEdge::None ? EGridEdge::North : LevelRuntimeActor->LevelAsset->StartFacing;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("GrimrockPartyPawn: LevelAsset start cell is invalid, keeping configured pawn cell (%d,%d)."), CurrentCellX,
				CurrentCellY);
		}
	}
	else if (!LevelRuntimeActor->bApplyLevelStartOnBeginPlay)
	{
		UE_LOG(LogTemp, Log, TEXT("GrimrockPartyPawn: LevelAsset start application is disabled, keeping configured pawn start."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GrimrockPartyPawn: LevelRuntimeActor has no LevelAsset, keeping configured pawn start."));
	}

	const bool bFreshDungeonPlaytest = GridPIEPlaytestRequest::Matches(LevelRuntimeActor);
	bool bLoadedSavedGame = false;
	if (PartyInventoryComponent)
	{
		if (bFreshDungeonPlaytest)
		{
			if (HasCurrentSave())
			{
				FText LoadError;
				bLoadedSavedGame = LoadCurrentGameData(LoadError, false);
				if (!bLoadedSavedGame)
				{
					UE_LOG(LogTemp, Warning, TEXT("PartySave PlaytestProfileLoad Failed Slot=%s Reason=%s"), *PartySaveSlotName, *LoadError.ToString());
					PartyInventoryComponent->ResetPartyForNewGame();
				}
			}
			else
			{
				PartyInventoryComponent->ResetPartyForNewGame();
			}
		}
		else if (PartyStartupMode == EGrimrockPartyStartupMode::NewGame)
		{
			if (HasCurrentSave())
			{
				UGameplayStatics::DeleteGameInSlot(PartySaveSlotName, PartySaveUserIndex);
			}
			PartyInventoryComponent->ResetPartyForNewGame();
		}
		else if (HasCurrentSave())
		{
			FText LoadError;
			bLoadedSavedGame = LoadCurrentGameData(LoadError, true);
			if (!bLoadedSavedGame)
			{
				const FString FailedLoadSlotName = PartySaveSlotName;
				UE_LOG(LogTemp, Warning, TEXT("PartySave Load Failed Slot=%s Reason=%s"), *FailedLoadSlotName, *LoadError.ToString());

				// A failed Continue is not a New Game. Preserve the save and the
				// rolled-back runtime party, disarm EndPlay autosave for this
				// dying pawn, and return to the main menu without ever opening
				// Character Creation.
				PartySaveSlotName.Empty();
				if (UGrimrockGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance<UGrimrockGameInstance>() : nullptr)
				{
					UE_LOG(LogTemp, Error, TEXT("PartySave ContinueAborted Slot=%s Reason=%s Action=ReturnToMainMenu"), *FailedLoadSlotName,
						*LoadError.ToString());
					GameInstance->RequestReturnToMainMenu(this);
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("PartySave ContinueAborted Slot=%s Reason=%s Action=None Error=NoGrimrockGameInstance"), *FailedLoadSlotName,
						*LoadError.ToString());
				}
				return;
			}
		}
		else
		{
			PartyInventoryComponent->ResetPartyForNewGame();
		}
	}

	SnapToCurrentCell();

	if (LevelRuntimeActor)
	{
		LevelRuntimeActor->HandlePartyCellChanged(CurrentCellX, CurrentCellY, CurrentCellX, CurrentCellY);
	}

	SyncHeldVisualFromSelectedCharacterEquipment();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (DefaultMappingContext)
				{
					Subsystem->AddMappingContext(DefaultMappingContext, 0);
				}
			}
		}
	}
	ApplyCameraLocalViewOffset();
	ShowCombatActionPanelWidget();

	if (bLoadedSavedGame && bFreshDungeonPlaytest)
	{
		UE_LOG(LogTemp, Log, TEXT("PartySave PlaytestProfileLoaded Slot=%s CharacterCount=%d DungeonState=Fresh"), *PartySaveSlotName,
			PartyInventoryComponent ? PartyInventoryComponent->GetActiveCharacterCount() : 0);
	}
	else if (bLoadedSavedGame)
	{
		UE_LOG(LogTemp, Log, TEXT("PartySave Continued Slot=%s CharacterCount=%d"), *PartySaveSlotName,
			PartyInventoryComponent ? PartyInventoryComponent->GetActiveCharacterCount() : 0);
	}

	if (PartyInventoryComponent && !PartyInventoryComponent->HasCompletedInitialCharacterCreation())
	{
		ShowInitialCharacterCreationWidget();
	}
}

void AGrimrockPartyPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HideCombatActionPanelWidget();

	const bool bFreshDungeonPlaytest = GridPIEPlaytestRequest::Matches(LevelRuntimeActor);
	if (bFreshDungeonPlaytest)
	{
		UE_LOG(LogTemp, Log, TEXT("PartySave PlaytestAutoSaveSkipped Slot=%s Reason=FreshPIERequest"), *PartySaveSlotName);
	}
	else if (!PartySaveSlotName.IsEmpty() && PartyInventoryComponent && PartyInventoryComponent->HasCompletedInitialCharacterCreation())
	{
		FText SaveError;
		if (!SaveCurrentGame(SaveError))
		{
			UE_LOG(LogTemp, Warning, TEXT("PartySave EndPlay Failed Slot=%s Reason=%s"), *PartySaveSlotName, *SaveError.ToString());
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AGrimrockPartyPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsMoving)
	{
		UpdateMove(DeltaSeconds);
	}

	if (bIsTurning)
	{
		UpdateTurn(DeltaSeconds);
	}

	UpdateHeadBob(DeltaSeconds);
	UpdateFreeLook(DeltaSeconds);

	if (BufferedCommandType != EBufferedCommandType::None)
	{
		BufferedCommandAge += DeltaSeconds;

		if (BufferedCommandAge > InputBufferMaxAge)
		{
			ClearBufferedCommand();
		}
	}

	if (!IsBusy())
	{
		TryConsumeBufferedCommand();
	}
}

void AGrimrockPartyPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!PlayerInputComponent)
	{
		return;
	}

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveForwardAction)
		{
			EIC->BindAction(MoveForwardAction, ETriggerEvent::Started, this, &AGrimrockPartyPawn::HandleMoveForward);
		}

		if (MoveBackwardAction)
		{
			EIC->BindAction(MoveBackwardAction, ETriggerEvent::Started, this, &AGrimrockPartyPawn::HandleMoveBackward);
		}

		if (TurnLeftAction)
		{
			EIC->BindAction(TurnLeftAction, ETriggerEvent::Started, this, &AGrimrockPartyPawn::HandleTurnLeft);
		}

		if (TurnRightAction)
		{
			EIC->BindAction(TurnRightAction, ETriggerEvent::Started, this, &AGrimrockPartyPawn::HandleTurnRight);
		}
		if (StrafeLeftAction)
		{
			EIC->BindAction(StrafeLeftAction, ETriggerEvent::Started, this, &AGrimrockPartyPawn::HandleStrafeLeft);
		}

		if (StrafeRightAction)
		{
			EIC->BindAction(StrafeRightAction, ETriggerEvent::Started, this, &AGrimrockPartyPawn::HandleStrafeRight);
		}

		if (bEnableLegacyKeyboardUseAction && UseAction)
		{
			EIC->BindAction(UseAction, ETriggerEvent::Started, this, &AGrimrockPartyPawn::HandleUse);
		}
	}
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AGrimrockPartyPawn::BeginFreeLook);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AGrimrockPartyPawn::EndFreeLook);
	PlayerInputComponent->BindKey(EKeys::I, IE_Pressed, this, &AGrimrockPartyPawn::ToggleInventoryWidget);

	const auto ConfigureHotbarBinding = [](FInputKeyBinding& Binding)
	{
		Binding.bConsumeInput = true;
		Binding.bExecuteWhenPaused = false;
	};
	ConfigureHotbarBinding(PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &AGrimrockPartyPawn::HandleCombatHotbarSlotOne));
	ConfigureHotbarBinding(PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AGrimrockPartyPawn::HandleCombatHotbarSlotTwo));
	ConfigureHotbarBinding(PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AGrimrockPartyPawn::HandleCombatHotbarSlotThree));
	ConfigureHotbarBinding(PlayerInputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AGrimrockPartyPawn::HandleCombatHotbarSlotFour));
	ConfigureHotbarBinding(PlayerInputComponent->BindKey(EKeys::Five, IE_Pressed, this, &AGrimrockPartyPawn::HandleCombatHotbarSlotFive));
	ConfigureHotbarBinding(PlayerInputComponent->BindKey(EKeys::Six, IE_Pressed, this, &AGrimrockPartyPawn::HandleCombatHotbarSlotSix));
	ConfigureHotbarBinding(PlayerInputComponent->BindKey(EKeys::Seven, IE_Pressed, this, &AGrimrockPartyPawn::HandleCombatHotbarSlotSeven));
	ConfigureHotbarBinding(PlayerInputComponent->BindKey(EKeys::Eight, IE_Pressed, this, &AGrimrockPartyPawn::HandleCombatHotbarSlotEight));
	ConfigureHotbarBinding(PlayerInputComponent->BindKey(EKeys::Nine, IE_Pressed, this, &AGrimrockPartyPawn::HandleCombatHotbarSlotNine));
	ConfigureHotbarBinding(PlayerInputComponent->BindKey(EKeys::Zero, IE_Pressed, this, &AGrimrockPartyPawn::HandleCombatHotbarSlotZero));
}

void AGrimrockPartyPawn::HandleCombatHotbarSlotOne()
{
	TryExecuteCombatHotbarSlot(0);
}

void AGrimrockPartyPawn::HandleCombatHotbarSlotTwo()
{
	TryExecuteCombatHotbarSlot(1);
}

void AGrimrockPartyPawn::HandleCombatHotbarSlotThree()
{
	TryExecuteCombatHotbarSlot(2);
}

void AGrimrockPartyPawn::HandleCombatHotbarSlotFour()
{
	TryExecuteCombatHotbarSlot(3);
}

void AGrimrockPartyPawn::HandleCombatHotbarSlotFive()
{
	TryExecuteCombatHotbarSlot(4);
}

void AGrimrockPartyPawn::HandleCombatHotbarSlotSix()
{
	TryExecuteCombatHotbarSlot(5);
}

void AGrimrockPartyPawn::HandleCombatHotbarSlotSeven()
{
	TryExecuteCombatHotbarSlot(6);
}

void AGrimrockPartyPawn::HandleCombatHotbarSlotEight()
{
	TryExecuteCombatHotbarSlot(7);
}

void AGrimrockPartyPawn::HandleCombatHotbarSlotNine()
{
	TryExecuteCombatHotbarSlot(8);
}

void AGrimrockPartyPawn::HandleCombatHotbarSlotZero()
{
	TryExecuteCombatHotbarSlot(9);
}

void AGrimrockPartyPawn::ApplyCameraLocalViewOffset()
{
	if (!Camera)
	{
		return;
	}
	Camera->SetRelativeLocation(CameraLocalOffset);
	Camera->SetRelativeRotation(CameraLocalRotationOffset);
}

void AGrimrockPartyPawn::HandleUse(const FInputActionValue& Value)
{
	(void)Value;

	if (bCharacterCreationModalActive)
	{
		return;
	}

	if (IsBusy())
	{
		BufferUseCommand();
		return;
	}

	TryUseFrontInteraction();
}

bool AGrimrockPartyPawn::TryUseFrontInteraction()
{
	if (bCharacterCreationModalActive || bIsMoving || bIsTurning || !HasLevelRuntimeActor())
	{
		return false;
	}

	if (LevelRuntimeActor->TryPickupItemAtCell(CurrentCellX, CurrentCellY, this))
	{
		return true;
	}

	const EGridEdge FrontEdge = GridDirectionUtils::GetForward(Facing);

	if (TryInteractOnLevel(CurrentCellX, CurrentCellY, FrontEdge))
	{
		return true;
	}

	return TryToggleDoorOnLevel(CurrentCellX, CurrentCellY, FrontEdge);
}

bool AGrimrockPartyPawn::HasLevelRuntimeActor() const
{
	return LevelRuntimeActor != nullptr;
}

bool AGrimrockPartyPawn::CanMoveOnLevel(int32 FromX, int32 FromY, EGridEdge Direction) const
{
	return LevelRuntimeActor && LevelRuntimeActor->CanMove(FromX, FromY, Direction);
}

bool AGrimrockPartyPawn::TryGetNeighborOnLevel(int32 X, int32 Y, EGridEdge Direction, int32& OutX, int32& OutY) const
{
	if (!LevelRuntimeActor)
	{
		OutX = X;
		OutY = Y;
		return false;
	}

	return LevelRuntimeActor->TryGetNeighborCell(X, Y, Direction, OutX, OutY);
}

FVector AGrimrockPartyPawn::GetCellCenterOnLevel(int32 X, int32 Y, float ZOffset) const
{
	if (!LevelRuntimeActor)
	{
		return GetActorLocation();
	}

	return LevelRuntimeActor->GetCellCenterWorld(X, Y, ZOffset);
}

bool AGrimrockPartyPawn::TryToggleDoorOnLevel(int32 X, int32 Y, EGridEdge Edge)
{
	return LevelRuntimeActor && LevelRuntimeActor->ToggleDoorOnEdge(X, Y, Edge);
}

bool AGrimrockPartyPawn::DismissReadableMessageIfVisible()
{
	return LevelRuntimeActor && LevelRuntimeActor->DismissReadableMessage();
}

UGridTurnManagerComponent* AGrimrockPartyPawn::FindTurnManager() const
{
	return IsValid(LevelRuntimeActor) ? LevelRuntimeActor->FindComponentByClass<UGridTurnManagerComponent>() : nullptr;
}

void AGrimrockPartyPawn::UpdateHeadBob(float DeltaSeconds)
{
	if (!bEnableHeadBob)
	{
		TargetHeadBobOffset = FVector::ZeroVector;
		CurrentHeadBobOffset = FMath::VInterpTo(CurrentHeadBobOffset, FVector::ZeroVector, DeltaSeconds, HeadBobReturnSpeed);

		ApplyCameraOffsets();
		return;
	}

	if (bIsMoving)
	{
		const float SafeDuration = FMath::Max(0.01f, MoveDuration);
		HeadBobAlpha = FMath::Clamp(MoveElapsed / SafeDuration, 0.f, 1.f);

		// Courbe simple type Grimrock : un seul "pas" par déplacement de case.
		const float VerticalCurve = FMath::Sin(HeadBobAlpha * PI);
		const float VerticalOffset = -VerticalCurve * HeadBobVerticalAmplitude;

		float HorizontalOffset = 0.f;

		if (bHeadBobStrafeSway)
		{
			const EGridEdge LeftDir = GridDirectionUtils::GetLeft(Facing);
			const EGridEdge RightDir = GridDirectionUtils::GetRight(Facing);

			if (ActiveMoveDirection == RightDir)
			{
				HorizontalOffset = VerticalCurve * HeadBobHorizontalAmplitude;
			}
			else if (ActiveMoveDirection == LeftDir)
			{
				HorizontalOffset = -VerticalCurve * HeadBobHorizontalAmplitude;
			}
		}

		TargetHeadBobOffset = FVector(0.f, HorizontalOffset, VerticalOffset);
	}
	else
	{
		HeadBobAlpha = 0.f;
		TargetHeadBobOffset = FVector::ZeroVector;
	}

	CurrentHeadBobOffset = FMath::VInterpTo(CurrentHeadBobOffset, TargetHeadBobOffset, DeltaSeconds, bIsMoving ? 18.f : HeadBobReturnSpeed);

	ApplyCameraOffsets();
}

void AGrimrockPartyPawn::ApplyCameraOffsets()
{
	if (!SpringArm)
	{
		return;
	}

	SpringArm->SetRelativeLocation(SpringArmBaseRelativeLocation + CurrentHeadBobOffset);
}

void AGrimrockPartyPawn::BeginFreeLook()
{
	bIsFreeLooking = true;
}

void AGrimrockPartyPawn::EndFreeLook()
{
	bIsFreeLooking = false;
}

void AGrimrockPartyPawn::UpdateFreeLook(float DeltaSeconds)
{
	if (!SpringArm)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (bIsFreeLooking)
		{
			float MouseDeltaX = 0.f;
			float MouseDeltaY = 0.f;
			PC->GetInputMouseDelta(MouseDeltaX, MouseDeltaY);

			FreeLookYaw += MouseDeltaX * FreeLookSensitivityYaw;
			FreeLookPitch = FMath::Clamp(FreeLookPitch + (MouseDeltaY * FreeLookSensitivityPitch), -FreeLookPitchDownLimit, FreeLookPitchUpLimit);

			FreeLookYaw = FMath::Clamp(FreeLookYaw, -FreeLookYawLimit, FreeLookYawLimit);
		}
		else if (bEnableFreeLookRecentering)
		{
			FreeLookYaw = FMath::FInterpTo(FreeLookYaw, 0.f, DeltaSeconds, FreeLookRecenteringSpeed);
			FreeLookPitch = FMath::FInterpTo(FreeLookPitch, 0.f, DeltaSeconds, FreeLookRecenteringSpeed);

			if (FMath::Abs(FreeLookYaw) < 0.01f)
			{
				FreeLookYaw = 0.f;
			}

			if (FMath::Abs(FreeLookPitch) < 0.01f)
			{
				FreeLookPitch = 0.f;
			}
		}
	}

	ApplyFreeLookRotation();
}

void AGrimrockPartyPawn::ApplyFreeLookRotation()
{
	if (!SpringArm)
	{
		return;
	}

	SpringArm->SetRelativeRotation(FRotator(FreeLookPitch, FreeLookYaw, 0.f));
}

bool AGrimrockPartyPawn::TryInteractOnLevel(int32 X, int32 Y, EGridEdge Edge)
{
	return LevelRuntimeActor && LevelRuntimeActor->TryInteractAtEdge(X, Y, Edge, this);
}

bool AGrimrockPartyPawn::HasInventoryItem(FName ItemDefinitionId) const
{
	return PartyInventoryComponent && PartyInventoryComponent->HasItemDefinitionInSelectedCharacterInventory(ItemDefinitionId);
}

bool AGrimrockPartyPawn::CanAddItemInstanceToSelectedCharacterInventory(const FGridItemInstance& ItemInstance) const
{
	if (!PartyInventoryComponent)
	{
		return false;
	}

	FGridItemInstance InventoryItem = ItemInstance;
	PartyInventoryComponent->ApplyItemDefinitionToInstance(InventoryItem);
	return PartyInventoryComponent->CanAddItemToSelectedCharacterInventory(InventoryItem);
}

bool AGrimrockPartyPawn::AddItemInstanceToSelectedCharacterInventory(const FGridItemInstance& ItemInstance)
{
	if (!PartyInventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Pickup Failed NoPartyInventoryComponent Item=%s RuntimeId=%s"), *ItemInstance.ItemDefinitionId.ToString(),
			*ItemInstance.RuntimeObjectId.ToString());
		return false;
	}

	PartyInventoryComponent->InitializeDefaultPartyIfNeeded();

	if (LevelRuntimeActor && !ItemInstance.ItemDefinitionId.IsNone())
	{
		if (UGridItemDefinitionAsset* RuntimeDefinition = LevelRuntimeActor->ResolveRuntimeItemDefinition(ItemInstance.ItemDefinitionId))
		{
			PartyInventoryComponent->RegisterItemDefinition(RuntimeDefinition);
		}
	}

	FGridItemInstance InventoryItem = ItemInstance;
	PartyInventoryComponent->ApplyItemDefinitionToInstance(InventoryItem);
	InventoryItem.OwnerType = EGridItemOwnerType::CharacterInventory;
	InventoryItem.OwnerCharacterIndex = PartyInventoryComponent->GetSelectedCharacterIndex();
	InventoryItem.EquipmentSlot = EGridEquipmentSlot::None;

	const bool bAdded = PartyInventoryComponent->AddItemToSelectedCharacterInventory(InventoryItem);
	UE_LOG(LogTemp, Log, TEXT("GridInventory Pickup AddedToSelectedCharacter Item=%s RuntimeId=%s CharacterIndex=%d Result=%s"),
		*InventoryItem.ItemDefinitionId.ToString(), *InventoryItem.RuntimeObjectId.ToString(), PartyInventoryComponent->GetSelectedCharacterIndex(),
		bAdded ? TEXT("true") : TEXT("false"));

	if (!bAdded)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory Pickup Failed InventoryFull Item=%s RuntimeId=%s"), *InventoryItem.ItemDefinitionId.ToString(),
			*InventoryItem.RuntimeObjectId.ToString());
		return false;
	}

	return true;
}

bool AGrimrockPartyPawn::AddRuntimeItemToSelectedCharacterInventory(
	const FGuid& RuntimeObjectId, FName ItemDefinitionId, float Weight, int32 Quantity, bool bLightsEnabled)
{
	FGridItemInstance ItemInstance;
	ItemInstance.RuntimeObjectId = RuntimeObjectId;
	ItemInstance.ItemDefinitionId = ItemDefinitionId;
	ItemInstance.Quantity = Quantity;
	ItemInstance.Weight = Weight;
	ItemInstance.bLightsEnabled = bLightsEnabled;
	return AddItemInstanceToSelectedCharacterInventory(ItemInstance);
}

void AGrimrockPartyPawn::LogPartyInventoryDiagnostics() const
{
	if (PartyInventoryComponent)
	{
		PartyInventoryComponent->LogPartyInventoryDiagnostics();
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("PartyInventoryComponent is null on %s"), *GetName());
}

void AGrimrockPartyPawn::LogItemDefinitionDiagnostics() const
{
	if (PartyInventoryComponent)
	{
		PartyInventoryComponent->LogItemDefinitionDiagnostics();
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("PartyInventoryComponent is null on %s"), *GetName());
}

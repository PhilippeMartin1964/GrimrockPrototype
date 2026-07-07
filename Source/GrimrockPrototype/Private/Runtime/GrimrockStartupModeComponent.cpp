#include "Runtime/GrimrockStartupModeComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockGameInstance.h"
#include "Runtime/GrimrockPartyPawn.h"

UGrimrockStartupModeComponent::UGrimrockStartupModeComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGrimrockStartupModeComponent::BeginPlay()
{
    Super::BeginPlay();

    AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn>(GetOwner());
    if (!PartyPawn)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("GrimrockStartupMode Apply Failed Owner=%s Reason=OwnerIsNotGrimrockPartyPawn"),
            *GetNameSafe(GetOwner()));
        return;
    }

    CachedPartyPawn = PartyPawn;

    UGrimrockGameInstance* GrimrockGameInstance = GetWorld()
        ? GetWorld()->GetGameInstance<UGrimrockGameInstance>()
        : nullptr;

    if (!GrimrockGameInstance)
    {
        UE_LOG(
            LogTemp,
            Verbose,
            TEXT("GrimrockStartupMode Apply Skipped Pawn=%s Reason=NoGrimrockGameInstance"),
            *GetNameSafe(PartyPawn));
        return;
    }

    PartyPawn->PartyStartupMode = GrimrockGameInstance->ConsumePendingStartupMode();

    FString PendingLoadSlotName;
    int32 PendingLoadSlotUserIndex = 0;
    if (GrimrockGameInstance->ConsumePendingLoadSlot(PendingLoadSlotName, PendingLoadSlotUserIndex))
    {
        PartyPawn->PartySaveSlotName = PendingLoadSlotName;
        PartyPawn->PartySaveUserIndex = PendingLoadSlotUserIndex;

        UE_LOG(
            LogTemp,
            Log,
            TEXT("GrimrockStartupMode AppliedSaveSlot Pawn=%s Slot=%s UserIndex=%d"),
            *GetNameSafe(PartyPawn),
            *PartyPawn->PartySaveSlotName,
            PartyPawn->PartySaveUserIndex);
    }

    if (PartyPawn->PartyStartupMode == EGrimrockPartyStartupMode::NewGame)
    {
        DeferNewGameRuntimeActivation(PartyPawn);
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("GrimrockStartupMode Applied Pawn=%s Mode=%d Slot=%s UserIndex=%d"),
        *GetNameSafe(PartyPawn),
        static_cast<int32>(PartyPawn->PartyStartupMode),
        *PartyPawn->PartySaveSlotName,
        PartyPawn->PartySaveUserIndex);
}

void UGrimrockStartupModeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    TryActivateDeferredNewGameRuntime();
}

void UGrimrockStartupModeComponent::DeferNewGameRuntimeActivation(AGrimrockPartyPawn* PartyPawn)
{
    if (!PartyPawn)
    {
        return;
    }

    AGridLevelRuntimeActor* RuntimeActor = PartyPawn->LevelRuntimeActor.Get();
    if (!RuntimeActor)
    {
        RuntimeActor = Cast<AGridLevelRuntimeActor>(
            UGameplayStatics::GetActorOfClass(GetWorld(), AGridLevelRuntimeActor::StaticClass()));
    }

    if (RuntimeActor)
    {
        PartyPawn->LevelRuntimeActor = RuntimeActor;
        RuntimeActor->DungeonRuntimeState = FGridDungeonRuntimeState();
        RuntimeActor->ClearVisuals(EGridRuntimeRebuildMode::Full);

        UE_LOG(
            LogTemp,
            Log,
            TEXT("GrimrockStartupMode DeferredRuntimeActivation Pawn=%s Runtime=%s Reason=InitialCharacterCreationPending"),
            *GetNameSafe(PartyPawn),
            *GetNameSafe(RuntimeActor));
    }
    else
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("GrimrockStartupMode DeferredRuntimeActivation MissingRuntimeActor Pawn=%s"),
            *GetNameSafe(PartyPawn));
    }

    DeferredRuntimeActor = RuntimeActor;
    bWaitingForInitialCharacterCreation = true;
    SetComponentTickEnabled(true);
}

void UGrimrockStartupModeComponent::TryActivateDeferredNewGameRuntime()
{
    if (!bWaitingForInitialCharacterCreation)
    {
        return;
    }

    AGrimrockPartyPawn* PartyPawn = CachedPartyPawn.Get();
    if (!PartyPawn)
    {
        PartyPawn = Cast<AGrimrockPartyPawn>(GetOwner());
        CachedPartyPawn = PartyPawn;
    }

    if (!PartyPawn || !PartyPawn->PartyInventoryComponent)
    {
        return;
    }

    if (!PartyPawn->PartyInventoryComponent->HasCompletedInitialCharacterCreation())
    {
        return;
    }

    AGridLevelRuntimeActor* RuntimeActor = DeferredRuntimeActor.Get();
    if (!RuntimeActor)
    {
        RuntimeActor = PartyPawn->LevelRuntimeActor.Get();
    }
    if (!RuntimeActor)
    {
        RuntimeActor = Cast<AGridLevelRuntimeActor>(
            UGameplayStatics::GetActorOfClass(GetWorld(), AGridLevelRuntimeActor::StaticClass()));
    }

    if (!RuntimeActor)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("GrimrockStartupMode ActivateDeferredRuntime Failed Pawn=%s Reason=NoRuntimeActor"),
            *GetNameSafe(PartyPawn));
        return;
    }

    PartyPawn->LevelRuntimeActor = RuntimeActor;
    RuntimeActor->DungeonRuntimeState = FGridDungeonRuntimeState();
    RuntimeActor->RebuildLevel(EGridRuntimeRebuildMode::Full);
    PartyPawn->SnapToCurrentCell();
    RuntimeActor->HandlePartyCellChanged(
        PartyPawn->CurrentCellX,
        PartyPawn->CurrentCellY,
        PartyPawn->CurrentCellX,
        PartyPawn->CurrentCellY);

    bWaitingForInitialCharacterCreation = false;
    DeferredRuntimeActor = nullptr;
    SetComponentTickEnabled(false);

    UE_LOG(
        LogTemp,
        Log,
        TEXT("GrimrockStartupMode ActivatedDeferredRuntime Pawn=%s Runtime=%s"),
        *GetNameSafe(PartyPawn),
        *GetNameSafe(RuntimeActor));
}

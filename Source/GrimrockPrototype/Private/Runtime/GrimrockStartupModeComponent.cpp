#include "Runtime/GrimrockStartupModeComponent.h"

#include "Runtime/GrimrockGameInstance.h"
#include "Runtime/GrimrockPartyPawn.h"

UGrimrockStartupModeComponent::UGrimrockStartupModeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
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

    UE_LOG(
        LogTemp,
        Log,
        TEXT("GrimrockStartupMode Applied Pawn=%s Mode=%d Slot=%s UserIndex=%d"),
        *GetNameSafe(PartyPawn),
        static_cast<int32>(PartyPawn->PartyStartupMode),
        *PartyPawn->PartySaveSlotName,
        PartyPawn->PartySaveUserIndex);
}
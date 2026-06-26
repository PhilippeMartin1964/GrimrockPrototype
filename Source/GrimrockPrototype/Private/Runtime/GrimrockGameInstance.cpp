#include "Runtime/GrimrockGameInstance.h"

#include "Kismet/GameplayStatics.h"

void UGrimrockGameInstance::SetPendingStartupMode(EGrimrockPartyStartupMode NewMode)
{
    PendingStartupMode = NewMode;

    UE_LOG(
        LogTemp,
        Log,
        TEXT("GrimrockGameInstance PendingStartupMode Set Mode=%d"),
        static_cast<int32>(PendingStartupMode));
}

EGrimrockPartyStartupMode UGrimrockGameInstance::GetPendingStartupMode() const
{
    return PendingStartupMode;
}

EGrimrockPartyStartupMode UGrimrockGameInstance::ConsumePendingStartupMode()
{
    const EGrimrockPartyStartupMode ConsumedMode = PendingStartupMode;
    PendingStartupMode = EGrimrockPartyStartupMode::Continue;

    UE_LOG(
        LogTemp,
        Log,
        TEXT("GrimrockGameInstance PendingStartupMode Consumed Mode=%d NextMode=%d"),
        static_cast<int32>(ConsumedMode),
        static_cast<int32>(PendingStartupMode));

    return ConsumedMode;
}

void UGrimrockGameInstance::ClearPendingStartupMode()
{
    PendingStartupMode = EGrimrockPartyStartupMode::Continue;

    UE_LOG(LogTemp, Log, TEXT("GrimrockGameInstance PendingStartupMode Cleared"));
}

bool UGrimrockGameInstance::HasDefaultPartySaveGame() const
{
    return HasPartySaveGame(DefaultPartySaveSlotName, DefaultPartySaveUserIndex);
}

bool UGrimrockGameInstance::HasPartySaveGame(const FString& SlotName, int32 UserIndex) const
{
    return !SlotName.IsEmpty() &&
        UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

FString UGrimrockGameInstance::GetDefaultPartySaveSlotName() const
{
    return DefaultPartySaveSlotName;
}

int32 UGrimrockGameInstance::GetDefaultPartySaveUserIndex() const
{
    return DefaultPartySaveUserIndex;
}

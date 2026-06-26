#include "Runtime/GrimrockGameInstance.h"

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

#include "Save/GrimrockSaveSlotLibrary.h"

#include "Runtime/GrimrockPartyPawn.h"

bool UGrimrockSaveSlotLibrary::SetPartySaveSlot(
    AGrimrockPartyPawn* PartyPawn,
    const FString& SlotName,
    int32 UserIndex,
    FText& OutError)
{
    OutError = FText::GetEmpty();

    if (!PartyPawn)
    {
        OutError = FText::FromString(TEXT("Le pawn du groupe est indisponible."));
        return false;
    }

    if (SlotName.IsEmpty())
    {
        OutError = FText::FromString(TEXT("Le nom du slot de sauvegarde est vide."));
        return false;
    }

    if (UserIndex < 0)
    {
        OutError = FText::FromString(TEXT("L'index utilisateur de sauvegarde est invalide."));
        return false;
    }

    PartyPawn->PartySaveSlotName = SlotName;
    PartyPawn->PartySaveUserIndex = UserIndex;

    UE_LOG(
        LogTemp,
        Log,
        TEXT("PartySave ActiveSlot Set Pawn=%s Slot=%s UserIndex=%d"),
        *GetNameSafe(PartyPawn),
        *PartyPawn->PartySaveSlotName,
        PartyPawn->PartySaveUserIndex);

    return true;
}

bool UGrimrockSaveSlotLibrary::SavePartyGameToSlot(
    AGrimrockPartyPawn* PartyPawn,
    const FString& SlotName,
    int32 UserIndex,
    FText& OutError)
{
    if (!SetPartySaveSlot(PartyPawn, SlotName, UserIndex, OutError))
    {
        return false;
    }

    if (!PartyPawn->SaveCurrentGame(OutError))
    {
        return false;
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("PartySave SavedToSlot Pawn=%s Slot=%s UserIndex=%d KeepActive=true"),
        *GetNameSafe(PartyPawn),
        *SlotName,
        UserIndex);

    return true;
}

bool UGrimrockSaveSlotLibrary::SavePartyGameCopyToSlot(
    AGrimrockPartyPawn* PartyPawn,
    const FString& SlotName,
    int32 UserIndex,
    FText& OutError)
{
    OutError = FText::GetEmpty();

    if (!PartyPawn)
    {
        OutError = FText::FromString(TEXT("Le pawn du groupe est indisponible."));
        return false;
    }

    const FString PreviousSlotName = PartyPawn->PartySaveSlotName;
    const int32 PreviousUserIndex = PartyPawn->PartySaveUserIndex;

    if (!SetPartySaveSlot(PartyPawn, SlotName, UserIndex, OutError))
    {
        return false;
    }

    const bool bSaved = PartyPawn->SaveCurrentGame(OutError);

    PartyPawn->PartySaveSlotName = PreviousSlotName;
    PartyPawn->PartySaveUserIndex = PreviousUserIndex;

    UE_LOG(
        LogTemp,
        Log,
        TEXT("PartySave CopyToSlot Pawn=%s Slot=%s UserIndex=%d RestoredActiveSlot=%s RestoredUserIndex=%d Result=%s"),
        *GetNameSafe(PartyPawn),
        *SlotName,
        UserIndex,
        *PartyPawn->PartySaveSlotName,
        PartyPawn->PartySaveUserIndex,
        bSaved ? TEXT("true") : TEXT("false"));

    return bSaved;
}

#include "Runtime/GridWallLockActor.h"

#include "Runtime/GridInteractionUtils.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

DEFINE_LOG_CATEGORY (LogGridWallLock);

void AGridWallLockActor::InitializeGridObject (
    const FGridLevelObjectData& ObjectData,
    UStaticMesh* Mesh,
    UMaterialInterface* Material,
    const FTransform& WorldTransform)
{
    Super::InitializeGridObject (ObjectData, Mesh, Material, WorldTransform);

    const FGridLockBehaviorParams& LockParams = ObjectData.Behavior.Lock;
    bIsUnlocked = LockParams.bStartsUnlocked;
    bConsumeKeyOnUnlock = LockParams.bConsumeKeyOnUnlock;
    LockedMessage = LockParams.LockedMessage;
    UnlockedMessage = LockParams.UnlockedMessage;
    MissingKeyMessage = LockParams.MissingKeyMessage;

    AcceptedKeyDefinitionIds.Reset ();
    for (const UGridItemDefinitionAsset* AcceptedKeyItem : LockParams.AcceptedKeyItems)
    {
        if (AcceptedKeyItem && !AcceptedKeyItem->ItemDefinitionId.IsNone ())
        {
            AcceptedKeyDefinitionIds.AddUnique (AcceptedKeyItem->ItemDefinitionId);
        }
    }
    for (const FName AcceptedKeyId : LockParams.AcceptedKeyIds)
    {
        if (!AcceptedKeyId.IsNone ())
        {
            AcceptedKeyDefinitionIds.AddUnique (AcceptedKeyId);
        }
    }

    UE_LOG (LogGridWallLock, Log,
        TEXT ("GridWallLock Init ObjectId=%s StartsUnlocked=%s ConsumeKey=%s AcceptedKeys=%d"),
        *ObjectId.ToString (),
        bIsUnlocked ? TEXT ("true") : TEXT ("false"),
        bConsumeKeyOnUnlock ? TEXT ("true") : TEXT ("false"),
        AcceptedKeyDefinitionIds.Num ());
}

bool AGridWallLockActor::TryInteractWithParty (AGrimrockPartyPawn* PartyPawn)
{
    if (!PartyPawn || !PartyPawn->PartyInventoryComponent || !PartyPawn->LevelRuntimeActor)
    {
        UE_LOG (LogGridWallLock, Warning,
            TEXT ("GridWallLock UnlockFailed MissingKey ObjectId=%s Reason=%s"),
            *ObjectId.ToString (),
            !PartyPawn
                ? TEXT ("MissingPartyPawn")
                : (!PartyPawn->PartyInventoryComponent
                    ? TEXT ("MissingPartyInventory")
                    : TEXT ("MissingRuntimeActor")));
        return false;
    }

    if (bIsUnlocked)
    {
        const FText Message = GetEffectiveUnlockedMessage ();
        ShowFeedback (PartyPawn, Message);
        UE_LOG (LogGridWallLock, Log,
            TEXT ("GridWallLock AlreadyUnlocked ObjectId=%s Message=%s"),
            *ObjectId.ToString (),
            *Message.ToString ());
        return true;
    }

    UGridPartyInventoryComponent* InventoryComponent = PartyPawn->PartyInventoryComponent;
    int32 KeyCharacterIndex = INDEX_NONE;
    FName MatchingKeyId = NAME_None;

    for (int32 CharacterIndex = 0;
        CharacterIndex < InventoryComponent->GetActiveCharacterCount () && MatchingKeyId.IsNone ();
        ++CharacterIndex)
    {
        for (const FName AcceptedKeyId : AcceptedKeyDefinitionIds)
        {
            if (InventoryComponent->HasItemDefinitionInCharacterInventory (CharacterIndex, AcceptedKeyId))
            {
                KeyCharacterIndex = CharacterIndex;
                MatchingKeyId = AcceptedKeyId;
                break;
            }
        }
    }

    if (MatchingKeyId.IsNone ())
    {
        const FText Message = GetEffectiveMissingKeyMessage ();
        ShowFeedback (PartyPawn, Message);
        UE_LOG (LogGridWallLock, Log,
            TEXT ("GridWallLock UnlockFailed MissingKey ObjectId=%s Message=%s"),
            *ObjectId.ToString (),
            *Message.ToString ());
        return false;
    }

    if (bConsumeKeyOnUnlock)
    {
        if (!InventoryComponent->RemoveItemDefinitionFromCharacterInventory (
            KeyCharacterIndex,
            MatchingKeyId,
            1))
        {
            UE_LOG (LogGridWallLock, Warning,
                TEXT ("GridWallLock UnlockFailed MissingKey ObjectId=%s Key=%s Reason=ConsumeFailed"),
                *ObjectId.ToString (),
                *MatchingKeyId.ToString ());
            return false;
        }

        UE_LOG (LogGridWallLock, Log,
            TEXT ("GridWallLock ConsumedKey ObjectId=%s Key=%s CharacterIndex=%d"),
            *ObjectId.ToString (),
            *MatchingKeyId.ToString (),
            KeyCharacterIndex);
    }

    bIsUnlocked = true;
    const FText Message = GetEffectiveUnlockedMessage ();
    ShowFeedback (PartyPawn, Message);

    UE_LOG (LogGridWallLock, Log,
        TEXT ("GridWallLock UnlockSuccess ObjectId=%s Key=%s CharacterIndex=%d Message=%s"),
        *ObjectId.ToString (),
        *MatchingKeyId.ToString (),
        KeyCharacterIndex,
        *Message.ToString ());

    PartyPawn->LevelRuntimeActor->ExecuteLinksFromRuntimeObject (
        ObjectId,
        EGridObjectEvent::Activated);

    return true;
}

bool AGridWallLockActor::CanInteract_Implementation (
    APawn* InstigatorPawn,
    UPrimitiveComponent* HitComponent) const
{
    const AGrimrockPartyPawn* PartyPawn = GridInteractionUtils::ResolvePartyPawn (InstigatorPawn);
    AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor (InstigatorPawn, this);
    return PartyPawn &&
        RuntimeActor &&
        HitComponent == MeshComponent &&
        RuntimeActor->CanPartyInteractWithEdgeObject (CellX, CellY, Edge, PartyPawn);
}

void AGridWallLockActor::Interact_Implementation (
    APawn* InstigatorPawn,
    UPrimitiveComponent* HitComponent)
{
    if (!CanInteract_Implementation (InstigatorPawn, HitComponent))
    {
        return;
    }

    TryInteractWithParty (GridInteractionUtils::ResolvePartyPawn (InstigatorPawn));
}

EGridInteractionCursor AGridWallLockActor::GetInteractionCursor_Implementation (
    UPrimitiveComponent* HitComponent) const
{
    return HitComponent == MeshComponent ? EGridInteractionCursor::Use : EGridInteractionCursor::Default;
}

FText AGridWallLockActor::GetInteractionText_Implementation (
    UPrimitiveComponent* HitComponent) const
{
    if (HitComponent != MeshComponent)
    {
        return FText::GetEmpty ();
    }

    return bIsUnlocked ? GetEffectiveUnlockedMessage () : GetEffectiveLockedMessage ();
}

FText AGridWallLockActor::GetEffectiveLockedMessage () const
{
    return LockedMessage.IsEmpty ()
        ? FText::FromString (TEXT ("Locked."))
        : LockedMessage;
}

FText AGridWallLockActor::GetEffectiveUnlockedMessage () const
{
    return UnlockedMessage.IsEmpty ()
        ? FText::FromString (TEXT ("Unlocked."))
        : UnlockedMessage;
}

FText AGridWallLockActor::GetEffectiveMissingKeyMessage () const
{
    return MissingKeyMessage.IsEmpty ()
        ? FText::FromString (TEXT ("The correct key is required."))
        : MissingKeyMessage;
}

void AGridWallLockActor::ShowFeedback (
    AGrimrockPartyPawn* PartyPawn,
    const FText& Message) const
{
    if (PartyPawn && PartyPawn->LevelRuntimeActor && !Message.IsEmpty ())
    {
        PartyPawn->LevelRuntimeActor->ShowInteractionFeedback (Message);
    }
}

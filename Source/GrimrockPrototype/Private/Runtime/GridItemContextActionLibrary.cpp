#include "Runtime/GridItemContextActionLibrary.h"

#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridGenericObjectActor.h"
#include "Runtime/GridInteractableInterface.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Runtime/GridWallLockActor.h"
#include "Runtime/GrimrockPartyPawn.h"

DEFINE_LOG_CATEGORY_STATIC (LogGridItemActions, Log, All);

namespace
{
    const TCHAR* ToActionTypeText (EGridItemActionType ActionType)
    {
        switch (ActionType)
        {
        case EGridItemActionType::Equip: return TEXT ("Equip");
        case EGridItemActionType::Unequip: return TEXT ("Unequip");
        case EGridItemActionType::Consume: return TEXT ("Consume");
        case EGridItemActionType::Read: return TEXT ("Read");
        case EGridItemActionType::Examine: return TEXT ("Examine");
        case EGridItemActionType::Use: return TEXT ("Use");
        case EGridItemActionType::UseOnTarget: return TEXT ("UseOnTarget");
        case EGridItemActionType::InsertIntoTarget: return TEXT ("InsertIntoTarget");
        case EGridItemActionType::PlaceOnTarget: return TEXT ("PlaceOnTarget");
        case EGridItemActionType::DropToGround: return TEXT ("DropToGround");
        case EGridItemActionType::Throw: return TEXT ("Throw");
        case EGridItemActionType::Combine: return TEXT ("Combine");
        case EGridItemActionType::SplitStack: return TEXT ("SplitStack");
        case EGridItemActionType::ToggleLight: return TEXT ("ToggleLight");
        case EGridItemActionType::None:
        default:
            return TEXT ("None");
        }
    }

    const TCHAR* ToTargetTypeText (EGridFacingTargetType TargetType)
    {
        switch (TargetType)
        {
        case EGridFacingTargetType::WallLock: return TEXT ("WallLock");
        case EGridFacingTargetType::Receptacle: return TEXT ("Receptacle");
        case EGridFacingTargetType::TorchHolder: return TEXT ("TorchHolder");
        case EGridFacingTargetType::Readable: return TEXT ("Readable");
        case EGridFacingTargetType::Door: return TEXT ("Door");
        case EGridFacingTargetType::Mechanism: return TEXT ("Mechanism");
        case EGridFacingTargetType::None:
        default:
            return TEXT ("None");
        }
    }

    AActor* ResolveTargetActor (AActor* HitActor)
    {
        if (!HitActor)
        {
            return nullptr;
        }

        if (HitActor->IsA<AGridRuntimeObjectActor> ())
        {
            return HitActor;
        }

        AActor* Owner = HitActor->GetOwner ();
        return Owner && Owner->IsA<AGridRuntimeObjectActor> () ? Owner : HitActor;
    }

    bool HasItemTag (const UGridItemDefinitionAsset* Definition, FName Tag)
    {
        return Definition && Definition->ItemTags.Contains (Tag);
    }

    bool IsReadableItem (
        const FGridItemInstance& Item,
        const UGridItemDefinitionAsset* Definition)
    {
        return !Item.ReadTextOverride.IsEmpty () ||
            (Item.ReadableContentAsset && !Item.ReadableContentAsset->BodyText.IsEmpty ()) ||
            (Definition &&
             (Definition->ItemType == EGridItemType::Book ||
             Definition->ItemType == EGridItemType::Scroll ||
             HasItemTag (Definition, TEXT ("Readable")) ||
             HasItemTag (Definition, TEXT ("Lisible")) ||
             !Definition->ReadText.IsEmpty ()));
    }

    void AddAction (
        TArray<FGridItemContextAction>& Actions,
        EGridItemActionType ActionType,
        const FText& Label,
        const FGridFacingTargetContext* Target = nullptr,
        EGridEquipmentSlot EquipmentSlot = EGridEquipmentSlot::None,
        bool bEnabled = true,
        const FText& DisabledReason = FText::GetEmpty ())
    {
        FGridItemContextAction& Action = Actions.AddDefaulted_GetRef ();
        Action.ActionType = ActionType;
        Action.Label = Label;
        Action.bEnabled = bEnabled;
        Action.DisabledReason = DisabledReason;
        Action.EquipmentSlot = EquipmentSlot;
        if (Target)
        {
            Action.TargetActor = Target->TargetActor;
            Action.TargetObjectId = Target->TargetObjectId;
        }
    }

    FText GetEquipmentSlotActionLabel (EGridEquipmentSlot Slot)
    {
        switch (Slot)
        {
        case EGridEquipmentSlot::MainHand:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotMainHand", "main directrice");
        case EGridEquipmentSlot::OffHand:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotOffHand", "main secondaire");
        case EGridEquipmentSlot::Head:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotHead", "tete");
        case EGridEquipmentSlot::Chest:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotChest", "torse");
        case EGridEquipmentSlot::Legs:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotLegs", "jambes");
        case EGridEquipmentSlot::Feet:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotFeet", "pieds");
        case EGridEquipmentSlot::Amulet:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotAmulet", "amulette");
        case EGridEquipmentSlot::Ring1:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotRing1", "anneau I");
        case EGridEquipmentSlot::Ring2:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotRing2", "anneau II");
        case EGridEquipmentSlot::Shoulders:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotShoulders", "epaules");
        case EGridEquipmentSlot::Gloves:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotGloves", "gants");
        case EGridEquipmentSlot::Belt:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotBelt", "ceinture");
        case EGridEquipmentSlot::Cloak:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotCloak", "cape");
        case EGridEquipmentSlot::Talisman:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotTalisman", "talisman");
        case EGridEquipmentSlot::QuickSlot1:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotQuickSlot1", "raccourci I");
        case EGridEquipmentSlot::QuickSlot2:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotQuickSlot2", "raccourci II");
        case EGridEquipmentSlot::Face:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotFace", "visage");
        case EGridEquipmentSlot::Shirt:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotShirt", "chemise");
        case EGridEquipmentSlot::Bracers:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotBracers", "brassards");
        case EGridEquipmentSlot::Earring1:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotEarring1", "bijou d'oreille I");
        case EGridEquipmentSlot::Earring2:
            return NSLOCTEXT ("GridItemActions", "EquipmentSlotEarring2", "bijou d'oreille II");
        case EGridEquipmentSlot::None:
        default:
            return FText::GetEmpty ();
        }
    }
}

bool UGridItemContextActionLibrary::BuildInventorySlotContextActions (
    AGrimrockPartyPawn* PartyPawn,
    int32 CharacterIndex,
    int32 InventorySlotIndex,
    FGridFacingTargetContext& OutFacingTarget,
    TArray<FGridItemContextAction>& OutActions)
{
    OutFacingTarget = FGridFacingTargetContext ();
    OutActions.Reset ();
    if (!PartyPawn || !PartyPawn->PartyInventoryComponent)
    {
        return false;
    }

    UGridPartyInventoryComponent* InventoryComponent = PartyPawn->PartyInventoryComponent;
    const FGridPartyInventoryState& State = InventoryComponent->PartyInventoryState;
    if (!State.ActiveCharacters.IsValidIndex (CharacterIndex))
    {
        return false;
    }

    const FGridCharacterInventoryState& CharacterState = State.ActiveCharacters[CharacterIndex];
    if (!CharacterState.InventorySlots.IsValidIndex (InventorySlotIndex) ||
        CharacterState.InventorySlots[InventorySlotIndex].IsEmpty ())
    {
        return false;
    }

    FGridItemActionContext ItemContext;
    ItemContext.PartyPawn = PartyPawn;
    ItemContext.Item = CharacterState.InventorySlots[InventorySlotIndex].Item;
    ItemContext.ItemDefinition = InventoryComponent->FindItemDefinition (ItemContext.Item.ItemDefinitionId);
    ItemContext.CharacterIndex = CharacterIndex;
    ItemContext.InventorySlotIndex = InventorySlotIndex;
    return BuildItemContextActions (ItemContext, OutFacingTarget, OutActions);
}

bool UGridItemContextActionLibrary::BuildItemContextActions (
    const FGridItemActionContext& ItemContext,
    FGridFacingTargetContext& OutFacingTarget,
    TArray<FGridItemContextAction>& OutActions)
{
    OutFacingTarget = FGridFacingTargetContext ();
    OutActions.Reset ();
    if (!ItemContext.PartyPawn || !ItemContext.Item.IsValid ())
    {
        return false;
    }

    UGridItemDefinitionAsset* Definition = ItemContext.ItemDefinition;
    if (!Definition && ItemContext.PartyPawn->PartyInventoryComponent)
    {
        Definition = ItemContext.PartyPawn->PartyInventoryComponent->FindItemDefinition (
            ItemContext.Item.ItemDefinitionId);
    }

    ResolveFacingTarget (
        ItemContext.PartyPawn,
        ItemContext.Item,
        Definition,
        OutFacingTarget);

    AddAction (
        OutActions,
        EGridItemActionType::Examine,
        NSLOCTEXT ("GridItemActions", "Examine", "Examiner"));

    if (ItemContext.EquipmentSlot != EGridEquipmentSlot::None ||
        ItemContext.Item.OwnerType == EGridItemOwnerType::EquipmentSlot)
    {
        const EGridEquipmentSlot SourceEquipmentSlot =
            ItemContext.EquipmentSlot != EGridEquipmentSlot::None
                ? ItemContext.EquipmentSlot
                : ItemContext.Item.EquipmentSlot;
        const FText UnequipLabel = NSLOCTEXT ("GridItemActions", "Unequip", "Enlever");
        AddAction (
            OutActions,
            EGridItemActionType::Unequip,
            UnequipLabel,
            nullptr,
            SourceEquipmentSlot);
    }
    else if (Definition)
    {
        if (Definition->CanEquipToSlot (EGridEquipmentSlot::MainHand))
        {
            AddAction (
                OutActions,
                EGridItemActionType::Equip,
                FText::Format (
                    NSLOCTEXT ("GridItemActions", "EquipSpecificSlot", "S'equiper : {0}"),
                    GetEquipmentSlotActionLabel (EGridEquipmentSlot::MainHand)),
                nullptr,
                EGridEquipmentSlot::MainHand);
        }
        if (Definition->CanEquipToSlot (EGridEquipmentSlot::OffHand))
        {
            AddAction (
                OutActions,
                EGridItemActionType::Equip,
                FText::Format (
                    NSLOCTEXT ("GridItemActions", "EquipSpecificSlot", "S'equiper : {0}"),
                    GetEquipmentSlotActionLabel (EGridEquipmentSlot::OffHand)),
                nullptr,
                EGridEquipmentSlot::OffHand);
        }
        for (const EGridEquipmentSlot CompatibleSlot : Definition->CompatibleEquipmentSlots)
        {
            if (CompatibleSlot == EGridEquipmentSlot::None ||
                CompatibleSlot == EGridEquipmentSlot::MainHand ||
                CompatibleSlot == EGridEquipmentSlot::OffHand)
            {
                continue;
            }

            const FText SlotLabel = GetEquipmentSlotActionLabel (CompatibleSlot);
            if (SlotLabel.IsEmpty ())
            {
                continue;
            }

            AddAction (
                OutActions,
                EGridItemActionType::Equip,
                FText::Format (
                    NSLOCTEXT ("GridItemActions", "EquipSpecificSlot", "S'equiper : {0}"),
                    SlotLabel),
                nullptr,
                CompatibleSlot);
        }
    }

    if (Definition &&
        (Definition->ItemType == EGridItemType::Potion ||
         Definition->ItemType == EGridItemType::Food))
    {
        AddAction (
            OutActions,
            EGridItemActionType::Consume,
            NSLOCTEXT ("GridItemActions", "Consume", "Consommer"));
    }

    if (IsReadableItem (ItemContext.Item, Definition))
    {
        AddAction (
            OutActions,
            EGridItemActionType::Read,
            NSLOCTEXT ("GridItemActions", "Read", "Lire"));
    }

    const bool bIsInventorySlotSource =
        ItemContext.InventorySlotIndex != INDEX_NONE &&
        ItemContext.EquipmentSlot == EGridEquipmentSlot::None;
    const bool bIsEquipmentSlotSource =
        ItemContext.EquipmentSlot != EGridEquipmentSlot::None;

    if (bIsInventorySlotSource &&
        OutFacingTarget.bIsValid &&
        OutFacingTarget.bAcceptsCurrentItem &&
        OutFacingTarget.TargetType == EGridFacingTargetType::WallLock)
    {
        AddAction (
            OutActions,
            EGridItemActionType::InsertIntoTarget,
            NSLOCTEXT ("GridItemActions", "InsertIntoLock", "Insérer dans la serrure"),
            &OutFacingTarget);
    }
    else if ((bIsInventorySlotSource || bIsEquipmentSlotSource) &&
             OutFacingTarget.bIsValid &&
             OutFacingTarget.bAcceptsCurrentItem &&
             (OutFacingTarget.TargetType == EGridFacingTargetType::Receptacle ||
              OutFacingTarget.TargetType == EGridFacingTargetType::TorchHolder))
    {
        const bool bLooksLikeAlcove =
            OutFacingTarget.TargetActor &&
            OutFacingTarget.TargetActor->GetName ().Contains (TEXT ("Alcove"));
        const FText Label = OutFacingTarget.TargetType == EGridFacingTargetType::TorchHolder
            ? NSLOCTEXT ("GridItemActions", "PlaceOnTorchHolder", "Placer sur le support")
            : (bLooksLikeAlcove
                ? NSLOCTEXT ("GridItemActions", "PlaceInAlcove", "Placer dans l'alcôve")
                : NSLOCTEXT ("GridItemActions", "PlaceOnTarget", "Placer sur la cible"));
        AddAction (
            OutActions,
            EGridItemActionType::PlaceOnTarget,
            Label,
            &OutFacingTarget);
    }

    if (Definition && Definition->bStackable && ItemContext.Item.Quantity > 1)
    {
        AddAction (
            OutActions,
            EGridItemActionType::SplitStack,
            NSLOCTEXT ("GridItemActions", "SplitStack", "Scinder la pile"));
    }

    const bool bCanDrop =
        ItemContext.PartyPawn->LevelRuntimeActor != nullptr &&
        (bIsInventorySlotSource || bIsEquipmentSlotSource);
    AddAction (
        OutActions,
        EGridItemActionType::DropToGround,
        NSLOCTEXT ("GridItemActions", "DropToGround", "Déposer au sol"),
        nullptr,
        ItemContext.EquipmentSlot,
        bCanDrop,
        bCanDrop
            ? FText::GetEmpty ()
            : NSLOCTEXT ("GridItemActions", "NoActiveLevel", "Aucun niveau actif"));

    UE_LOG (LogGridItemActions, Log,
        TEXT ("GridItemActions Build Item=%s Target=%s"),
        *ItemContext.Item.ItemDefinitionId.ToString (),
        OutFacingTarget.bIsValid
            ? *GetNameSafe (OutFacingTarget.TargetActor)
            : TEXT ("None"));
    for (const FGridItemContextAction& Action : OutActions)
    {
        UE_LOG (LogGridItemActions, Log,
            TEXT ("GridItemActions Action=%s Enabled=%s Reason=%s"),
            ToActionTypeText (Action.ActionType),
            Action.bEnabled ? TEXT ("true") : TEXT ("false"),
            Action.DisabledReason.IsEmpty ()
                ? TEXT ("None")
                : *Action.DisabledReason.ToString ());
    }

    return OutActions.Num () > 0;
}

bool UGridItemContextActionLibrary::ResolveFacingTarget (
    AGrimrockPartyPawn* PartyPawn,
    const FGridItemInstance& CurrentItem,
    UGridItemDefinitionAsset* ItemDefinition,
    FGridFacingTargetContext& OutFacingTarget,
    float TraceDistance)
{
    OutFacingTarget = FGridFacingTargetContext ();
    if (!PartyPawn || !PartyPawn->GetWorld ())
    {
        return false;
    }

    const FVector TraceStart = PartyPawn->Camera
        ? PartyPawn->Camera->GetComponentLocation ()
        : PartyPawn->GetActorLocation ();
    const FVector TraceEnd =
        TraceStart + PartyPawn->GetActorForwardVector () * FMath::Max (0.f, TraceDistance);

    FCollisionQueryParams QueryParams (SCENE_QUERY_STAT (GridFacingTargetTrace), true);
    QueryParams.AddIgnoredActor (PartyPawn);
    if (PartyPawn->HeldItemActor)
    {
        QueryParams.AddIgnoredActor (PartyPawn->HeldItemActor);
    }

    FHitResult HitResult;
    const bool bHasTraceHit = PartyPawn->GetWorld ()->LineTraceSingleByChannel (
        HitResult,
        TraceStart,
        TraceEnd,
        ECC_Visibility,
        QueryParams);
    UE_LOG (LogGridItemActions, Log,
        TEXT ("GridFacingTarget TraceHit Actor=%s Component=%s"),
        bHasTraceHit ? *GetNameSafe (HitResult.GetActor ()) : TEXT ("None"),
        bHasTraceHit ? *GetNameSafe (HitResult.GetComponent ()) : TEXT ("None"));

    AActor* TargetActor = bHasTraceHit
        ? ResolveTargetActor (HitResult.GetActor ())
        : nullptr;
    if (TargetActor)
    {
        OutFacingTarget.bIsValid = true;
        OutFacingTarget.TargetActor = TargetActor;
        OutFacingTarget.TargetComponent = HitResult.GetComponent ();
        if (const AGridRuntimeObjectActor* RuntimeObject = Cast<AGridRuntimeObjectActor> (TargetActor))
        {
            OutFacingTarget.TargetObjectId = RuntimeObject->ObjectId;
            OutFacingTarget.LevelObjectType = RuntimeObject->ObjectType;
        }
    }

    if (const AGridWallLockActor* WallLockActor = Cast<AGridWallLockActor> (TargetActor))
    {
        OutFacingTarget.TargetType = EGridFacingTargetType::WallLock;
        OutFacingTarget.bAcceptsCurrentItem =
            !WallLockActor->bIsUnlocked &&
            WallLockActor->CanAcceptKeyDefinition (CurrentItem.ItemDefinitionId);
        if (!OutFacingTarget.bAcceptsCurrentItem)
        {
            OutFacingTarget.IncompatibilityReason =
                NSLOCTEXT ("GridItemActions", "WrongKey", "Cet objet ne peut pas être utilisé ici");
        }
    }
    else if (const AGridReceptacleActor* ReceptacleActor = Cast<AGridReceptacleActor> (TargetActor))
    {
        FGridReceptacleAcceptanceResult AcceptanceResult;
        OutFacingTarget.bAcceptsCurrentItem =
            ReceptacleActor->EvaluateItemAcceptance (
                CurrentItem,
                AcceptanceResult,
                false);
        const bool bLooksLikeTorchHolder =
            TargetActor->GetName ().Contains (TEXT ("TorchHolder")) ||
            (ItemDefinition &&
             ItemDefinition->ItemType == EGridItemType::Torch &&
             OutFacingTarget.bAcceptsCurrentItem &&
             !ReceptacleActor->bAcceptAnyItem);
        OutFacingTarget.TargetType = bLooksLikeTorchHolder
            ? EGridFacingTargetType::TorchHolder
            : EGridFacingTargetType::Receptacle;
        if (!OutFacingTarget.bAcceptsCurrentItem)
        {
            OutFacingTarget.IncompatibilityReason =
                NSLOCTEXT ("GridItemActions", "TargetRejectsItem", "Cet objet ne peut pas être utilisé ici");
        }
    }
    else if (const AGridGenericObjectActor* GenericActor = Cast<AGridGenericObjectActor> (TargetActor);
             GenericActor && GenericActor->HasReadableText ())
    {
        OutFacingTarget.TargetType = EGridFacingTargetType::Readable;
    }
    else if (TargetActor && TargetActor->IsA<AGridDoorActor> ())
    {
        OutFacingTarget.TargetType = EGridFacingTargetType::Door;
    }
    else if (TargetActor &&
             TargetActor->GetClass ()->ImplementsInterface (UGridInteractableInterface::StaticClass ()))
    {
        OutFacingTarget.TargetType = EGridFacingTargetType::Mechanism;
    }

    if (OutFacingTarget.TargetType == EGridFacingTargetType::None &&
        PartyPawn->LevelRuntimeActor)
    {
        if (AGridWallLockActor* WallLockActor =
            PartyPawn->LevelRuntimeActor->FindWallLockAtEdge (
                PartyPawn->CurrentCellX,
                PartyPawn->CurrentCellY,
                PartyPawn->Facing))
        {
            OutFacingTarget.bIsValid = true;
            OutFacingTarget.TargetActor = WallLockActor;
            OutFacingTarget.TargetComponent = WallLockActor->MeshComponent;
            OutFacingTarget.TargetObjectId = WallLockActor->ObjectId;
            OutFacingTarget.LevelObjectType = WallLockActor->ObjectType;
            OutFacingTarget.TargetType = EGridFacingTargetType::WallLock;
            OutFacingTarget.bAcceptsCurrentItem =
                !WallLockActor->bIsUnlocked &&
                WallLockActor->CanAcceptKeyDefinition (CurrentItem.ItemDefinitionId);
            if (!OutFacingTarget.bAcceptsCurrentItem)
            {
                OutFacingTarget.IncompatibilityReason =
                    NSLOCTEXT ("GridItemActions", "WrongKey", "Cet objet ne peut pas être utilisé ici");
            }

        }
        else
        {
            UE_LOG (LogGridItemActions, Log,
                TEXT ("GridFacingTarget NoWallLockFound Cell=(%d,%d) Edge=%d"),
                PartyPawn->CurrentCellX,
                PartyPawn->CurrentCellY,
                static_cast<int32> (PartyPawn->Facing));
        }
    }

    if (OutFacingTarget.TargetType == EGridFacingTargetType::WallLock)
    {
        UE_LOG (LogGridItemActions, Log,
            TEXT ("GridFacingTarget Resolved WallLock ObjectId=%s"),
            *OutFacingTarget.TargetObjectId.ToString ());
    }

    if (OutFacingTarget.TargetType == EGridFacingTargetType::None)
    {
        OutFacingTarget = FGridFacingTargetContext ();
        UE_LOG (LogGridItemActions, Log,
            TEXT ("GridFacingTarget TargetActor=None ObjectId=None Type=None"));
        return false;
    }

    UE_LOG (LogGridItemActions, Log,
        TEXT ("GridFacingTarget TargetActor=%s ObjectId=%s Type=%s"),
        *GetNameSafe (OutFacingTarget.TargetActor),
        OutFacingTarget.TargetObjectId.IsValid ()
            ? *OutFacingTarget.TargetObjectId.ToString ()
            : TEXT ("None"),
        ToTargetTypeText (OutFacingTarget.TargetType));
    return true;
}

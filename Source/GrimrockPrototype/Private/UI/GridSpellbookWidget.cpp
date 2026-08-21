#include "UI/GridSpellbookWidget.h"

#include "Magic/GridPartySpellbookComponent.h"
#include "Misc/GuardValue.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"

void UGridSpellbookWidget::InitializeSpellbookWidget (
    AGrimrockPartyPawn* InPartyPawn)
{
    if (InventoryComponent)
    {
        InventoryComponent->OnPartyInventoryChanged.RemoveDynamic (
            this,
            &UGridSpellbookWidget::HandlePartyInventoryChanged);
    }
    if (SpellbookComponent)
    {
        SpellbookComponent->OnSpellbookChanged.RemoveDynamic (
            this,
            &UGridSpellbookWidget::HandleSpellbookChanged);
    }

    OwningPartyPawn = InPartyPawn;
    InventoryComponent =
        InPartyPawn ? InPartyPawn->PartyInventoryComponent : nullptr;
    SpellbookComponent = ResolveOrCreateSpellbookComponent (InPartyPawn);

    if (InventoryComponent)
    {
        InventoryComponent->OnPartyInventoryChanged.RemoveDynamic (
            this,
            &UGridSpellbookWidget::HandlePartyInventoryChanged);
        InventoryComponent->OnPartyInventoryChanged.AddDynamic (
            this,
            &UGridSpellbookWidget::HandlePartyInventoryChanged);
    }
    if (SpellbookComponent)
    {
        SpellbookComponent->OnSpellbookChanged.RemoveDynamic (
            this,
            &UGridSpellbookWidget::HandleSpellbookChanged);
        SpellbookComponent->OnSpellbookChanged.AddDynamic (
            this,
            &UGridSpellbookWidget::HandleSpellbookChanged);
    }

    RefreshSpellbook ();
}

void UGridSpellbookWidget::NativeDestruct ()
{
    if (InventoryComponent)
    {
        InventoryComponent->OnPartyInventoryChanged.RemoveDynamic (
            this,
            &UGridSpellbookWidget::HandlePartyInventoryChanged);
    }
    if (SpellbookComponent)
    {
        SpellbookComponent->OnSpellbookChanged.RemoveDynamic (
            this,
            &UGridSpellbookWidget::HandleSpellbookChanged);
    }

    Super::NativeDestruct ();
}

void UGridSpellbookWidget::ClearViewState ()
{
    SelectedCharacterIndex = INDEX_NONE;
    SelectedCharacterId.Invalidate ();
    SpellEntries.Reset ();
}

UGridPartySpellbookComponent*
UGridSpellbookWidget::ResolveOrCreateSpellbookComponent (
    AGrimrockPartyPawn* PartyPawn) const
{
    if (!PartyPawn)
    {
        return nullptr;
    }

    if (UGridPartySpellbookComponent* Existing =
            PartyPawn->FindComponentByClass<UGridPartySpellbookComponent> ())
    {
        return Existing;
    }

    UGridPartySpellbookComponent* Created =
        NewObject<UGridPartySpellbookComponent> (
            PartyPawn,
            UGridPartySpellbookComponent::StaticClass (),
            TEXT ("PartySpellbookComponent"));
    if (!Created)
    {
        UE_LOG (
            LogTemp,
            Warning,
            TEXT ("GridSpellbookWidget failed to create PartySpellbookComponent."));
        return nullptr;
    }

    PartyPawn->AddInstanceComponent (Created);
    Created->RegisterComponent ();

    UE_LOG (
        LogTemp,
        VeryVerbose,
        TEXT ("GridSpellbookWidget created runtime PartySpellbookComponent for %s."),
        *GetNameSafe (PartyPawn));

    return Created;
}

void UGridSpellbookWidget::RefreshSpellbook ()
{
    if (bRefreshInProgress)
    {
        return;
    }
    TGuardValue<bool> RefreshGuard (bRefreshInProgress, true);

    ClearViewState ();

    if (!InventoryComponent || !SpellbookComponent)
    {
        OnSpellbookRefreshed.Broadcast ();
        return;
    }

    SelectedCharacterIndex = InventoryComponent->GetSelectedCharacterIndex ();
    if (!InventoryComponent->IsValidCharacterIndex (SelectedCharacterIndex))
    {
        ClearViewState ();
        OnSpellbookRefreshed.Broadcast ();
        return;
    }

    SelectedCharacterId = InventoryComponent->
        PartyInventoryState.ActiveCharacters[SelectedCharacterIndex].CharacterId;
    if (!SelectedCharacterId.IsValid ())
    {
        ClearViewState ();
        OnSpellbookRefreshed.Broadcast ();
        return;
    }

    // Registration creates only the empty per-character knowledge container.
    // It never teaches a spell and keeps SpellId as the sole stable identity.
    if (!SpellbookComponent->EnsureCharacterSpellbook (SelectedCharacterId))
    {
        ClearViewState ();
        OnSpellbookRefreshed.Broadcast ();
        return;
    }

    const FGridCharacterSpellbookState* CharacterSpellbook =
        GetSelectedCharacterSpellbook ();
    if (!CharacterSpellbook)
    {
        ClearViewState ();
        OnSpellbookRefreshed.Broadcast ();
        return;
    }

    TArray<FGridCombatHotbarBinding> HotbarBindings;
    const int32 HotbarSlotCount = InventoryComponent->GetCombatHotbarSlotCount ();
    HotbarBindings.SetNum (HotbarSlotCount);

    for (int32 SlotIndex = 0; SlotIndex < HotbarSlotCount; ++SlotIndex)
    {
        HotbarBindings[SlotIndex].Reset (SlotIndex);
        InventoryComponent->GetCharacterCombatHotbarBinding (
            SelectedCharacterIndex,
            SlotIndex,
            HotbarBindings[SlotIndex]);
    }

    UGridSpellbookUILibrary::BuildProductionSpellbookEntries (
        *CharacterSpellbook,
        HotbarBindings,
        SpellEntries);

    OnSpellbookRefreshed.Broadcast ();
}

int32 UGridSpellbookWidget::GetSpellEntryCount () const
{
    return SpellEntries.Num ();
}

bool UGridSpellbookWidget::GetSpellEntry (
    int32 EntryIndex,
    FGridSpellbookEntryView& OutEntry) const
{
    if (!SpellEntries.IsValidIndex (EntryIndex))
    {
        OutEntry = FGridSpellbookEntryView ();
        return false;
    }

    OutEntry = SpellEntries[EntryIndex];
    return true;
}

EGridSpellHotbarAssignmentResult UGridSpellbookWidget::AssignSpellToHotbar (
    FName SpellId,
    int32 TargetSlotIndex)
{
    const FGridCharacterSpellbookState* CharacterSpellbook =
        GetSelectedCharacterSpellbook ();
    if (!InventoryComponent || !CharacterSpellbook)
    {
        return EGridSpellHotbarAssignmentResult::InvalidCharacter;
    }

    const EGridSpellHotbarAssignmentResult Result =
        UGridSpellbookUILibrary::AssignKnownSpellToHotbar (
            InventoryComponent,
            SelectedCharacterIndex,
            *CharacterSpellbook,
            SpellId,
            TargetSlotIndex);

    if (Result == EGridSpellHotbarAssignmentResult::Success)
    {
        RefreshSpellbook ();
    }
    return Result;
}

EGridSpellHotbarAssignmentResult UGridSpellbookWidget::UnassignSpellFromHotbar (
    FName SpellId)
{
    const FGridCharacterSpellbookState* CharacterSpellbook =
        GetSelectedCharacterSpellbook ();
    if (!InventoryComponent || !CharacterSpellbook)
    {
        return EGridSpellHotbarAssignmentResult::InvalidCharacter;
    }

    const EGridSpellHotbarAssignmentResult Result =
        UGridSpellbookUILibrary::UnassignSpellFromHotbar (
            InventoryComponent,
            SelectedCharacterIndex,
            *CharacterSpellbook,
            SpellId);

    if (Result == EGridSpellHotbarAssignmentResult::Success)
    {
        RefreshSpellbook ();
    }
    return Result;
}

void UGridSpellbookWidget::HandlePartyInventoryChanged (int32 CharacterIndex)
{
    RefreshSpellbook ();
}

void UGridSpellbookWidget::HandleSpellbookChanged ()
{
    RefreshSpellbook ();
}

const FGridCharacterSpellbookState*
UGridSpellbookWidget::GetSelectedCharacterSpellbook () const
{
    if (!SpellbookComponent || !SelectedCharacterId.IsValid ())
    {
        return nullptr;
    }

    return SpellbookComponent->SpellbookState.FindSpellbook (
        SelectedCharacterId);
}

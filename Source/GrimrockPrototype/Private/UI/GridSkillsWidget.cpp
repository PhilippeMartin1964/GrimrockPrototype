#include "UI/GridSkillsWidget.h"

#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridSkillsPageService.h"

void UGridSkillsWidget::InitializeSkillsWidget (
    AGrimrockPartyPawn* InPartyPawn)
{
    if (InventoryComponent)
    {
        InventoryComponent->OnPartyInventoryChanged.RemoveDynamic (
            this,
            &UGridSkillsWidget::HandlePartyInventoryChanged);
    }

    OwningPartyPawn = InPartyPawn;
    InventoryComponent =
        InPartyPawn ? InPartyPawn->PartyInventoryComponent : nullptr;

    if (InventoryComponent)
    {
        InventoryComponent->OnPartyInventoryChanged.RemoveDynamic (
            this,
            &UGridSkillsWidget::HandlePartyInventoryChanged);
        InventoryComponent->OnPartyInventoryChanged.AddDynamic (
            this,
            &UGridSkillsWidget::HandlePartyInventoryChanged);
    }

    RefreshSkills ();
}

void UGridSkillsWidget::NativeDestruct ()
{
    if (InventoryComponent)
    {
        InventoryComponent->OnPartyInventoryChanged.RemoveDynamic (
            this,
            &UGridSkillsWidget::HandlePartyInventoryChanged);
    }

    Super::NativeDestruct ();
}

void UGridSkillsWidget::ClearView ()
{
    View = FGridSkillsPageView ();
}

void UGridSkillsWidget::RefreshSkills ()
{
    if (bRefreshInProgress)
    {
        return;
    }
    TGuardValue<bool> RefreshGuard (bRefreshInProgress, true);

    ClearView ();
    if (!InventoryComponent)
    {
        OnSkillsRefreshed.Broadcast ();
        return;
    }

    TArray<const URPGSkillAsset*> SkillDefinitions;
    FGridSkillsPageService::ResolveCanonicalSkillDefinitions (
        SkillDefinitions);

    FGridSkillsPageView Candidate;
    if (FGridSkillsPageService::TryBuildSelectedCharacterView (
            InventoryComponent,
            SkillDefinitions,
            Candidate))
    {
        View = MoveTemp (Candidate);
    }

    OnSkillsRefreshed.Broadcast ();
}

int32 UGridSkillsWidget::GetSkillEntryCount () const
{
    return View.Skills.Num ();
}

bool UGridSkillsWidget::GetSkillEntry (
    int32 EntryIndex,
    FGridSkillEntryView& OutEntry) const
{
    if (!View.Skills.IsValidIndex (EntryIndex))
    {
        OutEntry = FGridSkillEntryView ();
        return false;
    }

    OutEntry = View.Skills[EntryIndex];
    return true;
}

int32 UGridSkillsWidget::GetTalentEntryCount () const
{
    return View.Talents.Num ();
}

bool UGridSkillsWidget::GetTalentEntry (
    int32 EntryIndex,
    FGridTalentEntryView& OutEntry) const
{
    if (!View.Talents.IsValidIndex (EntryIndex))
    {
        OutEntry = FGridTalentEntryView ();
        return false;
    }

    OutEntry = View.Talents[EntryIndex];
    return true;
}

void UGridSkillsWidget::HandlePartyInventoryChanged (int32 CharacterIndex)
{
    (void)CharacterIndex;
    RefreshSkills ();
}

#include "UI/GridSkillsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "RPG/RPGSkillTypes.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridSkillsPageService.h"

namespace GridSkillsWidgetPrivate
{
    FText GetAttributeLabel (ERPGSkillGoverningAttribute Attribute)
    {
        switch (Attribute)
        {
        case ERPGSkillGoverningAttribute::Strength:
            return FText::FromString (TEXT ("Force"));
        case ERPGSkillGoverningAttribute::Dexterity:
            return FText::FromString (TEXT ("Dextérité"));
        case ERPGSkillGoverningAttribute::Constitution:
            return FText::FromString (TEXT ("Constitution"));
        case ERPGSkillGoverningAttribute::Intelligence:
            return FText::FromString (TEXT ("Intelligence"));
        case ERPGSkillGoverningAttribute::Wisdom:
            return FText::FromString (TEXT ("Sagesse"));
        case ERPGSkillGoverningAttribute::Charisma:
            return FText::FromString (TEXT ("Charisme"));
        case ERPGSkillGoverningAttribute::None:
        default:
            return FText::FromString (TEXT ("Aucun"));
        }
    }

    UTextBlock* AddText (
        UWidgetTree* WidgetTree,
        UVerticalBox* Parent,
        const FText& Text,
        int32 FontSize,
        const FMargin& Padding,
        ETextJustify::Type Justification = ETextJustify::Left)
    {
        if (!WidgetTree || !Parent)
        {
            return nullptr;
        }

        UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock> (
            UTextBlock::StaticClass ());
        if (!TextBlock)
        {
            return nullptr;
        }

        TextBlock->SetText (Text);
        TextBlock->SetAutoWrapText (true);
        TextBlock->SetJustification (Justification);

        FSlateFontInfo Font = TextBlock->GetFont ();
        Font.Size = FontSize;
        TextBlock->SetFont (Font);

        if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox (TextBlock))
        {
            Slot->SetPadding (Padding);
            Slot->SetHorizontalAlignment (HAlign_Fill);
        }
        return TextBlock;
    }
}

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

    NativeContentBox = nullptr;
    NativeScrollBox = nullptr;

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
    if (InventoryComponent)
    {
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
    }

    RebuildPresentation ();
    OnSkillsRefreshed.Broadcast ();
}

void UGridSkillsWidget::RebuildPresentation ()
{
    if (!WidgetTree)
    {
        return;
    }

    UBorder* RootBorder = Cast<UBorder> (WidgetTree->RootWidget);
    if (!RootBorder)
    {
        UE_LOG (
            LogTemp,
            Warning,
            TEXT ("GridSkillsWidget native presentation requires WBP_GridSkills root widget to remain a Border."));
        return;
    }

    if (!IsValid (NativeScrollBox) || !IsValid (NativeContentBox))
    {
        NativeScrollBox = WidgetTree->ConstructWidget<UScrollBox> (
            UScrollBox::StaticClass (),
            TEXT ("NativeSkillsScroll"));
        NativeContentBox = WidgetTree->ConstructWidget<UVerticalBox> (
            UVerticalBox::StaticClass (),
            TEXT ("NativeSkillsContent"));
        if (!NativeScrollBox || !NativeContentBox)
        {
            UE_LOG (
                LogTemp,
                Warning,
                TEXT ("GridSkillsWidget failed to construct native presentation widgets."));
            return;
        }

        NativeScrollBox->AddChild (NativeContentBox);
        RootBorder->SetContent (NativeScrollBox);
    }
    else if (RootBorder->GetContent () != NativeScrollBox)
    {
        RootBorder->SetContent (NativeScrollBox);
    }

    NativeContentBox->ClearChildren ();

    using namespace GridSkillsWidgetPrivate;

    AddText (
        WidgetTree,
        NativeContentBox,
        FText::FromString (TEXT ("Compétences & talents")),
        28,
        FMargin (24.0f, 20.0f, 24.0f, 12.0f),
        ETextJustify::Center);

    if (!View.IsValid ())
    {
        AddText (
            WidgetTree,
            NativeContentBox,
            FText::FromString (
                TEXT ("Aucun personnage sélectionné ou données indisponibles.")),
            18,
            FMargin (32.0f, 24.0f));
        return;
    }

    AddText (
        WidgetTree,
        NativeContentBox,
        View.CharacterName.IsEmpty ()
            ? FText::FromString (TEXT ("Personnage sélectionné"))
            : View.CharacterName,
        22,
        FMargin (32.0f, 8.0f, 32.0f, 4.0f));

    AddText (
        WidgetTree,
        NativeContentBox,
        FText::FromString (FString::Printf (
            TEXT ("Points de talent : %d disponibles — %d dépensés — %d accordés"),
            View.RemainingTalentPoints,
            View.SpentTalentPoints,
            View.GrantedTalentPoints)),
        16,
        FMargin (32.0f, 0.0f, 32.0f, 20.0f));

    AddText (
        WidgetTree,
        NativeContentBox,
        FText::FromString (TEXT ("Compétences")),
        22,
        FMargin (32.0f, 8.0f, 32.0f, 8.0f));

    if (View.Skills.IsEmpty ())
    {
        AddText (
            WidgetTree,
            NativeContentBox,
            FText::FromString (TEXT ("Aucune compétence définie.")),
            16,
            FMargin (48.0f, 4.0f, 32.0f, 16.0f));
    }
    else
    {
        for (const FGridSkillEntryView& Skill : View.Skills)
        {
            const FString SkillName = Skill.DisplayName.IsEmpty ()
                ? Skill.SkillId.ToString ()
                : Skill.DisplayName.ToString ();

            AddText (
                WidgetTree,
                NativeContentBox,
                FText::FromString (FString::Printf (
                    TEXT ("%s — Rang %d/%d"),
                    *SkillName,
                    Skill.Rank,
                    Skill.MaxRank)),
                18,
                FMargin (48.0f, 6.0f, 32.0f, 2.0f));

            AddText (
                WidgetTree,
                NativeContentBox,
                FText::FromString (FString::Printf (
                    TEXT ("Attribut : %s%s"),
                    *GetAttributeLabel (Skill.GoverningAttribute).ToString (),
                    Skill.bTrained ? TEXT (" — Entraînée") : TEXT (" — Non entraînée"))),
                14,
                FMargin (64.0f, 0.0f, 32.0f, 2.0f));

            if (!Skill.Description.IsEmpty ())
            {
                AddText (
                    WidgetTree,
                    NativeContentBox,
                    Skill.Description,
                    14,
                    FMargin (64.0f, 0.0f, 40.0f, 8.0f));
            }
        }
    }

    AddText (
        WidgetTree,
        NativeContentBox,
        FText::FromString (TEXT ("Talents acquis")),
        22,
        FMargin (32.0f, 20.0f, 32.0f, 8.0f));

    if (View.Talents.IsEmpty ())
    {
        AddText (
            WidgetTree,
            NativeContentBox,
            FText::FromString (TEXT ("Aucun talent acquis.")),
            16,
            FMargin (48.0f, 4.0f, 32.0f, 20.0f));
    }
    else
    {
        for (const FGridTalentEntryView& Talent : View.Talents)
        {
            const FString TalentName = Talent.DisplayName.IsEmpty ()
                ? Talent.ChoiceId.ToString ()
                : Talent.DisplayName.ToString ();

            AddText (
                WidgetTree,
                NativeContentBox,
                FText::FromString (FString::Printf (
                    TEXT ("%s — coût %d"),
                    *TalentName,
                    Talent.PointCost)),
                18,
                FMargin (48.0f, 6.0f, 32.0f, 2.0f));

            if (!Talent.Description.IsEmpty ())
            {
                AddText (
                    WidgetTree,
                    NativeContentBox,
                    Talent.Description,
                    14,
                    FMargin (64.0f, 0.0f, 40.0f, 8.0f));
            }
        }
    }
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

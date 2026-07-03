#include "UI/GrimrockMenuWidget.h"

#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Texture2D.h"
#include "UI/GridInventoryWidget.h"

void UGrimrockMenuWidget::NativeConstruct ()
{
    Super::NativeConstruct ();

    BindTopTabButtons ();
    if (!bTopTabsInitialized)
    {
        bTopTabsInitialized = true;
        SetActiveTopTab (EInventoryTopTab::Inventory);
        return;
    }

    SetActiveTopTab (CurrentTopTab);
}

void UGrimrockMenuWidget::InitializeMenuWidget (AGrimrockPartyPawn* InPartyPawn)
{
    OwningPartyPawn = InPartyPawn;
    if (Page_Inventory)
    {
        Page_Inventory->InitializeInventoryWidget (InPartyPawn);
    }
}

void UGrimrockMenuWidget::RefreshInventory ()
{
    if (Page_Inventory)
    {
        Page_Inventory->RefreshInventory ();
    }
}

UGridInventoryWidget* UGrimrockMenuWidget::GetInventoryWidget () const
{
    return Page_Inventory;
}

UWidget* UGrimrockMenuWidget::GetTopTabPage (EInventoryTopTab Tab) const
{
    switch (Tab)
    {
    case EInventoryTopTab::Inventory:
        return Page_Inventory;
    case EInventoryTopTab::Skills:
        return Page_Skills;
    case EInventoryTopTab::Journal:
        return Page_Journal;
    case EInventoryTopTab::Map:
        return Page_Map;
    case EInventoryTopTab::Recipes:
        return Page_Recipes;
    case EInventoryTopTab::Codex:
        return Page_Codex;
    default:
        return nullptr;
    }
}

void UGrimrockMenuWidget::SetActiveTopTab (EInventoryTopTab NewTab)
{
    UWidget* TargetPage = GetTopTabPage (NewTab);
    if (!WidgetSwitcher_MainContent || !TargetPage)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GrimrockMenu cannot activate TopTab=%d"), static_cast<int32> (NewTab));
        return;
    }

    CurrentTopTab = NewTab;
    WidgetSwitcher_MainContent->SetActiveWidget (TargetPage);
    UpdateTopTabButtonStyles ();

    UE_LOG (LogTemp, VeryVerbose, TEXT ("GrimrockMenu active TopTab=%d Page=%s"),
        static_cast<int32> (NewTab),
        *GetNameSafe (TargetPage));
}

void UGrimrockMenuWidget::BindTopTabButtons ()
{
    const TArray<UButton*> Buttons = {
        Button_TabInventory,
        Button_TabSkills,
        Button_TabJournal,
        Button_TabMap,
        Button_TabRecipes,
        Button_TabCodex
    };

    for (UButton* Button : Buttons)
    {
        if (Button && !DefaultTopTabButtonStyles.Contains (Button))
        {
            DefaultTopTabButtonStyles.Add (Button, Button->GetStyle ());
        }
    }

    if (!SelectedTopTabTexture)
    {
        SelectedTopTabTexture = LoadObject<UTexture2D> (
            nullptr,
            TEXT ("/Game/GrimrockPrototype/Blueprints/UI/TopTabs/TopTabs/"
                  "T_ButtonTab_Selected_480x100.T_ButtonTab_Selected_480x100"));
    }

    if (Button_TabInventory)
    {
        Button_TabInventory->OnClicked.RemoveDynamic (this, &UGrimrockMenuWidget::HandleInventoryTopTabClicked);
        Button_TabInventory->OnClicked.AddDynamic (this, &UGrimrockMenuWidget::HandleInventoryTopTabClicked);
    }
    if (Button_TabSkills)
    {
        Button_TabSkills->OnClicked.RemoveDynamic (this, &UGrimrockMenuWidget::HandleSkillsTopTabClicked);
        Button_TabSkills->OnClicked.AddDynamic (this, &UGrimrockMenuWidget::HandleSkillsTopTabClicked);
    }
    if (Button_TabJournal)
    {
        Button_TabJournal->OnClicked.RemoveDynamic (this, &UGrimrockMenuWidget::HandleJournalTopTabClicked);
        Button_TabJournal->OnClicked.AddDynamic (this, &UGrimrockMenuWidget::HandleJournalTopTabClicked);
    }
    if (Button_TabMap)
    {
        Button_TabMap->OnClicked.RemoveDynamic (this, &UGrimrockMenuWidget::HandleMapTopTabClicked);
        Button_TabMap->OnClicked.AddDynamic (this, &UGrimrockMenuWidget::HandleMapTopTabClicked);
    }
    if (Button_TabRecipes)
    {
        Button_TabRecipes->OnClicked.RemoveDynamic (this, &UGrimrockMenuWidget::HandleRecipesTopTabClicked);
        Button_TabRecipes->OnClicked.AddDynamic (this, &UGrimrockMenuWidget::HandleRecipesTopTabClicked);
    }
    if (Button_TabCodex)
    {
        Button_TabCodex->OnClicked.RemoveDynamic (this, &UGrimrockMenuWidget::HandleCodexTopTabClicked);
        Button_TabCodex->OnClicked.AddDynamic (this, &UGrimrockMenuWidget::HandleCodexTopTabClicked);
    }
}

void UGrimrockMenuWidget::UpdateTopTabButtonStyles ()
{
    ApplyTopTabButtonStyle (Button_TabInventory, EInventoryTopTab::Inventory);
    ApplyTopTabButtonStyle (Button_TabSkills, EInventoryTopTab::Skills);
    ApplyTopTabButtonStyle (Button_TabJournal, EInventoryTopTab::Journal);
    ApplyTopTabButtonStyle (Button_TabMap, EInventoryTopTab::Map);
    ApplyTopTabButtonStyle (Button_TabRecipes, EInventoryTopTab::Recipes);
    ApplyTopTabButtonStyle (Button_TabCodex, EInventoryTopTab::Codex);
}

void UGrimrockMenuWidget::ApplyTopTabButtonStyle (UButton* Button, EInventoryTopTab Tab)
{
    if (!Button)
    {
        return;
    }

    const FButtonStyle* DefaultStyle = DefaultTopTabButtonStyles.Find (Button);
    if (!DefaultStyle)
    {
        return;
    }

    FButtonStyle Style = *DefaultStyle;
    if (Tab == CurrentTopTab && SelectedTopTabTexture)
    {
        Style.Normal.SetResourceObject (SelectedTopTabTexture);
        Style.Hovered.SetResourceObject (SelectedTopTabTexture);
        Style.Pressed.SetResourceObject (SelectedTopTabTexture);
    }
    Button->SetStyle (Style);
}

void UGrimrockMenuWidget::HandleInventoryTopTabClicked ()
{
    SetActiveTopTab (EInventoryTopTab::Inventory);
}

void UGrimrockMenuWidget::HandleSkillsTopTabClicked ()
{
    SetActiveTopTab (EInventoryTopTab::Skills);
}

void UGrimrockMenuWidget::HandleJournalTopTabClicked ()
{
    SetActiveTopTab (EInventoryTopTab::Journal);
}

void UGrimrockMenuWidget::HandleMapTopTabClicked ()
{
    SetActiveTopTab (EInventoryTopTab::Map);
}

void UGrimrockMenuWidget::HandleRecipesTopTabClicked ()
{
    SetActiveTopTab (EInventoryTopTab::Recipes);
}

void UGrimrockMenuWidget::HandleCodexTopTabClicked ()
{
    SetActiveTopTab (EInventoryTopTab::Codex);
}

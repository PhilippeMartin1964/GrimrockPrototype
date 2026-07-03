#include "UI/GrimrockMenuWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ContentWidget.h"
#include "Components/SafeZone.h"
#include "Components/SafeZoneSlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Texture2D.h"
#include "UI/GridInventoryWidget.h"
#include "Widgets/Layout/SScaleBox.h"

void UGrimrockMenuWidget::NativeConstruct ()
{
    ApplyDesignResolutionLayout ();

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

void UGrimrockMenuWidget::ApplyDesignResolutionLayout ()
{
    if (!WidgetTree || !WidgetTree->RootWidget)
    {
        return;
    }

    const float ViewportScale = FMath::Max (0.01f, UWidgetLayoutLibrary::GetViewportScale (this));
    const float MaxDesignWidthSlate = 1920.0f / ViewportScale;
    const float MaxDesignHeightSlate = 1080.0f / ViewportScale;

    UE_LOG (LogTemp, Log,
        TEXT ("GrimrockMenu DesignResolution ViewportScale=%.2f SlateSize=%.1fx%.1f PhysicalMax=1920x1080"),
        ViewportScale,
        MaxDesignWidthSlate,
        MaxDesignHeightSlate);

    auto ConfigureDesignResolutionWidgets = [MaxDesignWidthSlate, MaxDesignHeightSlate] (
        UScaleBox* ScaleBoxMenu,
        USizeBox* SizeBoxMenuDesignResolution)
    {
        if (ScaleBoxMenu)
        {
            ScaleBoxMenu->SetStretch (EStretch::ScaleToFit);
            ScaleBoxMenu->SetStretchDirection (EStretchDirection::DownOnly);
        }

        if (SizeBoxMenuDesignResolution)
        {
            SizeBoxMenuDesignResolution->SetWidthOverride (MaxDesignWidthSlate);
            SizeBoxMenuDesignResolution->SetHeightOverride (MaxDesignHeightSlate);

            if (UScaleBoxSlot* SizeBoxSlot = Cast<UScaleBoxSlot> (SizeBoxMenuDesignResolution->Slot))
            {
                SizeBoxSlot->SetHorizontalAlignment (HAlign_Center);
                SizeBoxSlot->SetVerticalAlignment (VAlign_Center);
            }
        }
    };

    auto ConfigureSafeZonePlacement = [] (USafeZone* SafeZoneMenu)
    {
        if (!SafeZoneMenu)
        {
            return;
        }

        if (UCanvasPanelSlot* SafeZoneCanvasSlot = Cast<UCanvasPanelSlot> (SafeZoneMenu->Slot))
        {
            SafeZoneCanvasSlot->SetAnchors (FAnchors (0.0f, 0.0f, 1.0f, 1.0f));
            SafeZoneCanvasSlot->SetOffsets (FMargin (48.0f));
            SafeZoneCanvasSlot->SetAlignment (FVector2D::ZeroVector);
        }

        if (UWidget* SafeZoneChild = SafeZoneMenu->GetContent ())
        {
            if (USafeZoneSlot* SafeZoneSlot = Cast<USafeZoneSlot> (SafeZoneChild->Slot))
            {
                SafeZoneSlot->SetHorizontalAlignment (HAlign_Fill);
                SafeZoneSlot->SetVerticalAlignment (VAlign_Fill);
            }
        }
    };

    UScaleBox* ExistingScaleBoxMenu = WidgetTree->FindWidget<UScaleBox> (TEXT ("ScaleBox_Menu"));
    if (!ExistingScaleBoxMenu)
    {
        ExistingScaleBoxMenu = WidgetTree->FindWidget<UScaleBox> (TEXT ("ScaleBox_MenuRoot"));
    }

    USizeBox* ExistingSizeBoxMenuDesignResolution =
        WidgetTree->FindWidget<USizeBox> (TEXT ("SizeBox_MenuDesignResolution"));
    if (!ExistingSizeBoxMenuDesignResolution)
    {
        ExistingSizeBoxMenuDesignResolution = WidgetTree->FindWidget<USizeBox> (TEXT ("SizeBox_MenuDesign"));
    }

    if (ExistingScaleBoxMenu && ExistingSizeBoxMenuDesignResolution)
    {
        ConfigureSafeZonePlacement (WidgetTree->FindWidget<USafeZone> (TEXT ("SafeZone_Menu")));
        ConfigureDesignResolutionWidgets (ExistingScaleBoxMenu, ExistingSizeBoxMenuDesignResolution);
        return;
    }

    UWidget* ExistingMenuContent = WidgetTree->RootWidget;
    ExistingMenuContent->RemoveFromParent ();

    UCanvasPanel* CanvasPanelRoot = WidgetTree->ConstructWidget<UCanvasPanel> (
        UCanvasPanel::StaticClass (),
        TEXT ("CanvasPanel_Root"));
    USafeZone* SafeZoneMenu = WidgetTree->ConstructWidget<USafeZone> (
        USafeZone::StaticClass (),
        TEXT ("SafeZone_Menu"));
    UScaleBox* ScaleBoxMenu = WidgetTree->ConstructWidget<UScaleBox> (
        UScaleBox::StaticClass (),
        TEXT ("ScaleBox_Menu"));
    USizeBox* SizeBoxMenuDesignResolution = WidgetTree->ConstructWidget<USizeBox> (
        USizeBox::StaticClass (),
        TEXT ("SizeBox_MenuDesignResolution"));

    if (!CanvasPanelRoot || !SafeZoneMenu || !ScaleBoxMenu || !SizeBoxMenuDesignResolution)
    {
        WidgetTree->RootWidget = ExistingMenuContent;
        return;
    }

    WidgetTree->RootWidget = CanvasPanelRoot;

    SafeZoneMenu->AddChild (ScaleBoxMenu);
    ScaleBoxMenu->AddChild (SizeBoxMenuDesignResolution);
    SizeBoxMenuDesignResolution->AddChild (ExistingMenuContent);

    CanvasPanelRoot->AddChildToCanvas (SafeZoneMenu);
    ConfigureSafeZonePlacement (SafeZoneMenu);
    ConfigureDesignResolutionWidgets (ScaleBoxMenu, SizeBoxMenuDesignResolution);
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

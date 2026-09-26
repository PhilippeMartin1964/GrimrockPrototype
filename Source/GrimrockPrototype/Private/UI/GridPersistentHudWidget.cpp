#include "UI/GridPersistentHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/WrapBoxSlot.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GrimrockPlayerController.h"
#include "UI/GridCombatHudWidget.h"
#include "UI/GrimrockMenuWidget.h"

namespace
{
	bool IsWidgetPresentationVisible(const UWidget* Widget)
	{
		return IsValid(Widget) && Widget->GetVisibility() != ESlateVisibility::Collapsed && Widget->GetVisibility() != ESlateVisibility::Hidden;
	}

	void SetSelectionFrame(UImage* Frame, bool bSelected)
	{
		if (Frame)
		{
			Frame->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

int32 UGridPersistentHudWidget::CalculateVisibleActionSlotCount(float ViewportWidth, float NavigationWidth, float ActionSlotWidth)
{
	if (ActionSlotWidth <= KINDA_SMALL_NUMBER)
	{
		return FGridCombatHotbarBinding::MinimumSlotCount;
	}

	const float AvailableWidth = FMath::Max(0.0f, ViewportWidth - FMath::Max(0.0f, NavigationWidth));
	return FMath::Max(FGridCombatHotbarBinding::MinimumSlotCount, FMath::FloorToInt(AvailableWidth / ActionSlotWidth));
}

void UGridPersistentHudWidget::InitializePersistentHud(AGrimrockPartyPawn* InPartyPawn)
{
	PartyPawn = InPartyPawn;
	InventoryComponent = IsValid(PartyPawn) ? PartyPawn->PartyInventoryComponent : nullptr;
	EnsureActionWidgets();
	RefreshFromSources();
}

void UGridPersistentHudWidget::NativeConstruct()
{
	Super::NativeConstruct();
	VisibleActionSlotCount = FGridCombatHotbarBinding::MinimumSlotCount;
	BindNavigationButtons();
	EnsureActionWidgets();
	RefreshFromSources();
}

void UGridPersistentHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	SynchronizeActionBarToViewport();
}

void UGridPersistentHudWidget::NativeDestruct()
{
	UnbindNavigationButtons();
	ActionWidgets.Reset();
	ActionBarRow = nullptr;
	LastSynchronizedViewportWidth = -1.0f;
	LastSynchronizedNavigationWidth = -1.0f;
	LastSynchronizedActionSlotWidth = -1.0f;
	LastSynchronizedCharacterIndex = INDEX_NONE;
	LastSynchronizedUsedSlotCount = INDEX_NONE;
	InventoryComponent = nullptr;
	PartyPawn = nullptr;
	Super::NativeDestruct();
}

void UGridPersistentHudWidget::RefreshFromSources()
{
	RefreshNavigationSelection();
	EnsureActionWidgets();
	RefreshActionWidgets();

	if (Panel_GlobalNavigation)
	{
		Panel_GlobalNavigation->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (Panel_ActionBar)
	{
		Panel_ActionBar->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UGridPersistentHudWidget::BindNavigationButtons()
{
	if (Button_NavEscape)
	{
		Button_NavEscape->OnClicked.AddUniqueDynamic(this, &UGridPersistentHudWidget::HandleNavEscapeClicked);
	}
	if (Button_NavInventory)
	{
		Button_NavInventory->OnClicked.AddUniqueDynamic(this, &UGridPersistentHudWidget::HandleNavInventoryClicked);
	}
	if (Button_NavSkills)
	{
		Button_NavSkills->OnClicked.AddUniqueDynamic(this, &UGridPersistentHudWidget::HandleNavSkillsClicked);
	}
	if (Button_NavCrafting)
	{
		Button_NavCrafting->OnClicked.AddUniqueDynamic(this, &UGridPersistentHudWidget::HandleNavCraftingClicked);
	}
	if (Button_NavMap)
	{
		Button_NavMap->OnClicked.AddUniqueDynamic(this, &UGridPersistentHudWidget::HandleNavMapClicked);
	}
	if (Button_NavJournal)
	{
		Button_NavJournal->OnClicked.AddUniqueDynamic(this, &UGridPersistentHudWidget::HandleNavJournalClicked);
	}
	if (Button_NavHelp)
	{
		Button_NavHelp->OnClicked.AddUniqueDynamic(this, &UGridPersistentHudWidget::HandleNavHelpClicked);
	}
}

void UGridPersistentHudWidget::UnbindNavigationButtons()
{
	if (Button_NavEscape)
	{
		Button_NavEscape->OnClicked.RemoveDynamic(this, &UGridPersistentHudWidget::HandleNavEscapeClicked);
	}
	if (Button_NavInventory)
	{
		Button_NavInventory->OnClicked.RemoveDynamic(this, &UGridPersistentHudWidget::HandleNavInventoryClicked);
	}
	if (Button_NavSkills)
	{
		Button_NavSkills->OnClicked.RemoveDynamic(this, &UGridPersistentHudWidget::HandleNavSkillsClicked);
	}
	if (Button_NavCrafting)
	{
		Button_NavCrafting->OnClicked.RemoveDynamic(this, &UGridPersistentHudWidget::HandleNavCraftingClicked);
	}
	if (Button_NavMap)
	{
		Button_NavMap->OnClicked.RemoveDynamic(this, &UGridPersistentHudWidget::HandleNavMapClicked);
	}
	if (Button_NavJournal)
	{
		Button_NavJournal->OnClicked.RemoveDynamic(this, &UGridPersistentHudWidget::HandleNavJournalClicked);
	}
	if (Button_NavHelp)
	{
		Button_NavHelp->OnClicked.RemoveDynamic(this, &UGridPersistentHudWidget::HandleNavHelpClicked);
	}
}

void UGridPersistentHudWidget::RefreshNavigationSelection()
{
	bool bInventory = false;
	bool bSkills = false;
	bool bCrafting = false;
	bool bMap = false;
	bool bJournal = false;
	bool bHelp = false;

	if (IsValid(PartyPawn))
	{
		bInventory = PartyPawn->IsInventoryWorkspaceVisible();
		const bool bMenuVisible = PartyPawn->bInventoryWidgetVisible && IsWidgetPresentationVisible(PartyPawn->MenuWidgetInstance);
		if (bMenuVisible)
		{
			switch (PartyPawn->MenuWidgetInstance->CurrentTopTab)
			{
				case EInventoryTopTab::Skills:
					bSkills = true;
					break;
				case EInventoryTopTab::Recipes:
					bCrafting = true;
					break;
				case EInventoryTopTab::Map:
					bMap = true;
					break;
				case EInventoryTopTab::Journal:
					bJournal = true;
					break;
				case EInventoryTopTab::Codex:
					bHelp = true;
					break;
				default:
					break;
			}
		}
	}

	// ESC opens a Blueprint-owned pause/main menu whose visibility is not yet a native authority.
	// Do not fabricate a selected state until that menu exposes one.
	SetSelectionFrame(Image_NavEscapeSelectionFrame, false);
	SetSelectionFrame(Image_NavInventorySelectionFrame, bInventory);
	SetSelectionFrame(Image_NavSkillsSelectionFrame, bSkills);
	SetSelectionFrame(Image_NavCraftingSelectionFrame, bCrafting);
	SetSelectionFrame(Image_NavMapSelectionFrame, bMap);
	SetSelectionFrame(Image_NavJournalSelectionFrame, bJournal);
	SetSelectionFrame(Image_NavHelpSelectionFrame, bHelp);
}

int32 UGridPersistentHudWidget::ResolveActionBarCharacterIndex() const
{
	if (!IsValid(InventoryComponent))
	{
		return INDEX_NONE;
	}
	if (IsValid(PartyPawn) && IsValid(PartyPawn->CombatHudWidgetInstance) &&
		PartyPawn->CombatHudWidgetInstance->View.ActiveCharacterIndex != INDEX_NONE)
	{
		return PartyPawn->CombatHudWidgetInstance->View.ActiveCharacterIndex;
	}
	return InventoryComponent->GetSelectedCharacterIndex();
}

float UGridPersistentHudWidget::ResolveActionSlotWidth() const
{
	if (!ActionWidgets.IsEmpty() && IsValid(ActionWidgets[0]))
	{
		const float DesiredWidth = ActionWidgets[0]->GetDesiredSize().X;
		if (DesiredWidth > KINDA_SMALL_NUMBER)
		{
			return DesiredWidth;
		}
	}
	return FMath::Max(1.0f, FallbackActionSlotWidth);
}

void UGridPersistentHudWidget::SynchronizeActionBarToViewport()
{
	if (!IsValid(InventoryComponent) || !Panel_ActionBar)
	{
		return;
	}

	const FVector2D ViewportPixels = UWidgetLayoutLibrary::GetViewportSize(this);
	const float ViewportScale = FMath::Max(KINDA_SMALL_NUMBER, UWidgetLayoutLibrary::GetViewportScale(this));
	const float ViewportWidth = ViewportPixels.X / ViewportScale;
	if (ViewportWidth <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float NavigationWidth = Panel_GlobalNavigation ? Panel_GlobalNavigation->GetDesiredSize().X : 0.0f;
	const float ActionSlotWidth = ResolveActionSlotWidth();
	const int32 CharacterIndex = ResolveActionBarCharacterIndex();
	if (!InventoryComponent->IsValidCharacterIndex(CharacterIndex))
	{
		return;
	}
	const int32 UsedSlotCount = InventoryComponent->GetCharacterCombatHotbarUsedSlotCount(CharacterIndex);

	if (FMath::IsNearlyEqual(ViewportWidth, LastSynchronizedViewportWidth, 0.5f) &&
		FMath::IsNearlyEqual(NavigationWidth, LastSynchronizedNavigationWidth, 0.5f) &&
		FMath::IsNearlyEqual(ActionSlotWidth, LastSynchronizedActionSlotWidth, 0.5f) &&
		CharacterIndex == LastSynchronizedCharacterIndex && UsedSlotCount == LastSynchronizedUsedSlotCount)
	{
		return;
	}

	LastSynchronizedViewportWidth = ViewportWidth;
	LastSynchronizedNavigationWidth = NavigationWidth;
	LastSynchronizedActionSlotWidth = ActionSlotWidth;
	LastSynchronizedCharacterIndex = CharacterIndex;
	LastSynchronizedUsedSlotCount = UsedSlotCount;

	const int32 FittedSlotCount = CalculateVisibleActionSlotCount(ViewportWidth, NavigationWidth, ActionSlotWidth);
	const int32 TargetVisibleSlotCount = FMath::Max(FittedSlotCount, UsedSlotCount);
	const bool bVisibleCountChanged = VisibleActionSlotCount != TargetVisibleSlotCount;
	VisibleActionSlotCount = TargetVisibleSlotCount;

	if (InventoryComponent->GetCharacterCombatHotbarSlotCount(CharacterIndex) < VisibleActionSlotCount)
	{
		InventoryComponent->EnsureCharacterCombatHotbarCapacity(CharacterIndex, VisibleActionSlotCount);
	}

	if (bVisibleCountChanged)
	{
		EnsureActionWidgets();
		RefreshActionWidgets();
	}
}

void UGridPersistentHudWidget::EnsureActionWidgets()
{
	if (!Panel_ActionBar || !IsValid(PartyPawn))
	{
		return;
	}

	if (!ActionWidgetClass)
	{
		return;
	}

	if (UHorizontalBox* DesignerRow = Cast<UHorizontalBox>(Panel_ActionBar))
	{
		ActionBarRow = DesignerRow;
	}
	else if (!IsValid(ActionBarRow) || ActionBarRow->GetParent() != Panel_ActionBar)
	{
		Panel_ActionBar->ClearChildren();
		ActionBarRow = WidgetTree
			? WidgetTree->ConstructWidget<UHorizontalBox>(
				  UHorizontalBox::StaticClass(), MakeUniqueObjectName(WidgetTree, UHorizontalBox::StaticClass(), TEXT("HorizontalBox_ActionBar_Runtime")))
			: NewObject<UHorizontalBox>(this, MakeUniqueObjectName(this, UHorizontalBox::StaticClass(), TEXT("HorizontalBox_ActionBar_Runtime")));
		if (!ActionBarRow)
		{
			return;
		}
		Panel_ActionBar->AddChild(ActionBarRow);
	}

	const int32 SlotCount = FMath::Max(FGridCombatHotbarBinding::MinimumSlotCount, VisibleActionSlotCount);
	bool bPoolValid = IsValid(ActionBarRow) && ActionWidgets.Num() == SlotCount && ActionBarRow->GetChildrenCount() == SlotCount;
	for (const UGridCombatHudActionWidget* ActionWidget : ActionWidgets)
	{
		bPoolValid = bPoolValid && IsValid(ActionWidget) && ActionWidget->GetParent() == ActionBarRow;
	}
	if (bPoolValid)
	{
		return;
	}

	ActionBarRow->ClearChildren();
	ActionWidgets.Reset(SlotCount);
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		UGridCombatHudActionWidget* ActionWidget = CreateWidget<UGridCombatHudActionWidget>(this, ActionWidgetClass);
		if (!ActionWidget)
		{
			continue;
		}

		UHorizontalBoxSlot* ActionSlot = ActionBarRow->AddChildToHorizontalBox(ActionWidget);
		if (ActionSlot)
		{
			ActionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			ActionSlot->SetPadding(FMargin(0.0f));
			ActionSlot->SetHorizontalAlignment(HAlign_Left);
			ActionSlot->SetVerticalAlignment(VAlign_Fill);
		}
		ActionWidgets.Add(ActionWidget);
	}
}

void UGridPersistentHudWidget::RefreshActionWidgets()
{
	if (!IsValid(PartyPawn) || !IsValid(PartyPawn->CombatHudWidgetInstance))
	{
		return;
	}

	UGridCombatHudWidget* CombatHud = PartyPawn->CombatHudWidgetInstance;
	for (int32 SlotIndex = 0; SlotIndex < ActionWidgets.Num(); ++SlotIndex)
	{
		UGridCombatHudActionWidget* ActionWidget = ActionWidgets[SlotIndex];
		if (!IsValid(ActionWidget) || !CombatHud->View.Actions.IsValidIndex(SlotIndex))
		{
			continue;
		}
		ActionWidget->InitializeAction(CombatHud, CombatHud->View.Actions[SlotIndex]);
		ActionWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void UGridPersistentHudWidget::HandleNavEscapeClicked()
{
	if (!IsValid(PartyPawn))
	{
		return;
	}
	if (AGrimrockPlayerController* Controller = Cast<AGrimrockPlayerController>(PartyPawn->GetController()))
	{
		Controller->RequestGlobalEscape();
	}
	else
	{
		PartyPawn->HandleGlobalEscape();
	}
	RefreshFromSources();
}

void UGridPersistentHudWidget::HandleNavInventoryClicked()
{
	if (IsValid(PartyPawn))
	{
		PartyPawn->ToggleInventoryWidget();
		RefreshFromSources();
	}
}

void UGridPersistentHudWidget::HandleNavSkillsClicked()
{
	if (IsValid(PartyPawn))
	{
		PartyPawn->ToggleSkillsWidget();
		RefreshFromSources();
	}
}

void UGridPersistentHudWidget::HandleNavCraftingClicked()
{
	if (IsValid(PartyPawn))
	{
		PartyPawn->ToggleCraftingWidget();
		RefreshFromSources();
	}
}

void UGridPersistentHudWidget::HandleNavMapClicked()
{
	if (IsValid(PartyPawn))
	{
		PartyPawn->ToggleMapWidget();
		RefreshFromSources();
	}
}

void UGridPersistentHudWidget::HandleNavJournalClicked()
{
	if (IsValid(PartyPawn))
	{
		PartyPawn->ToggleJournalWidget();
		RefreshFromSources();
	}
}

void UGridPersistentHudWidget::HandleNavHelpClicked()
{
	if (IsValid(PartyPawn))
	{
		PartyPawn->ToggleHelpWidget();
		RefreshFromSources();
	}
}

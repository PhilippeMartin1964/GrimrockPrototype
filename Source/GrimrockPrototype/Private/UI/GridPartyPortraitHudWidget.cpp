#include "UI/GridPartyPortraitHudWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "InputCoreTypes.h"
#include "UI/GridPartyPortraitDragDropOperation.h"
#include "UI/GridNavigationTooltipWidget.h"
#include "UI/GridPersistentHudWidget.h"

#define LOCTEXT_NAMESPACE "GridPartyPortraitHud"

void UGridPartyPortraitHudWidget::SetPortraitView(const FGridPartyPortraitView& InView)
{
	View = InView;

	// The project owns its visible pointer through WBP_GridMouseCursor.
	// Once the portrait became hit-testable for click/drag, Slate could expose
	// the platform cursor over this widget as well. Suppress only the portrait's
	// UMG cursor so Cursor_Default remains the single visible pointer.
	SetCursor(EMouseCursor::None);

	RefreshBoundVisuals();
	RefreshTooltip();
	RefreshPortraitVisual();
	SetVisibility(ESlateVisibility::Visible);
}

FGuid UGridPartyPortraitHudWidget::GetCharacterId() const
{
	return View.CharacterId;
}

bool UGridPartyPortraitHudWidget::IsSelected() const
{
	return View.bIsSelected;
}

FText UGridPartyPortraitHudWidget::GetTooltipText() const
{
	if (!View.CharacterId.IsValid() || View.DisplayName.IsEmpty())
	{
		return FText::GetEmpty();
	}

	const FText ResolvedClassName = View.ClassDisplayName.IsEmpty()
		? LOCTEXT("TooltipClassUnavailable", "Classe non configurée")
		: View.ClassDisplayName;

	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Name"), View.DisplayName);
	Arguments.Add(TEXT("Class"), ResolvedClassName);
	Arguments.Add(TEXT("Level"), FText::AsNumber(FMath::Max(1, View.Level)));
	Arguments.Add(TEXT("HealthCurrent"), FText::AsNumber(FMath::Max(0, View.CurrentHealth)));
	Arguments.Add(TEXT("HealthMaximum"), FText::AsNumber(FMath::Max(1, View.MaximumHealth)));
	Arguments.Add(TEXT("ManaCurrent"), FText::AsNumber(FMath::Max(0, View.CurrentMana)));
	Arguments.Add(TEXT("ManaMaximum"), FText::AsNumber(FMath::Max(0, View.MaximumMana)));
	Arguments.Add(TEXT("PhysicalCurrent"), FText::AsNumber(FMath::Max(0, View.CurrentPhysicalArmor)));
	Arguments.Add(TEXT("PhysicalReference"), FText::AsNumber(FMath::Max(0, View.ReferencePhysicalArmor)));
	Arguments.Add(TEXT("MagicalCurrent"), FText::AsNumber(FMath::Max(0, View.CurrentMagicalArmor)));
	Arguments.Add(TEXT("MagicalReference"), FText::AsNumber(FMath::Max(0, View.ReferenceMagicalArmor)));

	return FText::Format(
		View.bHasMana
			? LOCTEXT(
				  "TooltipWithMana",
				  "{Name}\n{Class} - Niveau {Level}\nPV {HealthCurrent} / {HealthMaximum}\nMana {ManaCurrent} / {ManaMaximum}\nArmure physique {PhysicalCurrent} / {PhysicalReference}\nArmure magique {MagicalCurrent} / {MagicalReference}")
			: LOCTEXT(
				  "TooltipWithoutMana",
				  "{Name}\n{Class} - Niveau {Level}\nPV {HealthCurrent} / {HealthMaximum}\nArmure physique {PhysicalCurrent} / {PhysicalReference}\nArmure magique {MagicalCurrent} / {MagicalReference}"),
		Arguments);
}

void UGridPartyPortraitHudWidget::SetTooltipWidgetClass(
	TSubclassOf<UGridNavigationTooltipWidget> InTooltipWidgetClass)
{
	if (TooltipWidgetClass == InTooltipWidgetClass)
	{
		return;
	}

	TooltipWidgetClass = InTooltipWidgetClass;
	TooltipWidget = nullptr;
	RefreshTooltip();
}

bool UGridPartyPortraitHudWidget::RequestSelection()
{
	if (!View.CharacterId.IsValid())
	{
		return false;
	}

	OnPortraitClicked.Broadcast(View.CharacterId);
	return true;
}

void UGridPartyPortraitHudWidget::SetOwnerPersistentHudWidget(UGridPersistentHudWidget* InOwnerPersistentHudWidget)
{
	OwningPersistentHudWidget = InOwnerPersistentHudWidget;
}

bool UGridPartyPortraitHudWidget::ResolveInsertAfter(float LocalY, float LocalHeight)
{
	return LocalHeight > KINDA_SMALL_NUMBER && LocalY >= LocalHeight * 0.5f;
}

UGridPartyPortraitDragDropOperation* UGridPartyPortraitHudWidget::CreatePortraitDragDropOperation()
{
	if (!View.CharacterId.IsValid())
	{
		return nullptr;
	}

	UGridPartyPortraitDragDropOperation* Operation =
		NewObject<UGridPartyPortraitDragDropOperation>(this);
	if (!Operation)
	{
		return nullptr;
	}

	Operation->InitializeFromPortrait(View.CharacterId);
	Operation->DefaultDragVisual = nullptr;
	Operation->Pivot = EDragPivot::MouseDown;
	return Operation;
}

FReply UGridPartyPortraitHudWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && View.CharacterId.IsValid())
	{
		bLeftMousePressed = true;
		bDragDetected = false;
		return UWidgetBlueprintLibrary::DetectDragIfPressed(
			InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UGridPartyPortraitHudWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bLeftMousePressed)
	{
		const bool bShouldSelect = !bDragDetected;
		bLeftMousePressed = false;
		bDragDetected = false;
		if (bShouldSelect)
		{
			RequestSelection();
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UGridPartyPortraitHudWidget::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
	bDragDetected = true;
	bLeftMousePressed = false;
	OutOperation = CreatePortraitDragDropOperation();
}

bool UGridPartyPortraitHudWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UGridPartyPortraitDragDropOperation* Operation =
		Cast<UGridPartyPortraitDragDropOperation>(InOperation);
	if (!Operation || !Operation->HasValidSource() || !View.CharacterId.IsValid() ||
		Operation->SourceCharacterId == View.CharacterId || !IsValid(OwningPersistentHudWidget))
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	const FVector2D LocalPointerPosition =
		InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
	const bool bInsertAfter = ResolveInsertAfter(
		LocalPointerPosition.Y, InGeometry.GetLocalSize().Y);

	return OwningPersistentHudWidget->HandlePartyPortraitDrop(
		Operation, View.CharacterId, bInsertAfter);
}

void UGridPartyPortraitHudWidget::RefreshPortraitVisual_Implementation()
{
}

void UGridPartyPortraitHudWidget::RefreshTooltip()
{
	const FText TooltipText = GetTooltipText();
	if (TooltipText.IsEmpty())
	{
		SetToolTip(nullptr);
		SetToolTipText(FText::GetEmpty());
		TooltipWidget = nullptr;
		return;
	}

	if (TooltipWidgetClass)
	{
		if (!IsValid(TooltipWidget) || !TooltipWidget->IsA(TooltipWidgetClass.Get()))
		{
			TooltipWidget = CreateWidget<UGridNavigationTooltipWidget>(this, TooltipWidgetClass);
		}

		if (IsValid(TooltipWidget))
		{
			TooltipWidget->InitializeTooltip(TooltipText);
			TooltipWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			SetToolTipText(FText::GetEmpty());
			SetToolTip(TooltipWidget);
			return;
		}
	}

	TooltipWidget = nullptr;
	SetToolTip(nullptr);
	SetToolTipText(TooltipText);
}

void UGridPartyPortraitHudWidget::RefreshBoundVisuals()
{
	if (Image_Portrait)
	{
		if (View.Portrait.IsNull())
		{
			Image_Portrait->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			Image_Portrait->SetBrushFromSoftTexture(View.Portrait, false);
			Image_Portrait->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	if (ProgressBar_Health)
	{
		ProgressBar_Health->SetPercent(FMath::Clamp(View.HealthRatio, 0.0f, 1.0f));
		ProgressBar_Health->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (ProgressBar_Mana)
	{
		ProgressBar_Mana->SetPercent(View.bHasMana ? FMath::Clamp(View.ManaRatio, 0.0f, 1.0f) : 0.0f);
		ProgressBar_Mana->SetVisibility(View.bHasMana ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (ProgressBar_PhysicalArmor)
	{
		ProgressBar_PhysicalArmor->SetPercent(FMath::Clamp(View.PhysicalArmorRatio, 0.0f, 1.0f));
		ProgressBar_PhysicalArmor->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (ProgressBar_MagicalArmor)
	{
		ProgressBar_MagicalArmor->SetPercent(FMath::Clamp(View.MagicalArmorRatio, 0.0f, 1.0f));
		ProgressBar_MagicalArmor->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (Border_Selected)
	{
		Border_Selected->SetVisibility(View.bIsSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

#undef LOCTEXT_NAMESPACE

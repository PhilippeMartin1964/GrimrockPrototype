#include "UI/GridItemExaminationWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "Runtime/Inspection/GridItemInspectionReadModel.h"
#include "UI/GridItemTooltipWidget.h"
#include "UI/GridItemTooltipWeaponLineWidget.h"
#include "UI/GridWeaponTooltipProjection.h"
#include "UI/UILocalizationText.h"

namespace
{
	void SetShown(UWidget* Widget, bool bShown)
	{
		if (Widget)
		{
			Widget->SetVisibility(bShown ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}

	void SetValue(UTextBlock* Widget, const FText& Value)
	{
		if (Widget)
		{
			Widget->SetText(Value);
			SetShown(Widget, !Value.IsEmpty());
		}
	}

	void AddFact(TArray<FGridWeaponTooltipFact>& Facts, FName Id, const TCHAR* Key, const FText& Value)
	{
		FGridWeaponTooltipFact& Fact = Facts.AddDefaulted_GetRef();
		Fact.StatId = Id;
		Fact.Label = FUILocalizationText::Get(Key);
		Fact.ValueText = Value;
	}
}

bool FGridItemExaminationData::Build(const UGridItemSubsystem* Registry, const FGridItemInstance& Instance,
	FGridItemExaminationData& Out)
{
	Out = FGridItemExaminationData();
	if (!FGridItemInspectionReadModel::ResolvePhysical(Registry, Instance, Out.View))
	{
		return false;
	}
	if (Out.View.HasLevel())
	{
		FGridWeaponTooltipPresentation Weapon;
		if (FGridWeaponTooltipProjection::Project(Instance, Out.View.Item, Weapon))
		{
			Out.Facts = MoveTemp(Weapon.Facts);
		}
	}
	else if (Out.View.HasProtection())
	{
		if (Out.View.GetNominalPhysicalArmor() > 0)
		{
			AddFact(Out.Facts, TEXT("Armor"), TEXT("Inventory.Tooltip.Weapon.Stat.ArmorPhysical"),
				FText::AsNumber(Out.View.GetNominalPhysicalArmor()));
		}
		if (Out.View.GetNominalMagicalArmor() > 0)
		{
			AddFact(Out.Facts, TEXT("WeaponMagicalArmor"), TEXT("Inventory.Tooltip.Weapon.Stat.ArmorMagical"),
				FText::AsNumber(Out.View.GetNominalMagicalArmor()));
		}
		if (Out.View.bHasPhysicalReserve && Out.View.PhysicalReserve.NominalPhysicalArmor > 0)
		{
			AddFact(Out.Facts, TEXT("WeaponPhysicalArmorRemaining"),
				TEXT("Inventory.Tooltip.Global.Stat.CurrentPhysicalArmor"),
				FText::FromString(FString::Printf(TEXT("%d/%d"), Out.View.PhysicalReserve.RemainingPhysicalArmor,
					Out.View.PhysicalReserve.NominalPhysicalArmor)));
		}
	}
	return true;
}

bool UGridItemExaminationWidget::HasDesignerContract() const
{
	return Image_RenderTarget && Image_FallbackIcon && Text_PreviewUnavailable &&
		Text_ItemName && Text_ItemLevel && Border_LevelBadge && Text_Description &&
		Text_Weight && Text_Quantity && VerticalBox_Stats && Section_Stats &&
		Button_Close && Button_Front && Button_Profile && Button_Back && Button_Reset &&
		StatRowWidgetClass;
}

void UGridItemExaminationWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_Close) Button_Close->OnClicked.AddUniqueDynamic(this, &UGridItemExaminationWidget::RequestClose);
	if (Button_Front) Button_Front->OnClicked.AddUniqueDynamic(this, &UGridItemExaminationWidget::FrameFront);
	if (Button_Profile) Button_Profile->OnClicked.AddUniqueDynamic(this, &UGridItemExaminationWidget::FrameProfile);
	if (Button_Back) Button_Back->OnClicked.AddUniqueDynamic(this, &UGridItemExaminationWidget::FrameBack);
	if (Button_Reset) Button_Reset->OnClicked.AddUniqueDynamic(this, &UGridItemExaminationWidget::FrameReset);
	if (!WorldCleanupHandle.IsValid())
	{
		WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UGridItemExaminationWidget::HandleWorldCleanup);
	}
}

void UGridItemExaminationWidget::NativeDestruct()
{
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	WorldCleanupHandle.Reset();
	ClearExamination();
	Super::NativeDestruct();
}

bool UGridItemExaminationWidget::PresentItem(const UGridItemSubsystem* Registry, const FGridItemInstance& Instance)
{
	ClearExamination();
	FGridItemExaminationData Data;
	if (!HasDesignerContract() || !FGridItemExaminationData::Build(Registry, Instance, Data))
	{
		return false;
	}

	const FGridItemInspectionView& View = Data.View;
	SetValue(Text_ItemName, View.Item.DisplayName);
	SetValue(Text_ItemLevel, View.HasLevel() ? UGridItemTooltipWidget::FormatItemLevel(View.Item.ItemLevel) : FText::GetEmpty());
	SetShown(Border_LevelBadge, View.HasLevel());
	SetValue(Text_Description, View.Item.Description);
	SetValue(Text_Weight, FText::AsNumber(View.Item.WeightKg));
	SetValue(Text_Quantity, FText::AsNumber(View.Quantity));
	for (const FGridWeaponTooltipFact& Fact : Data.Facts)
	{
		UGridItemTooltipWeaponLineWidget* Row = CreateWidget<UGridItemTooltipWeaponLineWidget>(this, StatRowWidgetClass);
		if (!Row || !Row->PresentFact(Fact))
		{
			ClearExamination();
			return false;
		}
		VerticalBox_Stats->AddChildToVerticalBox(Row);
	}
	SetShown(Section_Stats, !Data.Facts.IsEmpty());

	Image_FallbackIcon->SetBrushFromTexture(View.Item.ItemIcon.LoadSynchronous(), false);
	PresentationWorld = GetWorld();
	bPresented = true;
	if (View.HasMeshReference() && Preview.Open(PresentationWorld.Get()))
	{
		Preview.Present(View);
	}
	RefreshCapture();
	return true;
}

void UGridItemExaminationWidget::ClearExamination()
{
	bPresented = false;
	DragButton = FKey();
	PresentationWorld.Reset();
	if (Image_RenderTarget) Image_RenderTarget->SetBrushResourceObject(nullptr);
	if (Image_FallbackIcon) Image_FallbackIcon->SetBrushResourceObject(nullptr);
	SetShown(Image_RenderTarget, false);
	SetShown(Image_FallbackIcon, false);
	SetShown(Text_PreviewUnavailable, false);
	SetShown(Border_LevelBadge, false);
	SetShown(Section_Stats, false);
	SetValue(Text_ItemName, FText::GetEmpty());
	SetValue(Text_ItemLevel, FText::GetEmpty());
	SetValue(Text_Description, FText::GetEmpty());
	SetValue(Text_Weight, FText::GetEmpty());
	SetValue(Text_Quantity, FText::GetEmpty());
	if (VerticalBox_Stats) VerticalBox_Stats->ClearChildren();
	Preview.Close();
}

void UGridItemExaminationWidget::HandleWorldCleanup(UWorld* World, bool, bool)
{
	if (World && World == PresentationWorld.Get())
	{
		RequestClose();
	}
}

void UGridItemExaminationWidget::RequestClose()
{
	ClearExamination();
	OnCloseRequested.Broadcast();
}

void UGridItemExaminationWidget::RefreshCapture()
{
	UTextureRenderTarget2D* Target = Preview.Capture() ? Preview.GetRenderTarget() : nullptr;
	Image_RenderTarget->SetBrushResourceObject(Target);
	SetShown(Image_RenderTarget, Target != nullptr);
	SetShown(Image_FallbackIcon, !Target && Image_FallbackIcon->GetBrush().GetResourceObject() != nullptr);
	SetShown(Text_PreviewUnavailable, Target == nullptr);
	Button_Front->SetIsEnabled(Target != nullptr);
	Button_Profile->SetIsEnabled(Target != nullptr);
	Button_Back->SetIsEnabled(Target != nullptr);
	Button_Reset->SetIsEnabled(Target != nullptr);
}

void UGridItemExaminationWidget::FrameFront()
{
	if (bPresented) { Preview.Frame(FRotator::ZeroRotator); RefreshCapture(); }
}

void UGridItemExaminationWidget::FrameProfile()
{
	if (bPresented) { Preview.Frame(FRotator(0.0, 90.0, 0.0)); RefreshCapture(); }
}

void UGridItemExaminationWidget::FrameBack()
{
	if (bPresented) { Preview.Frame(FRotator(0.0, 180.0, 0.0)); RefreshCapture(); }
}

void UGridItemExaminationWidget::FrameReset()
{
	if (bPresented) { Preview.Frame(); RefreshCapture(); }
}

bool UGridItemExaminationWidget::IsOverPreview(const FVector2D& ScreenPosition) const
{
	return bPresented && Preview.GetOrbit().bReady && Image_RenderTarget &&
		Preview.GetRenderTarget() && Image_RenderTarget->GetCachedGeometry().IsUnderLocation(ScreenPosition);
}

FReply UGridItemExaminationWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (bPresented && Event.GetKey() == EKeys::Escape)
	{
		RequestClose();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnKeyDown(Geometry, Event);
}

FReply UGridItemExaminationWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	const FKey Button = Event.GetEffectingButton();
	if (IsOverPreview(Event.GetScreenSpacePosition()) &&
		(Button == EKeys::LeftMouseButton || Button == EKeys::RightMouseButton || Button == EKeys::MiddleMouseButton))
	{
		DragButton = Button;
		if (const TSharedPtr<SWidget> Cached = GetCachedWidget())
		{
			return FReply::Handled().CaptureMouse(Cached.ToSharedRef());
		}
		DragButton = FKey();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(Geometry, Event);
}

FReply UGridItemExaminationWidget::NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (DragButton.IsValid() && Event.GetEffectingButton() == DragButton)
	{
		DragButton = FKey();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(Geometry, Event);
}

FReply UGridItemExaminationWidget::NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (DragButton.IsValid())
	{
		if (!Event.IsMouseButtonDown(DragButton) || !bPresented || !Image_RenderTarget)
		{
			DragButton = FKey();
			return FReply::Handled().ReleaseMouseCapture();
		}
		const FGeometry& Surface = Image_RenderTarget->GetCachedGeometry();
		const FVector2D Delta = Surface.AbsoluteToLocal(Event.GetScreenSpacePosition()) -
			Surface.AbsoluteToLocal(Event.GetLastScreenSpacePosition());
		if (DragButton == EKeys::LeftMouseButton)
		{
			Preview.Drag(Delta);
		}
		else
		{
			const FVector2D Size = Surface.GetLocalSize();
			Preview.Pan(Delta, FMath::Max(Size.X, Size.Y));
		}
		RefreshCapture();
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(Geometry, Event);
}

FReply UGridItemExaminationWidget::NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (IsOverPreview(Event.GetScreenSpacePosition()))
	{
		Preview.Zoom(Event.GetWheelDelta());
		RefreshCapture();
		return FReply::Handled();
	}
	return Super::NativeOnMouseWheel(Geometry, Event);
}

void UGridItemExaminationWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& Event)
{
	DragButton = FKey();
	Super::NativeOnMouseCaptureLost(Event);
}

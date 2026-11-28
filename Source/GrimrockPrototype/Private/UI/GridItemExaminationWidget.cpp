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

DEFINE_LOG_CATEGORY_STATIC(LogGridItemExamination, Log, All);

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

	void AddBonus(TArray<FGridWeaponTooltipFact>& Facts, FName Id, const TCHAR* Key, double Value)
	{
		if (!FMath::IsNearlyZero(Value))
		{
			const FText Number = FText::AsNumber(Value);
			const FText Signed = Value > 0.0
				? FText::FormatNamed(FUILocalizationText::Get(TEXT("Inventory.Tooltip.Global.PositiveDelta")), TEXT("Value"), Number)
				: Number;
			AddFact(Facts, Id, Key, Signed);
		}
	}

	FText ActionCost(int32 AP, int32 Mana)
	{
		return FText::FormatNamed(FUILocalizationText::Get(TEXT("Combat.Cost.ActionPointsMana")),
			TEXT("ActionPoints"), AP, TEXT("Mana"), Mana);
	}

	void AddAction(TArray<FGridWeaponTooltipFact>& Actions, FName Id, const FText& Label, const FText& Value)
	{
		FGridWeaponTooltipFact& Fact = Actions.AddDefaulted_GetRef();
		Fact.StatId = Id;
		Fact.Label = Label;
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
	if (!Out.View.HasLevel() && Out.View.HasEquipment())
	{
		const FGridEquipmentStatBonus& Bonus = Out.View.Item.NonArmEquipmentBonuses;
		const FGridDamageResistanceSet& Resist = Out.View.Item.NonArmEquipmentResistances;
		AddBonus(Out.Facts, TEXT("Strength"), TEXT("Inventory.Tooltip.Global.Stat.Strength"), Bonus.StrengthBonus);
		AddBonus(Out.Facts, TEXT("Dexterity"), TEXT("Inventory.Tooltip.Global.Stat.Dexterity"), Bonus.DexterityBonus);
		AddBonus(Out.Facts, TEXT("Constitution"), TEXT("Inventory.Tooltip.Global.Stat.Constitution"), Bonus.ConstitutionBonus);
		AddBonus(Out.Facts, TEXT("Intelligence"), TEXT("Inventory.Tooltip.Global.Stat.Intelligence"), Bonus.IntelligenceBonus);
		AddBonus(Out.Facts, TEXT("Wisdom"), TEXT("Inventory.Tooltip.Global.Stat.Wisdom"), Bonus.WisdomBonus);
		AddBonus(Out.Facts, TEXT("Charisma"), TEXT("Inventory.Tooltip.Global.Stat.Charisma"), Bonus.CharismaBonus);
		AddBonus(Out.Facts, TEXT("MaxHealth"), TEXT("Inventory.Tooltip.Global.Stat.MaxHealth"), Bonus.MaxHealthBonus);
		AddBonus(Out.Facts, TEXT("MaxMana"), TEXT("Inventory.Tooltip.Global.Stat.MaxMana"), Bonus.MaxManaBonus);
		AddBonus(Out.Facts, TEXT("CarryWeight"), TEXT("Inventory.Tooltip.Global.Stat.CarryWeight"), Bonus.CarryWeightBonus);
		AddBonus(Out.Facts, TEXT("PhysicalResistance"), TEXT("Inventory.Tooltip.Global.Stat.PhysicalResistance"), Resist.PhysicalResistance);
		AddBonus(Out.Facts, TEXT("FireResistance"), TEXT("Inventory.Tooltip.Global.Stat.FireResistance"), Resist.FireResistance);
		AddBonus(Out.Facts, TEXT("IceResistance"), TEXT("Inventory.Tooltip.Global.Stat.IceResistance"), Resist.IceResistance);
		AddBonus(Out.Facts, TEXT("LightningResistance"), TEXT("Inventory.Tooltip.Global.Stat.LightningResistance"), Resist.LightningResistance);
		AddBonus(Out.Facts, TEXT("PoisonResistance"), TEXT("Inventory.Tooltip.Global.Stat.PoisonResistance"), Resist.PoisonResistance);
		AddBonus(Out.Facts, TEXT("HolyResistance"), TEXT("Inventory.Tooltip.Global.Stat.HolyResistance"), Resist.HolyResistance);
		AddBonus(Out.Facts, TEXT("NecroticResistance"), TEXT("Inventory.Tooltip.Global.Stat.NecroticResistance"), Resist.NecroticResistance);
		AddBonus(Out.Facts, TEXT("ArcaneResistance"), TEXT("Inventory.Tooltip.Global.Stat.ArcaneResistance"), Resist.ArcaneResistance);
	}
	// The physical reserve is authoritative; do not add a second nominal bonus.
	if (Out.View.bHasPhysicalReserve && Out.View.PhysicalReserve.NominalMagicalArmor > 0)
	{
		if (FGridWeaponTooltipFact* Magic = Out.Facts.FindByPredicate([](const FGridWeaponTooltipFact& Fact)
			{ return Fact.StatId == TEXT("WeaponMagicalArmor"); }))
		{
			Magic->ValueText = FText::FromString(FString::Printf(TEXT("%d/%d"),
				Out.View.PhysicalReserve.RemainingMagicalArmor, Out.View.PhysicalReserve.NominalMagicalArmor));
		}
	}
	for (const FGridItemActionBinding& Binding : Out.View.Item.AvailableActions)
	{
		FGridCombatActionDefinition Detailed;
		if (Registry->ResolveDetailedNonArmAction(Out.View.Item, Binding.ActionId, Detailed))
		{
			AddAction(Out.Actions, Binding.ActionId,
				Detailed.DisplayName.IsEmpty() ? Out.View.Item.DisplayName : Detailed.DisplayName,
				ActionCost(Detailed.ActionPointCost, Detailed.ResourceCosts.ManaCost));
			continue;
		}
		FGridResolvedBoundAction Bound;
		if (!Registry->ResolveBoundAction(Out.View.Item, Binding.ActionId, Bound))
		{
			continue;
		}
		// Compact actions have no authored display name. Present only their known
		// category/cost; never expose technical IDs or invent an AP cost for Use/Defend.
		switch (Bound.Action.Kind)
		{
			case EGridActionKind::Attack:
				AddAction(Out.Actions, Binding.ActionId,
					FUILocalizationText::Get(TEXT("Combat.Tooltip.Section.Attack")),
					Out.View.HasAttackStats() ? ActionCost(Out.View.Item.Weapon.ActionPointCost, Bound.Action.ManaCost) : FText::GetEmpty());
				break;
			case EGridActionKind::Defend:
				AddAction(Out.Actions, Binding.ActionId,
					FUILocalizationText::Get(TEXT("Combat.Tooltip.Label.Defense")), Out.View.Item.DisplayName);
				break;
			case EGridActionKind::Use:
			case EGridActionKind::Ability:
				AddAction(Out.Actions, Binding.ActionId,
					FUILocalizationText::Get(TEXT("UI.Section.Usage")), Out.View.Item.DisplayName);
				break;
		}
	}
	return true;
}

bool UGridItemExaminationWidget::HasDesignerContract() const
{
	TArray<FString> Missing;
	const auto Require = [&Missing](bool bPresent, const TCHAR* Name)
	{
		if (!bPresent) Missing.Add(Name);
	};
	Require(Image_RenderTarget != nullptr, TEXT("Image_RenderTarget"));
	Require(Image_FallbackIcon != nullptr, TEXT("Image_FallbackIcon"));
	Require(Text_PreviewUnavailable != nullptr, TEXT("Text_PreviewUnavailable"));
	Require(Text_ItemName != nullptr, TEXT("Text_ItemName"));
	Require(Text_ItemLevel != nullptr, TEXT("Text_ItemLevel"));
	Require(Border_LevelBadge != nullptr, TEXT("Border_LevelBadge"));
	Require(Text_Description != nullptr, TEXT("Text_Description"));
	Require(Text_Weight != nullptr, TEXT("Text_Weight"));
	Require(Text_Quantity != nullptr, TEXT("Text_Quantity"));
	Require(VerticalBox_Stats != nullptr, TEXT("VerticalBox_Stats"));
	Require(Section_Stats != nullptr, TEXT("Section_Stats"));
	Require(VerticalBox_Actions != nullptr, TEXT("VerticalBox_Actions"));
	Require(Section_Actions != nullptr, TEXT("Section_Actions"));
	Require(Button_Close != nullptr, TEXT("Button_Close"));
	Require(Button_Front != nullptr, TEXT("Button_Front"));
	Require(Button_Profile != nullptr, TEXT("Button_Profile"));
	Require(Button_Back != nullptr, TEXT("Button_Back"));
	Require(Button_Reset != nullptr, TEXT("Button_Reset"));
	Require(StatRowWidgetClass.Get() != nullptr, TEXT("StatRowWidgetClass (Class Defaults)"));
	if (!Missing.IsEmpty())
	{
		UE_LOG(LogGridItemExamination, Warning, TEXT("[ItemExamination] Missing Designer bindings Class=%s: %s"),
			*GetNameSafe(GetClass()), *FString::Join(Missing, TEXT(", ")));
		return false;
	}
	return true;
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
	if (!HasDesignerContract())
	{
		return false;
	}
	if (!FGridItemExaminationData::Build(Registry, Instance, Data))
	{
		UE_LOG(LogGridItemExamination, Warning, TEXT("[ItemExamination] Physical item data resolution failed Item=%s Level=%d"),
			*Instance.ItemDefinitionId.ToString(), Instance.ItemLevel);
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
			UE_LOG(LogGridItemExamination, Warning, TEXT("[ItemExamination] Stat row failed Class=%s Fact=%s; check Text_Label and Text_Value"),
				*GetNameSafe(StatRowWidgetClass.Get()), *Fact.StatId.ToString());
			ClearExamination();
			return false;
		}
		VerticalBox_Stats->AddChildToVerticalBox(Row);
	}
	SetShown(Section_Stats, !Data.Facts.IsEmpty());
	for (const FGridWeaponTooltipFact& Fact : Data.Actions)
	{
		UGridItemTooltipWeaponLineWidget* Row = CreateWidget<UGridItemTooltipWeaponLineWidget>(this, StatRowWidgetClass);
		if (!Row || !Row->PresentFact(Fact))
		{
			UE_LOG(LogGridItemExamination, Warning, TEXT("[ItemExamination] Stat row failed Class=%s Fact=%s; check Text_Label and Text_Value"),
				*GetNameSafe(StatRowWidgetClass.Get()), *Fact.StatId.ToString());
			ClearExamination();
			return false;
		}
		VerticalBox_Actions->AddChildToVerticalBox(Row);
	}
	SetShown(Section_Actions, !Data.Actions.IsEmpty());

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
	SetShown(Section_Actions, false);
	SetValue(Text_ItemName, FText::GetEmpty());
	SetValue(Text_ItemLevel, FText::GetEmpty());
	SetValue(Text_Description, FText::GetEmpty());
	SetValue(Text_Weight, FText::GetEmpty());
	SetValue(Text_Quantity, FText::GetEmpty());
	if (VerticalBox_Stats) VerticalBox_Stats->ClearChildren();
	if (VerticalBox_Actions) VerticalBox_Actions->ClearChildren();
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

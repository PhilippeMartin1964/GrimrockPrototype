#include "UI/GridSkillEntryWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "UI/UILocalizationText.h"

void UGridSkillEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_IncreaseSkill)
	{
		Button_IncreaseSkill->OnClicked.RemoveDynamic(this, &UGridSkillEntryWidget::HandleIncreaseSkillClicked);
		Button_IncreaseSkill->OnClicked.AddUniqueDynamic(this, &UGridSkillEntryWidget::HandleIncreaseSkillClicked);
	}
	if (Button_DecreaseSkill)
	{
		Button_DecreaseSkill->OnClicked.RemoveDynamic(this, &UGridSkillEntryWidget::HandleDecreaseSkillClicked);
		Button_DecreaseSkill->OnClicked.AddUniqueDynamic(this, &UGridSkillEntryWidget::HandleDecreaseSkillClicked);
	}
	RefreshEntryVisual();
}

void UGridSkillEntryWidget::NativeDestruct()
{
	if (Button_IncreaseSkill)
	{
		Button_IncreaseSkill->OnClicked.RemoveDynamic(this, &UGridSkillEntryWidget::HandleIncreaseSkillClicked);
	}
	if (Button_DecreaseSkill)
	{
		Button_DecreaseSkill->OnClicked.RemoveDynamic(this, &UGridSkillEntryWidget::HandleDecreaseSkillClicked);
	}
	Super::NativeDestruct();
}

FReply UGridSkillEntryWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && RequestSkillSelection())
	{
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UGridSkillEntryWidget::NativeOnMouseEnter(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	bHovered = true;
	RefreshInteractionVisual();
}

void UGridSkillEntryWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bHovered = false;
	RefreshInteractionVisual();
}

void UGridSkillEntryWidget::HandleIncreaseSkillClicked()
{
	if (bInitialized && Entry.bCanIncreaseRank && !Entry.SkillId.IsNone())
	{
		OnIncreaseSkillRequested.Broadcast(Entry.SkillId);
	}
}

void UGridSkillEntryWidget::HandleDecreaseSkillClicked()
{
	if (bInitialized && Entry.bCanDecreaseRank && !Entry.SkillId.IsNone())
	{
		OnDecreaseSkillRequested.Broadcast(Entry.SkillId);
	}
}

bool UGridSkillEntryWidget::InitializeSkillEntry(const FGridSkillEntryView& InEntry)
{
	ClearSkillEntry();
	if (!IsValidEntry(InEntry))
	{
		return false;
	}

	Entry = InEntry;
	bInitialized = true;
	RefreshEntryVisual();
	return true;
}

void UGridSkillEntryWidget::SetSkillIcon(UTexture2D* InIcon)
{
	SkillIcon = InIcon;
	if (Image_SkillIcon)
	{
		Image_SkillIcon->SetBrushFromTexture(SkillIcon.Get(), false);
		Image_SkillIcon->SetVisibility(SkillIcon.Get() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UGridSkillEntryWidget::SetSelected(bool bInSelected)
{
	bSelected = bInSelected;
	RefreshInteractionVisual();
}

bool UGridSkillEntryWidget::RequestSkillSelection()
{
	if (!bInitialized || Entry.SkillId.IsNone())
	{
		return false;
	}

	OnSelectSkillRequested.Broadcast(Entry.SkillId);
	return true;
}

EGridSkillEntryVisualState UGridSkillEntryWidget::ResolveVisualState(bool bInSelected, bool bInHovered)
{
	if (bInSelected)
	{
		return EGridSkillEntryVisualState::Selected;
	}
	return bInHovered
		? EGridSkillEntryVisualState::Hovered
		: EGridSkillEntryVisualState::Normal;
}

EGridSkillTrainingState UGridSkillEntryWidget::ResolveTrainingState(const FGridSkillEntryView& InEntry)
{
	if (InEntry.Rank > 0)
	{
		return EGridSkillTrainingState::Trained;
	}
	return InEntry.bAllowUntrainedChecks
		? EGridSkillTrainingState::UntrainedAllowed
		: EGridSkillTrainingState::TrainingRequired;
}

void UGridSkillEntryWidget::ClearSkillEntry()
{
	Entry = FGridSkillEntryView();
	bInitialized = false;
	bSelected = false;
	bHovered = false;
	VisualState = EGridSkillEntryVisualState::Normal;
	SkillIcon = nullptr;

	if (Text_SkillName) Text_SkillName->SetText(FText::GetEmpty());
	if (Text_SkillAttribute)
	{
		Text_SkillAttribute->SetText(FText::GetEmpty());
		Text_SkillAttribute->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (UBorder* AffinityBorder : {
		Border_AffinityRecommended.Get(),
		Border_AffinityUseful.Get(),
		Border_AffinityNeutral.Get(),
		Border_AffinityDiscouraged.Get()
	})
	{
		if (AffinityBorder)
		{
			AffinityBorder->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (Text_SkillRank) Text_SkillRank->SetText(FText::GetEmpty());
	if (Text_SkillTrainingPolicy) Text_SkillTrainingPolicy->SetText(FText::GetEmpty());
	if (Button_IncreaseSkill)
	{
		Button_IncreaseSkill->SetIsEnabled(false);
	}
	if (Button_DecreaseSkill)
	{
		Button_DecreaseSkill->SetIsEnabled(false);
	}
	if (Image_SkillIcon)
	{
		Image_SkillIcon->SetBrushFromTexture(nullptr, false);
		Image_SkillIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
	RefreshInteractionVisual();
}

void UGridSkillEntryWidget::RefreshEntryVisual()
{
	if (!bInitialized)
	{
		return;
	}

	if (Text_SkillName)
	{
		Text_SkillName->SetText(ResolveDisplayName(Entry));
	}
	if (Text_SkillAttribute)
	{
		const bool bHasAttribute = Entry.GoverningAttribute != ERPGSkillGoverningAttribute::None;
		Text_SkillAttribute->SetText(bHasAttribute ? ResolveAttributeLabel(Entry) : FText::GetEmpty());
		Text_SkillAttribute->SetVisibility(bHasAttribute ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	auto ApplyAffinityBorder = [this](UBorder* Border, EGridSkillClassAffinity Affinity)
	{
		if (Border)
		{
			Border->SetVisibility(
				Entry.ClassAffinity == Affinity
					? ESlateVisibility::HitTestInvisible
					: ESlateVisibility::Collapsed);
		}
	};
	ApplyAffinityBorder(Border_AffinityRecommended, EGridSkillClassAffinity::Recommended);
	ApplyAffinityBorder(Border_AffinityUseful, EGridSkillClassAffinity::Useful);
	ApplyAffinityBorder(Border_AffinityNeutral, EGridSkillClassAffinity::Neutral);
	ApplyAffinityBorder(Border_AffinityDiscouraged, EGridSkillClassAffinity::Discouraged);
	if (Text_SkillRank)
	{
		Text_SkillRank->SetText(ResolveRankLabel(Entry));
	}
	if (Text_SkillTrainingPolicy)
	{
		Text_SkillTrainingPolicy->SetText(ResolveTrainingLabel(Entry));
	}
	if (Image_SkillIcon)
	{
		Image_SkillIcon->SetBrushFromTexture(SkillIcon.Get(), false);
		Image_SkillIcon->SetVisibility(SkillIcon.Get() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (Button_IncreaseSkill)
	{
		Button_IncreaseSkill->SetIsEnabled(Entry.bCanIncreaseRank);
	}
	if (Button_DecreaseSkill)
	{
		Button_DecreaseSkill->SetIsEnabled(Entry.bCanDecreaseRank);
	}

	RefreshInteractionVisual();
}

void UGridSkillEntryWidget::RefreshInteractionVisual()
{
	VisualState = ResolveVisualState(bSelected, bHovered);

	if (Border_SkillSelection)
	{
		Border_SkillSelection->SetVisibility(
			VisualState == EGridSkillEntryVisualState::Selected
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}
	if (Border_SkillHover)
	{
		Border_SkillHover->SetVisibility(
			VisualState == EGridSkillEntryVisualState::Hovered
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}
}

FText UGridSkillEntryWidget::ResolveDisplayName(const FGridSkillEntryView& InEntry)
{
	if (!InEntry.DisplayName.IsEmpty())
	{
		return InEntry.DisplayName;
	}

	// Missing authored content must never leak a technical SkillId to the player.
	return FUILocalizationText::Get(TEXT("Skill.Fallback.Unknown"));
}

FText UGridSkillEntryWidget::ResolveAttributeLabel(const FGridSkillEntryView& InEntry)
{
	switch (InEntry.GoverningAttribute)
	{
		case ERPGSkillGoverningAttribute::None:
			return FText::GetEmpty();
		case ERPGSkillGoverningAttribute::Strength:
			return FUILocalizationText::Get(TEXT("CharacterCreation.Attribute.Strength"));
		case ERPGSkillGoverningAttribute::Dexterity:
			return FUILocalizationText::Get(TEXT("CharacterCreation.Attribute.Dexterity"));
		case ERPGSkillGoverningAttribute::Constitution:
			return FUILocalizationText::Get(TEXT("CharacterCreation.Attribute.Constitution"));
		case ERPGSkillGoverningAttribute::Intelligence:
			return FUILocalizationText::Get(TEXT("CharacterCreation.Attribute.Intelligence"));
		case ERPGSkillGoverningAttribute::Wisdom:
			return FUILocalizationText::Get(TEXT("CharacterCreation.Attribute.Wisdom"));
		case ERPGSkillGoverningAttribute::Charisma:
			return FUILocalizationText::Get(TEXT("CharacterCreation.Attribute.Charisma"));
		default:
			return FText::GetEmpty();
	}
}

FText UGridSkillEntryWidget::ResolveRankLabel(const FGridSkillEntryView& InEntry)
{
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Rank"), FText::AsNumber(InEntry.Rank));
	Arguments.Add(TEXT("MaxRank"), FText::AsNumber(InEntry.MaxRank));
	return FText::Format(
		FUILocalizationText::Get(TEXT("Skill.Rank.Format")),
		Arguments);
}

FText UGridSkillEntryWidget::ResolveTrainingLabel(const FGridSkillEntryView& InEntry)
{
	switch (ResolveTrainingState(InEntry))
	{
		case EGridSkillTrainingState::Trained:
			return FUILocalizationText::Get(TEXT("Skill.Training.Trained"));
		case EGridSkillTrainingState::UntrainedAllowed:
			return FUILocalizationText::Get(TEXT("Skill.Training.UntrainedAllowed"));
		case EGridSkillTrainingState::TrainingRequired:
			return FUILocalizationText::Get(TEXT("Skill.Training.Required"));
		default:
			return FText::GetEmpty();
	}
}

bool UGridSkillEntryWidget::IsValidEntry(const FGridSkillEntryView& InEntry)
{
	const bool bRankCapValid =
		InEntry.CurrentRankCap == 0 ||
		(InEntry.CurrentRankCap >= 1 && InEntry.CurrentRankCap <= InEntry.MaxRank);

	return !InEntry.SkillId.IsNone() &&
		InEntry.MaxRank > 0 &&
		InEntry.Rank >= 0 &&
		InEntry.Rank <= InEntry.MaxRank &&
		bRankCapValid &&
		InEntry.bTrained == (InEntry.Rank > 0) &&
		(!InEntry.bCanIncreaseRank ||
			(InEntry.CurrentRankCap > 0 && InEntry.Rank < InEntry.CurrentRankCap)) &&
		(!InEntry.bCanDecreaseRank || InEntry.Rank > 0);
}

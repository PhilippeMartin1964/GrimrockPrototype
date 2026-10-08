#include "UI/GridSkillEntryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

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

void UGridSkillEntryWidget::ClearSkillEntry()
{
	Entry = FGridSkillEntryView();
	bInitialized = false;

	if (Text_SkillName) Text_SkillName->SetText(FText::GetEmpty());
	if (Text_SkillAttribute)
	{
		Text_SkillAttribute->SetText(FText::GetEmpty());
		Text_SkillAttribute->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Text_SkillRank) Text_SkillRank->SetText(FText::GetEmpty());
	if (Text_SkillTrainingPolicy) Text_SkillTrainingPolicy->SetText(FText::GetEmpty());
	if (Text_SkillDescription)
	{
		Text_SkillDescription->SetText(FText::GetEmpty());
		Text_SkillDescription->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Button_IncreaseSkill)
	{
		Button_IncreaseSkill->SetIsEnabled(false);
	}
	if (Button_DecreaseSkill)
	{
		Button_DecreaseSkill->SetIsEnabled(false);
	}
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
	if (Text_SkillRank)
	{
		Text_SkillRank->SetText(ResolveRankLabel(Entry));
	}
	if (Text_SkillTrainingPolicy)
	{
		Text_SkillTrainingPolicy->SetText(ResolveTrainingLabel(Entry));
	}
	if (Text_SkillDescription)
	{
		const bool bHasDescription = HasDescription(Entry);
		Text_SkillDescription->SetText(bHasDescription ? Entry.Description : FText::GetEmpty());
		Text_SkillDescription->SetVisibility(bHasDescription ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Button_IncreaseSkill)
	{
		Button_IncreaseSkill->SetIsEnabled(Entry.bCanIncreaseRank);
	}
	if (Button_DecreaseSkill)
	{
		Button_DecreaseSkill->SetIsEnabled(Entry.bCanDecreaseRank);
	}
}

FText UGridSkillEntryWidget::ResolveDisplayName(const FGridSkillEntryView& InEntry)
{
	if (!InEntry.DisplayName.IsEmpty())
	{
		return InEntry.DisplayName;
	}
	return InEntry.SkillId.IsNone()
		? NSLOCTEXT("GridSkills", "UnknownSkill", "Compétence inconnue")
		: FText::FromName(InEntry.SkillId);
}

FText UGridSkillEntryWidget::ResolveAttributeLabel(const FGridSkillEntryView& InEntry)
{
	if (InEntry.GoverningAttribute == ERPGSkillGoverningAttribute::None)
	{
		return FText::GetEmpty();
	}

	if (const UEnum* AttributeEnum = StaticEnum<ERPGSkillGoverningAttribute>())
	{
		return AttributeEnum->GetDisplayNameTextByValue(static_cast<int64>(InEntry.GoverningAttribute));
	}
	return FText::GetEmpty();
}

FText UGridSkillEntryWidget::ResolveRankLabel(const FGridSkillEntryView& InEntry)
{
	return FText::Format(
		NSLOCTEXT("GridSkills", "SkillRankFormat", "Rang {0} / {1}"),
		FText::AsNumber(InEntry.Rank),
		FText::AsNumber(InEntry.MaxRank));
}

FText UGridSkillEntryWidget::ResolveTrainingLabel(const FGridSkillEntryView& InEntry)
{
	if (InEntry.bTrained)
	{
		return NSLOCTEXT("GridSkills", "SkillTrained", "Entraînée");
	}
	if (InEntry.bAllowUntrainedChecks)
	{
		return NSLOCTEXT("GridSkills", "SkillUntrainedAllowed", "Test possible sans entraînement");
	}
	return NSLOCTEXT("GridSkills", "SkillTrainingRequired", "Entraînement requis");
}

bool UGridSkillEntryWidget::HasDescription(const FGridSkillEntryView& InEntry)
{
	return !InEntry.Description.IsEmpty();
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

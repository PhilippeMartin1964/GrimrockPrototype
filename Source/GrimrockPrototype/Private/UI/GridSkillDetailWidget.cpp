#include "UI/GridSkillDetailWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Texture2D.h"
#include "UI/UILocalizationText.h"

void UGridSkillDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyLocalizedStaticLabels();

	if (bInitialized)
	{
		RefreshDetailVisual();
	}
	else
	{
		ClearSkillDetail();
	}
}

bool UGridSkillDetailWidget::InitializeSkillDetail(const FGridSkillDetailView& InDetailView)
{
	ClearSkillDetail();
	if (!InDetailView.IsValid())
	{
		return false;
	}

	DetailView = InDetailView;
	bInitialized = true;
	RefreshDetailVisual();
	return true;
}

void UGridSkillDetailWidget::ClearSkillDetail()
{
	DetailView = FGridSkillDetailView();
	bInitialized = false;

	ResolvedAttributeText = FText::GetEmpty();
	ResolvedRankText = FText::GetEmpty();
	ResolvedTrainingText = FText::GetEmpty();
	ResolvedCheckRankText = FText::GetEmpty();
	ResolvedCheckAttributeModifierText = FText::GetEmpty();
	ResolvedCheckBaseBonusText = FText::GetEmpty();
	ResolvedRecommendedClassesText = FText::GetEmpty();
	ResolvedUsefulClassesText = FText::GetEmpty();
	ResolvedSynergiesText = FText::GetEmpty();
	ResolvedCurrentRankCapText = FText::GetEmpty();
	ResolvedRemainingSkillPointsText = FText::GetEmpty();
	ResolvedNextRankText = FText::GetEmpty();
	ResolvedNextRankCostText = FText::GetEmpty();

	if (Image_SkillIcon)
	{
		Image_SkillIcon->SetBrushFromTexture(nullptr, false);
		Image_SkillIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Text_DetailName)
	{
		Text_DetailName->SetText(FText::GetEmpty());
		Text_DetailName->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Text_DetailAttribute)
	{
		Text_DetailAttribute->SetText(FText::GetEmpty());
		Text_DetailAttribute->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Text_DetailRank)
	{
		Text_DetailRank->SetText(FText::GetEmpty());
		Text_DetailRank->SetVisibility(ESlateVisibility::Collapsed);
	}

	ApplyOptionalSection(VB_DetailDescription, Text_DetailDescription, FText::GetEmpty());
	ApplyOptionalSection(VB_DetailTraining, Text_DetailTraining, FText::GetEmpty());
	ApplyOptionalSection(VB_DetailUsage, Text_DetailUsage, FText::GetEmpty());
	ApplyOptionalSection(VB_DetailBuildAdvice, Text_DetailBuildAdvice, FText::GetEmpty());

	if (VB_DetailCheck)
	{
		VB_DetailCheck->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (UTextBlock* Widget : {
		Text_DetailCheckRank.Get(),
		Text_DetailCheckAttributeModifier.Get(),
		Text_DetailCheckBaseBonus.Get()
	})
	{
		if (Widget)
		{
			Widget->SetText(FText::GetEmpty());
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (VB_DetailAffinities)
	{
		VB_DetailAffinities->SetVisibility(ESlateVisibility::Collapsed);
	}
	ApplyOptionalSection(
		VB_DetailRecommendedClasses,
		Text_DetailRecommendedClasses,
		FText::GetEmpty());
	ApplyOptionalSection(
		VB_DetailUsefulClasses,
		Text_DetailUsefulClasses,
		FText::GetEmpty());
	ApplyOptionalSection(VB_DetailSynergies, Text_DetailSynergies, FText::GetEmpty());

	if (VB_DetailProgression)
	{
		VB_DetailProgression->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (UTextBlock* Widget : {
		Text_DetailCurrentRankCap.Get(),
		Text_DetailRemainingSkillPoints.Get(),
		Text_DetailNextRank.Get(),
		Text_DetailNextRankCost.Get()
	})
	{
		if (Widget)
		{
			Widget->SetText(FText::GetEmpty());
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (VB_DetailNextRank)
	{
		VB_DetailNextRank->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (VB_DetailNextRankCost)
	{
		VB_DetailNextRankCost->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGridSkillDetailWidget::ApplyLocalizedStaticLabels()
{
	auto SetLabel = [](UTextBlock* Widget, const TCHAR* Key)
	{
		if (Widget)
		{
			Widget->SetText(FUILocalizationText::Get(Key));
		}
	};

	SetLabel(Text_LabelDescription, TEXT("UI.Section.Description"));
	SetLabel(Text_LabelTraining, TEXT("Skill.Section.Training"));
	SetLabel(Text_LabelUsage, TEXT("UI.Section.Usage"));
	SetLabel(Text_LabelBuildAdvice, TEXT("Skill.Section.BuildAdvice"));
	SetLabel(Text_LabelCheck, TEXT("Skill.Section.Check"));
	SetLabel(Text_LabelCheckRank, TEXT("Skill.Check.Rank"));
	SetLabel(Text_LabelCheckAttributeModifier, TEXT("Skill.Check.AttributeModifier"));
	SetLabel(Text_LabelCheckBaseBonus, TEXT("Skill.Check.BaseBonus"));
	SetLabel(Text_LabelAffinities, TEXT("Skill.Section.Affinities"));
	SetLabel(Text_LabelRecommendedClasses, TEXT("Skill.Affinity.RecommendedClasses"));
	SetLabel(Text_LabelUsefulClasses, TEXT("Skill.Affinity.UsefulClasses"));
	SetLabel(Text_LabelSynergies, TEXT("Skill.Section.Synergies"));
	SetLabel(Text_LabelProgression, TEXT("UI.Section.Progression"));
	SetLabel(Text_LabelCurrentRankCap, TEXT("Skill.Progression.CurrentRankCap"));
	SetLabel(Text_LabelRemainingSkillPoints, TEXT("Skill.Progression.RemainingSkillPoints"));
	SetLabel(Text_LabelNextRank, TEXT("Skill.Progression.NextRank"));
	SetLabel(Text_LabelNextRankCost, TEXT("Skill.Progression.NextRankCost"));
}

void UGridSkillDetailWidget::RefreshResolvedPresentation()
{
	if (!bInitialized)
	{
		return;
	}

	ResolvedAttributeText = ResolveAttributeLabel(DetailView.GoverningAttribute);
	ResolvedRankText = ResolveRankLabel(DetailView.Rank, DetailView.MaxRank);
	ResolvedTrainingText = ResolveTrainingLabel(DetailView.TrainingState);
	ResolvedCheckRankText = ResolveSignedInteger(DetailView.Rank);
	ResolvedCheckAttributeModifierText = ResolveSignedInteger(DetailView.GoverningAttributeModifier);
	ResolvedCheckBaseBonusText = ResolveSignedInteger(DetailView.BaseCheckBonus);
	ResolvedRecommendedClassesText = JoinClassNames(DetailView.RecommendedClasses);
	ResolvedUsefulClassesText = JoinClassNames(DetailView.UsefulClasses);
	ResolvedSynergiesText = JoinSynergyNames(DetailView.Synergies);
	ResolvedCurrentRankCapText = FText::AsNumber(DetailView.CurrentRankCap);
	ResolvedRemainingSkillPointsText = FText::AsNumber(DetailView.RemainingSkillPoints);
	ResolvedNextRankText =
		DetailView.NextRank > 0
			? FText::AsNumber(DetailView.NextRank)
			: FText::GetEmpty();
	ResolvedNextRankCostText =
		DetailView.NextRank > 0
			? FText::AsNumber(DetailView.NextRankCost)
			: FText::GetEmpty();
}

void UGridSkillDetailWidget::RefreshDetailVisual()
{
	if (!bInitialized)
	{
		return;
	}

	RefreshResolvedPresentation();

	UTexture2D* IconTexture = DetailView.Icon.LoadSynchronous();
	if (Image_SkillIcon)
	{
		Image_SkillIcon->SetBrushFromTexture(IconTexture, false);
		Image_SkillIcon->SetVisibility(
			IconTexture
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
	}

	if (Text_DetailName)
	{
		Text_DetailName->SetText(DetailView.DisplayName);
		Text_DetailName->SetVisibility(
			DetailView.DisplayName.IsEmpty()
				? ESlateVisibility::Collapsed
				: ESlateVisibility::SelfHitTestInvisible);
	}
	if (Text_DetailAttribute)
	{
		Text_DetailAttribute->SetText(ResolvedAttributeText);
		Text_DetailAttribute->SetVisibility(
			ResolvedAttributeText.IsEmpty()
				? ESlateVisibility::Collapsed
				: ESlateVisibility::SelfHitTestInvisible);
	}
	if (Text_DetailRank)
	{
		Text_DetailRank->SetText(ResolvedRankText);
		Text_DetailRank->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	ApplyOptionalSection(VB_DetailDescription, Text_DetailDescription, DetailView.Description);
	ApplyOptionalSection(VB_DetailTraining, Text_DetailTraining, ResolvedTrainingText);
	ApplyOptionalSection(VB_DetailUsage, Text_DetailUsage, DetailView.UsageDescription);
	ApplyOptionalSection(VB_DetailBuildAdvice, Text_DetailBuildAdvice, DetailView.BuildAdvice);

	if (VB_DetailCheck)
	{
		VB_DetailCheck->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (Text_DetailCheckRank)
	{
		Text_DetailCheckRank->SetText(ResolvedCheckRankText);
		Text_DetailCheckRank->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (Text_DetailCheckAttributeModifier)
	{
		Text_DetailCheckAttributeModifier->SetText(ResolvedCheckAttributeModifierText);
		Text_DetailCheckAttributeModifier->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (Text_DetailCheckBaseBonus)
	{
		Text_DetailCheckBaseBonus->SetText(ResolvedCheckBaseBonusText);
		Text_DetailCheckBaseBonus->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	ApplyOptionalSection(
		VB_DetailRecommendedClasses,
		Text_DetailRecommendedClasses,
		ResolvedRecommendedClassesText);
	ApplyOptionalSection(
		VB_DetailUsefulClasses,
		Text_DetailUsefulClasses,
		ResolvedUsefulClassesText);
	if (VB_DetailAffinities)
	{
		VB_DetailAffinities->SetVisibility(
			!ResolvedRecommendedClassesText.IsEmpty() || !ResolvedUsefulClassesText.IsEmpty()
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
	}

	ApplyOptionalSection(VB_DetailSynergies, Text_DetailSynergies, ResolvedSynergiesText);

	if (VB_DetailProgression)
	{
		VB_DetailProgression->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (Text_DetailCurrentRankCap)
	{
		Text_DetailCurrentRankCap->SetText(ResolvedCurrentRankCapText);
		Text_DetailCurrentRankCap->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (Text_DetailRemainingSkillPoints)
	{
		Text_DetailRemainingSkillPoints->SetText(ResolvedRemainingSkillPointsText);
		Text_DetailRemainingSkillPoints->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	const bool bHasNextRank = DetailView.NextRank > 0;
	if (VB_DetailNextRank)
	{
		VB_DetailNextRank->SetVisibility(
			bHasNextRank
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
	}
	if (Text_DetailNextRank)
	{
		Text_DetailNextRank->SetText(ResolvedNextRankText);
		Text_DetailNextRank->SetVisibility(
			bHasNextRank
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
	}
	if (VB_DetailNextRankCost)
	{
		VB_DetailNextRankCost->SetVisibility(
			bHasNextRank
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
	}
	if (Text_DetailNextRankCost)
	{
		Text_DetailNextRankCost->SetText(ResolvedNextRankCostText);
		Text_DetailNextRankCost->SetVisibility(
			bHasNextRank
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
	}
}

FText UGridSkillDetailWidget::ResolveAttributeLabel(ERPGSkillGoverningAttribute Attribute)
{
	switch (Attribute)
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

FText UGridSkillDetailWidget::ResolveRankLabel(int32 Rank, int32 MaxRank)
{
	if (Rank < 0 || MaxRank < 1 || Rank > MaxRank)
	{
		return FText::GetEmpty();
	}

	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("Rank"), FText::AsNumber(Rank));
	Arguments.Add(TEXT("MaxRank"), FText::AsNumber(MaxRank));
	return FText::Format(
		FUILocalizationText::Get(TEXT("Skill.Rank.Format")),
		Arguments);
}

FText UGridSkillDetailWidget::ResolveTrainingLabel(EGridSkillTrainingState TrainingState)
{
	switch (TrainingState)
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

FText UGridSkillDetailWidget::ResolveSignedInteger(int32 Value)
{
	return FText::FromString(FString::Printf(TEXT("%+d"), Value));
}

FText UGridSkillDetailWidget::JoinClassNames(const TArray<FGridSkillClassAffinityView>& Classes)
{
	TArray<FString> Names;
	Names.Reserve(Classes.Num());
	for (const FGridSkillClassAffinityView& ClassView : Classes)
	{
		if (!ClassView.DisplayName.IsEmpty())
		{
			Names.Add(ClassView.DisplayName.ToString());
		}
	}
	return Names.IsEmpty()
		? FText::GetEmpty()
		: FText::FromString(FString::Join(Names, TEXT(", ")));
}

FText UGridSkillDetailWidget::JoinSynergyNames(const TArray<FGridSkillSynergyView>& Synergies)
{
	TArray<FString> Names;
	Names.Reserve(Synergies.Num());
	for (const FGridSkillSynergyView& Synergy : Synergies)
	{
		if (!Synergy.DisplayName.IsEmpty())
		{
			Names.Add(Synergy.DisplayName.ToString());
		}
	}
	return Names.IsEmpty()
		? FText::GetEmpty()
		: FText::FromString(FString::Join(Names, TEXT(", ")));
}

void UGridSkillDetailWidget::ApplyOptionalSection(
	UVerticalBox* Section,
	UTextBlock* ValueWidget,
	const FText& Value)
{
	const ESlateVisibility Visibility =
		Value.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::SelfHitTestInvisible;

	if (Section)
	{
		Section->SetVisibility(Visibility);
	}
	if (ValueWidget)
	{
		ValueWidget->SetText(Value);
		ValueWidget->SetVisibility(Visibility);
	}
}

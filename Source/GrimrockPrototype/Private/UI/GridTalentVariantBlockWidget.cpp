#include "UI/GridTalentVariantBlockWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

void UGridTalentVariantBlockWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindChooseButton();
	ApplyPresentation();
}

void UGridTalentVariantBlockWidget::NativeDestruct()
{
	UnbindChooseButton();
	Super::NativeDestruct();
}

bool UGridTalentVariantBlockWidget::InitializeVariant(
	const FGridTalentVariantView& InVariant,
	const FText& InDisplayName,
	bool bInPendingChoice)
{
	ClearVariant();

	if (InVariant.ChoiceId.IsNone())
	{
		return false;
	}

	VariantView = InVariant;
	ResolvedDisplayName = InDisplayName.IsEmpty() ? InVariant.DisplayName : InDisplayName;
	if (ResolvedDisplayName.IsEmpty())
	{
		ClearVariant();
		return false;
	}

	ResolvedTypeText = InVariant.TypeText;
	ResolvedStatusText = InVariant.StatusText;
	ResolvedPrincipleText = InVariant.Principle;
	ResolvedEffectsText = FormatDetailLines(InVariant.Effects);
	ResolvedUsageText = FormatDetailLines(InVariant.Usage);
	bPendingChoice = bInPendingChoice;

	if (InVariant.bAcquired)
	{
		ResolvedChooseLabel = FText::FromString(TEXT("ACQUISE"));
		bChooseEnabled = false;
	}
	else if (bPendingChoice)
	{
		ResolvedChooseLabel = FText::FromString(TEXT("CHOIX EN COURS"));
		bChooseEnabled = false;
	}
	else if (InVariant.bCanChoose)
	{
		ResolvedChooseLabel = FText::FromString(TEXT("CHOISIR"));
		bChooseEnabled = true;
	}
	else
	{
		ResolvedChooseLabel = FText::FromString(TEXT("INDISPONIBLE"));
		bChooseEnabled = false;
	}

	bInitialized = true;
	ApplyPresentation();
	return true;
}

void UGridTalentVariantBlockWidget::ClearVariant()
{
	VariantView = FGridTalentVariantView();
	ResolvedDisplayName = FText::GetEmpty();
	ResolvedTypeText = FText::GetEmpty();
	ResolvedStatusText = FText::GetEmpty();
	ResolvedPrincipleText = FText::GetEmpty();
	ResolvedEffectsText = FText::GetEmpty();
	ResolvedUsageText = FText::GetEmpty();
	ResolvedChooseLabel = FText::GetEmpty();
	bChooseEnabled = false;
	bPendingChoice = false;
	bInitialized = false;
	ApplyPresentation();
}

void UGridTalentVariantBlockWidget::BindChooseButton()
{
	if (Button_ChooseVariant)
	{
		Button_ChooseVariant->OnClicked.RemoveDynamic(this, &UGridTalentVariantBlockWidget::HandleChooseClicked);
		Button_ChooseVariant->OnClicked.AddUniqueDynamic(this, &UGridTalentVariantBlockWidget::HandleChooseClicked);
	}
}

void UGridTalentVariantBlockWidget::UnbindChooseButton()
{
	if (Button_ChooseVariant)
	{
		Button_ChooseVariant->OnClicked.RemoveDynamic(this, &UGridTalentVariantBlockWidget::HandleChooseClicked);
	}
}

void UGridTalentVariantBlockWidget::HandleChooseClicked()
{
	if (bInitialized && bChooseEnabled && !VariantView.ChoiceId.IsNone())
	{
		OnChooseRequested.Broadcast(VariantView.ChoiceId);
	}
}

FText UGridTalentVariantBlockWidget::FormatDetailLines(const TArray<FGridTalentDetailLineView>& Lines)
{
	TArray<FString> Formatted;
	Formatted.Reserve(Lines.Num());
	for (const FGridTalentDetailLineView& Line : Lines)
	{
		if (Line.Value.IsEmpty())
		{
			continue;
		}
		Formatted.Add(Line.Label.IsEmpty()
			? Line.Value.ToString()
			: Line.Label.ToString() + TEXT(" : ") + Line.Value.ToString());
	}
	return FText::FromString(FString::Join(Formatted, TEXT("\n")));
}

void UGridTalentVariantBlockWidget::ApplyPresentation()
{
	auto SetOptionalText = [](UTextBlock* Widget, const FText& Text)
	{
		if (Widget)
		{
			Widget->SetText(Text);
			Widget->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
		}
	};
	auto SetOptionalSection = [&SetOptionalText](UVerticalBox* Section, UTextBlock* Widget, const FText& Text)
	{
		const ESlateVisibility Visibility = Text.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::SelfHitTestInvisible;
		if (Section)
		{
			Section->SetVisibility(Visibility);
		}
		SetOptionalText(Widget, Text);
	};

	SetOptionalText(Text_VariantName, ResolvedDisplayName);
	SetOptionalText(Text_VariantType, ResolvedTypeText);
	SetOptionalText(Text_VariantStatus, ResolvedStatusText);
	SetOptionalText(Text_VariantPrinciple, ResolvedPrincipleText);
	SetOptionalSection(VB_VariantEffects, Text_VariantEffects, ResolvedEffectsText);
	SetOptionalSection(VB_VariantUsage, Text_VariantUsage, ResolvedUsageText);

	if (Text_ChooseVariant)
	{
		Text_ChooseVariant->SetText(ResolvedChooseLabel);
	}
	if (Button_ChooseVariant)
	{
		Button_ChooseVariant->SetVisibility(bInitialized ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Button_ChooseVariant->SetIsEnabled(bChooseEnabled);
	}
}

#include "UI/GridTalentVariantBlockWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/World.h"
#include "UI/GridTalentDetailLineWidget.h"
#include "UI/RPGTalentDetailGlyphSet.h"
#include "UI/RPGTalentDetailTextSet.h"

namespace
{
	FName VariantChooseTextId(const FGridTalentVariantView& Variant, bool bPendingChoice)
	{
		if (Variant.bAcquired) return TEXT("Talent.Variant.Acquired");
		if (bPendingChoice) return TEXT("Talent.Variant.Pending");
		if (Variant.bCanChoose) return TEXT("Talent.Variant.Choose");
		return TEXT("Talent.Variant.Unavailable");
	}
}

void UGridTalentVariantBlockWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindChooseButton();
	RebuildDetailLineRows();
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

	ResolvedPrincipleText = InVariant.Principle;
	bPendingChoice = bInPendingChoice;

	ResolvedChooseLabel =
		URPGTalentDetailTextSet::ResolveProductionText(VariantChooseTextId(InVariant, bPendingChoice));
	bChooseEnabled = !InVariant.bAcquired && !bPendingChoice && InVariant.bCanChoose;

	bInitialized = true;
	RebuildDetailLineRows();
	ApplyPresentation();
	return true;
}

void UGridTalentVariantBlockWidget::ClearVariant()
{
	VariantView = FGridTalentVariantView();
	ResolvedDisplayName = FText::GetEmpty();
	ResolvedPrincipleText = FText::GetEmpty();
	ResolvedChooseLabel = FText::GetEmpty();
	bChooseEnabled = false;
	bPendingChoice = false;
	bInitialized = false;

	if (VB_VariantEffectsEntries)
	{
		VB_VariantEffectsEntries->ClearChildren();
		VB_VariantEffectsEntries->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (VB_VariantUsageEntries)
	{
		VB_VariantUsageEntries->ClearChildren();
		VB_VariantUsageEntries->SetVisibility(ESlateVisibility::Collapsed);
	}

	ApplyPresentation();
}

void UGridTalentVariantBlockWidget::ConfigureDetailRows(
	TSubclassOf<UGridTalentDetailLineWidget> InDetailLineWidgetClass,
	URPGTalentDetailGlyphSet* InDetailGlyphSet)
{
	DetailLineWidgetClass = InDetailLineWidgetClass;
	DetailGlyphSet = InDetailGlyphSet;
	if (bInitialized)
	{
		RebuildDetailLineRows();
		ApplyPresentation();
	}
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

void UGridTalentVariantBlockWidget::PopulateDetailLineRows(
	UVerticalBox* Container,
	const TArray<FGridTalentDetailLineView>& Lines)
{
	if (!Container)
	{
		return;
	}

	Container->ClearChildren();
	if (!bInitialized || !DetailLineWidgetClass || GetWorld() == nullptr)
	{
		Container->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	for (const FGridTalentDetailLineView& Line : Lines)
	{
		if (Line.Value.IsEmpty() && Line.ValueTextId.IsNone() && !Line.bHasNumericValue)
		{
			continue;
		}

		UGridTalentDetailLineWidget* Row =
			CreateWidget<UGridTalentDetailLineWidget>(GetWorld(), DetailLineWidgetClass);
		if (!Row)
		{
			continue;
		}

		UTexture2D* Glyph = DetailGlyphSet ? DetailGlyphSet->ResolveGlyph(Line.Semantic) : nullptr;
		const FText LabelText = URPGTalentDetailTextSet::ResolveProductionText(Line.LabelTextId);
		const FText ValueText = !Line.ValueTextId.IsNone()
			? URPGTalentDetailTextSet::ResolveProductionText(Line.ValueTextId)
			: Line.Value;
		if (!Row->InitializeLine(Line, Glyph, LabelText, ValueText))
		{
			continue;
		}
		Container->AddChildToVerticalBox(Row);
	}

	Container->SetVisibility(
		Container->GetChildrenCount() > 0
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
}

void UGridTalentVariantBlockWidget::RebuildDetailLineRows()
{
	PopulateDetailLineRows(VB_VariantEffectsEntries, VariantView.Effects);
	PopulateDetailLineRows(VB_VariantUsageEntries, VariantView.Usage);
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
	SetOptionalText(Text_VariantPrinciple, ResolvedPrincipleText);

	auto SetStructuredSection = [](UVerticalBox* Section, UVerticalBox* Entries)
	{
		const bool bHasRows = Entries && Entries->GetChildrenCount() > 0;
		const ESlateVisibility Visibility =
			bHasRows ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;

		if (Section)
		{
			Section->SetVisibility(Visibility);
		}
		if (Entries)
		{
			Entries->SetVisibility(Visibility);
		}
	};

	SetStructuredSection(VB_VariantEffects, VB_VariantEffectsEntries);
	SetStructuredSection(VB_VariantUsage, VB_VariantUsageEntries);

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

#include "UI/GridTalentDetailWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/World.h"
#include "UI/GridTalentVariantBlockWidget.h"

void UGridTalentDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindAcquireButtons();
	if (bInitialized)
	{
		RebuildVariantBlocks();
		ApplyDetailPresentation();
		ApplyAcquisitionPresentation();
	}
	else
	{
		// Designer labels must not leak before a Talent is actually consulted.
		ClearTalentDetail();
	}
}

void UGridTalentDetailWidget::NativeDestruct()
{
	UnbindAcquireButtons();
	Super::NativeDestruct();
}

void UGridTalentDetailWidget::BindAcquireButtons()
{
	if (Button_AcquireTalent)
	{
		Button_AcquireTalent->OnClicked.RemoveDynamic(this, &UGridTalentDetailWidget::HandleAcquireClicked);
		Button_AcquireTalent->OnClicked.AddUniqueDynamic(this, &UGridTalentDetailWidget::HandleAcquireClicked);
	}
	if (Button_ConfirmAcquire)
	{
		Button_ConfirmAcquire->OnClicked.RemoveDynamic(this, &UGridTalentDetailWidget::HandleConfirmAcquireClicked);
		Button_ConfirmAcquire->OnClicked.AddUniqueDynamic(this, &UGridTalentDetailWidget::HandleConfirmAcquireClicked);
	}
	if (Button_CancelAcquire)
	{
		Button_CancelAcquire->OnClicked.RemoveDynamic(this, &UGridTalentDetailWidget::HandleCancelAcquireClicked);
		Button_CancelAcquire->OnClicked.AddUniqueDynamic(this, &UGridTalentDetailWidget::HandleCancelAcquireClicked);
	}
}

void UGridTalentDetailWidget::UnbindAcquireButtons()
{
	if (Button_AcquireTalent)
	{
		Button_AcquireTalent->OnClicked.RemoveDynamic(this, &UGridTalentDetailWidget::HandleAcquireClicked);
	}
	if (Button_ConfirmAcquire)
	{
		Button_ConfirmAcquire->OnClicked.RemoveDynamic(this, &UGridTalentDetailWidget::HandleConfirmAcquireClicked);
	}
	if (Button_CancelAcquire)
	{
		Button_CancelAcquire->OnClicked.RemoveDynamic(this, &UGridTalentDetailWidget::HandleCancelAcquireClicked);
	}
}

bool UGridTalentDetailWidget::InitializeTalentDetail(
	const FGridTalentNodeView& InNodeView,
	const FRPGTalentBranchPresentationDefinition& InBranchPresentation)
{
	ClearTalentDetail();

	if (InNodeView.TalentNodeId.IsNone() ||
		InNodeView.TalentBranchId.IsNone() ||
		InNodeView.TalentBranchId != InBranchPresentation.TalentBranchId ||
		InNodeView.DisplayName.IsEmpty() ||
		InNodeView.TypeText.IsEmpty() ||
		InNodeView.StatusText.IsEmpty() ||
		(InNodeView.bHasExclusiveVariants && InNodeView.Variants.Num() < 2) ||
		(!InNodeView.bHasExclusiveVariants && !InNodeView.Variants.IsEmpty()))
	{
		return false;
	}

	NodeView = InNodeView;
	ResolvedDisplayName = NodeView.DisplayName;
	BranchAccentColor = InBranchPresentation.AccentColor;
	bInitialized = true;
	RefreshCanonicalSections();
	RebuildVariantBlocks();
	ApplyDetailPresentation();
	ApplyAcquisitionPresentation();
	return true;
}

void UGridTalentDetailWidget::ClearTalentDetail()
{
	NodeView = FGridTalentNodeView();
	ResolvedDisplayName = FText::GetEmpty();
	ResolvedTypeText = FText::GetEmpty();
	ResolvedStatusText = FText::GetEmpty();
	ResolvedPrincipleText = FText::GetEmpty();
	ResolvedEffectsText = FText::GetEmpty();
	ResolvedUsageText = FText::GetEmpty();
	ResolvedAcquisitionText = FText::GetEmpty();
	BranchAccentColor = FLinearColor::White;
	bInitialized = false;
	bAcquireConfirmationPending = false;
	bVariantSelectionPending = false;
	SelectedVariantChoiceId = NAME_None;
	AcquisitionFeedback = FText::GetEmpty();

	if (Border_DetailAccent)
	{
		Border_DetailAccent->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Text_DetailName)
	{
		Text_DetailName->SetText(FText::GetEmpty());
		Text_DetailName->SetVisibility(ESlateVisibility::Collapsed);
	}

	auto ClearSection = [](UVerticalBox* Section, UTextBlock* Value)
	{
		if (Section)
		{
			Section->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (Value)
		{
			Value->SetText(FText::GetEmpty());
			Value->SetVisibility(ESlateVisibility::Collapsed);
		}
	};

	ClearSection(VB_DetailType, Text_DetailType);
	ClearSection(VB_DetailStatus, Text_DetailStatus);
	ClearSection(VB_DetailPrinciple, Text_DetailPrinciple);
	ClearSection(VB_DetailEffects, Text_DetailEffects);
	ClearSection(VB_DetailUsage, Text_DetailUsage);
	ClearSection(VB_DetailAcquisition, Text_DetailAcquisition);

	if (VB_VariantEntries) VB_VariantEntries->ClearChildren();
	if (VB_DetailVariants) VB_DetailVariants->SetVisibility(ESlateVisibility::Collapsed);
	ApplyAcquisitionPresentation();
}

bool UGridTalentDetailWidget::HasExclusiveVariants() const
{
	return NodeView.bHasExclusiveVariants;
}

bool UGridTalentDetailWidget::CanRequestSimpleAcquisition() const
{
	return bInitialized &&
		!HasExclusiveVariants() &&
		NodeView.State == EGridTalentNodeState::Available &&
		NodeView.bCanAcquireSimple &&
		!NodeView.SimpleChoiceId.IsNone();
}

bool UGridTalentDetailWidget::CanRequestVariantAcquisition() const
{
	if (!bInitialized || !HasExclusiveVariants() || NodeView.State != EGridTalentNodeState::Available)
	{
		return false;
	}

	return NodeView.Variants.ContainsByPredicate(
		[](const FGridTalentVariantView& Variant)
		{
			return !Variant.ChoiceId.IsNone() && Variant.bCanChoose && !Variant.bAcquired;
		});
}

bool UGridTalentDetailWidget::BeginAcquireConfirmation()
{
	if (!CanRequestSimpleAcquisition()) return false;

	bAcquireConfirmationPending = true;
	bVariantSelectionPending = false;
	SelectedVariantChoiceId = NAME_None;
	AcquisitionFeedback = FText::GetEmpty();
	ApplyAcquisitionPresentation();
	return true;
}

bool UGridTalentDetailWidget::BeginVariantSelection()
{
	if (!CanRequestVariantAcquisition()) return false;

	bAcquireConfirmationPending = false;
	bVariantSelectionPending = true;
	SelectedVariantChoiceId = NAME_None;
	AcquisitionFeedback = FText::GetEmpty();
	RebuildVariantBlocks();
	ApplyAcquisitionPresentation();
	return true;
}

bool UGridTalentDetailWidget::SelectVariantChoice(FName ChoiceId)
{
	if (!bInitialized || !bVariantSelectionPending || !HasExclusiveVariants() || ChoiceId.IsNone())
	{
		return false;
	}

	const FGridTalentVariantView* Variant = NodeView.Variants.FindByPredicate(
		[ChoiceId](const FGridTalentVariantView& Candidate)
		{
			return Candidate.ChoiceId == ChoiceId;
		});
	if (!Variant || !Variant->bCanChoose || Variant->bAcquired)
	{
		return false;
	}

	SelectedVariantChoiceId = ChoiceId;
	RebuildVariantBlocks();
	ApplyAcquisitionPresentation();
	return true;
}

bool UGridTalentDetailWidget::GetVariantDisplayLabel(FName ChoiceId, FText& OutLabel) const
{
	OutLabel = FText::GetEmpty();
	const FGridTalentVariantView* Variant = NodeView.Variants.FindByPredicate(
		[ChoiceId](const FGridTalentVariantView& Candidate)
		{
			return Candidate.ChoiceId == ChoiceId;
		});
	if (!Variant) return false;

	OutLabel = MakeVariantDisplayLabel(*Variant);
	return !OutLabel.IsEmpty();
}

void UGridTalentDetailWidget::CancelAcquireConfirmation()
{
	bAcquireConfirmationPending = false;
	bVariantSelectionPending = false;
	SelectedVariantChoiceId = NAME_None;
	AcquisitionFeedback = FText::GetEmpty();
	RebuildVariantBlocks();
	ApplyAcquisitionPresentation();
}

bool UGridTalentDetailWidget::ConfirmAcquire()
{
	if (!bInitialized) return false;

	FName ChoiceId = NAME_None;
	if (bAcquireConfirmationPending)
	{
		if (!CanRequestSimpleAcquisition()) return false;
		ChoiceId = NodeView.SimpleChoiceId;
	}
	else if (bVariantSelectionPending)
	{
		ChoiceId = SelectedVariantChoiceId;
		const FGridTalentVariantView* Variant = NodeView.Variants.FindByPredicate(
			[ChoiceId](const FGridTalentVariantView& Candidate)
			{
				return Candidate.ChoiceId == ChoiceId;
			});
		if (!Variant || !Variant->bCanChoose || Variant->bAcquired)
		{
			return false;
		}
	}

	if (ChoiceId.IsNone()) return false;

	bAcquireConfirmationPending = false;
	bVariantSelectionPending = false;
	SelectedVariantChoiceId = NAME_None;
	ApplyAcquisitionPresentation();
	OnAcquireConfirmed.Broadcast(ChoiceId);
	return true;
}

void UGridTalentDetailWidget::SetAcquisitionFeedback(const FText& InFeedback)
{
	AcquisitionFeedback = InFeedback;
	ApplyAcquisitionPresentation();
}

void UGridTalentDetailWidget::HandleAcquireClicked()
{
	BeginAcquireConfirmation();
}

void UGridTalentDetailWidget::HandleVariantChooseRequested(FName ChoiceId)
{
	if (!HasExclusiveVariants() || ChoiceId.IsNone()) return;
	if (!bVariantSelectionPending && !BeginVariantSelection()) return;
	SelectVariantChoice(ChoiceId);
}

void UGridTalentDetailWidget::HandleConfirmAcquireClicked()
{
	ConfirmAcquire();
}

void UGridTalentDetailWidget::HandleCancelAcquireClicked()
{
	CancelAcquireConfirmation();
}

void UGridTalentDetailWidget::RebuildVariantBlocks()
{
	if (!VB_DetailVariants || !VB_VariantEntries) return;

	VB_VariantEntries->ClearChildren();
	const bool bShowVariants =
		bInitialized && HasExclusiveVariants() && VariantBlockWidgetClass != nullptr && GetWorld() != nullptr;
	VB_DetailVariants->SetVisibility(
		bShowVariants ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (!bShowVariants) return;

	for (const FGridTalentVariantView& Variant : NodeView.Variants)
	{
		UGridTalentVariantBlockWidget* Block =
			CreateWidget<UGridTalentVariantBlockWidget>(GetWorld(), VariantBlockWidgetClass);
		if (!Block) continue;

		const bool bPending = bVariantSelectionPending && Variant.ChoiceId == SelectedVariantChoiceId;
		if (!Block->InitializeVariant(Variant, MakeVariantDisplayLabel(Variant), bPending)) continue;

		Block->OnChooseRequested.AddUniqueDynamic(this, &UGridTalentDetailWidget::HandleVariantChooseRequested);
		VB_VariantEntries->AddChildToVerticalBox(Block);
	}
}

FText UGridTalentDetailWidget::MakeVariantDisplayLabel(const FGridTalentVariantView& Variant) const
{
	FString Label = Variant.DisplayName.IsEmpty() ? Variant.ChoiceId.ToString() : Variant.DisplayName.ToString();
	const FString Conceptual = ResolvedDisplayName.ToString();

	if (!Conceptual.IsEmpty())
	{
		const TArray<FString> Prefixes = {
			Conceptual + TEXT(" — "),
			Conceptual + TEXT(" – "),
			Conceptual + TEXT(" - "),
			Conceptual + TEXT(": ")
		};
		for (const FString& Prefix : Prefixes)
		{
			if (Label.StartsWith(Prefix, ESearchCase::CaseSensitive))
			{
				Label.RightChopInline(Prefix.Len(), EAllowShrinking::No);
				break;
			}
		}
	}

	Label.TrimStartAndEndInline();
	return FText::FromString(Label);
}

FText UGridTalentDetailWidget::FormatDetailLines(const TArray<FGridTalentDetailLineView>& Lines) const
{
	TArray<FString> Formatted;
	Formatted.Reserve(Lines.Num());
	for (const FGridTalentDetailLineView& Line : Lines)
	{
		if (Line.Value.IsEmpty()) continue;
		Formatted.Add(Line.Label.IsEmpty()
			? Line.Value.ToString()
			: Line.Label.ToString() + TEXT(" : ") + Line.Value.ToString());
	}
	return FText::FromString(FString::Join(Formatted, TEXT("\n")));
}

FText UGridTalentDetailWidget::BuildAcquisitionText() const
{
	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("Niveau requis : %d"), NodeView.Acquisition.MinimumLevel));
	Lines.Add(FString::Printf(TEXT("Coût : %d point%s de Talent"),
		NodeView.Acquisition.PointCost,
		NodeView.Acquisition.PointCost > 1 ? TEXT("s") : TEXT("")));

	if (!NodeView.Acquisition.PrerequisiteTalentNames.IsEmpty())
	{
		TArray<FString> Names;
		Names.Reserve(NodeView.Acquisition.PrerequisiteTalentNames.Num());
		for (const FText& Name : NodeView.Acquisition.PrerequisiteTalentNames)
		{
			if (!Name.IsEmpty()) Names.Add(Name.ToString());
		}
		if (!Names.IsEmpty()) Lines.Add(TEXT("Prérequis : ") + FString::Join(Names, TEXT(", ")));
	}
	if (!NodeView.Acquisition.ExclusivityText.IsEmpty())
	{
		Lines.Add(NodeView.Acquisition.ExclusivityText.ToString());
	}
	if (!NodeView.Acquisition.GrantedRecipeNames.IsEmpty())
	{
		TArray<FString> Recipes;
		for (const FText& Recipe : NodeView.Acquisition.GrantedRecipeNames)
		{
			if (!Recipe.IsEmpty()) Recipes.Add(Recipe.ToString());
		}
		if (!Recipes.IsEmpty())
		{
			Lines.Add(
				(Recipes.Num() > 1 ? TEXT("Recettes : ") : TEXT("Recette : ")) +
				FString::Join(Recipes, TEXT(", ")));
		}
	}
	return FText::FromString(FString::Join(Lines, TEXT("\n")));
}

void UGridTalentDetailWidget::RefreshCanonicalSections()
{
	ResolvedTypeText = NodeView.TypeText;
	ResolvedStatusText = NodeView.StatusText;
	ResolvedPrincipleText = NodeView.Principle;
	ResolvedEffectsText = FormatDetailLines(NodeView.Effects);
	ResolvedUsageText = FormatDetailLines(NodeView.Usage);
	ResolvedAcquisitionText = BuildAcquisitionText();
}

void UGridTalentDetailWidget::ApplyDetailPresentation()
{
	if (!bInitialized) return;

	if (Border_DetailAccent)
	{
		Border_DetailAccent->SetBrushColor(BranchAccentColor);
		Border_DetailAccent->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (Text_DetailName)
	{
		Text_DetailName->SetText(ResolvedDisplayName);
		Text_DetailName->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	auto ApplyOptionalSection = [](UVerticalBox* Section, UTextBlock* Widget, const FText& Text)
	{
		const ESlateVisibility Visibility = Text.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::SelfHitTestInvisible;
		if (Section) Section->SetVisibility(Visibility);
		if (Widget)
		{
			Widget->SetText(Text);
			Widget->SetVisibility(Visibility);
		}
	};

	ApplyOptionalSection(VB_DetailType, Text_DetailType, ResolvedTypeText);
	ApplyOptionalSection(VB_DetailStatus, Text_DetailStatus, ResolvedStatusText);
	ApplyOptionalSection(VB_DetailPrinciple, Text_DetailPrinciple, ResolvedPrincipleText);
	ApplyOptionalSection(VB_DetailEffects, Text_DetailEffects, ResolvedEffectsText);
	ApplyOptionalSection(VB_DetailUsage, Text_DetailUsage, ResolvedUsageText);
	ApplyOptionalSection(VB_DetailAcquisition, Text_DetailAcquisition, ResolvedAcquisitionText);
}

void UGridTalentDetailWidget::ApplyAcquisitionPresentation()
{
	const bool bCanAcquireSimple = CanRequestSimpleAcquisition();
	const bool bAnyPending = bAcquireConfirmationPending || bVariantSelectionPending;

	if (Button_AcquireTalent)
	{
		Button_AcquireTalent->SetVisibility(
			bCanAcquireSimple && !bAnyPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Button_ConfirmAcquire)
	{
		Button_ConfirmAcquire->SetVisibility(
			bAnyPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Button_ConfirmAcquire->SetIsEnabled(
			bAcquireConfirmationPending ||
			(bVariantSelectionPending && NodeView.Variants.ContainsByPredicate(
				[this](const FGridTalentVariantView& Variant)
				{
					return Variant.ChoiceId == SelectedVariantChoiceId &&
						Variant.bCanChoose && !Variant.bAcquired;
				})));
	}
	if (Button_CancelAcquire)
	{
		Button_CancelAcquire->SetVisibility(
			bAnyPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Text_AcquirePrompt)
	{
		FText Prompt = FText::GetEmpty();
		if (bAcquireConfirmationPending)
		{
			Prompt = FText::Format(
				NSLOCTEXT("GridTalentDetail", "ConfirmAcquirePrompt", "Confirmer l’acquisition de « {0} » ?"),
				ResolvedDisplayName);
		}
		else if (bVariantSelectionPending)
		{
			FText SelectedLabel;
			if (!SelectedVariantChoiceId.IsNone() &&
				GetVariantDisplayLabel(SelectedVariantChoiceId, SelectedLabel))
			{
				Prompt = FText::Format(
					NSLOCTEXT("GridTalentDetail", "ConfirmVariantPrompt", "Confirmer « {0} » pour « {1} » ?"),
					SelectedLabel,
					ResolvedDisplayName);
			}
			else
			{
				Prompt = FText::Format(
					NSLOCTEXT("GridTalentDetail", "ChooseVariantPrompt", "Choisissez une variante pour « {0} »."),
					ResolvedDisplayName);
			}
		}
		Text_AcquirePrompt->SetText(Prompt);
	}
	if (Text_AcquireFeedback)
	{
		Text_AcquireFeedback->SetText(AcquisitionFeedback);
	}
}

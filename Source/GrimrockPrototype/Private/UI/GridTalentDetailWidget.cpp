#include "UI/GridTalentDetailWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "UI/GridTalentNodeWidget.h"

namespace GridTalentDetailWidgetPrivate
{
	FText StateText(EGridTalentNodeState State)
	{
		switch (State)
		{
			case EGridTalentNodeState::Acquired: return FText::FromString(TEXT("État : acquis"));
			case EGridTalentNodeState::Available: return FText::FromString(TEXT("État : disponible"));
			case EGridTalentNodeState::LockedLevel: return FText::FromString(TEXT("État : niveau insuffisant"));
			case EGridTalentNodeState::LockedPrerequisite: return FText::FromString(TEXT("Prérequis manquant"));
			case EGridTalentNodeState::LockedPoints: return FText::FromString(TEXT("État : points de talent insuffisants"));
			case EGridTalentNodeState::LockedExclusive: return FText::FromString(TEXT("État : choix exclusif déjà effectué"));
			default: return FText::GetEmpty();
		}
	}

}

void UGridTalentDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindAcquireButtons();
	ApplyAcquisitionPresentation();
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
	if (Button_ChooseVariant)
	{
		Button_ChooseVariant->OnClicked.RemoveDynamic(this, &UGridTalentDetailWidget::HandleChooseVariantClicked);
		Button_ChooseVariant->OnClicked.AddUniqueDynamic(this, &UGridTalentDetailWidget::HandleChooseVariantClicked);
	}
	if (Combo_VariantChoice)
	{
		Combo_VariantChoice->OnSelectionChanged.RemoveDynamic(this, &UGridTalentDetailWidget::HandleVariantSelectionChanged);
		Combo_VariantChoice->OnSelectionChanged.AddUniqueDynamic(this, &UGridTalentDetailWidget::HandleVariantSelectionChanged);
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
	if (Button_ChooseVariant)
	{
		Button_ChooseVariant->OnClicked.RemoveDynamic(this, &UGridTalentDetailWidget::HandleChooseVariantClicked);
	}
	if (Combo_VariantChoice)
	{
		Combo_VariantChoice->OnSelectionChanged.RemoveDynamic(this, &UGridTalentDetailWidget::HandleVariantSelectionChanged);
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

	const bool bCanonical = !InNodeView.DisplayName.IsEmpty() &&
		!InNodeView.TypeText.IsEmpty() && !InNodeView.StatusText.IsEmpty();
	FText DisplayName;
	FText Description;
	if (!bCanonical && !UGridTalentNodeWidget::ResolvePresentationText(
			InNodeView, InBranchPresentation, DisplayName, Description))
	{
		return false;
	}

	NodeView = InNodeView;
	// Canonical production views no longer depend on a second name resolver.
	// The legacy resolver is used only by incomplete historical fixtures until 15.5.
	bHasCanonicalDetail = bCanonical;
	ResolvedDisplayName = !NodeView.DisplayName.IsEmpty() ? NodeView.DisplayName : DisplayName;
	ResolvedDescription = !NodeView.Principle.IsEmpty() ? NodeView.Principle : Description;
	BranchAccentColor = InBranchPresentation.AccentColor;
	bInitialized = true;
	RefreshCanonicalSections();
	RebuildVariantOptions();
	RefreshVariantDetailPreview();
	ApplyDetailPresentation();
	ApplyAcquisitionPresentation();
	return true;
}

void UGridTalentDetailWidget::ClearTalentDetail()
{
	NodeView = FGridTalentNodeView();
	bHasCanonicalDetail = false;
	ResolvedDisplayName = FText::GetEmpty();
	ResolvedDescription = FText::GetEmpty();
	ResolvedTypeText = FText::GetEmpty();
	ResolvedStatusText = FText::GetEmpty();
	ResolvedPrincipleText = FText::GetEmpty();
	ResolvedEffectsText = FText::GetEmpty();
	ResolvedUsageText = FText::GetEmpty();
	ResolvedAcquisitionText = FText::GetEmpty();
	ResolvedMainDetailText = FText::GetEmpty();
	ResolvedVariantDisplayName = FText::GetEmpty();
	ResolvedVariantDescription = FText::GetEmpty();
	ResolvedActionSummary = FText::GetEmpty();
	BranchAccentColor = FLinearColor::White;
	bInitialized = false;
	bAcquireConfirmationPending = false;
	bVariantSelectionPending = false;
	SelectedVariantChoiceId = NAME_None;
	AcquisitionFeedback = FText::GetEmpty();
	VariantOptionLabels.Reset();
	VariantOptionChoiceIds.Reset();
	if (Combo_VariantChoice)
	{
		Combo_VariantChoice->ClearOptions();
		Combo_VariantChoice->ClearSelection();
	}

	if (Text_DetailName) Text_DetailName->SetText(FText::GetEmpty());
	if (Text_DetailDescription) Text_DetailDescription->SetText(FText::GetEmpty());
	if (Text_DetailType) { Text_DetailType->SetText(FText::GetEmpty()); Text_DetailType->SetVisibility(ESlateVisibility::Collapsed); }
	if (Text_DetailStatus) { Text_DetailStatus->SetText(FText::GetEmpty()); Text_DetailStatus->SetVisibility(ESlateVisibility::Collapsed); }
	if (Text_DetailPrinciple) { Text_DetailPrinciple->SetText(FText::GetEmpty()); Text_DetailPrinciple->SetVisibility(ESlateVisibility::Collapsed); }
	if (Text_DetailEffects) { Text_DetailEffects->SetText(FText::GetEmpty()); Text_DetailEffects->SetVisibility(ESlateVisibility::Collapsed); }
	if (Text_DetailUsage) { Text_DetailUsage->SetText(FText::GetEmpty()); Text_DetailUsage->SetVisibility(ESlateVisibility::Collapsed); }
	if (Text_DetailAcquisition) { Text_DetailAcquisition->SetText(FText::GetEmpty()); Text_DetailAcquisition->SetVisibility(ESlateVisibility::Collapsed); }
	if (Text_DetailLevel) Text_DetailLevel->SetText(FText::GetEmpty());
	if (Text_DetailCost) Text_DetailCost->SetText(FText::GetEmpty());
	if (Text_DetailState) Text_DetailState->SetText(FText::GetEmpty());
	if (Text_DetailVariants)
	{
		if (NodeView.Variants.Num() <= 1)
		{
			Text_DetailVariants->SetText(FText::GetEmpty());
		}
		else
		{
			FText SelectedLabel;
			if (!NodeView.SelectedChoiceId.IsNone() &&
				GetVariantDisplayLabel(NodeView.SelectedChoiceId, SelectedLabel))
			{
				Text_DetailVariants->SetText(FText::Format(
					NSLOCTEXT("GridTalentDetail", "SelectedVariant", "Variante acquise : {0} — toutes les variantes restent détaillées ci-dessous."),
					SelectedLabel));
			}
			else
			{
				Text_DetailVariants->SetText(FText::FromString(FString::Printf(
					TEXT("%d variantes — toutes détaillées ci-dessous. Le choix n'est demandé qu'au moment de l'acquisition."),
					NodeView.Variants.Num())));
			}
		}
	}

	if (Text_DetailVariantName)
	{
		Text_DetailVariantName->SetText(FText::GetEmpty());
		Text_DetailVariantName->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Text_DetailVariantDescription)
	{
		Text_DetailVariantDescription->SetText(FText::GetEmpty());
		Text_DetailVariantDescription->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Text_DetailActionSummary)
	{
		Text_DetailActionSummary->SetText(FText::GetEmpty());
		Text_DetailActionSummary->SetVisibility(ESlateVisibility::Collapsed);
	}
	ApplyAcquisitionPresentation();
}

bool UGridTalentDetailWidget::CanRequestSimpleAcquisition() const
{
	return bInitialized &&
		NodeView.State == EGridTalentNodeState::Available &&
		NodeView.Variants.Num() == 1 &&
		!NodeView.Variants[0].ChoiceId.IsNone() &&
		!NodeView.Variants[0].bSelected;
}

bool UGridTalentDetailWidget::CanRequestVariantAcquisition() const
{
	if (!bInitialized ||
		NodeView.State != EGridTalentNodeState::Available ||
		NodeView.Variants.Num() <= 1)
	{
		return false;
	}

	return NodeView.Variants.ContainsByPredicate(
		[](const FGridTalentVariantView& Variant)
		{
			return !Variant.ChoiceId.IsNone() && Variant.bAvailable && !Variant.bSelected;
		});
}

bool UGridTalentDetailWidget::BeginAcquireConfirmation()
{
	if (!CanRequestSimpleAcquisition())
	{
		return false;
	}

	bAcquireConfirmationPending = true;
	bVariantSelectionPending = false;
	SelectedVariantChoiceId = NAME_None;
	AcquisitionFeedback = FText::GetEmpty();
	ApplyAcquisitionPresentation();
	return true;
}

bool UGridTalentDetailWidget::BeginVariantSelection()
{
	if (!CanRequestVariantAcquisition())
	{
		return false;
	}

	bAcquireConfirmationPending = false;
	bVariantSelectionPending = true;
	SelectedVariantChoiceId = NAME_None;
	AcquisitionFeedback = FText::GetEmpty();
	if (Combo_VariantChoice)
	{
		Combo_VariantChoice->ClearSelection();
	}
	RefreshVariantDetailPreview();
	ApplyDetailPresentation();
	ApplyAcquisitionPresentation();
	return true;
}

bool UGridTalentDetailWidget::SelectVariantChoice(FName ChoiceId)
{
	if (!bInitialized || !bVariantSelectionPending || NodeView.Variants.Num() <= 1 || ChoiceId.IsNone())
	{
		return false;
	}

	const FGridTalentVariantView* Variant = NodeView.Variants.FindByPredicate(
		[ChoiceId](const FGridTalentVariantView& Candidate)
		{
			return Candidate.ChoiceId == ChoiceId;
		});

	if (!Variant)
	{
		return false;
	}

	// Inspection never grants acquisition rights, including for locked variants.
	SelectedVariantChoiceId = ChoiceId;
	RefreshVariantDetailPreview();
	ApplyDetailPresentation();
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
	if (!Variant)
	{
		return false;
	}

	OutLabel = MakeVariantDisplayLabel(*Variant);
	return !OutLabel.IsEmpty();
}

void UGridTalentDetailWidget::CancelAcquireConfirmation()
{
	bAcquireConfirmationPending = false;
	bVariantSelectionPending = false;
	SelectedVariantChoiceId = NAME_None;
	AcquisitionFeedback = FText::GetEmpty();
	if (Combo_VariantChoice)
	{
		Combo_VariantChoice->ClearSelection();
	}
	RefreshVariantDetailPreview();
	ApplyDetailPresentation();
	ApplyAcquisitionPresentation();
}

bool UGridTalentDetailWidget::ConfirmAcquire()
{
	if (!bInitialized)
	{
		return false;
	}

	FName ChoiceId = NAME_None;
	if (bAcquireConfirmationPending && NodeView.Variants.Num() == 1)
	{
		ChoiceId = NodeView.Variants[0].ChoiceId;
	}
	else if (bVariantSelectionPending)
	{
		ChoiceId = SelectedVariantChoiceId;
	}

	if (ChoiceId.IsNone())
	{
		return false;
	}

	const FGridTalentVariantView* Variant = NodeView.Variants.FindByPredicate(
		[ChoiceId](const FGridTalentVariantView& Candidate)
		{
			return Candidate.ChoiceId == ChoiceId;
		});
	if (!Variant || !Variant->bAvailable || Variant->bSelected)
	{
		return false;
	}

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

void UGridTalentDetailWidget::HandleChooseVariantClicked()
{
	BeginVariantSelection();
}

void UGridTalentDetailWidget::HandleVariantSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	(void)SelectionType;
	const int32 Index = VariantOptionLabels.IndexOfByKey(SelectedItem);
	if (VariantOptionChoiceIds.IsValidIndex(Index))
	{
		SelectVariantChoice(VariantOptionChoiceIds[Index]);
	}
}

void UGridTalentDetailWidget::HandleConfirmAcquireClicked()
{
	ConfirmAcquire();
}

void UGridTalentDetailWidget::HandleCancelAcquireClicked()
{
	CancelAcquireConfirmation();
}

void UGridTalentDetailWidget::RebuildVariantOptions()
{
	VariantOptionLabels.Reset();
	VariantOptionChoiceIds.Reset();

	if (Combo_VariantChoice)
	{
		Combo_VariantChoice->ClearOptions();
		Combo_VariantChoice->ClearSelection();
	}

	if (!bInitialized || NodeView.Variants.Num() <= 1)
	{
		return;
	}

	for (const FGridTalentVariantView& Variant : NodeView.Variants)
	{
		if (Variant.ChoiceId.IsNone())
		{
			continue;
		}

		FString Label = MakeVariantDisplayLabel(Variant).ToString();
		if (Label.IsEmpty())
		{
			Label = Variant.ChoiceId.ToString();
		}

		FString UniqueLabel = Label;
		if (VariantOptionLabels.Contains(UniqueLabel))
		{
			UniqueLabel = FString::Printf(TEXT("%s [%s]"), *Label, *Variant.ChoiceId.ToString());
		}

		VariantOptionLabels.Add(UniqueLabel);
		VariantOptionChoiceIds.Add(Variant.ChoiceId);
		if (Combo_VariantChoice)
		{
			Combo_VariantChoice->AddOption(UniqueLabel);
		}
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

FText UGridTalentDetailWidget::MakePlayerReadableText(const FText& Source) const
{
	FString Text = Source.ToString();
	Text.ReplaceInline(TEXT(" WD"), TEXT(" des dégâts de l'arme"), ESearchCase::CaseSensitive);
	Text.ReplaceInline(TEXT(" PA"), TEXT(" points d'action"), ESearchCase::CaseSensitive);
	Text.ReplaceInline(TEXT("Accuracy"), TEXT("Précision"), ESearchCase::CaseSensitive);
	Text.ReplaceInline(TEXT("InitiativeModifier"), TEXT("Initiative"), ESearchCase::CaseSensitive);
	Text.ReplaceInline(TEXT("PhysicalArmor"), TEXT("armure physique"), ESearchCase::CaseSensitive);
	Text.ReplaceInline(TEXT("MagicalArmor"), TEXT("armure magique"), ESearchCase::CaseSensitive);
	Text.ReplaceInline(TEXT("RawDamage"), TEXT("dégâts bruts"), ESearchCase::CaseSensitive);
	Text.ReplaceInline(TEXT("MaxHP"), TEXT("PV maximum"), ESearchCase::CaseSensitive);
	Text.ReplaceInline(TEXT("ArmorGate"), TEXT("condition d'armure"), ESearchCase::CaseSensitive);
	Text.ReplaceInline(TEXT("Status_Stunned"), TEXT("Étourdi"), ESearchCase::CaseSensitive);
	Text.ReplaceInline(TEXT("Status_Burning"), TEXT("Brûlure"), ESearchCase::CaseSensitive);
	Text.ReplaceInline(TEXT("Status_Poison"), TEXT("Poison"), ESearchCase::CaseSensitive);
	Text.ReplaceInline(TEXT("Status_Bleeding"), TEXT("Saignement"), ESearchCase::CaseSensitive);
	Text.ReplaceInline(TEXT("Status_Slow"), TEXT("Ralentissement"), ESearchCase::CaseSensitive);
	Text.ReplaceInline(TEXT("Status_Silence"), TEXT("Silence"), ESearchCase::CaseSensitive);
	Text.ReplaceInline(TEXT("Status_Immobilized"), TEXT("Immobilisé"), ESearchCase::CaseSensitive);
	Text.ReplaceInline(TEXT("Status_Banished"), TEXT("Banni"), ESearchCase::CaseSensitive);
	Text.ReplaceInline(TEXT("bSkipActivation=true"), TEXT("fait perdre la prochaine activation"), ESearchCase::CaseSensitive);
	Text.ReplaceInline(TEXT("Spell.School."), TEXT("école "), ESearchCase::CaseSensitive);

	const TCHAR* Prefixes[] = { TEXT("Status_"), TEXT("Skill_"), TEXT("Action_"), TEXT("Recipe_"), TEXT("Item_"), TEXT("Surface_") };
	for (const TCHAR* Prefix : Prefixes)
	{
		int32 SearchFrom = 0;
		while (SearchFrom < Text.Len())
		{
			const int32 Start = Text.Find(Prefix, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
			if (Start == INDEX_NONE) break;
			const int32 PrefixLength = FCString::Strlen(Prefix);
			int32 End = Start + PrefixLength;
			while (End < Text.Len() && (FChar::IsAlnum(Text[End]) || Text[End] == TCHAR('_')))
			{
				++End;
			}
			FString Token = Text.Mid(Start + PrefixLength, End - Start - PrefixLength);
			Token.ReplaceInline(TEXT("_"), TEXT(" "));
			Text = Text.Left(Start) + Token + Text.Mid(End);
			SearchFrom = Start + Token.Len();
		}
	}
	return FText::FromString(Text);
}

FText UGridTalentDetailWidget::BuildVariantOverview() const
{
	if (!bInitialized || NodeView.Variants.Num() <= 1)
	{
		return FText::GetEmpty();
	}

	TArray<FString> Blocks;
	Blocks.Reserve(NodeView.Variants.Num());
	for (const FGridTalentVariantView& Variant : NodeView.Variants)
	{
		FString Header = MakeVariantDisplayLabel(Variant).ToString();
		if (Variant.bSelected)
		{
			Header += TEXT(" — CHOISIE");
		}
		else if (bVariantSelectionPending && Variant.ChoiceId == SelectedVariantChoiceId)
		{
			Header += TEXT(" — SÉLECTIONNÉE");
		}

		TArray<FString> Lines;
		Lines.Add(Header);
		if (!Variant.EffectCategory.IsEmpty())
		{
			Lines.Add(FString(TEXT("Type : ")) + Variant.EffectCategory.ToString());
		}
		if (!Variant.Description.IsEmpty())
		{
			Lines.Add(FString(TEXT("Fonctionnement : ")) + MakePlayerReadableText(Variant.Description).ToString());
		}
		if (!Variant.MechanicsSummary.IsEmpty())
		{
			Lines.Add(FString(TEXT("Effets : ")) + MakePlayerReadableText(Variant.MechanicsSummary).ToString());
		}
		if (!Variant.UnlockedActions.IsEmpty())
		{
			Lines.Add(BuildActionSummary(Variant, Variant.bSelected).ToString());
		}
		Blocks.Add(FString::Join(Lines, TEXT("\n")));
	}
	return FText::FromString(FString::Join(Blocks, TEXT("\n\n")));
}

FText UGridTalentDetailWidget::BuildMainDetailText() const
{
	if (!bInitialized || NodeView.Variants.IsEmpty())
	{
		return FText::GetEmpty();
	}

	TArray<FString> Parts;
	if (NodeView.Variants.Num() > 1)
	{
		Parts.Add(TEXT("TYPE\nCHOIX DE VARIANTE"));
		if (!ResolvedDescription.IsEmpty())
		{
			Parts.Add(FString(TEXT("FONCTIONNEMENT\n")) + MakePlayerReadableText(ResolvedDescription).ToString());
		}
		return FText::FromString(FString::Join(Parts, TEXT("\n\n")));
	}

	const FGridTalentVariantView& Variant = NodeView.Variants[0];
	Parts.Add(FString(TEXT("TYPE\n")) +
		(Variant.EffectCategory.IsEmpty() ? FString(TEXT("TALENT")) : Variant.EffectCategory.ToString()));
	if (!ResolvedDescription.IsEmpty())
	{
		Parts.Add(FString(TEXT("FONCTIONNEMENT\n")) + MakePlayerReadableText(ResolvedDescription).ToString());
	}
	if (!Variant.MechanicsSummary.IsEmpty())
	{
		Parts.Add(FString(TEXT("EFFETS\n")) + MakePlayerReadableText(Variant.MechanicsSummary).ToString());
	}
	return FText::FromString(FString::Join(Parts, TEXT("\n\n")));
}

FText UGridTalentDetailWidget::FormatDetailLines(const TArray<FGridTalentDetailLineView>& Lines) const
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

FText UGridTalentDetailWidget::BuildAcquisitionText() const
{
	if (!bHasCanonicalDetail)
	{
		return FText::GetEmpty();
	}

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
		if (!Recipes.IsEmpty()) Lines.Add(TEXT("Recette : ") + FString::Join(Recipes, TEXT(", ")));
	}
	return FText::FromString(FString::Join(Lines, TEXT("\n")));
}

void UGridTalentDetailWidget::RefreshCanonicalSections()
{
	ResolvedTypeText = FText::GetEmpty();
	ResolvedStatusText = FText::GetEmpty();
	ResolvedPrincipleText = FText::GetEmpty();
	ResolvedEffectsText = FText::GetEmpty();
	ResolvedUsageText = FText::GetEmpty();
	ResolvedAcquisitionText = FText::GetEmpty();

	if (!bHasCanonicalDetail)
	{
		return;
	}

	ResolvedTypeText = NodeView.TypeText;
	ResolvedStatusText = NodeView.StatusText;
	ResolvedPrincipleText = NodeView.Principle;
	ResolvedEffectsText = FormatDetailLines(NodeView.Effects);
	ResolvedUsageText = FormatDetailLines(NodeView.Usage);
	ResolvedAcquisitionText = BuildAcquisitionText();
}

void UGridTalentDetailWidget::RefreshVariantDetailPreview()
{
	ResolvedMainDetailText = FText::GetEmpty();
	ResolvedVariantDisplayName = FText::GetEmpty();
	ResolvedVariantDescription = FText::GetEmpty();
	ResolvedActionSummary = FText::GetEmpty();

	if (!bInitialized || NodeView.Variants.IsEmpty())
	{
		return;
	}

	if (bHasCanonicalDetail)
	{
		// Transitional aggregate for the pre-15.2 WBP; section texts below are the final presenter surface.
		TArray<FString> Sections;
		if (!ResolvedTypeText.IsEmpty()) Sections.Add(TEXT("TYPE\n") + ResolvedTypeText.ToString());
		if (!ResolvedStatusText.IsEmpty()) Sections.Add(TEXT("STATUT\n") + ResolvedStatusText.ToString());
		if (!ResolvedPrincipleText.IsEmpty()) Sections.Add(TEXT("PRINCIPE\n") + ResolvedPrincipleText.ToString());
		if (!ResolvedEffectsText.IsEmpty()) Sections.Add(TEXT("EFFETS\n") + ResolvedEffectsText.ToString());
		if (!ResolvedUsageText.IsEmpty()) Sections.Add(TEXT("UTILISATION\n") + ResolvedUsageText.ToString());
		ResolvedMainDetailText = FText::FromString(FString::Join(Sections, TEXT("\n\n")));
	}
	else
	{
		ResolvedMainDetailText = BuildMainDetailText();
	}

	if (NodeView.bHasExclusiveVariants || (!bHasCanonicalDetail && NodeView.Variants.Num() > 1))
	{
		ResolvedVariantDisplayName = FText::FromString(TEXT("VARIANTES"));
		ResolvedVariantDescription = BuildVariantOverview();
		return;
	}

	const FGridTalentVariantView& Variant = NodeView.Variants[0];
	// Canonical UTILISATION already contains the player-facing action properties.
	if (!bHasCanonicalDetail)
	{
		ResolvedActionSummary = BuildActionSummary(
			Variant,
			Variant.bSelected || NodeView.State == EGridTalentNodeState::Acquired);
	}
}

FText UGridTalentDetailWidget::BuildActionSummary(const FGridTalentVariantView& Variant, bool bAlreadyAcquired) const
{
	if (Variant.UnlockedActions.IsEmpty()) return FText::GetEmpty();

	TArray<FString> ActionBlocks;
	ActionBlocks.Reserve(Variant.UnlockedActions.Num());
	for (const FGridTalentUnlockedActionView& Action : Variant.UnlockedActions)
	{
		TArray<FString> Lines;
		Lines.Add(Action.DisplayName.IsEmpty() ? Action.ActionId.ToString() : Action.DisplayName.ToString());

		TArray<FString> Costs;
		Costs.Add(FString::Printf(TEXT("%d point%s d'action"), Action.ActionPointCost, Action.ActionPointCost > 1 ? TEXT("s") : TEXT("")));
		if (Action.ManaCost > 0) Costs.Add(FString::Printf(TEXT("%d mana"), Action.ManaCost));
		if (Action.SourceItemQuantityCost > 0)
		{
			Costs.Add(FString::Printf(TEXT("%d objet%s consommé%s"), Action.SourceItemQuantityCost,
				Action.SourceItemQuantityCost > 1 ? TEXT("s") : TEXT(""), Action.SourceItemQuantityCost > 1 ? TEXT("s") : TEXT("")));
		}
		Lines.Add(TEXT("Coût : ") + FString::Join(Costs, TEXT(" — ")));

		TArray<FString> Targeting;
		if (!Action.TargetSummary.IsEmpty()) Targeting.Add(Action.TargetSummary.ToString());
		if (Action.RangeCells > 0) Targeting.Add(FString::Printf(TEXT("portée : %d case%s"), Action.RangeCells, Action.RangeCells > 1 ? TEXT("s") : TEXT("")));
		if (Action.AreaRadiusCells > 0) Targeting.Add(FString::Printf(TEXT("zone : rayon %d case%s"), Action.AreaRadiusCells, Action.AreaRadiusCells > 1 ? TEXT("s") : TEXT("")));
		if (Action.MaximumResolvedTargets > 0) Targeting.Add(FString::Printf(TEXT("maximum %d cible%s"), Action.MaximumResolvedTargets, Action.MaximumResolvedTargets > 1 ? TEXT("s") : TEXT("")));
		if (Action.ChainJumpRangeCells > 0) Targeting.Add(FString::Printf(TEXT("enchaînement : %d case%s"), Action.ChainJumpRangeCells, Action.ChainJumpRangeCells > 1 ? TEXT("s") : TEXT("")));
		if (Action.bAreaCenteredOnParty) Targeting.Add(TEXT("zone centrée sur le groupe"));
		if (Action.bRequiresLineOfSight) Targeting.Add(TEXT("ligne de vue requise"));
		if (!Targeting.IsEmpty()) Lines.Add(TEXT("Cible : ") + FString::Join(Targeting, TEXT(" — ")));

		TArray<FString> Resolution;
		if (Action.ResolutionCount > 1)
		{
			Resolution.Add(FString::Printf(TEXT("%d résolutions"), Action.ResolutionCount));
			if (Action.SubsequentResolutionAccuracyModifier != 0)
				Resolution.Add(FString::Printf(TEXT("%+d précision à partir de la 2e"), Action.SubsequentResolutionAccuracyModifier));
		}
		if (Action.bAffectsAlliesInArea) Resolution.Add(TEXT("peut affecter les alliés dans la zone"));
		if (!Resolution.IsEmpty()) Lines.Add(TEXT("Résolution : ") + FString::Join(Resolution, TEXT(" — ")));

		if (Action.CooldownRounds > 0) Lines.Add(FString::Printf(TEXT("Recharge : %d tour%s"), Action.CooldownRounds, Action.CooldownRounds > 1 ? TEXT("s") : TEXT("")));

		FString Description = MakePlayerReadableText(Action.Description).ToString();
		Description.TrimStartAndEndInline();
		if (!Description.IsEmpty()) Lines.Add(FString(TEXT("Effet : ")) + Description);
		ActionBlocks.Add(FString::Join(Lines, TEXT("\n")));
	}
	const FString Heading = bAlreadyAcquired
		? TEXT("ACTION DISPONIBLE")
		: TEXT("ACTION ACCORDÉE APRÈS ACQUISITION");
	return FText::FromString(Heading + TEXT("\n") + FString::Join(ActionBlocks, TEXT("\n\n")));
}

void UGridTalentDetailWidget::ApplyDetailPresentation()
{
	if (!bInitialized)
	{
		return;
	}

	using namespace GridTalentDetailWidgetPrivate;

	if (Border_DetailAccent)
	{
		Border_DetailAccent->SetBrushColor(BranchAccentColor);
	}
	if (Text_DetailName)
	{
		Text_DetailName->SetText(ResolvedDisplayName);
	}
	if (Text_DetailDescription)
	{
		Text_DetailDescription->SetText(ResolvedMainDetailText);
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
	if (Text_DetailLevel)
	{
		Text_DetailLevel->SetText(
			FText::FromString(FString::Printf(TEXT("Niveau requis : %d"),
				bHasCanonicalDetail ? NodeView.Acquisition.MinimumLevel : NodeView.MinimumLevel)));
	}
	if (Text_DetailCost)
	{
		Text_DetailCost->SetText(
			FText::FromString(FString::Printf(TEXT("Coût : %d point%s"),
				bHasCanonicalDetail ? NodeView.Acquisition.PointCost : NodeView.PointCost,
				(bHasCanonicalDetail ? NodeView.Acquisition.PointCost : NodeView.PointCost) > 1 ? TEXT("s") : TEXT(""))));
	}
	if (Text_DetailState)
	{
		if (bHasCanonicalDetail)
		{
			Text_DetailState->SetText(NodeView.StatusText);
		}
		else if (NodeView.State == EGridTalentNodeState::LockedPrerequisite && !NodeView.PreviousNodeDisplayName.IsEmpty())
		{
			Text_DetailState->SetText(FText::Format(NSLOCTEXT("GridTalentDetail", "MissingNamedPrerequisite", "Prérequis manquant : {0}"), NodeView.PreviousNodeDisplayName));
		}
		else if (NodeView.State == EGridTalentNodeState::Available && !NodeView.PreviousNodeDisplayName.IsEmpty())
		{
			Text_DetailState->SetText(FText::Format(NSLOCTEXT("GridTalentDetail", "SatisfiedNamedPrerequisite", "Prérequis : {0} — satisfait"), NodeView.PreviousNodeDisplayName));
		}
		else Text_DetailState->SetText(StateText(NodeView.State));
	}
	if (Text_DetailVariants)
	{
		if (NodeView.Variants.Num() <= 1)
		{
			Text_DetailVariants->SetText(FText::GetEmpty());
		}
		else
		{
			TArray<FString> Labels;
			Labels.Reserve(NodeView.Variants.Num());
			for (const FGridTalentVariantView& Variant : NodeView.Variants)
			{
				Labels.Add(MakeVariantDisplayLabel(Variant).ToString());
			}

			FText SelectedLabel;
			if (!NodeView.SelectedChoiceId.IsNone() &&
				GetVariantDisplayLabel(NodeView.SelectedChoiceId, SelectedLabel))
			{
				Text_DetailVariants->SetText(
					FText::Format(
						NSLOCTEXT("GridTalentDetail", "SelectedVariant", "Variante choisie : {0}"),
						SelectedLabel));
			}
			else
			{
				Text_DetailVariants->SetText(
					FText::FromString(FString::Printf(TEXT("Variantes : %s"), *FString::Join(Labels, TEXT(" / ")))));
			}
		}
	}

	if (Text_DetailVariantName)
	{
		Text_DetailVariantName->SetText(ResolvedVariantDisplayName);
		Text_DetailVariantName->SetVisibility(
			ResolvedVariantDisplayName.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
	if (Text_DetailVariantDescription)
	{
		Text_DetailVariantDescription->SetText(ResolvedVariantDescription);
		Text_DetailVariantDescription->SetVisibility(
			ResolvedVariantDescription.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
	if (Text_DetailActionSummary)
	{
		Text_DetailActionSummary->SetText(ResolvedActionSummary);
		Text_DetailActionSummary->SetVisibility(
			ResolvedActionSummary.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
}

void UGridTalentDetailWidget::ApplyAcquisitionPresentation()
{
	const bool bCanAcquireSimple = CanRequestSimpleAcquisition();
	const bool bCanAcquireVariant = CanRequestVariantAcquisition();
	const bool bAnyPending = bAcquireConfirmationPending || bVariantSelectionPending;

	if (Button_AcquireTalent)
	{
		Button_AcquireTalent->SetVisibility(
			bCanAcquireSimple && !bAnyPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Button_ChooseVariant)
	{
		Button_ChooseVariant->SetVisibility(
			bCanAcquireVariant && !bAnyPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Combo_VariantChoice)
	{
		Combo_VariantChoice->SetVisibility(
			bVariantSelectionPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Button_ConfirmAcquire)
	{
		Button_ConfirmAcquire->SetVisibility(
			bAnyPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Button_ConfirmAcquire->SetIsEnabled(
			bAcquireConfirmationPending || (bVariantSelectionPending && NodeView.Variants.ContainsByPredicate(
			[this](const FGridTalentVariantView& Variant)
			{
				return Variant.ChoiceId == SelectedVariantChoiceId && Variant.bAvailable && !Variant.bSelected;
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

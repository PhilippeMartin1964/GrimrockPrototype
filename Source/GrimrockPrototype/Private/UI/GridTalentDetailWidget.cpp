#include "UI/GridTalentDetailWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/World.h"
#include "UI/GridTalentDetailLineWidget.h"
#include "UI/RPGTalentDetailGlyphSet.h"
#include "UI/RPGTalentDetailTextSet.h"

namespace
{
	FName TalentTypeTextId(ERPGTalentPresentationType Type)
	{
		switch (Type)
		{
			case ERPGTalentPresentationType::Active: return TEXT("Talent.Type.Active");
			case ERPGTalentPresentationType::ActiveSpell: return TEXT("Talent.Type.ActiveSpell");
			case ERPGTalentPresentationType::Passive: return TEXT("Talent.Type.Passive");
			case ERPGTalentPresentationType::AutomaticReaction: return TEXT("Talent.Type.AutomaticReaction");
			case ERPGTalentPresentationType::RecipeQuickItem: return TEXT("Talent.Type.RecipeQuickItem");
			case ERPGTalentPresentationType::RecipeActive: return TEXT("Talent.Type.RecipeActive");
			default: return NAME_None;
		}
	}

	FName TalentStatusTextId(EGridTalentNodeState State)
	{
		switch (State)
		{
			case EGridTalentNodeState::Acquired: return TEXT("Talent.Status.Acquired");
			case EGridTalentNodeState::Available: return TEXT("Talent.Status.Available");
			case EGridTalentNodeState::LockedLevel: return TEXT("Talent.Status.LockedLevel");
			case EGridTalentNodeState::LockedPrerequisite: return TEXT("Talent.Status.LockedPrerequisite");
			case EGridTalentNodeState::LockedPoints: return TEXT("Talent.Status.LockedPoints");
			case EGridTalentNodeState::LockedExclusive: return TEXT("Talent.Status.LockedExclusive");
			default: return NAME_None;
		}
	}
}

void UGridTalentDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindAcquireButtons();
	ApplyLocalizedStaticLabels();
	if (bInitialized)
	{
		RebuildDetailLineRows();
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

void UGridTalentDetailWidget::ApplyLocalizedStaticLabels()
{
	auto SetResolvedText = [](UTextBlock* Widget, const TCHAR* TextId)
	{
		if (Widget)
		{
			Widget->SetText(URPGTalentDetailTextSet::ResolveProductionText(FName(TextId)));
		}
	};

	SetResolvedText(Text_LabelType, TEXT("Talent.Section.Type"));
	SetResolvedText(Text_LabelStatus, TEXT("Talent.Section.Status"));
	SetResolvedText(Text_LabelPrinciple, TEXT("Talent.Section.Principle"));
	SetResolvedText(Text_LabelEffects, TEXT("Talent.Section.Effects"));
	SetResolvedText(Text_LabelUsage, TEXT("Talent.Section.Usage"));
	SetResolvedText(Text_LabelAcquisition, TEXT("Talent.Section.Acquisition"));
	SetResolvedText(Text_AcquireTalent, TEXT("Talent.Action.Acquire"));
	SetResolvedText(Text_ConfirmAcquire, TEXT("Talent.Action.Confirm"));
}

void UGridTalentDetailWidget::SetTalentTypeImage(UTexture2D* ImageTexture)
{
	if (!Image_TalentType)
	{
		return;
	}
	Image_TalentType->SetBrushFromTexture(ImageTexture, false);
	Image_TalentType->SetVisibility(ImageTexture ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
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
		InNodeView.Type == ERPGTalentPresentationType::None ||
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
	RebuildDetailLineRows();
	ApplyDetailPresentation();
	ApplyAcquisitionPresentation();
	return true;
}

void UGridTalentDetailWidget::SetTalentIcon(UTexture2D* IconTexture)
{
	const ESlateVisibility IconVisibility = IconTexture ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
	if (Image_DetailTalentIcon)
	{
		Image_DetailTalentIcon->SetBrushFromTexture(IconTexture, false);
		Image_DetailTalentIcon->SetVisibility(IconVisibility);
	}
	if (Border_DetailTalentIconFrame)
	{
		Border_DetailTalentIconFrame->SetVisibility(IconVisibility);
	}
}

void UGridTalentDetailWidget::ClearTalentDetail()
{
	NodeView = FGridTalentNodeView();
	ResolvedDisplayName = FText::GetEmpty();
	ResolvedTypeText = FText::GetEmpty();
	ResolvedStatusText = FText::GetEmpty();
	ResolvedPrincipleText = FText::GetEmpty();
	BranchAccentColor = FLinearColor::White;
	bInitialized = false;
	bAcquireConfirmationPending = false;
	SetTalentIcon(nullptr);
	SetTalentTypeImage(nullptr);

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

	if (VB_DetailEffects)
	{
		VB_DetailEffects->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (VB_DetailUsage)
	{
		VB_DetailUsage->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (VB_DetailAcquisition)
	{
		VB_DetailAcquisition->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (VB_DetailEffectsEntries)
	{
		VB_DetailEffectsEntries->ClearChildren();
		VB_DetailEffectsEntries->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (VB_DetailUsageEntries)
	{
		VB_DetailUsageEntries->ClearChildren();
		VB_DetailUsageEntries->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (VB_DetailAcquisitionEntries)
	{
		VB_DetailAcquisitionEntries->ClearChildren();
		VB_DetailAcquisitionEntries->SetVisibility(ESlateVisibility::Collapsed);
	}

	ApplyAcquisitionPresentation();
}

bool UGridTalentDetailWidget::CanRequestSimpleAcquisition() const
{
	return bInitialized &&
		!NodeView.bHasExclusiveVariants &&
		NodeView.State == EGridTalentNodeState::Available &&
		NodeView.bCanAcquireSimple &&
		!NodeView.SimpleChoiceId.IsNone();
}

bool UGridTalentDetailWidget::BeginAcquireConfirmation()
{
	if (!CanRequestSimpleAcquisition()) return false;

	bAcquireConfirmationPending = true;
	ApplyAcquisitionPresentation();
	return true;
}

bool UGridTalentDetailWidget::ConfirmAcquire()
{
	if (!bInitialized || !bAcquireConfirmationPending || !CanRequestSimpleAcquisition())
	{
		return false;
	}

	const FName ChoiceId = NodeView.SimpleChoiceId;
	if (ChoiceId.IsNone())
	{
		return false;
	}

	bAcquireConfirmationPending = false;
	ApplyAcquisitionPresentation();
	OnAcquireConfirmed.Broadcast(ChoiceId);
	return true;
}

void UGridTalentDetailWidget::HandleAcquireClicked()
{
	BeginAcquireConfirmation();
}

void UGridTalentDetailWidget::HandleConfirmAcquireClicked()
{
	ConfirmAcquire();
}

void UGridTalentDetailWidget::PopulateDetailLineRows(
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

void UGridTalentDetailWidget::BuildAcquisitionLines(
	TArray<FGridTalentDetailLineView>& OutLines) const
{
	OutLines.Reset();

	auto AddLine = [&OutLines](
		EGridTalentDetailLineSemantic Semantic,
		FName LabelTextId,
		const FText& Value)
	{
		if (Value.IsEmpty())
		{
			return;
		}
		FGridTalentDetailLineView Line;
		Line.Semantic = Semantic;
		Line.LabelTextId = LabelTextId;
		Line.Value = Value;
		OutLines.Add(MoveTemp(Line));
	};

	auto AddNumberLine = [&OutLines](
		EGridTalentDetailLineSemantic Semantic,
		FName LabelTextId,
		int32 NumericValue)
	{
		FGridTalentDetailLineView Line;
		Line.Semantic = Semantic;
		Line.LabelTextId = LabelTextId;
		Line.NumericValue = NumericValue;
		Line.bHasNumericValue = true;
		Line.ValueStyle = EGridTalentDetailValueStyle::Integer;
		OutLines.Add(MoveTemp(Line));
	};

	AddNumberLine(
		EGridTalentDetailLineSemantic::LevelRequirement,
		TEXT("Talent.Detail.LevelRequirement"),
		NodeView.Acquisition.MinimumLevel);

	AddNumberLine(
		EGridTalentDetailLineSemantic::TalentPointCost,
		TEXT("Talent.Detail.TalentPointCost"),
		NodeView.Acquisition.PointCost);

	if (!NodeView.Acquisition.PrerequisiteTalentNames.IsEmpty())
	{
		TArray<FString> Names;
		for (const FText& Name : NodeView.Acquisition.PrerequisiteTalentNames)
		{
			if (!Name.IsEmpty())
			{
				Names.Add(Name.ToString());
			}
		}
		if (!Names.IsEmpty())
		{
			AddLine(
				EGridTalentDetailLineSemantic::Prerequisite,
				TEXT("Talent.Detail.Prerequisite"),
				FText::FromString(FString::Join(Names, TEXT(", "))));
		}
	}

	auto AddTextIdLine = [&OutLines](
		EGridTalentDetailLineSemantic Semantic,
		FName LabelTextId,
		FName ValueTextId)
	{
		if (ValueTextId.IsNone())
		{
			return;
		}
		FGridTalentDetailLineView Line;
		Line.Semantic = Semantic;
		Line.LabelTextId = LabelTextId;
		Line.ValueTextId = ValueTextId;
		OutLines.Add(MoveTemp(Line));
	};

	AddTextIdLine(
		EGridTalentDetailLineSemantic::Exclusivity,
		NAME_None,
		NodeView.Acquisition.ExclusivityTextId);

	const int32 RecipeCount = NodeView.Acquisition.GrantedRecipeTextIds.Num();
	for (int32 RecipeIndex = 0; RecipeIndex < RecipeCount; ++RecipeIndex)
	{
		const FName LabelTextId = RecipeIndex == 0
			? (RecipeCount > 1 ? FName(TEXT("Talent.Detail.Recipes")) : FName(TEXT("Talent.Detail.Recipe")))
			: NAME_None;
		AddTextIdLine(
			EGridTalentDetailLineSemantic::Recipe,
			LabelTextId,
			NodeView.Acquisition.GrantedRecipeTextIds[RecipeIndex]);
	}
}

void UGridTalentDetailWidget::RebuildDetailLineRows()
{
	PopulateDetailLineRows(VB_DetailEffectsEntries, NodeView.Effects);
	PopulateDetailLineRows(VB_DetailUsageEntries, NodeView.Usage);

	TArray<FGridTalentDetailLineView> AcquisitionLines;
	BuildAcquisitionLines(AcquisitionLines);
	PopulateDetailLineRows(VB_DetailAcquisitionEntries, AcquisitionLines);
}

void UGridTalentDetailWidget::RefreshCanonicalSections()
{
	ResolvedTypeText = URPGTalentDetailTextSet::ResolveProductionText(TalentTypeTextId(NodeView.Type));
	ResolvedStatusText = URPGTalentDetailTextSet::ResolveProductionText(TalentStatusTextId(NodeView.State));
	ResolvedPrincipleText = NodeView.Principle;
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

	auto ApplyStructuredSection = [](UVerticalBox* Section, UVerticalBox* Entries)
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

	ApplyStructuredSection(VB_DetailEffects, VB_DetailEffectsEntries);
	ApplyStructuredSection(VB_DetailUsage, VB_DetailUsageEntries);

	const bool bHasAcquisitionRows =
		VB_DetailAcquisitionEntries && VB_DetailAcquisitionEntries->GetChildrenCount() > 0;
	const bool bHasAcquisitionAction = CanRequestSimpleAcquisition() || bAcquireConfirmationPending;
	const ESlateVisibility AcquisitionVisibility =
		(bHasAcquisitionRows || bHasAcquisitionAction)
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed;

	if (VB_DetailAcquisition)
	{
		VB_DetailAcquisition->SetVisibility(AcquisitionVisibility);
	}
	if (VB_DetailAcquisitionEntries)
	{
		VB_DetailAcquisitionEntries->SetVisibility(
			bHasAcquisitionRows ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UGridTalentDetailWidget::ApplyAcquisitionPresentation()
{
	const bool bCanAcquire = CanRequestSimpleAcquisition();

	if (Button_AcquireTalent)
	{
		Button_AcquireTalent->SetVisibility(
			bCanAcquire && !bAcquireConfirmationPending
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
	}

	if (Button_ConfirmAcquire)
	{
		Button_ConfirmAcquire->SetVisibility(
			bAcquireConfirmationPending
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
		Button_ConfirmAcquire->SetIsEnabled(bAcquireConfirmationPending && bCanAcquire);
	}
}

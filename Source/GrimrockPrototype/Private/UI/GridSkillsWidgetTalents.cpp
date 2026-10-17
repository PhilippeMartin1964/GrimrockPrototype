#include "UI/GridSkillsWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/Texture2D.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGClassVisualAsset.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillPointService.h"
#include "UI/GridSkillDetailWidget.h"
#include "UI/GridSkillEntryWidget.h"
#include "UI/GridSkillsPageService.h"
#include "UI/GridTalentBranchWidget.h"
#include "UI/GridTalentDetailWidget.h"
#include "UI/GridTalentNodeWidget.h"
#include "UI/GridTalentVariantFlyoutWidget.h"
#include "UI/GridRPGNotificationWidget.h"
#include "UI/RPGProgressionFeedbackService.h"
#include "UI/RPGSkillPresentationAsset.h"
#include "UI/UILocalizationText.h"

bool UGridSkillsWidget::GetCurrentClassPresentation(FRPGClassPresentationDefinition& OutPresentation) const
{
	OutPresentation = FRPGClassPresentationDefinition();
	if (!View.IsValid() || View.ClassId.IsNone())
	{
		return false;
	}

	const URPGTalentPresentationAsset* Catalog = URPGTalentPresentationAsset::ResolveCanonical();
	return Catalog && Catalog->GetClassPresentation(View.ClassId, OutPresentation);
}

bool UGridSkillsWidget::GetPresentedTalentBranch(
	int32 VisualIndex,
	FGridTalentBranchView& OutBranch,
	FRPGTalentBranchPresentationDefinition& OutPresentation) const
{
	OutBranch = FGridTalentBranchView();
	OutPresentation = FRPGTalentBranchPresentationDefinition();

	FRPGClassPresentationDefinition ClassPresentation;
	if (GetCurrentClassPresentation(ClassPresentation) && ClassPresentation.Branches.IsValidIndex(VisualIndex))
	{
		OutPresentation = ClassPresentation.Branches[VisualIndex];
		if (const FGridTalentBranchView* Branch = View.TalentTree.Branches.FindByPredicate(
			[&OutPresentation](const FGridTalentBranchView& Candidate)
			{
				return Candidate.TalentBranchId == OutPresentation.TalentBranchId;
			}))
		{
			OutBranch = *Branch;
			return true;
		}
		return false;
	}

	return false;
}

UGridTalentNodeWidget* UGridSkillsWidget::FindTalentNodeWidget(const FGridTalentNodeView& NodeView) const
{
	for (UGridTalentBranchWidget* BranchWidget : { Branch_Left.Get(), Branch_Center.Get(), Branch_Right.Get() })
	{
		if (BranchWidget && BranchWidget->BranchView.TalentBranchId == NodeView.TalentBranchId)
		{
			return BranchWidget->GetTalentNodeWidgetForTier(NodeView.Tier);
		}
	}
	return nullptr;
}

bool UGridSkillsWidget::OpenTalentVariantFlyout(FName TalentNodeId)
{
	const FGridTalentNodeView* Node = FindTalentNode(TalentNodeId);
	if (!Node || !Node->bHasExclusiveVariants || !Node->SelectedChoiceId.IsNone() || Node->Variants.Num() < 2)
	{
		return false;
	}

	UGridTalentNodeWidget* NodeWidget = FindTalentNodeWidget(*Node);
	if (!NodeWidget || !NodeWidget->GetVariantFlyoutWidgetClass())
	{
		return false;
	}

	CloseTalentVariantFlyout();

	UGridTalentVariantFlyoutWidget* Flyout =
		CreateWidget<UGridTalentVariantFlyoutWidget>(this, NodeWidget->GetVariantFlyoutWidgetClass());
	if (!Flyout || !Flyout->InitializeFlyout(*Node, NodeWidget->GetVariantIcons()))
	{
		return false;
	}

	ActiveVariantFlyout = Flyout;
	ActiveVariantFlyoutNodeId = TalentNodeId;

	Flyout->OnVariantChosen.AddUniqueDynamic(this, &UGridSkillsWidget::HandleTalentVariantFlyoutChosen);

	// Keep the popup in the normal game viewport hierarchy. The project's custom
	// Grimrock cursor is Z=9999, so this popup deliberately sits just below it.
	Flyout->AddToViewport(9000);
	Flyout->SetAlignmentInViewport(FVector2D::ZeroVector);
	Flyout->ForceLayoutPrepass();

	const FGeometry& NodeGeometry = NodeWidget->GetCachedGeometry();
	const FVector2D NodeSize = NodeGeometry.GetLocalSize();
	FVector2D PixelPosition;
	FVector2D ViewportPosition;
	USlateBlueprintLibrary::LocalToViewport(
		this,
		NodeGeometry,
		FVector2D(NodeSize.X + 8.0f, 0.0f),
		PixelPosition,
		ViewportPosition);

	const FVector2D DesiredSize = Flyout->GetDesiredSize();
	const float ViewportScale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), KINDA_SMALL_NUMBER);
	const FVector2D ViewportSize =
		UWidgetLayoutLibrary::GetViewportSize(this) / ViewportScale;

	if (ViewportPosition.X + DesiredSize.X > ViewportSize.X - 8.0f)
	{
		FVector2D LeftPixelPosition;
		FVector2D LeftViewportPosition;
		USlateBlueprintLibrary::LocalToViewport(
			this,
			NodeGeometry,
			FVector2D::ZeroVector,
			LeftPixelPosition,
			LeftViewportPosition);
		ViewportPosition.X = FMath::Max(8.0f, LeftViewportPosition.X - DesiredSize.X - 8.0f);
	}

	ViewportPosition.Y = FMath::Clamp(
		ViewportPosition.Y,
		8.0f,
		FMath::Max(8.0f, ViewportSize.Y - DesiredSize.Y - 8.0f));

	// ViewportPosition is already in widget/viewport space, so no inverse-DPI
	// transform must be applied again.
	Flyout->SetPositionInViewport(ViewportPosition, false);
	return true;
}

void UGridSkillsWidget::CloseTalentVariantFlyout()
{
	if (ActiveVariantFlyout)
	{
		ActiveVariantFlyout->OnVariantChosen.RemoveDynamic(
			this, &UGridSkillsWidget::HandleTalentVariantFlyoutChosen);
		ActiveVariantFlyout->RemoveFromParent();
		ActiveVariantFlyout = nullptr;
	}

	ActiveVariantFlyoutNodeId = NAME_None;
}

bool UGridSkillsWidget::SelectTalentNode(FName TalentNodeId)
{
	if (!View.IsValid() || !FindTalentNode(TalentNodeId))
	{
		return false;
	}

	const bool bChanged = SelectedTalentNodeId != TalentNodeId || !SelectedTalentChoiceId.IsNone();
	SelectedTalentNodeId = TalentNodeId;
	SelectedTalentChoiceId = NAME_None;
	if (bChanged)
	{
		OnTalentSelectionChanged.Broadcast(SelectedTalentNodeId);
	}
	return true;
}

bool UGridSkillsWidget::SelectTalentVariant(FName TalentNodeId, FName ChoiceId)
{
	if (!View.IsValid() || TalentNodeId.IsNone() || ChoiceId.IsNone())
	{
		return false;
	}

	const FGridTalentNodeView* Node = FindTalentNode(TalentNodeId);
	if (!Node || !Node->bHasExclusiveVariants || !Node->SelectedChoiceId.IsNone())
	{
		return false;
	}

	const bool bKnownVariant = Node->Variants.ContainsByPredicate(
		[ChoiceId](const FGridTalentVariantView& Variant)
		{
			return Variant.ChoiceId == ChoiceId;
		});
	if (!bKnownVariant)
	{
		return false;
	}

	const bool bChanged = SelectedTalentNodeId != TalentNodeId || SelectedTalentChoiceId != ChoiceId;
	SelectedTalentNodeId = TalentNodeId;
	SelectedTalentChoiceId = ChoiceId;
	if (bChanged)
	{
		OnTalentSelectionChanged.Broadcast(SelectedTalentNodeId);
	}
	return true;
}

void UGridSkillsWidget::ClearTalentSelection()
{
	if (SelectedTalentNodeId.IsNone() && SelectedTalentChoiceId.IsNone())
	{
		return;
	}

	SelectedTalentNodeId = NAME_None;
	SelectedTalentChoiceId = NAME_None;
	OnTalentSelectionChanged.Broadcast(NAME_None);
}

bool UGridSkillsWidget::GetSelectedTalentNode(FGridTalentNodeView& OutNode) const
{
	OutNode = FGridTalentNodeView();
	if (const FGridTalentNodeView* Node = FindTalentNode(SelectedTalentNodeId))
	{
		OutNode = *Node;
		return true;
	}
	return false;
}

bool UGridSkillsWidget::BuildSelectedTalentDetailView(FGridTalentDetailView& OutDetailView) const
{
	OutDetailView = FGridTalentDetailView();
	const FGridTalentNodeView* Node = FindTalentNode(SelectedTalentNodeId);
	if (!Node)
	{
		return false;
	}

	const FName ChoiceId = Node->bHasExclusiveVariants
		? (!Node->SelectedChoiceId.IsNone() ? Node->SelectedChoiceId : SelectedTalentChoiceId)
		: Node->SimpleChoiceId;
	if (Node->bHasExclusiveVariants && ChoiceId.IsNone())
	{
		return false;
	}

	return FGridTalentDetailView::TryBuildFromNode(*Node, ChoiceId, OutDetailView);
}

bool UGridSkillsWidget::CommitConfirmedTalentChoice(FName ChoiceId, FText& OutFeedback)
{
	OutFeedback = FText::GetEmpty();

	FGridTalentNodeView SelectedNode;
	if (!IsValid(InventoryComponent) ||
		!View.IsValid() ||
		!GetSelectedTalentNode(SelectedNode) ||
		ChoiceId.IsNone())
	{
		FRPGClassProgressionCommitResult InvalidResult;
		InvalidResult.RejectReason = ERPGClassProgressionCommitRejectReason::InvalidCurrentSelection;
		const FRPGProgressionNotificationView Notification =
			FRPGProgressionFeedbackService::MakeTalentCommitNotification(InvalidResult, FText::GetEmpty());
		OutFeedback = Notification.Message;
		PublishProgressionNotification(Notification);
		return false;
	}

	FText RequestedDisplayName = SelectedNode.DisplayName;
	if (SelectedNode.bHasExclusiveVariants)
	{
		const FGridTalentVariantView* RequestedVariant = SelectedNode.Variants.FindByPredicate(
			[ChoiceId](const FGridTalentVariantView& Variant)
			{
				return Variant.ChoiceId == ChoiceId;
			});
		if (!RequestedVariant)
		{
			FRPGClassProgressionCommitResult InvalidResult;
			InvalidResult.RejectReason = ERPGClassProgressionCommitRejectReason::UnknownChoice;
			const FRPGProgressionNotificationView Notification =
				FRPGProgressionFeedbackService::MakeTalentCommitNotification(InvalidResult, FText::GetEmpty());
			OutFeedback = Notification.Message;
			PublishProgressionNotification(Notification);
			return false;
		}
		RequestedDisplayName = RequestedVariant->DisplayName;
	}
	else if (SelectedNode.SimpleChoiceId != ChoiceId)
	{
		FRPGClassProgressionCommitResult InvalidResult;
		InvalidResult.RejectReason = ERPGClassProgressionCommitRejectReason::UnknownChoice;
		const FRPGProgressionNotificationView Notification =
			FRPGProgressionFeedbackService::MakeTalentCommitNotification(InvalidResult, FText::GetEmpty());
		OutFeedback = Notification.Message;
		PublishProgressionNotification(Notification);
		return false;
	}
	FRPGClassProgressionCommitResult Result;
	const bool bCommitted = FRPGClassProgressionTransactionService::TryCommitChoices(
		InventoryComponent, View.CharacterIndex, { ChoiceId }, Result);

	const FRPGProgressionNotificationView Notification =
		FRPGProgressionFeedbackService::MakeTalentCommitNotification(Result, RequestedDisplayName);
	OutFeedback = Notification.Message;
	PublishProgressionNotification(Notification);

	if (!bCommitted)
	{
		RefreshSkills();
		return false;
	}

	return true;
}

void UGridSkillsWidget::ApplyTalentDetailPresentation()
{
	if (!Detail_Talent)
	{
		return;
	}

	FGridTalentNodeView ConceptNode;
	FGridTalentDetailView DetailView;
	if (!GetSelectedTalentNode(ConceptNode) || !BuildSelectedTalentDetailView(DetailView))
	{
		Detail_Talent->ClearTalentDetail();
		return;
	}

	FRPGClassPresentationDefinition ClassPresentation;
	if (!GetCurrentClassPresentation(ClassPresentation))
	{
		Detail_Talent->ClearTalentDetail();
		return;
	}

	const FRPGTalentBranchPresentationDefinition* BranchPresentation =
		ClassPresentation.FindBranch(DetailView.TalentBranchId);
	if (!BranchPresentation ||
		!Detail_Talent->InitializeTalentDetail(DetailView, *BranchPresentation))
	{
		Detail_Talent->ClearTalentDetail();
		return;
	}

	UTexture2D* IconTexture = nullptr;
	UTexture2D* DetailPanelTexture = nullptr;
	if (const URPGClassVisualAsset* ClassVisual = ResolveCurrentClassVisual())
	{
		if (ConceptNode.bHasExclusiveVariants && !DetailView.ChoiceId.IsNone())
		{
			if (const FRPGTalentVariantVisualDefinition* VariantVisual =
				ClassVisual->FindTalentVariantVisual(DetailView.ChoiceId))
			{
				IconTexture = VariantVisual->Icon.LoadSynchronous();
			}
		}
		if (!IconTexture)
		{
			if (const FRPGTalentNodeVisualDefinition* NodeVisual =
				ClassVisual->FindTalentNodeVisual(ConceptNode.TalentNodeId))
			{
				IconTexture = NodeVisual->Icon.LoadSynchronous();
			}
		}
		if (const FRPGTalentBranchVisualDefinition* BranchVisual =
			ClassVisual->FindTalentBranchVisual(DetailView.TalentBranchId))
		{
			DetailPanelTexture = BranchVisual->DetailPanelImage.LoadSynchronous();
		}
	}

	Detail_Talent->SetTalentIcon(IconTexture);
	Detail_Talent->SetTalentTypeImage(DetailPanelTexture);
}

void UGridSkillsWidget::HandleTalentNodeClicked(FName TalentNodeId)
{
	const FGridTalentNodeView* Node = FindTalentNode(TalentNodeId);
	if (Node && Node->bHasExclusiveVariants && Node->SelectedChoiceId.IsNone())
	{
		OpenTalentVariantFlyout(TalentNodeId);
		return;
	}

	CloseTalentVariantFlyout();
	if (SelectTalentNode(TalentNodeId))
	{
		ApplyTalentDetailPresentation();
	}
}

void UGridSkillsWidget::HandleTalentVariantFlyoutChosen(FName ChoiceId)
{
	const FName TalentNodeId = ActiveVariantFlyoutNodeId;
	CloseTalentVariantFlyout();

	if (SelectTalentVariant(TalentNodeId, ChoiceId))
	{
		ApplyTalentDetailPresentation();
	}
}

void UGridSkillsWidget::HandleTalentAcquireConfirmed(FName ChoiceId)
{
	FText Feedback;
	CommitConfirmedTalentChoice(ChoiceId, Feedback);
}


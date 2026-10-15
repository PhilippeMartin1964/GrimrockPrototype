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
#include "UI/GridSkillEntryWidget.h"
#include "UI/GridSkillsPageService.h"
#include "UI/GridTalentBranchWidget.h"
#include "UI/GridTalentDetailWidget.h"
#include "UI/GridTalentNodeWidget.h"
#include "UI/GridTalentVariantFlyoutWidget.h"
#include "UI/GridRPGNotificationWidget.h"
#include "UI/RPGProgressionFeedbackService.h"
#include "UI/RPGTalentDetailTextSet.h"
#include "UI/UILocalizationText.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridSkillsUI, Log, All);

namespace GridSkillsWidgetPrivate
{
	const TCHAR* TalentPresentationAssetPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/UI/RPG/DA_RPGTalentPresentation.DA_RPGTalentPresentation");

	UGridTalentBranchWidget* BranchWidgetByIndex(
		int32 Index,
		UGridTalentBranchWidget* Left,
		UGridTalentBranchWidget* Center,
		UGridTalentBranchWidget* Right)
	{
		switch (Index)
		{
			case 0: return Left;
			case 1: return Center;
			case 2: return Right;
			default: return nullptr;
		}
	}
}

void UGridSkillsWidget::InitializeSkillsWidget(AGrimrockPartyPawn* InPartyPawn)
{
	if (InventoryComponent)
	{
		InventoryComponent->OnPartyInventoryChanged.RemoveDynamic(this, &UGridSkillsWidget::HandlePartyInventoryChanged);
	}

	OwningPartyPawn = InPartyPawn;
	InventoryComponent = InPartyPawn ? InPartyPawn->PartyInventoryComponent : nullptr;

	if (InventoryComponent)
	{
		InventoryComponent->OnPartyInventoryChanged.RemoveDynamic(this, &UGridSkillsWidget::HandlePartyInventoryChanged);
		InventoryComponent->OnPartyInventoryChanged.AddDynamic(this, &UGridSkillsWidget::HandlePartyInventoryChanged);
	}

	RefreshSkills();
}

void UGridSkillsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindDesignerShell();
	ApplyDesignerPresentation();
}

void UGridSkillsWidget::NativeDestruct()
{
	CloseTalentVariantFlyout();
	UnbindDesignerShell();

	if (InventoryComponent)
	{
		InventoryComponent->OnPartyInventoryChanged.RemoveDynamic(this, &UGridSkillsWidget::HandlePartyInventoryChanged);
	}

	Super::NativeDestruct();
}

void UGridSkillsWidget::BindDesignerShell()
{
	Button_SkillsTab->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleSkillsTabClicked);
	Button_SkillsTab->OnClicked.AddUniqueDynamic(this, &UGridSkillsWidget::HandleSkillsTabClicked);

	Button_TalentsTab->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentsTabClicked);
	Button_TalentsTab->OnClicked.AddUniqueDynamic(this, &UGridSkillsWidget::HandleTalentsTabClicked);

	Branch_Left->OnTalentNodeClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentNodeClicked);
	Branch_Left->OnTalentNodeClicked.AddUniqueDynamic(this, &UGridSkillsWidget::HandleTalentNodeClicked);
	Branch_Center->OnTalentNodeClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentNodeClicked);
	Branch_Center->OnTalentNodeClicked.AddUniqueDynamic(this, &UGridSkillsWidget::HandleTalentNodeClicked);
	Branch_Right->OnTalentNodeClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentNodeClicked);
	Branch_Right->OnTalentNodeClicked.AddUniqueDynamic(this, &UGridSkillsWidget::HandleTalentNodeClicked);

	if (Detail_Talent)
	{
		Detail_Talent->OnAcquireConfirmed.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentAcquireConfirmed);
		Detail_Talent->OnAcquireConfirmed.AddUniqueDynamic(this, &UGridSkillsWidget::HandleTalentAcquireConfirmed);
	}
}

void UGridSkillsWidget::UnbindDesignerShell()
{
	if (Button_SkillsTab)
	{
		Button_SkillsTab->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleSkillsTabClicked);
	}
	if (Button_TalentsTab)
	{
		Button_TalentsTab->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentsTabClicked);
	}
	if (Branch_Left)
	{
		Branch_Left->OnTalentNodeClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentNodeClicked);
	}
	if (Branch_Center)
	{
		Branch_Center->OnTalentNodeClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentNodeClicked);
	}
	if (Branch_Right)
	{
		Branch_Right->OnTalentNodeClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentNodeClicked);
	}
	if (Detail_Talent)
	{
		Detail_Talent->OnAcquireConfirmed.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentAcquireConfirmed);
	}
}

FReply UGridSkillsWidget::NativeOnPreviewMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	// The flyout is a top-level viewport sibling, not a child of this widget.
	// Any click that reaches the Skills hierarchy is therefore outside the popup.
	CloseTalentVariantFlyout();
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void UGridSkillsWidget::ClearView()
{
	CloseTalentVariantFlyout();
	View = FGridSkillsPageView();
}

void UGridSkillsWidget::BeginSkillAllocationSession()
{
	SessionPurchasedSkillRanks.Reset();
	SessionSkillRankFloors.Reset();
	RefreshSkills();
}

const URPGSkillAsset* UGridSkillsWidget::ResolveCanonicalSkillDefinition(FName SkillId) const
{
	if (SkillId.IsNone())
	{
		return nullptr;
	}

	TArray<const URPGSkillAsset*> Definitions;
	FGridSkillsPageService::ResolveCanonicalSkillDefinitions(Definitions);
	for (const URPGSkillAsset* Definition : Definitions)
	{
		if (IsValid(Definition) && Definition->SkillId == SkillId)
		{
			return Definition;
		}
	}
	return nullptr;
}

FString UGridSkillsWidget::MakeSkillAllocationSessionKey(const FGuid& CharacterId, FName SkillId) const
{
	return CharacterId.IsValid() && !SkillId.IsNone()
		? FString::Printf(TEXT("%s|%s"), *CharacterId.ToString(EGuidFormats::Digits), *SkillId.ToString())
		: FString();
}

int32 UGridSkillsWidget::GetSessionPurchasedSkillRankCount(const FGuid& CharacterId, FName SkillId) const
{
	const FString Key = MakeSkillAllocationSessionKey(CharacterId, SkillId);
	return Key.IsEmpty() ? 0 : SessionPurchasedSkillRanks.FindRef(Key);
}

int32 UGridSkillsWidget::GetSessionSkillRankFloor(const FGuid& CharacterId, FName SkillId) const
{
	const FString Key = MakeSkillAllocationSessionKey(CharacterId, SkillId);
	if (Key.IsEmpty())
	{
		return INDEX_NONE;
	}

	const int32* Floor = SessionSkillRankFloors.Find(Key);
	return Floor ? *Floor : INDEX_NONE;
}

void UGridSkillsWidget::RecordSessionSkillPurchase(const FGuid& CharacterId, FName SkillId, int32 PreviousRank)
{
	const FString Key = MakeSkillAllocationSessionKey(CharacterId, SkillId);
	if (Key.IsEmpty() || PreviousRank < 0)
	{
		return;
	}

	SessionSkillRankFloors.FindOrAdd(Key, PreviousRank);
	++SessionPurchasedSkillRanks.FindOrAdd(Key);
}

void UGridSkillsWidget::ConsumeSessionSkillPurchase(const FGuid& CharacterId, FName SkillId)
{
	const FString Key = MakeSkillAllocationSessionKey(CharacterId, SkillId);
	if (Key.IsEmpty())
	{
		return;
	}

	int32* Count = SessionPurchasedSkillRanks.Find(Key);
	if (!Count)
	{
		return;
	}

	--(*Count);
	if (*Count <= 0)
	{
		SessionPurchasedSkillRanks.Remove(Key);
		SessionSkillRankFloors.Remove(Key);
	}
}

void UGridSkillsWidget::RefreshSkills()
{
	if (bRefreshInProgress)
	{
		return;
	}
	TGuardValue<bool> RefreshGuard(bRefreshInProgress, true);

	const FGuid PreviousCharacterId = View.CharacterId;
	ClearView();
	if (InventoryComponent)
	{
		TArray<const URPGSkillAsset*> SkillDefinitions;
		FGridSkillsPageService::ResolveCanonicalSkillDefinitions(SkillDefinitions);

		FGridSkillsPageView Candidate;
		if (FGridSkillsPageService::TryBuildSelectedCharacterView(InventoryComponent, SkillDefinitions, Candidate))
		{
			for (FGridSkillEntryView& Entry : Candidate.Skills)
			{
				const int32 SessionPurchaseCount =
					GetSessionPurchasedSkillRankCount(Candidate.CharacterId, Entry.SkillId);
				const int32 SessionFloor =
					GetSessionSkillRankFloor(Candidate.CharacterId, Entry.SkillId);
				Entry.bCanDecreaseRank =
					SessionPurchaseCount > 0 &&
					SessionFloor >= 0 &&
					Entry.Rank > SessionFloor;
			}
			View = MoveTemp(Candidate);
		}
	}

	if (!SelectedTalentNodeId.IsNone())
	{
		const bool bCharacterChanged =
			PreviousCharacterId.IsValid() && (!View.IsValid() || PreviousCharacterId != View.CharacterId);
		const FGridTalentNodeView* RefreshedNode = FindTalentNode(SelectedTalentNodeId);
		if (bCharacterChanged || !RefreshedNode)
		{
			ClearTalentSelection();
		}
		else if (!SelectedTalentChoiceId.IsNone())
		{
			const bool bChoiceStillExists = RefreshedNode->Variants.ContainsByPredicate(
				[this](const FGridTalentVariantView& Variant)
				{
					return Variant.ChoiceId == SelectedTalentChoiceId;
				});
			if (!bChoiceStillExists)
			{
				ClearTalentSelection();
			}
			else if (RefreshedNode->SelectedChoiceId == SelectedTalentChoiceId)
			{
				SelectedTalentChoiceId = NAME_None;
			}
		}
	}

	ApplyDesignerPresentation();
	ApplyTalentDetailPresentation();
	OnSkillsRefreshed.Broadcast();
}

bool UGridSkillsWidget::GetCurrentClassPresentation(FRPGClassPresentationDefinition& OutPresentation) const
{
	OutPresentation = FRPGClassPresentationDefinition();
	if (!View.IsValid() || View.ClassId.IsNone())
	{
		return false;
	}

	const URPGTalentPresentationAsset* Catalog =
		LoadObject<URPGTalentPresentationAsset>(nullptr, GridSkillsWidgetPrivate::TalentPresentationAssetPath);
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

const FGridTalentNodeView* UGridSkillsWidget::FindTalentNode(FName TalentNodeId) const
{
	if (TalentNodeId.IsNone())
	{
		return nullptr;
	}

	for (const FGridTalentBranchView& Branch : View.TalentTree.Branches)
	{
		if (const FGridTalentNodeView* Node = Branch.Nodes.FindByPredicate(
			[TalentNodeId](const FGridTalentNodeView& Candidate)
			{
				return Candidate.TalentNodeId == TalentNodeId;
			}))
		{
			return Node;
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

bool UGridSkillsWidget::BuildSelectedTalentDetailView(FGridTalentNodeView& OutDetailNode) const
{
	OutDetailNode = FGridTalentNodeView();
	const FGridTalentNodeView* Node = FindTalentNode(SelectedTalentNodeId);
	if (!Node)
	{
		return false;
	}

	OutDetailNode = *Node;
	if (!Node->bHasExclusiveVariants)
	{
		return true;
	}

	const FName ChoiceId =
		!Node->SelectedChoiceId.IsNone() ? Node->SelectedChoiceId : SelectedTalentChoiceId;
	if (ChoiceId.IsNone())
	{
		return false;
	}

	const FGridTalentVariantView* Variant = Node->Variants.FindByPredicate(
		[ChoiceId](const FGridTalentVariantView& Candidate)
		{
			return Candidate.ChoiceId == ChoiceId;
		});
	if (!Variant)
	{
		return false;
	}

	OutDetailNode.DisplayName = Variant->DisplayName;
	OutDetailNode.State = Variant->State;
	OutDetailNode.Type = Variant->Type;
	OutDetailNode.Principle = Variant->Principle;
	OutDetailNode.Effects = Variant->Effects;
	OutDetailNode.Usage = Variant->Usage;
	OutDetailNode.SimpleChoiceId = Variant->ChoiceId;
	OutDetailNode.bCanAcquireSimple = Variant->bCanChoose && !Variant->bAcquired;
	OutDetailNode.bHasExclusiveVariants = false;
	OutDetailNode.Variants.Reset();
	OutDetailNode.SelectedChoiceId = Variant->bAcquired ? Variant->ChoiceId : NAME_None;
	return true;
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

bool UGridSkillsWidget::CommitSkillRankIncrease(FName SkillId, FText& OutFeedback)
{
	OutFeedback = FUILocalizationText::Get(TEXT("Progression.Skill.AllocationFailed"));

	const URPGSkillAsset* Definition = ResolveCanonicalSkillDefinition(SkillId);
	FText SkillDisplayName = FText::FromName(SkillId);
	if (const FGridSkillEntryView* Entry = View.Skills.FindByPredicate(
		[SkillId](const FGridSkillEntryView& Candidate)
		{
			return Candidate.SkillId == SkillId;
		}))
	{
		SkillDisplayName = Entry->DisplayName.IsEmpty() ? FText::FromName(SkillId) : Entry->DisplayName;
	}

	const FGuid SessionCharacterId = View.CharacterId;
	FRPGSkillPointMutationResult Result;
	const bool bCommitted =
		FRPGSkillPointService::TryPurchaseNextRank(InventoryComponent, View.CharacterIndex, Definition, Result);

	if (bCommitted)
	{
		RecordSessionSkillPurchase(SessionCharacterId, SkillId, Result.PreviousRank);
	}

	const FRPGProgressionNotificationView Notification =
		FRPGProgressionFeedbackService::MakeSkillRankPurchaseNotification(Result, SkillDisplayName);
	OutFeedback = Notification.Message;
	PublishProgressionNotification(Notification);

	// The transaction broadcasts inventory changes synchronously, before the
	// UI-session history is updated. Refresh once more to project the undo state.
	RefreshSkills();
	return bCommitted;
}

bool UGridSkillsWidget::CommitSkillRankDecrease(FName SkillId, FText& OutFeedback)
{
	OutFeedback = FUILocalizationText::Get(TEXT("Progression.Skill.CancellationFailed"));

	const URPGSkillAsset* Definition = ResolveCanonicalSkillDefinition(SkillId);
	FText SkillDisplayName = FText::FromName(SkillId);
	if (const FGridSkillEntryView* Entry = View.Skills.FindByPredicate(
		[SkillId](const FGridSkillEntryView& Candidate)
		{
			return Candidate.SkillId == SkillId;
		}))
	{
		SkillDisplayName = Entry->DisplayName.IsEmpty() ? FText::FromName(SkillId) : Entry->DisplayName;
	}

	const FGuid SessionCharacterId = View.CharacterId;
	const int32 SessionPurchaseCount = GetSessionPurchasedSkillRankCount(SessionCharacterId, SkillId);
	const int32 SessionFloor = GetSessionSkillRankFloor(SessionCharacterId, SkillId);

	FRPGSkillPointMutationResult Result;
	bool bCommitted = false;
	if (SessionPurchaseCount <= 0 || SessionFloor < 0)
	{
		Result.SkillId = SkillId;
		Result.RejectReason = ERPGSkillPointMutationRejectReason::NoSessionPurchaseToUndo;
	}
	else
	{
		bCommitted = FRPGSkillPointService::TryRefundPurchasedRank(
			InventoryComponent,
			View.CharacterIndex,
			Definition,
			SessionFloor,
			Result);
	}

	if (bCommitted)
	{
		ConsumeSessionSkillPurchase(SessionCharacterId, SkillId);
	}

	const FRPGProgressionNotificationView Notification =
		FRPGProgressionFeedbackService::MakeSkillRankRefundNotification(Result, SkillDisplayName);
	OutFeedback = Notification.Message;
	PublishProgressionNotification(Notification);

	RefreshSkills();
	return bCommitted;
}

void UGridSkillsWidget::PublishProgressionNotification(const FRPGProgressionNotificationView& Notification)
{
	if (!Notification.IsValid())
	{
		return;
	}

	LastProgressionNotification = Notification;
	if (Notification_Progression)
	{
		Notification_Progression->ShowNotification(Notification);
	}
	OnProgressionNotification.Broadcast(LastProgressionNotification);
}

void UGridSkillsWidget::ApplyDesignerPresentation()
{
	RebuildSkillEntryWidgets();
	ApplyClassVisualIdentity();
	RefreshTabSelection();

	if (!View.IsValid())
	{
		Text_CharacterName->SetText(
			URPGTalentDetailTextSet::ResolveProductionText(TEXT("Talent.Screen.NoCharacterSelected")));
		Text_ClassLevel->SetText(FText::GetEmpty());

		const FText TalentPointsFormat =
			URPGTalentDetailTextSet::ResolveProductionText(TEXT("Talent.Screen.TalentPointsFormat"));
		Text_TalentPoints->SetText(
			TalentPointsFormat.IsEmpty()
				? FText::GetEmpty()
				: FText::Format(TalentPointsFormat, FText::FromString(TEXT("—"))));

		if (Text_SkillPoints)
		{
			Text_SkillPoints->SetText(
				URPGTalentDetailTextSet::ResolveProductionText(TEXT("Talent.Screen.SkillPointsUnavailable")));
		}

		Branch_Left->ClearTalentBranch();
		Branch_Center->ClearTalentBranch();
		Branch_Right->ClearTalentBranch();
		return;
	}

	Text_CharacterName->SetText(
		View.CharacterName.IsEmpty()
			? URPGTalentDetailTextSet::ResolveProductionText(TEXT("Talent.Screen.SelectedCharacterFallback"))
			: View.CharacterName);

	const FText ClassLevelFormat =
		URPGTalentDetailTextSet::ResolveProductionText(TEXT("Talent.Screen.ClassLevelFormat"));
	Text_ClassLevel->SetText(
		ClassLevelFormat.IsEmpty() || View.ClassDisplayName.IsEmpty()
			? FText::GetEmpty()
			: FText::Format(ClassLevelFormat, View.ClassDisplayName, FText::AsNumber(View.CharacterLevel)));

	const FText TalentPointsFormat =
		URPGTalentDetailTextSet::ResolveProductionText(TEXT("Talent.Screen.TalentPointsFormat"));
	Text_TalentPoints->SetText(
		TalentPointsFormat.IsEmpty()
			? FText::GetEmpty()
			: FText::Format(TalentPointsFormat, FText::AsNumber(View.RemainingTalentPoints)));

	if (Text_SkillPoints)
	{
		const FText SkillPointsFormat =
			URPGTalentDetailTextSet::ResolveProductionText(TEXT("Talent.Screen.SkillPointsFormat"));
		Text_SkillPoints->SetText(
			SkillPointsFormat.IsEmpty()
				? FText::GetEmpty()
				: FText::Format(
					SkillPointsFormat,
					FText::AsNumber(View.RemainingSkillPoints),
					FText::AsNumber(View.SkillRankCap)));
	}

	FRPGClassPresentationDefinition ClassPresentation;
	if (GetCurrentClassPresentation(ClassPresentation))
	{
		Text_ClassLevel->SetColorAndOpacity(FSlateColor(ClassPresentation.AccentColor));
	}

	const URPGClassVisualAsset* ClassVisual = ResolveCurrentClassVisual();
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UGridTalentBranchWidget* BranchWidget = GridSkillsWidgetPrivate::BranchWidgetByIndex(
			Index, Branch_Left, Branch_Center, Branch_Right);
		if (!BranchWidget)
		{
			continue;
		}

		FGridTalentBranchView BranchView;
		FRPGTalentBranchPresentationDefinition BranchPresentation;
		if (GetPresentedTalentBranch(Index, BranchView, BranchPresentation))
		{
			if (BranchWidget->InitializeTalentBranch(BranchView, BranchPresentation))
			{
				UTexture2D* BackgroundTexture = nullptr;
				if (ClassVisual)
				{
					if (const FRPGTalentBranchVisualDefinition* BranchVisual =
						ClassVisual->FindTalentBranchVisual(BranchView.TalentBranchId))
					{
						BackgroundTexture = BranchVisual->Background.LoadSynchronous();
					}
				}
				BranchWidget->SetBranchBackground(BackgroundTexture);

				for (const FGridTalentNodeView& NodeView : BranchView.Nodes)
				{
					UTexture2D* IconTexture = nullptr;
					if (ClassVisual)
					{
						if (const FRPGTalentNodeVisualDefinition* NodeVisual =
							ClassVisual->FindTalentNodeVisual(NodeView.TalentNodeId))
						{
							IconTexture = NodeVisual->Icon.LoadSynchronous();
						}
						if (!NodeView.SelectedChoiceId.IsNone())
						{
							if (const FRPGTalentVariantVisualDefinition* VariantVisual =
								ClassVisual->FindTalentVariantVisual(NodeView.SelectedChoiceId))
							{
								if (UTexture2D* AcquiredVariantIcon = VariantVisual->Icon.LoadSynchronous())
								{
									IconTexture = AcquiredVariantIcon;
								}
							}
						}
					}

					if (UGridTalentNodeWidget* NodeWidget = BranchWidget->GetTalentNodeWidgetForTier(NodeView.Tier))
					{
						NodeWidget->SetTalentIcon(IconTexture);
						NodeWidget->ClearVariantIcons();
						if (ClassVisual && NodeView.bHasExclusiveVariants && NodeView.SelectedChoiceId.IsNone())
						{
							for (const FGridTalentVariantView& Variant : NodeView.Variants)
							{
								if (const FRPGTalentVariantVisualDefinition* VariantVisual =
									ClassVisual->FindTalentVariantVisual(Variant.ChoiceId))
								{
									NodeWidget->SetVariantIcon(
										Variant.ChoiceId,
										VariantVisual->Icon.LoadSynchronous());
								}
							}
						}
					}
				}
			}
		}
		else
		{
			BranchWidget->ClearTalentBranch();
		}
	}
}

const URPGClassVisualAsset* UGridSkillsWidget::ResolveCurrentClassVisual() const
{
	return View.IsValid() && !View.ClassId.IsNone()
		? FRPGAuthoringIdentityResolver::ResolveClassVisualByClassId(View.ClassId)
		: nullptr;
}

void UGridSkillsWidget::ApplyClassVisualIdentity()
{
	UTexture2D* BannerTexture = nullptr;
	UTexture2D* FlagTexture = nullptr;

	if (View.IsValid() && !View.ClassId.IsNone())
	{
		if (const URPGClassVisualAsset* ClassVisual = ResolveCurrentClassVisual())
		{
			BannerTexture = ClassVisual->Banner.LoadSynchronous();
			FlagTexture = ClassVisual->Flag.LoadSynchronous();
		}

		if (!BannerTexture || !FlagTexture)
		{
			UE_LOG(
				LogGridSkillsUI,
				Warning,
				TEXT("GridSkills ClassVisual Missing ClassId=%s Banner=%s Flag=%s"),
				*View.ClassId.ToString(),
				BannerTexture ? TEXT("OK") : TEXT("Missing"),
				FlagTexture ? TEXT("OK") : TEXT("Missing"));
		}
	}

	Border_Header->SetBrushFromTexture(BannerTexture);
	Image_ClassFlag->SetBrushFromTexture(FlagTexture, false);
}

void UGridSkillsWidget::ApplyTalentDetailPresentation()
{
	if (!Detail_Talent)
	{
		return;
	}

	FGridTalentNodeView ConceptNode;
	FGridTalentNodeView DetailNode;
	if (!GetSelectedTalentNode(ConceptNode) || !BuildSelectedTalentDetailView(DetailNode))
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
		ClassPresentation.FindBranch(DetailNode.TalentBranchId);
	if (!BranchPresentation ||
		!Detail_Talent->InitializeTalentDetail(DetailNode, *BranchPresentation))
	{
		Detail_Talent->ClearTalentDetail();
		return;
	}

	UTexture2D* IconTexture = nullptr;
	UTexture2D* DetailPanelTexture = nullptr;
	if (const URPGClassVisualAsset* ClassVisual = ResolveCurrentClassVisual())
	{
		if (ConceptNode.bHasExclusiveVariants && !DetailNode.SimpleChoiceId.IsNone())
		{
			if (const FRPGTalentVariantVisualDefinition* VariantVisual =
				ClassVisual->FindTalentVariantVisual(DetailNode.SimpleChoiceId))
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
			ClassVisual->FindTalentBranchVisual(DetailNode.TalentBranchId))
		{
			DetailPanelTexture = BranchVisual->DetailPanelImage.LoadSynchronous();
		}
	}

	Detail_Talent->SetTalentIcon(IconTexture);
	Detail_Talent->SetTalentTypeImage(DetailPanelTexture);
}

void UGridSkillsWidget::RebuildSkillEntryWidgets()
{
	if (Text_EmptySkills)
	{
		Text_EmptySkills->SetVisibility(View.Skills.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (!Panel_SkillEntries)
	{
		return;
	}

	Panel_SkillEntries->ClearChildren();
	if (View.Skills.IsEmpty())
	{
		return;
	}

	if (!SkillEntryWidgetClass)
	{
		UE_LOG(LogGridSkillsUI, Warning, TEXT("WBP_GridSkills has Skills data but no SkillEntryWidgetClass."));
		return;
	}

	for (const FGridSkillEntryView& Entry : View.Skills)
	{
		UGridSkillEntryWidget* EntryWidget = CreateWidget<UGridSkillEntryWidget>(this, SkillEntryWidgetClass);
		if (!EntryWidget)
		{
			UE_LOG(LogGridSkillsUI, Warning, TEXT("Failed to create Skill row for %s."), *Entry.SkillId.ToString());
			continue;
		}

		if (!EntryWidget->InitializeSkillEntry(Entry))
		{
			UE_LOG(LogGridSkillsUI, Warning, TEXT("Rejected invalid Skill row projection for %s."), *Entry.SkillId.ToString());
			continue;
		}

		EntryWidget->OnIncreaseSkillRequested.AddUniqueDynamic(this, &UGridSkillsWidget::HandleSkillIncreaseRequested);
		EntryWidget->OnDecreaseSkillRequested.AddUniqueDynamic(this, &UGridSkillsWidget::HandleSkillDecreaseRequested);
		Panel_SkillEntries->AddChild(EntryWidget);
	}
}

void UGridSkillsWidget::RefreshTabSelection()
{
	const int32 ActiveIndex = Switcher_SkillsTalents->GetActiveWidgetIndex();

	Image_SkillsTabSelectionFrame->SetVisibility(
		ActiveIndex == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	Image_TalentsTabSelectionFrame->SetVisibility(
		ActiveIndex == 1 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UGridSkillsWidget::ShowSkillsTab()
{
	CloseTalentVariantFlyout();
	Switcher_SkillsTalents->SetActiveWidgetIndex(0);
	RefreshTabSelection();
}

void UGridSkillsWidget::ShowTalentsTab()
{
	Switcher_SkillsTalents->SetActiveWidgetIndex(1);
	RefreshTabSelection();
}

void UGridSkillsWidget::HandleSkillsTabClicked()
{
	ShowSkillsTab();
}

void UGridSkillsWidget::HandleTalentsTabClicked()
{
	ShowTalentsTab();
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

void UGridSkillsWidget::HandleSkillIncreaseRequested(FName SkillId)
{
	FText Feedback;
	CommitSkillRankIncrease(SkillId, Feedback);
}

void UGridSkillsWidget::HandleSkillDecreaseRequested(FName SkillId)
{
	FText Feedback;
	CommitSkillRankDecrease(SkillId, Feedback);
}

int32 UGridSkillsWidget::GetSkillEntryCount() const
{
	return View.Skills.Num();
}

bool UGridSkillsWidget::GetSkillEntry(int32 EntryIndex, FGridSkillEntryView& OutEntry) const
{
	if (!View.Skills.IsValidIndex(EntryIndex))
	{
		OutEntry = FGridSkillEntryView();
		return false;
	}

	OutEntry = View.Skills[EntryIndex];
	return true;
}

void UGridSkillsWidget::HandlePartyInventoryChanged(int32 CharacterIndex)
{
	(void)CharacterIndex;
	RefreshSkills();
}

#include "UI/GridSkillsWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "UI/GridSkillEntryWidget.h"
#include "UI/GridSkillsPageService.h"
#include "UI/GridTalentBranchWidget.h"
#include "UI/GridTalentDetailWidget.h"
#include "UI/GridRPGNotificationWidget.h"
#include "UI/RPGProgressionFeedbackService.h"

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

void UGridSkillsWidget::ClearView()
{
	View = FGridSkillsPageView();
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
			View = MoveTemp(Candidate);
		}
	}

	if (!SelectedTalentNodeId.IsNone())
	{
		const bool bCharacterChanged =
			PreviousCharacterId.IsValid() && (!View.IsValid() || PreviousCharacterId != View.CharacterId);
		if (bCharacterChanged || !FindTalentNode(SelectedTalentNodeId))
		{
			ClearTalentSelection();
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

	if (!View.TalentTree.Branches.IsValidIndex(VisualIndex))
	{
		return false;
	}

	OutBranch = View.TalentTree.Branches[VisualIndex];
	OutPresentation.TalentBranchId = OutBranch.TalentBranchId;
	OutPresentation.DisplayName = FText::FromName(OutBranch.TalentBranchId);
	return true;
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

bool UGridSkillsWidget::SelectTalentNode(FName TalentNodeId)
{
	if (!View.IsValid() || !FindTalentNode(TalentNodeId))
	{
		return false;
	}

	if (SelectedTalentNodeId == TalentNodeId)
	{
		return true;
	}

	SelectedTalentNodeId = TalentNodeId;
	OnTalentSelectionChanged.Broadcast(SelectedTalentNodeId);
	return true;
}

void UGridSkillsWidget::ClearTalentSelection()
{
	if (SelectedTalentNodeId.IsNone())
	{
		return;
	}

	SelectedTalentNodeId = NAME_None;
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

bool UGridSkillsWidget::CommitConfirmedTalentChoice(FName ChoiceId, FText& OutFeedback)
{
	OutFeedback = FText::FromString(TEXT("Acquisition impossible."));

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

	const FText RequestedDisplayName =
		RequestedVariant->DisplayName.IsEmpty() ? FText::FromName(ChoiceId) : RequestedVariant->DisplayName;

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

	if (!View.IsValid())
	{
		Text_CharacterName->SetText(FText::FromString(TEXT("Aucun personnage sélectionné")));
		Text_ClassLevel->SetText(FText::GetEmpty());
		Text_TalentPoints->SetText(FText::FromString(TEXT("Points de talent : —")));

		Branch_Left->ClearTalentBranch();
		Branch_Center->ClearTalentBranch();
		Branch_Right->ClearTalentBranch();
		return;
	}

	Text_CharacterName->SetText(
		View.CharacterName.IsEmpty() ? FText::FromString(TEXT("Personnage sélectionné")) : View.CharacterName);

	const FString ClassName =
		View.ClassDisplayName.IsEmpty() ? View.ClassId.ToString() : View.ClassDisplayName.ToString();
	Text_ClassLevel->SetText(
		FText::FromString(FString::Printf(TEXT("%s — Niveau %d"), *ClassName, View.CharacterLevel)));

	Text_TalentPoints->SetText(
		FText::FromString(FString::Printf(TEXT("Points de talent : %d"), View.RemainingTalentPoints)));

	FRPGClassPresentationDefinition ClassPresentation;
	if (GetCurrentClassPresentation(ClassPresentation))
	{
		Text_ClassLevel->SetColorAndOpacity(FSlateColor(ClassPresentation.AccentColor));
	}

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
			BranchWidget->InitializeTalentBranch(BranchView, BranchPresentation);
		}
		else
		{
			BranchWidget->ClearTalentBranch();
		}
	}
}

void UGridSkillsWidget::ApplyTalentDetailPresentation()
{
	if (!Detail_Talent)
	{
		return;
	}

	FGridTalentNodeView SelectedNode;
	if (!GetSelectedTalentNode(SelectedNode))
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
		ClassPresentation.FindBranch(SelectedNode.TalentBranchId);
	if (!BranchPresentation ||
		!Detail_Talent->InitializeTalentDetail(SelectedNode, *BranchPresentation))
	{
		Detail_Talent->ClearTalentDetail();
	}
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

		Panel_SkillEntries->AddChild(EntryWidget);
	}
}

void UGridSkillsWidget::ShowSkillsTab()
{
	Switcher_SkillsTalents->SetActiveWidgetIndex(0);
}

void UGridSkillsWidget::ShowTalentsTab()
{
	Switcher_SkillsTalents->SetActiveWidgetIndex(1);
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
	if (SelectTalentNode(TalentNodeId))
	{
		ApplyTalentDetailPresentation();
	}
}

void UGridSkillsWidget::HandleTalentAcquireConfirmed(FName ChoiceId)
{
	FText Feedback;
	const bool bCommitted = CommitConfirmedTalentChoice(ChoiceId, Feedback);
	if (Detail_Talent)
	{
		// UI-RPG05: the toast is the sole success feedback surface.
		// Keep inline detail feedback only for a rejected transaction.
		Detail_Talent->SetAcquisitionFeedback(bCommitted ? FText::GetEmpty() : Feedback);
	}
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

int32 UGridSkillsWidget::GetTalentEntryCount() const
{
	return View.Talents.Num();
}

bool UGridSkillsWidget::GetTalentEntry(int32 EntryIndex, FGridTalentEntryView& OutEntry) const
{
	if (!View.Talents.IsValidIndex(EntryIndex))
	{
		OutEntry = FGridTalentEntryView();
		return false;
	}

	OutEntry = View.Talents[EntryIndex];
	return true;
}

void UGridSkillsWidget::HandlePartyInventoryChanged(int32 CharacterIndex)
{
	(void)CharacterIndex;
	RefreshSkills();
}

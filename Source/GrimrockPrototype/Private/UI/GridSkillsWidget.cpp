#include "UI/GridSkillsWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridSkillsPageService.h"
#include "UI/GridTalentBranchWidget.h"

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
	Button_SkillsTab->OnClicked.AddDynamic(this, &UGridSkillsWidget::HandleSkillsTabClicked);
	Button_SkillsTab->OnHovered.RemoveDynamic(this, &UGridSkillsWidget::HandleSkillsTabHovered);
	Button_SkillsTab->OnHovered.AddDynamic(this, &UGridSkillsWidget::HandleSkillsTabHovered);

	Button_TalentsTab->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentsTabClicked);
	Button_TalentsTab->OnClicked.AddDynamic(this, &UGridSkillsWidget::HandleTalentsTabClicked);
	Button_TalentsTab->OnHovered.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentsTabHovered);
	Button_TalentsTab->OnHovered.AddDynamic(this, &UGridSkillsWidget::HandleTalentsTabHovered);

	UE_LOG(LogGridSkillsUI, Log,
		TEXT("GridSkills Tabs Bound Widget=%s SkillsButton=%s TalentsButton=%s Switcher=%s ActiveIndex=%d"),
		*GetName(),
		*GetNameSafe(Button_SkillsTab),
		*GetNameSafe(Button_TalentsTab),
		*GetNameSafe(Switcher_SkillsTalents),
		Switcher_SkillsTalents ? Switcher_SkillsTalents->GetActiveWidgetIndex() : INDEX_NONE);
}

void UGridSkillsWidget::UnbindDesignerShell()
{
	if (Button_SkillsTab)
	{
		Button_SkillsTab->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleSkillsTabClicked);
		Button_SkillsTab->OnHovered.RemoveDynamic(this, &UGridSkillsWidget::HandleSkillsTabHovered);
	}
	if (Button_TalentsTab)
	{
		Button_TalentsTab->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentsTabClicked);
		Button_TalentsTab->OnHovered.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentsTabHovered);
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

	ApplyDesignerPresentation();
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

void UGridSkillsWidget::ApplyDesignerPresentation()
{
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

void UGridSkillsWidget::ShowSkillsTab()
{
	const int32 Before = Switcher_SkillsTalents->GetActiveWidgetIndex();
	Switcher_SkillsTalents->SetActiveWidgetIndex(0);
	UE_LOG(LogGridSkillsUI, Log, TEXT("GridSkills ShowSkillsTab Before=%d After=%d"), Before, Switcher_SkillsTalents->GetActiveWidgetIndex());
}

void UGridSkillsWidget::ShowTalentsTab()
{
	const int32 Before = Switcher_SkillsTalents->GetActiveWidgetIndex();
	Switcher_SkillsTalents->SetActiveWidgetIndex(1);
	UE_LOG(LogGridSkillsUI, Log, TEXT("GridSkills ShowTalentsTab Before=%d After=%d"), Before, Switcher_SkillsTalents->GetActiveWidgetIndex());
}

void UGridSkillsWidget::HandleSkillsTabClicked()
{
	UE_LOG(LogGridSkillsUI, Log, TEXT("GridSkills SkillsTab CLICKED"));
	ShowSkillsTab();
}

void UGridSkillsWidget::HandleTalentsTabClicked()
{
	UE_LOG(LogGridSkillsUI, Log, TEXT("GridSkills TalentsTab CLICKED"));
	ShowTalentsTab();
}

void UGridSkillsWidget::HandleSkillsTabHovered()
{
	UE_LOG(LogGridSkillsUI, Log, TEXT("GridSkills SkillsTab HOVERED"));
}

void UGridSkillsWidget::HandleTalentsTabHovered()
{
	UE_LOG(LogGridSkillsUI, Log, TEXT("GridSkills TalentsTab HOVERED"));
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

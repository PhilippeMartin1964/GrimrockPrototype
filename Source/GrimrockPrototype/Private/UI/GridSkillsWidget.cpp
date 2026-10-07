#include "UI/GridSkillsWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridSkillsPageService.h"
#include "UI/GridTalentBranchWidget.h"


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

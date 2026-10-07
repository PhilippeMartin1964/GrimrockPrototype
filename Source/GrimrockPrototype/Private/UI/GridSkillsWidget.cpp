#include "UI/GridSkillsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "RPG/RPGSkillTypes.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridSkillsPageService.h"

DEFINE_LOG_CATEGORY_STATIC(LogGrimrockInGameUI, Log, All);

namespace GridSkillsWidgetPrivate
{
	const TCHAR* TalentPresentationAssetPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/UI/RPG/DA_RPGTalentPresentation.DA_RPGTalentPresentation");

	FText GetAttributeLabel(ERPGSkillGoverningAttribute Attribute)
	{
		switch (Attribute)
		{
			case ERPGSkillGoverningAttribute::Strength:
				return FText::FromString(TEXT("Force"));
			case ERPGSkillGoverningAttribute::Dexterity:
				return FText::FromString(TEXT("Dextérité"));
			case ERPGSkillGoverningAttribute::Constitution:
				return FText::FromString(TEXT("Constitution"));
			case ERPGSkillGoverningAttribute::Intelligence:
				return FText::FromString(TEXT("Intelligence"));
			case ERPGSkillGoverningAttribute::Wisdom:
				return FText::FromString(TEXT("Sagesse"));
			case ERPGSkillGoverningAttribute::Charisma:
				return FText::FromString(TEXT("Charisme"));
			case ERPGSkillGoverningAttribute::None:
			default:
				return FText::FromString(TEXT("Aucun"));
		}
	}

	UTextBlock* AddText(UWidgetTree* WidgetTree, UVerticalBox* Parent, const FText& Text, int32 FontSize, const FMargin& Padding,
		ETextJustify::Type Justification = ETextJustify::Left)
	{
		if (!WidgetTree || !Parent)
		{
			return nullptr;
		}

		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (!TextBlock)
		{
			return nullptr;
		}

		TextBlock->SetText(Text);
		TextBlock->SetAutoWrapText(true);
		TextBlock->SetJustification(Justification);

		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = FontSize;
		TextBlock->SetFont(Font);

		if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(TextBlock))
		{
			Slot->SetPadding(Padding);
			Slot->SetHorizontalAlignment(HAlign_Fill);
		}
		return TextBlock;
	}

	UTextBlock* BranchTitleByIndex(int32 Index, UTextBlock* Left, UTextBlock* Center, UTextBlock* Right)
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

	NativeContentBox = nullptr;
	NativeScrollBox = nullptr;

	Super::NativeDestruct();
}

void UGridSkillsWidget::BindDesignerShell()
{
	if (Button_SkillsTab)
	{
		Button_SkillsTab->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleSkillsTabClicked);
		Button_SkillsTab->OnClicked.AddDynamic(this, &UGridSkillsWidget::HandleSkillsTabClicked);
	}
	if (Button_TalentsTab)
	{
		Button_TalentsTab->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentsTabClicked);
		Button_TalentsTab->OnClicked.AddDynamic(this, &UGridSkillsWidget::HandleTalentsTabClicked);
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

	RebuildPresentation();
	OnSkillsRefreshed.Broadcast();
}

bool UGridSkillsWidget::IsUsingDesignerPresentation() const
{
	return IsValid(Panel_GridSkillsDesignerRoot);
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
	if (!IsUsingDesignerPresentation())
	{
		return;
	}

	if (!View.IsValid())
	{
		if (Text_CharacterName) Text_CharacterName->SetText(FText::FromString(TEXT("Aucun personnage sélectionné")));
		if (Text_ClassLevel) Text_ClassLevel->SetText(FText::GetEmpty());
		if (Text_TalentPoints) Text_TalentPoints->SetText(FText::FromString(TEXT("Points de talent : —")));
		for (int32 Index = 0; Index < 3; ++Index)
		{
			if (UTextBlock* BranchTitle = GridSkillsWidgetPrivate::BranchTitleByIndex(Index, Text_BranchLeft, Text_BranchCenter, Text_BranchRight))
			{
				BranchTitle->SetText(FText::FromString(TEXT("—")));
			}
		}
		return;
	}

	if (Text_CharacterName)
	{
		Text_CharacterName->SetText(View.CharacterName.IsEmpty() ? FText::FromString(TEXT("Personnage sélectionné")) : View.CharacterName);
	}

	if (Text_ClassLevel)
	{
		const FString ClassName = View.ClassDisplayName.IsEmpty() ? View.ClassId.ToString() : View.ClassDisplayName.ToString();
		Text_ClassLevel->SetText(FText::FromString(FString::Printf(TEXT("%s — Niveau %d"), *ClassName, View.CharacterLevel)));
	}

	if (Text_TalentPoints)
	{
		Text_TalentPoints->SetText(FText::FromString(FString::Printf(TEXT("Points de talent : %d"), View.RemainingTalentPoints)));
	}

	FRPGClassPresentationDefinition ClassPresentation;
	if (GetCurrentClassPresentation(ClassPresentation) && Text_ClassLevel)
	{
		Text_ClassLevel->SetColorAndOpacity(FSlateColor(ClassPresentation.AccentColor));
	}

	for (int32 Index = 0; Index < 3; ++Index)
	{
		UTextBlock* BranchTitle = GridSkillsWidgetPrivate::BranchTitleByIndex(Index, Text_BranchLeft, Text_BranchCenter, Text_BranchRight);
		if (!BranchTitle) continue;

		FGridTalentBranchView BranchView;
		FRPGTalentBranchPresentationDefinition BranchPresentation;
		if (GetPresentedTalentBranch(Index, BranchView, BranchPresentation))
		{
			BranchTitle->SetText(BranchPresentation.DisplayName.IsEmpty() ? FText::FromName(BranchView.TalentBranchId) : BranchPresentation.DisplayName);
			BranchTitle->SetColorAndOpacity(FSlateColor(BranchPresentation.AccentColor));
		}
		else
		{
			BranchTitle->SetText(FText::FromString(TEXT("—")));
		}
	}
}

void UGridSkillsWidget::ShowSkillsTab()
{
	if (Switcher_SkillsTalents)
	{
		Switcher_SkillsTalents->SetActiveWidgetIndex(0);
	}
}

void UGridSkillsWidget::ShowTalentsTab()
{
	if (Switcher_SkillsTalents)
	{
		Switcher_SkillsTalents->SetActiveWidgetIndex(1);
	}
}

void UGridSkillsWidget::HandleSkillsTabClicked()
{
	ShowSkillsTab();
}

void UGridSkillsWidget::HandleTalentsTabClicked()
{
	ShowTalentsTab();
}

void UGridSkillsWidget::RebuildPresentation()
{
	if (IsUsingDesignerPresentation())
	{
		ApplyDesignerPresentation();
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	UBorder* RootBorder = Cast<UBorder>(WidgetTree->RootWidget);
	if (!RootBorder)
	{
		UE_LOG(LogGrimrockInGameUI, Warning, TEXT("GridSkillsWidget native fallback requires WBP_GridSkills root widget to remain a Border."));
		return;
	}

	if (!IsValid(NativeScrollBox) || !IsValid(NativeContentBox))
	{
		NativeScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("NativeSkillsScroll"));
		NativeContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("NativeSkillsContent"));
		if (!NativeScrollBox || !NativeContentBox)
		{
			UE_LOG(LogGrimrockInGameUI, Warning, TEXT("GridSkillsWidget failed to construct native fallback presentation widgets."));
			return;
		}

		NativeScrollBox->AddChild(NativeContentBox);
		RootBorder->SetContent(NativeScrollBox);
	}
	else if (RootBorder->GetContent() != NativeScrollBox)
	{
		RootBorder->SetContent(NativeScrollBox);
	}

	NativeContentBox->ClearChildren();

	using namespace GridSkillsWidgetPrivate;

	AddText(WidgetTree, NativeContentBox, FText::FromString(TEXT("Compétences & talents")), 28, FMargin(24.0f, 20.0f, 24.0f, 12.0f), ETextJustify::Center);

	if (!View.IsValid())
	{
		AddText(WidgetTree, NativeContentBox, FText::FromString(TEXT("Aucun personnage sélectionné ou données indisponibles.")), 18, FMargin(32.0f, 24.0f));
		return;
	}

	AddText(WidgetTree, NativeContentBox, View.CharacterName.IsEmpty() ? FText::FromString(TEXT("Personnage sélectionné")) : View.CharacterName, 22,
		FMargin(32.0f, 8.0f, 32.0f, 4.0f));

	AddText(WidgetTree, NativeContentBox,
		FText::FromString(FString::Printf(TEXT("Points de talent : %d disponibles — %d dépensés — %d accordés"), View.RemainingTalentPoints,
			View.SpentTalentPoints, View.GrantedTalentPoints)),
		16, FMargin(32.0f, 0.0f, 32.0f, 20.0f));

	AddText(WidgetTree, NativeContentBox, FText::FromString(TEXT("Compétences")), 22, FMargin(32.0f, 8.0f, 32.0f, 8.0f));

	if (View.Skills.IsEmpty())
	{
		AddText(WidgetTree, NativeContentBox, FText::FromString(TEXT("Aucune compétence définie.")), 16, FMargin(48.0f, 4.0f, 32.0f, 16.0f));
	}
	else
	{
		for (const FGridSkillEntryView& Skill : View.Skills)
		{
			const FString SkillName = Skill.DisplayName.IsEmpty() ? Skill.SkillId.ToString() : Skill.DisplayName.ToString();
			AddText(WidgetTree, NativeContentBox, FText::FromString(FString::Printf(TEXT("%s — Rang %d/%d"), *SkillName, Skill.Rank, Skill.MaxRank)), 18,
				FMargin(48.0f, 6.0f, 32.0f, 2.0f));
			AddText(WidgetTree, NativeContentBox,
				FText::FromString(FString::Printf(TEXT("Attribut : %s%s"), *GetAttributeLabel(Skill.GoverningAttribute).ToString(),
					Skill.bTrained ? TEXT(" — Entraînée") : TEXT(" — Non entraînée"))),
				14, FMargin(64.0f, 0.0f, 32.0f, 2.0f));
			if (!Skill.Description.IsEmpty())
			{
				AddText(WidgetTree, NativeContentBox, Skill.Description, 14, FMargin(64.0f, 0.0f, 40.0f, 8.0f));
			}
		}
	}

	AddText(WidgetTree, NativeContentBox, FText::FromString(TEXT("Talents acquis")), 22, FMargin(32.0f, 20.0f, 32.0f, 8.0f));

	if (View.Talents.IsEmpty())
	{
		AddText(WidgetTree, NativeContentBox, FText::FromString(TEXT("Aucun talent acquis.")), 16, FMargin(48.0f, 4.0f, 32.0f, 20.0f));
	}
	else
	{
		for (const FGridTalentEntryView& Talent : View.Talents)
		{
			const FString TalentName = Talent.DisplayName.IsEmpty() ? Talent.ChoiceId.ToString() : Talent.DisplayName.ToString();
			AddText(WidgetTree, NativeContentBox, FText::FromString(FString::Printf(TEXT("%s — coût %d"), *TalentName, Talent.PointCost)), 18,
				FMargin(48.0f, 6.0f, 32.0f, 2.0f));
			if (!Talent.Description.IsEmpty())
			{
				AddText(WidgetTree, NativeContentBox, Talent.Description, 14, FMargin(64.0f, 0.0f, 40.0f, 8.0f));
			}
		}
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

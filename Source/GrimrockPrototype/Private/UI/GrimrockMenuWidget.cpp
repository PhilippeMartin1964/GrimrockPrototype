#include "UI/GrimrockMenuWidget.h"

#include "Components/WidgetSwitcher.h"
#include "UI/GridMapWidget.h"
#include "UI/GridSkillsWidget.h"
#include "UI/GridSpellbookWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogGrimrockInGameUI, Log, All);

void UGrimrockMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (GetTopTabPage(CurrentTopTab))
	{
		SetActiveTopTab(CurrentTopTab);
	}
}

void UGrimrockMenuWidget::InitializeMenuWidget(AGrimrockPartyPawn* InPartyPawn)
{
	OwningPartyPawn = InPartyPawn;

	if (UGridSkillsWidget* SkillsWidget = GetSkillsWidget())
	{
		SkillsWidget->InitializeSkillsWidget(InPartyPawn);
	}
	if (UGridSpellbookWidget* SpellbookWidget = GetSpellbookWidget())
	{
		SpellbookWidget->InitializeSpellbookWidget(InPartyPawn);
	}
	if (UGridMapWidget* MapWidget = GetMapWidget())
	{
		MapWidget->InitializeMapWidget(InPartyPawn);
	}
}

void UGrimrockMenuWidget::RefreshSkills()
{
	if (UGridSkillsWidget* SkillsWidget = GetSkillsWidget())
	{
		SkillsWidget->RefreshSkills();
	}
}

void UGrimrockMenuWidget::RefreshSpellbook()
{
	if (UGridSpellbookWidget* SpellbookWidget = GetSpellbookWidget())
	{
		SpellbookWidget->RefreshSpellbook();
	}
}

void UGrimrockMenuWidget::RefreshMap()
{
	if (UGridMapWidget* MapWidget = GetMapWidget())
	{
		MapWidget->RefreshMap();
	}
}

UGridSkillsWidget* UGrimrockMenuWidget::GetSkillsWidget() const
{
	return Cast<UGridSkillsWidget>(Page_Skills);
}

UGridSpellbookWidget* UGrimrockMenuWidget::GetSpellbookWidget() const
{
	return Cast<UGridSpellbookWidget>(Page_Spellbook);
}

UGridMapWidget* UGrimrockMenuWidget::GetMapWidget() const
{
	return Cast<UGridMapWidget>(Page_Map);
}

UWidget* UGrimrockMenuWidget::GetTopTabPage(EInventoryTopTab Tab) const
{
	switch (Tab)
	{
		case EInventoryTopTab::Skills:
			return Page_Skills;
		case EInventoryTopTab::Journal:
			return Page_Journal;
		case EInventoryTopTab::Map:
			return Page_Map;
		case EInventoryTopTab::Recipes:
			return Page_Recipes;
		case EInventoryTopTab::Codex:
			return Page_Codex;
		case EInventoryTopTab::Spellbook:
			return Page_Spellbook;
		case EInventoryTopTab::Inventory:
		default:
			return nullptr;
	}
}

void UGrimrockMenuWidget::SetActiveTopTab(EInventoryTopTab NewTab)
{
	UWidget* TargetPage = GetTopTabPage(NewTab);
	if (!WidgetSwitcher_MainContent || !TargetPage)
	{
		UE_LOG(LogGrimrockInGameUI, Warning, TEXT("GrimrockMenu cannot activate Page=%d"), static_cast<int32>(NewTab));
		return;
	}

	CurrentTopTab = NewTab;
	WidgetSwitcher_MainContent->SetActiveWidget(TargetPage);

	if (NewTab == EInventoryTopTab::Skills)
	{
		RefreshSkills();
	}
	else if (NewTab == EInventoryTopTab::Spellbook)
	{
		RefreshSpellbook();
	}
	else if (NewTab == EInventoryTopTab::Map)
	{
		RefreshMap();
	}

	UE_LOG(LogGrimrockInGameUI, VeryVerbose, TEXT("GrimrockMenu active Page=%d Widget=%s"), static_cast<int32>(NewTab), *GetNameSafe(TargetPage));
}

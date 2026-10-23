#include "UI/GridCombatTooltipWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Runtime/Combat/GridCombatEventFeedFormatter.h"

#define LOCTEXT_NAMESPACE "GridCombatTooltipWidget"

namespace
{
	void SetIfPresent(UTextBlock* Target, const FText& Text)
	{
		if (Target) Target->SetText(Text);
	}

	FText SafeName(const FText& Text, const FText& Fallback)
	{
		return Text.IsEmpty() ? Fallback : Text;
	}
}

void UGridCombatTooltipWidget::InitializeCombatTooltip(const FGridHudEventEntry& Entry)
{
	CombatEntry = Entry;
	RefreshTooltip();
}

void UGridCombatTooltipWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshTooltip();
}

FText UGridCombatTooltipWidget::FormatSourceRow(const FGridArmorAbsorptionSourceTrace& Source)
{
	return FText::FromString(FString::Printf(TEXT("%s | %d | -%d | %d"),
		*Source.DisplayName.ToString(), Source.Before, Source.Absorbed, Source.After));
}

void UGridCombatTooltipWidget::RefreshTooltip()
{
	if (!CombatEntry.bHasCombatTooltipTrace || !CombatEntry.CombatTooltipTrace.bValid)
	{
		return;
	}
	const FGridCombatResolutionTrace& Trace = CombatEntry.CombatTooltipTrace;
	const FGridCombatDamageTrace& Damage = Trace.Damage;
	SetIfPresent(Text_Attacker, SafeName(CombatEntry.CombatAttacker, LOCTEXT("UnknownAttacker", "Attaquant")));
	SetIfPresent(Text_Defender, SafeName(CombatEntry.CombatDefender, LOCTEXT("UnknownDefender", "Défenseur")));

	const FGridCombatAttackTrace& Attack = Trace.Attack;
	const FText Roll = FGridCombatEventFeedFormatter::FormatRollAsDice(Attack.Roll);
	SetIfPresent(Text_Attack, FText::Format(
		LOCTEXT("AttackRoll", "{0} ; Total : {1}"),
		Roll, FText::AsNumber(Attack.FinalValue)));
	SetIfPresent(Text_Defense, FText::Format(
		LOCTEXT("DefenseRoll", "{0} + {1} = {2}"),
		FText::AsNumber(Trace.Defense.BaseValue),
		FText::AsNumber(Trace.Defense.EvasionModifier),
		FText::AsNumber(Trace.Defense.FinalValue)));

	SetIfPresent(Text_Damage, Damage.bResolved
		? FText::Format(LOCTEXT("DamageResolved", "{0} après modificateurs et résistances"),
			FText::AsNumber(Damage.DamageAfterResistance))
		: LOCTEXT("NoDamage", "Aucun dégât résolu"));

	SetIfPresent(Text_Health, Damage.bResolved
		? FText::Format(LOCTEXT("HealthAfterAttack", "{0} → {1} (−{2} PV)"),
			FText::AsNumber(Damage.HealthBefore), FText::AsNumber(Damage.HealthAfter), FText::AsNumber(Damage.HealthDamage))
		: LOCTEXT("NoHealthChange", "PV inchangés"));

	bool bExhausted = false;
	bool bAlreadyDepleted = false;
	if (VerticalBox_ArmorRows)
	{
		VerticalBox_ArmorRows->ClearChildren();
	}
	for (const FGridArmorAbsorptionSourceTrace& Armor : Damage.PhysicalArmorSources)
	{
		bExhausted |= Armor.bExhaustedByHit;
		bAlreadyDepleted |= Armor.bEquipment && Armor.Before == 0;
		if (!VerticalBox_ArmorRows || !WidgetTree) continue;

		UHorizontalBox* TableRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (!TableRow) continue;
		// Four dynamically generated cells under the UMG-authored header.
		const FText Cells[4] = {
			SafeName(Armor.DisplayName, LOCTEXT("ArmorFallback", "Armure")),
			FText::AsNumber(Armor.Before),
			FText::AsNumber(Armor.Absorbed),
			FText::AsNumber(Armor.After)
		};
		for (int32 Column = 0; Column < 4; ++Column)
		{
			UTextBlock* Cell = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			if (!Cell) continue;
			Cell->SetText(Cells[Column]);
			Cell->SetColorAndOpacity(FSlateColor(Armor.bExhaustedByHit ? WarningTextColor :
				(Column == 2 ? ArmorTextColor : NormalTextColor)));
			if (TableFont.Size > 0) Cell->SetFont(TableFont);
			Cell->SetAutoWrapText(Column == 0);
			if (UHorizontalBoxSlot* CellLayoutSlot = TableRow->AddChildToHorizontalBox(Cell))
			{
				CellLayoutSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				CellLayoutSlot->SetPadding(FMargin(Column == 0 ? 0.f : 6.f, 2.f));
				CellLayoutSlot->SetHorizontalAlignment(Column == 0 ? HAlign_Left : HAlign_Right);
			}
		}
		VerticalBox_ArmorRows->AddChild(TableRow);
	}
	if (Text_Status)
	{
		Text_Status->SetText(bExhausted
			? LOCTEXT("ArmorDepleted", "Protection épuisée !")
			: bAlreadyDepleted ? LOCTEXT("ArmorAlreadyDepleted", "Équipement sans protection restante")
			: FText::GetEmpty());
		Text_Status->SetColorAndOpacity(FSlateColor(WarningTextColor));
	}
}

#undef LOCTEXT_NAMESPACE

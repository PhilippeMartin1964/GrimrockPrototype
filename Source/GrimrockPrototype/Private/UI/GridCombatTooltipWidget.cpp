#include "UI/GridCombatTooltipWidget.h"

#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Widget.h"
#include "Runtime/Combat/GridCombatEventFeedFormatter.h"
#include "UI/GridCombatArmorRowWidget.h"
#include "UI/UILocalizationText.h"

namespace
{
	FText LocalizedSourceName(const FGridArmorAbsorptionSourceTrace& Source)
	{
		return !Source.DisplayName.IsEmpty()
			? Source.DisplayName
			: FUILocalizationText::Get(Source.bEquipment
				? TEXT("Combat.Tooltip.Fallback.Armor")
				: TEXT("Combat.Tooltip.Armor.Character"));
	}

	FText JoinArmorNames(const TArray<FGridArmorAbsorptionSourceTrace>& Sources)
	{
		TArray<FText> Names;
		Names.Reserve(Sources.Num());
		for (const FGridArmorAbsorptionSourceTrace& Source : Sources)
		{
			Names.Add(LocalizedSourceName(Source));
		}
		return FText::Join(FText::FromString(TEXT(", ")), Names);
	}
}

FGridCombatTooltipViewData UGridCombatTooltipWidget::BuildViewData(const FGridHudEventEntry& Entry)
{
	FGridCombatTooltipViewData Data;
	if (!Entry.bHasCombatTooltipTrace || !Entry.CombatTooltipTrace.bValid)
	{
		return Data;
	}

	Data.bValid = true;
	Data.Attacker = Entry.CombatAttacker;
	Data.Defender = Entry.CombatDefender;
	Data.Trace = Entry.CombatTooltipTrace;

	const FGridCombatRollTrace& Roll = Data.Trace.Attack.Roll;
	FGridCombatTooltipDiceNumbers& Dice = Data.AttackDice;
	Dice.Total = Roll.FinalValue;
	if (Roll.bUsed && Roll.Kind == EGridCombatRollTraceKind::Die
		&& Roll.DiceCount > 0 && Roll.DiceSides > 0)
	{
		Dice.bHasDice = true;
		Dice.Count = Roll.DiceCount;
		Dice.Sides = Roll.DiceSides;
		Dice.Offset = Roll.DiceFlatBonus;
		Dice.Rolled = Roll.FinalValue - Roll.DiceFlatBonus;
	}
	else if (Roll.bUsed && Roll.Kind == EGridCombatRollTraceKind::Range
		&& Roll.Maximum >= Roll.Minimum)
	{
		Dice.bHasDice = true;
		Dice.Count = 1;
		Dice.Sides = Roll.Maximum - Roll.Minimum + 1;
		Dice.Offset = Roll.Minimum - 1;
		Dice.Rolled = Roll.FinalValue - Dice.Offset;
	}

	const FGridCombatDamageTrace& Damage = Data.Trace.Damage;
	if (!Damage.bResolved)
	{
		return Data;
	}

	for (const FGridArmorAbsorptionSourceTrace& Source : Damage.PhysicalArmorSources)
	{
		if (Source.Absorbed > 0)
		{
			Data.VisibleArmorRows.Add(Source);
		}
		if (Source.bEquipment && Source.bExhaustedByHit)
		{
			Data.NewlyExhaustedSources.Add(Source);
		}
		else if (Source.bEquipment && Source.Before == 0)
		{
			Data.AlreadyDepletedSources.Add(Source);
		}
	}
	Data.bShowAbsorption = !Data.VisibleArmorRows.IsEmpty();
	return Data;
}

int32 UGridCombatTooltipWidget::CountAbsorbingSources(const FGridCombatDamageTrace& Damage)
{
	if (!Damage.bResolved)
	{
		return 0;
	}
	int32 Count = 0;
	for (const FGridArmorAbsorptionSourceTrace& Source : Damage.PhysicalArmorSources)
	{
		if (Source.Absorbed > 0)
		{
			++Count;
		}
	}
	return Count;
}

FGridCombatTooltipDisplayTexts UGridCombatTooltipWidget::BuildDisplayTexts(
	const FGridCombatTooltipViewData& Data)
{
	FGridCombatTooltipDisplayTexts Texts;
	if (!Data.bValid)
	{
		return Texts;
	}

	Texts.Attacker = Data.Attacker.IsEmpty()
		? FUILocalizationText::Get(TEXT("Combat.Tooltip.Label.Attacker")) : Data.Attacker;
	Texts.Defender = Data.Defender.IsEmpty()
		? FUILocalizationText::Get(TEXT("Combat.Tooltip.Label.Defender")) : Data.Defender;

	const FGridCombatAttackTrace& Attack = Data.Trace.Attack;
	FFormatNamedArguments AttackArgs;
	AttackArgs.Add(TEXT("Roll"), FGridCombatEventFeedFormatter::FormatAttackTerms(Attack));
	AttackArgs.Add(TEXT("Total"), FText::AsNumber(Attack.FinalValue));
	Texts.Attack = FText::Format(
		FUILocalizationText::Get(TEXT("Combat.Tooltip.Value.AttackRoll")), AttackArgs);

	FFormatNamedArguments DefenseArgs;
	DefenseArgs.Add(TEXT("Base"), FText::AsNumber(Data.Trace.Defense.BaseValue));
	DefenseArgs.Add(TEXT("Evasion"), FText::AsNumber(Data.Trace.Defense.EvasionModifier));
	DefenseArgs.Add(TEXT("Total"), FText::AsNumber(Data.Trace.Defense.FinalValue));
	Texts.Defense = FText::Format(
		FUILocalizationText::Get(TEXT("Combat.Tooltip.Value.DefenseRoll")), DefenseArgs);

	const FGridCombatDamageTrace& Damage = Data.Trace.Damage;
	if (Damage.bResolved)
	{
		FFormatNamedArguments DamageArgs;
		DamageArgs.Add(TEXT("Damage"), FText::AsNumber(Damage.DamageAfterResistance));
		Texts.Damage = FText::Format(
			FUILocalizationText::Get(TEXT("Combat.Tooltip.Value.DamageResolved")), DamageArgs);

		FFormatNamedArguments HealthArgs;
		HealthArgs.Add(TEXT("Before"), FText::AsNumber(Damage.HealthBefore));
		HealthArgs.Add(TEXT("After"), FText::AsNumber(Damage.HealthAfter));
		HealthArgs.Add(TEXT("Lost"), FText::AsNumber(Damage.HealthDamage));
		Texts.Health = FText::Format(
			FUILocalizationText::Get(TEXT("Combat.Tooltip.Value.HealthAfterAttack")), HealthArgs);
	}
	else
	{
		Texts.Damage = FUILocalizationText::Get(TEXT("Combat.Tooltip.Value.NoDamage"));
		Texts.Health = FUILocalizationText::Get(TEXT("Combat.Tooltip.Value.NoHealthChange"));
	}

	if (!Data.NewlyExhaustedSources.IsEmpty() || !Data.AlreadyDepletedSources.IsEmpty())
	{
		const bool bNewlyExhausted = !Data.NewlyExhaustedSources.IsEmpty();
		const TArray<FGridArmorAbsorptionSourceTrace>& Sources = bNewlyExhausted
			? Data.NewlyExhaustedSources : Data.AlreadyDepletedSources;
		FFormatNamedArguments StatusArgs;
		StatusArgs.Add(TEXT("Name"), JoinArmorNames(Sources));
		Texts.Status = FText::Format(
			FUILocalizationText::Get(bNewlyExhausted
				? TEXT("Combat.Tooltip.Status.DepletedNamed")
				: TEXT("Combat.Tooltip.Status.AlreadyDepletedNamed")), StatusArgs);
		Texts.bShowStatus = !Texts.Status.IsEmpty();
	}
	return Texts;
}

bool UGridCombatTooltipWidget::InitializeCombatTooltip(const FGridHudEventEntry& Entry)
{
	CombatEntry = Entry;
	ViewData = BuildViewData(Entry);
	return ViewData.bValid && PopulateDesignerWidgets(ViewData);
}

bool UGridCombatTooltipWidget::PopulateDesignerWidgets(
	const FGridCombatTooltipViewData& Data)
{
	// Only bind to existing Designer components. Never create or position cells,
	// set fonts, colors, padding or any other graphic property from C++.
	if (!Text_Attacker || !Text_Defender || !Text_Attack || !Text_Defense
		|| !Text_Damage || !Text_Health || !Text_Status
		|| !VerticalBox_AbsorptionSection || !VerticalBox_ArmorRows
		|| (Data.bShowAbsorption && !ArmorRowWidgetClass))
	{
		return false;
	}

	const FGridCombatTooltipDisplayTexts Texts = BuildDisplayTexts(Data);
	Text_Attacker->SetText(Texts.Attacker);
	Text_Defender->SetText(Texts.Defender);
	Text_Attack->SetText(Texts.Attack);
	Text_Defense->SetText(Texts.Defense);
	Text_Damage->SetText(Texts.Damage);
	Text_Health->SetText(Texts.Health);
	Text_Status->SetText(Texts.Status);
	Text_Status->SetVisibility(Texts.bShowStatus
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	VerticalBox_AbsorptionSection->SetVisibility(Data.bShowAbsorption
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	VerticalBox_ArmorRows->ClearChildren();
	for (const FGridArmorAbsorptionSourceTrace& Source : Data.VisibleArmorRows)
	{
		UGridCombatArmorRowWidget* Row = CreateWidget<UGridCombatArmorRowWidget>(
			this, ArmorRowWidgetClass);
		if (!Row)
		{
			return false;
		}
		VerticalBox_ArmorRows->AddChildToVerticalBox(Row);
		// Set AFTER AddChild, so an old BP Event Construct cannot overwrite the
		// four authored TextBlocks using a stale Blueprint ArmorRow variable.
		if (!Row->SetArmorData(Source))
		{
			return false;
		}
	}
	return true;
}

bool UGridCombatTooltipWidget::PresentCombatTooltip_Implementation(
	const FGridCombatTooltipViewData& /*Data*/)
{
	// Kept temporarily for Blueprint serialization/backward compatibility.
	// The native presenter above never calls this obsolete Blueprint graph.
	return false;
}

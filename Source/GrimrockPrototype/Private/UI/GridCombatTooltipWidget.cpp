#include "UI/GridCombatTooltipWidget.h"

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

bool UGridCombatTooltipWidget::InitializeCombatTooltip(const FGridHudEventEntry& Entry)
{
	CombatEntry = Entry;
	ViewData = BuildViewData(Entry);
	return ViewData.bValid && PresentCombatTooltip(ViewData);
}

bool UGridCombatTooltipWidget::PresentCombatTooltip_Implementation(
	const FGridCombatTooltipViewData& Data)
{
	// A WBP that has not yet implemented this function must NOT show a blank
	// or partially outdated tooltip. GridEventFeedWidget uses the old presenter.
	return false;
}

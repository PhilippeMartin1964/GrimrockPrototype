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

	FText FormatLocalizedLine(const TCHAR* Key, const FFormatNamedArguments& Args)
	{
		return FText::Format(FUILocalizationText::Get(Key), Args);
	}

	FText FormatSignedAccuracy(int32 Value)
	{
		if (Value > 0)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Value"), FText::AsNumber(Value));
			return FormatLocalizedLine(TEXT("Combat.Tooltip.Attack.PositiveValue"), Args);
		}
		return FText::AsNumber(Value);
	}

	FGridCombatTooltipDetailTexts FormatCombatDetailLines(const FGridCombatResolutionTrace& Trace)
	{
		FGridCombatTooltipDetailTexts Lines;
		const FGridCombatAttackTrace& Attack = Trace.Attack;
		if (!Attack.Roll.bUsed)
		{
			Lines.AttackDice = FUILocalizationText::Get(Trace.Damage.bDirectDamage
				? TEXT("Combat.Tooltip.Attack.DirectDamage")
				: TEXT("Combat.Tooltip.Attack.NoRoll"));
			Lines.DefenseBase = FUILocalizationText::Get(Trace.Damage.bDirectDamage
				? TEXT("Combat.Tooltip.Defense.DirectDamage")
				: TEXT("Combat.Tooltip.Defense.NoRoll"));
			return Lines;
		}

		FFormatNamedArguments DiceArgs;
		DiceArgs.Add(TEXT("Roll"), FGridCombatEventFeedFormatter::FormatRollAsDice(Attack.Roll));
		Lines.AttackDice = FormatLocalizedLine(TEXT("Combat.Tooltip.Attack.Dice"), DiceArgs);

		if (Attack.SourceAccuracy != 0)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Value"), FormatSignedAccuracy(Attack.SourceAccuracy));
			Lines.AttackSourceAccuracy =
				FormatLocalizedLine(TEXT("Combat.Tooltip.Attack.SourceAccuracy"), Args);
		}
		if (Attack.AttackAccuracyBonus != 0)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Value"), FormatSignedAccuracy(Attack.AttackAccuracyBonus));
			Lines.AttackActionAccuracy =
				FormatLocalizedLine(TEXT("Combat.Tooltip.Attack.ActionAccuracy"), Args);
		}

		FFormatNamedArguments AttackTotalArgs;
		AttackTotalArgs.Add(TEXT("Total"), FText::AsNumber(Attack.FinalValue));
		Lines.AttackTotal = FormatLocalizedLine(TEXT("Combat.Tooltip.Attack.Total"), AttackTotalArgs);

		const FGridCombatDefenseTrace& Defense = Trace.Defense;
		FFormatNamedArguments BaseArgs;
		BaseArgs.Add(TEXT("Base"), FText::AsNumber(Defense.BaseValue));
		Lines.DefenseBase = FormatLocalizedLine(TEXT("Combat.Tooltip.Defense.Base"), BaseArgs);

		if (Defense.EvasionModifier != 0)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Value"), FormatSignedAccuracy(Defense.EvasionModifier));
			Lines.DefenseEvasion =
				FormatLocalizedLine(TEXT("Combat.Tooltip.Defense.Evasion"), Args);
		}

		FFormatNamedArguments DefenseTotalArgs;
		DefenseTotalArgs.Add(TEXT("Total"), FText::AsNumber(Defense.FinalValue));
		Lines.DefenseTotal =
			FormatLocalizedLine(TEXT("Combat.Tooltip.Defense.Total"), DefenseTotalArgs);

		const bool bHit = Attack.bAlwaysHits
			|| (!Attack.bNaturalAutoMiss && (Attack.bNaturalAutoHit
				|| Attack.FinalValue >= Defense.FinalValue));
		const TCHAR* OutcomeKey = nullptr;
		if (Attack.bAlwaysHits)
		{
			OutcomeKey = TEXT("Combat.Tooltip.Outcome.AlwaysHit");
		}
		else if (Attack.bNaturalAutoMiss)
		{
			OutcomeKey = TEXT("Combat.Tooltip.Outcome.NaturalMiss");
		}
		else if (Attack.bNaturalAutoHit)
		{
			OutcomeKey = TEXT("Combat.Tooltip.Outcome.NaturalHit");
		}
		else
		{
			OutcomeKey = bHit
				? TEXT("Combat.Tooltip.Outcome.Hit")
				: TEXT("Combat.Tooltip.Outcome.Miss");
		}
		FFormatNamedArguments OutcomeArgs;
		OutcomeArgs.Add(TEXT("Attack"), FText::AsNumber(Attack.FinalValue));
		OutcomeArgs.Add(TEXT("Defense"), FText::AsNumber(Defense.FinalValue));
		Lines.AttackOutcome = FormatLocalizedLine(OutcomeKey, OutcomeArgs);

		// Preserve the existing critical condition; do not invent item-level
		// sources for an aggregated attack accuracy contribution.
		if (bHit && Attack.bCanCriticalHit && Attack.CriticalChancePercent > 0
			&& Attack.CriticalMinimumRoll <= 20
			&& Attack.Roll.Kind == EGridCombatRollTraceKind::Die
			&& Attack.Roll.DiceCount == 1
			&& Attack.Roll.IndividualRolls.Num() == 1
			&& Attack.Roll.IndividualRolls[0] >= Attack.CriticalMinimumRoll)
		{
			FFormatNamedArguments CriticalArgs;
			CriticalArgs.Add(TEXT("Roll"), FText::AsNumber(Attack.Roll.IndividualRolls[0]));
			CriticalArgs.Add(TEXT("Threshold"), FText::AsNumber(Attack.CriticalMinimumRoll));
			Lines.AttackCritical =
				FormatLocalizedLine(TEXT("Combat.Tooltip.Outcome.CriticalHit"), CriticalArgs);
		}

		return Lines;
	}



	FText FormatSignedDamage(int32 Value)
	{
		if (Value > 0)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Value"), FText::AsNumber(Value));
			return FormatLocalizedLine(TEXT("Combat.Tooltip.Damage.PositiveValue"), Args);
		}
		return FText::AsNumber(Value);
	}

	FText FormatDamageMultiplier(float Value)
	{
		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = 0;
		Options.MaximumFractionalDigits = 6;
		return FText::AsNumber(static_cast<double>(Value), &Options);
	}

	FGridCombatTooltipDamageTexts FormatDamageDetailLines(const FGridCombatDamageTrace& Damage)
	{
		FGridCombatTooltipDamageTexts Lines;
		if (!Damage.bResolved)
		{
			Lines.DamageRoll = FUILocalizationText::Get(TEXT("Combat.Tooltip.Value.NoDamage"));
			return Lines;
		}

		if (Damage.bDirectDamage)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Damage"), FText::AsNumber(Damage.RawDamage));
			Lines.DamageRoll = FormatLocalizedLine(TEXT("Combat.Tooltip.Damage.Direct"), Args);
		}
		else
		{
			if (Damage.Roll.bUsed)
			{
				FFormatNamedArguments Args;
				Args.Add(TEXT("Roll"), FGridCombatEventFeedFormatter::FormatRollAsDice(Damage.Roll));
				Lines.DamageRoll = FormatLocalizedLine(TEXT("Combat.Tooltip.Damage.Roll"), Args);
			}
			if (Damage.SourceDamageBonus != 0)
			{
				FFormatNamedArguments Args;
				Args.Add(TEXT("Value"), FormatSignedDamage(Damage.SourceDamageBonus));
				Lines.DamageSourceBonus =
					FormatLocalizedLine(TEXT("Combat.Tooltip.Damage.SourceBonus"), Args);
			}
			FFormatNamedArguments BaseArgs;
			BaseArgs.Add(TEXT("Damage"), FText::AsNumber(Damage.BaseDamage));
			Lines.DamageBase = FormatLocalizedLine(TEXT("Combat.Tooltip.Damage.Base"), BaseArgs);

			if (Damage.RawDamagePercent != 100)
			{
				FFormatNamedArguments PercentArgs;
				PercentArgs.Add(TEXT("Percent"), FText::AsNumber(Damage.RawDamagePercent));
				Lines.DamageRawPercent =
					FormatLocalizedLine(TEXT("Combat.Tooltip.Damage.RawPercent"), PercentArgs);
				FFormatNamedArguments ScaledArgs;
				ScaledArgs.Add(TEXT("Damage"), FText::AsNumber(Damage.ScaledBaseDamage));
				Lines.DamageScaledBase =
					FormatLocalizedLine(TEXT("Combat.Tooltip.Damage.ScaledBase"), ScaledArgs);
			}
			if (Damage.CriticalDamagePercent != 100)
			{
				FFormatNamedArguments CritArgs;
				CritArgs.Add(TEXT("Percent"), FText::AsNumber(Damage.CriticalDamagePercent));
				Lines.DamageCriticalPercent =
					FormatLocalizedLine(TEXT("Combat.Tooltip.Damage.CriticalPercent"), CritArgs);
				FFormatNamedArguments AfterCritArgs;
				AfterCritArgs.Add(TEXT("Damage"), FText::AsNumber(Damage.RawDamage));
				Lines.DamageAfterCritical =
					FormatLocalizedLine(TEXT("Combat.Tooltip.Damage.AfterCritical"), AfterCritArgs);
			}
		}

		const bool bSourceMultiplier = !FMath::IsNearlyEqual(Damage.SourceDamageMultiplier, 1.0f);
		const bool bTargetMultiplier = !FMath::IsNearlyEqual(Damage.TargetDamageMultiplier, 1.0f);
		if (bSourceMultiplier)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Value"), FormatDamageMultiplier(Damage.SourceDamageMultiplier));
			Lines.DamageSourceMultiplier =
				FormatLocalizedLine(TEXT("Combat.Tooltip.Damage.SourceMultiplier"), Args);
		}
		if (bTargetMultiplier)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Value"), FormatDamageMultiplier(Damage.TargetDamageMultiplier));
			Lines.DamageTargetMultiplier =
				FormatLocalizedLine(TEXT("Combat.Tooltip.Damage.TargetMultiplier"), Args);
		}
		if (bSourceMultiplier || bTargetMultiplier)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Damage"), FText::AsNumber(Damage.DamageAfterMultiplier));
			Lines.DamageAfterMultipliers =
				FormatLocalizedLine(TEXT("Combat.Tooltip.Damage.AfterMultipliers"), Args);
		}

		if (Damage.ResistancePercent != 0)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Percent"), FText::AsNumber(Damage.ResistancePercent));
			Lines.DamageResistance =
				FormatLocalizedLine(TEXT("Combat.Tooltip.Damage.Resistance"), Args);
		}

		// This is the authoritative result before physical/magical armor,
		// NOT health damage and NOT a re-evaluation of the original roll.
		FFormatNamedArguments TotalArgs;
		TotalArgs.Add(TEXT("Damage"), FText::AsNumber(Damage.DamageAfterResistance));
		Lines.DamageTotal =
			FormatLocalizedLine(TEXT("Combat.Tooltip.Damage.TotalBeforeArmor"), TotalArgs);
		return Lines;
	}


	void SetDesignerLine(UTextBlock* TextBlock, const FText& Text)
	{
		if (TextBlock)
		{
			TextBlock->SetText(Text);
			TextBlock->SetVisibility(Text.IsEmpty()
				? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		}
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

FGridCombatTooltipDetailTexts UGridCombatTooltipWidget::BuildDetailTexts(
	const FGridCombatTooltipViewData& Data)
{
	return Data.bValid ? FormatCombatDetailLines(Data.Trace) : FGridCombatTooltipDetailTexts{};
}

FGridCombatTooltipDamageTexts UGridCombatTooltipWidget::BuildDamageTexts(
	const FGridCombatTooltipViewData& Data)
{
	return Data.bValid ? FormatDamageDetailLines(Data.Trace.Damage) : FGridCombatTooltipDamageTexts{};
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

	const FGridCombatDamageTrace& Damage = Data.Trace.Damage;
	if (Damage.bResolved)
	{
		FFormatNamedArguments HealthArgs;
		HealthArgs.Add(TEXT("Before"), FText::AsNumber(Damage.HealthBefore));
		HealthArgs.Add(TEXT("After"), FText::AsNumber(Damage.HealthAfter));
		HealthArgs.Add(TEXT("Lost"), FText::AsNumber(Damage.HealthDamage));
		Texts.Health = FText::Format(
			FUILocalizationText::Get(TEXT("Combat.Tooltip.Value.HealthAfterAttack")), HealthArgs);
	}
	else
	{
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
	// All nine separate Designer fields are required (no legacy multiline UI).
	// The Event Feed still provides a generic text tooltip if this WBP is incomplete.
	if (!Text_Attacker || !Text_Defender
		|| !Text_AttackDice || !Text_AttackSourceAccuracy
		|| !Text_AttackActionAccuracy || !Text_AttackTotal
		|| !Text_DefenseBase || !Text_DefenseEvasion
		|| !Text_DefenseTotal || !Text_AttackOutcome || !Text_AttackCritical
		|| !Text_DamageRoll || !Text_DamageSourceBonus || !Text_DamageBase
		|| !Text_DamageRawPercent || !Text_DamageScaledBase
		|| !Text_DamageCriticalPercent || !Text_DamageAfterCritical
		|| !Text_DamageSourceMultiplier || !Text_DamageTargetMultiplier
		|| !Text_DamageAfterMultipliers || !Text_DamageResistance || !Text_DamageTotal
		|| !Text_Health || !Text_Status
		|| !VerticalBox_AbsorptionSection || !VerticalBox_ArmorRows
		|| (Data.bShowAbsorption && !ArmorRowWidgetClass))
	{
		return false;
	}

	const FGridCombatTooltipDisplayTexts Texts = BuildDisplayTexts(Data);
	Text_Attacker->SetText(Texts.Attacker);
	Text_Defender->SetText(Texts.Defender);
	const FGridCombatTooltipDetailTexts Detail = BuildDetailTexts(Data);
	SetDesignerLine(Text_AttackDice, Detail.AttackDice);
	SetDesignerLine(Text_AttackSourceAccuracy, Detail.AttackSourceAccuracy);
	SetDesignerLine(Text_AttackActionAccuracy, Detail.AttackActionAccuracy);
	SetDesignerLine(Text_AttackTotal, Detail.AttackTotal);
	SetDesignerLine(Text_DefenseBase, Detail.DefenseBase);
	SetDesignerLine(Text_DefenseEvasion, Detail.DefenseEvasion);
	SetDesignerLine(Text_DefenseTotal, Detail.DefenseTotal);
	SetDesignerLine(Text_AttackOutcome, Detail.AttackOutcome);
	SetDesignerLine(Text_AttackCritical, Detail.AttackCritical);
	const FGridCombatTooltipDamageTexts DamageDetail = BuildDamageTexts(Data);
	SetDesignerLine(Text_DamageRoll, DamageDetail.DamageRoll);
	SetDesignerLine(Text_DamageSourceBonus, DamageDetail.DamageSourceBonus);
	SetDesignerLine(Text_DamageBase, DamageDetail.DamageBase);
	SetDesignerLine(Text_DamageRawPercent, DamageDetail.DamageRawPercent);
	SetDesignerLine(Text_DamageScaledBase, DamageDetail.DamageScaledBase);
	SetDesignerLine(Text_DamageCriticalPercent, DamageDetail.DamageCriticalPercent);
	SetDesignerLine(Text_DamageAfterCritical, DamageDetail.DamageAfterCritical);
	SetDesignerLine(Text_DamageSourceMultiplier, DamageDetail.DamageSourceMultiplier);
	SetDesignerLine(Text_DamageTargetMultiplier, DamageDetail.DamageTargetMultiplier);
	SetDesignerLine(Text_DamageAfterMultipliers, DamageDetail.DamageAfterMultipliers);
	SetDesignerLine(Text_DamageResistance, DamageDetail.DamageResistance);
	SetDesignerLine(Text_DamageTotal, DamageDetail.DamageTotal);
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


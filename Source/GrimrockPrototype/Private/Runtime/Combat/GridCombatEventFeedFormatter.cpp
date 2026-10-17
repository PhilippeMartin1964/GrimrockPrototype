#include "Runtime/Combat/GridCombatEventFeedFormatter.h"

#define LOCTEXT_NAMESPACE "GridCombatEventFeed"

namespace
{
	FText FormatSigned(int32 Value)
	{
		return Value > 0
			? FText::Format(LOCTEXT("PositiveSigned", "+{0}"), FText::AsNumber(Value))
			: FText::AsNumber(Value);
	}

	FText FormatMultiplier(float Value)
	{
		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = 0;
		Options.MaximumFractionalDigits = 2;
		return FText::Format(LOCTEXT("MultiplierFormat", "×{0}"), FText::AsNumber(static_cast<double>(Value), &Options));
	}

	FText BuildAttackHeader(const FGridCombatLogEntry& Entry)
	{
		const FText Outcome = !Entry.AttackResult.bHit
			? LOCTEXT("OutcomeMiss", "ÉCHEC")
			: Entry.AttackResult.bCriticalHit
				? LOCTEXT("OutcomeCritical", "COUP CRITIQUE")
				: LOCTEXT("OutcomeHit", "TOUCHÉ");
		FFormatNamedArguments Args;
		Args.Add(TEXT("Source"), Entry.SourceDisplayName);
		Args.Add(TEXT("Target"), Entry.TargetDisplayName);
		Args.Add(TEXT("Outcome"), Outcome);
		return FText::Format(LOCTEXT("AttackHeader", "{Source} attaque {Target} — {Outcome}"), Args);
	}

	void AddAttackDetails(const FGridCombatAttackTrace& Attack, const FGridCombatDefenseTrace& Defense, TArray<FText>& OutDetails)
	{
		OutDetails.Add(LOCTEXT("AttackSection", "Jet d'attaque"));
		if (Attack.Roll.bUsed)
		{
			if (Attack.Roll.Kind == EGridCombatRollTraceKind::Die && Attack.Roll.DiceCount > 0 && Attack.Roll.DiceSides > 0)
			{
				FFormatNamedArguments Args;
				Args.Add(TEXT("Count"), FText::AsNumber(Attack.Roll.DiceCount));
				Args.Add(TEXT("Sides"), FText::AsNumber(Attack.Roll.DiceSides));
				Args.Add(TEXT("Value"), FText::AsNumber(Attack.Roll.FinalValue));
				OutDetails.Add(FText::Format(LOCTEXT("DiceRoll", "{Count}d{Sides} : {Value}"), Args));
			}
			else
			{
				OutDetails.Add(FText::Format(LOCTEXT("AttackRollValue", "Jet : {0}"), FText::AsNumber(Attack.Roll.FinalValue)));
			}
		}
		if (Attack.SourceAccuracy != 0)
		{
			OutDetails.Add(FText::Format(LOCTEXT("SourceAccuracy", "Précision : {0}"), FormatSigned(Attack.SourceAccuracy)));
		}
		if (Attack.AttackAccuracyBonus != 0)
		{
			OutDetails.Add(FText::Format(LOCTEXT("AttackAccuracyBonus", "Bonus d'attaque : {0}"), FormatSigned(Attack.AttackAccuracyBonus)));
		}
		OutDetails.Add(FText::Format(LOCTEXT("AttackTotal", "Total attaque : {0}"), FText::AsNumber(Attack.FinalValue)));
		if (Attack.bCanCriticalHit && Attack.CriticalChancePercent > 0 && Attack.CriticalMinimumRoll <= 20)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Chance"), FText::AsNumber(Attack.CriticalChancePercent));
			Args.Add(TEXT("Threshold"), FText::AsNumber(Attack.CriticalMinimumRoll));
			OutDetails.Add(FText::Format(LOCTEXT("CriticalThreshold", "Critique : {Chance} % (jet naturel {Threshold}+)"), Args));
		}
		OutDetails.Add(LOCTEXT("DefenseSection", "Défense"));
		OutDetails.Add(FText::Format(LOCTEXT("DefenseBase", "Base : {0}"), FText::AsNumber(Defense.BaseValue)));
		if (Defense.EvasionModifier != 0)
		{
			OutDetails.Add(FText::Format(LOCTEXT("DefenseEvasion", "Esquive : {0}"), FormatSigned(Defense.EvasionModifier)));
		}
		OutDetails.Add(FText::Format(LOCTEXT("DefenseTotal", "Total défense : {0}"), FText::AsNumber(Defense.FinalValue)));
	}

	void AddDamageDetails(const FGridCombatDamageTrace& Damage, TArray<FText>& OutDetails)
	{
		if (!Damage.bResolved)
		{
			return;
		}
		OutDetails.Add(LOCTEXT("DamageSection", "Dégâts"));
		if (Damage.Roll.bUsed)
		{
			if (Damage.Roll.Kind == EGridCombatRollTraceKind::Range)
			{
				FFormatNamedArguments Args;
				Args.Add(TEXT("Min"), FText::AsNumber(Damage.Roll.Minimum));
				Args.Add(TEXT("Max"), FText::AsNumber(Damage.Roll.Maximum));
				Args.Add(TEXT("Value"), FText::AsNumber(Damage.Roll.FinalValue));
				OutDetails.Add(FText::Format(LOCTEXT("DamageRangeRoll", "Tirage {Min}..{Max} : {Value}"), Args));
			}
			else if (Damage.Roll.Kind == EGridCombatRollTraceKind::Die)
			{
				FFormatNamedArguments Args;
				Args.Add(TEXT("Count"), FText::AsNumber(Damage.Roll.DiceCount));
				Args.Add(TEXT("Sides"), FText::AsNumber(Damage.Roll.DiceSides));
				Args.Add(TEXT("Value"), FText::AsNumber(Damage.Roll.FinalValue));
				OutDetails.Add(FText::Format(LOCTEXT("DamageDiceRoll", "{Count}d{Sides} : {Value}"), Args));
			}
		}
		if (Damage.SourceDamageBonus != 0)
		{
			OutDetails.Add(FText::Format(LOCTEXT("DamageBonus", "Bonus dégâts : {0}"), FormatSigned(Damage.SourceDamageBonus)));
		}
		if (!Damage.bDirectDamage)
		{
			OutDetails.Add(FText::Format(LOCTEXT("BaseDamage", "Dégâts de base : {0}"), FText::AsNumber(Damage.BaseDamage)));
			if (Damage.RawDamagePercent != 100)
			{
				OutDetails.Add(FText::Format(LOCTEXT("RawDamagePercent", "Coefficient dégâts : {0} %"), FText::AsNumber(Damage.RawDamagePercent)));
				OutDetails.Add(FText::Format(LOCTEXT("ScaledBaseDamage", "Après coefficient : {0}"), FText::AsNumber(Damage.ScaledBaseDamage)));
			}
			if (Damage.CriticalDamagePercent > 100)
			{
				OutDetails.Add(FText::Format(LOCTEXT("CriticalDamagePercent", "Coefficient critique : {0} %"), FText::AsNumber(Damage.CriticalDamagePercent)));
			}
		}
		OutDetails.Add(FText::Format(LOCTEXT("RawDamage", "Dégâts bruts : {0}"), FText::AsNumber(Damage.RawDamage)));
		const bool bHasMultiplier = !FMath::IsNearlyEqual(Damage.SourceDamageMultiplier, 1.0f) || !FMath::IsNearlyEqual(Damage.TargetDamageMultiplier, 1.0f);
		if (bHasMultiplier)
		{
			if (!FMath::IsNearlyEqual(Damage.SourceDamageMultiplier, 1.0f))
			{
				OutDetails.Add(FText::Format(LOCTEXT("SourceMultiplier", "Multiplicateur source : {0}"), FormatMultiplier(Damage.SourceDamageMultiplier)));
			}
			if (!FMath::IsNearlyEqual(Damage.TargetDamageMultiplier, 1.0f))
			{
				OutDetails.Add(FText::Format(LOCTEXT("TargetMultiplier", "Multiplicateur cible : {0}"), FormatMultiplier(Damage.TargetDamageMultiplier)));
			}
			OutDetails.Add(FText::Format(LOCTEXT("AfterMultipliers", "Après multiplicateurs : {0}"), FText::AsNumber(Damage.DamageAfterMultiplier)));
		}
		if (Damage.ResistancePercent != 0)
		{
			OutDetails.Add(FText::Format(LOCTEXT("ResistancePercent", "Résistance : {0} %"), FText::AsNumber(Damage.ResistancePercent)));
		}
		OutDetails.Add(FText::Format(LOCTEXT("AfterResistance", "Après résistance : {0}"), FText::AsNumber(Damage.DamageAfterResistance)));
		if (Damage.PhysicalArmorDamage > 0)
		{
			OutDetails.Add(FText::Format(LOCTEXT("PhysicalArmorAbsorbed", "Armure physique absorbée : {0}"), FText::AsNumber(Damage.PhysicalArmorDamage)));
		}
		if (Damage.MagicalArmorDamage > 0)
		{
			OutDetails.Add(FText::Format(LOCTEXT("MagicalArmorAbsorbed", "Armure magique absorbée : {0}"), FText::AsNumber(Damage.MagicalArmorDamage)));
		}
		OutDetails.Add(FText::Format(LOCTEXT("HealthDamage", "Dégâts PV : {0}"), FText::AsNumber(Damage.HealthDamage)));
		FFormatNamedArguments HealthArgs;
		HealthArgs.Add(TEXT("Before"), FText::AsNumber(Damage.HealthBefore));
		HealthArgs.Add(TEXT("After"), FText::AsNumber(Damage.HealthAfter));
		OutDetails.Add(FText::Format(LOCTEXT("HealthTransition", "PV : {Before} → {After}"), HealthArgs));
	}

	FName StatusEventCode(EGridCombatLogEntryType Type)
	{
		switch (Type)
		{
			case EGridCombatLogEntryType::StatusApplied: return FName(TEXT("Combat.Status.Applied"));
			case EGridCombatLogEntryType::StatusRefreshed: return FName(TEXT("Combat.Status.Refreshed"));
			case EGridCombatLogEntryType::StatusTicked: return FName(TEXT("Combat.Status.Ticked"));
			case EGridCombatLogEntryType::StatusExpired: return FName(TEXT("Combat.Status.Expired"));
			default: return NAME_None;
		}
	}
}

EGridHudEventSemantic FGridCombatEventFeedFormatter::ResolveDamageSemantic(EGridDamageType DamageType)
{
	switch (DamageType)
	{
		case EGridDamageType::Physical: return EGridHudEventSemantic::PhysicalDamage;
		case EGridDamageType::Fire: return EGridHudEventSemantic::Fire;
		case EGridDamageType::Ice: return EGridHudEventSemantic::Ice;
		case EGridDamageType::Lightning: return EGridHudEventSemantic::Lightning;
		case EGridDamageType::Poison: return EGridHudEventSemantic::Poison;
		case EGridDamageType::Holy: return EGridHudEventSemantic::Holy;
		case EGridDamageType::Necrotic: return EGridHudEventSemantic::Necrotic;
		case EGridDamageType::Arcane: return EGridHudEventSemantic::Arcane;
		default: return EGridHudEventSemantic::Default;
	}
}

bool FGridCombatEventFeedFormatter::BuildEntry(const FGridCombatLogEntry& CombatEntry, FGridHudEventEntry& OutEntry)
{
	OutEntry = FGridHudEventEntry();
	const bool bAttack = CombatEntry.Type == EGridCombatLogEntryType::AttackHit || CombatEntry.Type == EGridCombatLogEntryType::AttackMiss;
	const bool bStatus = CombatEntry.Type == EGridCombatLogEntryType::StatusApplied || CombatEntry.Type == EGridCombatLogEntryType::StatusRefreshed ||
		CombatEntry.Type == EGridCombatLogEntryType::StatusTicked || CombatEntry.Type == EGridCombatLogEntryType::StatusExpired;
	if (!bAttack && !bStatus)
	{
		return false;
	}
	OutEntry.Category = EGridHudEventCategory::Combat;
	OutEntry.Visibility = EGridEventVisibility::Always;
	OutEntry.SourceId = CombatEntry.SourceId;
	if (bAttack)
	{
		const FGridAttackResult& Result = CombatEntry.AttackResult;
		OutEntry.EventCode = !Result.bHit ? FName(TEXT("Combat.Attack.Miss"))
			: Result.bCriticalHit ? FName(TEXT("Combat.Attack.Critical")) : FName(TEXT("Combat.Attack.Hit"));
		OutEntry.Semantic = Result.bHit ? ResolveDamageSemantic(Result.DamageType) : EGridHudEventSemantic::Information;
		OutEntry.Header = BuildAttackHeader(CombatEntry);
		if (Result.Trace.bValid)
		{
			AddAttackDetails(Result.Trace.Attack, Result.Trace.Defense, OutEntry.Details);
			if (Result.bHit)
			{
				AddDamageDetails(Result.Trace.Damage, OutEntry.Details);
			}
		}
		else
		{
			OutEntry.Message = CombatEntry.Message;
		}
		return true;
	}
	OutEntry.EventCode = StatusEventCode(CombatEntry.Type);
	OutEntry.Semantic = CombatEntry.Type == EGridCombatLogEntryType::StatusTicked
		? ResolveDamageSemantic(CombatEntry.AttackResult.DamageType) : EGridHudEventSemantic::Information;
	OutEntry.Header = CombatEntry.Message;
	if (CombatEntry.Type == EGridCombatLogEntryType::StatusTicked && CombatEntry.AttackResult.Trace.bValid)
	{
		AddDamageDetails(CombatEntry.AttackResult.Trace.Damage, OutEntry.Details);
	}
	return !OutEntry.EventCode.IsNone();
}

#undef LOCTEXT_NAMESPACE

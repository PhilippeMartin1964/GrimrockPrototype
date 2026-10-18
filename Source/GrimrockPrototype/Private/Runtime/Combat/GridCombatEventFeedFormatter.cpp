#include "Runtime/Combat/GridCombatEventFeedFormatter.h"

#define LOCTEXT_NAMESPACE "GridCombatEventFeed"

namespace
{
	struct FVirtualDiceProjection
	{
		bool bValid = false;
		int32 DiceCount = 0;
		int32 DiceSides = 0;
		int32 RolledValue = 0;
		int32 FlatOffset = 0;
		int32 FinalValue = 0;
		TArray<int32> IndividualRolls;
	};

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
		return FText::Format(
			LOCTEXT("MultiplierFormat", "×{0}"),
			FText::AsNumber(static_cast<double>(Value), &Options));
	}

	FString FormatMultiplierCompact(float Value)
	{
		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = 0;
		Options.MaximumFractionalDigits = 2;
		return FText::AsNumber(static_cast<double>(Value), &Options).ToString();
	}

	void AppendSignedTerm(FString& Expression, int32 Value)
	{
		if (Value > 0)
		{
			Expression += FString::Printf(TEXT(" + %d"), Value);
		}
		else if (Value < 0)
		{
			Expression += FString::Printf(TEXT(" - %d"), FMath::Abs(Value));
		}
	}

	FVirtualDiceProjection ProjectRollToDice(const FGridCombatRollTrace& Roll)
	{
		FVirtualDiceProjection Projection;
		if (!Roll.bUsed)
		{
			return Projection;
		}

		if (Roll.Kind == EGridCombatRollTraceKind::Die &&
			Roll.DiceCount > 0 &&
			Roll.DiceSides > 0)
		{
			Projection.bValid = true;
			Projection.DiceCount = Roll.DiceCount;
			Projection.DiceSides = Roll.DiceSides;
			Projection.RolledValue = Roll.FinalValue;
			Projection.FinalValue = Roll.FinalValue;
			Projection.IndividualRolls = Roll.IndividualRolls;
			return Projection;
		}

		if (Roll.Kind == EGridCombatRollTraceKind::Range && Roll.Maximum >= Roll.Minimum)
		{
			// Uniform inclusive Min..Max is exactly equivalent to
			// 1d(Max-Min+1) + (Min-1). The die may be virtual (for example d7).
			Projection.bValid = true;
			Projection.DiceCount = 1;
			Projection.DiceSides = Roll.Maximum - Roll.Minimum + 1;
			Projection.FlatOffset = Roll.Minimum - 1;
			Projection.FinalValue = Roll.FinalValue;
			Projection.RolledValue = Roll.FinalValue - Projection.FlatOffset;
			Projection.IndividualRolls = { Projection.RolledValue };
			return Projection;
		}

		return Projection;
	}

	FString FormatProjectedDice(const FVirtualDiceProjection& Projection, bool bIncludeResolvedTotal)
	{
		if (!Projection.bValid)
		{
			return FString();
		}

		FString RollValue;
		if (Projection.IndividualRolls.Num() > 1)
		{
			TArray<FString> Parts;
			Parts.Reserve(Projection.IndividualRolls.Num());
			for (const int32 IndividualRoll : Projection.IndividualRolls)
			{
				Parts.Add(FString::FromInt(IndividualRoll));
			}
			RollValue = FString::Join(Parts, TEXT(" + "));
		}
		else
		{
			RollValue = FString::FromInt(Projection.RolledValue);
		}

		FString Result = FString::Printf(
			TEXT("%dd%d (%s)"),
			Projection.DiceCount,
			Projection.DiceSides,
			*RollValue);

		AppendSignedTerm(Result, Projection.FlatOffset);
		if (bIncludeResolvedTotal && Projection.FlatOffset != 0)
		{
			Result += FString::Printf(TEXT(" = %d"), Projection.FinalValue);
		}
		else if (bIncludeResolvedTotal && Projection.IndividualRolls.Num() > 1)
		{
			Result += FString::Printf(TEXT(" = %d"), Projection.FinalValue);
		}
		return Result;
	}

	FString FormatRollAsDiceString(const FGridCombatRollTrace& Roll, bool bIncludeResolvedTotal)
	{
		const FVirtualDiceProjection Projection = ProjectRollToDice(Roll);
		if (Projection.bValid)
		{
			return FormatProjectedDice(Projection, bIncludeResolvedTotal);
		}
		return Roll.bUsed ? FString::FromInt(Roll.FinalValue) : FString();
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

	FText BuildCompactAttackDefenseLine(
		const FGridCombatAttackTrace& Attack,
		const FGridCombatDefenseTrace& Defense)
	{
		FString AttackExpression = FormatRollAsDiceString(Attack.Roll, false);
		if (AttackExpression.IsEmpty())
		{
			AttackExpression = FString::FromInt(Attack.Roll.FinalValue);
		}
		AppendSignedTerm(AttackExpression, Attack.SourceAccuracy);
		AppendSignedTerm(AttackExpression, Attack.AttackAccuracyBonus);
		AttackExpression += FString::Printf(TEXT(" = %d"), Attack.FinalValue);

		FString DefenseExpression = FString::FromInt(Defense.BaseValue);
		AppendSignedTerm(DefenseExpression, Defense.EvasionModifier);
		DefenseExpression += FString::Printf(TEXT(" = %d"), Defense.FinalValue);

		return FText::FromString(FString::Printf(
			TEXT("Attaque : %s; Défense : %s"),
			*AttackExpression,
			*DefenseExpression));
	}

	FText BuildCompactDamageLine(
		const FGridCombatDamageTrace& Damage,
		const FText& TargetDisplayName)
	{
		if (!Damage.bResolved)
		{
			return FText::GetEmpty();
		}

		FString DamageExpression;
		if (Damage.Roll.bUsed)
		{
			DamageExpression = FormatRollAsDiceString(Damage.Roll, false);
			AppendSignedTerm(DamageExpression, Damage.SourceDamageBonus);
		}
		else
		{
			DamageExpression = FString::FromInt(Damage.RawDamage);
		}

		struct FCompactStage
		{
			FString Operation;
			int32 Result = 0;
		};

		TArray<FCompactStage> Stages;
		if (Damage.RawDamagePercent != 100)
		{
			Stages.Add(FCompactStage{
				FString::Printf(TEXT("× %d %%"), Damage.RawDamagePercent),
				Damage.ScaledBaseDamage
			});
		}
		if (Damage.CriticalDamagePercent != 100)
		{
			Stages.Add(FCompactStage{
				FString::Printf(TEXT("× %d %%"), Damage.CriticalDamagePercent),
				Damage.RawDamage
			});
		}

		FString DamageMultiplierOperation;
		if (!FMath::IsNearlyEqual(Damage.SourceDamageMultiplier, 1.0f))
		{
			DamageMultiplierOperation += FString::Printf(
				TEXT("× %s"),
				*FormatMultiplierCompact(Damage.SourceDamageMultiplier));
		}
		if (!FMath::IsNearlyEqual(Damage.TargetDamageMultiplier, 1.0f))
		{
			if (!DamageMultiplierOperation.IsEmpty())
			{
				DamageMultiplierOperation += TEXT(" ");
			}
			DamageMultiplierOperation += FString::Printf(
				TEXT("× %s"),
				*FormatMultiplierCompact(Damage.TargetDamageMultiplier));
		}
		if (!DamageMultiplierOperation.IsEmpty())
		{
			Stages.Add(FCompactStage{ DamageMultiplierOperation, Damage.DamageAfterMultiplier });
		}

		if (!Stages.IsEmpty() &&
			Damage.Roll.bUsed &&
			(Damage.SourceDamageBonus != 0 || ProjectRollToDice(Damage.Roll).FlatOffset != 0))
		{
			DamageExpression = FString::Printf(TEXT("(%s)"), *DamageExpression);
		}

		if (Stages.IsEmpty())
		{
			DamageExpression += FString::Printf(TEXT(" = %d"), Damage.DamageAfterMultiplier);
		}
		else
		{
			for (int32 StageIndex = 0; StageIndex < Stages.Num(); ++StageIndex)
			{
				const bool bFinalStage = StageIndex == Stages.Num() - 1;
				DamageExpression += FString::Printf(
					TEXT(" %s %s %d"),
					*Stages[StageIndex].Operation,
					bFinalStage ? TEXT("=") : TEXT("→"),
					Stages[StageIndex].Result);
			}
		}

		if (Damage.ResistancePercent != 0)
		{
			DamageExpression += FString::Printf(
				TEXT("; Résistance %d %% → %d"),
				Damage.ResistancePercent,
				Damage.DamageAfterResistance);
		}

		const int32 ArmorAbsorbed = Damage.PhysicalArmorDamage + Damage.MagicalArmorDamage;
		if (ArmorAbsorbed > 0)
		{
			DamageExpression += FString::Printf(TEXT("; Armure : -%d"), ArmorAbsorbed);
		}

		const FString TargetLabel = TargetDisplayName.IsEmpty()
			? FString(TEXT("PV"))
			: FString::Printf(TEXT("PV %s"), *TargetDisplayName.ToString());
		DamageExpression += FString::Printf(
			TEXT("; %s : %d → %d"),
			*TargetLabel,
			Damage.HealthBefore,
			Damage.HealthAfter);

		return FText::FromString(FString::Printf(TEXT("Dégâts : %s"), *DamageExpression));
	}

	FText BuildCompactAttackMessage(const FGridCombatLogEntry& Entry)
	{
		const FGridAttackResult& Result = Entry.AttackResult;
		if (!Result.Trace.bValid)
		{
			return Entry.Message;
		}

		TArray<FText> Lines;
		Lines.Add(BuildCompactAttackDefenseLine(Result.Trace.Attack, Result.Trace.Defense));
		if (Result.bHit && Result.Trace.Damage.bResolved)
		{
			Lines.Add(BuildCompactDamageLine(Result.Trace.Damage, Entry.TargetDisplayName));
		}
		return FText::Join(FText::FromString(TEXT("\n")), Lines);
	}

	void AddAttackDetails(
		const FGridCombatAttackTrace& Attack,
		const FGridCombatDefenseTrace& Defense,
		TArray<FText>& OutDetails)
	{
		OutDetails.Add(LOCTEXT("AttackSection", "Jet d'attaque"));
		if (Attack.Roll.bUsed)
		{
			const FString DiceText = FormatRollAsDiceString(Attack.Roll, true);
			OutDetails.Add(FText::Format(
				LOCTEXT("AttackDiceRoll", "Jet : {0}"),
				FText::FromString(DiceText)));
		}
		if (Attack.SourceAccuracy != 0)
		{
			OutDetails.Add(FText::Format(
				LOCTEXT("SourceAccuracy", "Précision : {0}"),
				FormatSigned(Attack.SourceAccuracy)));
		}
		if (Attack.AttackAccuracyBonus != 0)
		{
			OutDetails.Add(FText::Format(
				LOCTEXT("AttackAccuracyBonus", "Bonus d'attaque : {0}"),
				FormatSigned(Attack.AttackAccuracyBonus)));
		}
		OutDetails.Add(FText::Format(
			LOCTEXT("AttackTotal", "Total attaque : {0}"),
			FText::AsNumber(Attack.FinalValue)));

		if (Attack.bCanCriticalHit &&
			Attack.CriticalChancePercent > 0 &&
			Attack.CriticalMinimumRoll <= 20)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Chance"), FText::AsNumber(Attack.CriticalChancePercent));
			Args.Add(TEXT("Threshold"), FText::AsNumber(Attack.CriticalMinimumRoll));
			OutDetails.Add(FText::Format(
				LOCTEXT("CriticalThreshold", "Critique : {Chance} % (jet naturel {Threshold}+)"),
				Args));
		}

		OutDetails.Add(LOCTEXT("DefenseSection", "Défense"));
		OutDetails.Add(FText::Format(
			LOCTEXT("DefenseBase", "Base : {0}"),
			FText::AsNumber(Defense.BaseValue)));
		if (Defense.EvasionModifier != 0)
		{
			OutDetails.Add(FText::Format(
				LOCTEXT("DefenseEvasion", "Esquive : {0}"),
				FormatSigned(Defense.EvasionModifier)));
		}
		OutDetails.Add(FText::Format(
			LOCTEXT("DefenseTotal", "Total défense : {0}"),
			FText::AsNumber(Defense.FinalValue)));
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
			OutDetails.Add(FText::Format(
				LOCTEXT("DamageDiceRoll", "Jet de dégâts : {0}"),
				FGridCombatEventFeedFormatter::FormatRollAsDice(Damage.Roll)));
		}
		if (Damage.SourceDamageBonus != 0)
		{
			OutDetails.Add(FText::Format(
				LOCTEXT("DamageBonus", "Bonus dégâts : {0}"),
				FormatSigned(Damage.SourceDamageBonus)));
		}
		if (!Damage.bDirectDamage)
		{
			OutDetails.Add(FText::Format(
				LOCTEXT("BaseDamage", "Dégâts de base : {0}"),
				FText::AsNumber(Damage.BaseDamage)));
			if (Damage.RawDamagePercent != 100)
			{
				OutDetails.Add(FText::Format(
					LOCTEXT("RawDamagePercent", "Coefficient dégâts : {0} %"),
					FText::AsNumber(Damage.RawDamagePercent)));
				OutDetails.Add(FText::Format(
					LOCTEXT("ScaledBaseDamage", "Après coefficient : {0}"),
					FText::AsNumber(Damage.ScaledBaseDamage)));
			}
			if (Damage.CriticalDamagePercent > 100)
			{
				OutDetails.Add(FText::Format(
					LOCTEXT("CriticalDamagePercent", "Coefficient critique : {0} %"),
					FText::AsNumber(Damage.CriticalDamagePercent)));
			}
		}

		OutDetails.Add(FText::Format(
			LOCTEXT("RawDamage", "Dégâts bruts : {0}"),
			FText::AsNumber(Damage.RawDamage)));

		const bool bHasMultiplier =
			!FMath::IsNearlyEqual(Damage.SourceDamageMultiplier, 1.0f) ||
			!FMath::IsNearlyEqual(Damage.TargetDamageMultiplier, 1.0f);
		if (bHasMultiplier)
		{
			if (!FMath::IsNearlyEqual(Damage.SourceDamageMultiplier, 1.0f))
			{
				OutDetails.Add(FText::Format(
					LOCTEXT("SourceMultiplier", "Multiplicateur source : {0}"),
					FormatMultiplier(Damage.SourceDamageMultiplier)));
			}
			if (!FMath::IsNearlyEqual(Damage.TargetDamageMultiplier, 1.0f))
			{
				OutDetails.Add(FText::Format(
					LOCTEXT("TargetMultiplier", "Multiplicateur cible : {0}"),
					FormatMultiplier(Damage.TargetDamageMultiplier)));
			}
			OutDetails.Add(FText::Format(
				LOCTEXT("AfterMultipliers", "Après multiplicateurs : {0}"),
				FText::AsNumber(Damage.DamageAfterMultiplier)));
		}
		if (Damage.ResistancePercent != 0)
		{
			OutDetails.Add(FText::Format(
				LOCTEXT("ResistancePercent", "Résistance : {0} %"),
				FText::AsNumber(Damage.ResistancePercent)));
		}

		OutDetails.Add(FText::Format(
			LOCTEXT("AfterResistance", "Après résistance : {0}"),
			FText::AsNumber(Damage.DamageAfterResistance)));

		if (Damage.PhysicalArmorDamage > 0)
		{
			OutDetails.Add(FText::Format(
				LOCTEXT("PhysicalArmorAbsorbed", "Armure physique absorbée : {0}"),
				FText::AsNumber(Damage.PhysicalArmorDamage)));
		}
		if (Damage.MagicalArmorDamage > 0)
		{
			OutDetails.Add(FText::Format(
				LOCTEXT("MagicalArmorAbsorbed", "Armure magique absorbée : {0}"),
				FText::AsNumber(Damage.MagicalArmorDamage)));
		}

		OutDetails.Add(FText::Format(
			LOCTEXT("HealthDamage", "Dégâts PV : {0}"),
			FText::AsNumber(Damage.HealthDamage)));

		FFormatNamedArguments HealthArgs;
		HealthArgs.Add(TEXT("Before"), FText::AsNumber(Damage.HealthBefore));
		HealthArgs.Add(TEXT("After"), FText::AsNumber(Damage.HealthAfter));
		OutDetails.Add(FText::Format(
			LOCTEXT("HealthTransition", "PV : {Before} → {After}"),
			HealthArgs));
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

FText FGridCombatEventFeedFormatter::FormatRollAsDice(const FGridCombatRollTrace& Roll)
{
	return FText::FromString(FormatRollAsDiceString(Roll, true));
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

bool FGridCombatEventFeedFormatter::BuildEntry(
	const FGridCombatLogEntry& CombatEntry,
	FGridHudEventEntry& OutEntry)
{
	OutEntry = FGridHudEventEntry();

	const bool bAttack =
		CombatEntry.Type == EGridCombatLogEntryType::AttackHit ||
		CombatEntry.Type == EGridCombatLogEntryType::AttackMiss;
	const bool bStatus =
		CombatEntry.Type == EGridCombatLogEntryType::StatusApplied ||
		CombatEntry.Type == EGridCombatLogEntryType::StatusRefreshed ||
		CombatEntry.Type == EGridCombatLogEntryType::StatusTicked ||
		CombatEntry.Type == EGridCombatLogEntryType::StatusExpired;
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
		OutEntry.EventCode = !Result.bHit
			? FName(TEXT("Combat.Attack.Miss"))
			: Result.bCriticalHit
				? FName(TEXT("Combat.Attack.Critical"))
				: FName(TEXT("Combat.Attack.Hit"));
		OutEntry.Semantic = Result.bHit
			? ResolveDamageSemantic(Result.DamageType)
			: EGridHudEventSemantic::Information;
		OutEntry.Header = BuildAttackHeader(CombatEntry);

		if (Result.Trace.bValid)
		{
			OutEntry.Message = BuildCompactAttackMessage(CombatEntry);
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
		? ResolveDamageSemantic(CombatEntry.AttackResult.DamageType)
		: EGridHudEventSemantic::Information;
	OutEntry.Header = CombatEntry.Message;

	if (CombatEntry.Type == EGridCombatLogEntryType::StatusTicked &&
		CombatEntry.AttackResult.Trace.bValid)
	{
		OutEntry.Message = BuildCompactDamageLine(
			CombatEntry.AttackResult.Trace.Damage,
			CombatEntry.TargetDisplayName);
		AddDamageDetails(CombatEntry.AttackResult.Trace.Damage, OutEntry.Details);
	}

	return !OutEntry.EventCode.IsNone();
}

#undef LOCTEXT_NAMESPACE

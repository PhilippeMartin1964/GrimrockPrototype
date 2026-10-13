#include "RPG/StatusEffects/GridStatusEffectPresentation.h"

#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "UI/UILocalizationText.h"

namespace
{
	int32 SaturateInt64ToInt32(int64 Value)
	{
		return Value > static_cast<int64>(MAX_int32) ? MAX_int32 : Value < static_cast<int64>(MIN_int32) ? MIN_int32 : static_cast<int32>(Value);
	}

	FText FormatSignedInteger(int32 Value)
	{
		return FText::FromString(FString::Printf(TEXT("%+d"), Value));
	}

	FText BuildStatusLabel(const FGridStatusEffectPresentationView& View)
	{
		const bool bHasStacks = View.StackCount > 1;
		const bool bHasDuration = !View.CompactDurationText.IsEmpty();

		if (bHasStacks && bHasDuration)
		{
			return FText::FormatNamed(
				FUILocalizationText::Get(TEXT("Status.Label.StackedTimed")),
				TEXT("Name"), View.DisplayName,
				TEXT("Stacks"), View.StackCount,
				TEXT("Duration"), View.CompactDurationText);
		}
		if (bHasStacks)
		{
			return FText::FormatNamed(
				FUILocalizationText::Get(TEXT("Status.Label.Stacked")),
				TEXT("Name"), View.DisplayName,
				TEXT("Stacks"), View.StackCount);
		}
		if (bHasDuration)
		{
			return FText::FormatNamed(
				FUILocalizationText::Get(TEXT("Status.Label.Timed")),
				TEXT("Name"), View.DisplayName,
				TEXT("Duration"), View.CompactDurationText);
		}
		return View.DisplayName;
	}
}

FText FGridStatusEffectPresentationBuilder::ResolveDisplayName(const FGridStatusEffectRuntimeState& State)
{
	return IsValid(State.DefinitionAsset) && !State.DefinitionAsset->DisplayName.IsEmpty() ? State.DefinitionAsset->DisplayName
																						   : FText::FromName(State.EffectId);
}

bool FGridStatusEffectPresentationBuilder::BuildOne(const FGridStatusEffectRuntimeState& State, FGridStatusEffectPresentationView& OutView)
{
	OutView = FGridStatusEffectPresentationView();
	if (!State.IsValid())
	{
		return false;
	}

	OutView.EffectId = State.EffectId;
	OutView.DisplayName = FGridStatusEffectPresentationBuilder::ResolveDisplayName(State);
	OutView.DurationUnit = State.DurationUnit;
	OutView.RemainingDuration = State.RemainingDuration;
	OutView.StackCount = State.StackCount;
	OutView.Potency = State.Potency;
	OutView.CompactDurationText = FormatCompactDuration(State.DurationUnit, State.RemainingDuration);
	OutView.DurationText = FormatDuration(State.DurationUnit, State.RemainingDuration);

	if (IsValid(State.DefinitionAsset))
	{
		const UGridStatusEffectDefinitionAsset& Definition = *State.DefinitionAsset;
		OutView.Description = Definition.Description;
		OutView.Icon = Definition.Icon;
		OutView.Disposition = Definition.Disposition;
		OutView.bPeriodicDamage = Definition.PeriodicDamage.IsEnabled();
		OutView.bSkipActivation = Definition.Control.bSkipActivation;
		OutView.bBlockSpellActions = Definition.Control.bBlockSpellActions;
		OutView.bBlockTranslation = Definition.Control.bBlockTranslation;
		OutView.InitiativeContribution = SaturateInt64ToInt32(static_cast<int64>(Definition.InitiativeModifier) * static_cast<int64>(State.StackCount));
	}

	OutView.LabelText = BuildStatusLabel(OutView);

	TArray<FText> ToolTipLines;
	ToolTipLines.Add(OutView.DisplayName);
	if (!OutView.Description.IsEmpty())
	{
		ToolTipLines.Add(OutView.Description);
	}
	ToolTipLines.Add(
		FText::FormatNamed(
			FUILocalizationText::Get(TEXT("Status.Tooltip.Duration")),
			TEXT("Duration"), OutView.DurationText));
	if (OutView.StackCount > 1)
	{
		ToolTipLines.Add(
			FText::FormatNamed(
				FUILocalizationText::Get(TEXT("Status.Tooltip.Stacks")),
				TEXT("Count"), OutView.StackCount));
	}
	if (OutView.InitiativeContribution != 0)
	{
		ToolTipLines.Add(
			FText::FormatNamed(
				FUILocalizationText::Get(TEXT("Status.Tooltip.Initiative")),
				TEXT("Value"), FormatSignedInteger(OutView.InitiativeContribution)));
	}
	if (OutView.bPeriodicDamage)
	{
		ToolTipLines.Add(FUILocalizationText::Get(TEXT("Status.Tooltip.PeriodicDamage")));
	}
	if (OutView.bSkipActivation)
	{
		ToolTipLines.Add(FUILocalizationText::Get(TEXT("Status.Control.SkipActivation")));
	}
	if (OutView.bBlockSpellActions)
	{
		ToolTipLines.Add(FUILocalizationText::Get(TEXT("Status.Control.BlockSpellActions")));
	}
	if (OutView.bBlockTranslation)
	{
		ToolTipLines.Add(FUILocalizationText::Get(TEXT("Status.Control.BlockTranslation")));
	}
	OutView.ToolTipText = FText::Join(FText::FromString(TEXT("\n")), ToolTipLines);
	return true;
}

void FGridStatusEffectPresentationBuilder::Build(const FGridStatusEffectCollection& StatusEffects, TArray<FGridStatusEffectPresentationView>& OutViews)
{
	OutViews.Reset(StatusEffects.ActiveEffects.Num());
	for (const FGridStatusEffectRuntimeState& State : StatusEffects.ActiveEffects)
	{
		FGridStatusEffectPresentationView View;
		if (BuildOne(State, View))
		{
			OutViews.Add(MoveTemp(View));
		}
	}

	OutViews.Sort(
		[](const FGridStatusEffectPresentationView& Left, const FGridStatusEffectPresentationView& Right)
		{
			return Left.EffectId.ToString() < Right.EffectId.ToString();
		});
}

FText FGridStatusEffectPresentationBuilder::FormatCompactDuration(EGridStatusEffectDurationUnit DurationUnit, int32 RemainingDuration)
{
	const int32 SafeDuration = FMath::Max(0, RemainingDuration);
	switch (DurationUnit)
	{
		case EGridStatusEffectDurationUnit::Turns:
			return FText::FormatNamed(
				FUILocalizationText::Get(TEXT("Status.Duration.Compact.Turns")),
				TEXT("Count"), SafeDuration);
		case EGridStatusEffectDurationUnit::Rounds:
			return FText::FormatNamed(
				FUILocalizationText::Get(TEXT("Status.Duration.Compact.Rounds")),
				TEXT("Count"), SafeDuration);
		case EGridStatusEffectDurationUnit::Permanent:
			return FUILocalizationText::Get(TEXT("Status.Duration.Compact.Permanent"));
		default:
			return FText::GetEmpty();
	}
}

FText FGridStatusEffectPresentationBuilder::FormatDuration(EGridStatusEffectDurationUnit DurationUnit, int32 RemainingDuration)
{
	const int32 SafeDuration = FMath::Max(0, RemainingDuration);
	switch (DurationUnit)
	{
		case EGridStatusEffectDurationUnit::Turns:
			return FText::FormatNamed(
				FUILocalizationText::Get(TEXT("Status.Duration.Turns")),
				TEXT("Count"), SafeDuration);
		case EGridStatusEffectDurationUnit::Rounds:
			return FText::FormatNamed(
				FUILocalizationText::Get(TEXT("Status.Duration.Rounds")),
				TEXT("Count"), SafeDuration);
		case EGridStatusEffectDurationUnit::Permanent:
			return FUILocalizationText::Get(TEXT("Status.Duration.Permanent"));
		default:
			return FText::GetEmpty();
	}
}

FText FGridStatusEffectPresentationBuilder::BuildSummary(const TArray<FGridStatusEffectPresentationView>& Views)
{
	TArray<FText> Labels;
	Labels.Reserve(Views.Num());
	for (const FGridStatusEffectPresentationView& View : Views)
	{
		if (!View.LabelText.IsEmpty())
		{
			Labels.Add(View.LabelText);
		}
	}
	return FText::Join(FText::FromString(TEXT("   ")), Labels);
}

#include "RPG/StatusEffects/GridStatusEffectPresentation.h"

#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"

namespace
{
    int32 SaturateInt64ToInt32 (int64 Value)
    {
        return Value > static_cast<int64> (MAX_int32)
            ? MAX_int32
            : Value < static_cast<int64> (MIN_int32)
                ? MIN_int32
                : static_cast<int32> (Value);
    }

    FText ResolveDisplayName (
        const FGridStatusEffectRuntimeState& State)
    {
        return IsValid (State.DefinitionAsset) &&
                !State.DefinitionAsset->DisplayName.IsEmpty ()
            ? State.DefinitionAsset->DisplayName
            : FText::FromName (State.EffectId);
    }
}

bool FGridStatusEffectPresentationBuilder::BuildOne (
    const FGridStatusEffectRuntimeState& State,
    FGridStatusEffectPresentationView& OutView)
{
    OutView = FGridStatusEffectPresentationView ();
    if (!State.IsValid ())
    {
        return false;
    }

    OutView.EffectId = State.EffectId;
    OutView.DisplayName = ResolveDisplayName (State);
    OutView.DurationUnit = State.DurationUnit;
    OutView.RemainingDuration = State.RemainingDuration;
    OutView.StackCount = State.StackCount;
    OutView.Potency = State.Potency;
    OutView.CompactDurationText = FormatCompactDuration (
        State.DurationUnit,
        State.RemainingDuration);
    OutView.DurationText = FormatDuration (
        State.DurationUnit,
        State.RemainingDuration);

    if (IsValid (State.DefinitionAsset))
    {
        const UGridStatusEffectDefinitionAsset& Definition =
            *State.DefinitionAsset;
        OutView.Description = Definition.Description;
        OutView.Icon = Definition.Icon;
        OutView.Disposition = Definition.Disposition;
        OutView.bPeriodicDamage = Definition.PeriodicDamage.IsEnabled ();
        OutView.bSkipActivation = Definition.Control.bSkipActivation;
        OutView.bBlockSpellActions = Definition.Control.bBlockSpellActions;
        OutView.bBlockTranslation = Definition.Control.bBlockTranslation;
        OutView.InitiativeContribution = SaturateInt64ToInt32 (
            static_cast<int64> (Definition.InitiativeModifier) *
            static_cast<int64> (State.StackCount));
    }

    FString Label = OutView.DisplayName.ToString ();
    if (OutView.StackCount > 1)
    {
        Label += FString::Printf (TEXT (" x%d"), OutView.StackCount);
    }
    if (!OutView.CompactDurationText.IsEmpty ())
    {
        Label += TEXT (" | ");
        Label += OutView.CompactDurationText.ToString ();
    }
    OutView.LabelText = FText::FromString (Label);

    TArray<FString> ToolTipLines;
    ToolTipLines.Add (OutView.DisplayName.ToString ());
    if (!OutView.Description.IsEmpty ())
    {
        ToolTipLines.Add (OutView.Description.ToString ());
    }
    ToolTipLines.Add (FString::Printf (
        TEXT ("Durée : %s"),
        *OutView.DurationText.ToString ()));
    if (OutView.StackCount > 1)
    {
        ToolTipLines.Add (FString::Printf (
            TEXT ("Stacks : %d"),
            OutView.StackCount));
    }
    if (OutView.InitiativeContribution != 0)
    {
        ToolTipLines.Add (FString::Printf (
            TEXT ("Initiative : %+d"),
            OutView.InitiativeContribution));
    }
    if (OutView.bPeriodicDamage)
    {
        ToolTipLines.Add (TEXT ("Dégâts périodiques actifs."));
    }
    if (OutView.bSkipActivation)
    {
        ToolTipLines.Add (TEXT ("La prochaine activation correspondante est perdue."));
    }
    if (OutView.bBlockSpellActions)
    {
        ToolTipLines.Add (TEXT ("Les actions de type Sort sont bloquées."));
    }
    if (OutView.bBlockTranslation)
    {
        ToolTipLines.Add (TEXT ("Les déplacements de case sont bloqués."));
    }
    OutView.ToolTipText = FText::FromString (
        FString::Join (ToolTipLines, TEXT ("\n")));
    return true;
}

void FGridStatusEffectPresentationBuilder::Build (
    const FGridStatusEffectCollection& StatusEffects,
    TArray<FGridStatusEffectPresentationView>& OutViews)
{
    OutViews.Reset (StatusEffects.ActiveEffects.Num ());
    for (const FGridStatusEffectRuntimeState& State :
        StatusEffects.ActiveEffects)
    {
        FGridStatusEffectPresentationView View;
        if (BuildOne (State, View))
        {
            OutViews.Add (MoveTemp (View));
        }
    }

    OutViews.Sort ([] (
        const FGridStatusEffectPresentationView& Left,
        const FGridStatusEffectPresentationView& Right)
    {
        return Left.EffectId.ToString () < Right.EffectId.ToString ();
    });
}

FText FGridStatusEffectPresentationBuilder::FormatCompactDuration (
    EGridStatusEffectDurationUnit DurationUnit,
    int32 RemainingDuration)
{
    switch (DurationUnit)
    {
    case EGridStatusEffectDurationUnit::Turns:
        return FText::FromString (FString::Printf (
            TEXT ("T%d"),
            FMath::Max (0, RemainingDuration)));
    case EGridStatusEffectDurationUnit::Rounds:
        return FText::FromString (FString::Printf (
            TEXT ("R%d"),
            FMath::Max (0, RemainingDuration)));
    case EGridStatusEffectDurationUnit::Permanent:
        return FText::FromString (TEXT ("PERM"));
    default:
        return FText::GetEmpty ();
    }
}

FText FGridStatusEffectPresentationBuilder::FormatDuration (
    EGridStatusEffectDurationUnit DurationUnit,
    int32 RemainingDuration)
{
    const int32 SafeDuration = FMath::Max (0, RemainingDuration);
    switch (DurationUnit)
    {
    case EGridStatusEffectDurationUnit::Turns:
        if (SafeDuration == 1)
        {
            return FText::FromString (TEXT ("1 tour"));
        }
        return FText::FromString (FString::Printf (
            TEXT ("%d tours"),
            SafeDuration));
    case EGridStatusEffectDurationUnit::Rounds:
        if (SafeDuration == 1)
        {
            return FText::FromString (TEXT ("1 manche"));
        }
        return FText::FromString (FString::Printf (
            TEXT ("%d manches"),
            SafeDuration));
    case EGridStatusEffectDurationUnit::Permanent:
        return FText::FromString (TEXT ("Permanent"));
    default:
        return FText::GetEmpty ();
    }
}

FText FGridStatusEffectPresentationBuilder::BuildSummary (
    const TArray<FGridStatusEffectPresentationView>& Views)
{
    TArray<FText> Labels;
    Labels.Reserve (Views.Num ());
    for (const FGridStatusEffectPresentationView& View : Views)
    {
        if (!View.LabelText.IsEmpty ())
        {
            Labels.Add (View.LabelText);
        }
    }
    return FText::Join (FText::FromString (TEXT ("   ")), Labels);
}

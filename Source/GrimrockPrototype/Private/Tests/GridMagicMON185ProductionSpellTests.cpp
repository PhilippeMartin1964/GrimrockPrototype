#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Magic/GridProductionSpellLibrary.h"
#include "Magic/GridSpellEffectResolver.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"

namespace
{
    UGridStatusEffectDefinitionAsset* MakeStatusDefinition (
        FName EffectId,
        EGridStatusEffectStackPolicy StackPolicy = EGridStatusEffectStackPolicy::RefreshDuration)
    {
        UGridStatusEffectDefinitionAsset* Definition =
            NewObject<UGridStatusEffectDefinitionAsset> ();
        Definition->EffectId = EffectId;
        Definition->DisplayName = FText::FromName (EffectId);
        Definition->DurationUnit = EGridStatusEffectDurationUnit::Rounds;
        Definition->DefaultDuration = 2;
        Definition->DefaultPotency = 0;
        Definition->StackPolicy = StackPolicy;
        Definition->MaxStacks = 1;
        return Definition;
    }

    const UGridStatusEffectDefinitionAsset* ResolveNone (FName)
    {
        return nullptr;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON185ProductionDefinitionsTest,
    "Grimrock.Magic.MON18.5.ProductionDefinitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON185ProductionDefinitionsTest::RunTest (const FString& Parameters)
{
    TArray<FGridSpellDefinition> Definitions;
    FGridProductionSpellLibrary::BuildAll (Definitions);
    TestEqual (TEXT ("Four first production spells"), Definitions.Num (), 4);

    TSet<FName> SpellIds;
    for (const FGridSpellDefinition& Definition : Definitions)
    {
        TestEqual (
            *FString::Printf (TEXT ("%s is structurally valid"), *Definition.SpellId.ToString ()),
            FGridSpellContract::ValidateDefinition (Definition),
            EGridSpellValidationError::None);
        SpellIds.Add (Definition.SpellId);
    }
    TestEqual (TEXT ("All production ids are unique"), SpellIds.Num (), 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON185DamageResolutionTest,
    "Grimrock.Magic.MON18.5.DamageResolution",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON185DamageResolutionTest::RunTest (const FString& Parameters)
{
    const FGridSpellDefinition Spell = FGridProductionSpellLibrary::MakeArcaneBolt ();
    int32 Health = 10;
    FGridStatusEffectCollection Statuses;
    FGridSpellEffectResolutionResult Result;
    EGridSpellEffectResolutionRejectReason RejectReason = EGridSpellEffectResolutionRejectReason::InvalidTargetState;
    FString Error;

    TestTrue (
        TEXT ("Arcane Bolt resolves"),
        FGridSpellEffectResolver::ResolveEffects (
            Spell,
            FGuid::NewGuid (),
            10,
            Health,
            Statuses,
            ResolveNone,
            Result,
            RejectReason,
            Error));
    TestEqual (TEXT ("Health reduced by four"), Health, 6);
    TestEqual (TEXT ("Damage receipt"), Result.TotalDamage, 4);
    TestEqual (TEXT ("No reject"), RejectReason, EGridSpellEffectResolutionRejectReason::None);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON185HealingClampTest,
    "Grimrock.Magic.MON18.5.HealingClamp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON185HealingClampTest::RunTest (const FString& Parameters)
{
    const FGridSpellDefinition Spell = FGridProductionSpellLibrary::MakeLesserHeal ();
    FRPGDerivedStats Stats;
    Stats.MaxHealth = 10;
    Stats.CurrentHealth = 8;
    FGridStatusEffectCollection Statuses;
    FGridSpellEffectResolutionResult Result;
    EGridSpellEffectResolutionRejectReason RejectReason = EGridSpellEffectResolutionRejectReason::InvalidTargetState;
    FString Error;

    TestTrue (
        TEXT ("Lesser Heal resolves"),
        FGridSpellEffectResolver::ResolveCharacterEffects (
            Spell,
            FGuid::NewGuid (),
            Stats,
            Statuses,
            ResolveNone,
            Result,
            RejectReason,
            Error));
    TestEqual (TEXT ("Healing clamps to max health"), Stats.CurrentHealth, 10);
    TestEqual (TEXT ("Only missing health is reported"), Result.TotalHealing, 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON185ApplyStatusBridgeTest,
    "Grimrock.Magic.MON18.5.ApplyStatusBridge",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON185ApplyStatusBridgeTest::RunTest (const FString& Parameters)
{
    UGridStatusEffectDefinitionAsset* Haste = MakeStatusDefinition (TEXT ("Status_Haste"));
    const FGridSpellDefinition Spell = FGridProductionSpellLibrary::MakeHaste ();
    int32 Health = 10;
    FGridStatusEffectCollection Statuses;
    FGridSpellEffectResolutionResult Result;
    EGridSpellEffectResolutionRejectReason RejectReason = EGridSpellEffectResolutionRejectReason::InvalidTargetState;
    FString Error;

    const auto Resolver = [Haste] (FName EffectId) -> const UGridStatusEffectDefinitionAsset*
    {
        return EffectId == Haste->EffectId ? Haste : nullptr;
    };

    TestTrue (
        TEXT ("Haste resolves through MON16 collection"),
        FGridSpellEffectResolver::ResolveEffects (
            Spell,
            FGuid::NewGuid (),
            10,
            Health,
            Statuses,
            Resolver,
            Result,
            RejectReason,
            Error));
    TestTrue (TEXT ("Haste status is active"), Statuses.Contains (TEXT ("Status_Haste")));
    TestEqual (TEXT ("One effect receipt"), Result.Effects.Num (), 1);
    TestTrue (TEXT ("Status application mutated target"), Result.Effects[0].bMutatedTarget);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON185RemoveStatusTest,
    "Grimrock.Magic.MON18.5.RemoveStatus",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON185RemoveStatusTest::RunTest (const FString& Parameters)
{
    UGridStatusEffectDefinitionAsset* Poison = MakeStatusDefinition (TEXT ("Status_Poison"));
    FGridStatusEffectCollection Statuses;
    FString AddError;
    TestTrue (
        TEXT ("Poison fixture added"),
        Statuses.TryAdd (*Poison, FGuid::NewGuid (), AddError));

    const FGridSpellDefinition Spell = FGridProductionSpellLibrary::MakeCurePoison ();
    int32 Health = 10;
    FGridSpellEffectResolutionResult Result;
    EGridSpellEffectResolutionRejectReason RejectReason = EGridSpellEffectResolutionRejectReason::InvalidTargetState;
    FString Error;

    TestTrue (
        TEXT ("Cure Poison resolves"),
        FGridSpellEffectResolver::ResolveEffects (
            Spell,
            FGuid::NewGuid (),
            10,
            Health,
            Statuses,
            ResolveNone,
            Result,
            RejectReason,
            Error));
    TestFalse (TEXT ("Poison removed"), Statuses.Contains (TEXT ("Status_Poison")));
    TestTrue (TEXT ("Removal receipt reports mutation"), Result.Effects[0].bMutatedTarget);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON185AtomicFailureTest,
    "Grimrock.Magic.MON18.5.AtomicFailureNoMutation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON185AtomicFailureTest::RunTest (const FString& Parameters)
{
    FGridSpellDefinition Spell = FGridProductionSpellLibrary::MakeArcaneBolt ();
    FGridSpellEffectDefinition MissingStatus;
    MissingStatus.Type = EGridSpellEffectType::ApplyStatusEffect;
    MissingStatus.StatusEffectId = TEXT ("Status_Missing" );
    Spell.Effects.Add (MissingStatus);

    int32 Health = 10;
    FGridStatusEffectCollection Statuses;
    FGridSpellEffectResolutionResult Result;
    EGridSpellEffectResolutionRejectReason RejectReason = EGridSpellEffectResolutionRejectReason::None;
    FString Error;

    TestFalse (
        TEXT ("Missing status definition rejects whole effect batch"),
        FGridSpellEffectResolver::ResolveEffects (
            Spell,
            FGuid::NewGuid (),
            10,
            Health,
            Statuses,
            ResolveNone,
            Result,
            RejectReason,
            Error));
    TestEqual (TEXT ("Damage did not partially apply"), Health, 10);
    TestTrue (TEXT ("Statuses unchanged"), Statuses.IsEmpty ());
    TestEqual (
        TEXT ("Correct reject reason"),
        RejectReason,
        EGridSpellEffectResolutionRejectReason::MissingStatusEffectDefinition);
    return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Magic/GridPartySpellbookComponent.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON182CharacterRegistrationTest,
    "Grimrock.Magic.MON18.2.CharacterRegistration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON182CharacterRegistrationTest::RunTest (const FString& Parameters)
{
    UGridPartySpellbookComponent* Spellbook = NewObject<UGridPartySpellbookComponent> ();
    const FGuid CharacterId (18, 2, 1, 1);

    TestTrue (TEXT ("Valid character can be registered"), Spellbook->EnsureCharacterSpellbook (CharacterId));
    TestTrue (TEXT ("Registration is idempotent"), Spellbook->EnsureCharacterSpellbook (CharacterId));
    TestEqual (TEXT ("Exactly one spellbook exists"), Spellbook->SpellbookState.CharacterSpellbooks.Num (), 1);
    TestTrue (TEXT ("State remains valid"), Spellbook->SpellbookState.IsValid ());
    TestFalse (TEXT ("Invalid CharacterId is rejected"), Spellbook->EnsureCharacterSpellbook (FGuid ()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON182LearnForgetTest,
    "Grimrock.Magic.MON18.2.LearnForget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON182LearnForgetTest::RunTest (const FString& Parameters)
{
    UGridPartySpellbookComponent* Spellbook = NewObject<UGridPartySpellbookComponent> ();
    const FGuid CharacterId (18, 2, 2, 1);
    const FName SpellId (TEXT ("Spell_MagicMissile"));
    Spellbook->EnsureCharacterSpellbook (CharacterId);

    TestEqual (
        TEXT ("First learn succeeds"),
        Spellbook->LearnSpell (CharacterId, SpellId),
        EGridSpellbookMutationResult::Success);
    TestTrue (TEXT ("Spell becomes known"), Spellbook->KnowsSpell (CharacterId, SpellId));
    TestEqual (
        TEXT ("Duplicate learn is rejected"),
        Spellbook->LearnSpell (CharacterId, SpellId),
        EGridSpellbookMutationResult::AlreadyKnown);
    TestEqual (
        TEXT ("NAME_None cannot be learned"),
        Spellbook->LearnSpell (CharacterId, NAME_None),
        EGridSpellbookMutationResult::InvalidSpell);
    TestEqual (
        TEXT ("Forget succeeds"),
        Spellbook->ForgetSpell (CharacterId, SpellId),
        EGridSpellbookMutationResult::Success);
    TestFalse (TEXT ("Forgotten spell is no longer known"), Spellbook->KnowsSpell (CharacterId, SpellId));
    TestEqual (
        TEXT ("Forgetting unknown spell is explicit"),
        Spellbook->ForgetSpell (CharacterId, SpellId),
        EGridSpellbookMutationResult::NotKnown);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON182CharacterIsolationTest,
    "Grimrock.Magic.MON18.2.CharacterIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON182CharacterIsolationTest::RunTest (const FString& Parameters)
{
    UGridPartySpellbookComponent* Spellbook = NewObject<UGridPartySpellbookComponent> ();
    const FGuid MageId (18, 2, 3, 1);
    const FGuid ClericId (18, 2, 3, 2);
    const FName FireballId (TEXT ("Spell_Fireball"));
    const FName HealId (TEXT ("Spell_Heal"));

    Spellbook->EnsureCharacterSpellbook (MageId);
    Spellbook->EnsureCharacterSpellbook (ClericId);
    Spellbook->LearnSpell (MageId, FireballId);
    Spellbook->LearnSpell (ClericId, HealId);

    TestTrue (TEXT ("Mage knows Fireball"), Spellbook->KnowsSpell (MageId, FireballId));
    TestFalse (TEXT ("Mage does not inherit Heal"), Spellbook->KnowsSpell (MageId, HealId));
    TestTrue (TEXT ("Cleric knows Heal"), Spellbook->KnowsSpell (ClericId, HealId));
    TestFalse (TEXT ("Cleric does not inherit Fireball"), Spellbook->KnowsSpell (ClericId, FireballId));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON182StableIdentityTest,
    "Grimrock.Magic.MON18.2.StableIdentity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON182StableIdentityTest::RunTest (const FString& Parameters)
{
    UGridPartySpellbookComponent* Spellbook = NewObject<UGridPartySpellbookComponent> ();
    const FGuid CharacterId (18, 2, 4, 1);
    Spellbook->EnsureCharacterSpellbook (CharacterId);
    Spellbook->LearnSpell (CharacterId, TEXT ("Spell_Haste"));
    Spellbook->LearnSpell (CharacterId, TEXT ("Spell_Heal"));

    const TArray<FName> KnownSpellIds = Spellbook->GetKnownSpellIds (CharacterId);
    TestEqual (TEXT ("Spellbook stores two stable ids"), KnownSpellIds.Num (), 2);
    TestEqual (TEXT ("First identity is preserved"), KnownSpellIds[0], FName (TEXT ("Spell_Haste")));
    TestEqual (TEXT ("Second identity is preserved"), KnownSpellIds[1], FName (TEXT ("Spell_Heal")));

    FString ValidationError;
    TestTrue (TEXT ("Runtime state validates"), Spellbook->ValidateSpellbookState (ValidationError));
    TestTrue (TEXT ("No validation error"), ValidationError.IsEmpty ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON182TransientContractTest,
    "Grimrock.Magic.MON18.2.TransientContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON182TransientContractTest::RunTest (const FString& Parameters)
{
    const FProperty* StateProperty = FindFProperty<FProperty> (
        UGridPartySpellbookComponent::StaticClass (),
        GET_MEMBER_NAME_CHECKED (UGridPartySpellbookComponent, SpellbookState));

    TestNotNull (TEXT ("SpellbookState property exists"), StateProperty);
    if (StateProperty)
    {
        TestTrue (
            TEXT ("SpellbookState stays transient until MON18.8"),
            StateProperty->HasAnyPropertyFlags (CPF_Transient));
    }
    return true;
}

#endif

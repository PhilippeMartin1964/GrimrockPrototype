#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Magic/GridPartySpellbookComponent.h"
#include "Magic/GridProductionSpellLibrary.h"
#include "Magic/GridSpellbookPersistence.h"
#include "Misc/AutomationTest.h"
#include "RPG/RPGSaveMigrationService.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Save/GrimrockPartySaveGame.h"

namespace
{
    FGridCombatHotbarBinding MakeMON188SpellBinding (
        FName SpellId,
        int32 SlotIndex)
    {
        FGridCombatHotbarBinding Binding;
        Binding.SlotIndex = SlotIndex;
        Binding.ActionId = SpellId;
        Binding.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
        Binding.SourceDefinitionId = SpellId;
        return Binding;
    }

    FGridCharacterInventoryState MakeMON188Character (
        const FGuid& CharacterId)
    {
        FGridCharacterInventoryState Character;
        Character.CharacterId = CharacterId;
        Character.DisplayName = FText::FromString (TEXT ("MON18.8 Hero"));
        Character.Level = 1;
        Character.Experience = 0;
        Character.CombatHotbarSlots.SetNum (FGridCombatHotbarBinding::SlotCount);
        for (int32 SlotIndex = 0;
            SlotIndex < Character.CombatHotbarSlots.Num ();
            ++SlotIndex)
        {
            Character.CombatHotbarSlots[SlotIndex].Reset (SlotIndex);
        }
        return Character;
    }

    FGridPartyInventoryState MakeMON188Party (
        const TArray<FGuid>& ActiveIds,
        const TArray<FGuid>& PoolIds = {})
    {
        FGridPartyInventoryState Party;
        Party.MaxActiveCharacters = FMath::Max (4, ActiveIds.Num ());
        Party.SelectedCharacterIndex = ActiveIds.IsEmpty () ? INDEX_NONE : 0;
        Party.bInitialCharacterCreationCompleted = !ActiveIds.IsEmpty ();
        for (const FGuid& CharacterId : ActiveIds)
        {
            Party.ActiveCharacters.Add (MakeMON188Character (CharacterId));
        }
        for (const FGuid& CharacterId : PoolIds)
        {
            Party.CharacterPool.Add (MakeMON188Character (CharacterId));
        }
        Party.ActiveEquipment.SetNum (Party.ActiveCharacters.Num ());
        return Party;
    }

    void AddMON188EmptyProgressionSnapshots (
        UGrimrockPartySaveGame* Save)
    {
        if (!Save)
        {
            return;
        }
        Save->ClassProgressionStates.Reset ();
        for (const FGridCharacterInventoryState& Character :
            Save->PartyInventoryState.ActiveCharacters)
        {
            FRPGCharacterProgressionSaveState Progression;
            Progression.CharacterId = Character.CharacterId;
            Save->ClassProgressionStates.Add (Progression);
        }
    }

    struct FMON188DiskSlot
    {
        FString SlotName;
        int32 UserIndex = 0;

        FMON188DiskSlot ()
            : SlotName (FString::Printf (
                TEXT ("MON188_Spellbook_%s"),
                *FGuid::NewGuid ().ToString (EGuidFormats::Digits)))
        {
            UGameplayStatics::DeleteGameInSlot (SlotName, UserIndex);
        }

        ~FMON188DiskSlot ()
        {
            UGameplayStatics::DeleteGameInSlot (SlotName, UserIndex);
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMagicMON188SingleCharacterRoundTripTest,
    "Grimrock.Magic.MON18.8.SingleCharacterRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188SingleCharacterRoundTripTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    const FGuid CharacterId = FGuid (18, 8, 1, 1);
    const FGridPartyInventoryState Party = MakeMON188Party ({ CharacterId });

    FGridPartySpellbookState Runtime;
    TestTrue (TEXT ("Character spellbook is registered"),
        Runtime.EnsureCharacter (CharacterId));
    TestEqual (TEXT ("Arcane Bolt is learned"),
        Runtime.LearnSpell (CharacterId, FGridProductionSpellLibrary::ArcaneBoltId ()),
        EGridSpellbookMutationResult::Success);
    TestEqual (TEXT ("Lesser Heal is learned"),
        Runtime.LearnSpell (CharacterId, FGridProductionSpellLibrary::LesserHealId ()),
        EGridSpellbookMutationResult::Success);

    TArray<FGridCharacterSpellbookSaveState> Saved;
    FString Error;
    TestTrue (TEXT ("Runtime spellbook captures"),
        FGridSpellbookPersistence::CapturePartySpellbooks (
            Party, Runtime, Saved, Error));
    TestEqual (TEXT ("Sparse snapshot contains one character"),
        Saved.Num (), 1);

    FGridPartySpellbookState Restored;
    TestTrue (TEXT ("Saved spellbook restores"),
        FGridSpellbookPersistence::RestorePartySpellbooks (
            Party, Saved, Restored, Error));
    TestTrue (TEXT ("Arcane Bolt survives"),
        Restored.KnowsSpell (
            CharacterId,
            FGridProductionSpellLibrary::ArcaneBoltId ()));
    TestTrue (TEXT ("Lesser Heal survives"),
        Restored.KnowsSpell (
            CharacterId,
            FGridProductionSpellLibrary::LesserHealId ()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMagicMON188MultipleCharactersRoundTripTest,
    "Grimrock.Magic.MON18.8.MultipleCharactersRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188MultipleCharactersRoundTripTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    const FGuid MageId = FGuid (18, 8, 2, 1);
    const FGuid PriestId = FGuid (18, 8, 2, 2);
    const FGuid ReserveId = FGuid (18, 8, 2, 3);
    const FGridPartyInventoryState Party =
        MakeMON188Party ({ MageId, PriestId }, { ReserveId });

    FGridPartySpellbookState Runtime;
    Runtime.EnsureCharacter (MageId);
    Runtime.EnsureCharacter (PriestId);
    Runtime.EnsureCharacter (ReserveId);
    Runtime.LearnSpell (MageId, FGridProductionSpellLibrary::ArcaneBoltId ());
    Runtime.LearnSpell (MageId, FGridProductionSpellLibrary::HasteId ());
    Runtime.LearnSpell (PriestId, FGridProductionSpellLibrary::LesserHealId ());

    TArray<FGridCharacterSpellbookSaveState> Saved;
    FString Error;
    TestTrue (TEXT ("Multi-character spellbooks capture"),
        FGridSpellbookPersistence::CapturePartySpellbooks (
            Party, Runtime, Saved, Error));
    TestEqual (TEXT ("Empty reserve spellbook is omitted from sparse snapshot"),
        Saved.Num (), 2);

    FGridPartySpellbookState Restored;
    TestTrue (TEXT ("Multi-character spellbooks restore"),
        FGridSpellbookPersistence::RestorePartySpellbooks (
            Party, Saved, Restored, Error));
    TestEqual (TEXT ("Every active and pooled character gets a runtime container"),
        Restored.CharacterSpellbooks.Num (), 3);
    TestTrue (TEXT ("Mage keeps Arcane Bolt"),
        Restored.KnowsSpell (MageId, FGridProductionSpellLibrary::ArcaneBoltId ()));
    TestTrue (TEXT ("Mage keeps Haste"),
        Restored.KnowsSpell (MageId, FGridProductionSpellLibrary::HasteId ()));
    TestTrue (TEXT ("Priest keeps Lesser Heal"),
        Restored.KnowsSpell (PriestId, FGridProductionSpellLibrary::LesserHealId ()));
    const FGridCharacterSpellbookState* Reserve = Restored.FindSpellbook (ReserveId);
    TestTrue (TEXT ("Reserve character restores an empty spellbook"),
        Reserve && Reserve->KnownSpellIds.IsEmpty ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMagicMON188V5MigrationCreatesEmptySpellbookTest,
    "Grimrock.Magic.MON18.8.V5MigrationCreatesEmptySpellbook",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188V5MigrationCreatesEmptySpellbookTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    const FGuid CharacterId = FGuid (18, 8, 3, 1);
    UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame> ();
    Save->SaveVersion = 5;
    Save->PartyInventoryState = MakeMON188Party ({ CharacterId });
    AddMON188EmptyProgressionSnapshots (Save);

    FGridCharacterStatusEffectSaveState StatusSentinel;
    StatusSentinel.CharacterId = CharacterId;
    Save->CharacterStatusEffectStates.Add (StatusSentinel);

    FText Error;
    FRPGSaveMigrationReport Report;
    TestTrue (TEXT ("Version five migrates explicitly to version six"),
        FRPGSaveMigrationService::PrepareLoadedSave (
            Save, Error, &Report));
    TestEqual (TEXT ("Migration source is version five"),
        Report.SourceVersion, 5);
    TestEqual (TEXT ("Migration target is version six"),
        Save->SaveVersion, 6);
    TestTrue (TEXT ("Migration is reported"), Report.bMigrated);
    TestEqual (TEXT ("Version five performs no progression reconciliation"),
        Report.ReconciledCharacterCount, 0);
    TestTrue (TEXT ("Old save receives no invented spell knowledge"),
        Save->CharacterSpellbookStates.IsEmpty ());
    TestEqual (TEXT ("Existing MON16 status snapshots are preserved"),
        Save->CharacterStatusEffectStates.Num (), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMagicMON188UnknownDefinitionPreservedTest,
    "Grimrock.Magic.MON18.8.UnknownDefinitionPreserved",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188UnknownDefinitionPreservedTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    const FGuid CharacterId = FGuid (18, 8, 4, 1);
    const FGridPartyInventoryState Party = MakeMON188Party ({ CharacterId });
    FGridCharacterSpellbookSaveState SavedCharacter;
    SavedCharacter.CharacterId = CharacterId;
    SavedCharacter.KnownSpellIds.Add (TEXT ("Spell_RemovedContent"));

    FGridPartySpellbookState Restored;
    FString Error;
    TestTrue (TEXT ("Unknown definition identity does not invalidate persistence"),
        FGridSpellbookPersistence::RestorePartySpellbooks (
            Party, { SavedCharacter }, Restored, Error));
    TestTrue (TEXT ("Unknown SpellId remains known"),
        Restored.KnowsSpell (CharacterId, TEXT ("Spell_RemovedContent")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMagicMON188InvalidSpellIdRejectedTest,
    "Grimrock.Magic.MON18.8.InvalidSpellIdRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188InvalidSpellIdRejectedTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    const FGuid CharacterId = FGuid (18, 8, 5, 1);
    const FGridPartyInventoryState Party = MakeMON188Party ({ CharacterId });
    FGridCharacterSpellbookSaveState SavedCharacter;
    SavedCharacter.CharacterId = CharacterId;
    SavedCharacter.KnownSpellIds.Add (NAME_None);

    FString Error;
    TestFalse (TEXT ("NAME_None SpellId is rejected"),
        FGridSpellbookPersistence::ValidateSavedPartySpellbooks (
            Party, { SavedCharacter }, Error));
    TestTrue (TEXT ("Invalid SpellId reports a reason"), !Error.IsEmpty ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMagicMON188DuplicateSpellRejectedTest,
    "Grimrock.Magic.MON18.8.DuplicateSpellRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188DuplicateSpellRejectedTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    const FGuid CharacterId = FGuid (18, 8, 6, 1);
    const FGridPartyInventoryState Party = MakeMON188Party ({ CharacterId });
    FGridCharacterSpellbookSaveState SavedCharacter;
    SavedCharacter.CharacterId = CharacterId;
    SavedCharacter.KnownSpellIds = {
        FGridProductionSpellLibrary::ArcaneBoltId (),
        FGridProductionSpellLibrary::ArcaneBoltId ()
    };

    FString Error;
    TestFalse (TEXT ("Duplicate SpellId is rejected"),
        FGridSpellbookPersistence::ValidateSavedPartySpellbooks (
            Party, { SavedCharacter }, Error));
    TestTrue (TEXT ("Duplicate rejection reports a reason"), !Error.IsEmpty ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMagicMON188OrphanCharacterRejectedTest,
    "Grimrock.Magic.MON18.8.OrphanCharacterRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188OrphanCharacterRejectedTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    const FGuid CharacterId = FGuid (18, 8, 7, 1);
    const FGridPartyInventoryState Party = MakeMON188Party ({ CharacterId });
    FGridCharacterSpellbookSaveState SavedCharacter;
    SavedCharacter.CharacterId = FGuid (18, 8, 7, 99);
    SavedCharacter.KnownSpellIds.Add (
        FGridProductionSpellLibrary::ArcaneBoltId ());

    FString Error;
    TestFalse (TEXT ("Orphan CharacterId is rejected"),
        FGridSpellbookPersistence::ValidateSavedPartySpellbooks (
            Party, { SavedCharacter }, Error));
    TestTrue (TEXT ("Orphan rejection reports a reason"), !Error.IsEmpty ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMagicMON188AtomicRestoreFailureTest,
    "Grimrock.Magic.MON18.8.AtomicRestoreFailure",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188AtomicRestoreFailureTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    const FGuid CharacterId = FGuid (18, 8, 8, 1);
    const FGridPartyInventoryState Party = MakeMON188Party ({ CharacterId });

    FGridPartySpellbookState Runtime;
    Runtime.EnsureCharacter (CharacterId);
    Runtime.LearnSpell (CharacterId, FGridProductionSpellLibrary::HasteId ());

    FGridCharacterSpellbookSaveState InvalidSaved;
    InvalidSaved.CharacterId = CharacterId;
    InvalidSaved.KnownSpellIds = { NAME_None };

    FString Error;
    TestFalse (TEXT ("Invalid snapshot rejects restore"),
        FGridSpellbookPersistence::RestorePartySpellbooks (
            Party, { InvalidSaved }, Runtime, Error));
    TestTrue (TEXT ("Failed restore preserves prior runtime spell knowledge"),
        Runtime.KnowsSpell (CharacterId, FGridProductionSpellLibrary::HasteId ()));
    TestEqual (TEXT ("Failed restore does not partially replace runtime state"),
        Runtime.CharacterSpellbooks.Num (), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMagicMON188HotbarSpellRoundTripTest,
    "Grimrock.Magic.MON18.8.HotbarSpellRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188HotbarSpellRoundTripTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    const FGuid CharacterId = FGuid (18, 8, 9, 1);
    FGridPartyInventoryState Party = MakeMON188Party ({ CharacterId });
    Party.ActiveCharacters[0].CombatHotbarSlots[2] =
        MakeMON188SpellBinding (
            FGridProductionSpellLibrary::ArcaneBoltId (),
            2);

    FGridPartySpellbookState Runtime;
    Runtime.EnsureCharacter (CharacterId);
    Runtime.LearnSpell (
        CharacterId,
        FGridProductionSpellLibrary::ArcaneBoltId ());

    TArray<FGridCharacterSpellbookSaveState> Saved;
    FString Error;
    TestTrue (TEXT ("Known spell captures beside persistent hotbar"),
        FGridSpellbookPersistence::CapturePartySpellbooks (
            Party, Runtime, Saved, Error));

    FGridPartySpellbookState Restored;
    TestTrue (TEXT ("Known spell restores beside persistent hotbar"),
        FGridSpellbookPersistence::RestorePartySpellbooks (
            Party, Saved, Restored, Error));
    TestTrue (TEXT ("Spell remains known after restore"),
        Restored.KnowsSpell (
            CharacterId,
            FGridProductionSpellLibrary::ArcaneBoltId ()));
    const FGridCombatHotbarBinding& Binding =
        Party.ActiveCharacters[0].CombatHotbarSlots[2];
    TestTrue (TEXT ("Hotbar binding remains a Spell source"),
        Binding.SourcePolicy == EGridCombatActionSourcePolicy::Spell);
    TestEqual (TEXT ("Hotbar Spell identity remains stable"),
        Binding.SourceDefinitionId,
        FGridProductionSpellLibrary::ArcaneBoltId ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMagicMON188HotbarWithoutKnowledgePreservedTest,
    "Grimrock.Magic.MON18.8.HotbarWithoutKnowledgePreserved",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188HotbarWithoutKnowledgePreservedTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    const FGuid CharacterId = FGuid (18, 8, 10, 1);
    FGridPartyInventoryState Party = MakeMON188Party ({ CharacterId });
    Party.ActiveCharacters[0].CombatHotbarSlots[0] =
        MakeMON188SpellBinding (
            FGridProductionSpellLibrary::ArcaneBoltId (),
            0);

    FGridPartySpellbookState Restored;
    FString Error;
    TestTrue (TEXT ("Empty legacy spellbook restores"),
        FGridSpellbookPersistence::RestorePartySpellbooks (
            Party, {}, Restored, Error));
    TestFalse (TEXT ("Hotbar does not teach the referenced spell"),
        Restored.KnowsSpell (
            CharacterId,
            FGridProductionSpellLibrary::ArcaneBoltId ()));
    const FGridCombatHotbarBinding& Binding =
        Party.ActiveCharacters[0].CombatHotbarSlots[0];
    TestEqual (TEXT ("Legacy Spell hotbar binding is preserved"),
        Binding.ActionId,
        FGridProductionSpellLibrary::ArcaneBoltId ());
    TestTrue (TEXT ("Legacy binding remains structurally valid"),
        Binding.IsValid ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMagicMON188SpellBindingIsNotItemDefinitionTest,
    "Grimrock.Magic.MON18.8.SpellBindingIsNotItemDefinition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188SpellBindingIsNotItemDefinitionTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    const FGuid CharacterId = FGuid (18, 8, 11, 1);
    UGridPartyInventoryComponent* Inventory =
        NewObject<UGridPartyInventoryComponent> ();
    Inventory->PartyInventoryState = MakeMON188Party ({ CharacterId });
    Inventory->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[4] =
        MakeMON188SpellBinding (
            FGridProductionSpellLibrary::ArcaneBoltId (),
            4);

    int32 ResolverCallCount = 0;
    FName MissingDefinitionId = NAME_None;
    TestTrue (TEXT ("SAVEFIX.1 rehydration accepts Spell binding without item lookup"),
        Inventory->RehydrateOwnedItemDefinitions (
            [&ResolverCallCount] (FName) -> UGridItemDefinitionAsset*
            {
                ++ResolverCallCount;
                return nullptr;
            },
            MissingDefinitionId));
    TestEqual (TEXT ("Spell SourceDefinitionId is never sent to item resolver"),
        ResolverCallCount, 0);
    TestTrue (TEXT ("No missing ItemDefinition is reported for a Spell"),
        MissingDefinitionId.IsNone ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMagicMON188DiskRoundTripTest,
    "Grimrock.Magic.MON18.8.DiskRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON188DiskRoundTripTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FMON188DiskSlot DiskSlot;
    const FGuid CharacterId = FGuid (18, 8, 12, 1);

    UGrimrockPartySaveGame* Source = NewObject<UGrimrockPartySaveGame> ();
    Source->PartyInventoryState = MakeMON188Party ({ CharacterId });
    Source->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[1] =
        MakeMON188SpellBinding (
            FGridProductionSpellLibrary::LesserHealId (),
            1);

    FGridCharacterSpellbookSaveState Spellbook;
    Spellbook.CharacterId = CharacterId;
    Spellbook.KnownSpellIds = {
        FGridProductionSpellLibrary::ArcaneBoltId (),
        FGridProductionSpellLibrary::LesserHealId (),
        FGridProductionSpellLibrary::HasteId ()
    };
    Source->CharacterSpellbookStates.Add (Spellbook);

    TestTrue (TEXT ("Version-six Spellbook save writes to a real temporary slot"),
        UGameplayStatics::SaveGameToSlot (
            Source,
            DiskSlot.SlotName,
            DiskSlot.UserIndex));

    UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame> (
        UGameplayStatics::LoadGameFromSlot (
            DiskSlot.SlotName,
            DiskSlot.UserIndex));
    TestNotNull (TEXT ("Temporary disk save reloads"), Loaded);
    if (!Loaded)
    {
        return false;
    }

    TestEqual (TEXT ("Disk round trip uses SaveVersion six"),
        Loaded->SaveVersion, 6);
    TestTrue (TEXT ("Loaded save is compatible"), Loaded->IsCompatible ());
    TestEqual (TEXT ("One spellbook snapshot survives disk round trip"),
        Loaded->CharacterSpellbookStates.Num (), 1);
    if (Loaded->CharacterSpellbookStates.Num () == 1)
    {
        TestTrue (TEXT ("Arcane Bolt survives disk round trip"),
            Loaded->CharacterSpellbookStates[0].KnownSpellIds.Contains (
                FGridProductionSpellLibrary::ArcaneBoltId ()));
        TestTrue (TEXT ("Lesser Heal survives disk round trip"),
            Loaded->CharacterSpellbookStates[0].KnownSpellIds.Contains (
                FGridProductionSpellLibrary::LesserHealId ()));
        TestTrue (TEXT ("Haste survives disk round trip"),
            Loaded->CharacterSpellbookStates[0].KnownSpellIds.Contains (
                FGridProductionSpellLibrary::HasteId ()));
    }

    const FGridCombatHotbarBinding& LoadedBinding =
        Loaded->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[1];
    TestTrue (TEXT ("Spell hotbar source survives disk round trip"),
        LoadedBinding.SourcePolicy == EGridCombatActionSourcePolicy::Spell);
    TestEqual (TEXT ("Spell hotbar identity survives disk round trip"),
        LoadedBinding.SourceDefinitionId,
        FGridProductionSpellLibrary::LesserHealId ());
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

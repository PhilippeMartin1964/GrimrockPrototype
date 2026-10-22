#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "UI/RPGCharacterCreationWizardWidget.h"
#include "UI/UILocalizationText.h"

namespace ARMEMENT0193Localization
{
    bool LoadSourceFile(const TCHAR* RelativePath, FString& Out)
    {
        return FFileHelper::LoadFileToString(
            Out, *FPaths::Combine(FPaths::ProjectDir(), RelativePath));
    }

    URPGClassAsset* LoadWarrior()
    {
        return LoadObject<URPGClassAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Warrior.DA_Class_Warrior"));
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0193KitCsvCoverageTest,
    "Grimrock.RPG.ARMEMENT01.9.Localization.AuthoringKeysAndNoLiterals",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0193KitCsvCoverageTest::RunTest(const FString&)
{
    FString Csv;
    if (!TestTrue(TEXT("Canonical UI.csv exists"),
        ARMEMENT0193Localization::LoadSourceFile(TEXT("Localization/Source/UI.csv"), Csv))) return false;

    const TCHAR* CoreKeys[] = {
        TEXT("CharacterCreation.StartingKit.Title"),
        TEXT("CharacterCreation.StartingKit.UnknownName"),
        TEXT("CharacterCreation.StartingKit.UnknownItem"),
        TEXT("CharacterCreation.StartingKit.ItemRow"),
        TEXT("CharacterCreation.StartingKit.ItemRowWithLevel"),
        TEXT("CharacterCreation.StartingKit.TechnicalReference"),
        TEXT("CharacterCreation.StartingKit.DetailsFormat"),
        TEXT("CharacterCreation.StartingKit.SummaryFormat"),
        TEXT("CharacterCreation.StartingKit.InvalidSelection"),
        TEXT("CharacterCreation.StartingKit.Unavailable"),
        TEXT("CharacterCreation.StartingKit.InvalidKit"),
        TEXT("CharacterCreation.StartingKit.ConflictingDefinition")
    };
    for (const TCHAR* Key : CoreKeys)
    {
        TestTrue(*FString::Printf(TEXT("UI.csv defines %s"), Key),
            Csv.Contains(FString::Printf(TEXT("\"%s\","), Key)));
    }

    const TCHAR* KitKeys[] = {
        TEXT("WarriorDefender"), TEXT("WarriorSkirmisher"),
        TEXT("RogueShadow"), TEXT("RogueExplorer"),
        TEXT("RangerArcher"), TEXT("RangerPathfinder"),
        TEXT("MageScholar"), TEXT("MageWanderer"),
        TEXT("PriestPilgrim"), TEXT("PriestProtector"),
        TEXT("AlchemistApothecary"), TEXT("AlchemistFirestarter")
    };
    for (const TCHAR* Suffix : KitKeys)
    {
        TestTrue(*FString::Printf(TEXT("CSV names kit %s"), Suffix),
            Csv.Contains(FString::Printf(TEXT("\"CharacterCreation.StartingKit.Name.%s\","), Suffix)));
    }

    const TCHAR* SlotKeys[] = {
        TEXT("Bag"), TEXT("MainHand"), TEXT("OffHand"), TEXT("Head"), TEXT("Chest"),
        TEXT("Legs"), TEXT("Feet"), TEXT("Amulet"), TEXT("Ring1"), TEXT("Ring2"),
        TEXT("Shoulders"), TEXT("Gloves"), TEXT("Belt"), TEXT("Cloak"),
        TEXT("Talisman"), TEXT("QuickSlot1"), TEXT("QuickSlot2"),
        TEXT("Shirt"), TEXT("Bracers"), TEXT("Unknown")
    };
    for (const TCHAR* Suffix : SlotKeys)
    {
        TestTrue(*FString::Printf(TEXT("CSV localizes slot %s"), Suffix),
            Csv.Contains(FString::Printf(TEXT("\"CharacterCreation.StartingKit.Slot.%s\","), Suffix)));
    }

    FString WizardCpp, PresetCpp, InventoryCpp;
    if (!TestTrue(TEXT("Wizard C++ source exists"), ARMEMENT0193Localization::LoadSourceFile(
        TEXT("Source/GrimrockPrototype/Private/UI/RPGCharacterCreationWizardWidget.cpp"), WizardCpp)) ||
        !TestTrue(TEXT("Preset C++ source exists"), ARMEMENT0193Localization::LoadSourceFile(
        TEXT("Source/GrimrockPrototype/Private/RPG/RPGStartingEquipmentPresetCatalog.cpp"), PresetCpp)) ||
        !TestTrue(TEXT("Inventory C++ source exists"), ARMEMENT0193Localization::LoadSourceFile(
        TEXT("Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponent.cpp"), InventoryCpp)))
    {
        return false;
    }

    TestFalse(TEXT("Player-facing slot label is not C++ hardcoded"), WizardCpp.Contains(TEXT("TEXT(\"Sac\")")));
    TestFalse(TEXT("English Main Hand is not formatted for player"), WizardCpp.Contains(TEXT("GetDisplayNameTextByValue")));
    TestFalse(TEXT("Player-facing technical reference is not C++ hardcoded"),
        WizardCpp.Contains(TEXT("TEXT(\"Référence technique")));
    TestFalse(TEXT("A kit title cannot be built from unlocalized French literals"),
        PresetCpp.Contains(TEXT("TEXT(\"Escarmoucheur\")")));
    TestFalse(TEXT("Service error must not be formatted into UI from C++ French"),
        InventoryCpp.Contains(TEXT("TEXT(\"Équipement de départ invalide :")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0193KitProjectionTest,
    "Grimrock.RPG.ARMEMENT01.9.Localization.RuntimeStringTableProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0193KitProjectionTest::RunTest(const FString&)
{
    // This test intentionally requires ST_UI.uasset to have been materialized
    // with UIStringTableAuthoring from the source CSV.
    TestTrue(TEXT("Warrior Defender from String Table"),
        FUILocalizationText::Get(TEXT("CharacterCreation.StartingKit.Name.WarriorDefender"))
            .EqualTo(FText::FromString(TEXT("Défenseur"))));
    TestTrue(TEXT("Warrior Skirmisher from String Table"),
        FUILocalizationText::Get(TEXT("CharacterCreation.StartingKit.Name.WarriorSkirmisher"))
            .EqualTo(FText::FromString(TEXT("Escarmoucheur"))));
    TestTrue(TEXT("Bag vocabulary from String Table"),
        FUILocalizationText::Get(TEXT("CharacterCreation.StartingKit.Slot.Bag"))
            .EqualTo(FText::FromString(TEXT("Sac"))));
    TestTrue(TEXT("Main-hand vocabulary from String Table"),
        FUILocalizationText::Get(TEXT("CharacterCreation.StartingKit.Slot.MainHand"))
            .EqualTo(FText::FromString(TEXT("Main principale"))));

    URPGClassAsset* Class = ARMEMENT0193Localization::LoadWarrior();
    if (!TestNotNull(TEXT("Production Warrior definition"), Class)) return false;
    URPGCharacterCreationWizardWidget* Wizard = NewObject<URPGCharacterCreationWizardWidget>();
    Wizard->ClassDefinition = Class;
    if (!TestTrue(TEXT("Select alternative kit"), Wizard->SelectStartingEquipmentKit(TEXT("Warrior_Skirmisher"))))
    {
        return false;
    }
    const FString Name = Wizard->GetAvailableStartingEquipmentKits()[1].DisplayName.ToString();
    const FString Details = Wizard->GetSelectedStartingEquipmentKitDescription().ToString();
    TestEqual(TEXT("Player-facing name from string table"), Name, FString(TEXT("Escarmoucheur")));
    TestTrue(TEXT("Slot is in French"), Details.Contains(TEXT("Main principale")));
    TestFalse(TEXT("No untranslated Main Hand enum appears"), Details.Contains(TEXT("Main Hand")));
    TestTrue(TEXT("Technical reference is translated"), Details.Contains(TEXT("Référence technique : Warrior_Skirmisher")));
    TestFalse(TEXT("Details do not repeat kit title"), Details.StartsWith(Name));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

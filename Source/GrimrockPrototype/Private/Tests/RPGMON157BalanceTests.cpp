#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

namespace MON157Balance
{
    constexpr int32 RatExperienceReward = 500;
    constexpr int32 ReferencePartySize = 4;
    constexpr int32 ReferencePartyShare = RatExperienceReward / ReferencePartySize;

    int32 GetEquivalentRatKillsForLevelSolo (int32 Level)
    {
        const int32 RequiredExperience =
            URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel (Level);
        return FMath::DivideAndRoundUp (RequiredExperience, RatExperienceReward);
    }

    int32 GetEquivalentRatKillsForLevelParty (int32 Level)
    {
        const int32 RequiredExperience =
            URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel (Level);
        return FMath::DivideAndRoundUp (RequiredExperience, ReferencePartyShare);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON157FrozenCurveTest,
    "Grimrock.RPG.MON15.7.FrozenCurve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON157FrozenCurveTest::RunTest (const FString& Parameters)
{
    (void)Parameters;

    TestEqual (TEXT ("Level 1 starts at 0 XP"),
        URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel (1), 0);
    TestEqual (TEXT ("Level 2 starts at 1000 XP"),
        URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel (2), 1000);
    TestEqual (TEXT ("Level 3 starts at 3000 XP"),
        URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel (3), 3000);
    TestEqual (TEXT ("Level 4 starts at 6000 XP"),
        URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel (4), 6000);
    TestEqual (TEXT ("Level 5 starts at 10000 XP"),
        URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel (5), 10000);
    TestEqual (TEXT ("Maximum level remains 20"),
        URPGCharacterRulesLibrary::GetMaximumLevel (), 20);
    TestEqual (TEXT ("Level 20 starts at 190000 XP"),
        URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel (20), 190000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON157SoloRatPacingTest,
    "Grimrock.RPG.MON15.7.SoloRatPacing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON157SoloRatPacingTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace MON157Balance;

    TestEqual (TEXT ("One production rat does not level a solo level-1 character"),
        URPGCharacterRulesLibrary::GetLevelForExperience (RatExperienceReward), 1);
    TestEqual (TEXT ("Two production rats reach level 2 in solo play"),
        GetEquivalentRatKillsForLevelSolo (2), 2);
    TestEqual (TEXT ("Six cumulative production rats reach level 3 in solo play"),
        GetEquivalentRatKillsForLevelSolo (3), 6);
    TestEqual (TEXT ("Twelve cumulative production rats reach level 4 in solo play"),
        GetEquivalentRatKillsForLevelSolo (4), 12);
    TestEqual (TEXT ("Twenty cumulative production rats reach level 5 in solo play"),
        GetEquivalentRatKillsForLevelSolo (5), 20);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON157PartyRatPacingTest,
    "Grimrock.RPG.MON15.7.PartyRatPacing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON157PartyRatPacingTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace MON157Balance;

    TestEqual (TEXT ("Rat reward divides exactly across the reference four-character party"),
        RatExperienceReward % ReferencePartySize, 0);
    TestEqual (TEXT ("Each of four eligible characters receives 125 XP per rat"),
        ReferencePartyShare, 125);
    TestEqual (TEXT ("Eight cumulative rats reach level 2 for a four-character party"),
        GetEquivalentRatKillsForLevelParty (2), 8);
    TestEqual (TEXT ("Twenty-four cumulative rats reach level 3 for a four-character party"),
        GetEquivalentRatKillsForLevelParty (3), 24);
    TestEqual (TEXT ("Forty-eight cumulative rats reach level 4 for a four-character party"),
        GetEquivalentRatKillsForLevelParty (4), 48);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON157ProductionRatAssetTest,
    "Grimrock.RPG.MON15.7.ProductionRatAsset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON157ProductionRatAssetTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    using namespace MON157Balance;

    UGridMonsterDefinitionAsset* RatDefinition = LoadObject<UGridMonsterDefinitionAsset> (
        nullptr,
        TEXT ("/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.DA_MON_RatGiant"));

    TestNotNull (TEXT ("Production Rat Giant definition loads"), RatDefinition);
    if (!RatDefinition)
    {
        return false;
    }

    TestEqual (TEXT ("Production Rat Giant keeps its stable MonsterId"),
        RatDefinition->MonsterId, FName (TEXT ("MON_RatGiant")));
    TestEqual (TEXT ("Production Rat Giant uses the MON15.7 reward pool"),
        RatDefinition->ExperienceReward, RatExperienceReward);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

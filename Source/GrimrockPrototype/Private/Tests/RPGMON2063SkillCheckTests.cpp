#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillCheckService.h"
#include "Runtime/GridInventoryTypes.h"

namespace
{
    URPGSkillAsset* MakeMON2063Skill (
        UObject* Outer,
        bool bAllowUntrained = true,
        ERPGSkillGoverningAttribute Attribute =
            ERPGSkillGoverningAttribute::Dexterity)
    {
        URPGSkillAsset* Skill = NewObject<URPGSkillAsset> (Outer);
        Skill->SkillId = TEXT ("Skill_Lockpicking");
        Skill->DisplayName = FText::FromString (TEXT ("Crochetage"));
        Skill->Description =
            FText::FromString (TEXT ("Ouvrir les serrures mécaniques."));
        Skill->GoverningAttribute = Attribute;
        Skill->MaxRank = 5;
        Skill->bAllowUntrainedChecks = bAllowUntrained;
        return Skill;
    }

    FGridCharacterInventoryState MakeMON2063Character (int32 Dexterity = 14)
    {
        FGridCharacterInventoryState Character;
        Character.CharacterId = FGuid::NewGuid ();
        Character.Attributes = FRPGAttributes (10, Dexterity, 10, 10, 10, 10);
        Character.bRPGAttributesInitialized = true;
        return Character;
    }

    void AddMON2063Rank (
        FGridCharacterInventoryState& Character,
        FName SkillId,
        int32 Rank)
    {
        FRPGSkillRank Entry;
        Entry.SkillId = SkillId;
        Entry.Rank = Rank;
        Character.SkillRanks.Add (Entry);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2063DeterministicSameSeedTest,
    "Grimrock.MON20.6.Skills.SkillCheckDeterministicSameSeed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2063DeterministicSameSeedTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    URPGSkillAsset* Skill = MakeMON2063Skill (GetTransientPackage ());
    FGridCharacterInventoryState Character = MakeMON2063Character ();
    AddMON2063Rank (Character, Skill->SkillId, 3);

    FRandomStream StreamA (2063);
    FRandomStream StreamB (2063);
    FRPGSkillCheckResult A;
    FRPGSkillCheckResult B;

    TestTrue (TEXT ("First check resolves"),
        FRPGSkillCheckService::TryResolveSkillCheck (
            Character, Skill, 15, StreamA, A));
    TestTrue (TEXT ("Second check resolves"),
        FRPGSkillCheckService::TryResolveSkillCheck (
            Character, Skill, 15, StreamB, B));
    TestEqual (TEXT ("Same seed gives same roll"), A.Roll, B.Roll);
    TestEqual (TEXT ("Same seed gives same total"), A.Total, B.Total);
    TestEqual (TEXT ("Same seed gives same outcome"), A.bSuccess, B.bSuccess);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2063FormulaTest,
    "Grimrock.MON20.6.Skills.SkillCheckFormula",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2063FormulaTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    URPGSkillAsset* Skill = MakeMON2063Skill (GetTransientPackage ());
    FGridCharacterInventoryState Character = MakeMON2063Character (14);
    AddMON2063Rank (Character, Skill->SkillId, 3);

    FRandomStream Stream (42);
    FRPGSkillCheckResult Result;
    TestTrue (TEXT ("Check resolves"),
        FRPGSkillCheckService::TryResolveSkillCheck (
            Character, Skill, 1, Stream, Result));
    TestEqual (TEXT ("Rank is projected"), Result.Rank, 3);
    TestEqual (TEXT ("Dexterity value is projected"), Result.AttributeValue, 14);
    TestEqual (TEXT ("Dexterity modifier is +2"), Result.AttributeModifier, 2);
    TestEqual (TEXT ("Total is d20 + rank + attribute modifier"),
        Result.Total,
        Result.Roll + 5);
    TestTrue (TEXT ("Resolved flag is set"), Result.bResolved);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2063DifficultyThresholdTest,
    "Grimrock.MON20.6.Skills.SkillCheckDifficultyThreshold",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2063DifficultyThresholdTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    URPGSkillAsset* Skill = MakeMON2063Skill (GetTransientPackage ());
    FGridCharacterInventoryState Character = MakeMON2063Character (14);
    AddMON2063Rank (Character, Skill->SkillId, 2);

    FRandomStream ProbeStream (77);
    FRPGSkillCheckResult Probe;
    TestTrue (TEXT ("Probe resolves"),
        FRPGSkillCheckService::TryResolveSkillCheck (
            Character, Skill, 1, ProbeStream, Probe));

    FRandomStream SuccessStream (77);
    FRandomStream FailureStream (77);
    FRPGSkillCheckResult Success;
    FRPGSkillCheckResult Failure;
    TestTrue (TEXT ("Equal difficulty resolves"),
        FRPGSkillCheckService::TryResolveSkillCheck (
            Character, Skill, Probe.Total, SuccessStream, Success));
    TestTrue (TEXT ("Higher difficulty resolves"),
        FRPGSkillCheckService::TryResolveSkillCheck (
            Character, Skill, Probe.Total + 1, FailureStream, Failure));
    TestTrue (TEXT ("Total equal to difficulty succeeds"), Success.bSuccess);
    TestFalse (TEXT ("Total below difficulty fails"), Failure.bSuccess);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2063UntrainedAllowedTest,
    "Grimrock.MON20.6.Skills.SkillCheckUntrainedAllowed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2063UntrainedAllowedTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    URPGSkillAsset* Skill = MakeMON2063Skill (GetTransientPackage (), true);
    FGridCharacterInventoryState Character = MakeMON2063Character (12);

    FRandomStream Stream (11);
    FRPGSkillCheckResult Result;
    TestTrue (TEXT ("Untrained check is allowed"),
        FRPGSkillCheckService::TryResolveSkillCheck (
            Character, Skill, 10, Stream, Result));
    TestEqual (TEXT ("Untrained rank is zero"), Result.Rank, 0);
    TestTrue (TEXT ("Allowed untrained check is resolved"), Result.bResolved);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2063UntrainedRejectedNoRandomTest,
    "Grimrock.MON20.6.Skills.SkillCheckUntrainedRejectedNoRandom",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2063UntrainedRejectedNoRandomTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    URPGSkillAsset* Skill = MakeMON2063Skill (GetTransientPackage (), false);
    FGridCharacterInventoryState Character = MakeMON2063Character ();

    FRandomStream RejectedStream (101);
    FRandomStream ReferenceStream (101);
    FRPGSkillCheckResult Result;
    TestFalse (TEXT ("Untrained forbidden check is rejected"),
        FRPGSkillCheckService::TryResolveSkillCheck (
            Character, Skill, 10, RejectedStream, Result));
    TestTrue (TEXT ("Reject reason is UntrainedNotAllowed"),
        Result.RejectReason == ERPGSkillCheckRejectReason::UntrainedNotAllowed);
    TestFalse (TEXT ("Rejected check is not resolved"), Result.bResolved);
    TestEqual (TEXT ("Rejected check does not consume RNG"),
        RejectedStream.RandRange (1, 20),
        ReferenceStream.RandRange (1, 20));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2063NoneAttributeTest,
    "Grimrock.MON20.6.Skills.SkillCheckNoneAttribute",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2063NoneAttributeTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    URPGSkillAsset* Skill = MakeMON2063Skill (
        GetTransientPackage (),
        true,
        ERPGSkillGoverningAttribute::None);
    FGridCharacterInventoryState Character = MakeMON2063Character (18);
    AddMON2063Rank (Character, Skill->SkillId, 2);

    FRandomStream Stream (3);
    FRPGSkillCheckResult Result;
    TestTrue (TEXT ("None-attribute check resolves"),
        FRPGSkillCheckService::TryResolveSkillCheck (
            Character, Skill, 1, Stream, Result));
    TestEqual (TEXT ("None attribute exposes zero raw value"), Result.AttributeValue, 0);
    TestEqual (TEXT ("None attribute contributes no modifier"), Result.AttributeModifier, 0);
    TestEqual (TEXT ("Total uses roll plus rank only"), Result.Total, Result.Roll + 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2063InvalidDifficultyNoRandomTest,
    "Grimrock.MON20.6.Skills.SkillCheckInvalidDifficultyNoRandom",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2063InvalidDifficultyNoRandomTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    URPGSkillAsset* Skill = MakeMON2063Skill (GetTransientPackage ());
    FGridCharacterInventoryState Character = MakeMON2063Character ();
    AddMON2063Rank (Character, Skill->SkillId, 1);

    FRandomStream RejectedStream (9);
    FRandomStream ReferenceStream (9);
    FRPGSkillCheckResult Result;
    TestFalse (TEXT ("Zero difficulty is rejected"),
        FRPGSkillCheckService::TryResolveSkillCheck (
            Character, Skill, 0, RejectedStream, Result));
    TestTrue (TEXT ("Reject reason is InvalidDifficulty"),
        Result.RejectReason == ERPGSkillCheckRejectReason::InvalidDifficulty);
    TestEqual (TEXT ("Invalid difficulty does not consume RNG"),
        RejectedStream.RandRange (1, 20),
        ReferenceStream.RandRange (1, 20));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FRPGMON2063InvalidStateNoRandomTest,
    "Grimrock.MON20.6.Skills.SkillCheckInvalidStateNoRandom",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON2063InvalidStateNoRandomTest::RunTest (const FString& Parameters)
{
    (void)Parameters;
    URPGSkillAsset* Skill = MakeMON2063Skill (GetTransientPackage ());
    FGridCharacterInventoryState Character = MakeMON2063Character ();
    AddMON2063Rank (Character, Skill->SkillId, 1);
    AddMON2063Rank (Character, Skill->SkillId, 2);

    FRandomStream RejectedStream (55);
    FRandomStream ReferenceStream (55);
    FRPGSkillCheckResult Result;
    TestFalse (TEXT ("Malformed skill state is rejected"),
        FRPGSkillCheckService::TryResolveSkillCheck (
            Character, Skill, 10, RejectedStream, Result));
    TestTrue (TEXT ("Reject reason is InvalidCharacterState"),
        Result.RejectReason == ERPGSkillCheckRejectReason::InvalidCharacterState);
    TestEqual (TEXT ("Invalid state does not consume RNG"),
        RejectedStream.RandRange (1, 20),
        ReferenceStream.RandRange (1, 20));
    return true;
}

#endif

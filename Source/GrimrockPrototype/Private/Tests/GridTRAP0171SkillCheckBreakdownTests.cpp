#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillCheckService.h"
#include "Runtime/GridInventoryTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTRAP0171ClassGroupBreakdownTest,
    "Grimrock.TRAP01.7.1.ClassGroupComponents",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTRAP0171ClassGroupBreakdownTest::RunTest(const FString&)
{
    URPGSkillAsset* Skill = NewObject<URPGSkillAsset>(GetTransientPackage());
    Skill->SkillId = TEXT("Skill_Traps");
    Skill->DisplayName = FText::FromString(TEXT("Pièges"));
    Skill->GoverningAttribute = ERPGSkillGoverningAttribute::Dexterity;
    Skill->MaxRank = 5;
    Skill->bAllowUntrainedChecks = false;
    if (!TestTrue(TEXT("Skill contract valid"), Skill->IsValidDefinition())) return false;

    URPGClassAsset* Class = NewObject<URPGClassAsset>(GetTransientPackage());
    Class->ClassId = TEXT("Rogue_BreakdownTest");
    Class->DisplayName = FText::FromString(TEXT("Saboteur"));
    Class->HealthAtLevelOne = 10;

    FRPGClassProgressionChoiceDefinition Expert;
    Expert.ChoiceId = TEXT("Talent_ExpertTraps");
    Expert.DisplayName = FText::FromString(TEXT("Désamorçage expert"));
    Expert.MinimumLevel = 2;
    Expert.PointCost = 1;
    FRPGSkillProgressionModifier Personal;
    Personal.SkillId = Skill->SkillId;
    Personal.CheckModifier = 2;
    Expert.SkillModifiers.Add(Personal);
    Class->ProgressionChoices.Add(Expert);

    FRPGClassProgressionChoiceDefinition Guide;
    Guide.ChoiceId = TEXT("Talent_TestGroupGuide");
    Guide.DisplayName = FText::FromString(TEXT("Guide"));
    Guide.MinimumLevel = 2;
    Guide.PointCost = 1;
    FRPGPartyProgressionModifier Group;
    Group.StackingGroupId = TEXT("Party.TrapBreakdown.GroupGuide");
    Group.GroupSkillIds = { Skill->SkillId };
    Group.GroupSkillCheckModifier = 2;
    Guide.PartyModifiers.Add(Group);
    Class->ProgressionChoices.Add(Guide);
    if (!TestTrue(TEXT("Class and chosen talents structurally valid"), Class->IsValidDefinition())) return false;

    auto MakeMember = [Class](FName ChoiceId, int32 Dexterity, int32 Rank)
    {
        FGridCharacterInventoryState Member;
        Member.CharacterId = FGuid::NewGuid();
        Member.ClassId = Class->ClassId;
        Member.ClassDefinition = Class;
        Member.Level = 2;
        Member.Resources.CurrentHealth = 10;
        Member.Attributes.Dexterity = Dexterity;
        Member.SelectedClassProgressionChoiceIds.Add(ChoiceId);
        if (Rank > 0)
        {
            FRPGSkillRank SkillRank;
            SkillRank.SkillId = TEXT("Skill_Traps");
            SkillRank.Rank = Rank;
            Member.SkillRanks.Add(SkillRank);
        }
        return Member;
    };

    FGridPartyInventoryState Party;
    Party.ActiveCharacters.Add(MakeMember(Expert.ChoiceId, 14, 3));
    Party.ActiveCharacters.Add(MakeMember(Guide.ChoiceId, 10, 0));
    Party.ActiveCharacters.Add(MakeMember(Guide.ChoiceId, 10, 0));

    FRandomStream Random(1711);
    FRPGSkillCheckResult Result;
    int32 ActorIndex = INDEX_NONE;
    if (!TestTrue(TEXT("Party skill check resolves"),
        FRPGSkillCheckService::TryResolveBestPartySkillCheck(
            Party, Skill, 14, Random, Result, ActorIndex))) return false;
    TestEqual(TEXT("Trained actor selected"), ActorIndex, 0);
    TestEqual(TEXT("Character class grants +2"), Result.ClassModifier, 2);
    TestEqual(TEXT("Two same-group talents confer only +2"), Result.GroupModifier, 2);
    TestEqual(TEXT("Legacy progression equals combined modifiers"), Result.ProgressionModifier, 4);
    TestEqual(TEXT("No double counting in total"),
        Result.Total, Result.Roll + Result.Rank + Result.AttributeModifier +
            Result.ClassModifier + Result.GroupModifier);

    // A now-dead helper contributes nothing, and a fresh check uses the same roll.
    Party.ActiveCharacters[1].Resources.CurrentHealth = 0;
    Party.ActiveCharacters[2].Resources.CurrentHealth = 0;
    FRandomStream SameRandom(1711);
    FRPGSkillCheckResult WithoutGuides;
    ActorIndex = INDEX_NONE;
    if (!TestTrue(TEXT("Party check without surviving group guides resolves"),
        FRPGSkillCheckService::TryResolveBestPartySkillCheck(
            Party, Skill, 14, SameRandom, WithoutGuides, ActorIndex))) return false;
    TestEqual(TEXT("Same deterministic roll"), WithoutGuides.Roll, Result.Roll);
    TestEqual(TEXT("Same personal class bonus"), WithoutGuides.ClassModifier, 2);
    TestEqual(TEXT("No living helpers means no group bonus"), WithoutGuides.GroupModifier, 0);
    TestEqual(TEXT("Only the group bonus disappears"), WithoutGuides.Total, Result.Total - 2);
    return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterBalanceTypes.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

namespace MON177Balance
{
    constexpr int32 GoblinExperienceReward = 125;
    constexpr int32 RatExperienceReward = 500;
    constexpr int32 ReferencePartySize = 4;

    UGridMonsterDefinitionAsset* LoadGoblinDefinition (
        FAutomationTestBase& Test)
    {
        UGridMonsterDefinitionAsset* Definition =
            LoadObject<UGridMonsterDefinitionAsset> (
                nullptr,
                TEXT ("/Game/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower.DA_MON_GoblinThrower"));
        Test.TestNotNull (
            TEXT ("Production Goblin Thrower definition loads"),
            Definition);
        return Definition;
    }

    UGridItemDefinitionAsset* LoadItemDefinition (
        FAutomationTestBase& Test,
        const TCHAR* AssetPath,
        const TCHAR* Label)
    {
        UGridItemDefinitionAsset* Definition =
            LoadObject<UGridItemDefinitionAsset> (nullptr, AssetPath);
        Test.TestNotNull (Label, Definition);
        return Definition;
    }

    const FGridMonsterLootEntry* FindLootEntry (
        const UGridMonsterDefinitionAsset* Definition,
        FName ItemDefinitionId)
    {
        return Definition
            ? Definition->LootTable.FindByPredicate (
                [ItemDefinitionId] (const FGridMonsterLootEntry& Entry)
                {
                    return Entry.GetResolvedItemDefinitionId () ==
                        ItemDefinitionId;
                })
            : nullptr;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON177ProductionBalanceBaselineTest,
    "Grimrock.Monsters.MON17.7.ProductionBalanceBaseline",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON177ProductionBalanceBaselineTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    using namespace MON177Balance;

    UGridMonsterDefinitionAsset* Goblin = LoadGoblinDefinition (*this);
    if (!Goblin)
    {
        return false;
    }

    FString ValidationError;
    TestTrue (TEXT ("Production Goblin definition remains valid"),
        Goblin->ValidateDefinition (ValidationError));
    if (!ValidationError.IsEmpty ())
    {
        AddInfo (FString::Printf (
            TEXT ("Definition validation detail: %s"),
            *ValidationError));
    }

    TestEqual (TEXT ("Stable Goblin monster id"),
        Goblin->MonsterId, FName (TEXT ("MON_GoblinThrower")));
    TestEqual (TEXT ("Danger level remains three"),
        Goblin->DangerLevel, 3);
    TestEqual (TEXT ("Production health baseline remains ten"),
        Goblin->MaxHealth, 10);
    TestEqual (TEXT ("Production initiative baseline remains twelve"),
        Goblin->Initiative, 12);
    TestEqual (TEXT ("Production accuracy baseline remains two"),
        Goblin->Accuracy, 2);
    TestEqual (TEXT ("Production evasion baseline remains three"),
        Goblin->Evasion, 3);
    TestEqual (TEXT ("Production action points remain three"),
        Goblin->ActionPointsPerTurn, 3);
    TestEqual (TEXT ("Goblin keeps the RangedKeeper profile"),
        Goblin->PrimaryAIProfile,
        EGridMonsterAIProfile::RangedKeeper);
    TestEqual (TEXT ("Preferred minimum distance remains three"),
        Goblin->PreferredMinDistance, 3);
    TestEqual (TEXT ("Preferred maximum distance remains five"),
        Goblin->PreferredMaxDistance, 5);
    TestTrue (TEXT ("Goblin keeps group aggro sharing enabled"),
        Goblin->bSharesAggroWithGroup);
    TestEqual (TEXT ("Goblin aggro propagation range remains five"),
        Goblin->AggroPropagationRange, 5);
    TestEqual (TEXT ("Production reward baseline remains 125 XP"),
        Goblin->ExperienceReward, GoblinExperienceReward);

    FGridMonsterAttackDefinition ThrowKnife;
    TestTrue (TEXT ("Production ThrowKnife attack resolves"),
        Goblin->GetAttackDefinition (
            FName (TEXT ("Attack_ThrowKnife")),
            ThrowKnife));
    TestEqual (TEXT ("ThrowKnife minimum damage remains two"),
        ThrowKnife.MinDamage + ThrowKnife.DamageBonus, 2);
    TestEqual (TEXT ("ThrowKnife maximum damage remains five"),
        ThrowKnife.MaxDamage + ThrowKnife.DamageBonus, 5);
    TestEqual (TEXT ("ThrowKnife minimum range remains two"),
        ThrowKnife.MinRangeCells, 2);
    TestEqual (TEXT ("ThrowKnife maximum range remains six"),
        ThrowKnife.RangeCells, 6);
    TestEqual (TEXT ("ThrowKnife action point cost remains two"),
        ThrowKnife.ActionPointCost, 2);

    FGridMonsterBalanceSnapshot Snapshot;
    TestTrue (TEXT ("Production balance snapshot builds"),
        FGridMonsterBalanceAnalyzer::BuildSnapshot (Goblin, Snapshot));
    AddInfo (FString::Printf (
        TEXT ("MON17.7 baseline: Danger=%d HP=%d Armor=%d/%d Initiative=%d Accuracy=%d Evasion=%d AP=%d Attacks=%d Damage=%d..%d Average=%.2f XP=%d"),
        Snapshot.DangerLevel,
        Snapshot.MaxHealth,
        Snapshot.PhysicalArmor,
        Snapshot.MagicalArmor,
        Snapshot.Initiative,
        Snapshot.Accuracy,
        Snapshot.Evasion,
        Snapshot.ActionPointsPerTurn,
        Snapshot.AttackCount,
        Snapshot.MinimumBaseDamage,
        Snapshot.MaximumBaseDamage,
        Snapshot.AverageBaseDamage,
        Snapshot.ExperienceReward));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON177ProductionLootBaselineTest,
    "Grimrock.Monsters.MON17.7.ProductionLootBaseline",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON177ProductionLootBaselineTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    using namespace MON177Balance;

    UGridMonsterDefinitionAsset* Goblin = LoadGoblinDefinition (*this);
    UGridItemDefinitionAsset* Knife = LoadItemDefinition (
        *this,
        TEXT ("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_GoblinKnife.DA_Item_GoblinKnife"),
        TEXT ("Production GoblinKnife definition loads"));
    UGridItemDefinitionAsset* Stone = LoadItemDefinition (
        *this,
        TEXT ("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Stone.DA_Item_Stone"),
        TEXT ("Production Stone definition loads"));
    UGridItemDefinitionAsset* Vial = LoadItemDefinition (
        *this,
        TEXT ("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_EmptyVial.DA_Item_EmptyVial"),
        TEXT ("Production EmptyVial definition loads"));
    if (!Goblin || !Knife || !Stone || !Vial)
    {
        return false;
    }

    TestTrue (TEXT ("GoblinKnife definition is valid"),
        Knife->IsValidDefinition ());
    TestTrue (TEXT ("Stone definition is valid"),
        Stone->IsValidDefinition ());
    TestTrue (TEXT ("EmptyVial definition is valid"),
        Vial->IsValidDefinition ());
    TestEqual (TEXT ("GoblinKnife stable item id"),
        Knife->ItemDefinitionId, FName (TEXT ("GoblinKnife")));
    TestEqual (TEXT ("EmptyVial stable item id"),
        Vial->ItemDefinitionId, FName (TEXT ("EmptyVial")));
    TestEqual (TEXT ("Goblin loot table has exactly three authored entries"),
        Goblin->LootTable.Num (), 3);

    const FGridMonsterLootEntry* KnifeEntry =
        FindLootEntry (Goblin, FName (TEXT ("GoblinKnife")));
    const FGridMonsterLootEntry* StoneEntry =
        FindLootEntry (Goblin, FName (TEXT ("Stone")));
    const FGridMonsterLootEntry* VialEntry =
        FindLootEntry (Goblin, FName (TEXT ("EmptyVial")));
    TestNotNull (TEXT ("GoblinKnife loot entry exists"), KnifeEntry);
    TestNotNull (TEXT ("Stone loot entry exists"), StoneEntry);
    TestNotNull (TEXT ("EmptyVial loot entry exists"), VialEntry);

    float ExpectedItemsPerKill = 0.0f;
    for (const FGridMonsterLootEntry& Entry : Goblin->LootTable)
    {
        TestTrue (TEXT ("Every production loot entry is valid"),
            Entry.IsValidDefinition ());
        ExpectedItemsPerKill += Entry.DropChance *
            (static_cast<float> (Entry.MinQuantity + Entry.MaxQuantity) *
                0.5f);
        AddInfo (FString::Printf (
            TEXT ("MON17.7 loot baseline: Item=%s Chance=%.3f Quantity=%d..%d"),
            *Entry.GetResolvedItemDefinitionId ().ToString (),
            Entry.DropChance,
            Entry.MinQuantity,
            Entry.MaxQuantity));
    }
    AddInfo (FString::Printf (
        TEXT ("MON17.7 loot baseline: ExpectedItemsPerKill=%.3f"),
        ExpectedItemsPerKill));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON177RewardPacingBaselineTest,
    "Grimrock.Monsters.MON17.7.RewardPacingBaseline",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON177RewardPacingBaselineTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    using namespace MON177Balance;

    const int32 LevelTwoExperience =
        URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel (2);
    const int32 BaseShare =
        GoblinExperienceReward / ReferencePartySize;
    const int32 Remainder =
        GoblinExperienceReward % ReferencePartySize;

    TestEqual (TEXT ("Four Goblin rewards equal one Rat Giant reward"),
        GoblinExperienceReward * 4, RatExperienceReward);
    TestEqual (TEXT ("Eight Goblins reach level two in solo play"),
        FMath::DivideAndRoundUp (
            LevelTwoExperience,
            GoblinExperienceReward),
        8);
    TestEqual (TEXT ("Four-character base share is 31 XP"),
        BaseShare, 31);
    TestEqual (TEXT ("Stable first-character remainder is one XP"),
        Remainder, 1);
    TestEqual (TEXT ("First character reaches level two after 32 Goblins"),
        FMath::DivideAndRoundUp (
            LevelTwoExperience,
            BaseShare + 1),
        32);
    TestEqual (TEXT ("Other characters reach level two after 33 Goblins"),
        FMath::DivideAndRoundUp (
            LevelTwoExperience,
            BaseShare),
        33);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

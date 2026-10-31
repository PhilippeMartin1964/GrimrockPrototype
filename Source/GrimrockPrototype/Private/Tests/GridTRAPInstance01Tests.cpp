#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/GridDungeonTrapInstanceUtils.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTRAPInstance01,
    "Grimrock.TRAP_INSTANCE01.PerPlacementConfiguration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTRAPInstance01::RunTest(const FString&)
{
    UGridWorldObjectDefinitionAsset* Definition =
        NewObject<UGridWorldObjectDefinitionAsset>(GetTransientPackage());
    if (!TestNotNull(TEXT("Trap definition exists"), Definition)) return false;
    Definition->DungeonTrap.bIsDungeonTrap = true;
    Definition->DungeonTrap.DetectionDifficulty = 10;
    Definition->DungeonTrap.MaxDetectionRangeCells = 4;
    Definition->DungeonTrap.MinimumDamage = 4;
    Definition->DungeonTrap.MaximumDamage = 8;
    Definition->DisarmDifficulty = 10;
    FGridWorldObjectInstance First;
    FGridWorldObjectInstance Second;
    First.InstanceId = FGuid::NewGuid();
    Second.InstanceId = FGuid::NewGuid();
    First.WorldObjectDefinitionId = TEXT("SharedTrap");
    Second.WorldObjectDefinitionId = TEXT("SharedTrap");
    FGridDungeonTrapInstanceOverrides& Local = First.InstanceConfig.DungeonTrap;
    Local.bOverrideDetectionDifficulty = true;
    Local.DetectionDifficulty = 18;
    Local.bOverrideMaxDetectionRangeCells = true;
    Local.MaxDetectionRangeCells = 2;
    Local.bOverrideDisarmDifficulty = true;
    Local.DisarmDifficulty = 25;
    Local.bOverrideDamageType = true;
    Local.DamageType = EGridDamageType::Fire;
    Local.bOverridePhysicalSubtype = true;
    Local.PhysicalSubtype = EGridPhysicalDamageSubtype::None;
    Local.bOverrideMinimumDamage = true;
    Local.MinimumDamage = 11;
    Local.bOverrideMaximumDamage = true;
    Local.MaximumDamage = 22;
    const FGridDungeonTrapDefinitionParams A = GridDungeonTrapInstance::Resolve(First, *Definition);
    const FGridDungeonTrapDefinitionParams B = GridDungeonTrapInstance::Resolve(Second, *Definition);
    TestTrue(TEXT("First trap contract remains valid"), A.IsValidContract());
    TestEqual(TEXT("Instance A detection"), A.DetectionDifficulty, 18);
    TestEqual(TEXT("Instance A range"), A.MaxDetectionRangeCells, 2);
    TestEqual(TEXT("Instance A damage type"), A.DamageType, EGridDamageType::Fire);
    TestEqual(TEXT("Instance A minimum"), A.MinimumDamage, 11);
    TestEqual(TEXT("Instance A maximum"), A.MaximumDamage, 22);
    TestEqual(TEXT("Instance A disarm DC"), GridDungeonTrapInstance::ResolveDisarmDifficulty(First, *Definition), 25);
    TestEqual(TEXT("Instance B still inherits detection"), B.DetectionDifficulty, 10);
    TestEqual(TEXT("Instance B still inherits type"), B.DamageType, EGridDamageType::Physical);
    TestEqual(TEXT("Instance B still inherits damage"), B.MaximumDamage, 8);
    TestEqual(TEXT("Instance B still inherits disarm"), GridDungeonTrapInstance::ResolveDisarmDifficulty(Second, *Definition), 10);
    TestEqual(TEXT("Shared definition unchanged"), Definition->DungeonTrap.DetectionDifficulty, 10);
    TestEqual(TEXT("Shared disarm definition unchanged"), Definition->DisarmDifficulty, 10);
    Local = FGridDungeonTrapInstanceOverrides();
    TestEqual(TEXT("Reset inherits defaults"), GridDungeonTrapInstance::Resolve(First, *Definition).DetectionDifficulty, 10);
    const FProperty* InstanceProp = FGridWorldObjectInstanceConfig::StaticStruct()->FindPropertyByName(TEXT("DungeonTrap"));
    TestNotNull(TEXT("Trap values are serialized on the level instance"), InstanceProp);
    return true;
}
#endif

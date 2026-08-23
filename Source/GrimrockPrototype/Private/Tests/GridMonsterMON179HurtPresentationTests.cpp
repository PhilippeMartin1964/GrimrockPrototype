#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Sound/SoundWave.h"
#include "UObject/UnrealType.h"

namespace GridMonsterMON179
{
    const FName TestLevelId (TEXT ("MON179Level"));

    struct FTestWorld
    {
        UWorld* World = nullptr;

        FTestWorld ()
        {
            const UWorld::InitializationValues InitializationValues =
                UWorld::InitializationValues ()
                    .AllowAudioPlayback (false)
                    .RequiresHitProxies (false)
                    .CreatePhysicsScene (false)
                    .CreateNavigation (false)
                    .CreateAISystem (false)
                    .ShouldSimulatePhysics (false)
                    .SetTransactional (false);
            World = UWorld::CreateWorld (
                EWorldType::Game,
                false,
                FName (*FString::Printf (
                    TEXT ("MON179TestWorld_%s"),
                    *FGuid::NewGuid ().ToString (
                        EGuidFormats::Digits))),
                nullptr,
                true,
                ERHIFeatureLevel::Num,
                &InitializationValues);
            if (World && GEngine)
            {
                FWorldContext& Context =
                    GEngine->CreateNewWorldContext (
                        EWorldType::Game);
                Context.SetCurrentWorld (World);
            }
        }

        ~FTestWorld ()
        {
            if (!World)
            {
                return;
            }
            World->DestroyWorld (false);
            if (GEngine)
            {
                GEngine->DestroyWorldContext (World);
            }
        }
    };

    UGridLevelAsset* ConfigureFloor (
        AGridLevelRuntimeActor* Runtime,
        int32 Width = 5,
        int32 Height = 5)
    {
        if (!Runtime)
        {
            return nullptr;
        }

        UGridLevelAsset* LevelAsset =
            NewObject<UGridLevelAsset> (Runtime);
        LevelAsset->Width = Width;
        LevelAsset->Height = Height;
        LevelAsset->StartCellX = Width - 1;
        LevelAsset->StartCellY = Height - 1;
        LevelAsset->StartFacing = EGridEdge::North;
        LevelAsset->EnsureCellCount ();
        for (FGridLevelCellData& Cell : LevelAsset->Cells)
        {
            Cell.CellType = EGridCellType::Floor;
            Cell.bBlocksOccupancy = false;
        }
        Runtime->LevelAsset = LevelAsset;
        Runtime->CurrentDungeonLevelId = TestLevelId;
        return LevelAsset;
    }

    FGridMonsterAudioEventDefinition MakeAudioEvent (
        UObject* Outer,
        FName SoundName)
    {
        FGridMonsterAudioEventDefinition Definition;
        Definition.Sounds.Add (
            NewObject<USoundWave> (Outer, SoundName));
        return Definition;
    }

    UGridMonsterDefinitionAsset* MakeDefinition (
        UObject* Outer,
        FName MonsterId = TEXT ("MON179_HurtMonster"))
    {
        UGridMonsterDefinitionAsset* Definition =
            NewObject<UGridMonsterDefinitionAsset> (Outer);
        Definition->MonsterId = MonsterId;
        Definition->DisplayName =
            FText::FromString (TEXT ("MON17.9 Hurt Monster"));
        Definition->CategoryId = TEXT ("Test");
        Definition->MaxHealth = 10;
        Definition->ActionPointsPerTurn = 1;
        Definition->DeathExpectedDuration = 1.0f;
        Definition->HurtAudio =
            MakeAudioEvent (Definition, TEXT ("MON179HurtSound"));
        Definition->DeathAudio =
            MakeAudioEvent (Definition, TEXT ("MON179DeathSound"));
        return Definition;
    }

    AGridMonsterActor* SpawnMonster (
        UWorld* World,
        AGridLevelRuntimeActor* Runtime,
        UGridMonsterDefinitionAsset* Definition,
        const FGuid& PersistenceId,
        FIntPoint Cell,
        FName ActorName)
    {
        if (!World || !Definition)
        {
            return nullptr;
        }

        FActorSpawnParameters Params;
        Params.Name = ActorName;
        Params.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AGridMonsterActor* Monster =
            World->SpawnActor<AGridMonsterActor> (
                AGridMonsterActor::StaticClass (),
                Runtime
                    ? Runtime->GetCellCenterWorld (Cell.X, Cell.Y)
                    : FVector::ZeroVector,
                FRotator::ZeroRotator,
                Params);
        if (!Monster)
        {
            return nullptr;
        }

        Monster->MonsterDefinition = Definition;
        Monster->PersistentMonsterId = PersistenceId;
        Monster->HomeDungeonLevelId = TestLevelId;
        Monster->CurrentCell = Cell;
        Monster->Facing = EGridEdge::North;
        Monster->MonsterState = EGridMonsterState::Idle;
        Monster->CurrentHealth = Definition->MaxHealth;
        Monster->CurrentPhysicalArmor = Definition->PhysicalArmor;
        Monster->CurrentMagicalArmor = Definition->MagicalArmor;
        Monster->bCombatStatsInitialized = true;
        Monster->bMonsterEnabled = true;

        if (Monster->AudioComponent)
        {
            Monster->AudioComponent->bNativePlaybackEnabled = false;
            Monster->AudioComponent->InitializeMonsterAudio ();
        }
        if (Monster->VFXComponent)
        {
            Monster->VFXComponent->InitializeMonsterVFX ();
        }
        if (Monster->DeathComponent)
        {
            Monster->DeathComponent->InitializeDeathComponent (Runtime);
        }
        return Monster;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON179HurtDefinitionContractTest,
    "Grimrock.Monsters.MON17.9.HurtDefinitionContract",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON179HurtDefinitionContractTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    UGridMonsterDefinitionAsset* Definition =
        GridMonsterMON179::MakeDefinition (
            GetTransientPackage ());
    TestNotNull (
        TEXT ("Transient monster definition can be created"),
        Definition);
    if (!Definition)
    {
        return false;
    }

    TestTrue (
        TEXT ("Hurt montage is optional by default"),
        Definition->HurtMontage.IsNull ());

    FString ValidationError;
    TestTrue (
        TEXT ("A monster definition remains valid without HurtMontage"),
        Definition->ValidateDefinition (ValidationError));
    TestTrue (
        TEXT ("Missing optional HurtMontage produces no validation error"),
        ValidationError.IsEmpty ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON179HurtPresentationApiContractTest,
    "Grimrock.Monsters.MON17.9.HurtPresentationApiContract",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON179HurtPresentationApiContractTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    UClass* DefinitionClass =
        UGridMonsterDefinitionAsset::StaticClass ();
    UClass* MonsterClass = AGridMonsterActor::StaticClass ();
    TestNotNull (TEXT ("Monster definition class exists"), DefinitionClass);
    TestNotNull (TEXT ("Monster actor class exists"), MonsterClass);
    if (!DefinitionClass || !MonsterClass)
    {
        return false;
    }

    TestNotNull (
        TEXT ("Monster definition exposes HurtMontage"),
        FindFProperty<FProperty> (
            DefinitionClass,
            GET_MEMBER_NAME_CHECKED (
                UGridMonsterDefinitionAsset,
                HurtMontage)));
    TestNotNull (
        TEXT ("Monster actor exposes StartHurtPresentation"),
        MonsterClass->FindFunctionByName (
            TEXT ("StartHurtPresentation")));
    TestNotNull (
        TEXT ("Monster actor exposes StopHurtPresentation"),
        MonsterClass->FindFunctionByName (
            TEXT ("StopHurtPresentation")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON179NonFatalDamageRequestsHurtTest,
    "Grimrock.Monsters.MON17.9.NonFatalDamageRequestsHurt",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON179NonFatalDamageRequestsHurtTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    GridMonsterMON179::FTestWorld TestWorld;
    AGridLevelRuntimeActor* Runtime = TestWorld.World
        ? TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ()
        : nullptr;
    GridMonsterMON179::ConfigureFloor (Runtime);
    UGridMonsterDefinitionAsset* Definition =
        GridMonsterMON179::MakeDefinition (Runtime);
    AGridMonsterActor* Monster = GridMonsterMON179::SpawnMonster (
        TestWorld.World,
        Runtime,
        Definition,
        FGuid (179, 1, 1, 1),
        FIntPoint (2, 2),
        TEXT ("MON179_NonFatal"));
    if (!Monster || !Monster->AudioComponent)
    {
        return false;
    }

    FGridAttackResult Result;
    Result.bHit = true;
    Result.HealthDamage = 2;
    Monster->ApplyAttackResult (Result);

    TestEqual (
        TEXT ("Non-fatal effective damage enters Hurt state"),
        Monster->MonsterState,
        EGridMonsterState::Hurt);
    TestEqual (TEXT ("Non-fatal damage preserves positive health"),
        Monster->CurrentHealth,
        8);
    TestEqual (
        TEXT ("Non-fatal effective damage requests Hurt audio once"),
        Monster->AudioComponent->GetPlaybackRequestCountForEvent (
            EGridMonsterAudioEvent::Hurt),
        1);
    TestEqual (
        TEXT ("Non-fatal damage requests no Death audio"),
        Monster->AudioComponent->GetPlaybackRequestCountForEvent (
            EGridMonsterAudioEvent::Death),
        0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON179FatalDamageBypassesHurtTest,
    "Grimrock.Monsters.MON17.9.FatalDamageBypassesHurt",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON179FatalDamageBypassesHurtTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    GridMonsterMON179::FTestWorld TestWorld;
    AGridLevelRuntimeActor* Runtime = TestWorld.World
        ? TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ()
        : nullptr;
    GridMonsterMON179::ConfigureFloor (Runtime);
    UGridMonsterDefinitionAsset* Definition =
        GridMonsterMON179::MakeDefinition (
            Runtime,
            TEXT ("MON179_FatalMonster"));
    AGridMonsterActor* Monster = GridMonsterMON179::SpawnMonster (
        TestWorld.World,
        Runtime,
        Definition,
        FGuid (179, 2, 1, 1),
        FIntPoint (2, 2),
        TEXT ("MON179_Fatal"));
    if (!Monster || !Monster->AudioComponent)
    {
        return false;
    }

    FGridAttackResult Result;
    Result.bHit = true;
    Result.HealthDamage = 100;
    Monster->ApplyAttackResult (Result);

    TestTrue (TEXT ("Fatal damage commits death"), Monster->IsDead ());
    TestEqual (TEXT ("Fatal damage leaves zero health"),
        Monster->CurrentHealth,
        0);
    TestEqual (TEXT ("Fatal damage ends directly in Dead state"),
        Monster->MonsterState,
        EGridMonsterState::Dead);
    TestEqual (
        TEXT ("Fatal damage requests no Hurt audio"),
        Monster->AudioComponent->GetPlaybackRequestCountForEvent (
            EGridMonsterAudioEvent::Hurt),
        0);
    TestEqual (
        TEXT ("Fatal damage requests Death audio once"),
        Monster->AudioComponent->GetPlaybackRequestCountForEvent (
            EGridMonsterAudioEvent::Death),
        1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON179MissingHurtMontageIsSafeTest,
    "Grimrock.Monsters.MON17.9.MissingHurtMontageIsSafe",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON179MissingHurtMontageIsSafeTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    GridMonsterMON179::FTestWorld TestWorld;
    AGridLevelRuntimeActor* Runtime = TestWorld.World
        ? TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ()
        : nullptr;
    GridMonsterMON179::ConfigureFloor (Runtime);
    UGridMonsterDefinitionAsset* Definition =
        GridMonsterMON179::MakeDefinition (
            Runtime,
            TEXT ("MON179_NoMontage"));
    AGridMonsterActor* Monster = GridMonsterMON179::SpawnMonster (
        TestWorld.World,
        Runtime,
        Definition,
        FGuid (179, 3, 1, 1),
        FIntPoint (2, 2),
        TEXT ("MON179_NoMontage"));
    if (!Monster)
    {
        return false;
    }

    TestTrue (
        TEXT ("Test definition has no Hurt montage"),
        Definition->HurtMontage.IsNull ());
    TestFalse (
        TEXT ("Starting a missing Hurt montage is a safe no-op"),
        Monster->StartHurtPresentation ());

    FGridAttackResult Result;
    Result.bHit = true;
    Result.HealthDamage = 1;
    Monster->ApplyAttackResult (Result);
    TestEqual (
        TEXT ("A monster without HurtMontage still enters Hurt logically"),
        Monster->MonsterState,
        EGridMonsterState::Hurt);
    TestFalse (
        TEXT ("The monster remains alive without a Hurt montage"),
        Monster->IsDead ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON179HurtRestoreNormalizationTest,
    "Grimrock.Monsters.MON17.9.HurtRestoreNormalization",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON179HurtRestoreNormalizationTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    GridMonsterMON179::FTestWorld TestWorld;
    AGridLevelRuntimeActor* Runtime = TestWorld.World
        ? TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ()
        : nullptr;
    GridMonsterMON179::ConfigureFloor (Runtime);
    UGridMonsterDefinitionAsset* Definition =
        GridMonsterMON179::MakeDefinition (
            Runtime,
            TEXT ("MON179_Restore"));
    const FGuid PersistenceId (179, 4, 1, 1);
    AGridMonsterActor* Monster = GridMonsterMON179::SpawnMonster (
        TestWorld.World,
        Runtime,
        Definition,
        PersistenceId,
        FIntPoint (2, 2),
        TEXT ("MON179_Restore"));
    if (!Monster || !Runtime)
    {
        return false;
    }

    FGridRuntimeMonsterState HurtState;
    HurtState.PersistenceId = PersistenceId;
    HurtState.MonsterDefinitionId = Definition->MonsterId;
    HurtState.DungeonLevelId = GridMonsterMON179::TestLevelId;
    HurtState.CellX = 2;
    HurtState.CellY = 2;
    HurtState.Facing = EGridEdge::North;
    HurtState.MonsterState = EGridMonsterState::Hurt;
    HurtState.CurrentHealth = 7;
    HurtState.CurrentPhysicalArmor = 0;
    HurtState.CurrentMagicalArmor = 0;
    HurtState.bMonsterEnabled = true;
    HurtState.bHasLastKnownPartyCell = false;
    HurtState.bIsDead = false;

    Monster->bIsMoving = true;
    Monster->bIsTurning = true;
    Monster->MoveAlpha = 0.5f;
    Monster->TurnDirection = 1;

    TestTrue (
        TEXT ("A Hurt snapshot without a last-known party cell restores"),
        Monster->RestoreRuntimeMonsterState (
            HurtState,
            Runtime));
    TestEqual (
        TEXT ("Restored Hurt without party memory normalizes to Idle"),
        Monster->MonsterState,
        EGridMonsterState::Idle);
    TestFalse (TEXT ("Movement presentation is not restored"),
        Monster->bIsMoving);
    TestFalse (TEXT ("Turn presentation is not restored"),
        Monster->bIsTurning);

    HurtState.bHasLastKnownPartyCell = true;
    HurtState.LastKnownPartyCell = FIntPoint (4, 4);
    TestTrue (
        TEXT ("A Hurt snapshot with a last-known party cell restores"),
        Monster->RestoreRuntimeMonsterState (
            HurtState,
            Runtime));
    TestEqual (
        TEXT ("Restored Hurt with party memory normalizes to Alert"),
        Monster->MonsterState,
        EGridMonsterState::Alert);
    return true;
}

#endif

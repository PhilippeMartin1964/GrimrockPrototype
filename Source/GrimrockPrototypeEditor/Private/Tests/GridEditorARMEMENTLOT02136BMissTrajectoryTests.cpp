#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Runtime/Combat/GridProjectileGeometry.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridThrownItemActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMLOT02136BMissT,
    "Grimrock.RPG.ARMEMENT.LOT02.MissTDeflectionAndGravity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMLOT02136BMissT::RunTest(const FString&)
{
    const FVector TargetExtent(45.0f, 45.0f, 55.0f);
    const FVector Target(0.0f, 0.0f, 110.0f);
    const float HalfRange = 400.0f;

    // Four cardinal headings, left and right: the perpendicular must be
    // computed from the shot's heading, never from world N/S coordinates.
    for (const FVector& Forward : { FVector(0,1,0), FVector(1,0,0), FVector(0,-1,0), FVector(-1,0,0) })
    {
        const FVector Start = Target - Forward * HalfRange;
        const FVector Right = FVector::CrossProduct(Forward, FVector::UpVector);
        for (const int32 Side : { -1, 1 })
        {
            FVector Direction, Aim;
            TestTrue(TEXT("A T-shaped miss has a valid direction"),
                FGridProjectileGeometry::CalculateMissDeflection(Start, Target, TargetExtent,
                    12.0f, 12.0f, Side, Direction, Aim));
            TestTrue(TEXT("Miss is normalized"), FMath::IsNearlyEqual(Direction.Size(), 1.0f, 0.0001f));
            TestTrue(TEXT("Miss continues forward"), FVector::DotProduct(Direction, Forward) > 0.90f);
            TestTrue(TEXT("Miss clears the collision body plus projectile radius and margin"),
                FMath::IsNearlyEqual(FMath::Abs(FVector::DotProduct(Aim - Target, Right)), 69.0f, 0.001f));
            TestTrue(TEXT("Signed lateral deflection follows shot-local right"),
                FVector::DotProduct(Aim - Target, Right) * Side > 0.0f);
        }
    }
    FVector InvalidDirection, InvalidAim;
    TestFalse(TEXT("No flight direction for a zero-length T"),
        FGridProjectileGeometry::CalculateMissDeflection(Target, Target, TargetExtent, 12.0f, 12.0f,
            1, InvalidDirection, InvalidAim));

    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
        .SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
        FName(*FString::Printf(TEXT("ARMLOT02136B_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
        nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Transient world"), World)) return false;
    if (GEngine)
    {
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
        Context.SetCurrentWorld(World);
    }
    AGridLevelRuntimeActor* Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
    if (TestNotNull(TEXT("Runtime actor"), Runtime))
    {
        UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
        Level->Width = 4;
        Level->Height = 4;
        Level->CellSize = 200.0f;
        Level->EnsureCellCount();
        for (FGridLevelCellData& Cell : Level->Cells)
        {
            Cell.CellType = EGridCellType::Floor;
            Cell.bBlocksOccupancy = false;
            Cell.NorthWall = EGridWallType::None;
            Cell.EastWall = EGridWallType::None;
            Cell.SouthWall = EGridWallType::None;
            Cell.WestWall = EGridWallType::None;
        }
        Runtime->LevelAsset = Level;
        UGridItemDefinitionAsset* Arrow = NewObject<UGridItemDefinitionAsset>(Runtime);
        Arrow->ItemDefinitionId = TEXT("Item_TestMissT");
        Arrow->ItemType = EGridItemType::Misc;
        Arrow->Weight = 0.04f;
        Arrow->bStackable = true;
        Arrow->MaxStackSize = 64;
        Arrow->ThrowSpeed = 1800.0f;

        FGridItemInstance Ammo;
        Ammo.RuntimeObjectId = FGuid::NewGuid();
        Ammo.ItemDefinitionId = Arrow->ItemDefinitionId;
        Ammo.Weight = Arrow->Weight;
        Ammo.Quantity = 1;
        Ammo.OwnerType = EGridItemOwnerType::World;
        Ammo.OwnerCharacterIndex = INDEX_NONE;
        const FVector ShotStart = Runtime->GetCellCenterWorld(1, 1, 140.0f);
        const FVector ShotTarget = Runtime->GetCellCenterWorld(1, 2, 140.0f);
        const FVector InitialVelocity(0.0f, 1800.0f, 0.0f);
        AGridThrownItemActor* Projectile = Runtime->SpawnThrownItemProjectile(
            Ammo, Arrow, ShotStart, InitialVelocity, 1, 1, true);
        if (TestNotNull(TEXT("Recoverable ranged projectile spawned"), Projectile))
        {
            const FGuid IdBefore = Projectile->ThrownItemInstance.RuntimeObjectId;
            const FVector LocationBefore = Projectile->GetActorLocation();
            TestTrue(TEXT("Authoritative miss changes trajectory"),
                Projectile->ConfigureRangedMissDeflection(ShotTarget, TargetExtent));
            TestTrue(TEXT("Miss state enabled"), Projectile->IsRangedMissDeflected());
            TestFalse(TEXT("Miss does not embed"), Projectile->IsEmbeddedInMonster());
            TestFalse(TEXT("Miss does not create an immediate pickup"), Projectile->HasCompletedImpactConversion());
            TestEqual(TEXT("GUID preserved"), Projectile->ThrownItemInstance.RuntimeObjectId, IdBefore);
            TestEqual(TEXT("Position unchanged when miss is resolved"), Projectile->GetActorLocation(), LocationBefore);
            TestTrue(TEXT("Heading deviates laterally"), FMath::Abs(Projectile->ProjectileMovementComponent->Velocity.X) > 0.001f);
            TestTrue(TEXT("Speed unchanged"),
                FMath::IsNearlyEqual(Projectile->ProjectileMovementComponent->Velocity.Size(), InitialVelocity.Size(), 0.01f));
            TestFalse(TEXT("Miss config cannot run twice"), Projectile->ConfigureRangedMissDeflection(ShotTarget, TargetExtent));
            TestFalse(TEXT("Gravity waits until target is passed"), Projectile->HasRangedMissGravityStarted());
            Projectile->SetActorLocation(ShotTarget + FVector(0, 30, 0));
            Projectile->Tick(0.016f);
            TestTrue(TEXT("Gravity begins only beyond target plane"), Projectile->HasRangedMissGravityStarted());
            TestEqual(TEXT("Gravity resumes after monster"), Projectile->ProjectileMovementComponent->ProjectileGravityScale, 1.0f);
            TestEqual(TEXT("No ghost pickup generated by miss steering"), Runtime->GetWorldItemWeightAtCell(1, 2), 0.0f);
            Projectile->Destroy();
        }

        // Ordinary thrown weapons must never be re-aimed by the ranged ammo
        // miss policy; the stable/tumbling presentation is preserved.
        AGridThrownItemActor* Legacy = World->SpawnActor<AGridThrownItemActor>();
        if (TestNotNull(TEXT("Ordinary projectile actor"), Legacy))
        {
            TestFalse(TEXT("Non-ammunition projectile does not use miss T"),
                Legacy->ConfigureRangedMissDeflection(ShotTarget, TargetExtent));
            Legacy->Destroy();
        }
    }
    World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World);
    return true;
}

#endif

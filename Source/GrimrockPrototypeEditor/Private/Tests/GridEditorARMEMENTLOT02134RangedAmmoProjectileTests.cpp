#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridThrownItemActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMLot02134RangedArrow,
    "Grimrock.RPG.ARMEMENT.LOT02.RecoverableRangedAmmunitionProjectile",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMLot02134RangedArrow::RunTest(const FString&)
{
    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
        .SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
        FName(*FString::Printf(TEXT("ARMLOT02134_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
        nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Transient world"), World)) return false;
    if (GEngine)
    {
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
        Context.SetCurrentWorld(World);
    }
    AGridLevelRuntimeActor* Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
    if (!TestNotNull(TEXT("Runtime actor"), Runtime))
    {
        World->DestroyWorld(false);
        if (GEngine) GEngine->DestroyWorldContext(World);
        return false;
    }
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
        Cell.SouthWall = EGridWallType::None;
        Cell.EastWall = EGridWallType::None;
        Cell.WestWall = EGridWallType::None;
    }
    Runtime->LevelAsset = Level;
    UGridItemDefinitionAsset* Arrow = NewObject<UGridItemDefinitionAsset>(Runtime);
    Arrow->ItemDefinitionId = TEXT("Item_TestRecoverableAmmo");
    Arrow->ItemType = EGridItemType::Misc;
    Arrow->Weight = 0.04f;
    Arrow->bStackable = true;
    Arrow->MaxStackSize = 64;
    Arrow->ThrowSpeed = 1800.0f;
    TestFalse(TEXT("Inventory ammo is not hand-thrown"), Arrow->IsPhysicallyThrowable());
    TestFalse(TEXT("Ammo is not a combat throwing weapon"), Arrow->IsCombatThrowable());

    FGridItemInstance Unit;
    Unit.RuntimeObjectId = FGuid::NewGuid();
    Unit.ItemDefinitionId = Arrow->ItemDefinitionId;
    Unit.Quantity = 1;
    Unit.Weight = Arrow->Weight;
    Unit.OwnerType = EGridItemOwnerType::World;
    Unit.OwnerCharacterIndex = INDEX_NONE;
    const FVector Start = Runtime->GetCellCenterWorld(1, 1, 140.0f);
    const FVector Target = Runtime->GetCellCenterWorld(1, 2, 140.0f);
    const FVector Velocity = (Target - Start).GetSafeNormal() * 1800.0f;

    TestNull(TEXT("Normal throwing path rejects non-handheld ammo"),
        Runtime->SpawnThrownItemProjectile(Unit, Arrow, Start, Velocity, 1, 1));
    AGridThrownItemActor* Projectile = Runtime->SpawnThrownItemProjectile(Unit, Arrow, Start, Velocity, 1, 1, true);
    if (TestNotNull(TEXT("Explicit ranged ammo path spawns a projectile"), Projectile))
    {
        TestTrue(TEXT("Projectile is an ammunition visual"), Projectile->bRangedAmmunitionProjectile);
        TestEqual(TEXT("Projectile preserves unique unit GUID"), Projectile->ThrownItemInstance.RuntimeObjectId, Unit.RuntimeObjectId);
        TestEqual(TEXT("Only one arrow transported"), Projectile->ThrownItemInstance.Quantity, 1);
        TestEqual(TEXT("Ranged arrow does not spin"), Projectile->AppliedThrowVisualAngularVelocityDegreesPerSecond, FVector::ZeroVector);
        TestEqual(TEXT("Ranged arrow has no gravity"), Projectile->ProjectileMovementComponent->ProjectileGravityScale, 0.0f);
        Projectile->ConfigureCombatPresentationTarget(true, Target, 24.0f, FIntPoint(1, 2));
        Projectile->SetActorLocation(Target);
        Projectile->Tick(0.016f); // Simulate target arrival, not a complete PIE shot.
        TestTrue(TEXT("Impact converted projectile to pickup"), Projectile->HasCompletedImpactConversion());
        int32 PickupCount = 0;
        for (TActorIterator<AGridItemActor> It(World); It; ++It)
        {
            AGridItemActor* Item = *It;
            if (Item && !Item->IsActorBeingDestroyed() && !Item->IsA<AGridThrownItemActor>() &&
                Item->GetRuntimeObjectId() == Unit.RuntimeObjectId)
            {
                ++PickupCount;
                TestEqual(TEXT("Recovered item identity"), Item->GetItemDefinitionId(), Arrow->ItemDefinitionId);
                TestEqual(TEXT("Recovered in monster cell X"), Item->RuntimeCellX, 1);
                TestEqual(TEXT("Recovered in monster cell Y"), Item->RuntimeCellY, 2);
            }
        }
        TestEqual(TEXT("Exactly one recoverable arrow"), PickupCount, 1);
    }

    World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

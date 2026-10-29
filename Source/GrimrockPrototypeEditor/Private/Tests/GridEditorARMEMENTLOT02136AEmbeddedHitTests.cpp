#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridThrownItemActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMLOT02136AEmbeddedHit,
    "Grimrock.RPG.ARMEMENT.LOT02.EmbeddedRangedArrowHit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMLOT02136AEmbeddedHit::RunTest(const FString&)
{
    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
        .SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
        FName(*FString::Printf(TEXT("ARMLOT02136A_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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
        Cell.EastWall = EGridWallType::None;
        Cell.SouthWall = EGridWallType::None;
        Cell.WestWall = EGridWallType::None;
    }
    Runtime->LevelAsset = Level;
    UGridItemDefinitionAsset* Arrow = NewObject<UGridItemDefinitionAsset>(Runtime);
    Arrow->ItemDefinitionId = TEXT("Item_TestEmbeddedArrow");
    Arrow->ItemType = EGridItemType::Misc;
    Arrow->Weight = 0.04f;
    Arrow->bStackable = true;
    Arrow->MaxStackSize = 64;
    Arrow->ThrowSpeed = 1800.0f;
    Arrow->ProjectileMeshForwardAxis = EGridProjectileForwardAxis::PositiveY;
    FGridItemInstance Unit;
    Unit.RuntimeObjectId = FGuid::NewGuid();
    Unit.ItemDefinitionId = Arrow->ItemDefinitionId;
    Unit.Quantity = 1;
    Unit.Weight = Arrow->Weight;
    Unit.OwnerType = EGridItemOwnerType::World;
    Unit.OwnerCharacterIndex = INDEX_NONE;

    const FVector Start = Runtime->GetCellCenterWorld(1, 1, 140.0f);
    const FVector Target = Runtime->GetCellCenterWorld(1, 2, 140.0f);
    AGridMonsterActor* Monster = World->SpawnActor<AGridMonsterActor>();
    if (TestNotNull(TEXT("Monster exists"), Monster))
    {
        Monster->CurrentCell = FIntPoint(1, 2);
        Monster->SetActorLocation(Target - FVector(0,0,45));
        AGridThrownItemActor* Projectile = Runtime->SpawnThrownItemProjectile(
            Unit, Arrow, Start, (Target - Start).GetSafeNormal() * Arrow->ThrowSpeed, 1, 1, true);
        if (TestNotNull(TEXT("Physical arrow projectile exists"), Projectile))
        {
            Projectile->ConfigureCombatPresentationTarget(true, Target, 24.0f, Monster->CurrentCell);
            Projectile->ConfigureRangedHitEmbedding(Monster);
            Projectile->SetActorLocation(Target);
            Projectile->Tick(0.016f);
            TestTrue(TEXT("Successful hit embeds arrow"), Projectile->IsEmbeddedInMonster());
            TestFalse(TEXT("Embedded arrow was not converted immediately"), Projectile->HasCompletedImpactConversion());
            TestTrue(TEXT("Embedded arrow tracks target"), Projectile->GetEmbeddedMonster() == Monster);
            TestTrue(TEXT("Arrow attaches to the skeletal mesh"),
                Projectile->GetRootComponent() && Projectile->GetRootComponent()->GetAttachParent() == Monster->SkeletalMeshComponent);
            TestEqual(TEXT("Ammo identity unchanged in embedded state"), Projectile->ThrownItemInstance.RuntimeObjectId, Unit.RuntimeObjectId);
            TestEqual(TEXT("No premature pickup"), Runtime->GetWorldItemWeightAtCell(1, 2), 0.0f);
            const FVector InitialTip = Projectile->GetActorLocation();
            Monster->AddActorWorldOffset(FVector(32.0f, 0, 0));
            TestTrue(TEXT("Attached arrow follows moving monster"),
                Projectile->GetActorLocation().Equals(InitialTip + FVector(32.0f, 0, 0), 0.1f));
            Monster->SkeletalMeshComponent->SetVisibility(false, true);
            Projectile->Tick(0.016f);
            TestTrue(TEXT("Hidden corpse releases embedded item"), Projectile->HasCompletedImpactConversion());
            TestTrue(TEXT("Recovered in target cell"), Runtime->GetWorldItemWeightAtCell(1, 2) >= Arrow->Weight);
            int32 MatchingPickups = 0;
            for (TActorIterator<AGridItemActor> It(World); It; ++It)
            {
                AGridItemActor* Item = *It;
                if (Item && !Item->IsActorBeingDestroyed() && !Item->IsA<AGridThrownItemActor>() &&
                    Item->GetRuntimeObjectId() == Unit.RuntimeObjectId)
                {
                    ++MatchingPickups;
                }
            }
            TestEqual(TEXT("Exactly one pickup, original GUID"), MatchingPickups, 1);
        }
    }
    World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World);
    return true;
}

#endif

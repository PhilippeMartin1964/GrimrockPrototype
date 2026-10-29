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
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

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
        // Capturing the level also captures every monster. The test monster
        // must satisfy the SAME persistence requirements as production
        // monsters, or CaptureRuntimeMonsterState logs a genuine Error.
        // Use the minimal transient definition pattern from MON17.8.
        UGridMonsterDefinitionAsset* MonsterDefinition = NewObject<UGridMonsterDefinitionAsset>(Runtime);
        MonsterDefinition->MonsterId = TEXT("ARMLOT02136C_PersistenceMonster");
        MonsterDefinition->DisplayName = FText::FromString(TEXT("ARMEMENT saved embedded target"));
        MonsterDefinition->CategoryId = TEXT("Test");
        MonsterDefinition->MaxHealth = 10;
        MonsterDefinition->ActionPointsPerTurn = 2;
        MonsterDefinition->DeathExpectedDuration = 1.0f;
        Monster->MonsterDefinition = MonsterDefinition;
        TestTrue(TEXT("Transient target definition is valid for persistence"),
            MonsterDefinition->IsValidDefinition());
        TestTrue(TEXT("Transient target initializes combat stats before capture"),
            Monster->EnsureInitialCombatState());
        Monster->CurrentCell = FIntPoint(1, 2);
        Monster->SetActorLocation(Target - FVector(0,0,45));
        // Regression: a visual mesh displaced in MONSTER LOCAL coordinates
        // may cross into another world cell after Facing=North (Yaw=90).
        // The true world location must not be inferred from local X directly.
        Monster->SkeletalMeshComponent->SetRelativeLocation(FVector(130,0,0));
        AGridThrownItemActor* Projectile = Runtime->SpawnThrownItemProjectile(
            Unit, Arrow, Start, (Target - Start).GetSafeNormal() * Arrow->ThrowSpeed, 1, 1, true);
        if (TestNotNull(TEXT("Physical arrow projectile exists"), Projectile))
        {
            Projectile->ConfigureCombatPresentationTarget(true, Target, 24.0f, Monster->CurrentCell);
            Projectile->ConfigureRangedHitEmbedding(Monster);
            Projectile->SetActorLocation(Target);
            Projectile->Tick(0.016f);
            TestTrue(TEXT("Successful hit embeds arrow"), Projectile->IsEmbeddedInMonster());
            TestTrue(TEXT("Contact uses visual rather than logical collision box"),
                FMath::Abs(Projectile->GetActorLocation().X -
                    Monster->SkeletalMeshComponent->GetComponentLocation().X) < 30.0f);
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
            // Adjacent-cell regression: the animated visual can translate or
            // lean relative to the monster actor during a Hurt/Death montage.
            // Attachment to actor root alone would leave the arrow floating.
            const FVector BeforeVisualMove = Projectile->GetActorLocation();
            const FVector SkeletalOffset = Monster->SkeletalMeshComponent->GetRelativeLocation();
            Monster->SkeletalMeshComponent->SetRelativeLocation(SkeletalOffset + FVector(0, 0, 28.0f));
            Projectile->Tick(0.016f);
            TestTrue(TEXT("Arrow follows relative skeletal-body translation"),
                Projectile->GetActorLocation().Equals(BeforeVisualMove + FVector(0, 0, 28.0f), 0.2f));
            TestEqual(TEXT("Arrow remains embedded after body translation"), Projectile->IsEmbeddedInMonster(), true);

            // SaveGame regression: an embedded arrow is NOT a ground pickup.
            // One GUID maps to the monster persistence id and the native
            // skeletal attachment's relative pose.
            Monster->PersistentMonsterId = FGuid::NewGuid();
            TestTrue(TEXT("Embedded arrow is capturable by runtime SaveGame"),
                Runtime->CaptureCurrentLevelRuntimeState());
            const FGridLevelRuntimeState* CapturedLevel = Runtime->FindRuntimeStateForCurrentLevel();
            const FGridRuntimeItemState* EmbeddedState = CapturedLevel
                ? CapturedLevel->Items.Find(Unit.RuntimeObjectId) : nullptr;
            const FGridRuntimeMonsterState* CapturedMonster = CapturedLevel
                ? CapturedLevel->Monsters.Find(Monster->PersistentMonsterId) : nullptr;
            if (TestNotNull(TEXT("Target monster was validly captured"), CapturedMonster))
            {
                TestEqual(TEXT("Persisted monster definition ID"),
                    CapturedMonster->MonsterDefinitionId, MonsterDefinition->MonsterId);
                TestEqual(TEXT("Persisted monster combat health"),
                    CapturedMonster->CurrentHealth, MonsterDefinition->MaxHealth);
            }
            if (TestNotNull(TEXT("One saved entry for embedded ammo GUID"), EmbeddedState))
            {
                TestTrue(TEXT("Embedded state remains attached rather than ground pickup"),
                    EmbeddedState->bEmbeddedProjectile);
                TestEqual(TEXT("Saved embedded monster identity"),
                    EmbeddedState->EmbeddedMonsterPersistenceId, Monster->PersistentMonsterId);
                TestTrue(TEXT("Saved arrow preserves its rendered mesh pose"),
                    EmbeddedState->WorldMeshTransform.GetLocation().Equals(
                        Projectile->MeshComponent->GetComponentLocation(), 0.1f));
                AGridThrownItemActor* Restored = Runtime->SpawnThrownItemProjectile(
                    Unit, Arrow, EmbeddedState->WorldMeshTransform.GetLocation(),
                    FVector(0, 1400, 0), 1, 2, true);
                if (TestNotNull(TEXT("Embeddable save round-trip projectile"), Restored))
                {
                    TestTrue(TEXT("Restore attachment without rerolling combat"),
                        Restored->RestoreEmbeddedAttachment(Monster, EmbeddedState->EmbeddedBone,
                            EmbeddedState->EmbeddedBoneRelativeTransform));
                    TestTrue(TEXT("Restored arrow is attached to same skeletal mesh"),
                        Restored->GetRootComponent() &&
                        Restored->GetRootComponent()->GetAttachParent() == Monster->SkeletalMeshComponent);
                    TestEqual(TEXT("Restored projectile retains saved GUID"),
                        Restored->ThrownItemInstance.RuntimeObjectId, Unit.RuntimeObjectId);
                    Restored->Destroy();
                }
            }
            // Rotate the visual offset with the monster's facing. Here the
            // monster is North (yaw 90), so its +130 LOCAL X is WORLD +Y.
            // A previous test wrongly assumed local +X was world +X.
            const FVector MeshWorldFromLocal = Monster->GetActorTransform().TransformPosition(
                Monster->SkeletalMeshComponent->GetRelativeLocation());
            const FVector MeshWorld = Monster->SkeletalMeshComponent->GetComponentLocation();
            TestTrue(TEXT("Monster local visual offset transforms into world space"),
                MeshWorld.Equals(MeshWorldFromLocal, 0.1f));
            const FVector ReleaseWorldLocation = Projectile->GetActorLocation();
            TestTrue(TEXT("Embedded tip remains close to the displaced visual mesh"),
                FVector::Dist(ReleaseWorldLocation, MeshWorld) <= 30.0f);

            int32 ReleaseCellX = INDEX_NONE;
            int32 ReleaseCellY = INDEX_NONE;
            FVector ReleaseLocalOffset = FVector::ZeroVector;
            const bool bReleaseCellResolved = Runtime->TryResolveWorldCellFromImpactPoint(
                ReleaseWorldLocation, ReleaseCellX, ReleaseCellY, ReleaseLocalOffset);
            TestTrue(TEXT("Embedded arrow release location resolves to a walkable cell"), bReleaseCellResolved);
            // Resolve a second independent world point: the visual component.
            // Both points are near one another; the test rig intentionally
            // positions them well inside a different cell from (1,2).
            int32 VisualCellX = INDEX_NONE;
            int32 VisualCellY = INDEX_NONE;
            FVector VisualLocalOffset = FVector::ZeroVector;
            const bool bVisualCellResolved = Runtime->TryResolveWorldCellFromImpactPoint(
                MeshWorld, VisualCellX, VisualCellY, VisualLocalOffset);
            TestTrue(TEXT("Displaced monster visual is in a walkable cell"), bVisualCellResolved);
            if (bReleaseCellResolved && bVisualCellResolved)
            {
                TestEqual(TEXT("Arrow release cell matches visual mesh cell X"), ReleaseCellX, VisualCellX);
                TestEqual(TEXT("Arrow release cell matches visual mesh cell Y"), ReleaseCellY, VisualCellY);
                TestTrue(TEXT("Visual offset crosses the original target-cell boundary"),
                    ReleaseCellX != Monster->CurrentCell.X || ReleaseCellY != Monster->CurrentCell.Y);
            }
            AddInfo(FString::Printf(TEXT("Embedded release: monster facing=%d visual=%s tip=%s cell=(%d,%d)"),
                static_cast<int32>(Monster->Facing), *MeshWorld.ToCompactString(),
                *ReleaseWorldLocation.ToCompactString(), ReleaseCellX, ReleaseCellY));

            // Physical release must use the actual projectile mesh pose,
            // including the yaw from its flight orientation.
            const FVector MeshImpactLocation = Projectile->MeshComponent->GetComponentLocation();
            const FQuat MeshImpactRotation = Projectile->MeshComponent->GetComponentQuat();
            Monster->SkeletalMeshComponent->SetVisibility(false, true);
            Projectile->Tick(0.016f);
            TestTrue(TEXT("Hidden corpse releases embedded item"), Projectile->HasCompletedImpactConversion());
            TestTrue(TEXT("Pickup registered in arrow release cell"),
                bReleaseCellResolved && Runtime->GetWorldItemWeightAtCell(ReleaseCellX, ReleaseCellY) >= Arrow->Weight);
            if (bReleaseCellResolved &&
                (ReleaseCellX != Monster->CurrentCell.X || ReleaseCellY != Monster->CurrentCell.Y))
            {
                TestEqual(TEXT("Original combat cell has no phantom pickup"),
                    Runtime->GetWorldItemWeightAtCell(Monster->CurrentCell.X, Monster->CurrentCell.Y), 0.0f);
            }
            int32 MatchingPickups = 0;
            for (TActorIterator<AGridItemActor> It(World); It; ++It)
            {
                AGridItemActor* Item = *It;
                if (Item && !Item->IsActorBeingDestroyed() && !Item->IsA<AGridThrownItemActor>() &&
                    Item->GetRuntimeObjectId() == Unit.RuntimeObjectId)
                {
                    ++MatchingPickups;
                    TestTrue(TEXT("Released pickup preserves 3D impact mesh position"),
                        Item->MeshComponent->GetComponentLocation().Equals(MeshImpactLocation, 0.1f));
                    TestTrue(TEXT("Released pickup preserves 3D impact mesh orientation"),
                        Item->MeshComponent->GetComponentQuat().Equals(MeshImpactRotation, 0.001f));
                    TestEqual(TEXT("GUID pickup is registered at the release cell X"), Item->RuntimeCellX, ReleaseCellX);
                    TestEqual(TEXT("GUID pickup is registered at the release cell Y"), Item->RuntimeCellY, ReleaseCellY);
                    TestEqual(TEXT("GUID pickup preserves item definition"), Item->GetItemDefinitionId(), Arrow->ItemDefinitionId);
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

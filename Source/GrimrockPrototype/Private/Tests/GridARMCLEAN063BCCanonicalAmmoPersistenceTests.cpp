#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridThrownItemActor.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

namespace ARMCLEAN063BC
{
    struct FFixture
    {
        UWorld* World = nullptr;
        UGameInstance* GI = nullptr;
        UGridItemSubsystem* Registry = nullptr;
        AGridLevelRuntimeActor* Runtime = nullptr;

        FFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM063BC_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (!World || !GEngine) return;
            FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
            Context.SetCurrentWorld(World);
            GI = NewObject<UGameInstance>(GetTransientPackage());
            if (!GI) return;
            GI->Init();
            World->SetGameInstance(GI);
            Registry = GI->GetSubsystem<UGridItemSubsystem>();
            Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
            if (!Runtime) return;
            UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
            Level->Width = 4;
            Level->Height = 4;
            Level->CellSize = 200.f;
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
        }

        ~FFixture()
        {
            if (World) World->SetGameInstance(nullptr);
            if (GI) GI->Shutdown();
            if (World)
            {
                World->DestroyWorld(false);
                if (GEngine) GEngine->DestroyWorldContext(World);
            }
        }

        bool Ready() const
        {
            return Runtime && Registry && Runtime->IsWalkableCell(1, 1) &&
                Registry->ContainsItem(TEXT("Item_Arrows_01"));
        }

        FGridItemInstance Arrow() const
        {
            FGridItemInstance Item;
            Item.RuntimeObjectId = FGuid::NewGuid();
            Item.ItemDefinitionId = TEXT("Item_Arrows_01");
            Item.ItemLevel = 0;
            Item.Quantity = 1;
            Item.OwnerType = EGridItemOwnerType::World;
            Item.OwnerCharacterIndex = INDEX_NONE;
            return Item;
        }
    };

    bool RunScenario(FAutomationTestBase& Test, const TCHAR* Scenario)
    {
        FFixture F;
        if (!Test.TestTrue(TEXT("Real canonical arrows and walkable 4x4 runtime"), F.Ready()))
            return false;

        const FGridItemInstance Unit = F.Arrow();
        FGridResolvedItem Canonical;
        if (!Test.TestTrue(TEXT("Arrow N0 resolves from real registry"),
            F.Registry->ResolveItem(Unit.ItemDefinitionId, Unit.ItemLevel, Canonical)))
            return false;
        Test.TestTrue(TEXT("Arrow is stackable canonical ammunition"),
            !Canonical.bIsArmament && !Canonical.AmmoKindId.IsNone() && Canonical.MaxStack > 1);


        if (FCString::Strcmp(Scenario, TEXT("Drop")) == 0)
        {
            if (!Test.TestTrue(TEXT("Canonical ammo ignores contradictory legacy item while landing"),
                F.Runtime->TryDropItemInstanceAtCell(Unit, 1, 1, EGridEdge::None,
                    FVector::ZeroVector))) return false;
            int32 Count = 0;
            for (TActorIterator<AGridItemActor> It(F.World); It; ++It)
            {
                AGridItemActor* Item = *It;
                if (IsValid(Item) && !Item->IsA<AGridThrownItemActor>() &&
                    Item->GetRuntimeObjectId() == Unit.RuntimeObjectId)
                {
                    ++Count;
                    Test.TestEqual(TEXT("Original ItemId preserved"), Item->GetItemDefinitionId(),
                        Unit.ItemDefinitionId);
                    Test.TestTrue(TEXT("Actual canonical world mesh"),
                        Item->MeshComponent && Item->MeshComponent->GetStaticMesh() != nullptr);
                }
            }
            Test.TestEqual(TEXT("One recoverable pickup with original GUID"), Count, 1);
            Test.TestTrue(TEXT("Physical weight is not legacy 500 kg"),
                F.Runtime->GetWorldItemWeightAtCell(1, 1) < 500.0f);
            Test.TestEqual(TEXT("Pressure plate sees the exact canonical unit mass"),
                F.Runtime->GetWorldItemWeightAtCell(1, 1), Canonical.WeightKg);
            return !Test.HasAnyErrors();
        }

        const FVector Start = F.Runtime->GetCellCenterWorld(1, 1, 140.0f);
        const FVector Velocity(0, 1200.f, 0);
        AGridThrownItemActor* Projectile = F.Runtime->SpawnThrownItemProjectile(
            Unit, Start, Velocity, 1, 1, true);
        if (!Test.TestNotNull(TEXT("Canonical projectile can launch with obsolete metadata"),
            Projectile)) return false;

        if (FCString::Strcmp(Scenario, TEXT("InFlightSave")) == 0)
        {
            if (!Test.TestTrue(TEXT("Capture in-flight arrow"),
                F.Runtime->CaptureCurrentLevelRuntimeState())) return false;
            const FGridLevelRuntimeState* Snapshot = F.Runtime->FindRuntimeStateForCurrentLevel();
            const FGridRuntimeItemState* Saved = Snapshot ? Snapshot->Items.Find(Unit.RuntimeObjectId) : nullptr;
            if (!Test.TestNotNull(TEXT("Exactly identified saved projectile"), Saved)) return false;
            Test.TestEqual(TEXT("Saved identity"), Saved->ItemDefinitionId, Unit.ItemDefinitionId);
            Test.TestEqual(TEXT("Saved unlevelled arrow"), Saved->ItemLevel, 0);
            Test.TestTrue(TEXT("Saved true mesh pose"), Saved->bHasWorldMeshTransform);
            Test.TestFalse(TEXT("Flying arrow is not an embedded arrow"), Saved->bEmbeddedProjectile);
            Test.TestNull(TEXT("Projectile definition field is retired"),
                FGridRuntimeItemState::StaticStruct()->FindPropertyByName(TEXT("ProjectileDefinitionAsset")));
            // The stale input definition is still present in the fixture but
            // cannot influence reconstruction from the physical snapshot.
            if (!Test.TestTrue(TEXT("Save state restores as pickup"),
                F.Runtime->ApplyCurrentLevelRuntimeState())) return false;
            int32 Pickups = 0;
            for (TActorIterator<AGridItemActor> It(F.World); It; ++It)
            {
                AGridItemActor* Item = *It;
                if (IsValid(Item) && !Item->IsActorBeingDestroyed() &&
                    !Item->IsA<AGridThrownItemActor>() &&
                    Item->GetRuntimeObjectId() == Unit.RuntimeObjectId)
                {
                    ++Pickups;
                    Test.TestEqual(TEXT("Reconstituted ItemId"), Item->GetItemDefinitionId(),
                        Unit.ItemDefinitionId);
                }
            }
            Test.TestEqual(TEXT("One GUID materializes exactly one pickup"), Pickups, 1);
            return !Test.HasAnyErrors();
        }

        if (FCString::Strcmp(Scenario, TEXT("EmbeddedSave")) == 0)
        {
            const FVector Target = F.Runtime->GetCellCenterWorld(1, 2, 140.0f);
            AGridMonsterActor* Monster = F.World->SpawnActor<AGridMonsterActor>();
            if (!Test.TestNotNull(TEXT("Target monster spawned"), Monster)) return false;
            UGridMonsterDefinitionAsset* Def = NewObject<UGridMonsterDefinitionAsset>(F.Runtime);
            Def->MonsterId = TEXT("ARM063BC_PersistTarget");
            Def->DisplayName = FText::FromString(TEXT("ARM063BC target"));
            Def->CategoryId = TEXT("Test");
            Def->MaxHealth = 10;
            Def->ActionPointsPerTurn = 2;
            Def->DeathExpectedDuration = 1.f;
            Monster->MonsterDefinition = Def;
            if (!Test.TestTrue(TEXT("Capture monster definition is valid"),
                Def->IsValidDefinition())) return false;
            if (!Test.TestTrue(TEXT("Monster initial combat state valid"),
                Monster->EnsureInitialCombatState())) return false;
            Monster->PersistentMonsterId = FGuid::NewGuid();
            Monster->CurrentCell = FIntPoint(1, 2);
            Monster->SetActorLocation(Target - FVector(0, 0, 45));
            Monster->SkeletalMeshComponent->SetRelativeLocation(FVector(130, 0, 0));
            Projectile->ConfigureCombatPresentationTarget(true, Target, 24.f, Monster->CurrentCell);
            Projectile->ConfigureRangedHitEmbedding(Monster);
            Projectile->SetActorLocation(Target);
            Projectile->Tick(0.016f);
            if (!Test.TestTrue(TEXT("The real arrow embedded before capture"),
                Projectile->IsEmbeddedInMonster())) return false;
            if (!Test.TestTrue(TEXT("Save embedded arrow and monster"),
                F.Runtime->CaptureCurrentLevelRuntimeState())) return false;
            const FGridLevelRuntimeState* Snapshot = F.Runtime->FindRuntimeStateForCurrentLevel();
            const FGridRuntimeItemState* Saved = Snapshot ? Snapshot->Items.Find(Unit.RuntimeObjectId) : nullptr;
            if (!Test.TestNotNull(TEXT("Saved embedded arrow by same GUID"), Saved)) return false;
            Test.TestTrue(TEXT("Correct monster GUID captured"),
                Saved->bEmbeddedProjectile &&
                Saved->EmbeddedMonsterPersistenceId == Monster->PersistentMonsterId);
            Test.TestNull(TEXT("No saved projectile definition field"),
                FGridRuntimeItemState::StaticStruct()->FindPropertyByName(TEXT("ProjectileDefinitionAsset")));
            if (!Test.TestTrue(TEXT("Restore embedded arrow after snapshot"),
                F.Runtime->ApplyCurrentLevelRuntimeState())) return false;
            int32 Embedded = 0;
            int32 Pickups = 0;
            for (TActorIterator<AGridThrownItemActor> It(F.World); It; ++It)
            {
                AGridThrownItemActor* Arrow = *It;
                if (IsValid(Arrow) && !Arrow->IsActorBeingDestroyed() &&
                    Arrow->ThrownItemInstance.RuntimeObjectId == Unit.RuntimeObjectId)
                {
                    if (Arrow->IsEmbeddedInMonster() &&
                        Arrow->GetEmbeddedMonster() == Monster)
                    {
                        ++Embedded;
                        Test.TestEqual(TEXT("Attachment retains actual ItemId"),
                            Arrow->ThrownItemInstance.ItemDefinitionId, Unit.ItemDefinitionId);
                        Test.TestEqual(TEXT("Restored ammunition retains N0"), Arrow->ThrownItemInstance.ItemLevel, 0);
                        Test.TestEqual(TEXT("Restored ammunition retains one physical unit"), Arrow->ThrownItemInstance.Quantity, 1);
                    }
                }
            }
            for (TActorIterator<AGridItemActor> It(F.World); It; ++It)
            {
                AGridItemActor* Item = *It;
                if (IsValid(Item) && !Item->IsActorBeingDestroyed() &&
                    !Item->IsA<AGridThrownItemActor>() &&
                    Item->GetRuntimeObjectId() == Unit.RuntimeObjectId)
                    ++Pickups;
            }
            Test.TestEqual(TEXT("Living saved monster keeps a single attached arrow"), Embedded, 1);
            Test.TestEqual(TEXT("No duplicate pickup for live attached arrow"), Pickups, 0);
            return !Test.HasAnyErrors();
        }
        return false;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMCLEAN063BCDrop,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalAmmoPersistence.Drop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMCLEAN063BCDrop::RunTest(const FString&)
{
    return ARMCLEAN063BC::RunScenario(*this, TEXT("Drop"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMCLEAN063BCInFlightSave,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalAmmoPersistence.InFlightSave",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMCLEAN063BCInFlightSave::RunTest(const FString&)
{
    return ARMCLEAN063BC::RunScenario(*this, TEXT("InFlightSave"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMCLEAN063BCEmbeddedSave,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalAmmoPersistence.EmbeddedSave",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMCLEAN063BCEmbeddedSave::RunTest(const FString&)
{
    return ARMCLEAN063BC::RunScenario(*this, TEXT("EmbeddedSave"));
}

#endif

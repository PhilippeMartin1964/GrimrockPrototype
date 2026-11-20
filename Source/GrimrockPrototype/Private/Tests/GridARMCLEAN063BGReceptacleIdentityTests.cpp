#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridReceptacleContainedIdentityReadModel.h"
#include "../Runtime/GridReceptacleItemMetadataReadModel.h"

namespace ARMCLEAN063BG
{
    struct FFixture
    {
        UWorld* World = nullptr;
        UGameInstance* GI = nullptr;
        UGridItemSubsystem* Registry = nullptr;
        AGridReceptacleActor* Receptacle = nullptr;

        FFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM063BG_%s"),
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
            Receptacle = World->SpawnActor<AGridReceptacleActor>();
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
            return Registry && Receptacle &&
                Registry->HasCanonicalItem(TEXT("Item_Arrows_01")) &&
                Registry->HasCanonicalItem(TEXT("Weapon_StraightSword_01"));
        }

        UGridItemDefinitionAsset* Contradictory() const
        {
            UGridItemDefinitionAsset* Old =
                NewObject<UGridItemDefinitionAsset>(Receptacle);
            Old->ItemDefinitionId = TEXT("Weapon_StraightSword_01");
            Old->Weight = 1000.0f;
            return Old;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridBGPhysicalInsertion,
    "Grimrock.RPG.ARM.CLEAN06_3.ReceptacleIdentity.PhysicalInsertionCapture",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridBGPhysicalInsertion::RunTest(const FString&)
{
    ARMCLEAN063BG::FFixture F;
    if (!TestTrue(TEXT("Production registry and real receptacle"), F.Ready()))
        return false;
    const FName ArrowId(TEXT("Item_Arrows_01"));
    UGridItemDefinitionAsset* Old = F.Contradictory();
    TestFalse(TEXT("Empty explicit ID cannot be supplied by old pointer"),
        F.Receptacle->TryInsertItem(NAME_None, Old, nullptr));
    TestFalse(TEXT("Unknown explicit ID cannot be renamed by old pointer"),
        F.Receptacle->TryInsertItem(TEXT("CLEAN06_5G49_Unknown"), Old, nullptr));
    TestFalse(TEXT("ID-only insertion cannot guess an ARM level"),
        F.Receptacle->TryInsertItem(TEXT("Weapon_StraightSword_01"), Old, nullptr));
    if (!TestTrue(TEXT("Registered N0 arrow inserts with contradicting legacy asset"),
        F.Receptacle->TryInsertItem(ArrowId, Old, nullptr))) return false;
    if (!TestEqual(TEXT("One item physically inserted"),
        F.Receptacle->GetContainedItemCount(), 1)) return false;
    const FGridContainedReceptacleItem& Contained = F.Receptacle->ContainedItems[0];
    TestEqual(TEXT("Authoritative contained ItemId"), Contained.ItemDefinitionId, ArrowId);
    TestEqual(TEXT("N0 ammo level preserved"), Contained.ItemLevel, 0);
    TestNull(TEXT("Contradicting definition removed as visual authority"),
        Contained.ItemDefinition.Get());
    AGridItemActor* Physical = F.Receptacle->GetContainedItemActor(0);
    if (!TestNotNull(TEXT("Actual contained item actor"), Physical)) return false;
    TestEqual(TEXT("Physical actor has canonical identity"),
        Physical->GetItemDefinitionId(), ArrowId);
    TestNull(TEXT("Standalone actor never stores obsolete definition"), Physical->GetItemDefinitionAsset());
    TestTrue(TEXT("Standalone materialization has full canonical presentation"),
        Physical->CanonicalWorldPresentation.IsValid());
    TestEqual(TEXT("Standalone item-level remains N0"), Physical->CanonicalWorldItemLevel, 0);
    TestEqual(TEXT("Standalone actor carries the actual instance GUID"),
        Physical->GetRuntimeObjectId(), Contained.RuntimeObjectId);

    FGridRuntimeReceptacleState Snapshot;
    F.Receptacle->CaptureRuntimeReceptacleState(Snapshot);
    if (!TestEqual(TEXT("One contained state captured"), Snapshot.ContainedItems.Num(), 1))
        return false;
    TestEqual(TEXT("SaveGame item identity is canonical"),
        Snapshot.ContainedItems[0].ItemDefinitionId, ArrowId);
    TestEqual(TEXT("Saved level remains exactly N0"),
        Snapshot.ContainedItems[0].ItemLevel, 0);
    TestTrue(TEXT("Saved GUID is real contained GUID"),
        Snapshot.ContainedItems[0].ObjectId == Contained.RuntimeObjectId);
    const FName FakeTag(TEXT("CLEAN06_5G49_FakeTag"));
    Old->ItemTags.Add(FakeTag);
    F.Receptacle->ContainedItems[0].ItemDefinition = Old;
    TestFalse(TEXT("Contradictory historical tag cannot authorize registered item"),
        F.Receptacle->ContainsItemTag(FakeTag));
    F.Receptacle->ContainedItems[0].ItemDefinitionId = TEXT("CLEAN06_5G49_Unknown");
    Old->ItemDefinitionId = F.Receptacle->ContainedItems[0].ItemDefinitionId;
    TestFalse(TEXT("Even matching old pointer cannot authorize an unknown tag condition"),
        F.Receptacle->ContainsItemTag(FakeTag));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridBGLevelAndLegacy,
    "Grimrock.RPG.ARM.CLEAN06_3.ReceptacleIdentity.LevelAndLegacyRules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridBGLevelAndLegacy::RunTest(const FString&)
{
    ARMCLEAN063BG::FFixture F;
    if (!TestTrue(TEXT("Production registry and real receptacle"), F.Ready()))
        return false;
    UGridItemDefinitionAsset* Old = F.Contradictory();
    const FName ArrowId(TEXT("Item_Arrows_01"));
    const auto Arrow = FGridReceptacleItemMetadataReadModel::Resolve(
        F.Registry, Old, ArrowId, 0, false);
    TestTrue(TEXT("Registered arrow N0 ignores contradicting old definition"),
        Arrow.bCanonicalIdentity && Arrow.bResolved && Arrow.FixedItemLevel == 0 &&
        Arrow.UnitWeightKg < Old->Weight);
    const auto InvalidArrow = FGridReceptacleItemMetadataReadModel::Resolve(
        F.Registry, Old, ArrowId, 1, false);
    TestFalse(TEXT("N1 ammunition remains invalid"), InvalidArrow.bResolved);
    const FName SwordId(TEXT("Weapon_StraightSword_01"));
    TestEqual(TEXT("Explicit ItemId takes precedence for registered source"),
        FGridReceptacleContainedIdentityReadModel::Resolve(F.Registry, Old, ArrowId), ArrowId);
    const auto InvalidSword = FGridReceptacleItemMetadataReadModel::Resolve(
        F.Registry, nullptr, SwordId, 0, false);
    TestFalse(TEXT("Persisted N0 weapon cannot inherit N1"), InvalidSword.bResolved);
    const auto Sword = FGridReceptacleItemMetadataReadModel::Resolve(
        F.Registry, Old, SwordId, 20, false);
    TestTrue(TEXT("Real N20 item level retained"),
        Sword.bCanonicalIdentity && Sword.bResolved && Sword.FixedItemLevel == 20);

    Old->ItemDefinitionId = TEXT("BG_UnregisteredLegacy");
    TestEqual(TEXT("Non-migrated IDs retain old priority"),
        FGridReceptacleContainedIdentityReadModel::Resolve(
            F.Registry, Old, TEXT("BG_UnregisteredEntry")), Old->ItemDefinitionId);
    Old->ItemDefinitionId = ArrowId;
    TestTrue(TEXT("Old registered source cannot rename unrelated entry"),
        FGridReceptacleContainedIdentityReadModel::Resolve(
            F.Registry, Old, TEXT("BG_UnregisteredEntry")).IsNone());
    TestEqual(TEXT("No explicit initial ID may still use old authored ItemId"),
        FGridReceptacleContainedIdentityReadModel::Resolve(
            F.Registry, Old, NAME_None), ArrowId);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridBGConflictingActor,
    "Grimrock.RPG.ARM.CLEAN06_3.ReceptacleIdentity.ConflictingActorCapture",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridBGConflictingActor::RunTest(const FString&)
{
    ARMCLEAN063BG::FFixture F;
    if (!TestTrue(TEXT("Production registry and real receptacle"), F.Ready()))
        return false;
    const FName ArrowId(TEXT("Item_Arrows_01"));
    if (!TestTrue(TEXT("Registered arrow successfully inserted"),
        F.Receptacle->TryInsertItem(ArrowId, F.Contradictory(), nullptr)))
        return false;
    AGridItemActor* Actor = F.Receptacle->GetContainedItemActor(0);
    if (!TestNotNull(TEXT("Physical actor available"), Actor)) return false;
    Actor->ItemDefinitionId = TEXT("Weapon_StraightSword_01");
    FGridRuntimeReceptacleState Snapshot;
    TestFalse(TEXT("Conflict rejects entire receptacle capture"), F.Receptacle->CaptureRuntimeReceptacleState(Snapshot));
    TestEqual(TEXT("Cannot capture registered item through contradictory actor"),
        Snapshot.ContainedItems.Num(), 0);
    TestEqual(TEXT("No change to contained logical ItemId"),
        F.Receptacle->ContainedItems[0].ItemDefinitionId, ArrowId);
    return !HasAnyErrors();
}

#endif

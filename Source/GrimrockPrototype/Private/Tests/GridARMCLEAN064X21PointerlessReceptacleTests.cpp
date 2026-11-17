#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridItemVisualEffectsComponent.h"
#include "Runtime/Items/GridItemSubsystem.h"

namespace ARMCLEAN064X21
{
struct FWorld
{
    UWorld* World = nullptr;
    UGameInstance* GI = nullptr;
    UGridItemSubsystem* Registry = nullptr;
    AGridLevelRuntimeActor* Runtime = nullptr;
    FGuid Ids[3];

    FWorld()
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).RequiresHitProxies(false)
            .CreatePhysicsScene(false).CreateNavigation(false)
            .CreateAISystem(false).ShouldSimulatePhysics(false)
            .SetTransactional(false);
        World = UWorld::CreateWorld(EWorldType::Game, false,
            FName(*FString::Printf(TEXT("ARM_CLEAN064X21_%s"),
                *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
            nullptr, true, ERHIFeatureLevel::Num, &Values);
        if (!World || !GEngine) { return; }
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
        Context.SetCurrentWorld(World);
        GI = NewObject<UGameInstance>(GetTransientPackage());
        if (!GI) { return; }
        GI->Init();
        World->SetGameInstance(GI);
        Registry = GI->GetSubsystem<UGridItemSubsystem>();
        Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
        if (!Runtime) { return; }

        UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
        Level->Width = 4;
        Level->Height = 4;
        Level->EnsureCellCount();
        for (FGridLevelCellData& Cell : Level->Cells)
        {
            Cell.CellType = EGridCellType::Floor;
            Cell.bBlocksOccupancy = false;
        }
        Runtime->LevelAsset = Level;

        UGridWorldObjectDefinitionAsset* Definition =
            NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
        Definition->DefinitionId = TEXT("ARM_CLEAN064X21_Receptacle");
        Definition->SupportedType = EGridLevelObjectType::Receptacle;
        Definition->DefaultBehavior.Receptacle.MaxContainedItems = 2;
        Definition->PlacementSurface = EGridObjectPlacementKind::Wall;
        Definition->bIsInteractable = true;
        Definition->RuntimeActorClass = AGridReceptacleActor::StaticClass();
        Definition->StaticPart.Mesh = NewObject<UStaticMesh>(Runtime);
        Runtime->WorldObjectDefinitions.Add(Definition);

        for (int32 Index = 0; Index < 3; ++Index)
        {
            Ids[Index] = FGuid::NewGuid();
            FGridWorldObjectInstance& Placed =
                Level->WorldObjectInstances.AddDefaulted_GetRef();
            Placed.InstanceId = Ids[Index];
            Placed.Type = EGridLevelObjectType::Receptacle;
            Placed.WorldObjectDefinitionId = Definition->DefinitionId;
            Placed.CellX = Index;
            Placed.CellY = 1;
            Placed.WallSide = EGridEdge::North;
            FGridReceptacleInitialItemConfig& Initial =
                Placed.InstanceConfig.ReceptacleInitialContent.AddDefaulted_GetRef();
            Initial.ItemDefinition = nullptr;
            Initial.CanonicalItemId = TEXT("Weapon_Dagger_01");
            Initial.ItemLevel = Index == 0 ? 1 : Index == 1 ? 20 : 0;
            Initial.Quantity = 1;
        }
    }

    ~FWorld()
    {
        if (World) { World->SetGameInstance(nullptr); }
        if (GI) { GI->Shutdown(); }
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) { GEngine->DestroyWorldContext(World); }
        }
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X21PointerlessInitialArm,
    "Grimrock.RPG.ARM.CLEAN06_4X21.PointerlessCanonicalArmInitialReceptacle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X21PointerlessInitialArm::RunTest(const FString&)
{
    ARMCLEAN064X21::FWorld F;
    if (!TestNotNull(TEXT("World"), F.World) ||
        !TestNotNull(TEXT("Registry"), F.Registry) ||
        !TestNotNull(TEXT("Runtime"), F.Runtime))
    {
        return false;
    }

    F.Runtime->RebuildLevel();
    for (int32 Index = 0; Index < 3; ++Index)
    {
        AGridReceptacleActor* Receptacle =
            F.Runtime->FindRuntimeObjectActor<AGridReceptacleActor>(F.Ids[Index]);
        if (!TestNotNull(TEXT("Real receptacle materialized"), Receptacle))
        {
            return false;
        }
        if (Index == 2)
        {
            // An invalid persisted ARM N0 must never become N1.
            if (!TestEqual(TEXT("Invalid ARM N0 did not create initial item"),
                Receptacle->GetContainedItemCount(), 0))
            {
                return false;
            }
            continue;
        }
        const int32 Level = Index == 0 ? 1 : 20;
        FGridResolvedItem Expected;
        if (!TestTrue(TEXT("Real canonical ARM resolves at N1/N20"),
                F.Registry->ResolveItem(TEXT("Weapon_Dagger_01"), Level, Expected)) ||
            !TestEqual(TEXT("Initial pointerless ARM contained"),
                Receptacle->GetContainedItemCount(), 1))
        {
            return false;
        }
        const FGridContainedReceptacleItem& Item = Receptacle->ContainedItems[0];
        AGridItemActor* ItemActor = Item.ItemActor.Get();
        if (!TestNotNull(TEXT("Contained actor exists"), ItemActor) ||
            !TestTrue(TEXT("Stable initial GUID was created"),
                Item.RuntimeObjectId.IsValid()) ||
            !TestEqual(TEXT("Contained ItemId preserved"),
                Item.ItemDefinitionId, FName(TEXT("Weapon_Dagger_01"))) ||
            !TestEqual(TEXT("Contained physical ARM level"),
                Item.ItemLevel, Level) ||
            !TestNull(TEXT("No legacy UObject stored on contained item"),
                Item.ItemDefinition.Get()) ||
            !TestNull(TEXT("No legacy UObject attached to contained actor"),
                ItemActor->GetItemDefinitionAsset()) ||
            !TestTrue(TEXT("Canonical presentation exists"),
                ItemActor->CanonicalWorldPresentation.IsValid()) ||
            !TestTrue(TEXT("Canonical mesh from visual catalogue"),
                ItemActor->MeshComponent->GetStaticMesh() == Expected.WorldMesh.LoadSynchronous()) ||
            !TestEqual(TEXT("Canonical persistent Niagara applied"),
                ItemActor->VisualEffectsComponent->GetConfiguredEffectCount(),
                Expected.VisualEffects.Effects.Num()) ||
            !TestEqual(TEXT("Canonical light configuration applied"),
                ItemActor->LightEmitterComponent->RuntimeConfig.bUsePointLight,
                Expected.LightEmitter.bUsePointLight))
        {
            return false;
        }

        for (const FName CanonicalTag : Expected.Tags)
        {
            if (!TestTrue(TEXT("Canonical item tags are queried by ItemId"),
                    Receptacle->ContainsItemTag(CanonicalTag)))
            {
                return false;
            }
        }
        if (!TestFalse(TEXT("Unknown tag is not inherited from an old definition"),
                Receptacle->ContainsItemTag(TEXT("ARM_CLEAN06_4X21_AbsentTag"))))
        {
            return false;
        }

        FGridRuntimeReceptacleState Saved;
        Receptacle->CaptureRuntimeReceptacleState(Saved);
        if (!TestEqual(TEXT("Exactly one item captured"),
                Saved.ContainedItems.Num(), 1) ||
            !TestEqual(TEXT("Stable GUID captured"),
                Saved.ContainedItems[0].ObjectId, Item.RuntimeObjectId) ||
            !TestEqual(TEXT("Fixed N1/N20 captured"),
                Saved.ContainedItems[0].ItemLevel, Level))
        {
            return false;
        }
    }

    AddInfo(TEXT("[ARM-CLEAN06.4X.21] Canonical ARM N1/N20 initial contents no longer "
                 "need old ItemDefinition; real serialized ARM N0 is rejected. "
                 "No native package or SaveGame written."));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X22PointerlessArmRestore,
    "Grimrock.RPG.ARM.CLEAN06_4X22.PointerlessCanonicalArmReceptacleRestore",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X22PointerlessArmRestore::RunTest(const FString&)
{
    ARMCLEAN064X21::FWorld F;
    if (!TestNotNull(TEXT("World"), F.World) ||
        !TestNotNull(TEXT("Registry"), F.Registry) ||
        !TestNotNull(TEXT("Runtime"), F.Runtime))
    {
        return false;
    }
    F.Runtime->RebuildLevel();
    AGridReceptacleActor* Receptacle =
        F.Runtime->FindRuntimeObjectActor<AGridReceptacleActor>(F.Ids[1]);
    if (!TestNotNull(TEXT("Canonical N20 receptacle exists"), Receptacle) ||
        !TestEqual(TEXT("One initial N20 item"), Receptacle->GetContainedItemCount(), 1))
    {
        return false;
    }

    FGridRuntimeReceptacleState Saved;
    Receptacle->CaptureRuntimeReceptacleState(Saved);
    if (!TestEqual(TEXT("Captured one N20 item"), Saved.ContainedItems.Num(), 1))
    {
        return false;
    }
    const FGridRuntimeItemState ItemState = Saved.ContainedItems[0];
    Receptacle->ForceClearRuntimeContents(false);

    FGridItemInstance Instance;
    Instance.RuntimeObjectId = ItemState.ObjectId;
    Instance.ItemDefinitionId = ItemState.ItemDefinitionId;
    Instance.ItemLevel = ItemState.ItemLevel;
    Instance.Quantity = ItemState.Quantity;
    AGridItemActor* Prebuilt = F.World->SpawnActor<AGridItemActor>();
    if (!TestNotNull(TEXT("Prebuilt actor"), Prebuilt) ||
        !TestTrue(TEXT("Restored ARM resolves without historical source"),
            Prebuilt->InitializeFromCanonicalItem(Instance, F.Registry)) ||
        !TestTrue(TEXT("Restore accepts canonical actor without reattaching old source"),
            Receptacle->RestoreRuntimeContainedItem(ItemState, Prebuilt)))
    {
        return false;
    }

    if (!TestEqual(TEXT("One item restored"), Receptacle->GetContainedItemCount(), 1))
    {
        return false;
    }
    const FGridContainedReceptacleItem& Item = Receptacle->ContainedItems[0];
    return TestNull(TEXT("No old DA_Weapon attached during restore"), Item.ItemDefinition.Get()) &&
        TestNull(TEXT("Restored actor remains canonical"), Prebuilt->GetItemDefinitionAsset()) &&
        TestTrue(TEXT("Full canonical world presentation retained"),
            Prebuilt->CanonicalWorldPresentation.IsValid()) &&
        TestEqual(TEXT("Stable GUID preserved"), Item.RuntimeObjectId, ItemState.ObjectId) &&
        TestEqual(TEXT("ItemId preserved"), Item.ItemDefinitionId, ItemState.ItemDefinitionId) &&
        TestEqual(TEXT("Physical N20 preserved"), Item.ItemLevel, 20) &&
        TestEqual(TEXT("State light preserved"), Item.bLightsEnabled, ItemState.bLightsEnabled);
}

#endif

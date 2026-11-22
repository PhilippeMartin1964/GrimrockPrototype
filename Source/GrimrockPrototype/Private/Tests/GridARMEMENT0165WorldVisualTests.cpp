#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GridCanonicalWeightTestWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Runtime/GridEditorPreviewComponent.h"
#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Items/GridItemSubsystem.h"

namespace ARMEMENT0165
{
    struct FPilot {const TCHAR* ItemId; const TCHAR* VisualId;};
    constexpr FPilot Pilots[] =
    {
        {TEXT("Weapon_Dagger_01"),TEXT("VIS_Dagger_01")},
        {TEXT("Weapon_ShortBow_01"),TEXT("VIS_ShortBow_01")},
        {TEXT("Weapon_RoundShield_01"),TEXT("VIS_RoundShield_01")}
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0165RuntimeMeshTest,
    "Grimrock.RPG.ARMEMENT01.6.World.RuntimeMeshAndLegacy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT0165RuntimeMeshTest::RunTest(const FString&)
{
    FGridCanonicalWeightTestWorld Fixture({});
    if (!TestNotNull(TEXT("Canonical game world"), Fixture.World) ||
        !TestNotNull(TEXT("Registered world ItemSystem"), Fixture.Scope.Registry))
        return false;
    for (const ARMEMENT0165::FPilot& Pilot : ARMEMENT0165::Pilots)
    {
        FGridResolvedItem Canonical;
        if (!TestTrue(TEXT("Real N1 art source resolves"),
            Fixture.Scope.Registry->ResolveItem(FName(Pilot.ItemId), 1, Canonical)))
            continue;
        FGridItemInstance Instance;
        Instance.RuntimeObjectId = FGuid::NewGuid();
        Instance.ItemDefinitionId = Canonical.ItemId;
        Instance.ItemLevel = 1;
        Instance.Quantity = 1;
        Instance.OwnerType = EGridItemOwnerType::World;
        AGridItemActor* Actor = Fixture.World->SpawnActor<AGridItemActor>();
        if (!TestNotNull(TEXT("Native pickup actor spawned"), Actor)) continue;
        if (!TestTrue(TEXT("Runtime actor reads canonical physical instance"),
            Actor->InitializeFromItemInstance(Instance, Fixture.Scope.Registry)))
            continue;
        UStaticMesh* Expected = Canonical.WorldMesh.LoadSynchronous();
        TestNotNull(TEXT("Authored canonical real world geometry"), Expected);
        TestEqual(TEXT("World actor renders exact canonical mesh"),
            Actor->MeshComponent->GetStaticMesh().Get(), Expected);
        TestTrue(TEXT("World art offset is canonically authored"),
            Actor->MeshComponent->GetRelativeTransform().Equals(Canonical.WorldVisualTransform));
        TestEqual(TEXT("Runtime GUID remains untouched"), Actor->GetRuntimeObjectId(), Instance.RuntimeObjectId);
        TestEqual(TEXT("Correct visual ID"), Canonical.VisualId, FName(Pilot.VisualId));
        if (Expected)
        {
            TestEqual(TEXT("StaticMesh retains its PBR material"),
                Actor->MeshComponent->GetMaterial(0), Expected->GetMaterial(0));
        }
    }
    FGridItemInstance Invalid;
    Invalid.RuntimeObjectId = FGuid::NewGuid();
    Invalid.ItemDefinitionId = TEXT("Weapon_Dagger_01");
    Invalid.ItemLevel = 0;
    Invalid.Quantity = 1;
    AGridItemActor* Rejected = Fixture.World->SpawnActor<AGridItemActor>();
    if (TestNotNull(TEXT("Invalid visual fixture actor"), Rejected))
    {
        TestFalse(TEXT("Invalid canonical N0 cannot borrow N1 world mesh"),
            Rejected->InitializeFromItemInstance(Invalid, Fixture.Scope.Registry));
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0165EditorPreviewTest,
    "Grimrock.RPG.ARMEMENT01.6.World.EditorPreviewSameMesh",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT0165EditorPreviewTest::RunTest(const FString&)
{
    FGridCanonicalWeightTestWorld Fixture({});
    if (!TestNotNull(TEXT("Real world fixture"), Fixture.World) ||
        !TestNotNull(TEXT("Single configured ItemSystem"), Fixture.Scope.Registry))
        return false;
    AGridLevelRuntimeActor* Runtime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
    if (!TestNotNull(TEXT("Runtime actor"), Runtime)) return false;
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
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(ARMEMENT0165::Pilots); ++Index)
    {
        const ARMEMENT0165::FPilot& Pilot = ARMEMENT0165::Pilots[Index];
        FGridLooseItemInstance& Placed = Level->LooseItemInstances.AddDefaulted_GetRef();
        Placed.InstanceId = FGuid::NewGuid();
        Placed.ItemId = FName(Pilot.ItemId);
        Placed.ItemLevel = 1;
        Placed.CellX = Index;
        Placed.CellY = 1;
    }
    UGridEditorPreviewComponent* Preview =
        Runtime->FindComponentByClass<UGridEditorPreviewComponent>();
    if (!TestNotNull(TEXT("Native 3D preview component"), Preview)) return false;
    Preview->Initialize(Runtime);
    Preview->RebuildPreviewObjects();
    for (const FGridLooseItemInstance& Placed : Level->LooseItemInstances)
    {
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("Same canonical visual source as runtime actor"),
            Fixture.Scope.Registry->ResolveItem(Placed.ItemId, 1, Item)))
            continue;
        AGridEditorPreviewObjectActor* Actor = nullptr;
        for (TActorIterator<AGridEditorPreviewObjectActor> It(Fixture.World); It; ++It)
            if ((*It)->ObjectId == Placed.InstanceId) { Actor = *It; break; }
        if (!TestNotNull(TEXT("One canonical editor preview actor"), Actor)) continue;
        TestEqual(TEXT("Preview uses exact runtime mesh"),
            Actor->MeshComponent->GetStaticMesh().Get(), Item.WorldMesh.LoadSynchronous());
        TestTrue(TEXT("Preview preserves authored world art transform"),
            Actor->MeshComponent->GetRelativeTransform().Equals(Item.WorldVisualTransform));
        TestEqual(TEXT("Preview is not collidable"),
            Actor->MeshComponent->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
    }
    Preview->ClearPreviewObjects();
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS

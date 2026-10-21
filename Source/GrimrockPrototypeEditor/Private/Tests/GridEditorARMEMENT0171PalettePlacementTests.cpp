#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/Combat/GridWorldItemVisualResolver.h"
#include "Runtime/GridEditorPreviewComponent.h"
#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace ARMEMENT0171Tests
{
    constexpr const TCHAR* PalettePath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default");
    constexpr const TCHAR* ItemFolder = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons");

    struct FPilot
    {
        const TCHAR* EntryId;
        const TCHAR* AssetName;
        const TCHAR* ItemId;
    };
    const FPilot Pilots[] =
    {
        {TEXT("Weapon_Dagger_01"), TEXT("DA_Weapon_Dagger_01"), TEXT("Weapon_Dagger_01")},
        {TEXT("Weapon_ShortBow_01"), TEXT("DA_Weapon_ShortBow_01"), TEXT("Weapon_ShortBow_01")},
        {TEXT("Weapon_RoundShield_01"), TEXT("DA_Weapon_RoundShield_01"), TEXT("Weapon_RoundShield_01")}
    };

    UGridItemDefinitionAsset* LoadPilot(const FPilot& Pilot)
    {
        const FString Path = FString::Printf(TEXT("%s/%s.%s"), ItemFolder, Pilot.AssetName, Pilot.AssetName);
        return LoadObject<UGridItemDefinitionAsset>(nullptr, *Path);
    }

    struct FWorldFixture
    {
        UWorld* World = nullptr;
        FWorldFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
                .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::EditorPreview, false,
                FName(*FString::Printf(TEXT("ARM0171_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
                Context.SetCurrentWorld(World);
            }
        }
        ~FWorldFixture()
        {
            if (!World) { return; }
            World->DestroyWorld(false);
            if (GEngine) { GEngine->DestroyWorldContext(World); }
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0171ProductionPaletteTest,
    "Grimrock.RPG.ARMEMENT01.7.Palette.ThreeDirectCollectibles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0171ProductionPaletteTest::RunTest(const FString&)
{
    UGridObjectPaletteAsset* Palette = LoadObject<UGridObjectPaletteAsset>(nullptr, ARMEMENT0171Tests::PalettePath);
    if (!TestNotNull(TEXT("Production palette exists"), Palette)) { return false; }
    TArray<FGridWorldObjectDefinitionValidationMessage> Messages;
    TestTrue(TEXT("Production palette is valid"), Palette->ValidatePalette(Messages));
    for (const ARMEMENT0171Tests::FPilot& Pilot : ARMEMENT0171Tests::Pilots)
    {
        UGridItemDefinitionAsset* Item = ARMEMENT0171Tests::LoadPilot(Pilot);
        if (!TestNotNull(TEXT("Real item asset exists"), Item)) { continue; }
        const FName Id(Pilot.EntryId);
        const FGridObjectPaletteEntry* Entry = Palette->FindEntryById(Id);
        if (!TestNotNull(TEXT("Pilot has a palette entry"), Entry)) { continue; }
        const int32 Count = Palette->Entries.FilterByPredicate(
            [Id](const FGridObjectPaletteEntry& Candidate) { return Candidate.EntryId == Id; }).Num();
        TestEqual(TEXT("EntryId occurs once"), Count, 1);
        TestEqual(TEXT("Category is Items"), Entry->PaletteCategory, FName(TEXT("Items")));
        TestEqual(TEXT("Entry stores the canonical item"), Entry->DefaultItemDefinition.Get(), Item);
        TestTrue(TEXT("No companion WorldObjectDefinition"), Entry->DefaultWorldObjectDefinition == nullptr);
        TestTrue(TEXT("No duplicate palette Icon"), Entry->Icon == nullptr);
        TestTrue(TEXT("Direct collectible palette entry is valid"), Entry->IsValidEntry());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0171PlacementPreviewTest,
    "Grimrock.RPG.ARMEMENT01.7.Palette.PlaceAndPreviewRealItems",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0171PlacementPreviewTest::RunTest(const FString&)
{
    ARMEMENT0171Tests::FWorldFixture Fixture;
    if (!TestNotNull(TEXT("Transient editor world"), Fixture.World)) { return false; }
    UGridObjectPaletteAsset* Palette = LoadObject<UGridObjectPaletteAsset>(nullptr, ARMEMENT0171Tests::PalettePath);
    if (!TestNotNull(TEXT("Real palette"), Palette)) { return false; }
    AGridLevelEditorActor* Editor = Fixture.World->SpawnActor<AGridLevelEditorActor>();
    AGridLevelRuntimeActor* Runtime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
    if (!TestNotNull(TEXT("Editor actor"), Editor) || !TestNotNull(TEXT("Runtime preview actor"), Runtime))
    {
        return false;
    }
    UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Editor);
    Level->Width = 4;
    Level->Height = 4;
    Level->EnsureCellCount();
    for (FGridLevelCellData& Cell : Level->Cells)
    {
        Cell.CellType = EGridCellType::Floor;
        Cell.bBlocksOccupancy = false;
    }
    Editor->LevelAsset = Level;
    Editor->ObjectPalette = Palette;
    Runtime->LevelAsset = Level;

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(ARMEMENT0171Tests::Pilots); ++Index)
    {
        const ARMEMENT0171Tests::FPilot& Pilot = ARMEMENT0171Tests::Pilots[Index];
        UGridItemDefinitionAsset* Definition = ARMEMENT0171Tests::LoadPilot(Pilot);
        if (!TestNotNull(TEXT("Pilot Definition loads"), Definition)) { continue; }
        Editor->SelectedCellX = Index;
        Editor->SelectedCellY = 1;
        Editor->SelectedEdge = EGridEdge::None;
        if (!TestTrue(TEXT("Select palette entry via existing editor command"),
            Editor->ApplyPaletteEntry(FName(Pilot.EntryId)))) { continue; }
        TestEqual(TEXT("Paint type is collectible Item"), Editor->PaintObjectType, EGridLevelObjectType::Item);
        TestTrue(TEXT("No world-object definition selected"), Editor->WorldObjectDefinitionId.IsNone());
        Editor->PlaceSelectedObject();
        if (Level->LooseItemInstances.Num() <= Index) { continue; }
        const FGridLooseItemInstance& Placed = Level->LooseItemInstances[Index];
        TestEqual(TEXT("Canonical definition is persisted on placement"), Placed.ItemDefinition.Get(), Definition);
        TestEqual(TEXT("Palette identity is persisted on placement"), Placed.PaletteEntryId, FName(Pilot.EntryId));
        TestEqual(TEXT("Correct grid X"), Placed.CellX, Index);
        TestEqual(TEXT("Correct grid Y"), Placed.CellY, 1);
        TestEqual(TEXT("Default is floor placement"), Placed.SurfaceSide, EGridEdge::None);
        TestTrue(TEXT("Placement identity is valid"), Placed.InstanceId.IsValid());
    }
    TestEqual(TEXT("Three real items placed"), Level->LooseItemInstances.Num(), 3);
    TestTrue(TEXT("No companion world objects created"), Level->WorldObjectInstances.IsEmpty());
    TestEqual(TEXT("Only three typed placements"), Level->GetTypedPlacementCount(), 3);

    UGridEditorPreviewComponent* PreviewComponent = Runtime->FindComponentByClass<UGridEditorPreviewComponent>();
    if (!TestNotNull(TEXT("Existing preview component"), PreviewComponent)) { return false; }
    PreviewComponent->Initialize(Runtime);
    PreviewComponent->RebuildPreviewObjects();
    for (const FGridLooseItemInstance& Placed : Level->LooseItemInstances)
    {
        if (!TestNotNull(TEXT("Placed item definition"), Placed.ItemDefinition.Get())) { continue; }
        const FGridResolvedWorldItemMesh Visual = FGridWorldItemVisualResolver::Resolve(*Placed.ItemDefinition);
        if (!TestNotNull(TEXT("Resolved realistic mesh"), Visual.Mesh)) { continue; }
        AGridEditorPreviewObjectActor* Preview = nullptr;
        for (TActorIterator<AGridEditorPreviewObjectActor> It(Fixture.World); It; ++It)
        {
            if ((*It)->ObjectId == Placed.InstanceId)
            {
                Preview = *It;
                break;
            }
        }
        if (!TestNotNull(TEXT("Placed real item has editor preview"), Preview)) { continue; }
        TestEqual(TEXT("Preview mesh is real item mesh"), Preview->MeshComponent->GetStaticMesh().Get(), Visual.Mesh);
        TestTrue(TEXT("Preview retains visual transform"),
            Preview->MeshComponent->GetRelativeTransform().Equals(Visual.RelativeTransform));
        TestEqual(TEXT("Mesh PBR material slot retained"), Preview->MeshComponent->GetMaterial(0), Visual.Mesh->GetMaterial(0));
        TestEqual(TEXT("Preview has no collision"), Preview->MeshComponent->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
    }
    PreviewComponent->ClearPreviewObjects();
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

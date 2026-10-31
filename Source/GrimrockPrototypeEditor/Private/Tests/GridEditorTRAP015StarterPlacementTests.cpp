#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Runtime/GridEditorPreviewComponent.h"
#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace TRAP015
{
    constexpr const TCHAR* PalettePath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default");

    struct FWorldFixture
    {
        UWorld* World = nullptr;
        FWorldFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false)
                .ShouldSimulatePhysics(false).SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::EditorPreview, false,
                FName(*FString::Printf(TEXT("TRAP015_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
                Context.SetCurrentWorld(World);
            }
        }
        ~FWorldFixture()
        {
            if (!World) return;
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTRAP015StarterPaletteAndPlacement,
    "Grimrock.Editor.TRAP01.5.StarterPaletteAndPlacement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTRAP015StarterPaletteAndPlacement::RunTest(const FString&)
{
    TRAP015::FWorldFixture Fixture;
    if (!TestNotNull(TEXT("Editor preview world"), Fixture.World)) return false;
    UGridObjectPaletteAsset* Palette =
        LoadObject<UGridObjectPaletteAsset>(nullptr, TRAP015::PalettePath);
    if (!TestNotNull(TEXT("Canonical grid object palette"), Palette)) return false;

    AGridLevelEditorActor* Editor = Fixture.World->SpawnActor<AGridLevelEditorActor>();
    if (!TestNotNull(TEXT("Editor actor"), Editor)) return false;
    Editor->ObjectPalette = Palette;

    FString Error;
    if (!TestTrue(TEXT("Starter hazards can be provisioned"), Editor->EnsureDungeonTrapDefinitions(Error)))
    {
        AddError(FString::Printf(TEXT("TRAP01.5 provisioning: %s"), *Error));
        return false;
    }
    const int32 EntriesAfterFirstProvision = Palette->Entries.Num();
    TestTrue(TEXT("Provisioning twice never creates duplicate entries"),
        Editor->EnsureDungeonTrapDefinitions(Error));
    TestEqual(TEXT("Palette entry count remains stable"), Palette->Entries.Num(),
        EntriesAfterFirstProvision);

    UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Editor);
    Level->Width = 8;
    Level->Height = 8;
    Level->EnsureCellCount();
    for (FGridLevelCellData& Cell : Level->Cells)
    {
        Cell.CellType = EGridCellType::Floor;
        Cell.bHasCeiling = true;
        Cell.bBlocksOccupancy = false;
    }
    Editor->LevelAsset = Level;

    struct FExpectedTrap
    {
        const TCHAR* Id;
        EGridObjectPlacementKind Surface;
        EGridDamageType Damage;
        EGridEdge Edge;
        int32 X;
    };
    static const FExpectedTrap Expected[] = {
        { TEXT("Trap_Floor_Spikes_01"), EGridObjectPlacementKind::Floor,
            EGridDamageType::Physical, EGridEdge::None, 1 },
        { TEXT("Trap_Wall_Darts_01"), EGridObjectPlacementKind::Wall,
            EGridDamageType::Physical, EGridEdge::North, 2 },
        { TEXT("Trap_Ceiling_Fire_01"), EGridObjectPlacementKind::Ceiling,
            EGridDamageType::Fire, EGridEdge::None, 3 }
    };

    for (const FExpectedTrap& Spec : Expected)
    {
        const FName Id(Spec.Id);
        const FGridObjectPaletteEntry* Entry = Palette->FindEntryById(Id);
        if (!TestNotNull(TEXT("Starter hazard exists in palette"), Entry)) continue;
        const UGridWorldObjectDefinitionAsset* Definition =
            Entry->DefaultWorldObjectDefinition;
        if (!TestNotNull(TEXT("Starter hazard has a definition"), Definition)) continue;
        TestEqual(TEXT("Hazards category"), Entry->PaletteCategory, FName(TEXT("Hazards")));
        TestEqual(TEXT("Trigger gameplay type"), Definition->SupportedType,
            EGridLevelObjectType::Trigger);
        TestEqual(TEXT("Definition ID remains stable"), Definition->DefinitionId, Id);
        TestEqual(TEXT("Placement surface"), Definition->PlacementSurface, Spec.Surface);
        TestEqual(TEXT("Damage type"), Definition->DungeonTrap.DamageType, Spec.Damage);
        TestTrue(TEXT("Dungeon trap opt-in"), Definition->DungeonTrap.bIsDungeonTrap);
        TestTrue(TEXT("Hidden by default"), Definition->DungeonTrap.bInitiallyHidden);
        TestTrue(TEXT("Disarmable"), Definition->bCanBeDisarmed);
        TestFalse(TEXT("Does not erase/replace cell wall"), Definition->bReplacesStandardWall);
        TestNotNull(TEXT("Reveal-ready preview mesh"), Definition->StaticPart.Mesh.Get());

        TArray<FGridWorldObjectDefinitionValidationMessage> Messages;
        TestTrue(TEXT("Starter definition is valid"), Definition->ValidateDefinition(Messages));
        Editor->SelectedCellX = Spec.X;
        Editor->SelectedCellY = 2;
        Editor->SelectedEdge = Spec.Edge;
        if (!TestTrue(TEXT("Normal palette action selects trap"), Editor->ApplyPaletteEntry(Id)))
        {
            continue;
        }
        TestEqual(TEXT("Painting uses existing Trigger object type"),
            Editor->PaintObjectType, EGridLevelObjectType::Trigger);
        Editor->PlaceSelectedObject();
    }
    TestEqual(TEXT("Three world-object instances authored"), Level->WorldObjectInstances.Num(), 3);
    TestEqual(TEXT("No legacy placement created"), Level->GetTypedPlacementCount(), 3);
    for (int32 i = 0; i < Level->WorldObjectInstances.Num(); ++i)
    {
        const FGridWorldObjectInstance& Instance = Level->WorldObjectInstances[i];
        TestTrue(TEXT("Instance has stable valid GUID"), Instance.InstanceId.IsValid());
        TestEqual(TEXT("Instance stays in one asset's WorldObjectInstances"),
            Instance.WorldObjectDefinitionId, FName(Expected[i].Id));
        TestEqual(TEXT("Correct X cell"), Instance.CellX, Expected[i].X);
        TestEqual(TEXT("Correct Y cell"), Instance.CellY, 2);
        TestEqual(TEXT("Wall-only edge orientation persisted"),
            Instance.WallSide, Expected[i].Edge);
    }

    // Explicit regression: the old Trigger floor-only rule remains valid for
    // ordinary puzzle logic, but not for the TRAP01.5 hazard opt-in.
    UGridWorldObjectDefinitionAsset* LegacyTrigger =
        NewObject<UGridWorldObjectDefinitionAsset>(Editor);
    LegacyTrigger->DefinitionId = TEXT("TRAP015_LegacyLogicTrigger");
    LegacyTrigger->SupportedType = EGridLevelObjectType::Trigger;
    LegacyTrigger->PlacementSurface = EGridObjectPlacementKind::Wall;
    TArray<FGridWorldObjectDefinitionValidationMessage> LegacyValidation;
    TestFalse(TEXT("Ordinary logical Trigger cannot be placed on a wall"),
        LegacyTrigger->ValidateDefinition(LegacyValidation));
    TestTrue(TEXT("Legacy floor-only validation is explicit"),
        LegacyValidation.ContainsByPredicate(
            [](const FGridWorldObjectDefinitionValidationMessage& Message)
            {
                return Message.Severity ==
                        EGridWorldObjectDefinitionValidationSeverity::Error &&
                    Message.Message.Contains(TEXT("Ordinary Trigger Placement Surface"));
            }));

    // Editor preview must show all hazards, even though runtime hides them.
    AGridLevelRuntimeActor* Preview = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
    if (TestNotNull(TEXT("Runtime preview actor"), Preview))
    {
        Preview->LevelAsset = Level;
        for (const FExpectedTrap& Spec : Expected)
        {
            const FGridObjectPaletteEntry* Entry =
                Palette->FindEntryById(FName(Spec.Id));
            if (Entry) Preview->WorldObjectDefinitions.Add(Entry->DefaultWorldObjectDefinition);
        }
        UGridEditorPreviewComponent* PreviewComponent =
            Preview->FindComponentByClass<UGridEditorPreviewComponent>();
        if (TestNotNull(TEXT("Editor preview component"), PreviewComponent))
        {
            PreviewComponent->Initialize(Preview);
            PreviewComponent->RebuildPreviewObjects();
            int32 VisiblePreviewMarkers = 0;
            for (TActorIterator<AGridEditorPreviewObjectActor> It(Fixture.World); It; ++It)
            {
                if ((*It)->MeshComponent && (*It)->MeshComponent->GetStaticMesh())
                {
                    ++VisiblePreviewMarkers;
                }
            }
            TestEqual(TEXT("All three hazards remain visible in editor preview"),
                VisiblePreviewMarkers, 3);
        }
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

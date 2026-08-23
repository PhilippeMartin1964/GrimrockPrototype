#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "RPG/RPGStoryCompanionAsset.h"

namespace GridEditorMON204StoryCompanionPlacementTests
{
    struct FTestWorld
    {
        UWorld* World = nullptr;

        FTestWorld ()
        {
            const UWorld::InitializationValues Values =
                UWorld::InitializationValues ()
                    .AllowAudioPlayback (false)
                    .RequiresHitProxies (false)
                    .CreatePhysicsScene (false)
                    .CreateNavigation (false)
                    .CreateAISystem (false)
                    .ShouldSimulatePhysics (false)
                    .SetTransactional (false);

            World = UWorld::CreateWorld (
                EWorldType::EditorPreview,
                false,
                FName (*FString::Printf (
                    TEXT ("MON2045_%s"),
                    *FGuid::NewGuid ().ToString (EGuidFormats::Digits))),
                nullptr,
                true,
                ERHIFeatureLevel::Num,
                &Values);
            if (!World || !GEngine)
            {
                return;
            }

            FWorldContext& Context =
                GEngine->CreateNewWorldContext (EWorldType::EditorPreview);
            Context.SetCurrentWorld (World);
        }

        ~FTestWorld ()
        {
            if (!World)
            {
                return;
            }

            World->DestroyWorld (false);
            if (GEngine)
            {
                GEngine->DestroyWorldContext (World);
            }
        }
    };
}

using namespace GridEditorMON204StoryCompanionPlacementTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridEditorMON2045StoryCompanionPalettePlacementTest,
    "Grimrock.MON20.4.RecruitmentUI.PalettePlacement",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridEditorMON2045StoryCompanionPalettePlacementTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    FTestWorld TestWorld;
    if (!TestWorld.World)
    {
        AddError (TEXT ("Unable to create MON20.4.5 editor test world."));
        return false;
    }

    AGridLevelEditorActor* EditorActor =
        TestWorld.World->SpawnActor<AGridLevelEditorActor> ();
    TestNotNull (TEXT ("Grid editor actor spawns"), EditorActor);
    if (!EditorActor)
    {
        return false;
    }

    UGridLevelAsset* Level =
        NewObject<UGridLevelAsset> (EditorActor);
    Level->Width = 1;
    Level->Height = 1;
    Level->EnsureCellCount ();
    Level->Cells[0].CellType = EGridCellType::Floor;

    UGridObjectPaletteAsset* Palette =
        NewObject<UGridObjectPaletteAsset> (EditorActor);
    UGridObjectArchetypeAsset* CompanionArchetype =
        NewObject<UGridObjectArchetypeAsset> (Palette);
    CompanionArchetype->ArchetypeId = TEXT ("StoryCompanion_Recruit");
    CompanionArchetype->DisplayName = FText::FromString (TEXT ("Story Companion"));
    CompanionArchetype->SupportedType = EGridLevelObjectType::StoryCompanion;
    CompanionArchetype->PlacementKind = EGridObjectPlacementKind::Center;

    URPGStoryCompanionAsset* CompanionDefinition =
        NewObject<URPGStoryCompanionAsset> (Palette);

    FGridObjectPaletteEntry Entry;
    Entry.EntryId = TEXT ("StoryCompanion_TestCandidate");
    Entry.DefaultArchetype = CompanionArchetype;
    Entry.DefaultStoryCompanionDefinition = CompanionDefinition;
    Palette->Entries.Add (Entry);

    EditorActor->LevelAsset = Level;
    EditorActor->ObjectPalette = Palette;
    EditorActor->SelectedCellX = 0;
    EditorActor->SelectedCellY = 0;

    TestTrue (
        TEXT ("Story companion palette entry can be selected"),
        EditorActor->ApplyPaletteEntry (Entry.EntryId));
    EditorActor->PlaceSelectedObject ();

    TestEqual (
        TEXT ("Palette placement creates one level object"),
        Level->Objects.Num (),
        1);
    if (Level->Objects.Num () == 1)
    {
        const FGridLevelObjectData& Placed = Level->Objects[0];
        TestTrue (
            TEXT ("Placed object keeps StoryCompanion type"),
            Placed.Type == EGridLevelObjectType::StoryCompanion);
        TestTrue (
            TEXT ("Placed object copies the palette companion definition"),
            Placed.StoryCompanionDefinition == CompanionDefinition);
        TestEqual (
            TEXT ("Placed object keeps the source palette entry"),
            Placed.PaletteEntryId,
            Entry.EntryId);
        TestEqual (
            TEXT ("Placed object uses the story companion archetype"),
            Placed.ArchetypeId,
            CompanionArchetype->ArchetypeId);
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

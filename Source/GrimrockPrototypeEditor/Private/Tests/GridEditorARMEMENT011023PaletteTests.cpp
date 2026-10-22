#if WITH_DEV_AUTOMATION_TESTS
#include "ARMEMENT/GridWeaponFireBombPalettePlan.h"
#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace ARMEMENT011023Tests
{
    static constexpr const TCHAR* PalettePath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default");

    // ARM014 art/definition is currently local only. The tests also work
    // against a clean clone without the user's new ARM014 .uasset.
    bool ResolveAvailable(TArray<ARMEMENT011023::FResolved>& Out)
    {
        for (const ARMEMENT011023::FSpec& Spec : ARMEMENT011023::Specs)
        {
            if (FName(Spec.EntryId) == FName(TEXT("Weapon_StraightSword_01")))
            {
                continue;
            }
            ARMEMENT011023::FResolved& Target = Out.AddDefaulted_GetRef();
            Target.Spec = &Spec;
            Target.Item = LoadObject<UGridItemDefinitionAsset>(nullptr, Spec.ObjectPath);
            if (!IsValid(Target.Item) ||
                Target.Item->ItemDefinitionId != FName(Spec.ItemDefinitionId) ||
                !Target.Item->IsValidDefinition())
            {
                return false;
            }
        }
        return Out.Num() == 4;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT011023IdempotenceTest,
    "Grimrock.RPG.ARMEMENT01.10.Palette.IdempotentPreservesExisting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT011023IdempotenceTest::RunTest(const FString&)
{
    UGridObjectPaletteAsset* Source =
        LoadObject<UGridObjectPaletteAsset>(nullptr, ARMEMENT011023Tests::PalettePath);
    if (!TestNotNull(TEXT("Production palette available"), Source)) return false;

    TArray<ARMEMENT011023::FResolved> Targets;
    if (!TestTrue(TEXT("Three existing pilot items and bomb definitions valid"),
        ARMEMENT011023Tests::ResolveAvailable(Targets))) return false;

    const int32 OriginalCount = Source->Entries.Num();
    TArray<FGridObjectPaletteEntry> Proposal;
    int32 Added = 0, Moved = 0;
    FString Error;
    if (!TestTrue(TEXT("Builds safe proposal"),
        ARMEMENT011023::Propose(Source->Entries, Targets, Proposal, Added, Moved, Error)))
    {
        AddError(Error);
        return false;
    }
    TestEqual(TEXT("Original entry count not touched"), Source->Entries.Num(), OriginalCount);
    TestEqual(TEXT("Exactly the intended entries added"), Proposal.Num(), OriginalCount + Added);

    UGridObjectPaletteAsset* Copy = NewObject<UGridObjectPaletteAsset>();
    Copy->Entries = Proposal;
    TArray<FGridWorldObjectDefinitionValidationMessage> Messages;
    TestTrue(TEXT("Complete proposed palette validates"), Copy->ValidatePalette(Messages));
    for (const ARMEMENT011023::FResolved& Target : Targets)
    {
        const FGridObjectPaletteEntry* Entry = Copy->FindEntryById(FName(Target.Spec->EntryId));
        if (!TestNotNull(TEXT("Expected direct item entry"), Entry)) continue;
        TestEqual(TEXT("Expected category"), Entry->PaletteCategory, FName(Target.Spec->Category));
        TestEqual(TEXT("Canonical definition"), Entry->DefaultItemDefinition.Get(), Target.Item);
        TestEqual(TEXT("Gameplay type still Item"), Entry->GetEffectiveObjectType(), EGridLevelObjectType::Item);
        TestNull(TEXT("No world-object wrapper"), Entry->DefaultWorldObjectDefinition.Get());
        TestNull(TEXT("No duplicated icon"), Entry->Icon.Get());
    }

    TArray<FGridObjectPaletteEntry> Again;
    int32 AddedAgain = 0, MovedAgain = 0;
    TestTrue(TEXT("Second proposal succeeds"),
        ARMEMENT011023::Propose(Proposal, Targets, Again, AddedAgain, MovedAgain, Error));
    TestEqual(TEXT("No duplicate addition"), AddedAgain, 0);
    TestEqual(TEXT("No repeated category migration"), MovedAgain, 0);
    TestEqual(TEXT("Entry count stable"), Again.Num(), Proposal.Num());

    const FGridObjectPaletteEntry* Bomb = Copy->FindEntryById(TEXT("Item_Bomb_Fire"));
    if (TestNotNull(TEXT("Bomb exists for conflict test"), Bomb))
    {
        TArray<FGridObjectPaletteEntry> Conflict = Proposal;
        FGridObjectPaletteEntry Alias = *Bomb;
        Alias.EntryId = TEXT("Item_Bomb_Fire_UnrelatedAlias");
        Conflict.Add(Alias);
        TArray<FGridObjectPaletteEntry> Rejected;
        int32 A = 0, M = 0;
        TestFalse(TEXT("Second identity for same bomb rejected"),
            ARMEMENT011023::Propose(Conflict, Targets, Rejected, A, M, Error));
        TestEqual(TEXT("Conflict input remains intact"), Conflict.Num(), Proposal.Num() + 1);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT011023BombPlacementTest,
    "Grimrock.RPG.ARMEMENT01.10.Palette.FireBombDirectPlacement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT011023BombPlacementTest::RunTest(const FString&)
{
    UGridObjectPaletteAsset* Source =
        LoadObject<UGridObjectPaletteAsset>(nullptr, ARMEMENT011023Tests::PalettePath);
    if (!TestNotNull(TEXT("Production palette available"), Source)) return false;

    TArray<ARMEMENT011023::FResolved> Targets;
    if (!TestTrue(TEXT("Production collectible definitions load"),
        ARMEMENT011023Tests::ResolveAvailable(Targets))) return false;

    TArray<FGridObjectPaletteEntry> Proposal;
    int32 Added = 0, Moved = 0;
    FString Error;
    if (!TestTrue(TEXT("Transient palette constructed"),
        ARMEMENT011023::Propose(Source->Entries, Targets, Proposal, Added, Moved, Error)))
    {
        AddError(Error);
        return false;
    }
    UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>();
    Palette->Entries = Proposal;
    const FGridObjectPaletteEntry* Bomb = Palette->FindEntryById(TEXT("Item_Bomb_Fire"));
    if (!TestNotNull(TEXT("Direct fire bomb in proposed palette"), Bomb)) return false;
    TestEqual(TEXT("Fire bomb stays in Items"), Bomb->PaletteCategory, FName(TEXT("Items")));

    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
        .SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false,
        FName(*FString::Printf(TEXT("ARM011023_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
        nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Transient world exists"), World)) return false;
    if (GEngine)
    {
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
        Context.SetCurrentWorld(World);
    }
    AGridLevelEditorActor* Editor = World->SpawnActor<AGridLevelEditorActor>();
    if (TestNotNull(TEXT("Grid editor exists"), Editor))
    {
        UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Editor);
        Level->Width = 2;
        Level->Height = 2;
        Level->EnsureCellCount();
        for (FGridLevelCellData& Cell : Level->Cells)
        {
            Cell.CellType = EGridCellType::Floor;
            Cell.bBlocksOccupancy = false;
        }
        Editor->LevelAsset = Level;
        Editor->ObjectPalette = Palette;
        Editor->SelectedCellX = 0;
        Editor->SelectedCellY = 0;
        Editor->SelectedEdge = EGridEdge::None;
        if (TestTrue(TEXT("Bomb selectable via Grid Editor"),
            Editor->ApplyPaletteEntry(TEXT("Item_Bomb_Fire"))))
        {
            TestEqual(TEXT("Painting treats bomb as Item"),
                Editor->PaintObjectType, EGridLevelObjectType::Item);
            Editor->PlaceSelectedObject();
            if (TestEqual(TEXT("One collectible placed"), Level->LooseItemInstances.Num(), 1))
            {
                const FGridLooseItemInstance& Placed = Level->LooseItemInstances[0];
                TestEqual(TEXT("Correct persistent palette entry id"),
                    Placed.PaletteEntryId, FName(TEXT("Item_Bomb_Fire")));
                TestEqual(TEXT("Correct ItemDefinition persists"),
                    Placed.ItemDefinition.Get(), Bomb->DefaultItemDefinition.Get());
            }
        }
    }
    World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

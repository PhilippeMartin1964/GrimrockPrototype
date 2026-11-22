#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "ARMEMENT/GridWeaponPipelinePalette.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

namespace WeaponPipelineTests
{
    struct FCanonicalRegistry
    {
        TStrongObjectPtr<UGameInstance> GI{NewObject<UGameInstance>(GetTransientPackage())};
        TStrongObjectPtr<UGridItemSubsystem> Registry{NewObject<UGridItemSubsystem>(GI.Get())};
        bool bReady = false;
        FCanonicalRegistry()
        {
            UGridItemSystemAsset* Root = GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
            TArray<FString> Errors;
            bReady = IsValid(Root) && Registry.IsValid() && Registry->RegisterItemSystem(Root, Errors);
        }
    };
    constexpr const TCHAR* PalettePath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default");

    bool LegacyCategory(const FString& Archetype)
    {
        return Archetype == TEXT("ARM001") || Archetype == TEXT("ARM045") ||
            Archetype == TEXT("ARM075");
    }

    bool Read(TArray<FGridWeaponPipelineRow>& Rows, FAutomationTestBase& Test)
    {
        FString Error;
        return Test.TestTrue(TEXT("Generic production manifest valid"),
            FGridWeaponPipelineManifest::Load(
                FGridWeaponPipelineManifest::DefaultFile(), Rows, Error));
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPipelineManifestContracts,
    "Grimrock.RPG.ARMEMENT.Industrial.ManifestContracts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponPipelineManifestContracts::RunTest(const FString&)
{
    TArray<FGridWeaponPipelineRow> Rows;
    if (!WeaponPipelineTests::Read(Rows, *this))
        return false;
    TestTrue(TEXT("At least eight historical production rows"), Rows.Num() >= 8);
    const FGridWeaponPipelineRow* Assassin =
        FGridWeaponPipelineManifest::Find(Rows, TEXT("ARM002"));
    if (!TestNotNull(TEXT("ARM002 is represented"), Assassin))
        return false;
    TestEqual(TEXT("Existing ARM002 item ID"), Assassin->ItemDefinitionId,
        FString(TEXT("Weapon_AssassinDagger_01")));
    TestEqual(TEXT("Existing ARM002 visual ID"), Assassin->VisualId,
        FString(TEXT("VIS_AssassinDagger_01")));
    TestEqual(TEXT("ARM002 is an actual production row"), Assassin->ProductionState,
        FString(TEXT("Active")));

    const FGridWeaponPipelineRow* Axe =
        FGridWeaponPipelineManifest::Find(Rows, TEXT("ARM031"));
    if (!TestNotNull(TEXT("Two-hand archetype included"), Axe))
        return false;
    TestEqual(TEXT("Axe is two-handed"), Axe->BuildKind, FString(TEXT("MeleeTwoHand")));

    const FGridWeaponPipelineRow* Bow =
        FGridWeaponPipelineManifest::Find(Rows, TEXT("ARM045"));
    const FGridWeaponPipelineRow* Shield =
        FGridWeaponPipelineManifest::Find(Rows, TEXT("ARM075"));
    if (!TestNotNull(TEXT("Ranged pilot included"), Bow) ||
        !TestNotNull(TEXT("Passive shield included"), Shield))
        return false;
    TestEqual(TEXT("Bow routed to ranged"), Bow->BuildKind, FString(TEXT("Ranged")));
    TestEqual(TEXT("Shield routed to passive"), Shield->BuildKind,
        FString(TEXT("ShieldPassive")));
    TestTrue(TEXT("Passive shield has no fictitious AttackId"), Shield->AttackId.IsEmpty());

    const FGridWeaponPipelineRow* Stiletto =
        FGridWeaponPipelineManifest::Find(Rows, TEXT("ARM003"));
    if (Stiletto)
    {
        TestEqual(TEXT("Produced Stiletto declared active"), Stiletto->ProductionState,
            FString(TEXT("Active")));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPipelineWorkbookContracts,
    "Grimrock.RPG.ARMEMENT.Industrial.WorkbookN1Contracts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponPipelineWorkbookContracts::RunTest(const FString&)
{
    WeaponPipelineTests::FCanonicalRegistry Canonical;
    if (!TestTrue(TEXT("Unique canonical registry validates"), Canonical.bReady))
        return false;
    TArray<FGridWeaponPipelineRow> Rows;
    if (!WeaponPipelineTests::Read(Rows, *this)) return false;
    int32 Active = 0, Checked = 0;
    for (const FGridWeaponPipelineRow& Row : Rows)
    {
        if (Row.ProductionState != TEXT("Active")) continue;
        ++Active;
        FGridResolvedItem N1, N20;
        if (!TestTrue(*FString::Printf(TEXT("%s N1 canonical workbook row"), *Row.ArchetypeId),
            Canonical.Registry->ResolveItem(FName(Row.ItemDefinitionId), 1, N1)))
            continue;
        if (!TestTrue(*FString::Printf(TEXT("%s N20 canonical workbook row"), *Row.ArchetypeId),
            Canonical.Registry->ResolveItem(FName(Row.ItemDefinitionId), 20, N20)))
            continue;
        ++Checked;
        TestTrue(TEXT("Both profiles are ARM weapons"), N1.bIsArmament && N20.bIsArmament);
        TestEqual(TEXT("One authored archetype identity"), N1.ArchetypeId, FName(Row.ArchetypeId));
        TestEqual(TEXT("Archetype remains stable across 20 levels"), N20.ArchetypeId, N1.ArchetypeId);
        TestTrue(TEXT("N1/N20 weight finite and physical"),
            FMath::IsFinite(N1.WeightKg) && N1.WeightKg >= 0.f &&
            FMath::IsFinite(N20.WeightKg) && N20.WeightKg >= 0.f);
        TestEqual(TEXT("Passive N1 still has no dice"), !N1.Weapon.DamageDice.bEnabled, Row.IsPassive());
        TestEqual(TEXT("Passive N20 still has no dice"), !N20.Weapon.DamageDice.bEnabled, Row.IsPassive());
    }
    TestEqual(TEXT("All active rows resolved N1 and N20"), Checked, Active);
    TestTrue(TEXT("Full production manifest contains 22 authored ARM"), Active == 22);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPipelineExistingContracts,
    "Grimrock.RPG.ARMEMENT.Industrial.ExistingItemsAndVisuals",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponPipelineExistingContracts::RunTest(const FString&)
{
    WeaponPipelineTests::FCanonicalRegistry Canonical;
    UGridObjectPaletteAsset* Palette =
        LoadObject<UGridObjectPaletteAsset>(nullptr, WeaponPipelineTests::PalettePath);
    if (!TestTrue(TEXT("Real canonical ItemSystem"), Canonical.bReady) ||
        !TestNotNull(TEXT("Production palette"), Palette))
        return false;
    const bool bPaletteDirty = Palette->GetOutermost()->IsDirty();
    TArray<FGridWorldObjectDefinitionValidationMessage> Errors;
    TestTrue(TEXT("Production palette validates"), Palette->ValidatePalette(Errors));

    TArray<FGridWeaponPipelineRow> Rows;
    if (!WeaponPipelineTests::Read(Rows, *this)) return false;
    TSet<FName> SeenItemIds, SeenArchetypeIds;
    int32 Active = 0;
    for (const FGridWeaponPipelineRow& Row : Rows)
    {
        if (Row.ProductionState != TEXT("Active")) continue;
        ++Active;
        const FName Id(*Row.ItemDefinitionId);
        const FName Archetype(*Row.ArchetypeId);
        TestFalse(TEXT("No duplicate canonical ItemId"), SeenItemIds.Contains(Id));
        TestFalse(TEXT("No duplicate canonical archetype"), SeenArchetypeIds.Contains(Archetype));
        SeenItemIds.Add(Id);
        SeenArchetypeIds.Add(Archetype);
        FGridResolvedItem Item;
        if (!TestTrue(*FString::Printf(TEXT("%s N1 resolves canonically"), *Row.ArchetypeId),
            Canonical.Registry->ResolveItem(Id, 1, Item))) continue;
        TestTrue(TEXT("N1 is ARM and physically valid"), Item.IsValid() && Item.bIsArmament);
        TestEqual(TEXT("Exact canonical ItemId"), Item.ItemId, Id);
        TestEqual(TEXT("Exact canonical archetype"), Item.ArchetypeId, Archetype);
        TestEqual(TEXT("Exactly the authored visual identity"), Item.VisualId, FName(*Row.VisualId));
        TestTrue(TEXT("Icon and world mesh are loadable"),
            Item.ItemIcon.LoadSynchronous() && Item.WorldMesh.LoadSynchronous());
        TestTrue(TEXT("Manifest references the real icon"),
            Item.ItemIcon.ToSoftObjectPath() == FSoftObjectPath(Row.IconPath));
        TestTrue(TEXT("Manifest references the real world mesh"),
            Item.WorldMesh.ToSoftObjectPath() == FSoftObjectPath(Row.MeshPath));
        if (Row.IsPassive())
        {
            TestTrue(TEXT("Passive ARM has no attacking bindings"), Item.AvailableActions.IsEmpty());
        }
        else
        {
            TestTrue(TEXT("Authored attack id remains bound"),
                Item.AvailableActions.ContainsByPredicate([&Row](const FGridItemActionBinding& Bound)
                {
                    return Bound.ActionId == FName(*Row.AttackId);
                }));
        }
        FString PaletteReason;
        const EGridWeaponPipelinePaletteState PaletteState =
            FGridWeaponPipelinePalette::Inspect(*Palette, Id, Canonical.Registry.Get(),
                WeaponPipelineTests::LegacyCategory(Row.ArchetypeId), PaletteReason);
        TestTrue(*FString::Printf(TEXT("Canonical palette entry retained %s (%s)"),
            *Row.ArchetypeId, *PaletteReason),
            PaletteState == EGridWeaponPipelinePaletteState::PresentSame);
    }
    TestEqual(TEXT("All 22 ARM present"), Active, 22);
    TestEqual(TEXT("One canonical identity for every ARM"), SeenItemIds.Num(), Active);
    TestEqual(TEXT("No duplicate archetype"), SeenArchetypeIds.Num(), Active);
    TestEqual(TEXT("Palette remains read-only"), Palette->GetOutermost()->IsDirty(), bPaletteDirty);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPipelinePalettePlanSafety,
    "Grimrock.RPG.ARMEMENT.Industrial.PalettePlanIdempotenceAndConflicts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponPipelinePalettePlanSafety::RunTest(const FString&)
{
    UGridObjectPaletteAsset* Production =
        LoadObject<UGridObjectPaletteAsset>(nullptr, WeaponPipelineTests::PalettePath);
    WeaponPipelineTests::FCanonicalRegistry Canonical;
    if (!TestNotNull(TEXT("Real production palette"), Production) ||
        !TestTrue(TEXT("Configured canonical registry"), Canonical.bReady))
        return false;
    FGridResolvedItem Item;
    if (!TestTrue(TEXT("ARM002 is a real canonical item"),
        Canonical.Registry->ResolveItem(TEXT("Weapon_AssassinDagger_01"), 1, Item)))
        return false;
    const bool bBeforeDirty = Production->GetOutermost()->IsDirty();
    const FName EntryId(TEXT("Weapon_AssassinDagger_01"));
    const int32 OriginalCount = Production->Entries.Num();

    // Present item: idempotence, zero added entries, no production package write.
    TArray<FGridObjectPaletteEntry> Proposed;
    bool bAdded = false;
    FString Error;
    TestTrue(TEXT("Already-registered item can be proposed unchanged"),
        FGridWeaponPipelinePalette::Propose(*Production, EntryId, Canonical.Registry.Get(), 1, false,
            Proposed, bAdded, Error));
    TestFalse(TEXT("Idempotence: no entry appended"), bAdded);
    TestEqual(TEXT("Idempotence: array size unchanged"), Proposed.Num(), OriginalCount);

    // Duplicate the already canonical production palette. No old source may be introduced.
    UGridObjectPaletteAsset* Pointerless = NewObject<UGridObjectPaletteAsset>(GetTransientPackage());
    Pointerless->Entries = Production->Entries;
    FString PointerlessError;
    TArray<FGridObjectPaletteEntry> PointerlessProposal;
    bool bPointerlessAdded = true;
    TestTrue(TEXT("Historical inspection recognizes existing canonical-only identity"),
        FGridWeaponPipelinePalette::Inspect(*Pointerless, EntryId, Canonical.Registry.Get(), false, PointerlessError) ==
            EGridWeaponPipelinePaletteState::PresentSame);
    TestTrue(TEXT("Existing pointerless item remains idempotent"),
        FGridWeaponPipelinePalette::Propose(*Pointerless, EntryId, Canonical.Registry.Get(), 1, false,
            PointerlessProposal, bPointerlessAdded, PointerlessError));
    TestFalse(TEXT("Canonical-only identity must never be re-added"), bPointerlessAdded);
    const FGridObjectPaletteEntry* Preserved = PointerlessProposal.FindByPredicate([EntryId](const FGridObjectPaletteEntry& Entry)
        { return Entry.EntryId == EntryId; });
    if (TestNotNull(TEXT("Pointerless proposal retains entry"), Preserved))
        TestNull(TEXT("No legacy source reattached"), FGridObjectPaletteEntry::StaticStruct()->FindPropertyByName(TEXT("DefaultItemDefinition")));

    // Simulate an absent entry in a transient copy. No production data is changed.
    UGridObjectPaletteAsset* Missing = NewObject<UGridObjectPaletteAsset>(GetTransientPackage());
    Missing->Entries = Production->Entries;
    Missing->Entries.RemoveAll([EntryId](const FGridObjectPaletteEntry& Entry)
    {
        return Entry.EntryId == EntryId;
    });
    TestEqual(TEXT("Test fixture removed one entry"), Missing->Entries.Num(), OriginalCount - 1);
    Proposed.Reset();
    bAdded = false;
    Error.Reset();
    TestFalse(TEXT("Missing ARM cannot be proposed at N0"),
        FGridWeaponPipelinePalette::Propose(*Missing, EntryId, Canonical.Registry.Get(), 0, false,
            Proposed, bAdded, Error));
    TestTrue(TEXT("Rejected plan exposes no partial output"), Proposed.IsEmpty() && !bAdded);
    if (TestTrue(TEXT("Can propose one absent entry"),
        FGridWeaponPipelinePalette::Propose(*Missing, EntryId, Canonical.Registry.Get(), 1, false,
            Proposed, bAdded, Error)))
    {
        TestTrue(TEXT("Exactly one proposal added"), bAdded);
        TestEqual(TEXT("Proposed entry count restored"), Proposed.Num(), OriginalCount);
        UGridObjectPaletteAsset* Candidate = NewObject<UGridObjectPaletteAsset>(GetTransientPackage());
        Candidate->Entries = Proposed;
        const FGridObjectPaletteEntry* NewEntry = Candidate->FindEntryById(EntryId);
        if (TestNotNull(TEXT("Authored new canonical entry"), NewEntry))
        {
            TestEqual(TEXT("Explicit physical N1"), NewEntry->CanonicalDefaultItemLevel, 1);
            TestEqual(TEXT("Explicit ItemId"), NewEntry->CanonicalItemId, EntryId);
            TestNull(TEXT("No legacy source created"), FGridObjectPaletteEntry::StaticStruct()->FindPropertyByName(TEXT("DefaultItemDefinition")));
        }
        FString Detail;
        TestTrue(TEXT("Proposed entry is compatible"),
            FGridWeaponPipelinePalette::Inspect(*Candidate, EntryId, Canonical.Registry.Get(), false, Detail) ==
                EGridWeaponPipelinePaletteState::PresentSame);
        TArray<FGridObjectPaletteEntry> SecondPass;
        bool bSecondAdded = true;
        FString SecondError;
        TestTrue(TEXT("Second pass succeeds"),
            FGridWeaponPipelinePalette::Propose(*Candidate, EntryId, Canonical.Registry.Get(), 1, false,
                SecondPass, bSecondAdded, SecondError));
        TestFalse(TEXT("Second pass does not duplicate item"), bSecondAdded);
        TestEqual(TEXT("Second pass preserves all entries"), SecondPass.Num(), OriginalCount);

        const FGridObjectPaletteEntry Duplicate = *Candidate->FindEntryById(EntryId);
        Candidate->Entries.Add(Duplicate);
        Detail.Reset();
        TestTrue(TEXT("Duplicate entry rejected"),
            FGridWeaponPipelinePalette::Inspect(*Candidate, EntryId, Canonical.Registry.Get(), false, Detail) ==
                EGridWeaponPipelinePaletteState::Conflict);

        Candidate->Entries.Pop();
        FGridObjectPaletteEntry* TargetEntry =
            Candidate->Entries.FindByPredicate([EntryId](const FGridObjectPaletteEntry& Entry)
            {
                return Entry.EntryId == EntryId;
            });
        if (TargetEntry)
        {
            TargetEntry->PaletteCategory = FName(TEXT("Misc"));
            TestTrue(TEXT("Unexpected category rejected"),
                FGridWeaponPipelinePalette::Inspect(*Candidate, EntryId, Canonical.Registry.Get(), false, Detail) ==
                    EGridWeaponPipelinePaletteState::Conflict);
            TargetEntry->PaletteCategory = FName(TEXT("Weapon"));
            TargetEntry->EntryId = FName(TEXT("Weapon_AlternateAlias_TestOnly"));
            TestTrue(TEXT("Same item registered under another EntryId rejected"),
                FGridWeaponPipelinePalette::Inspect(*Candidate, EntryId, Canonical.Registry.Get(), false, Detail) ==
                    EGridWeaponPipelinePaletteState::Conflict);
            TargetEntry->EntryId = EntryId;
            FGridResolvedItem Other;
            if (TestTrue(TEXT("Canonical dagger is a second real item"),
                Canonical.Registry->ResolveItem(TEXT("Weapon_Dagger_01"), 1, Other)))
            {
                TargetEntry->CanonicalItemId = Other.ItemId;
                TestTrue(TEXT("Palette entry pointing to a different item rejected"),
                    FGridWeaponPipelinePalette::Inspect(*Candidate, EntryId, Canonical.Registry.Get(), false, Detail) ==
                        EGridWeaponPipelinePaletteState::Conflict);
            }
        }
    }
    TestEqual(TEXT("Production array preserved"), Production->Entries.Num(), OriginalCount);
    TestEqual(TEXT("Production package stays clean"), Production->GetOutermost()->IsDirty(), bBeforeDirty);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

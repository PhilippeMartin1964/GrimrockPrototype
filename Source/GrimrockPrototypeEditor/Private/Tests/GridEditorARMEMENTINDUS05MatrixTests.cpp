#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/GridInventoryTypes.h"
#include "UI/GridWeaponTooltipProjection.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectGlobals.h"

/**
 * INDUS05.3: manifest-driven, cross-archetype matrix. One test for all
 * produced weapons; no new test class or hard-coded stats per new weapon.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponIndustrial05ActiveMatrix,
    "Grimrock.RPG.ARMEMENT.Industrial05.ActiveN1ToN20TooltipAndNarrative",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponIndustrial05ActiveMatrix::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Canonical;
    if (!TestTrue(TEXT("Single canonical production registry"), Canonical.bReady))
        return false;
    TArray<FGridWeaponPipelineRow> Rows;
    FString Error;
    if (!TestTrue(TEXT("Source manifest still parses"),
        FGridWeaponPipelineManifest::Load(FGridWeaponPipelineManifest::DefaultFile(), Rows, Error)))
        return false;

    int32 ActiveCount = 0, ProfilesChecked = 0, PassiveProfiles = 0;
    TArray<FString> Report = {
        TEXT("ArchetypeId,BuildKind,ItemDefinitionId,ProfilesChecked,PassiveProfiles,DescriptionPresent,Status")
    };
    for (const FGridWeaponPipelineRow& Row : Rows)
    {
        if (Row.ProductionState != TEXT("Active")) continue;
        ++ActiveCount;
        int32 Count = 0, Passive = 0;
        for (int32 Level = 1; Level <= 20; ++Level)
        {
            FGridResolvedItem Definition;
            if (!TestTrue(*FString::Printf(TEXT("%s canonical N%d"), *Row.ArchetypeId, Level),
                Canonical.Resolve(FName(Row.ItemDefinitionId), Level, Definition)))
                continue;
            ++Count;
            ++ProfilesChecked;
            TestTrue(TEXT("All levels are ARM with stable archetype"),
                Definition.bIsArmament && Definition.ArchetypeId == FName(Row.ArchetypeId));
            TestTrue(TEXT("Every physical level is finite"),
                FMath::IsFinite(Definition.WeightKg) && Definition.WeightKg >= 0.f);
            FGridItemInstance Item;
            Item.RuntimeObjectId = FGuid::NewGuid();
            Item.ItemDefinitionId = Definition.ItemId;
            Item.ItemLevel = Level;
            Item.Quantity = 1;
            Item.OwnerType = EGridItemOwnerType::CharacterInventory;
            Item.OwnerCharacterIndex = 0;

            FGridWeaponTooltipPresentation Tooltip;
            if (!TestTrue(*FString::Printf(TEXT("%s N%d canonical tooltip"), *Row.ArchetypeId, Level),
                FGridWeaponTooltipProjection::Build(nullptr, Item, Tooltip, Canonical.Registry.Get())))
                continue;
            TestEqual(TEXT("Tooltip has exact item level"), Tooltip.ItemLevel, Level);
            TestEqual(TEXT("Tooltip passive state matches resolved dice"),
                Tooltip.bPassive, !Definition.Weapon.DamageDice.bEnabled);
            if (Row.IsPassive())
            {
                ++Passive;
                ++PassiveProfiles;
                TestFalse(TEXT("Passive shield/focus has no invented dice"),
                    Definition.Weapon.DamageDice.bEnabled);
                const FGridWeaponTooltipFact* Armor =
                    Tooltip.Facts.FindByPredicate([](const FGridWeaponTooltipFact& Fact)
                    {
                        return Fact.StatId == TEXT("WeaponPhysicalArmor");
                    });
                if (Definition.Weapon.PhysicalArmor > 0)
                    TestNotNull(TEXT("Nominal physical armor is visible"), Armor);
            }
            else
            {
                TestTrue(TEXT("Active attacking weapon retains dice"),
                    Definition.Weapon.DamageDice.bEnabled);
            }
        }
        TestEqual(*FString::Printf(TEXT("%s has twenty physical profiles"), *Row.ArchetypeId),
            Count, 20);
        if (Row.IsPassive())
            TestEqual(TEXT("All twenty passive profiles remain passive"), Passive, 20);
        FGridResolvedItem N1;
        const bool bHasN1 = Canonical.Resolve(FName(Row.ItemDefinitionId), 1, N1);
        const bool bDescription = bHasN1 &&
            !N1.Description.ToString().TrimStartAndEnd().IsEmpty();
        TestTrue(TEXT("Canonical item narrative description retained"), bDescription);
        Report.Add(FString::Printf(TEXT("%s,%s,%s,%d,%d,%s,%s"),
            *Row.ArchetypeId, *Row.BuildKind, *Row.ItemDefinitionId,
            Count, Passive, bDescription ? TEXT("true") : TEXT("false"),
            Count == 20 ? TEXT("CHECKED") : TEXT("INCOMPLETE")));
    }
    TestEqual(TEXT("Exactly 22 active canonical ARM"), ActiveCount, 22);
    TestEqual(TEXT("Every N1-N20 ARM profile was read"), ProfilesChecked, ActiveCount * 20);
    TestTrue(TEXT("Passive category retains all 20 levels"), PassiveProfiles >= 20);
    const FString Folder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ARMEMENT-INDUS"));
    IFileManager::Get().MakeDirectory(*Folder, true);
    const FString Filename = FPaths::Combine(Folder, TEXT("INDUS05_ActiveMatrix.csv"));
    TestTrue(TEXT("Canonical active matrix CSV written under Saved"),
        FFileHelper::SaveStringArrayToFile(Report, *Filename));
    AddInfo(FString::Printf(TEXT("[INDUS05] CanonicalARM=%d Profiles=%d Passive=%d Report=%s"),
        ActiveCount, ProfilesChecked, PassiveProfiles, *Filename));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponIndustrial05LegacyExceptions,
    "Grimrock.RPG.ARMEMENT.Industrial05.LegacyThrowingItemsPreserved",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponIndustrial05LegacyExceptions::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Canonical;
    if (!TestTrue(TEXT("Canonical registry configured"), Canonical.bReady))
        return false;
    FGridResolvedItem Shuriken, ThrowingKnife;
    if (!TestTrue(TEXT("Shuriken non-ARM N0 resolves without legacy package"),
        Canonical.Resolve(TEXT("Shuriken"), 0, Shuriken)))
        return false;
    TestFalse(TEXT("Shuriken remains an actual non-ARM"), Shuriken.bIsArmament);
    TestTrue(TEXT("Shuriken retains authored physical throw"),
        Shuriken.GenericThrow.bCombatRecoverable &&
        Shuriken.GenericThrow.InitialSpeed > 0.f);
    if (!TestTrue(TEXT("ARM054 N1 throwing knife resolves without legacy redirector"),
        Canonical.Resolve(TEXT("Weapon_ThrowingKnife_01"), 1, ThrowingKnife)))
        return false;
    TestTrue(TEXT("ARM054 remains a canonical thrown weapon"),
        ThrowingKnife.bIsArmament &&
        ThrowingKnife.ArchetypeId == FName(TEXT("ARM054")) &&
        ThrowingKnife.Weapon.Projectile.bEnabled);
    TestFalse(TEXT("Legacy ThrowingKnife redirector is not a canonical ItemId"),
        Canonical.Registry->ContainsItem(TEXT("Weapon_ThrowingKnife")));

    // Confirm the approved Shadow/Assassin recipes use stable ItemIds, not
    // old DataAsset pointers; historical two-item kits are not production.
    URPGClassAsset* Rogue = LoadObject<URPGClassAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Rogue.DA_Class_Rogue"));
    if (!TestNotNull(TEXT("Production Rogue loads"), Rogue)) return false;
    const bool bOriginalDirty = Rogue->GetOutermost()->IsDirty();
    URPGClassAsset* Temporary = DuplicateObject<URPGClassAsset>(Rogue, GetTransientPackage());
    FString Error;
    if (!TestTrue(TEXT("Approved Rogue kits materialize from canonical registry"),
        Temporary && FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(
            *Temporary, Error, Canonical.Registry.Get())))
    {
        AddError(Error);
        return false;
    }
    const FRPGStartingEquipmentKit* Assassin =
        Temporary->StartingEquipmentKits.FindByPredicate(
            [](const FRPGStartingEquipmentKit& Kit)
            {
                return Kit.KitId == FName(TEXT("Kit_Rogue_Assassin"));
            });
    if (!TestNotNull(TEXT("Canonical Assassin kit exists"), Assassin))
        return false;
    TestEqual(TEXT("Assassin kit keeps exactly two items"), Assassin->Items.Num(), 2);
    if (Assassin->Items.Num() == 2)
    {
        TestEqual(TEXT("N1 assassin dagger remains equipped"),
            Assassin->Items[0].ItemId, FName(TEXT("Weapon_AssassinDagger_01")));
        TestEqual(TEXT("Mainhand retains its physical placement"),
            Assassin->Items[0].TargetSlot, EGridEquipmentSlot::MainHand);
        TestEqual(TEXT("N1 throwing knife remains in bag"),
            Assassin->Items[1].ItemId, FName(TEXT("Weapon_ThrowingKnife_01")));
        TestEqual(TEXT("Throwing knife not equipped"),
            Assassin->Items[1].TargetSlot, EGridEquipmentSlot::None);
        TestNull(TEXT("No old item pointer in canonical starter"),
            Assassin->Items[1].ItemDefinition.Get());
    }
    TestEqual(TEXT("Production Rogue remained unchanged"),
        Rogue->GetOutermost()->IsDirty(), bOriginalDirty);

    // Read-only binary reference inventory for the retired ThrowingKnife
    // redirector and its artwork. Do not infer deletion authorization from
    // absent registry edges: standalone C++ and private saves also matter.
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
        TEXT("AssetRegistry")).Get();
    Registry.SearchAllAssets(true);
    struct FLegacyPackage { const TCHAR* Label; const TCHAR* Path; };
    static const FLegacyPackage Candidates[] =
    {
        {TEXT("LegacyItem"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ThrowingKnife")},
        {TEXT("LegacyMesh"), TEXT("/Game/GrimrockPrototype/Meshes/Weapons/SM_ThrowingKnife")},
        {TEXT("LegacyIcon"), TEXT("/Game/GrimrockPrototype/UI/Icons/Weapons/T_Weapon_ThrowingKnife")},
        {TEXT("LegacyMaterial"), TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Weapons/MI_ThrowingKnife")},
        {TEXT("LegacyBC"), TEXT("/Game/GrimrockPrototype/Art/Textures/Final/Weapons/ThrowingKnife/T_ThrowingKnife_01_BC")},
        {TEXT("LegacyN"), TEXT("/Game/GrimrockPrototype/Art/Textures/Final/Weapons/ThrowingKnife/T_ThrowingKnife_01_N")},
        {TEXT("LegacyORM"), TEXT("/Game/GrimrockPrototype/Art/Textures/Final/Weapons/ThrowingKnife/T_ThrowingKnife_01_ORM")}
    };
    TArray<FString> Audit =
    {
        TEXT("Label,Package,Exists,RegistryQuery,ReferencerCount,Referencers,Decision")
    };
    for (const FLegacyPackage& Candidate : Candidates)
    {
        const FName Package(Candidate.Path);
        TArray<FAssetData> Assets;
        const bool bExists = Registry.GetAssetsByPackageName(
            Package, Assets, true, false) && !Assets.IsEmpty();
        TArray<FName> Referencers;
        const bool bQueried = bExists && Registry.GetReferencers(
            Package, Referencers, UE::AssetRegistry::EDependencyCategory::All,
            UE::AssetRegistry::FDependencyQuery());
        Referencers.Sort([](const FName& A, const FName& B)
        {
            return A.ToString() < B.ToString();
        });
        TArray<FString> Names;
        for (const FName Name : Referencers) Names.Add(Name.ToString());
        const FString Decision = !bExists ? TEXT("NOT_FOUND") :
            !bQueried ? TEXT("QUERY_UNCONFIRMED") :
            !Referencers.IsEmpty() ? TEXT("BLOCKED_REFERENCERS") :
            TEXT("REGISTRY_CANDIDATE_CONFIRM_IN_EDITOR");
        Audit.Add(FString::Printf(TEXT("%s,%s,%d,%d,%d,%s,%s"),
            Candidate.Label, Candidate.Path, bExists ? 1 : 0, bQueried ? 1 : 0,
            Referencers.Num(), *FString::Join(Names, TEXT("|")), *Decision));
    }
    const FString ReportDir = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("ARMEMENT-LOT02"));
    IFileManager::Get().MakeDirectory(*ReportDir, true);
    const FString Report = FPaths::Combine(
        ReportDir, TEXT("LOT02_14_LegacyThrowingKnifeReferences.csv"));
    TestTrue(TEXT("Private native references report remains read-only"),
        FFileHelper::SaveStringArrayToFile(Audit, *Report));
    AddInfo(FString::Printf(
        TEXT("[ARM-CLEAN06] LegacyRedirectorCandidates=%d CandidateRemoval=NOT_AUTHORIZED Report=%s"),
        UE_ARRAY_COUNT(Candidates), *Report));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponIndustrial05SingleEntrypoint,
    "Grimrock.RPG.ARM.CLEAN06_5G3.RetiredLegacyWeaponWriters",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponIndustrial05SingleEntrypoint::RunTest(const FString&)
{
    const FString Scripts = FPaths::Combine(FPaths::ProjectDir(), TEXT("Scripts"));
    const TCHAR* RemovedWrappers[] = {
        TEXT("WeaponPipeline.ps1"),
        TEXT("ProduceStarterWeaponWave.ps1"),
        TEXT("StarterKitProductionBatch.ps1"),
        TEXT("PlanStarterWeaponWaves.ps1"),
        TEXT("RepairARMEMENT0171Palette.ps1"),
        TEXT("RepairRecurveBowAmmo.ps1"),
        TEXT("ProduceMarksmanAmmunition.ps1"),
        TEXT("ApplyARMEMENT011023WeaponFireBombPalette.ps1"),
        TEXT("AuthorARMEMENT01102WarriorN1.ps1"),
        TEXT("AuthorARMEMENT011122AssassinDaggerN1.ps1"),
        TEXT("RegisterARMEMENT011021WarriorVisuals.ps1"),
        TEXT("RegisterARMEMENT011121AssassinDaggerVisual.ps1"),
        TEXT("WeaponNarrativeApply.ps1"),
        TEXT("WeaponNarrativePrototypeBatch.ps1")
    };
    for (const TCHAR* Old : RemovedWrappers)
    {
        const FString File = FPaths::Combine(Scripts, Old);
        TestFalse(*FString::Printf(TEXT("Retired legacy weapon wrapper remains absent: %s"), Old),
            IFileManager::Get().FileExists(*File));
    }
    const TCHAR* RemovedCommandlets[] = {
        TEXT("GridWeaponPipelineCommandlet"),
        TEXT("GridWeaponPilotItemAuthoringCommandlet"),
        TEXT("GridWeaponPilotPaletteAuthoringCommandlet"),
        TEXT("GridMarksmanAmmunitionAuthoringCommandlet"),
        TEXT("GridWeaponFireBombPaletteCommandlet"),
    };
    for (const TCHAR* Old : RemovedCommandlets)
    {
        const FString Name(Old);
        const FString Header = FPaths::Combine(FPaths::ProjectDir(),
            TEXT("Source/GrimrockPrototypeEditor/Public/ARMEMENT"), Name + TEXT(".h"));
        const FString Cpp = FPaths::Combine(FPaths::ProjectDir(),
            TEXT("Source/GrimrockPrototypeEditor/Private/ARMEMENT"), Name + TEXT(".cpp"));
        const FString ClassPath = TEXT("/Script/GrimrockPrototypeEditor.") + Name;
        TestFalse(*FString::Printf(TEXT("Old authoring header absent: %s"), Old), IFileManager::Get().FileExists(*Header));
        TestFalse(*FString::Printf(TEXT("Old authoring source absent: %s"), Old), IFileManager::Get().FileExists(*Cpp));
        TestTrue(*FString::Printf(TEXT("Old authoring commandlet not registered: %s"), Old),
            FindObject<UClass>(nullptr, *ClassPath) == nullptr);
    }

    // The approved recipe and source manifests remain read-only evidence
    // until the final legacy parity audits are replaced, not writer inputs.
    TestTrue(TEXT("Approved 18 kit recipe source remains available"),
        IFileManager::Get().FileExists(*FPaths::Combine(FPaths::ProjectDir(),
            TEXT("docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv"))));
    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

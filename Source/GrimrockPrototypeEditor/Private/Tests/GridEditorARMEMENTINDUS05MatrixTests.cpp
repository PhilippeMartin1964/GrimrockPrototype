#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
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
    TArray<FGridWeaponPipelineRow> Rows;
    FString Error;
    if (!TestTrue(TEXT("Authoritative manifest parses"),
        FGridWeaponPipelineManifest::Load(FGridWeaponPipelineManifest::DefaultFile(), Rows, Error)))
        return false;

    UGridWeaponCatalogueAsset* Catalogue = LoadObject<UGridWeaponCatalogueAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
    if (!TestNotNull(TEXT("Authoritative 1640-profile catalogue"), Catalogue))
        return false;
    const bool bCatalogueDirtyBefore = Catalogue->GetOutermost()->IsDirty();
    TestTrue(TEXT("Canonical workbook profile contract valid"), Catalogue->IsValidCatalogue());

    int32 ActiveCount = 0, ProfilesChecked = 0, PassiveProfiles = 0;
    TArray<FString> Report = {
        TEXT("ArchetypeId,BuildKind,ItemDefinitionId,ProfilesChecked,PassiveProfiles,DescriptionPresent,Status")
    };
    for (const FGridWeaponPipelineRow& Row : Rows)
    {
        if (Row.ProductionState != TEXT("Active"))
            continue;

        ++ActiveCount;
        UGridItemDefinitionAsset* Definition =
            LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
        if (!TestNotNull(*FString::Printf(TEXT("%s actual ItemDefinition"), *Row.ArchetypeId), Definition))
        {
            Report.Add(FString::Printf(TEXT("%s,%s,%s,0,0,false,MISSING_ITEM"),
                *Row.ArchetypeId, *Row.BuildKind, *Row.ItemDefinitionId));
            continue;
        }
        const bool bItemDirtyBefore = Definition->GetOutermost()->IsDirty();
        TestEqual(*FString::Printf(TEXT("%s stable definition ID"), *Row.ArchetypeId),
            Definition->ItemDefinitionId, FName(Row.ItemDefinitionId));
        TestEqual(*FString::Printf(TEXT("%s stable archetype ID"), *Row.ArchetypeId),
            Definition->WeaponArchetypeId, FName(Row.ArchetypeId));
        TestTrue(*FString::Printf(TEXT("%s narrative description published"), *Row.ArchetypeId),
            !Definition->Description.ToString().TrimStartAndEnd().IsEmpty());

        int32 Count = 0, Passive = 0;
        for (int32 Level = 1; Level <= 20; ++Level)
        {
            const FGridWeaponLevelProfile* Profile =
                Catalogue->FindProfile(FName(Row.ArchetypeId), Level);
            if (!TestNotNull(*FString::Printf(TEXT("%s N%d exists"), *Row.ArchetypeId, Level), Profile))
                continue;

            ++Count;
            ++ProfilesChecked;
            TestTrue(*FString::Printf(TEXT("%s N%d valid profile"), *Row.ArchetypeId, Level),
                Profile->IsValidProfile());

            FGridItemInstance Item;
            Item.RuntimeObjectId = FGuid::NewGuid();
            Item.ItemDefinitionId = Definition->ItemDefinitionId;
            Item.DisplayName = Definition->DisplayName;
            Item.ItemLevel = Level;
            Item.Quantity = 1;
            Item.OwnerType = EGridItemOwnerType::CharacterInventory;
            Item.OwnerCharacterIndex = 0;

            FGridWeaponTooltipPresentation Tooltip;
            if (!TestTrue(*FString::Printf(TEXT("%s N%d tooltip builds"), *Row.ArchetypeId, Level),
                FGridWeaponTooltipProjection::Build(Definition, Item, Tooltip)))
                continue;
            TestEqual(*FString::Printf(TEXT("%s N%d no implicit level fallback"), *Row.ArchetypeId, Level),
                Tooltip.ItemLevel, Level);
            TestEqual(*FString::Printf(TEXT("%s N%d passive flag matches canonical dice"), *Row.ArchetypeId, Level),
                Tooltip.bPassive, !Profile->DamageDice.bEnabled);
            if (Row.IsPassive())
            {
                ++Passive;
                ++PassiveProfiles;
                TestFalse(*FString::Printf(TEXT("%s N%d passive shield has no attack"), *Row.ArchetypeId, Level),
                    Profile->DamageDice.bEnabled);
                const FGridWeaponTooltipFact* Armor =
                    Tooltip.Facts.FindByPredicate([](const FGridWeaponTooltipFact& Fact)
                    {
                        return Fact.StatId == TEXT("WeaponPhysicalArmor");
                    });
                if (Profile->PhysicalArmor > 0)
                    TestNotNull(*FString::Printf(TEXT("%s N%d nominal armor shown"), *Row.ArchetypeId, Level), Armor);
            }
            else
            {
                TestTrue(*FString::Printf(TEXT("%s N%d attack remains authoritative"), *Row.ArchetypeId, Level),
                    Profile->DamageDice.bEnabled);
            }
        }
        TestEqual(*FString::Printf(TEXT("%s exactly 20 levels"), *Row.ArchetypeId), Count, 20);
        if (Row.IsPassive())
            TestEqual(TEXT("Every passive level stays passive"), Passive, 20);
        TestEqual(*FString::Printf(TEXT("%s item package never dirtied"), *Row.ArchetypeId),
            Definition->GetOutermost()->IsDirty(), bItemDirtyBefore);
        Report.Add(FString::Printf(TEXT("%s,%s,%s,%d,%d,%s,%s"),
            *Row.ArchetypeId, *Row.BuildKind, *Row.ItemDefinitionId, Count, Passive,
            Definition->Description.ToString().TrimStartAndEnd().IsEmpty()
                ? TEXT("false") : TEXT("true"),
            Count == 20 ? TEXT("CHECKED") : TEXT("INCOMPLETE")));
    }
    TestTrue(TEXT("At least one active weapon authored"), ActiveCount >= 8);
    TestEqual(TEXT("All active profiles N1 to N20 checked"), ProfilesChecked, ActiveCount * 20);
    TestTrue(TEXT("Passive category remains present"), PassiveProfiles >= 20);
    TestEqual(TEXT("Canonical catalogue package stays clean"),
        Catalogue->GetOutermost()->IsDirty(), bCatalogueDirtyBefore);

    const FString Folder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ARMEMENT-INDUS"));
    IFileManager::Get().MakeDirectory(*Folder, true);
    const FString Filename = FPaths::Combine(Folder, TEXT("INDUS05_ActiveMatrix.csv"));
    TestTrue(TEXT("Single reusable industrial matrix report written under Saved"),
        FFileHelper::SaveStringArrayToFile(Report, *Filename));
    AddInfo(FString::Printf(TEXT("[INDUS05] Active=%d Profiles=%d Passive=%d Report=%s"),
        ActiveCount, ProfilesChecked, PassiveProfiles, *Filename));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponIndustrial05LegacyExceptions,
    "Grimrock.RPG.ARMEMENT.Industrial05.LegacyThrowingItemsPreserved",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponIndustrial05LegacyExceptions::RunTest(const FString&)
{
    const TCHAR* LegacyPaths[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Shuriken.DA_Weapon_Shuriken"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ThrowingKnife.DA_Weapon_ThrowingKnife")
    };
    TArray<FGridWeaponPipelineRow> Rows;
    FString Error;
    if (!TestTrue(TEXT("Manifest parse"), FGridWeaponPipelineManifest::Load(
        FGridWeaponPipelineManifest::DefaultFile(), Rows, Error))) return false;
    for (const TCHAR* Path : LegacyPaths)
    {
        UGridItemDefinitionAsset* Legacy = LoadObject<UGridItemDefinitionAsset>(nullptr, Path);
        if (!TestNotNull(TEXT("Existing throwing-item asset retained pending deletion audit"), Legacy)) continue;
        const bool bDirtyBefore = Legacy->GetOutermost()->IsDirty();
        const FGridWeaponPipelineRow* Migrated = Rows.FindByPredicate([Path](const FGridWeaponPipelineRow& Row)
        {
            return Row.ItemPath == Path;
        });
        TestNull(TEXT("Special legacy throwing assets not silently treated as standard BuildKind"), Migrated);
        TestEqual(TEXT("Legacy item untouched by industrial checks"),
            Legacy->GetOutermost()->IsDirty(), bDirtyBefore);
    }

    // LOT02.14: migrate the two runtime starter presets in one logical batch.
    // The replacement exists but its Planned -> Active transition is a
    // separate, PIE-gated decision. No production class asset is modified.
    const FGridWeaponPipelineRow* CanonicalRow = FGridWeaponPipelineManifest::Find(
        Rows, TEXT("ARM054"));
    if (!TestNotNull(TEXT("ARM054 canonical production row exists"), CanonicalRow))
    {
        return false;
    }
    UGridItemDefinitionAsset* Canonical = LoadObject<UGridItemDefinitionAsset>(
        nullptr, *CanonicalRow->ItemPath);
    if (!TestNotNull(TEXT("ARM054 real ItemDefinition loads"), Canonical))
    {
        return false;
    }
    const bool bCanonicalDirty = Canonical->GetOutermost()->IsDirty();
    TestEqual(TEXT("ARM054 canonical definition ID"),
        Canonical->ItemDefinitionId, FName(CanonicalRow->ItemDefinitionId));
    TestEqual(TEXT("ARM054 canonical archetype ID"),
        Canonical->WeaponArchetypeId, FName(TEXT("ARM054")));
    TestTrue(TEXT("ARM054 definition is valid"), Canonical->IsValidDefinition());
    TestTrue(TEXT("ARM054 is still a physical throwing weapon"),
        Canonical->bCombatThrowWeapon && Canonical->IsPhysicallyThrowable());
    TestTrue(TEXT("ARM054 can rehydrate the same saved definition ID"),
        FRPGStartingEquipmentPresetCatalog::ResolveKnownStartingItemDefinition(
            Canonical->ItemDefinitionId) == Canonical);

    struct FKitToMigrate
    {
        const TCHAR* ClassId;
        const TCHAR* KitId;
        const TCHAR* ExpectedMainAsset;
    };
    static const FKitToMigrate Migrations[] =
    {
        { TEXT("Warrior"), TEXT("Warrior_Skirmisher"), TEXT("DA_Weapon_Dagger_01") },
        { TEXT("Rogue"), TEXT("Rogue_Shadow"), TEXT("DA_Weapon_Dagger_01") }
    };
    for (const FKitToMigrate& Migration : Migrations)
    {
        const FString ClassPath = FString::Printf(
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_%s.DA_Class_%s"),
            Migration.ClassId, Migration.ClassId);
        URPGClassAsset* Production = LoadObject<URPGClassAsset>(nullptr, *ClassPath);
        if (!TestNotNull(TEXT("Real starter class loads for migration audit"), Production))
        {
            continue;
        }
        const bool bProductionDirty = Production->GetOutermost()->IsDirty();
        URPGClassAsset* Temporary = DuplicateObject<URPGClassAsset>(
            Production, GetTransientPackage());
        if (!TestNotNull(TEXT("Temporary class copy exists"), Temporary))
        {
            continue;
        }
        FString PopulateError;
        if (!TestTrue(TEXT("Transient starter kit materializes"),
            FRPGStartingEquipmentPresetCatalog::TryPopulateTransientClass(*Temporary, PopulateError)))
        {
            AddError(FString::Printf(TEXT("[LOT02.14] Class=%s Error=%s"),
                Migration.ClassId, *PopulateError));
            continue;
        }
        const FRPGStartingEquipmentKit* Kit = Temporary->StartingEquipmentKits.FindByPredicate(
            [&Migration](const FRPGStartingEquipmentKit& Candidate)
            {
                return Candidate.KitId == FName(Migration.KitId);
            });
        if (!TestNotNull(TEXT("Migrated starter kit exists"), Kit))
        {
            continue;
        }
        TestEqual(TEXT("Migrated kit still contains exactly two items"), Kit->Items.Num(), 2);
        if (Kit->Items.Num() == 2)
        {
            const FRPGStartingEquipmentEntry& Main = Kit->Items[0];
            const FRPGStartingEquipmentEntry& Throw = Kit->Items[1];
            TestEqual(TEXT("Original starting dagger remains equipped"),
                Main.ItemDefinition->GetName(), FString(Migration.ExpectedMainAsset));
            TestEqual(TEXT("Original dagger remains in MainHand"),
                Main.TargetSlot, EGridEquipmentSlot::MainHand);
            TestTrue(TEXT("Starter throwing knife is exact ARM054 DataAsset"),
                Throw.ItemDefinition.Get() == Canonical);
            TestEqual(TEXT("Starter throwing knife remains in the bag"),
                Throw.TargetSlot, EGridEquipmentSlot::None);
            TestEqual(TEXT("Starter throwing knife is one unit"),
                Throw.Quantity, 1);
        }
        TestEqual(TEXT("Production starter class not dirtied"),
            Production->GetOutermost()->IsDirty(), bProductionDirty);
    }
    TestEqual(TEXT("Canonical ARM054 asset not dirtied"),
        Canonical->GetOutermost()->IsDirty(), bCanonicalDirty);

    // Read-only snapshot of actual binary Unreal references. GitHub search
    // cannot see these (.uasset files are LFS objects). This deliberately
    // does not delete anything, or assume that zero references is proven.
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
        TEXT("AssetRegistry")).Get();
    Registry.SearchAllAssets(true);
    struct FLegacyPackage
    {
        const TCHAR* Label;
        const TCHAR* Path;
    };
    static const FLegacyPackage Candidates[] =
    {
        { TEXT("LegacyItem"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ThrowingKnife") },
        { TEXT("LegacyMesh"), TEXT("/Game/GrimrockPrototype/Meshes/Weapons/SM_ThrowingKnife") },
        { TEXT("LegacyIcon"), TEXT("/Game/GrimrockPrototype/UI/Icons/Weapons/T_Weapon_ThrowingKnife") },
        { TEXT("LegacyMaterial"), TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Weapons/MI_ThrowingKnife") },
        { TEXT("LegacyBC"), TEXT("/Game/GrimrockPrototype/Art/Textures/Final/Weapons/ThrowingKnife/T_ThrowingKnife_01_BC") },
        { TEXT("LegacyN"), TEXT("/Game/GrimrockPrototype/Art/Textures/Final/Weapons/ThrowingKnife/T_ThrowingKnife_01_N") },
        { TEXT("LegacyORM"), TEXT("/Game/GrimrockPrototype/Art/Textures/Final/Weapons/ThrowingKnife/T_ThrowingKnife_01_ORM") }
    };
    TArray<FString> Audit = {
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
        for (const FName Name : Referencers)
        {
            Names.Add(Name.ToString());
            AddInfo(FString::Printf(TEXT("[LOT02.14] Asset=%s Referencer=%s"),
                Candidate.Label, *Name.ToString()));
        }
        const FString Decision = !bExists ? TEXT("NOT_FOUND") :
            !bQueried ? TEXT("QUERY_UNCONFIRMED") :
            !Referencers.IsEmpty() ? TEXT("BLOCKED_REFERENCERS") :
            TEXT("REGISTRY_CANDIDATE_CONFIRM_IN_EDITOR");
        Audit.Add(FString::Printf(TEXT("%s,%s,%d,%d,%d,%s,%s"),
            Candidate.Label, Candidate.Path, bExists ? 1 : 0, bQueried ? 1 : 0,
            Referencers.Num(), *FString::Join(Names, TEXT("|")), *Decision));
        AddInfo(FString::Printf(
            TEXT("[LOT02.14] Asset=%s Exists=%d RefQuery=%d Referencers=%d Decision=%s"),
            Candidate.Label, bExists ? 1 : 0, bQueried ? 1 : 0,
            Referencers.Num(), *Decision));
    }
    const FString ReportDir = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("ARMEMENT-LOT02"));
    IFileManager::Get().MakeDirectory(*ReportDir, true);
    const FString Report = FPaths::Combine(
        ReportDir, TEXT("LOT02_14_LegacyThrowingKnifeReferences.csv"));
    TestTrue(TEXT("Persist read-only binary reference audit under Saved"),
        FFileHelper::SaveStringArrayToFile(Audit, *Report));
    AddInfo(FString::Printf(TEXT("[LOT02.14] Report=%s OriginalLegacyAssetRetained=1"),
        *Report));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponIndustrial05SingleEntrypoint,
    "Grimrock.RPG.ARMEMENT.Industrial05.SinglePublicPipeline",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponIndustrial05SingleEntrypoint::RunTest(const FString&)
{
    const FString Scripts = FPaths::Combine(FPaths::ProjectDir(), TEXT("Scripts"));
    FString Text;
    const FString Pipeline = FPaths::Combine(Scripts, TEXT("WeaponPipeline.ps1"));
    if (!TestTrue(TEXT("Canonical PowerShell pipeline exists"),
        FFileHelper::LoadFileToString(Text, *Pipeline))) return false;
    TestTrue(TEXT("Generic commandlet invocation retained"),
        Text.Contains(TEXT("-run=GridWeaponPipeline")));
    TestTrue(TEXT("Narrative publishing integrated in main script"),
        Text.Contains(TEXT("-Action=NarrativeItem")));
    TestTrue(TEXT("Explicit Apply retained"),
        Text.Contains(TEXT("if ($Apply)")));
    TestTrue(TEXT("No automatic git staging or pushing"),
        !Text.Contains(TEXT("git add -A")) && !Text.Contains(TEXT("git push origin")));
    const TCHAR* RemovedWrappers[] = {
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
        TestFalse(*FString::Printf(TEXT("Retired per-weapon wrapper remains absent: %s"), Old),
            IFileManager::Get().FileExists(*File));
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

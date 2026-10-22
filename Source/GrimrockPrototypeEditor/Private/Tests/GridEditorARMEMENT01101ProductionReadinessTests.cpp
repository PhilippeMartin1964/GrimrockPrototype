#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetData.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/Csv/CsvParser.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENT01101
{
    static const TCHAR* const WeaponFolder =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons");
    static const TCHAR* const RpgPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue");
    static const TCHAR* const VisualPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue");

    bool ReadRequiredArchetypes(TSet<FName>& OutRequired, FString& OutError)
    {
        OutRequired.Reset();
        OutError.Reset();

        const FString SourcePath = FPaths::Combine(
            FPaths::ProjectDir(), TEXT("docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv"));
        FString Csv;
        if (!FFileHelper::LoadFileToString(Csv, *SourcePath))
        {
            OutError = TEXT("Approved starter-kit recipe manifest not found");
            return false;
        }

        FCsvParser Parser(Csv);
        const auto& Rows = Parser.GetRows();
        if (Rows.Num() < 2 || Rows[0].Num() != 15 ||
            FCString::Strcmp(Rows[0][3], TEXT("Weapon1ArchetypeId")) != 0 ||
            FCString::Strcmp(Rows[0][6], TEXT("Weapon2ArchetypeId")) != 0 ||
            FCString::Strcmp(Rows[0][13], TEXT("ApprovalState")) != 0)
        {
            OutError = TEXT("Unexpected recipe manifest schema");
            return false;
        }

        int32 RecipeCount = 0;
        for (int32 Index = 1; Index < Rows.Num(); ++Index)
        {
            const TArray<const TCHAR*>& Row = Rows[Index];
            if (Row.IsEmpty() || !Row[0] || !*Row[0])
            {
                continue;
            }
            if (Row.Num() != 15 || FCString::Strcmp(Row[13], TEXT("ApprovedDesign")) != 0)
            {
                OutError = FString::Printf(TEXT("Recipe row %d is malformed or not approved"), Index + 1);
                return false;
            }

            const FName First(Row[3]);
            if (!UGridWeaponVisualCatalogueAsset::IsCanonicalArchetypeId(First))
            {
                OutError = FString::Printf(TEXT("Invalid first archetype at row %d"), Index + 1);
                return false;
            }
            OutRequired.Add(First);

            if (FCString::Strcmp(Row[6], TEXT("None")) != 0)
            {
                const FName Second(Row[6]);
                if (!UGridWeaponVisualCatalogueAsset::IsCanonicalArchetypeId(Second))
                {
                    OutError = FString::Printf(TEXT("Invalid second archetype at row %d"), Index + 1);
                    return false;
                }
                OutRequired.Add(Second);
            }
            ++RecipeCount;
        }

        if (RecipeCount != 18 || OutRequired.Num() != 22)
        {
            OutError = FString::Printf(TEXT("Expected 18 approved recipes and 22 archetypes; got %d and %d"),
                RecipeCount, OutRequired.Num());
            return false;
        }
        return true;
    }

    bool IsPilot(FName ArchetypeId)
    {
        return ArchetypeId == TEXT("ARM001") ||
            ArchetypeId == TEXT("ARM045") ||
            ArchetypeId == TEXT("ARM075");
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01101ReadinessAudit,
    "Grimrock.RPG.ARMEMENT01.10.Readiness.Audit22N1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01101ReadinessAudit::RunTest(const FString&)
{
    TSet<FName> Required;
    FString Error;
    if (!TestTrue(TEXT("18 approved recipes determine the 22 N1 archetypes"),
        ARMEMENT01101::ReadRequiredArchetypes(Required, Error)))
    {
        AddInfo(Error);
        return false;
    }

    UGridWeaponCatalogueAsset* Rpg =
        LoadObject<UGridWeaponCatalogueAsset>(nullptr, ARMEMENT01101::RpgPath);
    UGridWeaponVisualCatalogueAsset* Visuals =
        LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, ARMEMENT01101::VisualPath);
    if (!TestNotNull(TEXT("Production RPG catalogue"), Rpg) ||
        !TestNotNull(TEXT("Production visual catalogue"), Visuals))
    {
        return false;
    }
    if (!TestTrue(TEXT("1640 authoritative RPG profiles"), Rpg->IsValidCatalogue()) ||
        !TestTrue(TEXT("Existing partial visual catalogue is valid"), Visuals->IsValidCatalogue()))
    {
        return false;
    }

    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
        TEXT("AssetRegistry")).Get();
    const FString WeaponFolder(ARMEMENT01101::WeaponFolder);
    TArray<FString> ScanPaths = { WeaponFolder };
    Registry.ScanPathsSynchronous(ScanPaths, true);
    TArray<FAssetData> Assets;
    Registry.GetAssetsByPath(FName(*WeaponFolder), Assets, true);

    TMap<FName, TArray<UGridItemDefinitionAsset*>> ByArchetype;
    for (const FAssetData& AssetData : Assets)
    {
        UGridItemDefinitionAsset* Item = Cast<UGridItemDefinitionAsset>(AssetData.GetAsset());
        if (Item && Required.Contains(Item->WeaponArchetypeId))
        {
            ByArchetype.FindOrAdd(Item->WeaponArchetypeId).Add(Item);
        }
    }

    TArray<FName> SortedRequired = Required.Array();
    SortedRequired.Sort([](FName Left, FName Right)
    {
        return Left.ToString() < Right.ToString();
    });

    FString CsvReport = TEXT(
        "ArchetypeId,ItemLevel,VisualMapped,IconLoaded,WorldMeshLoaded,ItemCount,ItemId,DefinitionValid,ActionChecked,Status\n");
    int32 Candidates = 0;
    int32 MissingDefinitions = 0;
    int32 MissingVisuals = 0;
    int32 DuplicateDefinitions = 0;
    int32 VerifiedPilots = 0;

    const bool bRpgDirtyBefore = Rpg->GetOutermost()->IsDirty();
    const bool bVisualDirtyBefore = Visuals->GetOutermost()->IsDirty();

    for (const FName Archetype : SortedRequired)
    {
        const FGridWeaponLevelProfile* Profile = Rpg->FindProfile(Archetype, 1);
        if (!TestNotNull(TEXT("Authoritative level-1 profile"), Profile))
        {
            return false;
        }

        const FGridWeaponVisualProfile* Look = Visuals->FindProfileForArchetype(Archetype);
        const bool bMapped = Look && Look->IsValidProfile();
        const bool bIcon = bMapped && !Look->BaseIcon.IsNull() &&
            Look->BaseIcon.LoadSynchronous() != nullptr;
        const bool bMesh = bMapped && !Look->WorldMesh.IsNull() &&
            Look->WorldMesh.LoadSynchronous() != nullptr;
        const bool bVisualReady = bIcon && bMesh;

        const TArray<UGridItemDefinitionAsset*>* Matches = ByArchetype.Find(Archetype);
        const int32 ItemCount = Matches ? Matches->Num() : 0;
        UGridItemDefinitionAsset* Item = (ItemCount == 1) ? (*Matches)[0] : nullptr;
        if (ItemCount > 1)
        {
            ++DuplicateDefinitions;
        }
        if (ItemCount == 0)
        {
            ++MissingDefinitions;
        }
        if (!bVisualReady)
        {
            ++MissingVisuals;
        }

        const bool bDefinitionValid = Item &&
            Item->IsValidDefinition() &&
            Item->bUseWeaponCatalogue &&
            Item->WeaponCatalogue.LoadSynchronous() == Rpg &&
            Item->WeaponArchetypeId == Archetype &&
            Item->DefaultWeaponItemLevel == 1 &&
            Item->bUseWeaponVisualCatalogue &&
            Item->WeaponVisualCatalogue.LoadSynchronous() == Visuals &&
            !Item->bStackable &&
            FMath::IsNearlyEqual(Item->Weight, Profile->WeightKg);

        // Advisory only: throwing items and passive foci require separate
        // feature-specific tests. This does NOT certify runtime readiness.
        const bool bActionChecked = bDefinitionValid &&
            (Profile->DamageDice.bEnabled
                ? (Item->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand) ||
                   Item->CanProvideAttackFromSlot(EGridEquipmentSlot::OffHand))
                : Item->CombatActions.IsEmpty());

        const TCHAR* Status =
            ItemCount > 1 ? TEXT("DuplicateDefinitions") :
            ItemCount == 0 && !bVisualReady ? TEXT("MissingItemAndVisual") :
            ItemCount == 0 ? TEXT("MissingItem") :
            !bVisualReady ? TEXT("MissingVisual") :
            !bDefinitionValid ? TEXT("InvalidDefinition") :
            !bActionChecked ? TEXT("NeedsActionReview") :
            TEXT("CandidateForPlaytest");

        const bool bCandidate = FCString::Strcmp(Status, TEXT("CandidateForPlaytest")) == 0;
        if (bCandidate)
        {
            ++Candidates;
        }

        if (ARMEMENT01101::IsPilot(Archetype))
        {
            TestTrue(TEXT("Existing pilot remains a valid playtest candidate"), bCandidate);
            if (bCandidate)
            {
                ++VerifiedPilots;
            }
        }
        else if (ItemCount > 0)
        {
            // No newly authored item may silently pass with an incomplete
            // definition or missing visual. Missing items are expected today.
            TestTrue(TEXT("Additional definition must pass the production preflight"),
                bCandidate);
        }

        CsvReport += FString::Printf(TEXT("%s,1,%d,%d,%d,%d,%s,%d,%d,%s\n"),
            *Archetype.ToString(), bMapped ? 1 : 0, bIcon ? 1 : 0, bMesh ? 1 : 0,
            ItemCount, Item ? *Item->ItemDefinitionId.ToString() : TEXT("None"),
            bDefinitionValid ? 1 : 0, bActionChecked ? 1 : 0, Status);
        AddInfo(FString::Printf(TEXT("[ARMEMENT01.10.1] %s -> %s"),
            *Archetype.ToString(), Status));
    }

    const FString ReportDirectory =
        FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ARMEMENT01"));
    IFileManager::Get().MakeDirectory(*ReportDirectory, true);
    const FString ReportPath =
        FPaths::Combine(ReportDirectory, TEXT("ARMEMENT01_10_ProductionReadiness.csv"));
    TestTrue(TEXT("Read-only production readiness report was written under Saved"),
        FFileHelper::SaveStringToFile(CsvReport, *ReportPath,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));

    TestEqual(TEXT("Three existing pilots must survive unchanged"), VerifiedPilots, 3);
    TestEqual(TEXT("No conflicting item definitions by archetype"), DuplicateDefinitions, 0);
    TestEqual(TEXT("Read-only audit leaves RPG catalogue untouched"),
        Rpg->GetOutermost()->IsDirty(), bRpgDirtyBefore);
    TestEqual(TEXT("Read-only audit leaves visual catalogue untouched"),
        Visuals->GetOutermost()->IsDirty(), bVisualDirtyBefore);

    AddInfo(FString::Printf(
        TEXT("[ARMEMENT01.10.1] 22 N1 archetypes; %d candidates, %d without item, %d without complete visuals; report=%s"),
        Candidates, MissingDefinitions, MissingVisuals, *ReportPath));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

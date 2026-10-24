#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/Csv/CsvParser.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENT01111
{
    constexpr const TCHAR* RpgPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue");
    constexpr const TCHAR* VisualPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue");
    constexpr const TCHAR* WeaponFolder = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons");

    // Only canonical identities and approved recipe placements. Never duplicate workbook stats.
    struct FSpec
    {
        const TCHAR* ArchetypeId;
        const TCHAR* ClassId;
        const TCHAR* BranchId;
        const TCHAR* RequiredSlot;
    };

    static const FSpec Specs[] = {
        { TEXT("ARM002"), TEXT("Rogue"), TEXT("Assassin"), TEXT("MainHand") },
        { TEXT("ARM003"), TEXT("Rogue"), TEXT("Saboteur"), TEXT("MainHand") },
        { TEXT("ARM013"), TEXT("Rogue"), TEXT("Shadow"), TEXT("OffHand") },
        { TEXT("ARM020"), TEXT("Priest"), TEXT("Protection"), TEXT("MainHand") }
    };

    bool IsApprovedPlacement(const FSpec& Spec, const TArray<TArray<const TCHAR*>>& Rows)
    {
        int32 Matches = 0;
        for (int32 Index = 1; Index < Rows.Num(); ++Index)
        {
            const TArray<const TCHAR*>& Row = Rows[Index];
            if (Row.Num() != 15 || !Row[0] || !*Row[0])
            {
                continue;
            }
            if (FCString::Strcmp(Row[0], Spec.ClassId) != 0 ||
                FCString::Strcmp(Row[1], Spec.BranchId) != 0 ||
                FCString::Strcmp(Row[13], TEXT("ApprovedDesign")) != 0)
            {
                continue;
            }

            const bool bFirst = FCString::Strcmp(Row[3], Spec.ArchetypeId) == 0 &&
                FCString::Strcmp(Row[5], Spec.RequiredSlot) == 0 &&
                FCString::Strcmp(Row[4], TEXT("1")) == 0;
            const bool bSecond = FCString::Strcmp(Row[6], Spec.ArchetypeId) == 0 &&
                FCString::Strcmp(Row[8], Spec.RequiredSlot) == 0 &&
                FCString::Strcmp(Row[7], TEXT("1")) == 0;
            Matches += (bFirst || bSecond) ? 1 : 0;
        }
        return Matches == 1;
    }

    FString AsCsvBool(bool bValue)
    {
        return bValue ? TEXT("true") : TEXT("false");
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01111ApprovedSources,
    "Grimrock.RPG.ARMEMENT01.11.1.ApprovedN1Sources",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01111ApprovedSources::RunTest(const FString&)
{
    FString Csv;
    const FString Manifest = FPaths::Combine(
        FPaths::ProjectDir(), TEXT("docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv"));
    if (!TestTrue(TEXT("Canonical approved kit manifest is readable"),
        FFileHelper::LoadFileToString(Csv, *Manifest)))
    {
        return false;
    }

    FCsvParser Parser(Csv);
    const auto& Rows = Parser.GetRows();
    if (!TestTrue(TEXT("Manifest schema is unchanged"),
        Rows.Num() >= 19 && Rows[0].Num() == 15 &&
        FCString::Strcmp(Rows[0][3], TEXT("Weapon1ArchetypeId")) == 0 &&
        FCString::Strcmp(Rows[0][6], TEXT("Weapon2ArchetypeId")) == 0 &&
        FCString::Strcmp(Rows[0][13], TEXT("ApprovalState")) == 0))
    {
        return false;
    }

    UGridWeaponCatalogueAsset* Rpg =
        LoadObject<UGridWeaponCatalogueAsset>(nullptr, ARMEMENT01111::RpgPath);
    if (!TestNotNull(TEXT("Canonical N1 catalogue"), Rpg) ||
        !TestTrue(TEXT("Catalogue profiles are valid"), Rpg->IsValidCatalogue()) ||
        !TestTrue(TEXT("Class affinities are valid"), Rpg->IsValidClassAffinityData()))
    {
        return false;
    }

    const bool bCatalogueInitiallyDirty = Rpg->GetOutermost()->IsDirty();
    TSet<FName> Seen;
    for (const ARMEMENT01111::FSpec& Spec : ARMEMENT01111::Specs)
    {
        const FName Archetype(Spec.ArchetypeId);
        TestFalse(TEXT("No repeated archetype in this batch"), Seen.Contains(Archetype));
        Seen.Add(Archetype);

        TestTrue(*FString::Printf(TEXT("%s is an approved N1 kit placement"), Spec.ArchetypeId),
            ARMEMENT01111::IsApprovedPlacement(Spec, Rows));

        const FGridWeaponLevelProfile* Profile = Rpg->FindProfile(Archetype, 1);
        if (!TestNotNull(*FString::Printf(TEXT("%s has a real level-one profile"), Spec.ArchetypeId), Profile))
        {
            continue;
        }

        TestTrue(TEXT("N1 profile contains valid offensive dice"),
            Profile->IsValidProfile() && Profile->DamageDice.bEnabled);
        TestEqual(TEXT("Approved melee weapons use one hand"), Profile->HandUsage, FString(TEXT("1 main")));
        FGridAttackDefinition Attack;
        TestTrue(TEXT("Attack definition comes from N1 profile"), Profile->TryBuildAttackDefinition(Attack));

        const EGridWeaponKitAffinity Affinity =
            Rpg->GetKitRecommendationAffinity(Archetype, FName(Spec.ClassId));
        TestTrue(TEXT("Class has unqualified primary/secondary affinity"),
            Affinity == EGridWeaponKitAffinity::Primary ||
            Affinity == EGridWeaponKitAffinity::Secondary);
    }
    TestEqual(TEXT("Exactly four distinct melee archetypes"), Seen.Num(), 4);
    TestEqual(TEXT("RPG catalogue package remains untouched"),
        Rpg->GetOutermost()->IsDirty(), bCatalogueInitiallyDirty);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01111Readiness,
    "Grimrock.RPG.ARMEMENT01.11.1.MeleeAssetReadiness",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01111Readiness::RunTest(const FString&)
{
    UGridWeaponCatalogueAsset* Rpg =
        LoadObject<UGridWeaponCatalogueAsset>(nullptr, ARMEMENT01111::RpgPath);
    UGridWeaponVisualCatalogueAsset* Visuals =
        LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, ARMEMENT01111::VisualPath);
    if (!TestNotNull(TEXT("Canonical RPG catalogue"), Rpg) ||
        !TestNotNull(TEXT("Canonical visual catalogue"), Visuals) ||
        !TestTrue(TEXT("RPG catalogue structurally valid"), Rpg->IsValidCatalogue()) ||
        !TestTrue(TEXT("Visual catalogue structurally valid"), Visuals->IsValidCatalogue()))
    {
        return false;
    }

    const bool bRpgInitiallyDirty = Rpg->GetOutermost()->IsDirty();
    const bool bVisualInitiallyDirty = Visuals->GetOutermost()->IsDirty();

    IAssetRegistry& Registry =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    Registry.ScanPathsSynchronous({ FString(ARMEMENT01111::WeaponFolder) }, true);
    TArray<FAssetData> Assets;
    Registry.GetAssetsByPath(FName(ARMEMENT01111::WeaponFolder), Assets, true);

    TArray<FString> Lines;
    Lines.Add(TEXT("ArchetypeId,VisualMapped,IconLoaded,WorldMeshLoaded,ItemCount,ItemAsset,ItemDefinitionId,ItemValid,MainHandAttack,OffHandAttack,Status"));

    for (const ARMEMENT01111::FSpec& Spec : ARMEMENT01111::Specs)
    {
        const FName Archetype(Spec.ArchetypeId);
        const FGridWeaponLevelProfile* Profile = Rpg->FindProfile(Archetype, 1);
        if (!TestNotNull(TEXT("N1 profile is available to readiness audit"), Profile))
        {
            continue;
        }

        const FName VisualId = Visuals->FindVisualId(Archetype);
        const FGridWeaponVisualProfile* Visual = Visuals->FindProfileForArchetype(Archetype);
        const bool bVisualMapped = !VisualId.IsNone() && Visual && Visual->IsValidProfile();
        const bool bIconLoaded = bVisualMapped && Visual->BaseIcon.LoadSynchronous() != nullptr;
        const bool bMeshLoaded = bVisualMapped && Visual->WorldMesh.LoadSynchronous() != nullptr;
        const bool bVisualReady = bVisualMapped && bIconLoaded && bMeshLoaded;

        int32 ItemCount = 0;
        UGridItemDefinitionAsset* Item = nullptr;
        for (const FAssetData& Data : Assets)
        {
            UGridItemDefinitionAsset* Candidate = Cast<UGridItemDefinitionAsset>(Data.GetAsset());
            if (Candidate && Candidate->WeaponArchetypeId == Archetype)
            {
                ++ItemCount;
                Item = Candidate;
            }
        }

        TestTrue(TEXT("No duplicate definitions for a canonical archetype"), ItemCount <= 1);
        bool bItemValid = false;
        bool bMainAttack = false;
        bool bOffAttack = false;
        FString ItemAsset;
        FString ItemId;
        if (ItemCount == 1 && Item)
        {
            ItemAsset = Item->GetName();
            ItemId = Item->ItemDefinitionId.ToString();
            bItemValid = Item->IsValidDefinition() && !Item->bStackable &&
                Item->bUseWeaponCatalogue && Item->bUseWeaponVisualCatalogue &&
                Item->DefaultWeaponItemLevel == 1 &&
                Item->WeaponCatalogue.LoadSynchronous() == Rpg &&
                Item->WeaponVisualCatalogue.LoadSynchronous() == Visuals &&
                Item->GetEffectiveHandUsage() == EGridItemHandUsage::OneHanded &&
                Item->CanEquipToSlot(Spec.RequiredSlot == FString(TEXT("OffHand"))
                    ? EGridEquipmentSlot::OffHand : EGridEquipmentSlot::MainHand) &&
                FMath::IsNearlyEqual(Item->Weight, Profile->WeightKg);
            bMainAttack = Item->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand);
            bOffAttack = Item->CanProvideAttackFromSlot(EGridEquipmentSlot::OffHand);
            TestTrue(TEXT("Present definition follows canonical N1 data and intended slot"), bItemValid);
            if (FCString::Strcmp(Spec.RequiredSlot, TEXT("MainHand")) == 0)
            {
                TestTrue(TEXT("Present main-hand melee weapon provides an attack"), bMainAttack);
            }
        }

        FString Status;
        if (ItemCount > 1)
        {
            Status = TEXT("DuplicateDefinitions");
        }
        else if (ItemCount == 1 && !bItemValid)
        {
            Status = TEXT("InvalidDefinition");
        }
        else if (!bVisualReady && ItemCount == 0)
        {
            Status = TEXT("MissingItemAndVisual");
        }
        else if (ItemCount == 0)
        {
            Status = TEXT("MissingItem");
        }
        else if (!bVisualReady)
        {
            Status = TEXT("MissingVisual");
        }
        else if (Archetype == TEXT("ARM013") && !bOffAttack)
        {
            Status = TEXT("NeedsOffHandActionReview");
        }
        else if (Archetype == TEXT("ARM002") || Archetype == TEXT("ARM003") ||
                 Archetype == TEXT("ARM013"))
        {
            Status = TEXT("NeedsSpecialBehaviorReview");
        }
        else
        {
            Status = TEXT("CandidateForPlaytest");
        }

        Lines.Add(FString::Printf(TEXT("%s,%s,%s,%s,%d,%s,%s,%s,%s,%s,%s"),
            Spec.ArchetypeId, *ARMEMENT01111::AsCsvBool(bVisualMapped),
            *ARMEMENT01111::AsCsvBool(bIconLoaded), *ARMEMENT01111::AsCsvBool(bMeshLoaded),
            ItemCount, *ItemAsset, *ItemId,
            *ARMEMENT01111::AsCsvBool(bItemValid),
            *ARMEMENT01111::AsCsvBool(bMainAttack),
            *ARMEMENT01111::AsCsvBool(bOffAttack), *Status));
        AddInfo(FString::Printf(TEXT("[ARMEMENT01.11.1] %s: %s"), Spec.ArchetypeId, *Status));
    }

    const FString ReportDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ARMEMENT01"));
    IFileManager::Get().MakeDirectory(*ReportDir, true);
    const FString ReportPath = FPaths::Combine(ReportDir, TEXT("ARMEMENT01_11_1_MeleeReadiness.csv"));
    TestTrue(TEXT("Write factual local readiness report under Saved"),
        FFileHelper::SaveStringArrayToFile(Lines, *ReportPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));

    TestEqual(TEXT("RPG catalogue remains unchanged"), Rpg->GetOutermost()->IsDirty(), bRpgInitiallyDirty);
    TestEqual(TEXT("Visual catalogue remains unchanged"), Visuals->GetOutermost()->IsDirty(), bVisualInitiallyDirty);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

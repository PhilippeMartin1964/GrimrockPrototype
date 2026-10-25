#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridAssassinDaggerN1ItemAuthoringSpecs.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENT011122Tests
{
    const TCHAR* const RpgPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue");
    const TCHAR* const VisualsPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue");
    const TCHAR* const Folder = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT011122SourceContract,
    "Grimrock.RPG.ARMEMENT01.11.2.2.AssassinDaggerSource",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT011122SourceContract::RunTest(const FString&)
{
    UGridWeaponCatalogueAsset* Rpg =
        LoadObject<UGridWeaponCatalogueAsset>(nullptr, ARMEMENT011122Tests::RpgPath);
    UGridWeaponVisualCatalogueAsset* Visuals =
        LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, ARMEMENT011122Tests::VisualsPath);
    if (!TestNotNull(TEXT("Canonical RPG catalogue"), Rpg) ||
        !TestNotNull(TEXT("Canonical visual catalogue"), Visuals))
    {
        return false;
    }
    const bool bRpgDirty = Rpg->GetOutermost()->IsDirty();
    const bool bVisualDirty = Visuals->GetOutermost()->IsDirty();
    TestTrue(TEXT("Catalogue structurally valid"), Rpg->IsValidCatalogue());
    TestTrue(TEXT("Class affinities valid"), Rpg->IsValidClassAffinityData());
    TestTrue(TEXT("Visual catalogue structurally valid"), Visuals->IsValidCatalogue());

    const FGridWeaponLevelProfile* N1 = Rpg->FindProfile(TEXT("ARM002"), 1);
    if (!TestNotNull(TEXT("ARM002 level-one profile"), N1))
    {
        return false;
    }
    TestTrue(TEXT("Real offensive profile"), N1->IsValidProfile() && N1->DamageDice.bEnabled);
    TestEqual(TEXT("One handed in canonical N1 profile"), N1->HandUsage, FString(TEXT("1 main")));
    FGridAttackDefinition Attack;
    TestTrue(TEXT("N1 damage dice resolve into attack"), N1->TryBuildAttackDefinition(Attack));
    const EGridWeaponKitAffinity Affinity =
        Rpg->GetKitRecommendationAffinity(TEXT("ARM002"), TEXT("Rogue"));
    TestTrue(TEXT("Approved Rogue affinity"),
        Affinity == EGridWeaponKitAffinity::Primary ||
        Affinity == EGridWeaponKitAffinity::Secondary);

    TestEqual(TEXT("Already registered ARM002 visual"), Visuals->FindVisualId(TEXT("ARM002")),
        FName(TEXT("VIS_AssassinDagger_01")));
    const FGridWeaponVisualProfile* Look = Visuals->FindProfileForArchetype(TEXT("ARM002"));
    if (!TestNotNull(TEXT("ARM002 real visual profile"), Look))
    {
        return false;
    }
    TestTrue(TEXT("Real icon loads"), Look->BaseIcon.LoadSynchronous() != nullptr);
    TestTrue(TEXT("Real world mesh loads"), Look->WorldMesh.LoadSynchronous() != nullptr);
    TestEqual(TEXT("RPG package untouched"), Rpg->GetOutermost()->IsDirty(), bRpgDirty);
    TestEqual(TEXT("Visual package untouched"), Visuals->GetOutermost()->IsDirty(), bVisualDirty);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT011122Definition,
    "Grimrock.RPG.ARMEMENT01.11.2.2.AssassinDaggerDefinition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT011122Definition::RunTest(const FString&)
{
    UGridWeaponCatalogueAsset* Rpg =
        LoadObject<UGridWeaponCatalogueAsset>(nullptr, ARMEMENT011122Tests::RpgPath);
    UGridWeaponVisualCatalogueAsset* Visuals =
        LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, ARMEMENT011122Tests::VisualsPath);
    if (!TestNotNull(TEXT("Canonical RPG catalogue"), Rpg) ||
        !TestNotNull(TEXT("Canonical visual catalogue"), Visuals))
    {
        return false;
    }
    const bool bRpgDirty = Rpg->GetOutermost()->IsDirty();
    const bool bVisualDirty = Visuals->GetOutermost()->IsDirty();

    IAssetRegistry& Registry =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    TArray<FString> Paths = { FString(ARMEMENT011122Tests::Folder) };
    Registry.ScanPathsSynchronous(Paths, true);
    TArray<FAssetData> Assets;
    Registry.GetAssetsByPath(FName(ARMEMENT011122Tests::Folder), Assets, true);
    UGridItemDefinitionAsset* Item = nullptr;
    int32 Found = 0;
    for (const FAssetData& Asset : Assets)
    {
        UGridItemDefinitionAsset* Candidate = Cast<UGridItemDefinitionAsset>(Asset.GetAsset());
        if (Candidate && Candidate->WeaponArchetypeId == TEXT("ARM002"))
        {
            ++Found;
            Item = Candidate;
        }
    }
    TestTrue(TEXT("No duplicate ARM002 definition"), Found <= 1);
    if (Found == 0)
    {
        AddInfo(TEXT("[ARMEMENT01.11.2.2] ARM002 MissingItem; audit passed but authoring not applied"));
    }
    else if (TestNotNull(TEXT("Actual ARM002 item"), Item))
    {
        const FGridWeaponLevelProfile* N1 = Rpg->FindProfile(TEXT("ARM002"), 1);
        if (!TestNotNull(TEXT("Real N1 profile"), N1))
        {
            return false;
        }
        const FGridWeaponVisualProfile* Visual = Visuals->FindProfileForArchetype(TEXT("ARM002"));
        if (!TestNotNull(TEXT("Real visual profile"), Visual))
        {
            return false;
        }

        TestEqual(TEXT("Exact DataAsset name"), Item->GetName(),
            FString(TEXT("DA_Weapon_AssassinDagger_01")));
        TestEqual(TEXT("Stable item ID"), Item->ItemDefinitionId,
            FName(TEXT("Weapon_AssassinDagger_01")));
        TestEqual(TEXT("N1 default level"), Item->DefaultWeaponItemLevel, 1);
        TestEqual(TEXT("Weapon archetype"), Item->WeaponArchetypeId, FName(TEXT("ARM002")));
        TestEqual(TEXT("Item type"), Item->ItemType, EGridItemType::Weapon);
        TestEqual(TEXT("N1 weight"), Item->Weight, N1->WeightKg);
        TestEqual(TEXT("N1 display name"), Item->DisplayName.ToString(), N1->DisplayName.ToString());
        TestFalse(TEXT("Non-stackable"), Item->bStackable);
        TestEqual(TEXT("One-handed physical usage"), Item->GetEffectiveHandUsage(),
            EGridItemHandUsage::OneHanded);
        TestTrue(TEXT("Explicit MainHand slot"),
            Item->CompatibleEquipmentSlots.Contains(EGridEquipmentSlot::MainHand));
        TestTrue(TEXT("Both existing catalogues enabled"),
            Item->bUseWeaponCatalogue && Item->bUseWeaponVisualCatalogue);
        TestEqual(TEXT("RPG catalogue identity"), Item->WeaponCatalogue.LoadSynchronous(), Rpg);
        TestEqual(TEXT("Visual catalogue identity"), Item->WeaponVisualCatalogue.LoadSynchronous(), Visuals);
        TestTrue(TEXT("Same visual icon"), Item->Icon.ToSoftObjectPath() ==
            Visual->BaseIcon.ToSoftObjectPath());
        TestTrue(TEXT("Same visual world mesh"), Item->WorldMesh.ToSoftObjectPath() ==
            Visual->WorldMesh.ToSoftObjectPath());
        TestTrue(TEXT("Definition valid"), Item->IsValidDefinition());
        TestTrue(TEXT("MainHand attack exists"), Item->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand));
        TestFalse(TEXT("No special OffHand attack invented"),
            Item->CanProvideAttackFromSlot(EGridEquipmentSlot::OffHand));
        TestFalse(TEXT("Not a combat throwing weapon"), Item->bCombatThrowWeapon);
        TestEqual(TEXT("Exactly one equipment attack"), Item->CombatActions.Num(), 1);
        if (Item->CombatActions.Num() == 1)
        {
            const FGridCombatActionDefinition& Action = Item->CombatActions[0];
            TestEqual(TEXT("Attack identity"), Action.ActionId,
                FName(TEXT("Attack_AssassinDagger_01")));
            TestEqual(TEXT("Source action point cost"), Action.ActionPointCost, N1->ActionPointCost);
            TestEqual(TEXT("Source range"), Action.RangeCells, N1->RangeCells);
            TestTrue(TEXT("Valid equipment melee action"), Action.IsValid() &&
                Action.SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
                Action.ActionType == EGridCombatActionType::MeleeAttack &&
                Action.ResolutionProfile == EGridCombatActionResolutionProfile::Attack);
        }
    }
    TestEqual(TEXT("RPG package remains untouched"), Rpg->GetOutermost()->IsDirty(), bRpgDirty);
    TestEqual(TEXT("Visual package remains untouched"), Visuals->GetOutermost()->IsDirty(), bVisualDirty);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

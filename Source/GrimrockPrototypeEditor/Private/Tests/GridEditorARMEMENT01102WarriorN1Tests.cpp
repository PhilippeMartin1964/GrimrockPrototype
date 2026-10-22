#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"
#include "ARMEMENT/GridWarriorN1ItemAuthoringSpecs.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetData.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENT01102Tests
{
    const TCHAR* const RpgPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue");
    const TCHAR* const VisualPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue");
    const TCHAR* const Folder = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons");

    UGridWeaponCatalogueAsset* LoadRpg()
    {
        return LoadObject<UGridWeaponCatalogueAsset>(nullptr, RpgPath);
    }

    bool ReadWarriorRecipes(TMap<FString, TPair<FName, FName>>& Out)
    {
        Out.Reset();
        FString Text;
        const FString Path = FPaths::Combine(
            FPaths::ProjectDir(), TEXT("docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv"));
        if (!FFileHelper::LoadFileToString(Text, *Path))
        {
            return false;
        }
        FCsvParser Parser(Text);
        const auto& Rows = Parser.GetRows();
        if (Rows.Num() < 2 || Rows[0].Num() != 15 ||
            FCString::Strcmp(Rows[0][3], TEXT("Weapon1ArchetypeId")) != 0 ||
            FCString::Strcmp(Rows[0][6], TEXT("Weapon2ArchetypeId")) != 0)
        {
            return false;
        }

        for (int32 I = 1; I < Rows.Num(); ++I)
        {
            const TArray<const TCHAR*>& Row = Rows[I];
            if (Row.Num() != 15 || !Row[0] || !*Row[0])
            {
                continue;
            }
            if (FCString::Strcmp(Row[0], TEXT("Warrior")) == 0)
            {
                if (FCString::Strcmp(Row[13], TEXT("ApprovedDesign")) != 0 ||
                    Out.Contains(FString(Row[1])))
                {
                    return false;
                }
                Out.Add(FString(Row[1]), TPair<FName, FName>(FName(Row[3]), FName(Row[6])));
            }
        }
        return Out.Num() == 3;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01102WarriorSource,
    "Grimrock.RPG.ARMEMENT01.10.Warrior.ApprovedN1Source",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01102WarriorSource::RunTest(const FString&)
{
    TMap<FString, TPair<FName, FName>> Recipes;
    if (!TestTrue(TEXT("The three Warrior branches have approved source recipes"),
        ARMEMENT01102Tests::ReadWarriorRecipes(Recipes)))
    {
        return false;
    }

    const auto* Guardian = Recipes.Find(TEXT("Guardian"));
    const auto* Breaker = Recipes.Find(TEXT("Breaker"));
    const auto* Master = Recipes.Find(TEXT("WeaponMaster"));
    if (!TestNotNull(TEXT("Guardian recipe"), Guardian) ||
        !TestNotNull(TEXT("Breaker recipe"), Breaker) ||
        !TestNotNull(TEXT("WeaponMaster recipe"), Master))
    {
        return false;
    }
    TestEqual(TEXT("Guardian primary"), Guardian->Key, FName(TEXT("ARM014")));
    TestEqual(TEXT("Guardian legacy pilot shield"), Guardian->Value, FName(TEXT("ARM075")));
    TestEqual(TEXT("Breaker great axe"), Breaker->Key, FName(TEXT("ARM031")));
    TestEqual(TEXT("Breaker has no second weapon"), Breaker->Value, FName(TEXT("None")));
    TestEqual(TEXT("WeaponMaster long sword"), Master->Key, FName(TEXT("ARM015")));
    TestEqual(TEXT("WeaponMaster secondary war pick"), Master->Value, FName(TEXT("ARM025")));

    UGridWeaponCatalogueAsset* Rpg = ARMEMENT01102Tests::LoadRpg();
    if (!TestNotNull(TEXT("1640-profile production catalogue"), Rpg) ||
        !TestTrue(TEXT("Catalogue valid"), Rpg->IsValidCatalogue()) ||
        !TestTrue(TEXT("Workbook class preferences valid"), Rpg->IsValidClassAffinityData()))
    {
        return false;
    }

    TSet<FName> UniqueArchetypes;
    TSet<FName> UniqueItemIds;
    for (const ARMEMENT01102::FWarriorItemSpec& Spec : ARMEMENT01102::WarriorItems)
    {
        const FName Archetype(Spec.ArchetypeId);
        TestFalse(TEXT("Every archetype is unique"), UniqueArchetypes.Contains(Archetype));
        UniqueArchetypes.Add(Archetype);
        TestFalse(TEXT("Every ItemDefinitionId is unique"), UniqueItemIds.Contains(FName(Spec.ItemDefinitionId)));
        UniqueItemIds.Add(FName(Spec.ItemDefinitionId));

        const FGridWeaponLevelProfile* Profile = Rpg->FindProfile(Archetype, 1);
        if (!TestNotNull(TEXT("Real N1 source profile"), Profile))
        {
            return false;
        }
        TestTrue(TEXT("Offensive N1 profile is valid"), Profile->IsValidProfile() && Profile->DamageDice.bEnabled);
        TestTrue(TEXT("Melee weapon hand usage derives from the workbook"),
            Profile->HandUsage == TEXT("1 main") || Profile->HandUsage == TEXT("2 mains"));
        const EGridWeaponKitAffinity Affinity =
            Rpg->GetKitRecommendationAffinity(Archetype, TEXT("Warrior"));
        TestTrue(TEXT("Approved Warrior affinity, no qualified exception"),
            Affinity == EGridWeaponKitAffinity::Primary ||
            Affinity == EGridWeaponKitAffinity::Secondary);
        FGridAttackDefinition Attack;
        TestTrue(TEXT("Real N1 melee attack can be resolved"), Profile->TryBuildAttackDefinition(Attack));
        if (Archetype == TEXT("ARM031"))
        {
            TestEqual(TEXT("Breaker great axe must truly use both hands"), Profile->HandUsage,
                FString(TEXT("2 mains")));
        }
    }
    TestEqual(TEXT("Exactly four new Warrior archetypes"), UniqueArchetypes.Num(), 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01102WarriorGuard,
    "Grimrock.RPG.ARMEMENT01.10.Warrior.ExistingAssetsGuard",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01102WarriorGuard::RunTest(const FString&)
{
    UGridWeaponCatalogueAsset* Rpg = ARMEMENT01102Tests::LoadRpg();
    UGridWeaponVisualCatalogueAsset* Visuals =
        LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, ARMEMENT01102Tests::VisualPath);
    if (!TestNotNull(TEXT("Production RPG catalogue"), Rpg) ||
        !TestNotNull(TEXT("Production visual catalogue"), Visuals))
    {
        return false;
    }

    const bool bRpgDirty = Rpg->GetOutermost()->IsDirty();
    const bool bVisualsDirty = Visuals->GetOutermost()->IsDirty();
    IAssetRegistry& Registry =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    TArray<FString> Paths = { FString(ARMEMENT01102Tests::Folder) };
    Registry.ScanPathsSynchronous(Paths, true);
    TArray<FAssetData> Assets;
    Registry.GetAssetsByPath(FName(ARMEMENT01102Tests::Folder), Assets, true);

    int32 Present = 0;
    for (const ARMEMENT01102::FWarriorItemSpec& Spec : ARMEMENT01102::WarriorItems)
    {
        int32 Matches = 0;
        UGridItemDefinitionAsset* Item = nullptr;
        for (const FAssetData& AssetData : Assets)
        {
            UGridItemDefinitionAsset* Candidate = Cast<UGridItemDefinitionAsset>(AssetData.GetAsset());
            if (Candidate && Candidate->WeaponArchetypeId == FName(Spec.ArchetypeId))
            {
                ++Matches;
                Item = Candidate;
            }
        }
        TestTrue(TEXT("At most one authored item per archetype"), Matches <= 1);
        if (Matches == 0)
        {
            AddInfo(FString::Printf(TEXT("[ARMEMENT01.10.2] %s remains unmaterialized"),
                Spec.ArchetypeId));
            continue;
        }

        ++Present;
        if (!TestNotNull(TEXT("Already authored item"), Item))
        {
            return false;
        }
        TestEqual(TEXT("Stable item identity"), Item->ItemDefinitionId, FName(Spec.ItemDefinitionId));
        TestEqual(TEXT("Expected asset name"), Item->GetName(), FString(Spec.AssetName));
        TestEqual(TEXT("Level one only"), Item->DefaultWeaponItemLevel, 1);
        TestTrue(TEXT("Valid single equipment definition"), Item->IsValidDefinition() && !Item->bStackable);
        TestEqual(TEXT("Uses existing RPG catalogue"), Item->WeaponCatalogue.LoadSynchronous(), Rpg);
        TestEqual(TEXT("Uses existing visual catalogue"), Item->WeaponVisualCatalogue.LoadSynchronous(), Visuals);
        TestTrue(TEXT("Both catalogues explicitly enabled"),
            Item->bUseWeaponCatalogue && Item->bUseWeaponVisualCatalogue);
        TestTrue(TEXT("Combat item has a main-hand attack"),
            Item->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand));
        TestEqual(TEXT("Exactly one equipment attack"), Item->CombatActions.Num(), 1);
        if (Item->CombatActions.Num() == 1)
        {
            TestEqual(TEXT("Stable attack identity"), Item->CombatActions[0].ActionId, FName(Spec.AttackId));
            const FGridWeaponLevelProfile* Profile = Rpg->FindProfile(FName(Spec.ArchetypeId), 1);
            if (!TestNotNull(TEXT("N1 profile"), Profile)) return false;
            TestEqual(TEXT("Actual AP cost, not guessed"), Item->CombatActions[0].ActionPointCost, Profile->ActionPointCost);
            TestEqual(TEXT("Actual attack range"), Item->CombatActions[0].RangeCells, Profile->RangeCells);
        }
    }
    AddInfo(FString::Printf(TEXT("[ARMEMENT01.10.2] %d/4 items already materialized (absences are not successes)"),
        Present));
    TestEqual(TEXT("Catalogue remains untouched"), Rpg->GetOutermost()->IsDirty(), bRpgDirty);
    TestEqual(TEXT("Visual catalogue remains untouched"), Visuals->GetOutermost()->IsDirty(), bVisualsDirty);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridItemInstanceMaterializationReadModel.h"
#include "../Runtime/GridItemWeightReadModel.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AGMaterialization,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalInstanceMaterialization",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063AGMaterialization::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real ItemSystem"), Root.Get())) return false;
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Register real canonical registry"), Registry->RegisterItemSystem(Root.Get(), Errors)))
    {
        for (const FString& Error : Errors) AddError(Error);
        return false;
    }

    TStrongObjectPtr<UGridItemDefinitionAsset> Old(NewObject<UGridItemDefinitionAsset>());
    Old->DisplayName = FText::FromString(TEXT("WRONG OLD NAME"));
    Old->Weight = 999.0f;
    Old->bUseWeaponCatalogue = true;
    Old->DefaultWeaponItemLevel = 6;
    // Deliberately no old WeaponCatalogue: registered IDs must not need it.
    const FName Arms[] = {
        TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
        TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01") };
    for (const FName Id : Arms)
    {
        Old->ItemDefinitionId = Id;
        for (const int32 Level : {1, 20})
        {
            FGridResolvedItem Canonical;
            if (!TestTrue(TEXT("Production ARM profile resolves"), Registry->ResolveItem(Id, Level, Canonical)))
                return false;
            FGridItemInstance Item;
            Item.RuntimeObjectId = FGuid::NewGuid();
            Item.ItemDefinitionId = Id;
            Item.ItemLevel = Level;
            Item.Quantity = 3;
            Item.OwnerType = EGridItemOwnerType::Cursor;
            Item.OwnerGuid = FGuid::NewGuid();
            Item.AbsorbedEquipmentPhysicalArmor = 2;
            Item.AbsorbedEquipmentMagicalArmor = 1;
            Item.bLightsEnabled = true;
            const FGridItemInstance Before = Item;
            TestTrue(TEXT("Canonical materialization succeeds without old profile"),
                FGridItemInstanceMaterializationReadModel::TryApply(Registry.Get(), Old.Get(), Item));
            TestEqual(TEXT("Keeps actual fixed N-level"), Item.ItemLevel, Level);
            TestTrue(TEXT("Cached weight from ItemSystem"), FMath::IsNearlyEqual(Item.Weight, Canonical.WeightKg));
            TestEqual(TEXT("Fresh snapshot name canonical"), Item.DisplayName.ToString(),
                Canonical.DisplayName.ToString());
            TestEqual(TEXT("Physical GUID stable"), Item.RuntimeObjectId, Before.RuntimeObjectId);
            TestEqual(TEXT("Owner GUID stable"), Item.OwnerGuid, Before.OwnerGuid);
            TestEqual(TEXT("Owner type stable"), Item.OwnerType, Before.OwnerType);
            TestEqual(TEXT("Quantity unchanged"), Item.Quantity, Before.Quantity);
            TestEqual(TEXT("Physical armor debt kept"),
                Item.AbsorbedEquipmentPhysicalArmor, Before.AbsorbedEquipmentPhysicalArmor);
            TestEqual(TEXT("Magical armor debt kept"),
                Item.AbsorbedEquipmentMagicalArmor, Before.AbsorbedEquipmentMagicalArmor);
            TestEqual(TEXT("Lights unaffected"), Item.bLightsEnabled, Before.bLightsEnabled);
            float UnitWeight = 0.0f;
            TestTrue(TEXT("Existing inventory weight reader resolves"),
                FGridItemWeightReadModel::TryResolveUnitWeight(Registry.Get(), Item, UnitWeight));
            TestTrue(TEXT("Cached and computed unit weights match"),
                FMath::IsNearlyEqual(Item.Weight, UnitWeight));

            Item.DisplayName = FText::FromString(TEXT("CUSTOM SAVED LABEL"));
            Item.Weight = -123.0f;
            TestTrue(TEXT("Refresh stale weight from canonical on re-entry"),
                FGridItemInstanceMaterializationReadModel::TryApply(Registry.Get(), Old.Get(), Item));
            TestEqual(TEXT("Preserve custom saved name until 06.4 migration"),
                Item.DisplayName.ToString(), FString(TEXT("CUSTOM SAVED LABEL")));
            TestTrue(TEXT("Stale snapshot weight corrected"),
                FMath::IsNearlyEqual(Item.Weight, Canonical.WeightKg));
        }
        FGridResolvedItem Default;
        if (!TestTrue(TEXT("Inherited canonical N6 exists"),
            Registry->ResolveItem(Id, 6, Default))) return false;
        FGridItemInstance Fresh;
        Fresh.RuntimeObjectId = FGuid::NewGuid();
        Fresh.ItemDefinitionId = Id;
        Fresh.ItemLevel = 0;
        Fresh.Quantity = 1;
        Old->bUseWeaponCatalogue = false; // Obsolete flag cannot override canonical ARM type.
        TestTrue(TEXT("ARM inherited default works without old flag"),
            FGridItemInstanceMaterializationReadModel::TryApply(Registry.Get(), Old.Get(), Fresh));
        TestEqual(TEXT("N0 authored ARM materializes exactly N6"), Fresh.ItemLevel, 6);
        TestTrue(TEXT("Inherited weight matches canonical definition"),
            FMath::IsNearlyEqual(Fresh.Weight, Default.WeightKg));
        Old->bUseWeaponCatalogue = true;
    }

    Old->ItemDefinitionId = TEXT("Weapon_RoundShield_01");
    Old->DefaultWeaponItemLevel = 0;
    FGridItemInstance Bad;
    Bad.RuntimeObjectId = FGuid::NewGuid();
    Bad.ItemDefinitionId = Old->ItemDefinitionId;
    Bad.ItemLevel = 0;
    Bad.Quantity = 1;
    Bad.Weight = 67.0f;
    Bad.DisplayName = FText::FromString(TEXT("PRIOR UNCHANGED"));
    const FGridItemInstance Original = Bad;
    TestFalse(TEXT("Canonical ARM invalid default N0 rejects"),
        FGridItemInstanceMaterializationReadModel::TryApply(Registry.Get(), Old.Get(), Bad));
    TestEqual(TEXT("Rejected level unchanged"), Bad.ItemLevel, Original.ItemLevel);
    TestEqual(TEXT("Rejected weight unchanged"), Bad.Weight, Original.Weight);
    TestEqual(TEXT("Rejected name unchanged"), Bad.DisplayName.ToString(),
        Original.DisplayName.ToString());
    Old->DefaultWeaponItemLevel = 6;
    Bad.ItemLevel = 21;
    TestFalse(TEXT("N21 does not downgrade to legacy"),
        FGridItemInstanceMaterializationReadModel::TryApply(Registry.Get(), Old.Get(), Bad));
    TestEqual(TEXT("Rejected N21 input unchanged"), Bad.ItemLevel, 21);

    Old->ItemDefinitionId = TEXT("Item_Arrows_01");
    Old->bUseWeaponCatalogue = true; // Intentionally wrong old flag.
    FGridItemInstance Ammo;
    Ammo.RuntimeObjectId = FGuid::NewGuid();
    Ammo.ItemDefinitionId = Old->ItemDefinitionId;
    Ammo.ItemLevel = 0;
    Ammo.Quantity = 12;
    FGridResolvedItem CanonicalArrow;
    if (!TestTrue(TEXT("Real arrow N0 resolves"),
        Registry->ResolveItem(Ammo.ItemDefinitionId, 0, CanonicalArrow))) return false;
    TestTrue(TEXT("Canonical arrows do not require old weapon catalogue"),
        FGridItemInstanceMaterializationReadModel::TryApply(Registry.Get(), Old.Get(), Ammo));
    TestEqual(TEXT("Arrows retain genuine N0"), Ammo.ItemLevel, 0);
    TestEqual(TEXT("Arrows quantity unchanged"), Ammo.Quantity, 12);
    TestEqual(TEXT("Arrow name canonical"), Ammo.DisplayName.ToString(),
        CanonicalArrow.DisplayName.ToString());
    TestTrue(TEXT("Arrow unit mass canonical"),
        FMath::IsNearlyEqual(Ammo.Weight, CanonicalArrow.WeightKg));

    Old->ItemDefinitionId = TEXT("UnregisteredItem_AG");
    Old->DisplayName = FText::FromString(TEXT("LEGACY ORIGINAL"));
    Old->Weight = 1.75f;
    Old->bUseWeaponCatalogue = false;
    FGridItemInstance Legacy;
    Legacy.RuntimeObjectId = FGuid::NewGuid();
    Legacy.ItemDefinitionId = Old->ItemDefinitionId;
    Legacy.Quantity = 1;
    TestTrue(TEXT("Unknown identity still uses legacy metadata"),
        FGridItemInstanceMaterializationReadModel::TryApply(Registry.Get(), Old.Get(), Legacy));
    TestTrue(TEXT("Legacy mass unchanged"), FMath::IsNearlyEqual(Legacy.Weight, 1.75f));
    TestEqual(TEXT("Legacy name unchanged"), Legacy.DisplayName.ToString(),
        FString(TEXT("LEGACY ORIGINAL")));
    Old->bUseWeaponCatalogue = true;
    const FGridItemInstance BeforeReject = Legacy;
    TestFalse(TEXT("Unknown linked item without old profile still rejects"),
        FGridItemInstanceMaterializationReadModel::TryApply(Registry.Get(), Old.Get(), Legacy));
    TestEqual(TEXT("Rejected legacy mass unchanged"), Legacy.Weight, BeforeReject.Weight);
    TestEqual(TEXT("Rejected legacy GUID unchanged"), Legacy.RuntimeObjectId, BeforeReject.RuntimeObjectId);
    return true;
}

#endif

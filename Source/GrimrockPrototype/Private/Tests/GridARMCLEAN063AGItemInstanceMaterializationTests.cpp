#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridItemInstanceMaterializationReadModel.h"
#include "../Runtime/GridItemWeightReadModel.h"
#include "../UI/GridInventoryItemMetadataReadModel.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AGMaterialization,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalInstanceMaterialization",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063AGMaterialization::RunTest(const FString&)
{
    TestNull(TEXT("No reflected instance name"), FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("DisplayName")));
    TestNull(TEXT("No reflected instance weight"), FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("Weight")));
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
            TestTrue(TEXT("All durable fields remain unchanged"),
                FGridItemInstance::StaticStruct()->CompareScriptStruct(&Item, &Before, 0));
            const auto Metadata = FGridInventoryItemMetadataReadModel::Resolve(Registry.Get(), Old.Get(), Item);
            TestTrue(TEXT("Canonical metadata resolves"), Metadata.bResolved);
            TestEqual(TEXT("Name from canonical reader"), Metadata.DisplayName.ToString(), Canonical.DisplayName.ToString());
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
            TestTrue(TEXT("Computed unit weight matches canonical source"),
                FMath::IsNearlyEqual(Canonical.WeightKg, UnitWeight));

            TestTrue(TEXT("Canonical re-entry remains idempotent"),
                FGridItemInstanceMaterializationReadModel::TryApply(Registry.Get(), Old.Get(), Item));
            TestTrue(TEXT("Repeated materialization leaves physical state intact"),
                FGridItemInstance::StaticStruct()->CompareScriptStruct(&Item, &Before, 0));
        }
        FGridItemInstance Fresh;
        Fresh.RuntimeObjectId = FGuid::NewGuid();
        Fresh.ItemDefinitionId = Id;
        Fresh.ItemLevel = 0;
        Fresh.Quantity = 1;
        const FGridItemInstance BeforeFresh = Fresh;
        Old->bUseWeaponCatalogue = false;
        TestFalse(TEXT("Registered physical ARM N0 cannot inherit obsolete N6"),
            FGridItemInstanceMaterializationReadModel::TryApply(Registry.Get(), Old.Get(), Fresh));
        TestEqual(TEXT("Refused level stays N0"), Fresh.ItemLevel, 0);
        TestTrue(TEXT("Refused durable fields remain unchanged"),
            FGridItemInstance::StaticStruct()->CompareScriptStruct(&Fresh, &BeforeFresh, 0));
        Old->bUseWeaponCatalogue = true;
    }

    Old->ItemDefinitionId = TEXT("Weapon_RoundShield_01");
    Old->DefaultWeaponItemLevel = 0;
    FGridItemInstance Bad;
    Bad.RuntimeObjectId = FGuid::NewGuid();
    Bad.ItemDefinitionId = Old->ItemDefinitionId;
    Bad.ItemLevel = 0;
    Bad.Quantity = 1;
    const FGridItemInstance Original = Bad;
    TestFalse(TEXT("Canonical ARM invalid default N0 rejects"),
        FGridItemInstanceMaterializationReadModel::TryApply(Registry.Get(), Old.Get(), Bad));
    TestEqual(TEXT("Rejected level unchanged"), Bad.ItemLevel, Original.ItemLevel);
    TestTrue(TEXT("Rejected durable fields unchanged"),
        FGridItemInstance::StaticStruct()->CompareScriptStruct(&Bad, &Original, 0));
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
    const auto ArrowMeta = FGridInventoryItemMetadataReadModel::Resolve(Registry.Get(), Old.Get(), Ammo);
    TestTrue(TEXT("Arrow metadata resolves"), ArrowMeta.bResolved);
    TestEqual(TEXT("Arrow name canonical"), ArrowMeta.DisplayName.ToString(), CanonicalArrow.DisplayName.ToString());
    TestTrue(TEXT("Arrow unit mass canonical"), FMath::IsNearlyEqual(ArrowMeta.UnitWeightKg, CanonicalArrow.WeightKg));

    // A stale historical identity cannot veto the actual canonical item.
    Old->ItemDefinitionId = TEXT("UnrelatedHistorical_AG");
    FGridItemInstance Contradictory = Ammo;
    TestTrue(TEXT("Canonical item resolves despite unrelated old pointer"),
        FGridItemInstanceMaterializationReadModel::TryApply(Registry.Get(), Old.Get(), Contradictory));
    TestEqual(TEXT("Canonical identity remains unchanged"), Contradictory.ItemDefinitionId, Ammo.ItemDefinitionId);
    TestTrue(TEXT("Unrelated old definition cannot change durable state"),
        FGridItemInstance::StaticStruct()->CompareScriptStruct(&Contradictory, &Ammo, 0));
    Old->ItemDefinitionId = Ammo.ItemDefinitionId;

    FGridItemInstance InvalidAmmo = Ammo;
    InvalidAmmo.ItemLevel = 1;
    const FGridItemInstance BeforeInvalidAmmo = InvalidAmmo;
    TestFalse(TEXT("Non-armament physical N1 cannot use registry family alias"),
        FGridItemInstanceMaterializationReadModel::TryApply(Registry.Get(), Old.Get(), InvalidAmmo));
    TestEqual(TEXT("Refused ammo level unchanged"), InvalidAmmo.ItemLevel, 1);
    TestTrue(TEXT("Refused ammo durable fields unchanged"),
        FGridItemInstance::StaticStruct()->CompareScriptStruct(&InvalidAmmo, &BeforeInvalidAmmo, 0));

    Old->ItemDefinitionId = TEXT("UnregisteredItem_AG");
    Old->DisplayName = FText::FromString(TEXT("LEGACY ORIGINAL"));
    Old->Weight = 1.75f;
    Old->bUseWeaponCatalogue = false;
    FGridItemInstance Legacy;
    Legacy.RuntimeObjectId = FGuid::NewGuid();
    Legacy.ItemDefinitionId = Old->ItemDefinitionId;
    Legacy.Quantity = 1;
    TestTrue(TEXT("Explicit Automation fixture uses historical metadata"),
        FGridItemInstanceMaterializationReadModel::TryApply(Registry.Get(), Old.Get(), Legacy));
    TestTrue(TEXT("Legacy mass unchanged"), FMath::IsNearlyEqual(Legacy.Weight, 1.75f));
    TestEqual(TEXT("Legacy name unchanged"), Legacy.DisplayName.ToString(),
        FString(TEXT("LEGACY ORIGINAL")));
    const FGridItemInstance BeforeReject = Legacy;
    Old->Weight = -0.5f;
    TestFalse(TEXT("Invalid fixture mass rejects materialization"),
        FGridItemInstanceMaterializationReadModel::TryApply(Registry.Get(), Old.Get(), Legacy));
    TestEqual(TEXT("Rejected fixture mass is unchanged"), Legacy.Weight, BeforeReject.Weight);
    Old->Weight = 1.75f;
    Old->bUseWeaponCatalogue = true;
    TestFalse(TEXT("Unknown linked item without old profile still rejects"),
        FGridItemInstanceMaterializationReadModel::TryApply(Registry.Get(), Old.Get(), Legacy));
    TestEqual(TEXT("Rejected legacy mass unchanged"), Legacy.Weight, BeforeReject.Weight);
    TestEqual(TEXT("Rejected legacy GUID unchanged"), Legacy.RuntimeObjectId, BeforeReject.RuntimeObjectId);
    return true;
}

#endif

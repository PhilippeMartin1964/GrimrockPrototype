#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../UI/GridStartingKitDisplayReadModel.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AFStartingKitDisplay,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalStartingKitDisplayByActualLevel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063AFStartingKitDisplay::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real published ItemSystem"), Root.Get())) return false;
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Register actual ItemSystem"), Registry->RegisterItemSystem(Root.Get(), Errors)))
    {
        for (const FString& Error : Errors) AddError(Error);
        return false;
    }

    TStrongObjectPtr<UGridItemDefinitionAsset> Old(NewObject<UGridItemDefinitionAsset>());
    Old->DisplayName = FText::FromString(TEXT("INCORRECT OLD STARTING NAME"));
    Old->Weight = 100.0f;
    Old->DefaultWeaponItemLevel = 4;
    Old->bUseWeaponCatalogue = false; // Intentionally contradict ARM identity.
    const FName Arms[] = {
        TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
        TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01") };
    for (const FName Id : Arms)
    {
        Old->ItemDefinitionId = Id;
        for (const int32 Level : {1, 20})
        {
            FGridResolvedItem Canonical;
            if (!TestTrue(TEXT("Actual ARM N1/N20 resolves"),
                Registry->ResolveItem(Id, Level, Canonical))) return false;
            const FGridStartingKitDisplayReadView Read =
                FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), Old.Get(), Level);
            TestTrue(TEXT("Recognize canonical ARM"), Read.bCanonicalIdentity && Read.bResolved);
            TestEqual(TEXT("Show actual fixed level"), Read.FixedItemLevel, Level);
            TestEqual(TEXT("Show canonical name, not old item name"),
                Read.DisplayName.ToString(), Canonical.DisplayName.ToString());
        }
        FGridResolvedItem Level4;
        if (!TestTrue(TEXT("Canonical inherited N4 exists"),
            Registry->ResolveItem(Id, 4, Level4))) return false;
        const FGridStartingKitDisplayReadView Inherited =
            FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), Old.Get(), 0);
        TestTrue(TEXT("Authored N0 inherits default only for ARM"), Inherited.bResolved);
        TestEqual(TEXT("ARM inherited default materialized N4"), Inherited.FixedItemLevel, 4);
        TestEqual(TEXT("ARM default preview name is canonical"), Inherited.DisplayName.ToString(),
            Level4.DisplayName.ToString());
        const FGridStartingKitDisplayReadView Invalid = 
            FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), Old.Get(), 21);
        TestTrue(TEXT("Known invalid ARM still classified canonical"), Invalid.bCanonicalIdentity);
        TestFalse(TEXT("Invalid N21 cannot inherit old data"), Invalid.bResolved);
        TestTrue(TEXT("Invalid N21 has no obsolete name"), Invalid.DisplayName.IsEmpty());
    }

    Old->ItemDefinitionId = TEXT("Weapon_RoundShield_01");
    Old->DefaultWeaponItemLevel = 0;
    const FGridStartingKitDisplayReadView BadDefault =
        FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), Old.Get(), 0);
    TestTrue(TEXT("Known invalid default remains canonical"), BadDefault.bCanonicalIdentity);
    TestFalse(TEXT("Invalid ARM N0 never silently becomes N1"), BadDefault.bResolved);
    TestTrue(TEXT("Invalid default never exposes historic name"), BadDefault.DisplayName.IsEmpty());
    Old->DefaultWeaponItemLevel = 4;

    Old->ItemDefinitionId = TEXT("Item_Arrows_01");
    Old->bUseWeaponCatalogue = true; // Intentionally contradict ammo type.
    FGridResolvedItem Arrows;
    if (!TestTrue(TEXT("Real canonical arrows N0 resolve"),
        Registry->ResolveItem(Old->ItemDefinitionId, 0, Arrows))) return false;
    const FGridStartingKitDisplayReadView Ammo =
        FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), Old.Get(), 0);
    TestTrue(TEXT("Ammo N0 still has canonical identity"), Ammo.bCanonicalIdentity && Ammo.bResolved);
    TestEqual(TEXT("Ammo N0 is a real fixed level 0"), Ammo.FixedItemLevel, 0);
    TestEqual(TEXT("Ammo name canonical not obsolete"), Ammo.DisplayName.ToString(),
        Arrows.DisplayName.ToString());
    TestFalse(TEXT("Ammo N1 is rejected for starting kit, despite transitional resolver support"),
        FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), Old.Get(), 1).bResolved);

    Old->ItemDefinitionId = TEXT("LegacyStarter_AF");
    Old->bUseWeaponCatalogue = false;
    const FGridStartingKitDisplayReadView Legacy =
        FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), Old.Get(), 0);
    TestFalse(TEXT("Unknown item remains outside registry"), Legacy.bCanonicalIdentity);
    TestTrue(TEXT("Legacy unlevelled item still displays"), Legacy.bResolved);
    TestEqual(TEXT("Legacy authored text retained"), Legacy.DisplayName.ToString(),
        Old->DisplayName.ToString());
    TestEqual(TEXT("Legacy level remains N0"), Legacy.FixedItemLevel, 0);
    TestFalse(TEXT("Negative authored level rejected"),
        FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), Old.Get(), -1).bResolved);
    TestFalse(TEXT("Legacy item cannot invent N9"),
        FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), Old.Get(), 9).bResolved);
    TestEqual(TEXT("Preview never edits source's default"), Old->DefaultWeaponItemLevel, 4);
    TestEqual(TEXT("Preview never edits source's display name"),
        Old->DisplayName.ToString(), FString(TEXT("INCORRECT OLD STARTING NAME")));
    return true;
}

#endif

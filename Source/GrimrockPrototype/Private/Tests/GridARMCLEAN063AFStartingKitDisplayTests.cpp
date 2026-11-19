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
                FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), Old.Get(), Level, Id);
            TestTrue(TEXT("Recognize canonical ARM"), Read.bCanonicalIdentity && Read.bResolved);
            TestEqual(TEXT("Show actual fixed level"), Read.FixedItemLevel, Level);
            TestEqual(TEXT("Show canonical name, not old item name"),
                Read.DisplayName.ToString(), Canonical.DisplayName.ToString());
        }
        const auto MissingId = FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), Old.Get(), 20);
        TestTrue(TEXT("Pointer-only registered ARM remains canonical"), MissingId.bCanonicalIdentity);
        TestFalse(TEXT("Pointer-only registered ARM cannot preview a valid N20"), MissingId.bResolved);
        const auto Inherited = FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), Old.Get(), 0, Id);
        TestTrue(TEXT("Known malformed ARM remains canonical"), Inherited.bCanonicalIdentity);
        TestFalse(TEXT("Explicit ARM N0 cannot inherit obsolete N4"), Inherited.bResolved);
        TestTrue(TEXT("No obsolete label on refusal"), Inherited.DisplayName.IsEmpty());
        const FGridStartingKitDisplayReadView Invalid = 
            FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), Old.Get(), 21, Id);
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
    TestFalse(TEXT("Registered ammo also needs an explicit authored ID"),
        FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), Old.Get(), 0).bResolved);
    const FGridStartingKitDisplayReadView Ammo =
        FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), Old.Get(), 0, Old->ItemDefinitionId);
    TestTrue(TEXT("Ammo N0 still has canonical identity"), Ammo.bCanonicalIdentity && Ammo.bResolved);
    TestEqual(TEXT("Ammo N0 is a real fixed level 0"), Ammo.FixedItemLevel, 0);
    TestEqual(TEXT("Ammo name canonical not obsolete"), Ammo.DisplayName.ToString(),
        Arrows.DisplayName.ToString());
    TestFalse(TEXT("Ammo N1 is rejected for starting kit, despite transitional resolver support"),
        FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), Old.Get(), 1, Old->ItemDefinitionId).bResolved);

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
    // X.65: the production wizard can display an explicit native N0
    // ingredient without a legacy pointer, using only the real registry.
    FGridResolvedItem Oil;
    const FName OilId(TEXT("Item_Flask_Oil"));
    if (!TestTrue(TEXT("Canonical Oil resolves"), Registry->ResolveItem(OilId, 0, Oil))) return false;
    const auto Explicit = FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), nullptr, 0, OilId);
    TestTrue(TEXT("Pointerless item preview resolved"), Explicit.bCanonicalIdentity && Explicit.bResolved);
    TestTrue(TEXT("DataAsset FText retained"), Explicit.DisplayName.EqualTo(Oil.DisplayName));
    TestEqual(TEXT("Physical N0 retained"), Explicit.FixedItemLevel, 0);
    TestFalse(TEXT("Explicit unknown ID cannot fall back to legacy"),
        FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), Old.Get(), 0, TEXT("Unknown_X65")).bResolved);
    TestFalse(TEXT("Contradictory old pointer forbidden"),
        FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), Old.Get(), 0, OilId).bResolved);
    TestFalse(TEXT("Explicit invalid N1 consumable forbidden"),
        FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), nullptr, 1, OilId).bResolved);
    TestFalse(TEXT("Missing registry not permitted for explicit ID"),
        FGridStartingKitDisplayReadModel::Resolve(nullptr, nullptr, 0, OilId).bResolved);
    TestFalse(TEXT("Explicit ARM N0 cannot inherit a level"),
        FGridStartingKitDisplayReadModel::Resolve(Registry.Get(), nullptr, 0, TEXT("Weapon_RoundShield_01")).bResolved);
    return true;
}

#endif

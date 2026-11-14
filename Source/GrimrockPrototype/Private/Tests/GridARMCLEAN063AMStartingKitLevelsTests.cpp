#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGStartingEquipmentService.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AMStartingKitLevels,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalStartingKitMaterializedLevels",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063AMStartingKitLevels::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real canonical ItemSystem"), Root.Get())) return false;
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Register the actual ItemSystem"), Registry->RegisterItemSystem(Root.Get(), Errors)))
    {
        for (const FString& Error : Errors) AddError(Error);
        return false;
    }

    TStrongObjectPtr<URPGClassAsset> Class(NewObject<URPGClassAsset>());
    FRPGStartingEquipmentKit& Kit = Class->StartingEquipmentKits.AddDefaulted_GetRef();
    Kit.KitId = TEXT("CanonicalStarter_063AM");

    TStrongObjectPtr<UGridItemDefinitionAsset> Sword(NewObject<UGridItemDefinitionAsset>());
    Sword->ItemDefinitionId = TEXT("Weapon_StraightSword_01");
    Sword->bUseWeaponCatalogue = false; // Intentionally contradicts the real ARM family.
    Sword->DefaultWeaponItemLevel = 6;
    Sword->DisplayName = FText::FromString(TEXT("WRONG OLD SWORD NAME"));
    Sword->Weight = 9999.0f;
    FRPGStartingEquipmentEntry& Inherited = Kit.Items.AddDefaulted_GetRef();
    Inherited.ItemDefinition = Sword.Get();
    Inherited.ItemLevel = 0;
    Inherited.Quantity = 1;

    FRPGStartingEquipmentEntry& Explicit = Kit.Items.AddDefaulted_GetRef();
    Explicit.ItemDefinition = Sword.Get();
    Explicit.ItemLevel = 20;
    Explicit.Quantity = 1;

    TStrongObjectPtr<UGridItemDefinitionAsset> Arrows(NewObject<UGridItemDefinitionAsset>());
    Arrows->ItemDefinitionId = TEXT("Item_Arrows_01");
    Arrows->bUseWeaponCatalogue = true; // Intentionally contradicts non-ARM family.
    Arrows->DefaultWeaponItemLevel = 6;
    Arrows->DisplayName = FText::FromString(TEXT("WRONG OLD AMMO NAME"));
    Arrows->Weight = 9999.0f;
    FRPGStartingEquipmentEntry& Ammunition = Kit.Items.AddDefaulted_GetRef();
    Ammunition.ItemDefinition = Arrows.Get();
    Ammunition.ItemLevel = 0;
    Ammunition.Quantity = 1;

    FGridCharacterInventoryState Character;
    Character.CharacterId = FGuid::NewGuid();
    Character.InventorySlots.SetNum(6);
    FGridCharacterEquipmentState Equipment;
    TArray<UGridItemDefinitionAsset*> Definitions;
    FString Failure;
    if (!TestTrue(TEXT("Canonical starter creates all three physical items"),
        FRPGStartingEquipmentService::TryBuildKit(Class.Get(), Kit.KitId,
            Character, Equipment, Definitions, Failure, Registry.Get())))
    {
        AddError(Failure);
        return false;
    }
    TestEqual(TEXT("Kit ID is stored after successful grant"), Character.StartingEquipmentKitId, Kit.KitId);
    TestEqual(TEXT("Two unique definitions despite two sword levels"), Definitions.Num(), 2);
    const int32 ExpectedLevels[] = {6, 20, 0};
    const FName ExpectedIds[] = {
        TEXT("Weapon_StraightSword_01"),
        TEXT("Weapon_StraightSword_01"),
        TEXT("Item_Arrows_01")};
    for (int32 Index = 0; Index < 3; ++Index)
    {
        const FGridItemInstance& Item = Character.InventorySlots[Index].Item;
        TestTrue(TEXT("Granted physical instance exists"), Item.IsValid());
        TestEqual(TEXT("Each item retains its canonical ID"), Item.ItemDefinitionId, ExpectedIds[Index]);
        TestEqual(TEXT("Authoring materializes the correct fixed level"), Item.ItemLevel, ExpectedLevels[Index]);
        FGridResolvedItem Canonical;
        if (!TestTrue(TEXT("Real canonical profile resolves"), Registry->ResolveItem(ExpectedIds[Index], ExpectedLevels[Index], Canonical)))
            return false;
        const FString ExpectedName = Canonical.DisplayName.IsEmpty()
            ? ExpectedIds[Index].ToString() : Canonical.DisplayName.ToString();
        TestEqual(TEXT("Starter name comes from ItemSystem"), Item.DisplayName.ToString(), ExpectedName);
        TestTrue(TEXT("Starter weight comes from ItemSystem"),
            FMath::IsNearlyEqual(Item.Weight, Canonical.WeightKg));
    }
    TestFalse(TEXT("No extra physical item in bag"), Character.InventorySlots[3].Item.IsValid());
    TestFalse(TEXT("No MainHand item invented"), Equipment.MainHand.IsValid());

    // An invalid registered level must reject the ENTIRE kit, including the
    // preceding legitimate sword grants, without mutating the caller's state.
    TStrongObjectPtr<UGridItemDefinitionAsset> Invalid(NewObject<UGridItemDefinitionAsset>());
    Invalid->ItemDefinitionId = TEXT("Weapon_RoundShield_01");
    Invalid->DefaultWeaponItemLevel = 0;
    Invalid->bUseWeaponCatalogue = false;
    FRPGStartingEquipmentEntry& Bad = Kit.Items.AddDefaulted_GetRef();
    Bad.ItemDefinition = Invalid.Get();
    Bad.ItemLevel = 0;
    Bad.Quantity = 1;
    FGridCharacterInventoryState Rejected;
    Rejected.CharacterId = FGuid::NewGuid();
    Rejected.InventorySlots.SetNum(6);
    const FGuid OriginalCharacterId = Rejected.CharacterId;
    FGridCharacterEquipmentState RejectedGear;
    Definitions.Reset();
    Failure.Reset();
    TestFalse(TEXT("Canonical ARM N0 without a valid default rejects the kit"),
        FRPGStartingEquipmentService::TryBuildKit(Class.Get(), Kit.KitId,
            Rejected, RejectedGear, Definitions, Failure, Registry.Get()));
    TestFalse(TEXT("Rejection has an explicit diagnostic"), Failure.IsEmpty());
    TestTrue(TEXT("No partial bag grant survives"), Rejected.InventorySlots[0].IsEmpty());
    TestTrue(TEXT("Rejected equipment remains empty"), !RejectedGear.MainHand.IsValid());
    TestTrue(TEXT("Rejected kit ID is not recorded"), Rejected.StartingEquipmentKitId.IsNone());
    TestEqual(TEXT("Character identity is unchanged"), Rejected.CharacterId, OriginalCharacterId);
    TestTrue(TEXT("Rejected definitions not published"), Definitions.IsEmpty());

    // Non-ARM N1 must not be accepted merely because a legacy flag says ARM.
    Bad.ItemDefinition = Arrows.Get();
    Bad.ItemLevel = 1;
    Failure.Reset();
    TestFalse(TEXT("Canonical ammunition N1 is invalid"),
        FRPGStartingEquipmentService::TryBuildKit(Class.Get(), Kit.KitId,
            Rejected, RejectedGear, Definitions, Failure, Registry.Get()));
    TestTrue(TEXT("A failed ammo kit still grants nothing"), Rejected.InventorySlots[0].IsEmpty());
    return true;
}
#endif

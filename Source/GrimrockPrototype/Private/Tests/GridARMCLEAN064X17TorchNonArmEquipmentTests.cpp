#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Core/GridLevelAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridPartyIlluminationComponent.h"
#include "Components/PointLightComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridCanonicalEquipmentSlotsReadModel.h"

namespace ARMCLEAN064X17
{
    struct FWorldFixture
    {
        UWorld* World = nullptr;
        UGameInstance* GI = nullptr;
        AGrimrockPartyPawn* Pawn = nullptr;
        UGridPartyInventoryComponent* Inventory = nullptr;
        UGridItemSubsystem* Registry = nullptr;

        FWorldFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM_CLEAN064X17_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (!World || !GEngine) {return;}
            FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
            Context.SetCurrentWorld(World);
            GI = NewObject<UGameInstance>(GetTransientPackage());
            GI->Init();
            World->SetGameInstance(GI);
            Registry = GI->GetSubsystem<UGridItemSubsystem>();
            Pawn = World->SpawnActor<AGrimrockPartyPawn>();
            if (Pawn)
            {
                Inventory = Pawn->PartyInventoryComponent.Get();
                if (Inventory) {Inventory->InitializeDefaultPartyIfNeeded();}
            }
        }
        ~FWorldFixture()
        {
            if (World) {World->SetGameInstance(nullptr);}
            if (GI) {GI->Shutdown();}
            if (World)
            {
                World->DestroyWorld(false);
                if (GEngine) {GEngine->DestroyWorldContext(World);}
            }
        }
    };

    FGridItemInstance MakeItem(FName Id, int32 Level, const FGuid OwnerId,
        bool bLit = false)
    {
        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid();
        Item.ItemDefinitionId = Id;
        Item.ItemLevel = Level;
        Item.Quantity = 1;
        Item.OwnerType = EGridItemOwnerType::CharacterInventory;
        Item.OwnerGuid = OwnerId;
        Item.OwnerCharacterIndex = 0;
        Item.EquipmentSlot = EGridEquipmentSlot::None;
        Item.bLightsEnabled = bLit;
        return Item;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X17TorchHandRegression,
    "Grimrock.RPG.ARM.CLEAN06_4X17.TorchCanonicalNonArmHandEquip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X17TorchHandRegression::RunTest(const FString&)
{
    ARMCLEAN064X17::FWorldFixture F;
    if (!TestNotNull(TEXT("Runtime item registry"), F.Registry) ||
        !TestNotNull(TEXT("Party inventory"), F.Inventory) ||
        !TestTrue(TEXT("Active party member exists"), F.Inventory->IsValidCharacterIndex(0)))
        {return false;}

    UGridItemDefinitionAsset* Torch = LoadObject<UGridItemDefinitionAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Torch.DA_Item_Torch"));
    if (!TestNotNull(TEXT("Real torch definition"), Torch) ||
        !TestTrue(TEXT("Torch ItemId is registered canonically"),
            F.Registry->HasCanonicalItem(Torch->ItemDefinitionId)))
        {return false;}

    FGridResolvedItem Resolved;
    if (!TestTrue(TEXT("Real torch physical N0 resolves canonically"),
        F.Registry->ResolveItem(Torch->ItemDefinitionId, 0, Resolved)) ||
        !TestFalse(TEXT("Torch is NOT an armament"), Resolved.bIsArmament) ||
        !TestTrue(TEXT("Registered torch allows main hand"),
            Resolved.NonArmEquipmentSlots.Contains(EGridEquipmentSlot::MainHand)) ||
        !TestTrue(TEXT("Registered torch allows off hand"),
            Resolved.NonArmEquipmentSlots.Contains(EGridEquipmentSlot::OffHand)) ||
        !TestTrue(TEXT("Real torch still has a light emitter"),
            Resolved.LightEmitter.bUsePointLight))
        {return false;}

    if (!TestTrue(TEXT("Legacy torch registered for held-light visual during cutover"),
        F.Inventory->RegisterItemDefinition(Torch)))
        {return false;}
    FGridCharacterInventoryState& Character = F.Inventory->PartyInventoryState.ActiveCharacters[0];
    if (!TestTrue(TEXT("Two inventory slots available"),
        Character.InventorySlots.Num() >= 2)) {return false;}
    FGridItemInstance Main = ARMCLEAN064X17::MakeItem(
        Torch->ItemDefinitionId, 0, Character.CharacterId, true);
    FGridItemInstance Off = ARMCLEAN064X17::MakeItem(
        Torch->ItemDefinitionId, 0, Character.CharacterId, true);
    Character.InventorySlots[0].bOccupied = true;
    Character.InventorySlots[0].Item = Main;
    Character.InventorySlots[1].bOccupied = true;
    Character.InventorySlots[1].Item = Off;

    const auto MainSlots = FGridCanonicalEquipmentSlotsReadModel::Resolve(
        F.Registry, Torch, Main);
    if (!TestTrue(TEXT("Torch menu offers both hands"),
        MainSlots.bCanonicalIdentity && MainSlots.bResolved &&
        MainSlots.AllowedSlots.Contains(EGridEquipmentSlot::MainHand) &&
        MainSlots.AllowedSlots.Contains(EGridEquipmentSlot::OffHand) &&
        !MainSlots.bTwoHanded) ||
        !TestTrue(TEXT("Torch is equippable to main hand from inventory"),
            F.Inventory->CanEquipItemToSlot(0, Main, EGridEquipmentSlot::MainHand)) ||
        !TestTrue(TEXT("Torch is equippable to off hand from inventory"),
            F.Inventory->CanEquipItemToSlot(0, Off, EGridEquipmentSlot::OffHand)))
        {return false;}

    if (!TestTrue(TEXT("Bag to main hand succeeds"),
        F.Inventory->EquipItemFromInventorySlot(0, 0, EGridEquipmentSlot::MainHand)))
        {return false;}
    FGridItemInstance Equipped;
    if (!TestTrue(TEXT("Main torch is equipped"), F.Inventory->GetEquippedItem(
        0, EGridEquipmentSlot::MainHand, Equipped)) ||
        !TestTrue(TEXT("Main torch retains GUID and light state"),
            Equipped.RuntimeObjectId == Main.RuntimeObjectId &&
            Equipped.bLightsEnabled && Equipped.ItemLevel == 0))
        {return false;}

    if (!TestTrue(TEXT("Bag to off hand succeeds"),
        F.Inventory->EquipItemFromInventorySlot(0, 1, EGridEquipmentSlot::OffHand)) ||
        !TestTrue(TEXT("Off hand torch is equipped"), F.Inventory->GetEquippedItem(
            0, EGridEquipmentSlot::OffHand, Equipped)) ||
        !TestTrue(TEXT("Off torch preserves light state"),
            Equipped.RuntimeObjectId == Off.RuntimeObjectId && Equipped.bLightsEnabled))
        {return false;}

    // The real torch must illuminate the PARTY after being equipped,
    // not merely occupy a valid equipment slot.
    UGridPartyIlluminationComponent* PartyLight =
        NewObject<UGridPartyIlluminationComponent>(F.Pawn, TEXT("TorchPartyLightRegression"));
    if (!TestNotNull(TEXT("Explicit party light component"), PartyLight))
        {return false;}
    PartyLight->SetupAttachment(F.Pawn->GetRootComponent());
    F.Pawn->AddInstanceComponent(PartyLight);
    PartyLight->RegisterComponent();
    PartyLight->RefreshFromEquipment(F.Inventory, nullptr);
    if (!TestTrue(TEXT("Lit equipped torch activates party illumination"),
        PartyLight->HasActiveIlluminationSource()) ||
        !TestEqual(TEXT("Torch ItemId is party light source"),
            PartyLight->ActiveSourceId, Torch->ItemDefinitionId) ||
        !TestTrue(TEXT("Torch supplies actual point light"),
            PartyLight->RuntimeConfig.bUsePointLight &&
            PartyLight->RuntimeConfig.LightIntensity > 0.0f &&
            PartyLight->RuntimeConfig.LightRadius > 0.0f) ||
        !TestTrue(TEXT("Party source uses real canonical torch intensity"),
            FMath::IsNearlyEqual(PartyLight->RuntimeConfig.LightIntensity,
                Resolved.LightEmitter.LightIntensity)))
        {return false;}
    UPointLightComponent* Light = F.Pawn->FindComponentByClass<UPointLightComponent>();
    if (!TestNotNull(TEXT("Equipped torch creates real point-light component"), Light))
        {return false;}

    // Canonical ammunition never borrows obsolete old hand policy.
    UGridItemDefinitionAsset* OldAmmo = NewObject<UGridItemDefinitionAsset>(F.Inventory);
    OldAmmo->ItemDefinitionId = TEXT("Item_Arrows_01");
    OldAmmo->HandUsage = EGridItemHandUsage::OneHanded;
    FGridItemInstance Ammo = ARMCLEAN064X17::MakeItem(
        OldAmmo->ItemDefinitionId, 0, Character.CharacterId);
    auto AmmoSlots = FGridCanonicalEquipmentSlotsReadModel::Resolve(
        F.Registry, OldAmmo, Ammo);
    if (!TestTrue(TEXT("Canonical ammo has no equip actions"),
        AmmoSlots.bCanonicalIdentity && AmmoSlots.bResolved &&
        AmmoSlots.AllowedSlots.IsEmpty()) ||
        !TestFalse(TEXT("Canonical ammo cannot equip despite old OneHanded flag"),
            F.Inventory->CanEquipItemToSlot(0, Ammo, EGridEquipmentSlot::MainHand)))
        {return false;}

    Ammo.ItemLevel = 1;
    if (!TestFalse(TEXT("Physical nonARM N1 never equips"),
        F.Inventory->CanEquipItemToSlot(0, Ammo, EGridEquipmentSlot::MainHand)))
        {return false;}

    AddInfo(TEXT("[ARM-CLEAN06.4X.17] real torch canonical N0 equipped in both hands; "
        "GUID and lit state preserved; ammo N0 cannot equip, nonARM N1 rejected."));
    return !HasAnyErrors();
}
#endif

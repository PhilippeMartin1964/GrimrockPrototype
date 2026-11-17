#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Core/GridLevelAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridItemActor.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X23PointerlessHeldTorch,
    "Grimrock.RPG.ARM.CLEAN06_4X23.CanonicalHeldTorchWithoutLegacySource",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X23PointerlessHeldTorch::RunTest(const FString&)
{
    ARMCLEAN064X17::FWorldFixture F;
    if (!TestNotNull(TEXT("Configured canonical registry"), F.Registry) ||
        !TestNotNull(TEXT("Party inventory"), F.Inventory) ||
        !TestNotNull(TEXT("Party pawn"), F.Pawn) ||
        !TestTrue(TEXT("Active character and equipment available"),
            !F.Inventory->PartyInventoryState.ActiveCharacters.IsEmpty() &&
            !F.Inventory->PartyInventoryState.ActiveEquipment.IsEmpty()))
    {
        return false;
    }

    FGridResolvedItem Canonical;
    const FName TorchId(TEXT("Item_Torch"));
    if (!TestTrue(TEXT("Real torch resolves without historical source"),
            F.Registry->ResolveItem(TorchId, 0, Canonical)) ||
        !TestTrue(TEXT("Torch world light is a canonical property"),
            Canonical.LightEmitter.bUsePointLight) ||
        !TestFalse(TEXT("Torch is non-ARM N0"), Canonical.bIsArmament))
    {
        return false;
    }

    UGridPartyIlluminationComponent* PartyLight =
        NewObject<UGridPartyIlluminationComponent>(F.Pawn, TEXT("X23PartyLight"));
    if (!TestNotNull(TEXT("Runtime party light"), PartyLight))
    {
        return false;
    }
    PartyLight->SetupAttachment(F.Pawn->GetRootComponent());
    F.Pawn->AddInstanceComponent(PartyLight);
    PartyLight->RegisterComponent();

    FGridCharacterEquipmentState& Equipment =
        F.Inventory->PartyInventoryState.ActiveEquipment[0];
    const FGuid CharacterId =
        F.Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId;

    for (const EGridEquipmentSlot Slot : {
        EGridEquipmentSlot::MainHand, EGridEquipmentSlot::OffHand})
    {
        Equipment.MainHand = FGridItemInstance();
        Equipment.OffHand = FGridItemInstance();
        FGridItemInstance Torch = ARMCLEAN064X17::MakeItem(
            TorchId, 0, CharacterId, true);
        Torch.OwnerType = EGridItemOwnerType::EquipmentSlot;
        Torch.EquipmentSlot = Slot;
        if (Slot == EGridEquipmentSlot::MainHand)
        {
            Equipment.MainHand = Torch;
        }
        else
        {
            Equipment.OffHand = Torch;
        }

        F.Pawn->SyncHeldVisualFromSelectedCharacterEquipment();
        PartyLight->RefreshFromEquipment(F.Inventory, nullptr);
        if (!TestEqual(TEXT("Pointerless held torch is visible from either hand"),
                F.Pawn->GetHeldItemDefinitionId(), TorchId) ||
            !TestNotNull(TEXT("Visual item actor exists"), F.Pawn->HeldItemActor.Get()) ||
            !TestNull(TEXT("Held visual never references a legacy torch definition"),
                F.Pawn->HeldItemActor->GetItemDefinitionAsset()) ||
            !TestTrue(TEXT("Held actor retains canonical presentation"),
                F.Pawn->HeldItemActor->CanonicalWorldPresentation.IsValid()) ||
            !TestTrue(TEXT("Party illumination works with pointerless torch"),
                PartyLight->HasActiveIlluminationSource()) ||
            !TestEqual(TEXT("Correct light source ItemId"),
                PartyLight->ActiveSourceId, TorchId) ||
            !TestTrue(TEXT("Canonical light intensity used"),
                FMath::IsNearlyEqual(PartyLight->RuntimeConfig.LightIntensity,
                    Canonical.LightEmitter.LightIntensity)))
        {
            return false;
        }

        // Extinguishing the same instance must turn OFF both light and held
        // presentation, not leave a visually lit phantom torch.
        if (Slot == EGridEquipmentSlot::MainHand)
        {
            Equipment.MainHand.bLightsEnabled = false;
        }
        else
        {
            Equipment.OffHand.bLightsEnabled = false;
        }
        F.Pawn->SyncHeldVisualFromSelectedCharacterEquipment();
        PartyLight->RefreshFromEquipment(F.Inventory, nullptr);
        if (!TestFalse(TEXT("Extinguished torch does not illuminate the party"),
                PartyLight->HasActiveIlluminationSource()) ||
            !TestTrue(TEXT("No active held-light visual after extinction"),
                F.Pawn->GetHeldItemDefinitionId().IsNone()))
        {
            return false;
        }
    }
    return true;
}


// X.41: Two real equipped-light readers must agree on the registered
// physical item identity. Legacy sources may exist in inventory while
// the 22 canonical ARM visual profiles have already been native-verified.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X41CanonicalEquippedLight,
    "Grimrock.RPG.ARM.CLEAN06_4X41.CanonicalEquippedLightReaders",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X41CanonicalEquippedLight::RunTest(const FString&)
{
    ARMCLEAN064X17::FWorldFixture F;
    if (!TestNotNull(TEXT("Canonical registry"), F.Registry) ||
        !TestNotNull(TEXT("Inventory"), F.Inventory) ||
        !TestNotNull(TEXT("Party pawn"), F.Pawn) ||
        !TestTrue(TEXT("Active equipment exists"),
            !F.Inventory->PartyInventoryState.ActiveEquipment.IsEmpty()) ||
        !TestTrue(TEXT("Active hero exists"),
            !F.Inventory->PartyInventoryState.ActiveCharacters.IsEmpty()))
    {
        return false;
    }

    const FName ArmId(TEXT("Weapon_StraightSword_01"));
    FGridResolvedItem CanonicalArm;
    if (!TestTrue(TEXT("Real ARM physical N1 resolves"),
            F.Registry->ResolveItem(ArmId, 1, CanonicalArm)) ||
        !TestTrue(TEXT("Real armament family"), CanonicalArm.bIsArmament))
    {
        return false;
    }
    const bool bArmEmitsLight = CanonicalArm.LightEmitter.HasEmitter();
    UGridItemDefinitionAsset* StaleArm = NewObject<UGridItemDefinitionAsset>(F.Inventory);
    StaleArm->ItemDefinitionId = ArmId;
    StaleArm->LightEmitter.bUsePointLight = !bArmEmitsLight;
    StaleArm->LightEmitter.LightIntensity = 12345.f;
    StaleArm->LightEmitter.LightRadius = 9999.f;
    if (!TestTrue(TEXT("Conflicting same-ID source registered in old inventory"),
            F.Inventory->RegisterItemDefinition(StaleArm)))
    {
        return false;
    }

    UGridPartyIlluminationComponent* PartyLight =
        NewObject<UGridPartyIlluminationComponent>(F.Pawn, TEXT("X41PartyLight"));
    if (!TestNotNull(TEXT("Party illumination component"), PartyLight))
    {
        return false;
    }
    PartyLight->SetupAttachment(F.Pawn->GetRootComponent());
    F.Pawn->AddInstanceComponent(PartyLight);
    PartyLight->RegisterComponent();

    FGridCharacterEquipmentState& Equipment =
        F.Inventory->PartyInventoryState.ActiveEquipment[0];
    const FGuid CharacterId =
        F.Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId;
    for (const int32 PhysicalLevel : {1, 20})
    {
        FGridResolvedItem LevelProjection;
        if (!TestTrue(TEXT("Real ARM resolves at equipped level"),
                F.Registry->ResolveItem(ArmId, PhysicalLevel, LevelProjection)))
        {
            return false;
        }
        FGridItemInstance Arm = ARMCLEAN064X17::MakeItem(
            ArmId, PhysicalLevel, CharacterId, true);
        Arm.OwnerType = EGridItemOwnerType::EquipmentSlot;
        Arm.EquipmentSlot = EGridEquipmentSlot::MainHand;
        Equipment.MainHand = Arm;
        Equipment.OffHand = FGridItemInstance();
        PartyLight->RefreshFromEquipment(F.Inventory, nullptr);
        if (!TestEqual(TEXT("Party light uses canonical ARM rather than stale inventory"),
                PartyLight->HasActiveIlluminationSource(),
                LevelProjection.LightEmitter.HasEmitter()) ||
            !TestEqual(TEXT("Pawn held-light predicate uses canonical ARM"),
                F.Pawn->DoesEquippedItemEmitLight(Arm),
                LevelProjection.LightEmitter.HasEmitter()))
        {
            return false;
        }
        if (LevelProjection.LightEmitter.HasEmitter() &&
            (!TestEqual(TEXT("Registered ARM is the illumination source"),
                PartyLight->ActiveSourceId, ArmId) ||
             !TestTrue(TEXT("ARM light intensity is canonical, not 12345"),
                FMath::IsNearlyEqual(PartyLight->RuntimeConfig.LightIntensity,
                    LevelProjection.LightEmitter.LightIntensity *
                    PartyLight->IntensityMultiplier))))
        {
            return false;
        }
    }

    // An invalid physical N0 ARM must not revive the stale LightEmitter.
    FGridItemInstance BadArm = ARMCLEAN064X17::MakeItem(
        ArmId, 0, CharacterId, true);
    BadArm.OwnerType = EGridItemOwnerType::EquipmentSlot;
    BadArm.EquipmentSlot = EGridEquipmentSlot::MainHand;
    Equipment.MainHand = BadArm;
    PartyLight->RefreshFromEquipment(F.Inventory, nullptr);
    if (!TestFalse(TEXT("ARM N0 cannot illuminate from legacy source"),
            F.Pawn->DoesEquippedItemEmitLight(BadArm)) ||
        !TestFalse(TEXT("ARM N0 does not illuminate party"),
            PartyLight->HasActiveIlluminationSource()))
    {
        return false;
    }

    // ResolveItem supports non-ARM N1 family metadata in some callers;
    // that level is not a valid persisted physical equipped torch.
    FGridItemInstance BadTorch = ARMCLEAN064X17::MakeItem(
        TEXT("Item_Torch"), 1, CharacterId, true);
    BadTorch.OwnerType = EGridItemOwnerType::EquipmentSlot;
    BadTorch.EquipmentSlot = EGridEquipmentSlot::MainHand;
    Equipment.MainHand = BadTorch;
    PartyLight->RefreshFromEquipment(F.Inventory, nullptr);
    if (!TestFalse(TEXT("Physical torch N1 cannot illuminate in held predicate"),
            F.Pawn->DoesEquippedItemEmitLight(BadTorch)) ||
        !TestFalse(TEXT("Physical torch N1 cannot illuminate party"),
            PartyLight->HasActiveIlluminationSource()))
    {
        return false;
    }

    // Unknown historical items retain their old, explicit emitter until
    // their authoring is converted separately.
    const FName LegacyId(TEXT("Legacy_X41_Light"));
    UGridItemDefinitionAsset* Legacy = NewObject<UGridItemDefinitionAsset>(F.Inventory);
    Legacy->ItemDefinitionId = LegacyId;
    Legacy->LightEmitter.bUsePointLight = true;
    Legacy->LightEmitter.LightIntensity = 1200.f;
    Legacy->LightEmitter.LightRadius = 500.f;
    if (!TestTrue(TEXT("Unregistered legacy item source registered"),
            F.Inventory->RegisterItemDefinition(Legacy)))
    {
        return false;
    }
    FGridItemInstance LegacyItem = ARMCLEAN064X17::MakeItem(
        LegacyId, 0, CharacterId, true);
    LegacyItem.OwnerType = EGridItemOwnerType::EquipmentSlot;
    LegacyItem.EquipmentSlot = EGridEquipmentSlot::MainHand;
    Equipment.MainHand = LegacyItem;
    PartyLight->RefreshFromEquipment(F.Inventory, nullptr);
    return TestTrue(TEXT("Out-of-registry legacy held light is preserved"),
            F.Pawn->DoesEquippedItemEmitLight(LegacyItem)) &&
        TestTrue(TEXT("Out-of-registry light remains available to party"),
            PartyLight->HasActiveIlluminationSource()) &&
        TestEqual(TEXT("Legacy source identity preserved"),
            PartyLight->ActiveSourceId, LegacyId) &&
        !HasAnyErrors();
}

#endif

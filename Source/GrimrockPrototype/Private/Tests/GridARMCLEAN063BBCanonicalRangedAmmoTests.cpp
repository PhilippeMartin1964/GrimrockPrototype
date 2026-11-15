#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridRangedAmmunitionPolicy.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridThrownItemActor.h"
#include "Runtime/Items/GridItemSubsystem.h"

namespace ARMCLEAN063BB
{
    struct FFixture
    {
        UWorld* World = nullptr;
        UGameInstance* GI = nullptr;
        UGridItemSubsystem* Registry = nullptr;
        UGridPartyInventoryComponent* Inventory = nullptr;
        AGridLevelRuntimeActor* Runtime = nullptr;

        explicit FFixture(bool bWorldProjectile = false)
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM063BB_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (!World || !GEngine) return;
            FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
            Context.SetCurrentWorld(World);
            GI = NewObject<UGameInstance>(GetTransientPackage());
            if (!GI) return;
            GI->Init();
            World->SetGameInstance(GI);
            Registry = GI->GetSubsystem<UGridItemSubsystem>();
            Inventory = NewObject<UGridPartyInventoryComponent>(GetTransientPackage());
            Inventory->InitializeDefaultPartyIfNeeded();
            if (!bWorldProjectile) return;

            Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
            if (!Runtime) return;
            UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
            Level->Width = 2;
            Level->Height = 2;
            Level->EnsureCellCount();
            for (int32 X = 0; X < 2; ++X)
                for (int32 Y = 0; Y < 2; ++Y)
                    Level->GetCellMutable(X, Y).CellType = EGridCellType::Floor;
            Runtime->LevelAsset = Level;
        }

        ~FFixture()
        {
            if (World) World->SetGameInstance(nullptr);
            if (GI) GI->Shutdown();
            if (World)
            {
                World->DestroyWorld(false);
                if (GEngine) GEngine->DestroyWorldContext(World);
            }
        }

        bool Ready() const
        {
            return Registry && Inventory &&
                Registry->HasCanonicalItem(TEXT("Weapon_ShortBow_01")) &&
                Registry->HasCanonicalItem(TEXT("Item_Arrows_01"));
        }

        FGridRangedAmmunitionRequirement Resolve(
            const UGridItemDefinitionAsset* Legacy = nullptr, int32 Level = 1,
            bool bRanged = true) const
        {
            return FGridRangedAmmunitionPolicy::ResolveForAttack(
                Registry, TEXT("Weapon_ShortBow_01"), Level, Legacy, bRanged);
        }
    };

    bool RunScenario(FAutomationTestBase& Test, const TCHAR* Scenario)
    {
        const bool bFlight = FCString::Strcmp(Scenario, TEXT("Projectile")) == 0;
        FFixture F(bFlight);
        if (!Test.TestTrue(TEXT("Production canonical bow, arrows and inventory"), F.Ready()))
            return false;
        const FName ArrowId(TEXT("Item_Arrows_01"));
        FGridResolvedItem Weapon;
        if (!Test.TestTrue(TEXT("Actual bow resolved at N1"),
            F.Registry->ResolveItem(TEXT("Weapon_ShortBow_01"), 1, Weapon)))
            return false;
        Test.TestEqual(TEXT("Bow requires the real canonical ammo kind"),
            Weapon.Weapon.RequiredAmmoKindId, ArrowId);
        FGridResolvedItem Ammo;
        if (!Test.TestTrue(TEXT("Unique real arrow resolved at N0"),
            F.Registry->ResolveAmmoKind(ArrowId, Ammo)))
            return false;
        Test.TestEqual(TEXT("Canonical arrow ID"), Ammo.ItemId, ArrowId);
        Test.TestTrue(TEXT("Canonical arrows have stack capacity"), Ammo.MaxStack > 1);
        FGridResolvedItem MissingAmmo;
        Test.TestFalse(TEXT("No guessed ammo ID for unknown kind"),
            F.Registry->ResolveAmmoKind(TEXT("MissingAmmoKind"), MissingAmmo));

        UGridItemDefinitionAsset* OldWeapon = NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
        OldWeapon->ItemDefinitionId = TEXT("Weapon_ShortBow_01");
        OldWeapon->RequiredAmmunitionItemDefinitionId = TEXT("Item_Old_Contradictory");
        const FGridRangedAmmunitionRequirement Required = F.Resolve(OldWeapon);
        const auto NoLegacy = F.Resolve();
        Test.TestTrue(TEXT("Canonical source wins over conflicting old definition"),
            Required.bCanonicalSource && Required.bValid && Required.AmmunitionItemId == ArrowId);
        Test.TestTrue(TEXT("No old weapon needed for canonical ammo resolution"),
            NoLegacy.bCanonicalSource && NoLegacy.bValid &&
            NoLegacy.AmmunitionItemId == Required.AmmunitionItemId);
        Test.TestTrue(TEXT("Arrow weight comes from registry"),
            FMath::IsNearlyEqual(Required.AmmunitionWeightKg, Ammo.WeightKg));

        if (FCString::Strcmp(Scenario, TEXT("Projection")) == 0)
        {
            const auto InvalidN0 = F.Resolve(OldWeapon, 0);
            Test.TestTrue(TEXT("Known but invalid canonical N0 fails closed"),
                InvalidN0.bCanonicalSource && !InvalidN0.bValid);
            const auto Melee = F.Resolve(OldWeapon, 1, false);
            Test.TestTrue(TEXT("Melee needs no arrow"), Melee.bValid && Melee.AmmunitionItemId.IsNone());
            UGridItemDefinitionAsset* Legacy = NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
            Legacy->ItemDefinitionId = TEXT("Legacy_Bow_BB");
            Legacy->RequiredAmmunitionItemDefinitionId = TEXT("Legacy_Arrow_BB");
            const auto LegacyRequirement = FGridRangedAmmunitionPolicy::ResolveForAttack(
                F.Registry, Legacy->ItemDefinitionId, 0, Legacy, true);
            Test.TestTrue(TEXT("Unregistered weapon keeps original ammo join"),
                !LegacyRequirement.bCanonicalSource && LegacyRequirement.bValid &&
                LegacyRequirement.AmmunitionItemId == Legacy->RequiredAmmunitionItemDefinitionId);
            return !Test.HasAnyErrors();
        }

        if (FCString::Strcmp(Scenario, TEXT("Inventory")) == 0)
        {
            Test.TestFalse(TEXT("No ammo: no ranged attack"),
                FGridRangedAmmunitionPolicy::HasRequiredAmmunitionForAttack(*F.Inventory, 0, Required));
            Test.TestFalse(TEXT("Absent arrows cannot be debited"),
                FGridRangedAmmunitionPolicy::ConsumeRequiredAmmunitionForAttack(*F.Inventory, 0, Required));
            FGridCharacterInventoryState& Hero = F.Inventory->PartyInventoryState.ActiveCharacters[0];
            FGridInventorySlot& Slot = Hero.InventorySlots[0];
            Slot.bOccupied = true;
            Slot.Item.RuntimeObjectId = FGuid::NewGuid();
            Slot.Item.ItemDefinitionId = ArrowId;
            Slot.Item.ItemLevel = 0;
            Slot.Item.Quantity = 2;
            Slot.Item.OwnerType = EGridItemOwnerType::CharacterInventory;
            Slot.Item.OwnerGuid = Hero.CharacterId;
            Slot.Item.OwnerCharacterIndex = 0;
            const FGuid StackGuid = Slot.Item.RuntimeObjectId;
            const auto NonRanged = F.Resolve(OldWeapon, 1, false);
            Test.TestTrue(TEXT("Non ranged call no-op"),
                FGridRangedAmmunitionPolicy::ConsumeRequiredAmmunitionForAttack(*F.Inventory, 0, NonRanged));
            Test.TestEqual(TEXT("Melee leaves stack unchanged"), Slot.Item.Quantity, 2);
            Test.TestTrue(TEXT("Ranged sees real arrows"),
                FGridRangedAmmunitionPolicy::HasRequiredAmmunitionForAttack(*F.Inventory, 0, Required));
            Test.TestTrue(TEXT("First accepted arrow debit"),
                FGridRangedAmmunitionPolicy::ConsumeRequiredAmmunitionForAttack(*F.Inventory, 0, Required));
            Test.TestEqual(TEXT("Two to one"), Slot.Item.Quantity, 1);
            Test.TestEqual(TEXT("Partial debit keeps stack GUID"), Slot.Item.RuntimeObjectId, StackGuid);
            Test.TestTrue(TEXT("Second accepted debit"),
                FGridRangedAmmunitionPolicy::ConsumeRequiredAmmunitionForAttack(*F.Inventory, 0, Required));
            Test.TestTrue(TEXT("Last arrow removes stack"), Slot.IsEmpty());
            Test.TestFalse(TEXT("Third shot refused at zero"),
                FGridRangedAmmunitionPolicy::HasRequiredAmmunitionForAttack(*F.Inventory, 0, Required));
            Test.TestFalse(TEXT("No ammo underflow"),
                FGridRangedAmmunitionPolicy::ConsumeRequiredAmmunitionForAttack(*F.Inventory, 0, Required));
            Test.TestFalse(TEXT("Known invalid level never debits anything"),
                FGridRangedAmmunitionPolicy::ConsumeRequiredAmmunitionForAttack(
                    *F.Inventory, 0, F.Resolve(OldWeapon, 0)));
            return !Test.HasAnyErrors();
        }

        if (FCString::Strcmp(Scenario, TEXT("Projectile")) == 0)
        {
            if (!Test.TestNotNull(TEXT("Walkable runtime"), F.Runtime)) return false;
            Test.TestTrue(TEXT("Projectile source cell walkable"), F.Runtime->IsWalkableCell(0, 0));
            FGridItemInstance Arrow;
            Arrow.RuntimeObjectId = FGuid::NewGuid();
            Arrow.ItemDefinitionId = ArrowId;
            Arrow.ItemLevel = 0;
            Arrow.Quantity = 1;
            Arrow.Weight = Ammo.WeightKg;
            Arrow.OwnerType = EGridItemOwnerType::World;
            Arrow.OwnerCharacterIndex = INDEX_NONE;
            const FVector Spawn = F.Runtime->GetCellCenterWorld(0, 0, 100.0f);
            const FVector Velocity(1200.0f, 0.0f, 0.0f);
            AGridThrownItemActor* Projectile = F.Runtime->SpawnThrownItemProjectile(
                Arrow, nullptr, Spawn, Velocity, 0, 0, true);
            if (!Test.TestNotNull(TEXT("Canonical arrow launches without old item DA"),
                Projectile)) return false;
            Test.TestTrue(TEXT("Projectile flagged as ranged ammunition"),
                Projectile->bRangedAmmunitionProjectile);
            Test.TestEqual(TEXT("Projectile ItemId preserved"),
                Projectile->ThrownItemInstance.ItemDefinitionId, ArrowId);
            Test.TestEqual(TEXT("Projectile GUID preserved"),
                Projectile->ThrownItemInstance.RuntimeObjectId, Arrow.RuntimeObjectId);
            Test.TestTrue(TEXT("Projectile uses real canonical mesh"),
                Projectile->MeshComponent && Projectile->MeshComponent->GetStaticMesh() != nullptr);
            Test.TestEqual(TEXT("Spawning alone never charges inventory"),
                F.Inventory->CountItemDefinitionInCharacterInventory(0, ArrowId), 0);
            return !Test.HasAnyErrors();
        }
        return false;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMCLEAN063BBProjection,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalRangedAmmo.Projection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMCLEAN063BBProjection::RunTest(const FString&)
{
    return ARMCLEAN063BB::RunScenario(*this, TEXT("Projection"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMCLEAN063BBInventory,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalRangedAmmo.Inventory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMCLEAN063BBInventory::RunTest(const FString&)
{
    return ARMCLEAN063BB::RunScenario(*this, TEXT("Inventory"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMCLEAN063BBProjectile,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalRangedAmmo.Projectile",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMCLEAN063BBProjectile::RunTest(const FString&)
{
    return ARMCLEAN063BB::RunScenario(*this, TEXT("Projectile"));
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridThrownItemActor.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridItemVisualEffectsComponent.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridCanonicalThrowBallisticsReadModel.h"
#include "../Runtime/GridPhysicalThrowStrengthReadModel.h"

namespace ARMCLEAN063AV
{
    struct FFixture
    {
        UWorld* World = nullptr;
        UGameInstance* GI = nullptr;
        UGridItemSubsystem* Registry = nullptr;
        AGridLevelRuntimeActor* Runtime = nullptr;

        FFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM063AV_%s"),
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
            return Registry && Runtime && Runtime->IsWalkableCell(0, 0) &&
                Registry->HasCanonicalItem(TEXT("Weapon_ThrowingKnife_01"));
        }

        FGridItemInstance Make(FName Id, int32 Level = 1) const
        {
            FGridItemInstance Item;
            Item.RuntimeObjectId = FGuid::NewGuid();
            Item.ItemDefinitionId = Id;
            Item.ItemLevel = Level;
            Item.Quantity = 1;
            Item.Weight = 1000.0f; // Stale SaveGame metadata is never an authority.
            Item.OwnerType = EGridItemOwnerType::World;
            Item.OwnerCharacterIndex = INDEX_NONE;
            return Item;
        }
    };
}

// Distinct Automation entries isolate catalogue, projectile, pickup and
// invalid-identity failures in the public Windows validation diagnostics.
namespace ARMCLEAN063AV
{
    bool RunScenario(FAutomationTestBase& Test, const TCHAR* Scenario)
    {
    ARMCLEAN063AV::FFixture F;
    if (!Test.TestTrue(TEXT("Real canonical registry and walkable test cell"), F.Ready())) return false;
    const FName Ids[] = { TEXT("Weapon_ThrowingKnife_01"),
        TEXT("Weapon_Javelin_01"), TEXT("Weapon_Dart_01") };

    UGridItemDefinitionAsset* Contradictory = NewObject<UGridItemDefinitionAsset>(F.Runtime);
    Contradictory->Weight = 1000.0f;
    Contradictory->ThrowSpeed = 0.0f;
    Contradictory->bUseWeaponCatalogue = false;

    if (FCString::Strcmp(Scenario, TEXT("Catalogues")) == 0)
    {
    for (const FName Id : Ids)
    {
        Contradictory->ItemDefinitionId = Id;
        for (const int32 Level : {1, 20})
        {
            const FGridItemInstance Instance = F.Make(Id, Level);
            FGridResolvedItem Resolved;
            if (!Test.TestTrue(TEXT("Real N1/N20 ARM resolves"), F.Registry->ResolveItem(Id, Level, Resolved)))
                return false;
            if (!Test.TestTrue(TEXT("Specialized policy is really authored"), Resolved.bIsArmament &&
                Resolved.Weapon.Projectile.bEnabled)) return false;

            const FGridCanonicalThrowBallisticsReadView Flight =
                FGridCanonicalThrowBallisticsReadModel::Resolve(F.Registry, nullptr, Instance);
            const auto FlightWrong =
                FGridCanonicalThrowBallisticsReadModel::Resolve(F.Registry, Contradictory, Instance);
            Test.TestTrue(TEXT("Canonical ballistics resolve without DA_Weapon"),
                Flight.bCanonicalIdentity && Flight.bResolved && Flight.bSpecializedProjectile);
            Test.TestTrue(TEXT("Old zero speed cannot override canonical projectile"),
                FlightWrong.bResolved && FlightWrong.bSpecializedProjectile);
            Test.TestTrue(TEXT("N-level speed and arc are authoritative"),
                FMath::IsNearlyEqual(Flight.InitialSpeed, Resolved.Weapon.Projectile.InitialSpeed) &&
                FMath::IsNearlyEqual(Flight.Arc, Resolved.Weapon.Projectile.Arc) &&
                FMath::IsNearlyEqual(FlightWrong.InitialSpeed, Flight.InitialSpeed));

            const auto Gate =
                FGridPhysicalThrowStrengthReadModel::Resolve(F.Registry, nullptr, Instance, 100);
            const auto GateWrong =
                FGridPhysicalThrowStrengthReadModel::Resolve(F.Registry, Contradictory, Instance, 100);
            Test.TestTrue(TEXT("Throw gate resolves with canonical item only"),
                Gate.bCanonicalIdentity && Gate.bResolved && Gate.bOneHanded);
            Test.TestTrue(TEXT("Force threshold and mass are canonical"),
                FMath::IsNearlyEqual(Gate.MaxWeightKg, 25.0f) &&
                FMath::IsNearlyEqual(Gate.UnitWeightKg, Resolved.WeightKg));
            Test.TestEqual(TEXT("Legacy weight and speed cannot change throw permission"),
                GateWrong.bCanThrow, Gate.bCanThrow);
            Test.TestTrue(TEXT("Strength 100 launches lightweight specialized arms"),
                Gate.bCanThrow && Gate.SpeedScale > 0.0f);
        }
    }

    return !Test.HasAnyErrors();
    }

    if (FCString::Strcmp(Scenario, TEXT("Projectile")) == 0 ||
        FCString::Strcmp(Scenario, TEXT("Landing")) == 0)
    {
    const FGridItemInstance Valid = F.Make(Ids[0], 20);
    const FVector Start = F.Runtime->GetCellCenterWorld(0, 0, 100.0f);
    const FVector Velocity(1200.0f, 0.0f, 100.0f);
    AGridThrownItemActor* Projectile = F.Runtime->SpawnThrownItemProjectile(
        Valid, nullptr, Start, Velocity, 0, 0);
    if (!Test.TestNotNull(TEXT("Real canonical specialized projectile spawns without old asset"),
        Projectile)) return false;
    Test.TestNull(TEXT("Projectile has no obsolete item asset"), Projectile->GetItemDefinitionAsset());
    Test.TestEqual(TEXT("Projectile retains its ID"), Projectile->GetItemDefinitionId(), Valid.ItemDefinitionId);
    FGridResolvedItem CanonicalFlightSource;
    if (!Test.TestTrue(TEXT("Physical source resolves before presentation checks"),
        F.Registry->ResolveItem(Valid.ItemDefinitionId, Valid.ItemLevel, CanonicalFlightSource))) return false;
    Test.TestTrue(TEXT("Pointerless ARM projectile has complete canonical presentation"),
        Projectile->CanonicalWorldPresentation.IsValid());
    Test.TestEqual(TEXT("Projectile keeps the physical N20 level"),
        Projectile->CanonicalWorldItemLevel, Valid.ItemLevel);
    Test.TestTrue(TEXT("Flight mesh is still the dedicated projectile mesh"),
        Projectile->MeshComponent->GetStaticMesh() == CanonicalFlightSource.ProjectileMesh.LoadSynchronous());
    Test.TestEqual(TEXT("Point-light profile comes from ItemSystem"),
        Projectile->LightEmitterComponent->RuntimeConfig.bUsePointLight,
        CanonicalFlightSource.LightEmitter.bUsePointLight);
    Test.TestEqual(TEXT("Persistent Niagara setup comes from ItemSystem"),
        Projectile->VisualEffectsComponent->GetConfiguredEffectCount(),
        CanonicalFlightSource.VisualEffects.Effects.Num());

    if (FCString::Strcmp(Scenario, TEXT("Projectile")) == 0)
        return !Test.HasAnyErrors();

    Test.TestTrue(TEXT("Canonical projectile can land as real world pickup without old asset"),
        F.Runtime->TryDropItemInstanceAtCell(
            Valid, nullptr, 0, 0, EGridEdge::None, FVector::ZeroVector));
    AGridItemActor* Pickup = nullptr;
    for (TActorIterator<AGridItemActor> It(F.World); It; ++It)
    {
        if (!It->IsA<AGridThrownItemActor>() &&
            It->GetItemDefinitionId() == Valid.ItemDefinitionId &&
            It->GetRuntimeObjectId() == Valid.RuntimeObjectId)
        {
            Pickup = *It;
            break;
        }
    }
    if (!Test.TestNotNull(TEXT("Landing created an addressable pickup with original GUID"),
        Pickup)) return false;
    Test.TestNull(TEXT("Pickup has no legacy definition"), Pickup->GetItemDefinitionAsset());
    Test.TestTrue(TEXT("Pickup has the canonical world mesh"),
        Pickup->MeshComponent && Pickup->MeshComponent->GetStaticMesh() != nullptr);
    Test.TestTrue(TEXT("Canonical pickup weight is independent of old serialized mass"),
        F.Runtime->GetWorldItemWeightAtCell(0, 0) < 1000.0f);

    return !Test.HasAnyErrors();
    }

    if (FCString::Strcmp(Scenario, TEXT("InvalidAndLegacy")) == 0)
    {
    const FVector Start = F.Runtime->GetCellCenterWorld(0, 0, 100.0f);
    const FVector Velocity(1200.0f, 0.0f, 100.0f);
    Contradictory->ItemDefinitionId = Ids[0];
    for (const int32 Invalid : {0, 21})
    {
        const FGridItemInstance Bad = F.Make(Ids[0], Invalid);
        const auto Flight =
            FGridCanonicalThrowBallisticsReadModel::Resolve(F.Registry, Contradictory, Bad);
        const auto Gate =
            FGridPhysicalThrowStrengthReadModel::Resolve(F.Registry, Contradictory, Bad, 100);
        Test.TestTrue(TEXT("Invalid known identity remains registered"),
            Flight.bCanonicalIdentity && Gate.bCanonicalIdentity);
        Test.TestFalse(TEXT("Invalid ARM has no canonical flight"), Flight.bResolved);
        Test.TestFalse(TEXT("Invalid ARM cannot be thrown"), Gate.bCanThrow);
        Test.TestNull(TEXT("Invalid ARM cannot spawn a projectile"),
            F.Runtime->SpawnThrownItemProjectile(Bad, Contradictory, Start, Velocity, 0, 0));
    }

    // The general resolver retains non-ARM N1 family reads for legacy UI.
    // A PHYSICAL arrow N1 must still be rejected before spawning, even if an
    // old matching asset falsely advertises a valid hand-throw policy.
    const FGridItemInstance BadArrow = F.Make(TEXT("Item_Arrows_01"), 1);
    FGridResolvedItem N1Family;
    Test.TestTrue(TEXT("Non-ARM N1 family read remains supported"),
        F.Registry->ResolveItem(BadArrow.ItemDefinitionId, BadArrow.ItemLevel, N1Family));
    UGridItemDefinitionAsset* MatchingLegacyArrow = NewObject<UGridItemDefinitionAsset>(F.Runtime);
    MatchingLegacyArrow->ItemDefinitionId = BadArrow.ItemDefinitionId;
    MatchingLegacyArrow->HandUsage = EGridItemHandUsage::OneHanded;
    MatchingLegacyArrow->Weight = 0.25f;
    MatchingLegacyArrow->ThrowSpeed = 1800.0f;
    Test.TestTrue(TEXT("Legacy arrow fixture would otherwise be throwable"),
        MatchingLegacyArrow->IsPhysicallyThrowable());
    Test.TestNull(TEXT("Registered non-ARM N1 cannot launch via matching old pointer"),
        F.Runtime->SpawnThrownItemProjectile(BadArrow, MatchingLegacyArrow, Start, Velocity, 0, 0));

    const FGridItemInstance Unknown = F.Make(TEXT("ARM063AV_Unknown"), 0);
    Test.TestNull(TEXT("Unknown item without old definition cannot spawn"),
        F.Runtime->SpawnThrownItemProjectile(Unknown, nullptr, Start, Velocity, 0, 0));
    Test.TestFalse(TEXT("Unknown item without old definition has no throw gate"),
        FGridPhysicalThrowStrengthReadModel::Resolve(F.Registry, nullptr, Unknown, 20).bResolved);

    UGridItemDefinitionAsset* Legacy = NewObject<UGridItemDefinitionAsset>(F.Runtime);
    Legacy->ItemDefinitionId = Unknown.ItemDefinitionId;
    Legacy->HandUsage = EGridItemHandUsage::OneHanded;
    Legacy->ThrowSpeed = 1800.0f;
    Legacy->Weight = 0.25f;
    const auto LegacyGate =
        FGridPhysicalThrowStrengthReadModel::Resolve(F.Registry, Legacy, Unknown, 20);
    Test.TestFalse(TEXT("Unregistered item stays on legacy path"), LegacyGate.bCanonicalIdentity);
    Test.TestEqual(TEXT("Legacy speed-scale curve unchanged"),
        LegacyGate.SpeedScale, Legacy->GetThrowSpeedScaleForStrength(20));
    return !Test.HasAnyErrors();
    }
    Test.AddError(TEXT("Unknown ARM-CLEAN06.3AV test stage"));
    return false;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AVCatalogues,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalSpecializedThrowWithoutLegacy.Catalogues",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean063AVCatalogues::RunTest(const FString&)
{
    return ARMCLEAN063AV::RunScenario(*this, TEXT("Catalogues"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AVProjectile,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalSpecializedThrowWithoutLegacy.Projectile",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean063AVProjectile::RunTest(const FString&)
{
    return ARMCLEAN063AV::RunScenario(*this, TEXT("Projectile"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AVLanding,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalSpecializedThrowWithoutLegacy.Landing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean063AVLanding::RunTest(const FString&)
{
    return ARMCLEAN063AV::RunScenario(*this, TEXT("Landing"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AVInvalidAndLegacy,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalSpecializedThrowWithoutLegacy.InvalidAndLegacy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean063AVInvalidAndLegacy::RunTest(const FString&)
{
    return ARMCLEAN063AV::RunScenario(*this, TEXT("InvalidAndLegacy"));
}
#endif

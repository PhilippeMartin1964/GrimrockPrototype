#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../UI/GridInventoryItemMetadataReadModel.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridDungeonAsset.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridThrownItemActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "../Runtime/GridLegacyItemLookup.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridItemVisualEffectsComponent.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridThrowBallisticsReadModel.h"
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
                Registry->ContainsItem(TEXT("Weapon_ThrowingKnife_01"));
        }

        FGridItemInstance Make(FName Id, int32 Level = 1) const
        {
            FGridItemInstance Item;
            Item.RuntimeObjectId = FGuid::NewGuid();
            Item.ItemDefinitionId = Id;
            Item.ItemLevel = Level;
            Item.Quantity = 1;
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

            const FGridThrowBallisticsReadView Flight =
                FGridThrowBallisticsReadModel::Resolve(F.Registry, nullptr, Instance);
            const auto FlightWrong =
                FGridThrowBallisticsReadModel::Resolve(F.Registry, Contradictory, Instance);
            Test.TestTrue(TEXT("Canonical ballistics resolve without DA_Weapon"),
                Flight.bRegisteredIdentity && Flight.bResolved && Flight.bSpecializedProjectile);
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
                Gate.bRegisteredIdentity && Gate.bResolved && Gate.bOneHanded);
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
    const FGridItemInstance BeforeLaunch = Valid;
    const FVector Start = F.Runtime->GetCellCenterWorld(0, 0, 100.0f);
    const FVector Velocity(1200.0f, 0.0f, 100.0f);
    Contradictory->ItemDefinitionId = Valid.ItemDefinitionId;
    UGridItemDefinitionAsset* SuppliedOld = FCString::Strcmp(Scenario, TEXT("Projectile")) == 0
        ? Contradictory : nullptr;
    AGridThrownItemActor* Projectile = F.Runtime->SpawnThrownItemProjectile(
        Valid, SuppliedOld, Start, Velocity, 0, 0);
    if (!Test.TestNotNull(TEXT("Real canonical specialized projectile spawns without old asset"),
        Projectile)) return false;
    Test.TestNull(TEXT("Projectile has no obsolete item asset"), Projectile->GetItemDefinitionAsset());
    Test.TestTrue(TEXT("Input durable instance is unmodified"),
        FGridItemInstance::StaticStruct()->CompareScriptStruct(&Valid, &BeforeLaunch, 0));
    Test.TestEqual(TEXT("Projectile retains its ID"), Projectile->GetItemDefinitionId(), Valid.ItemDefinitionId);
    FGridResolvedItem CanonicalFlightSource;
    if (!Test.TestTrue(TEXT("Physical source resolves before presentation checks"),
        F.Registry->ResolveItem(Valid.ItemDefinitionId, Valid.ItemLevel, CanonicalFlightSource))) return false;
    const auto FlightMetadata = FGridInventoryItemMetadataReadModel::Resolve(F.Registry, nullptr, Projectile->ThrownItemInstance);
    Test.TestTrue(TEXT("In-flight physical identity resolves canonical name and mass"), FlightMetadata.bResolved &&
        FlightMetadata.DisplayName.EqualTo(CanonicalFlightSource.DisplayName) &&
        FMath::IsNearlyEqual(FlightMetadata.UnitWeightKg, CanonicalFlightSource.WeightKg));
    Test.TestEqual(TEXT("Projectile retains its GUID"), Projectile->ThrownItemInstance.RuntimeObjectId, Valid.RuntimeObjectId);
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
            FGridThrowBallisticsReadModel::Resolve(F.Registry, Contradictory, Bad);
        const auto Gate =
            FGridPhysicalThrowStrengthReadModel::Resolve(F.Registry, Contradictory, Bad, 100);
        Test.TestTrue(TEXT("Invalid known identity remains registered"),
            Flight.bRegisteredIdentity && Gate.bRegisteredIdentity);
        Test.TestFalse(TEXT("Invalid ARM has no canonical flight"), Flight.bResolved);
        Test.TestFalse(TEXT("Invalid ARM cannot be thrown"), Gate.bCanThrow);
        Test.TestNull(TEXT("Invalid ARM cannot spawn a projectile"),
            F.Runtime->SpawnThrownItemProjectile(Bad, Contradictory, Start, Velocity, 0, 0));
    }

    // A malformed arrow N1 is rejected at the sole registry and also at
    // spawn, even if an old matching asset advertises a valid throw policy.
    const FGridItemInstance BadArrow = F.Make(TEXT("Item_Arrows_01"), 1);
    FGridResolvedItem N1Family;
    Test.TestFalse(TEXT("Registry rejects former non-ARM N1 family alias"),
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
    Test.TestNull(TEXT("Matching obsolete asset cannot authorize an unknown projectile"),
        F.Runtime->SpawnThrownItemProjectile(Unknown, Legacy, Start, Velocity, 0, 0));
    FGridItemInstance Oversized = F.Make(Ids[0], 20);
    Oversized.Quantity = 2;
    Test.TestNull(TEXT("One physical projectile cannot carry two armaments"),
        F.Runtime->SpawnThrownItemProjectile(Oversized, Contradictory, Start, Velocity, 0, 0));
    Test.TestFalse(TEXT("Unknown identity cannot resolve Strength with matching old asset"),
        FGridPhysicalThrowStrengthReadModel::Resolve(F.Registry, Legacy, Unknown, 20).bResolved);
    const auto LegacyGate =
        FGridPhysicalThrowStrengthReadModel::Resolve(nullptr, Legacy, Unknown, 20);
    Test.TestFalse(TEXT("Unregistered item stays on legacy path"), LegacyGate.bRegisteredIdentity);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065DGenericThrow,
    "Grimrock.RPG.ARM.CLEAN06_5D.PointerlessNonArmThrow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean065DGenericThrow::RunTest(const FString&)
{
    ARMCLEAN063AV::FFixture F;
    if (!TestTrue(TEXT("Configured real registry"), F.Ready())) return false;
    auto* Root = LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem"));
    if (!TestNotNull(TEXT("Native root"), Root)) return false;
    const FVector Start = F.Runtime->GetCellCenterWorld(0, 0, 100.f);
    const FVector Velocity(1200.f, 0.f, 100.f);
    int32 NonArmCount = 0;
    int32 HandheldCount = 0;
    for (const auto& Entry : Root->ItemDefinitions)
    {
        const auto* Native = Cast<UGridNonArmamentDefinitionAsset>(Entry.Get());
        if (!Native) continue;
        ++NonArmCount;
        FGridItemInstance Item = F.Make(Native->ItemId, 0);
        FGridResolvedItem Resolved;
        if (!TestTrue(TEXT("Native N0 resolves"), F.Registry->ResolveItem(Native->ItemId, 0, Resolved))) return false;
        TestTrue(TEXT("Exact authored throw policy projected"),
            FGridNonArmamentThrowPolicy::StaticStruct()->CompareScriptStruct(&Native->GenericThrow, &Resolved.GenericThrow, 0));
        auto* Stale = NewObject<UGridItemDefinitionAsset>(F.Runtime);
        Stale->ItemDefinitionId = Native->ItemId;
        Stale->HandUsage = EGridItemHandUsage::OneHanded;
        Stale->Weight = 0.1f;
        Stale->ThrowSpeed = 777.f;
        const auto* Equipment = Cast<UGridOtherEquipmentDefinitionAsset>(Native);
        const bool bOneHand = Equipment && Equipment->HandPolicy == EGridNonArmamentHandPolicy::OneHanded;
        const auto Gate = FGridPhysicalThrowStrengthReadModel::Resolve(F.Registry, Stale, Item, 100);
        const auto Flight = FGridThrowBallisticsReadModel::Resolve(F.Registry, Stale, Item);
        TestEqual(TEXT("Hand permission does not use obsolete definition"), Gate.bOneHanded, bOneHand);
        if (Flight.bResolved)
            TestEqual(TEXT("Authored speed"), Flight.InitialSpeed, Native->GenericThrow.InitialSpeed);
        if (!bOneHand)
        {
            TestFalse(TEXT("No invented hand permission"), Gate.bCanThrow);
            TestNull(TEXT("No obsolete hand permission at spawn"), F.Runtime->SpawnThrownItemProjectile(Item, Stale, Start, Velocity, 0, 0));
        }
        if (bOneHand && Gate.bCanThrow && Flight.bResolved && !Native->WorldMesh.IsNull())
        {
            ++HandheldCount;
            auto* Projectile = F.Runtime->SpawnThrownItemProjectile(Item, Stale, Start, Velocity, 0, 0);
            if (!TestNotNull(TEXT("Registered non-ARM launches without old asset"), Projectile)) return false;
            TestNull(TEXT("Stale definition was discarded"), Projectile->GetItemDefinitionAsset());
            TestTrue(TEXT("Native world mesh in flight"), Projectile->MeshComponent &&
                Projectile->MeshComponent->GetStaticMesh() == Native->WorldMesh.LoadSynchronous());
            Projectile->Destroy();
        }
        Item.ItemLevel = 1;
        TestFalse(TEXT("Nonphysical non-ARM level refused"), FGridThrowBallisticsReadModel::Resolve(F.Registry, Stale, Item).bResolved);
        TestFalse(TEXT("Nonphysical non-ARM hand gate refused"), FGridPhysicalThrowStrengthReadModel::Resolve(F.Registry, Stale, Item, 100).bResolved);
    }
    TestEqual(TEXT("All non-ARM definitions covered"), NonArmCount, 26);
    TestTrue(TEXT("At least one actual handheld item launched"), HandheldCount > 0);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065D3GenericArmFlight,
    "Grimrock.RPG.ARM.CLEAN06_5D3.PersistedOrdinaryARMFlight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean065D3GenericArmFlight::RunTest(const FString&)
{
    ARMCLEAN063AV::FFixture F;
    if (!TestTrue(TEXT("Actual runtime registry"), F.Ready())) return false;
    auto* Root = LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem"));
    if (!TestNotNull(TEXT("Native root"), Root)) return false;
    int32 Authored = 0;
    for (const auto& D : Root->ItemDefinitions)
    {
        const auto* Arm = Cast<UGridArmamentDefinitionAsset>(D.Get());
        if (!Arm) continue;
        for (int32 N : {1,20})
        {
            FGridResolvedItem Item;
            auto Instance = F.Make(Arm->ItemId, N);
            if (!TestTrue(TEXT("Exact ARM level"), F.Registry->ResolveItem(Arm->ItemId, N, Item))) return false;
            if (Item.Weapon.Projectile.bEnabled) continue;
            if (!TestTrue(TEXT("Every ordinary ARM requires materialized flight policy"), Item.Weapon.Projectile.bGenericThrowAuthored)) return false;
            if (N == 1) ++Authored;
            auto* Stale = NewObject<UGridItemDefinitionAsset>(F.Runtime);
            Stale->ItemDefinitionId = Arm->ItemId;
            Stale->ThrowSpeed = 99999.f;
            Stale->ThrowArc = 99.f;
            Stale->Weight = 99999.f;
            const auto Flight = FGridThrowBallisticsReadModel::Resolve(F.Registry, Stale, Instance);
            const auto Strength = FGridPhysicalThrowStrengthReadModel::Resolve(F.Registry, Stale, Instance, 100);
            TestFalse(TEXT("No specialist attack invented"), Item.Weapon.Projectile.bEnabled || Flight.bSpecializedProjectile);
            TestEqual(TEXT("Canonical speed success"), Flight.bResolved, Item.Weapon.Projectile.InitialSpeed > KINDA_SMALL_NUMBER);
            if (Flight.bResolved) TestEqual(TEXT("Old speed cannot override ordinary flight"), Flight.InitialSpeed, Item.Weapon.Projectile.InitialSpeed);
            if (Strength.bCanThrow && Flight.bResolved && !Item.WorldMesh.IsNull())
            {
                const FVector Start = F.Runtime->GetCellCenterWorld(0, 0, 100.f);
                auto* Projectile = F.Runtime->SpawnThrownItemProjectile(Instance, Stale, Start, FVector(1200,0,100), 0,0);
                if (!TestNotNull(TEXT("Ordinary ARM spawns without old definition"), Projectile)) return false;
                TestNull(TEXT("Old definition discarded"), Projectile->GetItemDefinitionAsset());
                TestTrue(TEXT("Complete canonical world presentation"), Projectile->CanonicalWorldPresentation.IsValid());
                TestEqual(TEXT("Physical N preserved"), Projectile->CanonicalWorldItemLevel, N);
                TestTrue(TEXT("Ordinary flight mesh is real world mesh"), Projectile->MeshComponent->GetStaticMesh() == Item.WorldMesh.LoadSynchronous());
                Projectile->Destroy();
            }
        }
    }
    TestEqual(TEXT("All 19 ordinary ARM policies materialized"), Authored, 19);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065E4DLegacyLookup,
    "Grimrock.RPG.ARM.CLEAN06_5E4D.PhysicalTransferLegacyLookup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean065E4DLegacyLookup::RunTest(const FString&)
{
    ARMCLEAN063AV::FFixture F;
    if (!TestTrue(TEXT("Real registry and runtime"), F.Ready())) return false;
    const FName Known(TEXT("Weapon_StraightSword_01"));
    const FName Unknown(TEXT("ARM065E4D_Unregistered"));
    auto* OldKnown = NewObject<UGridItemDefinitionAsset>(F.Runtime);
    OldKnown->ItemDefinitionId = Known;
    auto* OldUnknown = NewObject<UGridItemDefinitionAsset>(F.Runtime);
    OldUnknown->ItemDefinitionId = Unknown;
    FGridLooseItemInstance KnownPlacement;
    KnownPlacement.CanonicalItemId = Known;
    KnownPlacement.ItemLevel = 1;
    F.Runtime->LevelAsset->LooseItemInstances.Add(KnownPlacement);
    FGridLooseItemInstance UnknownPlacement;
    UnknownPlacement.CanonicalItemId = Unknown;
    F.Runtime->LevelAsset->LooseItemInstances.Add(UnknownPlacement);
    FGridResolvedItem Resolved;
    TestTrue(TEXT("Known ID resolves independently of level placement"), F.Registry->ResolveItem(Known, 1, Resolved));
    TestFalse(TEXT("Unregistered authored ItemId is not a definition catalogue"), F.Registry->ResolveItem(Unknown, 0, Resolved));
    auto* Party = F.World->SpawnActor<AGrimrockPartyPawn>();
    if (!TestNotNull(TEXT("Party"), Party)) return false;
    Party->LevelRuntimeActor = F.Runtime;
    auto* Inventory = Party->PartyInventoryComponent.Get();
    if (!TestNotNull(TEXT("Inventory"), Inventory)) return false;
    Inventory->InitializeDefaultPartyIfNeeded();
    TestTrue(TEXT("Known registration is already satisfied by ItemSystem"), Inventory->RegisterItemDefinition(OldKnown));
    TestNull(TEXT("Registered ID cannot enter the obsolete map"), Inventory->FindItemDefinition(Known));
    TestNull(TEXT("Canonical transfer ignores old inventory entry"), FGridLegacyItemLookup::Resolve(Party, Known));
    TestNull(TEXT("Unknown transfer cannot use an old level entry"), FGridLegacyItemLookup::Resolve(Party, Unknown));
    TestTrue(TEXT("Unknown entry registered for historical fixture"), Inventory->RegisterItemDefinition(OldUnknown));
    TestNull(TEXT("Configured canonical world refuses unknown legacy inventory lookup"),
        FGridLegacyItemLookup::Resolve(Party, Unknown));
    // Isolated Automation without a GameInstance may still characterize
    // old authoring assets, but cannot leak that authority into real games.
    F.World->SetGameInstance(nullptr);
    TestEqual(TEXT("Worldless Automation fixture reads only its explicit old map"),
        FGridLegacyItemLookup::Resolve(Party, Unknown), OldUnknown);
    F.World->SetGameInstance(F.GI);
    TestNull(TEXT("Restored configured world rejects the same old fixture"),
        FGridLegacyItemLookup::Resolve(Party, Unknown));
    TestFalse(TEXT("Explicit inventory map cannot alter ItemSystem"),
        F.Registry->ResolveItem(Unknown, 0, Resolved));
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065G59InFlightTravel,
    "Grimrock.RPG.ARM.CLEAN06_5G59.InFlightItemTravel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean065G59InFlightTravel::RunTest(const FString&)
{
    ARMCLEAN063AV::FFixture F;
    if (!TestTrue(TEXT("Canonical runtime"), F.Ready())) return false;
    UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(F.Runtime);
    UGridLevelAsset* SourceLevel = F.Runtime->LevelAsset;
    UGridLevelAsset* TargetLevel = NewObject<UGridLevelAsset>(Dungeon);
    TargetLevel->Width = 2;
    TargetLevel->Height = 2;
    TargetLevel->EnsureCellCount();
    for (FGridLevelCellData& Cell : TargetLevel->Cells)
    {
        Cell.CellType = EGridCellType::Floor;
        Cell.bBlocksOccupancy = false;
    }
    FGridDungeonLevelEntry Source;
    Source.LevelId = TEXT("FlightSource");
    Source.LevelAsset = SourceLevel;
    Source.bEnabled = true;
    FGridDungeonLevelEntry Target;
    Target.LevelId = TEXT("FlightTarget");
    Target.LevelAsset = TargetLevel;
    Target.bEnabled = true;
    Dungeon->Levels = {Source, Target};
    Dungeon->DefaultLevelId = Source.LevelId;
    F.Runtime->DungeonAsset = Dungeon;
    F.Runtime->CurrentDungeonLevelId = Source.LevelId;
    AGrimrockPartyPawn* Party = F.World->SpawnActor<AGrimrockPartyPawn>();
    if (!TestNotNull(TEXT("Party"), Party)) return false;
    Party->SetGridStart(F.Runtime, 0, 0, EGridEdge::North);
    FGridItemInstance Item = F.Make(TEXT("Weapon_ThrowingKnife_01"), 20);
    Item.ReadableContentId = TEXT("FixtureFlightRead");
    Item.ReadTitleOverride = FText::FromString(TEXT("Flight text"));
    Item.bLightsEnabled = false;
    AGridThrownItemActor* Flight = F.Runtime->SpawnThrownItemProjectile(
        Item, nullptr, F.Runtime->GetCellCenterWorld(0, 0, 100.f), FVector(1200,0,100), 0,0);
    if (!TestNotNull(TEXT("Live non-ammo projectile"), Flight)) return false;
    if (!TestTrue(TEXT("Capture includes hand-thrown armament"), F.Runtime->CaptureCurrentLevelRuntimeState())) return false;
    const FGridLevelRuntimeState* Saved = F.Runtime->FindRuntimeStateForCurrentLevel();
    const FGridRuntimeItemState* Row = Saved ? Saved->Items.Find(Item.RuntimeObjectId) : nullptr;
    if (!TestNotNull(TEXT("Original GUID in source snapshot"), Row)) return false;
    TestEqual(TEXT("Exact N20"), Row->ItemLevel, 20);
    TestEqual(TEXT("Readable metadata preserved"), Row->ReadableContentId, Item.ReadableContentId);
    TestFalse(TEXT("Pickup does not inherit flight mesh scale"), Row->bHasWorldMeshTransform);
    TestFalse(TEXT("Current light state preserved"), Row->bLightsEnabled);
    if (!TestTrue(TEXT("Travel to unexplored level"), F.Runtime->TravelToDungeonLevel(Target.LevelId, 0,0,EGridEdge::North,Party))) return false;
    for (TActorIterator<AGridThrownItemActor> It(F.World); It; ++It)
        if (IsValid(*It) && !It->IsActorBeingDestroyed() && It->GetRuntimeObjectId() == Item.RuntimeObjectId)
        {
            AddError(TEXT("Old projectile escaped into new level"));
            return false;
        }
    if (!TestTrue(TEXT("Return to saved source"), F.Runtime->TravelToDungeonLevel(Source.LevelId, 0,0,EGridEdge::North,Party))) return false;
    int32 Pickups = 0;
    for (TActorIterator<AGridItemActor> It(F.World); It; ++It)
    {
        AGridItemActor* Actor = *It;
        if (!IsValid(Actor) || Actor->IsActorBeingDestroyed() || Actor->GetRuntimeObjectId() != Item.RuntimeObjectId) continue;
        ++Pickups;
        TestFalse(TEXT("Reload yields pickup, not projectile"), Actor->IsA<AGridThrownItemActor>());
        TestEqual(TEXT("Restored N20"), Actor->CanonicalWorldItemLevel, 20);
        TestEqual(TEXT("Restored actual ItemId"), Actor->ItemDefinitionId, Item.ItemDefinitionId);
        TestEqual(TEXT("Restored readable metadata"), Actor->ReadableContentId, Item.ReadableContentId);
        TestFalse(TEXT("Restored light state"), Actor->AreItemLightsEnabled());
        TestNull(TEXT("Restored pickup is pointerless"), Actor->GetItemDefinitionAsset());
    }
    TestEqual(TEXT("Exactly one recoverable physical item"), Pickups, 1);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065G60ImpactRetry,
    "Grimrock.RPG.ARM.CLEAN06_5G60.FailedImpactRetainsPhysicalSource",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean065G60ImpactRetry::RunTest(const FString&)
{
    ARMCLEAN063AV::FFixture F;
    if (!TestTrue(TEXT("Canonical runtime"), F.Ready())) return false;
    const FGridItemInstance Item = F.Make(TEXT("Weapon_ThrowingKnife_01"), 20);
    const FVector Target = F.Runtime->GetCellCenterWorld(0, 0, 100.f);
    AGridThrownItemActor* Projectile = F.Runtime->SpawnThrownItemProjectile(
        Item, nullptr, Target, FVector(1200,0,100), 0,0);
    if (!TestNotNull(TEXT("Physical projectile"), Projectile)) return false;
    // No usable landing cell, including the source fallback.
    for (FGridLevelCellData& Cell : F.Runtime->LevelAsset->Cells)
        Cell.bBlocksOccupancy = true;
    if (!TestFalse(TEXT("Source is not a usable landing cell"), F.Runtime->IsWalkableCell(0,0))) return false;
    FHitResult Hit;
    Hit.ImpactPoint = Target;
    Hit.Location = Target;
    Hit.ImpactNormal = FVector::UpVector;
    // Invoke the actual native stop handler; transient worlds do not
    // necessarily execute reflected actor callbacks outside BeginPlay.
    Projectile->HandleProjectileStopped(Hit);
    if (!TestTrue(TEXT("Failed landing retains actor"), IsValid(Projectile) && !Projectile->IsActorBeingDestroyed())) return false;
    TestFalse(TEXT("Failed landing is not completed conversion"), Projectile->HasCompletedImpactConversion());
    TestEqual(TEXT("Original physical GUID retained"), Projectile->GetRuntimeObjectId(), Item.RuntimeObjectId);
    TestEqual(TEXT("Original N20 retained"), Projectile->CanonicalWorldItemLevel, 20);
    TestTrue(TEXT("Failure schedules actual retry timer"),
        F.World->GetTimerManager().TimerExists(Projectile->ExpirationTimerHandle));
    for (FGridLevelCellData& Cell : F.Runtime->LevelAsset->Cells)
        Cell.bBlocksOccupancy = false;
    if (!TestTrue(TEXT("Source can now receive pickup"), F.Runtime->IsWalkableCell(0,0))) return false;
    // Invoke the exact timer callback, independently of Editor tick timing.
    Projectile->HandleProjectileExpired();
    TestTrue(TEXT("Retry commits conversion"), Projectile->HasCompletedImpactConversion());
    int32 Count = 0;
    for (TActorIterator<AGridItemActor> It(F.World); It; ++It)
    {
        AGridItemActor* Actor = *It;
        if (!IsValid(Actor) || Actor->IsActorBeingDestroyed() || Actor->GetRuntimeObjectId() != Item.RuntimeObjectId) continue;
        ++Count;
        TestFalse(TEXT("Retry yields recoverable pickup"), Actor->IsA<AGridThrownItemActor>());
        TestEqual(TEXT("Retry preserves physical N20"), Actor->CanonicalWorldItemLevel, 20);
    }
    TestEqual(TEXT("Exactly one physical item after retry"), Count, 1);
    return !HasAnyErrors();
}

#endif

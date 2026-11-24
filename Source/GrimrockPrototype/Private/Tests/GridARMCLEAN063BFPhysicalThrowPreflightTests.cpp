#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridPhysicalThrowPreflightReadModel.h"
#include <limits>

namespace ARMCLEAN063BF
{
    struct FFixture
    {
        UWorld* World = nullptr;
        UGameInstance* GI = nullptr;
        UGridItemSubsystem* Registry = nullptr;
        AGrimrockPartyPawn* Pawn = nullptr;

        FFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM063BF_%s"),
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
            Pawn = World->SpawnActor<AGrimrockPartyPawn>();
            if (Pawn && Pawn->PartyInventoryComponent)
                Pawn->PartyInventoryComponent->InitializeDefaultPartyIfNeeded();
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
            return Registry && Pawn && Pawn->PartyInventoryComponent &&
                Pawn->PartyInventoryComponent->IsValidCharacterIndex(0) &&
                Registry->ContainsItem(TEXT("Weapon_ThrowingKnife_01")) &&
                Registry->ContainsItem(TEXT("Weapon_StraightSword_01"));
        }
        FGridItemInstance Make(FName Id, int32 Level=1) const
        {
            FGridItemInstance I;
            I.RuntimeObjectId = FGuid::NewGuid();
            I.ItemDefinitionId = Id;
            I.ItemLevel = Level;
            I.Quantity = 1;
            I.OwnerType = EGridItemOwnerType::EquipmentSlot;
            I.EquipmentSlot = EGridEquipmentSlot::MainHand;
            I.OwnerCharacterIndex = 0;
            return I;
        }
        int32 Strength() const
        {
            FGridInventoryCharacterSummary Summary;
            return Pawn && Pawn->PartyInventoryComponent &&
                Pawn->PartyInventoryComponent->GetCharacterSummary(0, Summary)
                ? Summary.Attributes.Strength : 0;
        }
        bool MenuThrow(const FGridItemInstance& I,
            bool& bPresent) const
        {
            FGridItemActionContext Context;
            Context.PartyPawn = Pawn;
            Context.Item = I;
            Context.CharacterIndex = 0;
            Context.EquipmentSlot = EGridEquipmentSlot::MainHand;
            FGridFacingTargetContext Target;
            TArray<FGridItemContextAction> Actions;
            UGridItemContextActionLibrary::BuildItemContextActions(Context, Target, Actions);
            const FGridItemContextAction* Throw = Actions.FindByPredicate(
                [](const FGridItemContextAction& A)
                {
                    return A.ActionType == EGridItemActionType::Throw;
                });
            bPresent = Throw != nullptr;
            return Throw && Throw->bEnabled;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridBFCanonical,
    "Grimrock.RPG.ARM.CLEAN06_3.PhysicalThrowPreflight.CanonicalAndMenu",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridBFCanonical::RunTest(const FString&)
{
    ARMCLEAN063BF::FFixture F;
    if (!TestTrue(TEXT("Real registry and character"), F.Ready())) return false;
    for (const FName Id : {FName(TEXT("Weapon_ThrowingKnife_01")),
        FName(TEXT("Weapon_Javelin_01")), FName(TEXT("Weapon_Dart_01"))})
    {
        for (int32 Level : {1, 20})
        {
            const auto I = F.Make(Id, Level);
            const auto NoOld = FGridPhysicalThrowPreflightReadModel::Resolve(
                F.Registry, I, 100);
            const auto Wrong = FGridPhysicalThrowPreflightReadModel::Resolve(
                F.Registry, I, 100);
            TestTrue(TEXT("Real canonical specialized N1/N20 flight is valid"),
                NoOld.bRegisteredIdentity && NoOld.bResolved && NoOld.bCanThrow &&
                NoOld.Ballistics.bSpecializedProjectile);
        }
        const auto I = F.Make(Id, 1);
        const auto Expected = FGridPhysicalThrowPreflightReadModel::Resolve(
            F.Registry, I, F.Strength());
        bool Present = false;
        const bool Enabled = F.MenuThrow(I, Present);
        TestTrue(TEXT("Physical throw command is offered by actual menu"), Present);
        TestEqual(TEXT("Menu uses same actual Strength and flight as Pawn"),
            Enabled, Expected.bCanThrow);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridBFGeneric,
    "Grimrock.RPG.ARM.CLEAN06_3.PhysicalThrowPreflight.GenericFallback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridBFGeneric::RunTest(const FString&)
{
    ARMCLEAN063BF::FFixture F;
    if (!TestTrue(TEXT("Real registry and character"), F.Ready())) return false;
    const auto I = F.Make(TEXT("Weapon_StraightSword_01"));
    FGridResolvedItem Canonical;
    if (!TestTrue(TEXT("Real ordinary ARM N1 resolves"), F.Registry->ResolveItem(I.ItemDefinitionId, I.ItemLevel, Canonical))) return false;
    if (!TestTrue(TEXT("5D.3 ordinary flight must be materialized"), Canonical.Weapon.Projectile.bGenericThrowAuthored)) return false;
    {
        // After the native cutover, a matching obsolete definition must
        // neither enable, disable nor modify a real registered flight.
        const auto& Policy = Canonical.Weapon.Projectile;
        TestFalse(TEXT("Ordinary hand toss is not a specialized combat projectile"), Policy.bEnabled);
        const bool bExpectedFlight = FMath::IsFinite(Policy.InitialSpeed) &&
            Policy.InitialSpeed > KINDA_SMALL_NUMBER && FMath::IsFinite(Policy.Arc);
        const bool bExpectedPermission = bExpectedFlight && Policy.bGenericThrowOneHanded &&
            Canonical.WeightKg >= 0.0f && Canonical.WeightKg <= 25.0f + KINDA_SMALL_NUMBER;
        const auto Native = FGridPhysicalThrowPreflightReadModel::Resolve(F.Registry, I, 100);
        TestTrue(TEXT("Ordinary registered source remains canonical"), Native.bRegisteredIdentity);
        TestFalse(TEXT("No specialized action authorized"), Native.Ballistics.bSpecializedProjectile);
        TestEqual(TEXT("Native flight resolution matches exact authored validity"), Native.bResolved, bExpectedFlight);
        TestEqual(TEXT("Native physical permission matches authored hand, mass and speed"), Native.bCanThrow, bExpectedPermission);
        if (bExpectedFlight)
        {
            TestEqual(TEXT("Native speed is authoritative"), Native.Ballistics.InitialSpeed, Policy.InitialSpeed);
            TestEqual(TEXT("Native signed arc is authoritative"), Native.Ballistics.Arc, Policy.Arc);
        }

        const auto ExpectedMenu = FGridPhysicalThrowPreflightReadModel::Resolve(F.Registry, I, F.Strength());
        bool bPresent = false;
        const bool bEnabled = F.MenuThrow(I, bPresent);
        TestTrue(TEXT("Actual main-hand context exposes Throw"), bPresent);
        TestEqual(TEXT("Actual menu uses native policy at actual strength"), bEnabled, ExpectedMenu.bCanThrow);
        for (int32 InvalidLevel : {0,21})
        {
            const auto Invalid = FGridPhysicalThrowPreflightReadModel::Resolve(
                F.Registry, F.Make(I.ItemDefinitionId, InvalidLevel), 100);
            TestTrue(TEXT("Invalid physical level cannot borrow old data"),
                Invalid.bRegisteredIdentity && !Invalid.bResolved && !Invalid.bCanThrow);
        }
        return !HasAnyErrors();
    }


}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridBFLegacy,
    "Grimrock.RPG.ARM.CLEAN06_3.PhysicalThrowPreflight.LegacyAndInvalid",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridBFLegacy::RunTest(const FString&)
{
    ARMCLEAN063BF::FFixture F;
    if (!TestTrue(TEXT("Real registry and character"), F.Ready())) return false;
    const auto I = F.Make(TEXT("BF_Unregistered_HandThrow"), 0);
    const auto Rejected = FGridPhysicalThrowPreflightReadModel::Resolve(F.Registry, I, 25);
    TestFalse(TEXT("Configured registry cannot advertise a throw that launch refuses"), Rejected.bCanThrow);
    TestFalse(TEXT("Unknown item has no canonical preflight"), Rejected.bResolved);
    bool Present = false;
    TestFalse(TEXT("Unknown item cannot launch via actual context menu"), F.MenuThrow(I, Present));
    TestTrue(TEXT("An unavailable throw remains visible but disabled"), Present);
    const auto Valid = FGridPhysicalThrowPreflightReadModel::Resolve(
        nullptr, I, 25);
    TestTrue(TEXT("Registry-less throw yields no flight or permission"),
        !Valid.bRegisteredIdentity && !Valid.bResolved && !Valid.bCanThrow && Valid.Ballistics.Arc == 0.0f);
    for (int32 Level : {0, 21})
    {
        const auto Bad = FGridPhysicalThrowPreflightReadModel::Resolve(
            F.Registry, F.Make(TEXT("Weapon_ThrowingKnife_01"), Level), 100);
        TestTrue(TEXT("Registered invalid ARM N0/N21 never receives old throw"),
            Bad.bRegisteredIdentity && !Bad.bResolved && !Bad.bCanThrow);
    }
    return !HasAnyErrors();
}
#endif

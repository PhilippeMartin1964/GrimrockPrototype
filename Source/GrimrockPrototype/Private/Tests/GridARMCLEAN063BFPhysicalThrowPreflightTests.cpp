#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridItemDefinitionAsset.h"
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
                Registry->HasCanonicalItem(TEXT("Weapon_ThrowingKnife_01")) &&
                Registry->HasCanonicalItem(TEXT("Weapon_StraightSword_01"));
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
            UGridItemDefinitionAsset* Old, bool& bPresent) const
        {
            FGridItemActionContext Context;
            Context.PartyPawn = Pawn;
            Context.Item = I;
            Context.ItemDefinition = Old;
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
    UGridItemDefinitionAsset* Old = NewObject<UGridItemDefinitionAsset>(F.GI);
    Old->ThrowSpeed = 0.0f;
    Old->ThrowArc = std::numeric_limits<float>::quiet_NaN();
    for (const FName Id : {FName(TEXT("Weapon_ThrowingKnife_01")),
        FName(TEXT("Weapon_Javelin_01")), FName(TEXT("Weapon_Dart_01"))})
    {
        Old->ItemDefinitionId = Id;
        for (int32 Level : {1, 20})
        {
            const auto I = F.Make(Id, Level);
            const auto NoOld = FGridPhysicalThrowPreflightReadModel::Resolve(
                F.Registry, nullptr, I, 100);
            const auto Wrong = FGridPhysicalThrowPreflightReadModel::Resolve(
                F.Registry, Old, I, 100);
            TestTrue(TEXT("Real canonical specialized N1/N20 flight is valid"),
                NoOld.bCanonicalIdentity && NoOld.bResolved && NoOld.bCanThrow &&
                NoOld.Ballistics.bSpecializedProjectile);
            TestTrue(TEXT("Contradictory old flight cannot override canonical policy"),
                Wrong.bCanThrow && Wrong.Ballistics.bSpecializedProjectile &&
                FMath::IsNearlyEqual(Wrong.Ballistics.InitialSpeed, NoOld.Ballistics.InitialSpeed));
        }
        const auto I = F.Make(Id, 1);
        const auto Expected = FGridPhysicalThrowPreflightReadModel::Resolve(
            F.Registry, Old, I, F.Strength());
        bool Present = false;
        const bool Enabled = F.MenuThrow(I, Old, Present);
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
    UGridItemDefinitionAsset* Old = NewObject<UGridItemDefinitionAsset>(F.GI);
    Old->ItemDefinitionId = TEXT("Weapon_StraightSword_01");
    Old->HandUsage = EGridItemHandUsage::OneHanded;
    Old->Weight = 0.05f;
    Old->ThrowSpeed = 1200.0f;
    Old->ThrowArc = -0.12f;
    const auto I = F.Make(Old->ItemDefinitionId);
    const auto Missing = FGridPhysicalThrowPreflightReadModel::Resolve(
        F.Registry, nullptr, I, 100);
    TestTrue(TEXT("Generic canonical hand toss cannot invent missing physics"),
        Missing.bCanonicalIdentity && !Missing.bCanThrow && !Missing.bResolved);
    const auto Legacy = FGridPhysicalThrowPreflightReadModel::Resolve(
        F.Registry, Old, I, 100);
    TestTrue(TEXT("Existing ordinary throw retains speed and signed arc"),
        Legacy.bResolved && Legacy.bCanThrow &&
        FMath::IsNearlyEqual(Legacy.Ballistics.InitialSpeed, 1200.0f) &&
        FMath::IsNearlyEqual(Legacy.Ballistics.Arc, -0.12f));
    Old->ThrowArc = std::numeric_limits<float>::quiet_NaN();
    const auto BadArc = FGridPhysicalThrowPreflightReadModel::Resolve(
        F.Registry, Old, I, 100);
    TestTrue(TEXT("Gate alone cannot authorize missing valid flight"),
        BadArc.Strength.bCanThrow && !BadArc.Ballistics.bResolved &&
        !BadArc.bResolved && !BadArc.bCanThrow);
    bool Present = false;
    const bool Enabled = F.MenuThrow(I, Old, Present);
    TestTrue(TEXT("Actual MainHand context offers Throw, but disabled"), Present);
    TestFalse(TEXT("Actual menu refuses undefined flight"), Enabled);
    Old->ThrowArc = 0.1f;
    Old->ThrowSpeed = 0.0f;
    TestFalse(TEXT("Zero speed cannot create a thrown item"),
        FGridPhysicalThrowPreflightReadModel::Resolve(F.Registry, Old, I, 100).bCanThrow);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridBFLegacy,
    "Grimrock.RPG.ARM.CLEAN06_3.PhysicalThrowPreflight.LegacyAndInvalid",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridBFLegacy::RunTest(const FString&)
{
    ARMCLEAN063BF::FFixture F;
    if (!TestTrue(TEXT("Real registry and character"), F.Ready())) return false;
    UGridItemDefinitionAsset* Old = NewObject<UGridItemDefinitionAsset>(F.GI);
    Old->ItemDefinitionId = TEXT("BF_Unregistered_HandThrow");
    Old->HandUsage = EGridItemHandUsage::OneHanded;
    Old->Weight = 0.25f;
    Old->ThrowSpeed = 1800.0f;
    Old->ThrowArc = 0.08f;
    const auto I = F.Make(Old->ItemDefinitionId, 0);
    const auto Valid = FGridPhysicalThrowPreflightReadModel::Resolve(
        F.Registry, Old, I, 25);
    TestTrue(TEXT("Historical unregistered throw uses exact old physics"),
        !Valid.bCanonicalIdentity && Valid.bResolved && Valid.bCanThrow &&
        FMath::IsNearlyEqual(Valid.Ballistics.Arc, Old->ThrowArc));
    TestFalse(TEXT("Wrong old definition cannot supply speed"),
        FGridPhysicalThrowPreflightReadModel::Resolve(
            F.Registry, Old, F.Make(TEXT("BF_Unregistered_Other"), 0), 25).bCanThrow);
    TestFalse(TEXT("Missing old definition cannot supply speed"),
        FGridPhysicalThrowPreflightReadModel::Resolve(F.Registry, nullptr, I, 25).bCanThrow);
    Old->ItemDefinitionId = TEXT("Weapon_ThrowingKnife_01");
    for (int32 Level : {0, 21})
    {
        const auto Bad = FGridPhysicalThrowPreflightReadModel::Resolve(
            F.Registry, Old, F.Make(Old->ItemDefinitionId, Level), 100);
        TestTrue(TEXT("Registered invalid ARM N0/N21 never receives old throw"),
            Bad.bCanonicalIdentity && !Bad.bResolved && !Bad.bCanThrow);
    }
    return !HasAnyErrors();
}
#endif

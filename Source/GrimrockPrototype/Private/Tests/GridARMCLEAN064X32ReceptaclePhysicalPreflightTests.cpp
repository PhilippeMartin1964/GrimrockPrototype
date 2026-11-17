#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/Items/GridItemSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X32ReceptacleAcceptance,
    "Grimrock.RPG.ARM.CLEAN06_4X32.ReceptacleCursorPhysicalLevelPreflight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X32ReceptacleAcceptance::RunTest(const FString&)
{
    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false)
        .CreatePhysicsScene(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false)
        .SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
        FName(*FString::Printf(TEXT("ARM_CLEAN064X32_%s"),
            *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
        nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Transient runtime world"), World)) return false;
    if (GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    UGameInstance* GI = NewObject<UGameInstance>(GetTransientPackage());
    GI->Init();
    World->SetGameInstance(GI);
    const UGridItemSubsystem* Registry = GI->GetSubsystem<UGridItemSubsystem>();
    AGridReceptacleActor* Receptacle = World->SpawnActor<AGridReceptacleActor>();
    bool bOk = TestNotNull(TEXT("Registered real ItemSystem"), Registry) &&
        TestNotNull(TEXT("Real receptacle actor"), Receptacle);
    if (bOk)
    {
        const FName Dagger(TEXT("Weapon_Dagger_01"));
        const FName Copper(TEXT("Key_Copper"));
        FGridResolvedItem Resolved;
        bOk &= TestTrue(TEXT("Real ARM N1 resolves"), Registry->ResolveItem(Dagger, 1, Resolved));
        bOk &= TestTrue(TEXT("Real key N0 resolves"), Registry->ResolveItem(Copper, 0, Resolved));

        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid();
        Item.Quantity = 1;
        FGridReceptacleAcceptanceResult Decision;

        Item.ItemDefinitionId = Dagger;
        Item.ItemLevel = 1;
        bOk &= TestTrue(TEXT("ARM N1 is accepted in any-item receptacle"),
            Receptacle->EvaluateItemAcceptance(Item, Decision));
        bOk &= TestTrue(TEXT("CanAcceptItemInstance matches accepted ARM"),
            Receptacle->CanAcceptItemInstance(Item));
        Item.ItemLevel = 20;
        bOk &= TestTrue(TEXT("ARM N20 is accepted"),
            Receptacle->EvaluateItemAcceptance(Item, Decision));
        Item.ItemLevel = 0;
        bOk &= TestFalse(TEXT("Persisted ARM N0 rejected before insertion"),
            Receptacle->EvaluateItemAcceptance(Item, Decision));
        bOk &= TestEqual(TEXT("ARM N0 uses InvalidItem reason"),
            Decision.RejectReason, EGridReceptacleRejectReason::InvalidItem);
        bOk &= TestFalse(TEXT("ARM N0 is not advertised for cursor insertion"),
            Receptacle->CanAcceptItemInstance(Item));

        Item.ItemDefinitionId = Copper;
        Item.ItemLevel = 0;
        bOk &= TestTrue(TEXT("Canonical non-ARM N0 remains accepted"),
            Receptacle->EvaluateItemAcceptance(Item, Decision));
        Item.ItemLevel = 1;
        bOk &= TestFalse(TEXT("Non-ARM N1 rejected before insertion"),
            Receptacle->EvaluateItemAcceptance(Item, Decision));
        bOk &= TestEqual(TEXT("Non-ARM N1 uses InvalidItem reason"),
            Decision.RejectReason, EGridReceptacleRejectReason::InvalidItem);
        bOk &= TestFalse(TEXT("Non-ARM N1 is not advertised for cursor insertion"),
            Receptacle->CanAcceptItemInstance(Item));

        Item.ItemDefinitionId = FName(TEXT("ARM_CLEAN064X32_Unregistered"));
        Item.ItemLevel = 0;
        bOk &= TestTrue(TEXT("Unknown legacy ID retains acceptance policy"),
            Receptacle->EvaluateItemAcceptance(Item, Decision));
        Item.ItemLevel = 7;
        bOk &= TestTrue(TEXT("Unknown legacy fixed level remains compatible"),
            Receptacle->EvaluateItemAcceptance(Item, Decision));

        Item.RuntimeObjectId.Invalidate();
        bOk &= TestFalse(TEXT("Invalid instance still rejected"),
            Receptacle->EvaluateItemAcceptance(Item, Decision));
        bOk &= TestEqual(TEXT("Invalid GUID uses existing reason"),
            Decision.RejectReason, EGridReceptacleRejectReason::InvalidItem);
        Item.RuntimeObjectId = FGuid::NewGuid();

        Receptacle->bCanInsertItems = false;
        bOk &= TestFalse(TEXT("Disabled insertion still rejects"),
            Receptacle->EvaluateItemAcceptance(Item, Decision));
        bOk &= TestEqual(TEXT("Disabled insertion keeps its precedence"),
            Decision.RejectReason, EGridReceptacleRejectReason::InsertionDisabled);
        Receptacle->bCanInsertItems = true;
        bOk &= TestEqual(TEXT("Preflight never changes receptacle contents"),
            Receptacle->GetContainedItemCount(), 0);
    }
    World->SetGameInstance(nullptr);
    GI->Shutdown();
    World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World);
    return bOk && !HasAnyErrors();
}
#endif

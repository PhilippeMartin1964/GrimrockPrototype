#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridReceptacleAcceptedReadModel.h"

namespace ARMCLEAN064X161
{
struct FFixture
{
    UWorld* World = nullptr;
    UGameInstance* GI = nullptr;
    UGridItemSubsystem* Registry = nullptr;
    FFixture()
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).RequiresHitProxies(false)
            .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false)
            .ShouldSimulatePhysics(false).SetTransactional(false);
        World = UWorld::CreateWorld(EWorldType::Game, false,
            FName(*FString::Printf(TEXT("ARM_CLEAN064X161_%s"),
                *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
            nullptr, true, ERHIFeatureLevel::Num, &Values);
        if (!World || !GEngine) {return;}
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
        Context.SetCurrentWorld(World);
        GI = NewObject<UGameInstance>(GetTransientPackage());
        GI->Init();
        World->SetGameInstance(GI);
        Registry = GI->GetSubsystem<UGridItemSubsystem>();
    }
    ~FFixture()
    {
        if (World) {World->SetGameInstance(nullptr);}
        if (GI) {GI->Shutdown();}
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) {GEngine->DestroyWorldContext(World);}
        }
    }
    AGridReceptacleActor* Spawn(const FGridReceptacleBehaviorParams& Rules)
    {
        if (!World) {return nullptr;}
        AGridReceptacleActor* Actor = World->SpawnActor<AGridReceptacleActor>();
        if (!Actor) {return nullptr;}
        FGridRuntimeWorldObjectData Data;
        Data.ObjectId = FGuid::NewGuid();
        Data.CellX = 0;
        Data.CellY = 0;
        Data.Behavior.Receptacle = Rules;
        Actor->InitializeRuntimeWorldObject(Data, nullptr, FTransform::Identity);
        return Actor;
    }
};
FGridItemInstance Item(FName Id)
{
    FGridItemInstance Out;
    Out.RuntimeObjectId = FGuid::NewGuid();
    Out.ItemDefinitionId = Id;
    Out.Quantity = 1;
    Out.ItemLevel = 0;
    Out.OwnerType = EGridItemOwnerType::World;
    return Out;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X161Rules,
    "Grimrock.RPG.ARM.CLEAN06_4X161.CanonicalReceptacleRuleReader",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X161Rules::RunTest(const FString&)
{
    ARMCLEAN064X161::FFixture F;
    if (!TestNotNull(TEXT("Canonical item registry"), F.Registry) ||
        !TestTrue(TEXT("Production arrow and sword identities registered"),
            F.Registry->ContainsItem(TEXT("Item_Arrows_01")) &&
            F.Registry->ContainsItem(TEXT("Weapon_StraightSword_01"))))
        {return false;}

    FGridReceptacleAcceptedItemConfig Rule;
    Rule.ItemId = TEXT("Item_Arrows_01");
    FName Found = NAME_None;
    if (!TestTrue(TEXT("Canonical ID-only rule resolves"),
            FGridReceptacleAcceptedReadModel::TryResolveItemId(F.Registry, Rule, Found)) ||
        !TestEqual(TEXT("ID-only rule is exact"), Found, Rule.ItemId) ||
        !TestFalse(TEXT("Canonical rule without registry fails closed"),
            FGridReceptacleAcceptedReadModel::TryResolveItemId(nullptr, Rule, Found)) ||
        !TestTrue(TEXT("Failure clears previous result"), Found.IsNone()))
        {return false;}

    FGridReceptacleAcceptedItemConfig Unknown = Rule;
    Unknown.ItemId = TEXT("ARM_CLEAN06_X161_NOT_REGISTERED");
    if (!TestFalse(TEXT("Unknown persisted ItemId cannot fallback"),
            FGridReceptacleAcceptedReadModel::TryResolveItemId(F.Registry, Unknown, Found)))
        {return false;}
    FGridReceptacleAcceptedItemConfig Sword;
    Sword.ItemId = TEXT("Weapon_StraightSword_01");
    FGridReceptacleAcceptedItemConfig Empty;
    if (!TestFalse(TEXT("Empty identity cannot accept"), FGridReceptacleAcceptedReadModel::TryResolveItemId(F.Registry, Empty, Found))) return false;
    FGridReceptacleBehaviorParams Params;
    Params.bAcceptAnyItem = false;
    Params.MaxContainedItems = 2;
    Params.AcceptedItems.Add(Rule);
    AGridReceptacleActor* R = F.Spawn(Params);
    if (!TestNotNull(TEXT("Runtime receptacle"), R) ||
        !TestTrue(TEXT("Accepted canonical arrow matches"),
            R->CanAcceptItem(TEXT("Item_Arrows_01"))) ||
        !TestFalse(TEXT("Unlisted sword does not match"),
            R->CanAcceptItem(TEXT("Weapon_StraightSword_01"))))
        {return false;}
    FGridReceptacleAcceptanceResult AcceptResult;
    FGridReceptacleAcceptanceResult RejectResult;
    if (!TestTrue(TEXT("Canonical cursor arrow allowed"),
            R->EvaluateItemAcceptance(ARMCLEAN064X161::Item(TEXT("Item_Arrows_01")), AcceptResult)) ||
        !TestFalse(TEXT("Unlisted cursor sword rejected"),
            R->EvaluateItemAcceptance(ARMCLEAN064X161::Item(TEXT("Weapon_StraightSword_01")), RejectResult)) ||
        !TestEqual(TEXT("Unlisted object preserves reject reason"),
            RejectResult.RejectReason, EGridReceptacleRejectReason::NoMatchingAcceptanceRule))
        {return false;}
    Params.AcceptedItems[0] = Unknown;
    AGridReceptacleActor* Invalid = F.Spawn(Params);
    if (!TestNotNull(TEXT("Fail-closed rule host"), Invalid) ||
        !TestFalse(TEXT("Unknown canonical rule cannot accept previous arrow"),
            Invalid->CanAcceptItem(TEXT("Item_Arrows_01"))))
        {return false;}
    Params.bAcceptAnyItem = true;
    AGridReceptacleActor* AcceptAny = F.Spawn(Params);
    if (!TestNotNull(TEXT("Accept-any rule host"), AcceptAny) ||
        !TestTrue(TEXT("Explicit accept-any remains permissive"),
            AcceptAny->CanAcceptItem(TEXT("Item_Arrows_01"))))
        {return false;}

    UGridWorldObjectDefinitionAsset* Definition =
        NewObject<UGridWorldObjectDefinitionAsset>(F.World);
    Definition->DefaultBehavior.Receptacle = Params;
    Definition->DefaultBehavior.Receptacle.bAcceptAnyItem = false;
    Definition->DefaultBehavior.Receptacle.AcceptedItems = { Rule };

    FGridWorldObjectInstance Placement;
    Placement.InstanceId = FGuid::NewGuid();
    Placement.InstanceConfig.InteractionOverrides.bOverrideReceptacleRules = true;
    Placement.InstanceConfig.InteractionOverrides.ReceptacleRules.bAcceptAnyItem = false;
    Placement.InstanceConfig.InteractionOverrides.ReceptacleRules.AcceptedItems = { Sword };
    Placement.InstanceConfig.InteractionOverrides.ReceptacleRules.MaxContainedItems = 3;
    const FGridObjectBehaviorParams Effective =
        GridObjectInstanceBehavior::Resolve(Placement, Definition);
    if (!TestTrue(TEXT("Instance override replaces definition rule"),
            Effective.Receptacle.AcceptedItems.Num() == 1 &&
            Effective.Receptacle.AcceptedItems[0].ItemId == Sword.ItemId) ||
        !TestEqual(TEXT("Override capacity remains instance-owned"),
            Effective.Receptacle.MaxContainedItems, 3))
        {return false;}
    AGridReceptacleActor* Overridden = F.Spawn(Effective.Receptacle);
    if (!TestNotNull(TEXT("Canonical override host"), Overridden) ||
        !TestTrue(TEXT("Explicit sword override accepted"),
            Overridden->CanAcceptItem(TEXT("Weapon_StraightSword_01"))) ||
        !TestFalse(TEXT("Default canonical rule does not leak through override"),
            Overridden->CanAcceptItem(TEXT("Item_Arrows_01"))))
        {return false;}

    AddInfo(TEXT("[ARM-CLEAN06.4X.16.1] canonical acceptance is exact and fail-closed; "
        "sparse override semantics preserved; no binary writes."));
    return !HasAnyErrors();
}
#endif

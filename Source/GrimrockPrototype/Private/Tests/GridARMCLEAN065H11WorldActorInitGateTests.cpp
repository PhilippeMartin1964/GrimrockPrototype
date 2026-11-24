#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "GridCanonicalLootTestScope.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/Items/GridItemSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065H11LegacyActorGate,
    "Grimrock.RPG.ARM.CLEAN06_5H.OldWorldActorInitializationGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean065H11LegacyActorGate::RunTest(const FString&)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Transient world"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    AGridItemActor* Actor = World->SpawnActor<AGridItemActor>();
    if (!TestNotNull(TEXT("Transient actor"), Actor)) return false;
    FGridItemInstance Item;
    Item.ItemDefinitionId = TEXT("Weapon_Dagger_01");
    Item.RuntimeObjectId = FGuid::NewGuid();
    Item.ItemLevel = 1;
    Item.Quantity = 1;
    TestFalse(TEXT("Physical materialization requires a registry"), Actor->InitializeFromItemInstance(Item, nullptr));
    TestTrue(TEXT("Registry-less refusal leaves identity, GUID and presentation unset"),
        Actor->GetItemDefinitionId().IsNone() && !Actor->GetRuntimeObjectId().IsValid() &&
        !Actor->ResolvedWorldPresentation.IsValid());

    FGridCanonicalLootTestScope Scope;
    if (!TestTrue(TEXT("Production registry configured for the isolated fixture"), Scope.Attach(World)))
        return false;
    Item.ItemLevel = 0;
    TestFalse(TEXT("A known ARM identity cannot materialize at N0"), Actor->InitializeFromItemInstance(Item, Scope.Registry));
    TestTrue(TEXT("Level refusal leaves identity, GUID and presentation unset"),
        Actor->GetItemDefinitionId().IsNone() && !Actor->GetRuntimeObjectId().IsValid() &&
        !Actor->ResolvedWorldPresentation.IsValid());
    return !HasAnyErrors();
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/Items/GridItemSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X31ReceptacleTypeConditions,
    "Grimrock.RPG.ARM.CLEAN06_4X31.CanonicalReceptacleTypeConditions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X31ReceptacleTypeConditions::RunTest(const FString&)
{
    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false)
        .CreatePhysicsScene(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false)
        .SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
        FName(*FString::Printf(TEXT("ARM_CLEAN064X31_%s"),
            *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
        nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Transient world"), World)) return false;
    if (GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    UGameInstance* GI = NewObject<UGameInstance>(GetTransientPackage());
    GI->Init();
    World->SetGameInstance(GI);
    UGridItemSubsystem* Registry = GI->GetSubsystem<UGridItemSubsystem>();
    AGridReceptacleActor* Receptacle = World->SpawnActor<AGridReceptacleActor>();
    bool bOk = TestNotNull(TEXT("Real ItemSystem"), Registry) &&
        TestNotNull(TEXT("Real receptacle"), Receptacle);

    if (bOk)
    {
        const FName Dagger(TEXT("Weapon_Dagger_01"));
        const FName Shield(TEXT("Weapon_RoundShield_01"));
        const FName Key(TEXT("Key_Copper"));
        FGridResolvedItem Resolved;
        bOk &= TestTrue(TEXT("Dagger N1 belongs to actual registry"),
            Registry->ResolveItem(Dagger, 1, Resolved));
        bOk &= TestTrue(TEXT("Shield N1 belongs to actual registry"),
            Registry->ResolveItem(Shield, 1, Resolved));
        bOk &= TestTrue(TEXT("Copper key N0 belongs to actual registry"),
            Registry->ResolveItem(Key, 0, Resolved));

        AGridItemActor* StaleVisual = World->SpawnActor<AGridItemActor>();
        if (!TestNotNull(TEXT("Optional stale visual actor"), StaleVisual))
        {
            World->SetGameInstance(nullptr);
            GI->Shutdown();
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
            return false;
        }
        auto Contain = [Receptacle, StaleVisual](FName Id, int32 Level, bool bAttachVisual)
        {
            Receptacle->ContainedItems.Reset();
            FGridContainedReceptacleItem& Item = Receptacle->ContainedItems.AddDefaulted_GetRef();
            Item.ItemDefinitionId = Id;
            Item.ItemLevel = Level;
            Item.RuntimeObjectId = FGuid::NewGuid();
            if (bAttachVisual)
            {
                StaleVisual->ItemDefinitionId = Id;
                StaleVisual->WorldItemLevel = Level;
                StaleVisual->SetRuntimeObjectId(Item.RuntimeObjectId);
                Item.ItemActor = StaleVisual;
            }
        };

        Contain(Dagger, 1, false);
        bOk &= TestTrue(TEXT("Pointerless N1 weapon satisfies type condition"),
            Receptacle->ContainsItemType(EGridItemType::Weapon));
        bOk &= TestFalse(TEXT("Pointerless weapon is not shield"),
            Receptacle->ContainsItemType(EGridItemType::Shield));
        Contain(Dagger, 20, false);
        bOk &= TestTrue(TEXT("Pointerless N20 weapon remains recognized"),
            Receptacle->ContainsItemType(EGridItemType::Weapon));
        Contain(Shield, 1, false);
        bOk &= TestTrue(TEXT("Pointerless shield uses canonical weapon kind"),
            Receptacle->ContainsItemType(EGridItemType::Shield));
        bOk &= TestFalse(TEXT("Shield is not weapon for link conditions"),
            Receptacle->ContainsItemType(EGridItemType::Weapon));

        Contain(Dagger, 1, true);
        bOk &= TestTrue(TEXT("A visual actor does not change the canonical category"),
            Receptacle->ContainsItemType(EGridItemType::Weapon));
        bOk &= TestFalse(TEXT("An attached visual cannot authorize a potion condition"),
            Receptacle->ContainsItemType(EGridItemType::Potion));
        Contain(Dagger, 0, true);
        bOk &= TestFalse(TEXT("Registered ARM N0 cannot borrow old category"),
            Receptacle->ContainsItemType(EGridItemType::Potion));
        bOk &= TestFalse(TEXT("Registered ARM N0 cannot pass canonical category"),
            Receptacle->ContainsItemType(EGridItemType::Weapon));
        Contain(Dagger, 21, true);
        bOk &= TestFalse(TEXT("Registered ARM N21 fails closed"),
            Receptacle->ContainsItemType(EGridItemType::Potion));

        Contain(Key, 0, true);
        bOk &= TestTrue(TEXT("Registered non-ARM has canonical puzzle category"),
            Receptacle->ContainsItemType(EGridItemType::Key));
        Contain(Key, 1, true);
        bOk &= TestFalse(TEXT("Invalid non-ARM N1 never falls back"),
            Receptacle->ContainsItemType(EGridItemType::Key));
        Contain(Key, 0, false);
        bOk &= TestTrue(TEXT("Pointerless non-ARM uses authored puzzle category"),
            Receptacle->ContainsItemType(EGridItemType::Key));
        Contain(Key, 0, true);
        bOk &= TestTrue(TEXT("An attached visual preserves the key category"),
            Receptacle->ContainsItemType(EGridItemType::Key));
        bOk &= TestFalse(TEXT("A key visual cannot satisfy a gem condition"),
            Receptacle->ContainsItemType(EGridItemType::Gem));
        Receptacle->ContainedItems[0].Quantity = 0;
        bOk &= TestFalse(TEXT("Empty physical quantity is refused"),
            Receptacle->ContainsItemType(EGridItemType::Key));
        Receptacle->ContainedItems[0].Quantity = MAX_int32;
        bOk &= TestFalse(TEXT("Over-capacity physical quantity is refused"),
            Receptacle->ContainsItemType(EGridItemType::Key));
        Receptacle->ContainedItems[0].Quantity = 1;
        Receptacle->ContainedItems[0].RuntimeObjectId.Invalidate();
        bOk &= TestFalse(TEXT("Invalid physical GUID is refused"),
            Receptacle->ContainsItemType(EGridItemType::Key));

        const TPair<FName, EGridItemType> Cases[] = {
            {TEXT("Stone"), EGridItemType::Misc},
            {TEXT("Shuriken"), EGridItemType::Weapon},
            {TEXT("GoblinKnife"), EGridItemType::Weapon},
            {TEXT("Item_Arrows_01"), EGridItemType::Misc},
            {TEXT("EmptyVial"), EGridItemType::Component},
            {TEXT("Item_Torch"), EGridItemType::Torch},
            {TEXT("Gem_Blue"), EGridItemType::Gem},
            {TEXT("Item_Potion_Healing_01"), EGridItemType::Potion},
            {TEXT("Item_TestNote"), EGridItemType::Scroll},
            {TEXT("Item_RatMeat"), EGridItemType::Food}
        };
        for (const auto& Row : Cases)
        {
            Contain(Row.Key, 0, false);
            bOk &= TestTrue(FString::Printf(TEXT("Native pointerless category: %s"), *Row.Key.ToString()),
                Receptacle->ContainsItemType(Row.Value));
            bOk &= TestFalse(TEXT("None never matches"), Receptacle->ContainsItemType(EGridItemType::None));
            bOk &= TestFalse(TEXT("Malformed enum never matches"), Receptacle->ContainsItemType(static_cast<EGridItemType>(255)));
        }

        Contain(TEXT("ARM_CLEAN064X31_UnregisteredGem"), 0, true);
        bOk &= TestFalse(TEXT("Unregistered historic gem cannot authorize a puzzle"),
            Receptacle->ContainsItemType(EGridItemType::Gem));
        bOk &= TestFalse(TEXT("Unregistered gem does not become a key"),
            Receptacle->ContainsItemType(EGridItemType::Key));
    }

    World->SetGameInstance(nullptr);
    GI->Shutdown();
    World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World);
    return bOk && !HasAnyErrors();
}
#endif

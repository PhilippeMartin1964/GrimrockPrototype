#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridCombatHudWidget.h"

namespace ARMEMENT01152
{
    struct FWorldFixture
    {
        UWorld* World = nullptr;

        FWorldFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM01152_Hotbar_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
                Context.SetCurrentWorld(World);
            }
        }

        ~FWorldFixture()
        {
            if (!World) return;
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01152ExplorationHotbar,
    "Grimrock.RPG.ARMEMENT01.15.ExplorationHotbar.PriestAndApothecary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01152ExplorationHotbar::RunTest(const FString&)
{
    URPGRaceAsset* Human = LoadObject<URPGRaceAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
    if (!TestNotNull(TEXT("Production human race"), Human)) return false;

    struct FCase { const TCHAR* ClassId; const TCHAR* KitId; };
    const FCase Cases[] = {
        { TEXT("Priest"), TEXT("Kit_Priest_Restoration") },
        { TEXT("Alchemist"), TEXT("Kit_Alchemist_Apothecary") }
    };

    const FName PotionId(TEXT("Item_Potion_Healing_01"));
    for (const FCase& Case : Cases)
    {
        ARMEMENT01152::FWorldFixture Fixture;
        if (!TestNotNull(TEXT("Transient world available"), Fixture.World)) return false;
        AGrimrockPartyPawn* Pawn = Fixture.World->SpawnActor<AGrimrockPartyPawn>();
        if (!TestNotNull(TEXT("Party pawn available"), Pawn) ||
            !TestNotNull(TEXT("Party inventory available"), Pawn ? Pawn->PartyInventoryComponent.Get() : nullptr))
            return false;

        UGridPartyInventoryComponent* Inventory = Pawn->PartyInventoryComponent.Get();
        Inventory->InitializeDefaultPartyIfNeeded();
        const FString ClassPath = FString::Printf(
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_%s.DA_Class_%s"),
            Case.ClassId, Case.ClassId);
        URPGClassAsset* Class = LoadObject<URPGClassAsset>(nullptr, *ClassPath);
        if (!TestNotNull(TEXT("Production class available"), Class)) return false;

        FRPGCharacterCreationRequest Request;
        Request.DisplayName = FText::FromString(TEXT("Exploration Potion Hotbar"));
        Request.RaceDefinition = Human;
        Request.ClassDefinition = Class;
        Request.CombatActionSourceClassDefinition = Class;
        Request.StartingEquipmentKitId = FName(Case.KitId);
        FText Failure;
        if (!TestTrue(TEXT("Create a real hero and kit"), Inventory->CreateInitialCharacter(Request, Failure)))
        {
            AddError(Failure.ToString());
            return false;
        }

        FGridCharacterInventoryState& Hero = Inventory->PartyInventoryState.ActiveCharacters[0];
        const FGridInventorySlot* Potion = Hero.InventorySlots.FindByPredicate(
            [PotionId](const FGridInventorySlot& Slot)
            {
                return !Slot.IsEmpty() && Slot.Item.ItemDefinitionId == PotionId;
            });
        if (!TestNotNull(TEXT("Approved potion in the bag"), Potion)) return false;
        const FGridItemInstance PotionItem = Potion->Item;
        if (!TestTrue(TEXT("Potion can be assigned to hotbar slot 2"),
            Inventory->SetCharacterCombatHotbarBindingFromItem(
                0, 1, PotionItem, EGridEquipmentSlot::None))) return false;

        UGridCombatHudWidget* Hud = CreateWidget<UGridCombatHudWidget>(
            Fixture.World, UGridCombatHudWidget::StaticClass());
        if (!TestNotNull(TEXT("Combat HUD exists"), Hud)) return false;
        Hud->InitializeCombatHud(Pawn, nullptr);
        Hud->RefreshFromSources();

        if (!TestTrue(TEXT("Persistent hotbar contains slot 2"), Hud->View.Actions.IsValidIndex(1))) return false;
        TestFalse(TEXT("Exploration has no active combat"), Hud->View.bCombatActive);
        TestTrue(TEXT("Self QuickItem resolves in exploration"), Hud->View.Actions[1].bResolved);
        TestEqual(TEXT("Hotbar retains exact potion definition"),
            Hud->View.Actions[1].Action.SourceDefinitionId, PotionId);
        TestEqual(TEXT("Hotbar shows one potion"), Hud->View.Actions[1].Action.CurrentSourceItemQuantity, 1);
        TestFalse(TEXT("Full-health potion is visibly unavailable"), Hud->View.Actions[1].Action.bEnabled);
        TestEqual(TEXT("Full-health reason is NoApplicableEffect"),
            Hud->View.Actions[1].Action.AvailabilityReason,
            EGridCombatActionAvailabilityReason::NoApplicableEffect);

        FGridCombatActionRequestResult FullHealth;
        TestFalse(TEXT("Full-health hotbar click cannot spend potion"), Hud->RequestHotbarSlot(1, FullHealth));
        TestEqual(TEXT("Refused hotbar click preserves quantity"),
            Inventory->CountItemDefinitionInCharacterInventory(0, PotionId), 1);

        FGridInventoryCharacterSummary Summary;
        if (!TestTrue(TEXT("Effective resources available"), Inventory->GetCharacterSummary(0, Summary))) return false;
        const int32 MaxHealth = Summary.DerivedStats.MaxHealth;
        Hero.Resources.CurrentHealth = FMath::Max(1, MaxHealth - 9);
        Hud->RefreshFromSources();
        TestTrue(TEXT("Damaged hero can use hotbar potion out of combat"), Hud->View.Actions[1].Action.bEnabled);
        TestEqual(TEXT("Exploration hotbar has no combat AP cost"),
            Hud->View.Actions[1].Action.CurrentActionPointCost, 0);

        FGridCombatActionRequestResult Used;
        if (!TestTrue(TEXT("Hotbar executes the shared self-QuickItem inventory transaction"),
            Hud->RequestHotbarSlot(1, Used))) return false;
        TestTrue(TEXT("Hotbar accepted result is reported"), Used.bAccepted);
        TestEqual(TEXT("Health restored to effective maximum"), Hero.Resources.CurrentHealth, MaxHealth);
        TestEqual(TEXT("One unit has been consumed"), Inventory->CountItemDefinitionInCharacterInventory(0, PotionId), 0);
        TestTrue(TEXT("Last potion clears only its own hotbar binding"),
            !Hud->View.Actions[1].bHasBinding);

        FGridCombatActionRequestResult Reuse;
        TestFalse(TEXT("A consumed potion cannot be used twice from hotbar"),
            Hud->RequestHotbarSlot(1, Reuse));
        FString OwnershipError;
        TestTrue(TEXT("Item GUID/ownership invariants retained"), Inventory->ValidateInventoryOwnership(OwnershipError));
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

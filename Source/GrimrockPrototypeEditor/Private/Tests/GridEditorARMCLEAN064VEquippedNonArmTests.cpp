#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalLootRegistryTestUtils.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064VEquippedNonArm,
    "Grimrock.RPG.ARM.CLEAN06_4.NativeEquippedNonArm",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064VEquippedNonArm::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/"
                      "DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Native registry"), Root.Get()) ||
        !TestNotNull(TEXT("Single action catalogue"), Root->ActionCatalogue.Get()) ||
        !TestNotNull(TEXT("Single presentation catalogue"), Root->ActionPresentationCatalogue.Get()))
    {
        return false;
    }
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Real registry validates"), Root->Validate(Errors)))
    {
        for (const FString& Error : Errors) { AddError(Error); }
        return false;
    }
    const int32 OriginalCount = Root->ItemDefinitions.Num();
    if (!TestTrue(TEXT("Native source: V baseline or exact X.61 extension"),
        OriginalCount == 32 || GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root.Get())))
    {
        return false;
    }
    const int32 ExpectedEquipped = OriginalCount == 32 ? 0 : 1;
    if (!TestEqual(TEXT("Exactly 3 QuickItems plus equipped detail"),
            Root->ActionCatalogue->DetailedActions.Num(), 3 + ExpectedEquipped + (OriginalCount == 48 ? 10 : 0)) ||
        !TestEqual(TEXT("Original full presentation is authored only after V"),
            Root->ActionPresentationCatalogue->DetailedPresentations.Num(), ExpectedEquipped))
    {
        return false;
    }
    TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(
        NewObject<UGridItemSubsystem>(GameInstance.Get()));
    if (!TestTrue(TEXT("Canonical registry accepted"), Registry->RegisterItemSystem(Root.Get(), Errors)))
    {
        return false;
    }
    int32 Validated = 0;
    for (const FGridCanonicalRuntimeAction& Record : Root->ActionCatalogue->DetailedActions)
    {
        const FGridCombatActionDefinition& Source = Record.Definition;
        if (Source.SourcePolicy != EGridCombatActionSourcePolicy::Equipment) { continue; }
        if (!TestTrue(TEXT("Source equipped action valid"), Source.IsValid()))
        {
            return false;
        }
        int32 Owners = 0;
        for (const UGridCanonicalItemDefinitionAsset* Definition : Root->ItemDefinitions)
        {
            const UGridOtherEquipmentDefinitionAsset* Equipment =
                Cast<UGridOtherEquipmentDefinitionAsset>(Definition);
            if (!IsValid(Equipment)) { continue; }
            if (!Equipment->ActionBindings.ContainsByPredicate(
                [&Source](const FGridItemActionBinding& Link) {
                    return Link.ActionId == Source.ActionId;
                }))
            {
                continue;
            }
            ++Owners;
            if (!TestTrue(TEXT("Non-ARM equipped stack capacity valid"),
                Equipment->MaxStack >= 1))
            {
                return false;
            }
            EGridEquipmentSlot AttackHand = EGridEquipmentSlot::None;
            for (const EGridEquipmentSlot Hand : {
                EGridEquipmentSlot::MainHand, EGridEquipmentSlot::OffHand })
            {
                if (Equipment->EquipmentSlots.Contains(Hand))
                {
                    AttackHand = Hand;
                    break;
                }
            }
            if (!TestTrue(TEXT("Only a physical authored equipment hand may attack"),
                AttackHand != EGridEquipmentSlot::None))
            {
                return false;
            }
            FGridItemInstance Item;
            Item.RuntimeObjectId = FGuid::NewGuid();
            Item.ItemDefinitionId = Equipment->ItemId;
            Item.ItemLevel = 0;
            Item.Quantity = 1;
            Item.EquipmentSlot = AttackHand;
            FGridCombatActionDefinition Actual;
            if (!TestTrue(TEXT("Native N0 detailed equipped action resolves"),
                    Registry->ResolveDetailedNonArmEquippedAction(
                        Item, AttackHand, Source.ActionId, Actual)) ||
                !TestTrue(TEXT("No PA/target/effect/cooldown loss"),
                    FGridCombatActionDefinition::StaticStruct()->
                        CompareScriptStruct(&Source, &Actual, 0)))
            {
                return false;
            }
            Item.ItemLevel = 21;
            TestFalse(TEXT("N21 equipped action rejected"),
                Registry->ResolveDetailedNonArmEquippedAction(
                    Item, AttackHand, Source.ActionId, Actual));
            Item.ItemLevel = 0;
            const EGridEquipmentSlot WrongHand =
                AttackHand == EGridEquipmentSlot::MainHand
                    ? EGridEquipmentSlot::OffHand
                    : EGridEquipmentSlot::MainHand;
            TestFalse(TEXT("A virtual or wrong physical hand never borrows the action"),
                Registry->ResolveDetailedNonArmEquippedAction(
                    Item, WrongHand, Source.ActionId, Actual));
            TestFalse(TEXT("Unknown action never borrows an equipped binding"),
                Registry->ResolveDetailedNonArmEquippedAction(
                    Item, AttackHand, FName(TEXT("UnknownActionId")), Actual));
            FGridPlayerAttackPresentationProfile AuthoritativePresentation;
            if (!TestTrue(TEXT("Native detailed equipped presentation resolves"),
                    Registry->ResolveDetailedNonArmEquippedPresentation(
                        Equipment->ItemId, AttackHand, AuthoritativePresentation)))
            {
                return false;
            }
            const FGridItemActionBinding* Link =
                Equipment->ActionBindings.FindByPredicate(
                    [&Source](const FGridItemActionBinding& Binding) {
                        return Binding.ActionId == Source.ActionId;
                    });
            const FGridPlayerAttackPresentationProfile* SavedPresentation = Link ?
                Root->ActionPresentationCatalogue->FindDetailed(Link->PresentationId) : nullptr;
            if (!TestNotNull(TEXT("Full presentation is persisted globally"),
                    SavedPresentation) ||
                !TestTrue(TEXT("No authored presentation VFX/audio/motion lost"),
                    FGridPlayerAttackPresentationProfile::StaticStruct()->
                        CompareScriptStruct(&AuthoritativePresentation,
                            SavedPresentation, 0)))
            {
                return false;
            }
            ++Validated;
        }
        if (!TestEqual(TEXT("Only one concrete owner for the equipped action"), Owners, 1))
        {
            return false;
        }
    }
    return TestEqual(TEXT("Native equipped action count"), Validated, ExpectedEquipped);
}
#endif

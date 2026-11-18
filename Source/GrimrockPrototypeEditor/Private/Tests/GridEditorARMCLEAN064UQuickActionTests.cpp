#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalLootRegistryTestUtils.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064UQuickActionMigration,
    "Grimrock.RPG.ARM.CLEAN06_4.RealDetailedNonArmQuickItems",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064UQuickActionMigration::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/"
                      "DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Native canonical registry loaded"), Root.Get()) ||
        !TestNotNull(TEXT("Single action catalogue loaded"), Root->ActionCatalogue.Get()))
    {
        return false;
    }
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Persisted registry remains valid"), Root->Validate(Errors)))
    {
        for (const FString& Error : Errors) { AddError(Error); }
        return false;
    }
    const int32 DetailedCount = Root->ActionCatalogue->DetailedActions.Num();
    // 06.4V adds one separately checked non-ARM equipped action to the
    // SAME catalogue. This test retains exactly its three original QuickItems.
    if (!TestTrue(TEXT("Three original QuickItems, plus optional equipped action"),
        DetailedCount == 3 || DetailedCount == 4 || (DetailedCount == 14 &&
            Root->ItemDefinitions.Num() == 48 && GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root.Get()))))
    {
        return false;
    }
    TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Resolver(
        NewObject<UGridItemSubsystem>(GameInstance.Get()));
    Errors.Reset();
    if (!TestTrue(TEXT("One canonical root registered"), Resolver->RegisterItemSystem(Root.Get(), Errors)))
    {
        return false;
    }
    int32 Checked = 0;
    for (const FGridCanonicalRuntimeAction& Entry : Root->ActionCatalogue->DetailedActions)
    {
        const FGridCombatActionDefinition& Saved = Entry.Definition;
        if (Saved.SourcePolicy == EGridCombatActionSourcePolicy::Equipment)
        {
            continue; // Separate V test checks its exact equipped owner/hand.
        }
        if (!TestTrue(TEXT("Original QuickItem action valid"), Saved.IsValid()) ||
            !TestEqual(TEXT("QuickItem source policy retained"), Saved.SourcePolicy,
                EGridCombatActionSourcePolicy::QuickItem))
        {
            return false;
        }
        int32 Owners = 0;
        for (const UGridCanonicalItemDefinitionAsset* Concrete : Root->ItemDefinitions)
        {
            const UGridNonArmamentDefinitionAsset* Item =
                Cast<UGridNonArmamentDefinitionAsset>(Concrete);
            if (!IsValid(Item)) { continue; }
            const UGridConsumableDefinitionAsset* Consumable =
                Cast<UGridConsumableDefinitionAsset>(Item);
            const TArray<FGridItemActionBinding>& Bindings = Consumable
                ? Consumable->UseActions : Item->ActionBindings;
            if (Bindings.ContainsByPredicate([&Saved](const FGridItemActionBinding& Link)
                { return Link.ActionId == Saved.ActionId; }))
            {
                ++Owners;
                FGridCombatActionDefinition Actual;
                if (!TestTrue(TEXT("Exact N0 canonical full action resolves"),
                        Resolver->ResolveDetailedNonArmAction(
                            Item->ItemId, 0, Saved.ActionId, Actual)) ||
                    !TestTrue(TEXT("Full authored profile has no truncated mechanics"),
                        FGridCombatActionDefinition::StaticStruct()->CompareScriptStruct(
                            &Saved, &Actual, 0)))
                {
                    return false;
                }
                FGridCombatActionDefinition Invalid;
                TestFalse(TEXT("Invalid level must be rejected"),
                    Resolver->ResolveDetailedNonArmAction(
                        Item->ItemId, 21, Saved.ActionId, Invalid));
                TestFalse(TEXT("Unbound identity must never borrow this action"),
                    Resolver->ResolveDetailedNonArmAction(
                        NAME_None, 0, Saved.ActionId, Invalid));
                ++Checked;
            }
        }
        if (!TestEqual(TEXT("Each detailed ActionId has one concrete owner"), Owners, 1))
        {
            return false;
        }
    }
    return TestEqual(TEXT("Native action owners mapped after authoring"),
        Checked, DetailedCount == 14 ? 13 : 3);
}

#endif

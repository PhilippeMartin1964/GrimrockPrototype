#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../ARMEMENT/GridEditorRetirementAudit.h"
#include "Misc/PackageName.h"

#include "RPG/RPGAlchemistAuthoring.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Tests/GridCanonicalRegistryFixture.h"

namespace UIRPGCONTENTTEXT021
{
	void CollectProductionItemIds(TArray<FName>& OutItemIds)
	{
		TArray<FName> B1ItemIds;
		TArray<FName> B2ItemIds;
		FRPGAlchemistAuthoring::GetB1ItemIds(B1ItemIds);
		FRPGAlchemistAuthoring::GetB2ItemIds(B2ItemIds);

		OutItemIds = MoveTemp(B1ItemIds);
		OutItemIds.Append(B2ItemIds);
	}

    bool IsNativeAlchemyTextComplete()
    {
        FGridCanonicalRegistryFixture Fixture;
        if (!Fixture.bReady) return false;
        TArray<FName> Ids;
        CollectProductionItemIds(Ids);
        if (Ids.Num() != 11) return false;
        TSet<FName> Unique;
        for (const FName Id : Ids)
        {
            if (Id.IsNone() || Unique.Contains(Id)) return false;
            Unique.Add(Id);
            FGridResolvedItem Item;
            if (!Fixture.Resolve(Id, 0, Item) || Item.bIsArmament ||
                Item.DisplayName.IsEmpty() ||
                Item.DisplayName.ToString().StartsWith(TEXT("AUTHOR_TEXT_REQUIRED:")) ||
                Item.Description.ToString().StartsWith(TEXT("AUTHOR_TEXT_REQUIRED:")) ||
                Item.AvailableActions.Num() != 1)
                return false;
            FGridCombatActionDefinition Action;
            if (!Fixture.Registry->ResolveDetailedNonArmAction(
                    Id, 0, Item.AvailableActions[0].ActionId, Action) ||
                Action.ActionId != Item.AvailableActions[0].ActionId ||
                Action.DisplayName.ToString().StartsWith(TEXT("AUTHOR_TEXT_REQUIRED:")) ||
                Action.Description.ToString().StartsWith(TEXT("AUTHOR_TEXT_REQUIRED:")))
                return false;
        }
        return Unique.Num() == 11;
    }

}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPGCONTENTTEXT021ItemTextOwnershipAuditTest,
    "Grimrock.UI.RPG.CONTENTTEXT02.ItemOwnershipAudit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT021ItemTextOwnershipAuditTest::RunTest(const FString&)
{
    using namespace UIRPGCONTENTTEXT021;
    TArray<FName> Ids;
    CollectProductionItemIds(Ids);
    TestEqual(TEXT("Eleven historical alchemy identities audited"), Ids.Num(), 11);
    for (FName Id : Ids)
    {
        const FString Package = FPackageName::ObjectPathToPackageName(FRPGAlchemistAuthoring::GetItemObjectPath(Id));
        TestTrue(TEXT("Exact alchemy source belongs to the retirement receipt"),
            GridEditorRetirementAudit::IsRetiredItemPackage(FName(*Package)));
        TestFalse(TEXT("Retired alchemy source package has not reappeared"),
            FPackageName::DoesPackageExist(Package));
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPGCONTENTTEXT022ItemTextPreservationTest,
    "Grimrock.UI.RPG.CONTENTTEXT02.Preservation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT022ItemTextPreservationTest::RunTest(const FString&)
{
    using namespace UIRPGCONTENTTEXT021;
    FGridCanonicalRegistryFixture Fixture;
    const auto* Root = GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Configured item system"), Root) || !Fixture.bReady) return false;
    TArray<FName> Ids;
    CollectProductionItemIds(Ids);
    for (FName Id : Ids)
    {
        const auto* Asset = Root->ItemDefinitions.FindByPredicate(
            [Id](const auto& Definition) { return Definition && Definition->ItemId == Id; });
        if (!TestNotNull(TEXT("Canonical item owns its text"), Asset)) continue;
        const bool bWasDirty = (*Asset)->GetOutermost()->IsDirty();
        FGridResolvedItem Native;
        if (!TestTrue(TEXT("Read-only item resolves"), Fixture.Resolve(Id, 0, Native))) continue;
        TestTrue(TEXT("Authored name is preserved"), Native.DisplayName.EqualTo((*Asset)->DisplayName));
        TestTrue(TEXT("Authored description is preserved"), Native.Description.EqualTo((*Asset)->Description));
        TestEqual(TEXT("Read model leaves the source package unchanged"), (*Asset)->GetOutermost()->IsDirty(), bWasDirty);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPGCONTENTTEXT023BootstrapOnlyTest,
    "Grimrock.UI.RPG.CONTENTTEXT02.BootstrapOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT023BootstrapOnlyTest::RunTest(const FString&)
{
	for (const EGridCombatSurfaceType Output : {
		EGridCombatSurfaceType::Fire,
		EGridCombatSurfaceType::Ice,
		EGridCombatSurfaceType::Poison,
		EGridCombatSurfaceType::Oil })
	{
		FGridCombatActionDefinition Action;
		if (!TestTrue(
				TEXT("Major Transmutation recipe action builds with technical presentation"),
				FRPGAlchemistAuthoring::BuildMajorTransmutationRecipeAction(Output, Action)))
		{
			continue;
		}

		const FString StableActionId = Action.ActionId.ToString();
		TestEqual(
			TEXT("Major Transmutation bootstrap name is its stable ActionId"),
			Action.DisplayName.ToString(),
			StableActionId);
		TestEqual(
			TEXT("Major Transmutation bootstrap description is technical only"),
			Action.Description.ToString(),
			FString::Printf(TEXT("AUTHOR_TEXT_REQUIRED:%s"), *StableActionId));
	}

    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPGCONTENTTEXT024FinalItemOwnershipAuditTest,
    "Grimrock.UI.RPG.CONTENTTEXT02.FinalAudit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT024FinalItemOwnershipAuditTest::RunTest(const FString&)
{
    using namespace UIRPGCONTENTTEXT021;
    TestTrue(TEXT("Production alchemy resolves authored non-placeholder item and action text"), IsNativeAlchemyTextComplete());
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPGCONTENTTEXT025CanonicalTextRetirementParityTest,
    "Grimrock.UI.RPG.CONTENTTEXT02.CanonicalTextRetirementParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT025CanonicalTextRetirementParityTest::RunTest(const FString&)
{
    using namespace UIRPGCONTENTTEXT021;
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Configured native ItemSystem is registered"), Fixture.bReady)) return false;
    TArray<FName> Ids;
    CollectProductionItemIds(Ids);
    TSet<FName> ActionIds;
    for (FName Id : Ids)
    {
        FGridResolvedItem Native;
        if (!TestTrue(TEXT("Canonical N0 Alchemist item resolves"), Fixture.Resolve(Id, 0, Native)) ||
            !TestEqual(TEXT("Exactly one authored action is bound"), Native.AvailableActions.Num(), 1)) continue;
        const FName ActionId = Native.AvailableActions[0].ActionId;
        TestFalse(TEXT("Another alchemy item cannot own the same action"), ActionIds.Contains(ActionId));
        ActionIds.Add(ActionId);
        FGridCombatActionDefinition Action;
        if (!TestTrue(TEXT("The action resolves from its actual owner"),
            Fixture.Registry->ResolveDetailedNonArmAction(Id, 0, ActionId, Action))) continue;
        TestEqual(TEXT("Detailed action keeps the bound identity"), Action.ActionId, ActionId);
        TestEqual(TEXT("Detailed action is owned by QuickItem"), Action.SourcePolicy, EGridCombatActionSourcePolicy::QuickItem);
        TestEqual(TEXT("One source item consumed"), Action.ResourceCosts.SourceItemQuantityCost, 1);
    }
    TestEqual(TEXT("Eleven unique authored actions"), ActionIds.Num(), 11);
    return !HasAnyErrors();
}

#endif

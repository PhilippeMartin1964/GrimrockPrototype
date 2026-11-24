#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../UI/GridInventorySortReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063OSortProjection,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalInventorySortProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063OSortProjection::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Persisted ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Real registry"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	const FName IDs[] = {TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
		TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01")};
	const int32 Levels[] = {1, 20};
	for (const FName Id : IDs)
	{
		for (const int32 Level : Levels)
		{
			FGridResolvedItem Definition;
			if (!TestTrue(TEXT("Real N1/N20 item resolves"),
				Registry->ResolveItem(Id, Level, Definition))) return false;
			FGridItemInstance Item;
			Item.RuntimeObjectId = FGuid::NewGuid();
			Item.ItemDefinitionId = Id;
			Item.ItemLevel = Level;
			Item.Quantity = 2;
			FGridInventorySortReadView View =
				FGridInventorySortReadModel::Resolve(Registry.Get(), Item);
			TestEqual(TEXT("Canonical display name drives alphabetical sorting"),
				View.DisplayName, Definition.DisplayName.IsEmpty()
					? Id.ToString() : Definition.DisplayName.ToString());
			TestTrue(TEXT("Canonical 2x unit weight drives weight sorting"),
				FMath::IsNearlyEqual(View.TotalWeight, Definition.WeightKg * 2.0f));
			Item.ItemLevel = 0;
			View = FGridInventorySortReadModel::Resolve(Registry.Get(), Item);
			TestEqual(TEXT("Known invalid canonical has no legacy mass"), View.TotalWeight, 0.0f);
			TestEqual(TEXT("Known invalid canonical does not leak old name"), View.DisplayName, Id.ToString());
		}
	}
	FGridItemInstance Legacy;
	Legacy.RuntimeObjectId = FGuid::NewGuid();
	Legacy.ItemDefinitionId = TEXT("LegacySort_063O");
	Legacy.Quantity = 3;
	Legacy.ItemLevel = 0;
	const FGridInventorySortReadView Old =
		FGridInventorySortReadModel::Resolve(Registry.Get(), Legacy);
	TestEqual(TEXT("Unregistered sort name is technical identity"), Old.DisplayName, Legacy.ItemDefinitionId.ToString());
	TestEqual(TEXT("Unregistered sort cannot borrow old mass"), Old.TotalWeight, 0.0f);
	const auto NoDefinition = FGridInventorySortReadModel::Resolve(nullptr, Legacy);
	TestEqual(TEXT("No source definition sorts by technical identity"),
		NoDefinition.DisplayName, Legacy.ItemDefinitionId.ToString());
	TestEqual(TEXT("No source definition contributes no artificial weight"),
		NoDefinition.TotalWeight, 0.0f);
	return true;
}

#endif

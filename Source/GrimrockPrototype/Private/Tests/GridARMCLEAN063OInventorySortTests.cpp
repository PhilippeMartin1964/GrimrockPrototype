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
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Persisted ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Real registry"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	TStrongObjectPtr<UGridItemDefinitionAsset> ConflictingLegacy(NewObject<UGridItemDefinitionAsset>());
	ConflictingLegacy->DisplayName = FText::FromString(TEXT("Old wrong weapon"));
	ConflictingLegacy->Weight = 9000.0f;
	const FName IDs[] = {TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
		TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01")};
	const int32 Levels[] = {1, 20};
	for (const FName Id : IDs)
	{
		ConflictingLegacy->ItemDefinitionId = Id;
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
			Item.Weight = 999.0f;
			Item.DisplayName = FText::FromString(TEXT("Old instance name"));
			FGridInventorySortReadView View =
				FGridInventorySortReadModel::Resolve(Registry.Get(), ConflictingLegacy.Get(), Item);
			TestEqual(TEXT("Canonical display name drives alphabetical sorting"),
				View.DisplayName, Definition.DisplayName.IsEmpty()
					? Id.ToString() : Definition.DisplayName.ToString());
			TestTrue(TEXT("Canonical 2x unit weight drives weight sorting"),
				FMath::IsNearlyEqual(View.TotalWeight, Definition.WeightKg * 2.0f));
			Item.ItemLevel = 0;
			View = FGridInventorySortReadModel::Resolve(Registry.Get(), ConflictingLegacy.Get(), Item);
			TestEqual(TEXT("Known invalid canonical has no legacy mass"), View.TotalWeight, 0.0f);
			TestEqual(TEXT("Known invalid canonical does not leak old name"), View.DisplayName, Id.ToString());
		}
	}
	FGridItemInstance Legacy;
	Legacy.RuntimeObjectId = FGuid::NewGuid();
	Legacy.ItemDefinitionId = TEXT("LegacySort_063O");
	Legacy.Quantity = 3;
	Legacy.ItemLevel = 0;
	Legacy.Weight = 2.0f;
	ConflictingLegacy->ItemDefinitionId = Legacy.ItemDefinitionId;
	ConflictingLegacy->Weight = 1.5f;
	const FGridInventorySortReadView Old =
		FGridInventorySortReadModel::Resolve(Registry.Get(), ConflictingLegacy.Get(), Legacy);
	TestEqual(TEXT("Unregistered name keeps old definition"),
		Old.DisplayName, FString(TEXT("Old wrong weapon")));
	TestEqual(TEXT("Unregistered weight keeps old definition"),
		Old.TotalWeight, 4.5f);
	return true;
}

#endif

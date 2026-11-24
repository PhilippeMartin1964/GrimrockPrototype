#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../UI/GridInventoryItemMetadataReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063KInventoryMetadata,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalInventoryMetadata",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063KInventoryMetadata::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Persisted ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Real registry accepted"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}

	const FName ItemIds[] =
	{
		TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
		TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01")
	};
	const int32 Levels[] = {1, 20};
	for (const FName ItemId : ItemIds)
	{
		for (const int32 Level : Levels)
		{
			FGridResolvedItem Expected;
			if (!TestTrue(TEXT("Canonical ARM metadata resolves"),
				Registry->ResolveItem(ItemId, Level, Expected))) return false;
			FGridItemInstance Instance;
			Instance.RuntimeObjectId = FGuid::NewGuid();
			Instance.ItemDefinitionId = ItemId;
			Instance.ItemLevel = Level;
			Instance.Quantity = 1;
			const FGridInventoryItemMetadata Metadata =
				FGridInventoryItemMetadataReadModel::Resolve(Registry.Get(), Instance);
			TestTrue(TEXT("Registered ARM metadata is canonical"), Metadata.bRegisteredIdentity);
			TestTrue(TEXT("Authored N1/N20 identity resolves"), Metadata.bResolved);
			TestEqual(TEXT("DisplayName comes from item definition"), Metadata.DisplayName.ToString(),
				Expected.DisplayName.ToString());
			TestEqual(TEXT("Description comes from item definition"), Metadata.Description.ToString(),
				Expected.Description.ToString());
			TestTrue(TEXT("WeightKg ignores contradictory old definition"),
				FMath::IsNearlyEqual(Metadata.UnitWeightKg, Expected.WeightKg));

			Instance.ItemLevel = 0;
			const FGridInventoryItemMetadata Invalid =
				FGridInventoryItemMetadataReadModel::Resolve(Registry.Get(), Instance);
			TestTrue(TEXT("Known canonical identity stays classified canonical"), Invalid.bRegisteredIdentity);
			TestFalse(TEXT("Level zero does not silently rehydrate legacy fields"), Invalid.bResolved);
			TestEqual(TEXT("Invalid canonical weight never falls back"), Invalid.UnitWeightKg, 0.0f);
			TestTrue(TEXT("Invalid canonical description never copies old text"), Invalid.Description.IsEmpty());
			TestEqual(TEXT("Invalid canonical identity displays technical ItemId"),
				Invalid.DisplayName.ToString(), ItemId.ToString());
		}
	}

	FGridItemInstance Outside;
	Outside.ItemDefinitionId = TEXT("LegacyNonCanonical_063K");
	Outside.RuntimeObjectId = FGuid::NewGuid();
	Outside.Quantity = 1;
	const FGridInventoryItemMetadata Old =
		FGridInventoryItemMetadataReadModel::Resolve(Registry.Get(), Outside);
	TestFalse(TEXT("Unregistered source is not resolved"), Old.bRegisteredIdentity || Old.bResolved);
	TestEqual(TEXT("Unknown source shows only technical identity"), Old.DisplayName.ToString(), Outside.ItemDefinitionId.ToString());
	TestEqual(TEXT("Unknown source has no mass projection"), Old.UnitWeightKg, 0.0f);
	TestTrue(TEXT("Unknown source has no old description"), Old.Description.IsEmpty());
	const auto NoDefinition = FGridInventoryItemMetadataReadModel::Resolve(nullptr, Outside);
	TestFalse(TEXT("Fixture without definition cannot project static metadata"),
		NoDefinition.bResolved);
	TestEqual(TEXT("Definition-less fixture exposes only technical name"),
		NoDefinition.DisplayName.ToString(), Outside.ItemDefinitionId.ToString());
	TestTrue(TEXT("Definition-less fixture has no description"), NoDefinition.Description.IsEmpty());
	TestEqual(TEXT("Definition-less fixture has zero mass"), NoDefinition.UnitWeightKg, 0.0f);
	Outside.ItemDefinitionId = TEXT("Item_Arrows_01");
	Outside.ItemLevel = 1;
	TestFalse(TEXT("Non-ARM N1 refused"), FGridInventoryItemMetadataReadModel::Resolve(Registry.Get(), Outside).bResolved);
	TestTrue(TEXT("Invalid instance returns empty metadata"),
		!FGridInventoryItemMetadataReadModel::Resolve(Registry.Get(), FGridItemInstance()).bResolved);
	return true;
}

#endif

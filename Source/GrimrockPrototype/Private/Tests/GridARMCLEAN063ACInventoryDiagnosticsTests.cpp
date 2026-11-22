#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../Runtime/GridInventoryDiagnosticWeightReadModel.h"
#include "../Runtime/GridInventoryDefinitionDiagnosticReadModel.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063ACInventoryDiagnosticWeight,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalInventoryDiagnosticWeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063ACInventoryDiagnosticWeight::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real canonical ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register real ItemSystem"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	TArray<FGridInventoryDefinitionDiagnosticRow> Rows;
	if (!TestTrue(TEXT("Canonical definitions available without inventory cache"),
		FGridInventoryDefinitionDiagnosticReadModel::Build(Registry.Get(), Rows))) return false;
	TestEqual(TEXT("Every registered identity is present"), Rows.Num(), Root->ItemDefinitions.Num());
	TSet<FName> Seen;
	for (const auto& Row : Rows)
	{
		TestFalse(TEXT("Canonical diagnostics never duplicate identities"), Seen.Contains(Row.ItemId));
		Seen.Add(Row.ItemId);
		TestTrue(TEXT("Real canonical row resolves"), Row.bResolved);
		TestTrue(TEXT("Weight reports actual level endpoints"), Row.WeightText.StartsWith(Row.bArmament ? TEXT("N1:") : TEXT("N0:")));
		if (Row.bAmmo) TestTrue(TEXT("Ammunition has no equipment slots"), Row.Slots.IsEmpty());
	}
	const auto* Shield = Rows.FindByPredicate([](const auto& Row) { return Row.ItemId == FName(TEXT("Weapon_RoundShield_01")); });
	if (TestNotNull(TEXT("Canonical shield row"), Shield))
	{
		TestTrue(TEXT("Shield allowed in OffHand"), Shield->Slots.Contains(EGridEquipmentSlot::OffHand));
		TestFalse(TEXT("Shield forbidden in MainHand"), Shield->Slots.Contains(EGridEquipmentSlot::MainHand));
	}
	TestFalse(TEXT("Absent registry is explicit"), FGridInventoryDefinitionDiagnosticReadModel::Build(nullptr, Rows));
	TestTrue(TEXT("Absent registry clears rows"), Rows.IsEmpty());
	TStrongObjectPtr<UGridItemDefinitionAsset> Old(NewObject<UGridItemDefinitionAsset>());
	Old->Weight = 99999.0f;
	const FName Arms[] = {
		TEXT("Weapon_MainGauche_01"), TEXT("Weapon_StraightSword_01"),
		TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01")
	};
	const int32 Levels[] = {1, 20};
	for (const FName ItemId : Arms)
	{
		Old->ItemDefinitionId = ItemId;
		FGridResolvedItem First, Last;
		if (!TestTrue(TEXT("Canonical N1 ARM resolves"), Registry->ResolveItem(ItemId, 1, First)) ||
			!TestTrue(TEXT("Canonical N20 ARM resolves"), Registry->ResolveItem(ItemId, 20, Last)))
		{
			return false;
		}
		const FString ExpectedDefinition = FString::Printf(
			TEXT("N1:%.1f,N20:%.1f"), First.WeightKg, Last.WeightKg);
		TestEqual(TEXT("Definition shows explicit level endpoints without old weight"),
			FGridInventoryDiagnosticWeightReadModel::FormatDefinitionWeight(Registry.Get(), Old.Get()),
			ExpectedDefinition);
		for (const int32 Level : Levels)
		{
			FGridResolvedItem Canonical;
			if (!TestTrue(TEXT("Canonical item at instance level resolves"),
				Registry->ResolveItem(ItemId, Level, Canonical))) return false;
			FGridItemInstance Instance;
			Instance.RuntimeObjectId = FGuid::NewGuid();
			Instance.ItemDefinitionId = ItemId;
			Instance.ItemLevel = Level;
			Instance.Quantity = 3;
			TestEqual(TEXT("Slot diagnostics use canonical unit weight, not stacked weight"),
				FGridInventoryDiagnosticWeightReadModel::FormatInstanceUnitWeight(Registry.Get(), Instance),
				FString::Printf(TEXT("%.1f"), Canonical.WeightKg));
			Instance.ItemLevel = 0;
			TestEqual(TEXT("Known invalid ARM N0 is reported invalid"),
				FGridInventoryDiagnosticWeightReadModel::FormatInstanceUnitWeight(Registry.Get(), Instance),
				FString(TEXT("INVALID")));
		}
	}
	Old->ItemDefinitionId = TEXT("Item_Arrows_01");
	FGridResolvedItem Arrows;
	if (!TestTrue(TEXT("Production unlevelled arrows resolve at N0"),
		Registry->ResolveItem(Old->ItemDefinitionId, 0, Arrows))) return false;
	TestEqual(TEXT("Unlevelled definition explicitly reports N0"),
		FGridInventoryDiagnosticWeightReadModel::FormatDefinitionWeight(Registry.Get(), Old.Get()),
		FString::Printf(TEXT("N0:%.1f"), Arrows.WeightKg));
	FGridItemInstance Arrow;
	Arrow.RuntimeObjectId = FGuid::NewGuid();
	Arrow.ItemDefinitionId = Old->ItemDefinitionId;
	Arrow.ItemLevel = 0;
	Arrow.Quantity = 12;
	TestEqual(TEXT("Unlevelled ammo slot uses canonical unit weight"),
		FGridInventoryDiagnosticWeightReadModel::FormatInstanceUnitWeight(Registry.Get(), Arrow),
		FString::Printf(TEXT("%.1f"), Arrows.WeightKg));

	Old->ItemDefinitionId = TEXT("UnregisteredDiagnostic063AC");
	FGridItemInstance Unknown;
	Unknown.RuntimeObjectId = FGuid::NewGuid();
	Unknown.ItemDefinitionId = Old->ItemDefinitionId;
	Unknown.Quantity = 2;
	TestEqual(TEXT("Configured registry rejects copied weight of unknown instance"),
		FGridInventoryDiagnosticWeightReadModel::FormatInstanceUnitWeight(Registry.Get(), Unknown),
		FString(TEXT("INVALID")));
	TestEqual(TEXT("Configured registry rejects copied weight of unknown definition"),
		FGridInventoryDiagnosticWeightReadModel::FormatDefinitionWeight(Registry.Get(), Old.Get()),
		FString(TEXT("INVALID")));
	TestEqual(TEXT("Registry-less fixture rejects unit mass with no canonical authority"),
		FGridInventoryDiagnosticWeightReadModel::FormatInstanceUnitWeight(nullptr, Unknown),
		FString(TEXT("INVALID")));
	TestEqual(TEXT("Isolated registry-less fixture retains numeric definition diagnostic"),
		FGridInventoryDiagnosticWeightReadModel::FormatDefinitionWeight(nullptr, Old.Get()),
		FString(TEXT("99999.0")));
	return true;
}

#endif

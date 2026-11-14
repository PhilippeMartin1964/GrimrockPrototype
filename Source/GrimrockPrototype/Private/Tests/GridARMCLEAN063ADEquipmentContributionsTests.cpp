#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../Runtime/GridEquipmentContributionReadModel.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063ADEquipmentContributions,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalEquipmentDiagnosticContributions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063ADEquipmentContributions::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real persisted ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register canonical ItemSystem"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}

	TStrongObjectPtr<UGridItemDefinitionAsset> Old(NewObject<UGridItemDefinitionAsset>());
	Old->EquipmentStatBonus.StrengthBonus = 3; // Still authored outside canonical weapon schema.
	Old->EquipmentStatBonus.CarryWeightBonus = 2.5f;
	Old->EquipmentStatBonus.ArmorBonus = 999;
	Old->EquipmentStatBonus.MagicalArmorBonus = 999;
	Old->EquipmentResistanceBonus.FireResistance = 12;
	Old->EquipmentResistanceBonus.IceResistance = 7;
	const FName Ids[] = {TEXT("Weapon_MainGauche_01"), TEXT("Weapon_RoundShield_01"),
		TEXT("Weapon_HolyAegis_01"), TEXT("Weapon_StraightSword_01")};
	const int32 Levels[] = {1, 20};
	for (const FName Id : Ids)
	{
		Old->ItemDefinitionId = Id;
		for (const int32 Level : Levels)
		{
			FGridItemInstance Equipped;
			Equipped.ItemDefinitionId = Id;
			Equipped.RuntimeObjectId = FGuid::NewGuid();
			Equipped.ItemLevel = Level;
			Equipped.Quantity = 1;
			Equipped.OwnerType = EGridItemOwnerType::EquipmentSlot;
			Equipped.EquipmentSlot = EGridEquipmentSlot::OffHand;
			FGridResolvedItem Canonical;
			if (!TestTrue(TEXT("Production ARM level resolves"),
				Registry->ResolveItem(Id, Level, Canonical))) return false;

			const FGridEquipmentStatBonus Bonuses =
				FGridEquipmentContributionReadModel::ResolveStatBonuses(Registry.Get(), Old.Get(), Equipped);
			TestEqual(TEXT("Physical armor uses canonical N1-N20"), Bonuses.ArmorBonus,
				Canonical.Weapon.PhysicalArmor);
			TestEqual(TEXT("Magical armor uses canonical N1-N20"), Bonuses.MagicalArmorBonus,
				Canonical.Weapon.MagicalArmor);
			TestEqual(TEXT("Legacy strength bonus remains gameplay authority"), Bonuses.StrengthBonus, 3);
			TestTrue(TEXT("Legacy carry weight bonus remains gameplay authority"),
				FMath::IsNearlyEqual(Bonuses.CarryWeightBonus, 2.5f));

			const FGridDamageResistanceSet Resistances =
				FGridEquipmentContributionReadModel::ResolveResistances(Old.Get(), Equipped);
			TestEqual(TEXT("Fire resistance uses existing gameplay source"), Resistances.FireResistance, 12);
			TestEqual(TEXT("Ice resistance uses existing gameplay source"), Resistances.IceResistance, 7);
			Equipped.ItemLevel = 0;
			const FGridEquipmentStatBonus Invalid =
				FGridEquipmentContributionReadModel::ResolveStatBonuses(Registry.Get(), Old.Get(), Equipped);
			TestFalse(TEXT("Invalid registered ARM N0 cannot inherit old armor"), Invalid.HasAnyBonus());
		}
	}

	// Without an old definition, valid canonical shield armor still resolves.
	FGridItemInstance NoOld;
	NoOld.RuntimeObjectId = FGuid::NewGuid();
	NoOld.ItemDefinitionId = TEXT("Weapon_RoundShield_01");
	NoOld.ItemLevel = 20;
	NoOld.Quantity = 1;
	FGridResolvedItem Shield;
	if (!TestTrue(TEXT("Canonical N20 round shield exists"),
		Registry->ResolveItem(NoOld.ItemDefinitionId, 20, Shield))) return false;
	const FGridEquipmentStatBonus WithoutDefinition =
		FGridEquipmentContributionReadModel::ResolveStatBonuses(Registry.Get(), nullptr, NoOld);
	TestEqual(TEXT("Canonical shield armor survives missing old definition"),
		WithoutDefinition.ArmorBonus, Shield.Weapon.PhysicalArmor);

	// Nonregistered entries and standalone/no-GI tests retain the old behavior.
	Old->ItemDefinitionId = TEXT("LegacyStats_063AD");
	Old->EquipmentStatBonus.ArmorBonus = 7;
	Old->EquipmentStatBonus.MagicalArmorBonus = 4;
	FGridItemInstance Legacy;
	Legacy.RuntimeObjectId = FGuid::NewGuid();
	Legacy.ItemDefinitionId = Old->ItemDefinitionId;
	Legacy.ItemLevel = 0;
	Legacy.Quantity = 1;
	const FGridEquipmentStatBonus LegacyBonuses =
		FGridEquipmentContributionReadModel::ResolveStatBonuses(Registry.Get(), Old.Get(), Legacy);
	TestEqual(TEXT("Legacy armor remains seven"), LegacyBonuses.ArmorBonus, 7);
	TestEqual(TEXT("Legacy magical armor remains four"), LegacyBonuses.MagicalArmorBonus, 4);
	TestEqual(TEXT("Legacy attribute contribution unchanged"), LegacyBonuses.StrengthBonus, 3);
	TestEqual(TEXT("Legacy resistance contribution unchanged"),
		FGridEquipmentContributionReadModel::ResolveResistances(Old.Get(), Legacy).FireResistance, 12);
	TestTrue(TEXT("Missing definition gives no resistance contribution"),
		FGridEquipmentContributionReadModel::ResolveResistances(nullptr, Legacy).IsEmpty());
	return true;
}

#endif

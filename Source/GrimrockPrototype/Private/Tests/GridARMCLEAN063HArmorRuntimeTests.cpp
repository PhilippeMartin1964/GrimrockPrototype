#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../Runtime/GridEquipmentArmorReadModel.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063HArmorRuntime,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalEquipmentArmorReadModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063HArmorRuntime::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Persisted canonical ARM registry"), Root.Get()))
	{
		return false;
	}
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GameInstance.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register real item system once"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) { AddError(Error); }
		return false;
	}

	TStrongObjectPtr<UGridItemDefinitionAsset> ConflictingLegacy(NewObject<UGridItemDefinitionAsset>());
	ConflictingLegacy->ItemDefinitionId = TEXT("Weapon_RoundShield_01");
	ConflictingLegacy->EquipmentStatBonus.ArmorBonus = 999;
	ConflictingLegacy->EquipmentStatBonus.MagicalArmorBonus = 999;
	FGridItemInstance Shield;
	Shield.RuntimeObjectId = FGuid::NewGuid();
	Shield.ItemDefinitionId = ConflictingLegacy->ItemDefinitionId;
	Shield.Quantity = 1;
	Shield.ItemLevel = 20;
	Shield.AbsorbedEquipmentPhysicalArmor = 2;
	Shield.AbsorbedEquipmentMagicalArmor = 1;

	FGridCanonicalArmorReserveView Expected;
	if (!TestTrue(TEXT("Real N20 shield reserve resolves"), Registry->ResolveArmorReserveForInstance(Shield, Expected)))
	{
		return false;
	}
	FGridEquipmentArmorReadView Armor;
	TestTrue(TEXT("Runtime read uses canonical N20 with conflicting legacy source"),
		FGridEquipmentArmorReadModel::TryResolve(Registry.Get(), ConflictingLegacy.Get(), Shield, Armor));
	TestEqual(TEXT("Canonical physical nominal not 999"), Armor.NominalPhysical, Expected.NominalPhysicalArmor);
	TestEqual(TEXT("Canonical magic nominal not 999"), Armor.NominalMagical, Expected.NominalMagicalArmor);
	TestEqual(TEXT("Spent physical reserve follows actual item"), Armor.RemainingPhysical, Expected.RemainingPhysicalArmor);
	TestEqual(TEXT("Spent magical reserve follows actual item"), Armor.RemainingMagical, Expected.RemainingMagicalArmor);
	TestEqual(TEXT("Read does not repair physical debt"), Shield.AbsorbedEquipmentPhysicalArmor, 2);
	TestEqual(TEXT("Read does not repair magical debt"), Shield.AbsorbedEquipmentMagicalArmor, 1);

	// Another real passive armament, at both ends of the level progression,
	// must resolve even when no old item definition is supplied.
	FGridItemInstance Aegis = Shield;
	Aegis.RuntimeObjectId = FGuid::NewGuid();
	Aegis.ItemDefinitionId = TEXT("Weapon_HolyAegis_01");
	const int32 ProductionLevels[] = {1, 20};
	for (const int32 Level : ProductionLevels)
	{
		Aegis.ItemLevel = Level;
		FGridCanonicalArmorReserveView Authoritative;
		if (!TestTrue(TEXT("Real Holy Aegis reserve resolves"),
			Registry->ResolveArmorReserveForInstance(Aegis, Authoritative)))
		{
			return false;
		}
		TestTrue(TEXT("Registered ARM resolves without a legacy DA_Weapon definition"),
			FGridEquipmentArmorReadModel::TryResolve(Registry.Get(), nullptr, Aegis, Armor));
		TestEqual(TEXT("Holy Aegis nominal physical comes from real catalogue"),
			Armor.NominalPhysical, Authoritative.NominalPhysicalArmor);
		TestEqual(TEXT("Holy Aegis nominal magical comes from real catalogue"),
			Armor.NominalMagical, Authoritative.NominalMagicalArmor);
		TestEqual(TEXT("Holy Aegis physical debt is respected"),
			Armor.RemainingPhysical, Authoritative.RemainingPhysicalArmor);
		TestEqual(TEXT("Holy Aegis magical debt is respected"),
			Armor.RemainingMagical, Authoritative.RemainingMagicalArmor);
	}

	Shield.ItemLevel = 0;
	Armor.NominalPhysical = 123;
	TestFalse(TEXT("Canonical level zero cannot fall back to conflicting DA_Weapon"),
		FGridEquipmentArmorReadModel::TryResolve(Registry.Get(), ConflictingLegacy.Get(), Shield, Armor));
	TestEqual(TEXT("Failed canonical read resets output"), Armor.NominalPhysical, 0);
	Shield.ItemLevel = 20;
	Shield.AbsorbedEquipmentPhysicalArmor = Expected.NominalPhysicalArmor + 7;
	TestTrue(TEXT("Spent beyond nominal is conserved in the instance"),
		FGridEquipmentArmorReadModel::TryResolve(Registry.Get(), ConflictingLegacy.Get(), Shield, Armor));
	TestEqual(TEXT("Overspent canonical physical armor remains zero"), Armor.RemainingPhysical, 0);
	TestEqual(TEXT("Overspent debt is not reset"), Shield.AbsorbedEquipmentPhysicalArmor,
		Expected.NominalPhysicalArmor + 7);

	// Unknown IDs retain their existing definition-based reader until migration.
	TStrongObjectPtr<UGridItemDefinitionAsset> Legacy(NewObject<UGridItemDefinitionAsset>());
	Legacy->ItemDefinitionId = TEXT("LegacyArmor_063H");
	Legacy->EquipmentStatBonus.ArmorBonus = 8;
	Legacy->EquipmentStatBonus.MagicalArmorBonus = 6;
	FGridItemInstance LegacyItem;
	LegacyItem.RuntimeObjectId = FGuid::NewGuid();
	LegacyItem.ItemDefinitionId = Legacy->ItemDefinitionId;
	LegacyItem.Quantity = 1;
	LegacyItem.AbsorbedEquipmentPhysicalArmor = 3;
	LegacyItem.AbsorbedEquipmentMagicalArmor = 2;
	TestTrue(TEXT("Non-registered item keeps its prior armor reader"),
		FGridEquipmentArmorReadModel::TryResolve(Registry.Get(), Legacy.Get(), LegacyItem, Armor));
	TestEqual(TEXT("Legacy physical reserve remains five"), Armor.RemainingPhysical, 5);
	TestEqual(TEXT("Legacy magical reserve remains four"), Armor.RemainingMagical, 4);
	TestFalse(TEXT("Unknown item with no definition is rejected"),
		FGridEquipmentArmorReadModel::TryResolve(Registry.Get(), nullptr, LegacyItem, Armor));
	TestEqual(TEXT("Rejected unknown result has zero remaining armor"), Armor.RemainingPhysical, 0);
	return true;
}

#endif

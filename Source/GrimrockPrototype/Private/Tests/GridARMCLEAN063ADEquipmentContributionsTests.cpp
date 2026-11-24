#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../Runtime/GridEquipmentContributionReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063ADEquipmentContributions,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalEquipmentDiagnosticContributions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063ADEquipmentContributions::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real persisted ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register canonical ItemSystem"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}

	FName FixtureId;
	const FName Ids[] = {TEXT("Weapon_MainGauche_01"), TEXT("Weapon_RoundShield_01"),
		TEXT("Weapon_HolyAegis_01"), TEXT("Weapon_StraightSword_01")};
	const int32 Levels[] = {1, 20};
	for (const FName Id : Ids)
	{
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
				FGridEquipmentContributionReadModel::ResolveStatBonuses(Registry.Get(), Equipped);
			TestEqual(TEXT("Physical armor uses canonical N1-N20"), Bonuses.ArmorBonus,
				Canonical.Weapon.PhysicalArmor);
			TestEqual(TEXT("Magical armor uses canonical N1-N20"), Bonuses.MagicalArmorBonus,
				Canonical.Weapon.MagicalArmor);
			FGridEquipmentStatBonus Expected;
			Expected.ArmorBonus = Canonical.Weapon.PhysicalArmor;
			Expected.MagicalArmorBonus = Canonical.Weapon.MagicalArmor;
			TestTrue(TEXT("ARM only contributes catalogued armor"),
				FGridEquipmentStatBonus::StaticStruct()->CompareScriptStruct(&Bonuses, &Expected, 0));
			const auto Pointerless = FGridEquipmentContributionReadModel::ResolveStatBonuses(
				Registry.Get(), Equipped);
			TestTrue(TEXT("ARM bonuses identical without old pointer"),
				FGridEquipmentStatBonus::StaticStruct()->CompareScriptStruct(&Pointerless, &Expected, 0));

			const FGridDamageResistanceSet Resistances =
				FGridEquipmentContributionReadModel::ResolveResistances(Registry.Get(), Equipped);
			TestTrue(TEXT("Registered ARM ignores old resistance"), Resistances.IsEmpty());
			TestTrue(TEXT("Pointerless ARM has no resistance contribution"),
				FGridEquipmentContributionReadModel::ResolveResistances(Registry.Get(), Equipped).IsEmpty());
			FixtureId = TEXT("Conflicting_ARM_5D5");
			const auto WrongIdentity = FGridEquipmentContributionReadModel::ResolveStatBonuses(
				Registry.Get(), Equipped);
			TestTrue(TEXT("Wrong historical identity cannot affect registered ARM"),
				FGridEquipmentStatBonus::StaticStruct()->CompareScriptStruct(&WrongIdentity, &Expected, 0));
			TestTrue(TEXT("Wrong identity cannot affect resistance"),
				FGridEquipmentContributionReadModel::ResolveResistances(Registry.Get(), Equipped).IsEmpty());
			for (const int32 InvalidLevel : {0, 21})
			{
				Equipped.ItemLevel = InvalidLevel;
				const FGridEquipmentStatBonus Invalid =
					FGridEquipmentContributionReadModel::ResolveStatBonuses(Registry.Get(), Equipped);
				TestFalse(TEXT("Invalid registered ARM cannot inherit old bonuses"), Invalid.HasAnyBonus());
				TestTrue(TEXT("Invalid registered ARM cannot inherit old resistances"),
					FGridEquipmentContributionReadModel::ResolveResistances(Registry.Get(), Equipped).IsEmpty());
			}
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
		FGridEquipmentContributionReadModel::ResolveStatBonuses(Registry.Get(), NoOld);
	TestEqual(TEXT("Canonical shield armor survives missing old definition"),
		WithoutDefinition.ArmorBonus, Shield.Weapon.PhysicalArmor);

	// Unregistered identities are invalid in a configured canonical game.
	// Historical bonus and resistance characterization remains registry-less only.
	FixtureId = TEXT("LegacyStats_063AD");
	FGridItemInstance Legacy;
	Legacy.RuntimeObjectId = FGuid::NewGuid();
	Legacy.ItemDefinitionId = FixtureId;
	Legacy.ItemLevel = 0;
	Legacy.Quantity = 1;
	const FGridEquipmentStatBonus Refused =
		FGridEquipmentContributionReadModel::ResolveStatBonuses(Registry.Get(), Legacy);
	TestFalse(TEXT("Configured canonical game rejects unknown legacy bonus"),
		Refused.HasAnyBonus());
	TestTrue(TEXT("Configured canonical game rejects unknown legacy resistances"),
		FGridEquipmentContributionReadModel::ResolveResistances(Registry.Get(), Legacy).IsEmpty());
	const FGridEquipmentStatBonus LegacyBonuses =
		FGridEquipmentContributionReadModel::ResolveStatBonuses(nullptr, Legacy);
	TestEqual(TEXT("Registry-less armor is absent"), LegacyBonuses.ArmorBonus, 0);
	TestEqual(TEXT("Registry-less magical armor is absent"), LegacyBonuses.MagicalArmorBonus, 0);
	TestEqual(TEXT("Registry-less attribute contribution is absent"), LegacyBonuses.StrengthBonus, 0);
	TestEqual(TEXT("Registry-less resistance contribution is absent"),
		FGridEquipmentContributionReadModel::ResolveResistances(nullptr, Legacy).FireResistance, 0);
	TestTrue(TEXT("Missing definition gives no resistance contribution"),
		FGridEquipmentContributionReadModel::ResolveResistances(Registry.Get(), Legacy).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065D4NonArmContributions,
	"Grimrock.RPG.ARM.CLEAN06_5D4.CanonicalNonArmEquipmentContributions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean065D4NonArmContributions::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Native registry"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register native root"), Registry->RegisterItemSystem(Root.Get(), Errors))) return false;
	int32 Count = 0;
	for (const auto& Entry : Root->ItemDefinitions)
	{
		const auto* Gear = Cast<UGridOtherEquipmentDefinitionAsset>(Entry.Get());
		if (!Gear || Gear->GetAvailableSlots().IsEmpty()) continue;
		++Count;
		FName UnusedId;
		UnusedId = Gear->ItemId;
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = Gear->ItemId;
		Item.ItemLevel = 0;
		Item.Quantity = 1;
		Item.AbsorbedEquipmentPhysicalArmor = 2;
		Item.AbsorbedEquipmentMagicalArmor = 3;
		const auto Bonus = FGridEquipmentContributionReadModel::ResolveStatBonuses(Registry.Get(), Item);
		const auto NoOld = FGridEquipmentContributionReadModel::ResolveStatBonuses(Registry.Get(), Item);
		TestTrue(TEXT("Exact native bonuses, no old source"), FGridEquipmentStatBonus::StaticStruct()->CompareScriptStruct(&Bonus, &Gear->Bonuses, 0));
		TestTrue(TEXT("Same without pointer"), FGridEquipmentStatBonus::StaticStruct()->CompareScriptStruct(&NoOld, &Gear->Bonuses, 0));
		const auto R = FGridEquipmentContributionReadModel::ResolveResistances(Registry.Get(), Item);
		TestTrue(TEXT("Exact native resistance"), FGridDamageResistanceSet::StaticStruct()->CompareScriptStruct(&R, &Gear->Resistances, 0));
		FGridEquipmentArmorReadView Armor;
		if (!TestTrue(TEXT("Physical non-ARM armor resolves"), FGridEquipmentArmorReadModel::TryResolve(Registry.Get(), Item, Armor))) return false;
		FGridArmorReserveView Direct;
		TestTrue(TEXT("Public registry resolves same non-ARM reserve"), Registry->ResolveArmorReserveForInstance(Item, Direct));
		TestEqual(TEXT("Exact spent physical debt"), Direct.SpentPhysicalArmor, 2);
		TestEqual(TEXT("Exact spent magical debt"), Direct.SpentMagicalArmor, 3);
		TestEqual(TEXT("Single physical reserve authority"), Armor.RemainingPhysical, Direct.RemainingPhysicalArmor);
		TestEqual(TEXT("Single magical reserve authority"), Armor.RemainingMagical, Direct.RemainingMagicalArmor);
		TestEqual(TEXT("Nominal armor"), Armor.NominalPhysical, Gear->Bonuses.ArmorBonus);
		TestEqual(TEXT("Physical spent reserve"), Armor.RemainingPhysical, Gear->Bonuses.ArmorBonus > 0 ? FMath::Max(0, Gear->Bonuses.ArmorBonus-2) : Gear->Bonuses.ArmorBonus);
		TestEqual(TEXT("Magical spent reserve"), Armor.RemainingMagical, Gear->Bonuses.MagicalArmorBonus > 0 ? FMath::Max(0, Gear->Bonuses.MagicalArmorBonus-3) : Gear->Bonuses.MagicalArmorBonus);
		Item.AbsorbedEquipmentMagicalArmor = -1;
		TestFalse(TEXT("Negative canonical armor debt rejected"), Registry->ResolveArmorReserveForInstance(Item, Direct));
		Item.AbsorbedEquipmentMagicalArmor = 3;
		Item.ItemLevel = 1;
		TestFalse(TEXT("Invalid registry reserve rejected"), Registry->ResolveArmorReserveForInstance(Item, Direct));
		TestFalse(TEXT("Physical N1 rejected"), FGridEquipmentContributionReadModel::ResolveStatBonuses(Registry.Get(), Item).HasAnyBonus());
		TestTrue(TEXT("Invalid levels do not borrow resistance"), FGridEquipmentContributionReadModel::ResolveResistances(Registry.Get(), Item).IsEmpty());
	}
	TestTrue(TEXT("Real equipment tested"), Count > 0);
	return !HasAnyErrors();
}

#endif

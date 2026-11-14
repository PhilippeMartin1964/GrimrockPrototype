#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridEquipmentPreviewItemReadModel.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AEEquipmentPreviewReadModel,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalEquipmentPreviewReadModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063AEEquipmentPreviewReadModel::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real canonical ItemSystem exists"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register actual ItemSystem"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	TStrongObjectPtr<UGridItemDefinitionAsset> ConflictingOld(NewObject<UGridItemDefinitionAsset>());
	ConflictingOld->DisplayName = FText::FromString(TEXT("Wrong ancient item name"));
	ConflictingOld->EquipmentStatBonus.ArmorBonus = 999;
	ConflictingOld->EquipmentStatBonus.MagicalArmorBonus = 999;
	ConflictingOld->EquipmentStatBonus.StrengthBonus = 7;
	ConflictingOld->EquipmentResistanceBonus.FireResistance = 14;
	const FName Items[] = {
		TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01"),
		TEXT("Weapon_StraightSword_01"), TEXT("Weapon_MainGauche_01")
	};
	const int32 Levels[] = {1, 20};
	for (const FName Id : Items)
	{
		ConflictingOld->ItemDefinitionId = Id;
		for (const int32 Level : Levels)
		{
			FGridResolvedItem Canonical;
			if (!TestTrue(TEXT("Live N1/N20 armament resolves"),
				Registry->ResolveItem(Id, Level, Canonical))) return false;
			FGridItemInstance Item;
			Item.RuntimeObjectId = FGuid::NewGuid();
			Item.ItemDefinitionId = Id;
			Item.Quantity = 1;
			Item.ItemLevel = Level;
			Item.AbsorbedEquipmentPhysicalArmor = 2;
			const FGuid OriginalGuid = Item.RuntimeObjectId;
			const FGridEquipmentPreviewItemReadView Read =
				FGridEquipmentPreviewItemReadModel::Resolve(Registry.Get(), ConflictingOld.Get(), Item);
			TestEqual(TEXT("Preview nominal physical from canonical N-level"),
				Read.Bonuses.ArmorBonus, Canonical.Weapon.PhysicalArmor);
			TestEqual(TEXT("Preview nominal magical from canonical N-level"),
				Read.Bonuses.MagicalArmorBonus, Canonical.Weapon.MagicalArmor);
			TestEqual(TEXT("Preview armor remains debt-aware"),
				Read.RemainingPhysicalArmor,
				FMath::Max(0, Canonical.Weapon.PhysicalArmor - 2));
			TestEqual(TEXT("Preview strength retains as-yet-unmigrated definition authority"),
				Read.Bonuses.StrengthBonus, 7);
			TestEqual(TEXT("Preview fire resistance equals actual gameplay definition contribution"),
				Read.Resistances.FireResistance, 14);
			TestEqual(TEXT("Displaced item name comes from canonical item definition"),
				Read.DisplayName.ToString(), Canonical.DisplayName.ToString());
			TestEqual(TEXT("Preview never alters item identity"), Item.RuntimeObjectId, OriginalGuid);
			TestEqual(TEXT("Preview never repairs absorbed armor"), Item.AbsorbedEquipmentPhysicalArmor, 2);
			Item.ItemLevel = 0;
			const FGridEquipmentPreviewItemReadView Invalid =
				FGridEquipmentPreviewItemReadModel::Resolve(Registry.Get(), ConflictingOld.Get(), Item);
			TestFalse(TEXT("Known ARM N0 cannot borrow old physical/magical armor"),
				Invalid.Bonuses.HasAnyBonus());
			TestEqual(TEXT("Invalid ARM N0 leaves no remaining armor"), Invalid.RemainingPhysicalArmor, 0);
			TestEqual(TEXT("Invalid ARM N0 name never reverts to old definition"),
				Invalid.DisplayName.ToString(), Id.ToString());
		}
	}
	ConflictingOld->ItemDefinitionId = TEXT("LegacyPreview_AE");
	ConflictingOld->EquipmentStatBonus.ArmorBonus = 4;
	FGridItemInstance Legacy;
	Legacy.RuntimeObjectId = FGuid::NewGuid();
	Legacy.ItemDefinitionId = ConflictingOld->ItemDefinitionId;
	Legacy.ItemLevel = 0;
	Legacy.Quantity = 1;
	Legacy.AbsorbedEquipmentPhysicalArmor = 1;
	const FGridEquipmentPreviewItemReadView LegacyView =
		FGridEquipmentPreviewItemReadModel::Resolve(Registry.Get(), ConflictingOld.Get(), Legacy);
	TestEqual(TEXT("Unregistered item preserves legacy armor"), LegacyView.Bonuses.ArmorBonus, 4);
	TestEqual(TEXT("Unregistered item keeps remaining armor logic"), LegacyView.RemainingPhysicalArmor, 3);
	TestEqual(TEXT("Unregistered item displays its authored name"),
		LegacyView.DisplayName.ToString(), FString(TEXT("Wrong ancient item name")));
	TestEqual(TEXT("Unregistered item resistance remains unchanged"), LegacyView.Resistances.FireResistance, 14);
	TestEqual(TEXT("No definition means no fabricated resistance"),
		FGridEquipmentPreviewItemReadModel::Resolve(Registry.Get(), nullptr, Legacy).Resistances.FireResistance, 0);
	return true;
}

#endif

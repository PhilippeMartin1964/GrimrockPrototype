#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/Combat/GridEquippedGuardItemReadModel.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063ABGuardIdentity,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalGuardItemIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063ABGuardIdentity::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real canonical ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Real ItemSystem registers"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	TStrongObjectPtr<UGridItemDefinitionAsset> ConflictingLegacy(NewObject<UGridItemDefinitionAsset>());
	ConflictingLegacy->ItemType = EGridItemType::Shield; // Contradicts ARM013.
	ConflictingLegacy->bUseWeaponCatalogue = true;
	ConflictingLegacy->WeaponArchetypeId = TEXT("ARM013");
	const FName Ids[] = {
		TEXT("Weapon_MainGauche_01"), TEXT("Weapon_StraightSword_01"),
		TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01")
	};
	const int32 Levels[] = {1, 20};
	for (const FName Id : Ids)
	{
		ConflictingLegacy->ItemDefinitionId = Id;
		for (const int32 Level : Levels)
		{
			FGridResolvedItem Definition;
			if (!TestTrue(TEXT("Actual production N1/N20 resolves"),
				Registry->ResolveItem(Id, Level, Definition))) return false;
			FGridItemInstance OffHand;
			OffHand.RuntimeObjectId = FGuid::NewGuid();
			OffHand.ItemDefinitionId = Id;
			OffHand.ItemLevel = Level;
			OffHand.Quantity = 1;
			OffHand.OwnerType = EGridItemOwnerType::EquipmentSlot;
			OffHand.EquipmentSlot = EGridEquipmentSlot::OffHand;
			const FGridEquippedGuardItemReadView View =
				FGridEquippedGuardItemReadModel::Resolve(
					Registry.Get(), ConflictingLegacy.Get(), OffHand);
			TestTrue(TEXT("Known ARM identified canonically"), View.bCanonicalIdentity && View.bResolved);
			const bool bOffHand = Definition.Weapon.CanEquipInHand(EGridEquipmentSlot::OffHand);
			const bool bExpectedParry = bOffHand &&
				Definition.Weapon.Kind == EGridCanonicalArmamentKind::Weapon &&
				Definition.ArchetypeId == FName(TEXT("ARM013"));
			const bool bExpectedShield = bOffHand &&
				Definition.Weapon.Kind == EGridCanonicalArmamentKind::Shield;
			TestEqual(TEXT("Parry eligibility ignores contradictory old archetype/type"),
				View.bCanParry, bExpectedParry);
			TestEqual(TEXT("Shield classification ignores contradictory old type"),
				View.bIsShield, bExpectedShield);
			if (Id == FName(TEXT("Weapon_MainGauche_01")))
			{
				TestTrue(TEXT("ARM013 actually grants existing offhand parry"), View.bCanParry);
				TestFalse(TEXT("Main-gauche cannot be reclassified as shield"), View.bIsShield);
			}
			if (Id == FName(TEXT("Weapon_RoundShield_01")))
			{
				TestTrue(TEXT("ARM075 is an actual shield"), View.bIsShield);
				TestFalse(TEXT("Shield cannot inherit ARM013 parry"), View.bCanParry);
			}
			OffHand.ItemLevel = 0;
			const FGridEquippedGuardItemReadView BadLevel =
				FGridEquippedGuardItemReadModel::Resolve(
					Registry.Get(), ConflictingLegacy.Get(), OffHand);
			TestTrue(TEXT("Known N0 remains classified as canonical"), BadLevel.bCanonicalIdentity);
			TestFalse(TEXT("Known N0 cannot fall back to parry"), BadLevel.bCanParry);
			TestFalse(TEXT("Known N0 cannot fall back to shield"), BadLevel.bIsShield);
			TestFalse(TEXT("Known N0 does not resolve"), BadLevel.bResolved);
			OffHand.ItemLevel = Level;
			OffHand.EquipmentSlot = EGridEquipmentSlot::MainHand;
			const FGridEquippedGuardItemReadView WrongHand =
				FGridEquippedGuardItemReadModel::Resolve(
					Registry.Get(), ConflictingLegacy.Get(), OffHand);
			TestFalse(TEXT("Mainhand never grants offhand parry"), WrongHand.bCanParry);
			TestFalse(TEXT("Mainhand cannot claim offhand shield block"), WrongHand.bIsShield);
		}
	}

	// Unregistered legacy item and standalone tests without a world keep
	// the original fallback until the 06.4 asset migration.
	ConflictingLegacy->ItemDefinitionId = TEXT("LegacyGuard_063AB");
	ConflictingLegacy->ItemType = EGridItemType::Weapon;
	FGridItemInstance Unknown;
	Unknown.RuntimeObjectId = FGuid::NewGuid();
	Unknown.ItemDefinitionId = ConflictingLegacy->ItemDefinitionId;
	Unknown.ItemLevel = 1;
	Unknown.Quantity = 1;
	Unknown.EquipmentSlot = EGridEquipmentSlot::OffHand;
	const FGridEquippedGuardItemReadView Old =
		FGridEquippedGuardItemReadModel::Resolve(Registry.Get(), ConflictingLegacy.Get(), Unknown);
	TestFalse(TEXT("Unknown identity stays outside canonical registry"), Old.bCanonicalIdentity);
	TestTrue(TEXT("Unknown item preserves legacy ARM013 rule"), Old.bCanParry);
	ConflictingLegacy->ItemType = EGridItemType::Shield;
	TestTrue(TEXT("Unknown shield keeps historical classification"),
		FGridEquippedGuardItemReadModel::Resolve(
			Registry.Get(), ConflictingLegacy.Get(), Unknown).bIsShield);
	TestFalse(TEXT("Unknown item without definition has no guard reaction"),
		FGridEquippedGuardItemReadModel::Resolve(Registry.Get(), nullptr, Unknown).bResolved);
	return true;
}

#endif

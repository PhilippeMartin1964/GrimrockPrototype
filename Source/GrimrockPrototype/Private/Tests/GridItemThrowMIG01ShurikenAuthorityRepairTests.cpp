#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/PackageName.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace GridItemThrowMIG01
{
	const TCHAR* ShurikenAssetPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Shuriken.DA_Weapon_Shuriken");

	bool SaveAssetPackage(UGridItemDefinitionAsset* Asset)
	{
		if (!IsValid(Asset))
		{
			return false;
		}

		UPackage* Package = Asset->GetOutermost();
		if (!Package)
		{
			return false;
		}

		const FString Filename =
			FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		SaveArgs.Error = GError;
		return UPackage::SavePackage(Package, Asset, *Filename, SaveArgs);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemThrowMIG01ShurikenAuthorityRepairTest,
	"Grimrock.Items.ITEM_THROW_MIG01.ShurikenAuthorityRepair",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemThrowMIG01ShurikenAuthorityRepairTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridItemThrowMIG01;

	UGridItemDefinitionAsset* Shuriken = LoadObject<UGridItemDefinitionAsset>(nullptr, ShurikenAssetPath);
	if (!TestNotNull(TEXT("Production DA_Weapon_Shuriken loads"), Shuriken))
	{
		return false;
	}

	TestEqual(TEXT("Shuriken business id remains stable"), Shuriken->ItemDefinitionId, FName(TEXT("Shuriken")));
	TestTrue(TEXT("Pre-cleanup Shuriken still contains the serialized legacy throwable bit"), Shuriken->bThrowable);

	const bool bNeedsRepair = Shuriken->HandUsage != EGridItemHandUsage::OneHanded || !Shuriken->bCombatThrowWeapon;
	if (bNeedsRepair)
	{
		Shuriken->Modify();
		Shuriken->HandUsage = EGridItemHandUsage::OneHanded;
		Shuriken->bCombatThrowWeapon = true;
		Shuriken->MarkPackageDirty();
		if (!TestTrue(TEXT("Canonical Shuriken throw authority package saves"), SaveAssetPackage(Shuriken)))
		{
			return false;
		}
	}

	TestEqual(TEXT("Shuriken explicitly authors one-handed physical usage"), Shuriken->HandUsage, EGridItemHandUsage::OneHanded);
	TestTrue(TEXT("Shuriken explicitly authors combat projectile semantics"), Shuriken->bCombatThrowWeapon);
	TestTrue(TEXT("Shuriken is physically throwable from canonical authoring"), Shuriken->IsPhysicallyThrowable());
	TestTrue(TEXT("Shuriken is combat throwable from canonical authoring"), Shuriken->IsCombatThrowable());
	TestTrue(TEXT("Shuriken remains a MainHand item"), Shuriken->CompatibleEquipmentSlots.Contains(EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("Shuriken remains a valid item definition"), Shuriken->IsValidDefinition());

	AddInfo(bNeedsRepair
		? TEXT("DA_Weapon_Shuriken was migrated to explicit HandUsage=OneHanded and bCombatThrowWeapon=true.")
		: TEXT("DA_Weapon_Shuriken already had canonical throw authority; no package mutation was required."));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

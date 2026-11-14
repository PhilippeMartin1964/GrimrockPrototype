#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../Runtime/Monsters/GridLootStaticMetadataReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063XLootMetadata,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalMonsterLootMetadata",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063XLootMetadata::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real canonical ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register real persisted registry"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	const FText OldName = FText::FromString(TEXT("WRONG OLD LOOT NAME"));
	const float OldWeight = 54321.0f;
	const FName Arms[] = {TEXT("Weapon_StraightSword_01"), TEXT("Weapon_ThrowingKnife_01"),
		TEXT("Weapon_GreatAxe_01"), TEXT("Weapon_RoundShield_01")};
	const int32 Levels[] = {1, 20};
	for (const FName Id : Arms)
	{
		for (const int32 Level : Levels)
		{
			FGridResolvedItem Canonical;
			if (!TestTrue(TEXT("Real authored canonical loot profile"), Registry->ResolveItem(Id, Level, Canonical)))
				return false;
			FGridItemInstance Drop;
			Drop.RuntimeObjectId = FGuid::NewGuid();
			Drop.ItemDefinitionId = Id;
			Drop.ItemLevel = Level;
			Drop.Quantity = 2;
			const FGridLootStaticMetadataReadView Meta =
				FGridLootStaticMetadataReadModel::Resolve(Registry.Get(), Drop, OldName, OldWeight);
			TestTrue(TEXT("Registered ARM loot uses canonical identity"), Meta.bCanonicalIdentity);
			TestTrue(TEXT("Registered ARM loot resolves"), Meta.bResolved);
			TestEqual(TEXT("Loot name ignores stale DA_Weapon"), Meta.DisplayName.ToString(),
				Canonical.DisplayName.IsEmpty() ? Id.ToString() : Canonical.DisplayName.ToString());
			TestTrue(TEXT("Loot weight ignores old N1 profile"),
				FMath::IsNearlyEqual(Meta.UnitWeightKg, Canonical.WeightKg));
			Drop.ItemLevel = 0;
			const FGridLootStaticMetadataReadView Invalid =
				FGridLootStaticMetadataReadModel::Resolve(Registry.Get(), Drop, OldName, OldWeight);
			TestTrue(TEXT("Known ARM cannot use old value for invalid N0"), Invalid.bCanonicalIdentity);
			TestFalse(TEXT("Invalid level blocks canonical loot projection"), Invalid.bResolved);
			TestTrue(TEXT("No old name on invalid canonical item"), Invalid.DisplayName.IsEmpty());
			TestEqual(TEXT("No old mass on invalid canonical item"), Invalid.UnitWeightKg, 0.0f);
		}
	}
	FGridItemInstance Arrow;
	Arrow.RuntimeObjectId = FGuid::NewGuid();
	Arrow.ItemDefinitionId = TEXT("Item_Arrows_01");
	Arrow.ItemLevel = 0;
	Arrow.Quantity = 12;
	FGridResolvedItem CanonicalArrow;
	if (!TestTrue(TEXT("Real unlevelled arrows exist"), Registry->ResolveItem(Arrow.ItemDefinitionId, 0, CanonicalArrow)))
		return false;
	const FGridLootStaticMetadataReadView Ammo =
		FGridLootStaticMetadataReadModel::Resolve(Registry.Get(), Arrow, OldName, OldWeight);
	TestTrue(TEXT("Unlevelled arrows use same canonical owner"), Ammo.bCanonicalIdentity && Ammo.bResolved);
	TestTrue(TEXT("Unlevelled arrows retain unit mass"),
		FMath::IsNearlyEqual(Ammo.UnitWeightKg, CanonicalArrow.WeightKg));
	FGridItemInstance Legacy;
	Legacy.RuntimeObjectId = FGuid::NewGuid();
	Legacy.ItemDefinitionId = TEXT("LegacyMonsterLoot_063X");
	Legacy.ItemLevel = 0;
	Legacy.Quantity = 1;
	const FGridLootStaticMetadataReadView Old =
		FGridLootStaticMetadataReadModel::Resolve(Registry.Get(), Legacy, OldName, OldWeight);
	TestFalse(TEXT("Unknown ID remains old"), Old.bCanonicalIdentity);
	TestTrue(TEXT("Unknown ID remains supported"), Old.bResolved);
	TestEqual(TEXT("Old loot name preserved"), Old.DisplayName.ToString(), OldName.ToString());
	TestEqual(TEXT("Old loot mass preserved"), Old.UnitWeightKg, OldWeight);
	return true;
}

#endif

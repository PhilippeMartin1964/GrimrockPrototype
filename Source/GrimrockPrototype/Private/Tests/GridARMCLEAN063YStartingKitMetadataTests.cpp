#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../RPG/GridStartingKitStaticMetadataReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063YStartingKitStaticMetadata,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalStartingKitStaticMetadata",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063YStartingKitStaticMetadata::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real staged ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Valid canonical system"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	const FText WrongName = FText::FromString(TEXT("WRONG STARTER COPY"));
	const float WrongWeight = 123456.0f;
	const FName Ids[] = {TEXT("Weapon_StraightSword_01"), TEXT("Weapon_ThrowingKnife_01"),
		TEXT("Weapon_GreatAxe_01"), TEXT("Weapon_RoundShield_01")};
	const int32 Levels[] = {1, 20};
	for (const FName Id : Ids)
	{
		for (const int32 Level : Levels)
		{
			FGridResolvedItem Canonical;
			if (!TestTrue(TEXT("Canonical kit ARM resolves at N1 and N20"),
				Registry->ResolveItem(Id, Level, Canonical))) return false;
			const FGridStartingKitStaticMetadataReadView View =
				FGridStartingKitStaticMetadataReadModel::Resolve(Registry.Get(), Id, Level, WrongName, WrongWeight);
			TestTrue(TEXT("Registered starter uses registry"), View.bCanonicalIdentity && View.bResolved);
			TestEqual(TEXT("Starting kit name is canonical"), View.DisplayName.ToString(),
				Canonical.DisplayName.IsEmpty() ? Id.ToString() : Canonical.DisplayName.ToString());
			TestTrue(TEXT("Starting kit mass is canonical"), FMath::IsNearlyEqual(View.UnitWeightKg, Canonical.WeightKg));
		}
		const FGridStartingKitStaticMetadataReadView N0 =
			FGridStartingKitStaticMetadataReadModel::Resolve(Registry.Get(), Id, 0, WrongName, WrongWeight);
		TestTrue(TEXT("Invalid N0 ARM classified canonical"), N0.bCanonicalIdentity);
		TestFalse(TEXT("Invalid N0 ARM never granted"), N0.bResolved);
		TestTrue(TEXT("Invalid N0 ARM cannot inherit obsolete name"), N0.DisplayName.IsEmpty());
		TestEqual(TEXT("Invalid N0 ARM cannot inherit old mass"), N0.UnitWeightKg, 0.0f);
	}
	FGridResolvedItem Arrows;
	if (!TestTrue(TEXT("Unlevelled arrow N0 resolves"), Registry->ResolveItem(TEXT("Item_Arrows_01"), 0, Arrows)))
		return false;
	const FGridStartingKitStaticMetadataReadView Ammo =
		FGridStartingKitStaticMetadataReadModel::Resolve(Registry.Get(), TEXT("Item_Arrows_01"), 0, WrongName, WrongWeight);
	TestTrue(TEXT("Arrows without level remain canonical"), Ammo.bCanonicalIdentity && Ammo.bResolved);
	TestTrue(TEXT("Starting arrow weight is canonical"),
		FMath::IsNearlyEqual(Ammo.UnitWeightKg, Arrows.WeightKg));
	const FGridStartingKitStaticMetadataReadView Legacy =
		FGridStartingKitStaticMetadataReadModel::Resolve(
			Registry.Get(), TEXT("LegacyStarter063Y"), 0, WrongName, WrongWeight);
	TestFalse(TEXT("Nonregistered starter still legacy"), Legacy.bCanonicalIdentity);
	TestTrue(TEXT("Legacy starter remains allowed"), Legacy.bResolved);
	TestEqual(TEXT("Legacy name unchanged"), Legacy.DisplayName.ToString(), WrongName.ToString());
	TestEqual(TEXT("Legacy weight unchanged"), Legacy.UnitWeightKg, WrongWeight);
	return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/Items/GridItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridPhysicalThrowStrengthReadModel.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemThrowCLEAN01Test, "Grimrock.Items.ITEM_THROW_CLEAN01",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemThrowCLEAN01Test::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* ItemClass = UGridNonArmamentDefinitionAsset::StaticClass();
	TestNotNull(TEXT("Item definition class exists"), ItemClass);
	if (!ItemClass)
	{
		return false;
	}

	TestNull(TEXT("Legacy bThrowable property is physically removed"), ItemClass->FindPropertyByName(TEXT("bThrowable")));

	// Physical throw eligibility is read from ItemSystem.
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem"));
	if (!TestNotNull(TEXT("Production canonical root loads"), Root)) return false;
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Production canonical registry validates"),
		Registry->RegisterItemSystem(Root, Errors))) return false;
	FGridResolvedItem Shuriken;
	if (!TestTrue(TEXT("Shuriken N0 resolves canonically"),
		Registry->ResolveItem(TEXT("Shuriken"), 0, Shuriken))) return false;
	// H.25.1: Unlike the 22 ARM N1-N20 weapons, production Shuriken
	// is one of the 26 canonical NON-armament N0 identities.
	// Its throw is authored on the non-armament item, not WeaponCatalogue.
	TestFalse(TEXT("Shuriken is a non-ARM canonical item"), Shuriken.bIsArmament);
	TestEqual(TEXT("Shuriken has the physical non-ARM level"), Shuriken.ItemLevel, 0);
	TestTrue(TEXT("Shuriken owns recoverable combat throw policy"),
		Shuriken.GenericThrow.bCombatRecoverable);
	TestTrue(TEXT("Shuriken physically launches with positive authored speed"),
		FMath::IsFinite(Shuriken.GenericThrow.InitialSpeed) &&
		Shuriken.GenericThrow.InitialSpeed > KINDA_SMALL_NUMBER);

	FGridItemInstance PhysicalShuriken;
	PhysicalShuriken.RuntimeObjectId = FGuid::NewGuid();
	PhysicalShuriken.ItemDefinitionId = Shuriken.ItemId;
	PhysicalShuriken.ItemLevel = 0;
	PhysicalShuriken.Quantity = 1;
	const FGridPhysicalThrowStrengthReadView Allowed =
		FGridPhysicalThrowStrengthReadModel::Resolve(Registry.Get(), PhysicalShuriken, 100);
	TestTrue(TEXT("Physical throw gate recognizes the canonical Shuriken"), Allowed.bRegisteredIdentity);
	TestTrue(TEXT("Shuriken physical throw is resolved without historical item definition"), Allowed.bResolved);
	TestTrue(TEXT("Shuriken is carried and thrown with one hand"), Allowed.bOneHanded);
	TestTrue(TEXT("High enough Strength allows real Shuriken physical throwing"), Allowed.bCanThrow);
	TestTrue(TEXT("Physical throw uses the only canonical unit mass"),
		FMath::IsNearlyEqual(Allowed.UnitWeightKg, Shuriken.WeightKg));
	TestTrue(TEXT("Valid physical throw has a positive speed multiplier"), Allowed.SpeedScale > 0.f);

	const FGridPhysicalThrowStrengthReadView NoStrength =
		FGridPhysicalThrowStrengthReadModel::Resolve(Registry.Get(), PhysicalShuriken, 0);
	TestTrue(TEXT("Strength zero still recognizes canonical identity"), NoStrength.bRegisteredIdentity);
	TestTrue(TEXT("Strength zero resolves without a legacy definition"), NoStrength.bResolved);
	TestFalse(TEXT("Strength zero cannot physically throw the Shuriken"), NoStrength.bCanThrow);

	PhysicalShuriken.ItemLevel = 1;
	const FGridPhysicalThrowStrengthReadView WrongLevel =
		FGridPhysicalThrowStrengthReadModel::Resolve(Registry.Get(), PhysicalShuriken, 100);
	TestTrue(TEXT("Wrong-level N1 Shuriken cannot escape canonical membership"), WrongLevel.bRegisteredIdentity);
	TestFalse(TEXT("N1 non-ARM Shuriken is not resolved"), WrongLevel.bResolved);
	TestFalse(TEXT("N1 non-ARM Shuriken cannot be thrown"), WrongLevel.bCanThrow);

	PhysicalShuriken.ItemDefinitionId = TEXT("Unknown_ItemThrowClean01");
	PhysicalShuriken.ItemLevel = 0;
	const FGridPhysicalThrowStrengthReadView Unknown =
		FGridPhysicalThrowStrengthReadModel::Resolve(Registry.Get(), PhysicalShuriken, 100);
	TestFalse(TEXT("Unknown item is not a canonical identity"), Unknown.bRegisteredIdentity);
	TestFalse(TEXT("Configured registry does not invent legacy throw authority"), Unknown.bResolved);
	TestFalse(TEXT("Unknown item cannot be thrown without an authored source"), Unknown.bCanThrow);

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean06ActionIconsProjection.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062KRealIconPaletteProjection,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionActionIconPalette",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062KRealIconPaletteProjection::RunTest(const FString&)
{
	const UGridEquippedWeaponActionIconPalette* Source =
		LoadObject<UGridEquippedWeaponActionIconPalette>(nullptr,
			TEXT("/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons.DA_EquippedWeaponActionIcons"));
	if (!TestNotNull(TEXT("Production weapon action palette is present"), Source))
	{
		return false;
	}
	const bool bWasDirty = Source->GetOutermost()->IsDirty();
	TStrongObjectPtr<UGridCanonicalWeaponVisualCatalogueAsset> Candidate(
		NewObject<UGridCanonicalWeaponVisualCatalogueAsset>(GetTransientPackage()));
	const bool bProjected = GridArmClean06ActionIconsProjection::TryProject(*Source, *Candidate);
	if (!TestTrue(TEXT("Existing palette projected without loss"), bProjected))
	{
		return false;
	}
	TestTrue(TEXT("Every existing kind and texture preserved"),
		GridArmClean06ActionIconsProjection::IsIdentical(*Source, *Candidate));
	int32 Configured = 0;
	int32 Empty = 0;
	for (const TPair<EGridEquippedWeaponActionIconKind, TSoftObjectPtr<UTexture2D>>& Entry : Candidate->ActionIcons)
	{
		if (Entry.Value.IsNull()) { ++Empty; } else { ++Configured; }
	}
	TArray<FString> Errors;
	TestTrue(TEXT("Canonical palette shape validates"), Candidate->Validate(Errors));
	TestEqual(TEXT("One source entry produces one canonical entry"),
		Source->Icons.Num(), Candidate->ActionIcons.Num());
	TestTrue(TEXT("No source package change"),
		Source->GetOutermost()->IsDirty() == bWasDirty);
	AddInfo(FString::Printf(TEXT(
		"ARM 06.2K icons: %d kinds, %d configured, %d intentionally empty. "
		"Every soft path preserved, no texture, action or asset created."),
		Candidate->ActionIcons.Num(), Configured, Empty));
	return Errors.IsEmpty() && Source->GetOutermost()->IsDirty() == bWasDirty;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062KIconSafety,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ActionIconProjectionSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062KIconSafety::RunTest(const FString&)
{
	TStrongObjectPtr<UGridEquippedWeaponActionIconPalette> Source(
		NewObject<UGridEquippedWeaponActionIconPalette>(GetTransientPackage()));
	TStrongObjectPtr<UGridCanonicalWeaponVisualCatalogueAsset> Candidate(
		NewObject<UGridCanonicalWeaponVisualCatalogueAsset>(GetTransientPackage()));
	TestFalse(TEXT("Empty palette cannot pretend migrated"),
		GridArmClean06ActionIconsProjection::TryProject(*Source, *Candidate));
	Source->Icons.Add(EGridEquippedWeaponActionIconKind::None, TSoftObjectPtr<UTexture2D>());
	TestFalse(TEXT("None is never an actionable category"),
		GridArmClean06ActionIconsProjection::TryProject(*Source, *Candidate));
	TestEqual(TEXT("No bogus icon added on failure"), Candidate->ActionIcons.Num(), 0);

	Source->Icons.Reset();
	Source->Icons.Add(EGridEquippedWeaponActionIconKind::MeleeOneHand,
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(
			TEXT("/Game/Tests/ARM062/TestActionIcon.TestActionIcon"))));
	TestTrue(TEXT("A legitimate icon soft reference can be projected"),
		GridArmClean06ActionIconsProjection::TryProject(*Source, *Candidate));
	TestFalse(TEXT("Projection never overwrites an authored destination map"),
		GridArmClean06ActionIconsProjection::TryProject(*Source, *Candidate));
	TestTrue(TEXT("Rejected duplicate leaves existing mapping identical"),
		GridArmClean06ActionIconsProjection::IsIdentical(*Source, *Candidate));
	return true;
}
#endif

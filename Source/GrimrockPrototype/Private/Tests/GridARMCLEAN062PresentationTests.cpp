#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridClean062PresentationVariantsTest,
	"Grimrock.RPG.ARM.CLEAN06_2.Presentation.Variants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062PresentationVariantsTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGridActionPresentationCatalogueAsset> Catalogue(
		NewObject<UGridActionPresentationCatalogueAsset>());
	FGridCanonicalActionPresentation& Presentation =
		Catalogue->Presentations.AddDefaulted_GetRef();
	Presentation.PresentationId = TEXT("SwordSwingStandard");
	Presentation.MotionStyle = EGridPlayerAttackMotionStyle::Swing;
	Presentation.bAnimateHeldItem = true;
	Presentation.MotionDurationSeconds = 0.24f;
	Presentation.ImpactDelaySeconds = 0.17f;
	Presentation.RecoveryDelaySeconds = 0.31f;
	Presentation.FeedbackDurationSeconds = 1.4f;
	Presentation.PeakLocationOffset = FVector(12.0f, -7.0f, 2.0f);
	Presentation.PeakRotationOffset = FRotator(5.0f, -15.0f, 0.0f);
	Presentation.AttackAudio.VolumeMultiplier = 0.85f;
	Presentation.AttackAudio.PitchMin = 0.92f;
	Presentation.AttackAudio.PitchMax = 1.08f;
	TArray<FString> ExpectedSounds;
	for (int32 Index = 1; Index <= 4; ++Index)
	{
		const FString Name = FString::Printf(TEXT("TestSwordSwing%02d"), Index);
		const FString Path = FString::Printf(TEXT("/Game/Tests/ARM062/%s.%s"),
			*Name, *Name);
		ExpectedSounds.Add(Path);
		Presentation.AttackAudio.Sounds.Add(
			TSoftObjectPtr<USoundBase>(FSoftObjectPath(Path)));
	}
	Presentation.ImpactHitAudio.Sounds.Add(TSoftObjectPtr<USoundBase>(
		FSoftObjectPath(TEXT("/Game/Tests/ARM062/TestHit.TestHit"))));
	Presentation.ImpactMissAudio.Sounds.Add(TSoftObjectPtr<USoundBase>(
		FSoftObjectPath(TEXT("/Game/Tests/ARM062/TestMiss.TestMiss"))));
	Presentation.AttackVFX.Systems.Add(TSoftObjectPtr<UNiagaraSystem>(
		FSoftObjectPath(TEXT("/Game/Tests/ARM062/TestSwingFX.TestSwingFX"))));
	Presentation.AttackVFX.SocketName = TEXT("SwordSocket");
	Presentation.AttackVFX.LocationOffset = FVector(1.0f, 2.0f, 3.0f);
	Presentation.ImpactHitVFX.Systems.Add(TSoftObjectPtr<UNiagaraSystem>(
		FSoftObjectPath(TEXT("/Game/Tests/ARM062/TestHitFX.TestHitFX"))));
	
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Lossless canonical presentation validates"),
		Catalogue->Validate(Errors)))
	{
		for (const FString& Error : Errors)
		{
			AddError(Error);
		}
		return false;
	}
	const FGridCanonicalActionPresentation* Found =
		Catalogue->Find(TEXT("SwordSwingStandard"));
	if (!TestNotNull(TEXT("Presentation resolved by one stable ID"), Found))
	{
		return false;
	}
	TestEqual(TEXT("All four Sword sound variants survive"), Found->AttackAudio.Sounds.Num(), 4);
	for (int32 Index = 0; Index < ExpectedSounds.Num(); ++Index)
	{
		TestEqual(FString::Printf(TEXT("Sword sound variant %d retains order and path"), Index + 1),
			Found->AttackAudio.Sounds[Index].ToSoftObjectPath().ToString(), ExpectedSounds[Index]);
	}
	TestEqual(TEXT("Attack volume remains authored once"), Found->AttackAudio.VolumeMultiplier, 0.85f);
	TestEqual(TEXT("Minimum pitch preserved"), Found->AttackAudio.PitchMin, 0.92f);
	TestEqual(TEXT("Maximum pitch preserved"), Found->AttackAudio.PitchMax, 1.08f);
	TestEqual(TEXT("Impact delay preserved"), Found->ImpactDelaySeconds, 0.17f);
	TestEqual(TEXT("Recovery delay preserved"), Found->RecoveryDelaySeconds, 0.31f);
	TestEqual(TEXT("Impact hit has an independent sound"), Found->ImpactHitAudio.Sounds.Num(), 1);
	TestEqual(TEXT("Impact miss has an independent sound"), Found->ImpactMissAudio.Sounds.Num(), 1);
	TestEqual(TEXT("Niagara remains an array"), Found->AttackVFX.Systems.Num(), 1);
	TestEqual(TEXT("Niagara attachment socket preserved"), Found->AttackVFX.SocketName,
		FName(TEXT("SwordSocket")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridClean062PresentationInvalidTest,
	"Grimrock.RPG.ARM.CLEAN06_2.Presentation.RejectInvalid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062PresentationInvalidTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGridActionPresentationCatalogueAsset> Catalogue(
		NewObject<UGridActionPresentationCatalogueAsset>());
	FGridCanonicalActionPresentation& Presentation = Catalogue->Presentations.AddDefaulted_GetRef();
	Presentation.PresentationId = TEXT("TestAttack");
	TArray<FString> Errors;
	TestTrue(TEXT("Empty optional event channels are valid"), Catalogue->Validate(Errors));

	Presentation.AttackAudio.PitchMin = 1.3f;
	Presentation.AttackAudio.PitchMax = 0.9f;
	Errors.Reset();
	TestFalse(TEXT("Inverted sound pitch range is rejected"), Catalogue->Validate(Errors));
	Presentation.AttackAudio.PitchMin = 1.0f;
	Presentation.AttackAudio.PitchMax = 1.0f;

	Presentation.AttackAudio.Sounds.Add(TSoftObjectPtr<USoundBase>());
	Errors.Reset();
	TestFalse(TEXT("Null configured audio variant is rejected"), Catalogue->Validate(Errors));
	Presentation.AttackAudio.Sounds.Reset();

	Presentation.AttackVFX.Scale = FVector(1.0f, 0.0f, 1.0f);
	Errors.Reset();
	TestFalse(TEXT("Degenerate Niagara attachment scale is rejected"), Catalogue->Validate(Errors));
	Presentation.AttackVFX.Scale = FVector::OneVector;

	Presentation.ImpactDelaySeconds = -0.2f;
	Errors.Reset();
	TestFalse(TEXT("Negative impact timing is rejected"), Catalogue->Validate(Errors));
	Presentation.ImpactDelaySeconds = 0.16f;

	Errors.Reset();
	TestTrue(TEXT("Restored presentation validates"), Catalogue->Validate(Errors));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

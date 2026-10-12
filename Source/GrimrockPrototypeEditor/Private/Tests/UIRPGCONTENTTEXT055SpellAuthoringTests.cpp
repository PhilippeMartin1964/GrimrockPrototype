#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Magic/GridSpellCatalogAuthoring.h"
#include "Magic/GridSpellDefinitionAsset.h"
#include "Runtime/Combat/GridPlayerAttackPresentationTypes.h"

namespace UIRPGCONTENTTEXT055
{
	FGridSpellPresentationProfile MakeSentinelPresentation()
	{
		FGridSpellPresentationProfile Presentation;
		Presentation.Projectile.bEnabled = false;
		Presentation.Projectile.TravelDurationSeconds = 0.75f;
		Presentation.Projectile.VisualScale = FVector(1.5f, 1.25f, 1.0f);
		Presentation.FeedbackDurationSeconds = 3.5f;
		return Presentation;
	}
}

/**
 * TEXT05.5 validates the safe materialization contract without saving packages.
 * Binary DA_Spell_* creation remains a separate local Unreal execution step.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGCONTENTTEXT055SpellMaterializationAuthoringTest,
	"Grimrock.UI.RPG.CONTENTTEXT05.SpellMaterializationAuthoring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT055SpellMaterializationAuthoringTest::RunTest(const FString&)
{
	using namespace UIRPGCONTENTTEXT055;

	TArray<FGridProductionSpellAuthoringSeed> Seeds;
	FString SeedError;
	if (!TestTrue(
			TEXT("Production Spell authoring seeds build from the existing runtime library"),
			FGridSpellCatalogAuthoring::BuildProductionSeeds(Seeds, SeedError)))
	{
		if (!SeedError.IsEmpty())
		{
			AddError(SeedError);
		}
		return false;
	}

	FGridPlayerAttackPresentationProfile CrossModuleAttackPresentation;
	TestTrue(
		TEXT("Player attack presentation validation links across runtime/editor module boundary"),
		CrossModuleAttackPresentation.IsValid());

	TestEqual(TEXT("Exactly four production Spell seeds exist"), Seeds.Num(), 4);
	TestEqual(
		TEXT("Spell production folder is stable"),
		FString(FGridSpellCatalogAuthoring::ProductionFolder()),
		FString(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Magic/Spells")));

	int32 FreshBootstrapCount = 0;
	int32 PreservedPresentationCount = 0;

	for (const FGridProductionSpellAuthoringSeed& Seed : Seeds)
	{
		const FName SpellId = Seed.Definition.SpellId;
		TestFalse(
			*FString::Printf(TEXT("%s seed SpellId is valid"), *SpellId.ToString()),
			SpellId.IsNone());
		TestTrue(
			*FString::Printf(TEXT("%s seed definition validates"), *SpellId.ToString()),
			FGridSpellContract::ValidateDefinition(Seed.Definition) == EGridSpellValidationError::None);
		TestTrue(
			*FString::Printf(TEXT("%s seed presentation validates"), *SpellId.ToString()),
			Seed.Presentation.IsValid());

		UGridSpellDefinitionAsset* IdentityA =
			NewObject<UGridSpellDefinitionAsset>(GetTransientPackage(), NAME_None);
		UGridSpellDefinitionAsset* IdentityB =
			NewObject<UGridSpellDefinitionAsset>(GetTransientPackage(), NAME_None);
		IdentityA->Definition.SpellId = SpellId;
		IdentityB->Definition.SpellId = SpellId;

		const FPrimaryAssetId ExpectedPrimaryId(
			FPrimaryAssetType(TEXT("GridSpell")),
			SpellId);
		TestTrue(
			*FString::Printf(TEXT("%s PrimaryAssetId is stable"), *SpellId.ToString()),
			IdentityA->GetPrimaryAssetId() == ExpectedPrimaryId);
		TestTrue(
			*FString::Printf(TEXT("%s PrimaryAssetId is independent from UObject identity"), *SpellId.ToString()),
			IdentityA->GetPrimaryAssetId() == IdentityB->GetPrimaryAssetId());

		UGridSpellDefinitionAsset* Fresh =
			NewObject<UGridSpellDefinitionAsset>(GetTransientPackage());
		FString FreshError;
		if (TestTrue(
				*FString::Printf(TEXT("%s fresh asset configures from production seed"), *SpellId.ToString()),
				FGridSpellCatalogAuthoring::ConfigureSpellAsset(*Fresh, Seed, FreshError)))
		{
			TestTrue(
				*FString::Printf(TEXT("%s fresh asset has canonical mechanics"), *SpellId.ToString()),
				FGridSpellCatalogAuthoring::IsCanonicalMechanics(*Fresh, Seed));
			TestEqual(
				*FString::Printf(TEXT("%s fresh asset keeps seed DisplayName"), *SpellId.ToString()),
				Fresh->Definition.DisplayName.ToString(),
				Seed.Definition.DisplayName.ToString());
			TestEqual(
				*FString::Printf(TEXT("%s fresh asset keeps seed Description"), *SpellId.ToString()),
				Fresh->Definition.Description.ToString(),
				Seed.Definition.Description.ToString());
			TestEqual(
				*FString::Printf(TEXT("%s fresh asset keeps seed feedback duration"), *SpellId.ToString()),
				Fresh->Presentation.FeedbackDurationSeconds,
				Seed.Presentation.FeedbackDurationSeconds);
			++FreshBootstrapCount;
		}
		else if (!FreshError.IsEmpty())
		{
			AddError(FreshError);
		}

		UGridSpellDefinitionAsset* Existing =
			NewObject<UGridSpellDefinitionAsset>(GetTransientPackage());
		Existing->Definition = Seed.Definition;
		Existing->Presentation = Seed.Presentation;

		const FString AuthoredName =
			FString::Printf(TEXT("TEXT05.5 Authored Name %s"), *SpellId.ToString());
		const FString AuthoredDescription =
			FString::Printf(TEXT("TEXT05.5 Authored Description %s"), *SpellId.ToString());
		const FGridSpellPresentationProfile AuthoredPresentation =
			MakeSentinelPresentation();

		Existing->Definition.DisplayName = FText::FromString(AuthoredName);
		Existing->Definition.Description = FText::FromString(AuthoredDescription);
		Existing->Presentation = AuthoredPresentation;

		// Poison mechanics: the authoring pass must repair these fields.
		Existing->Definition.School = EGridSpellSchool::None;
		Existing->Definition.ManaCost = 99;
		Existing->Definition.ActionPointCost = 99;
		Existing->Definition.MinRangeCells = 9;
		Existing->Definition.MaxRangeCells = 9;
		Existing->Definition.TargetingPolicy = EGridCombatTargetingPolicy::Self;
		Existing->Definition.bRequiresLineOfSight = false;
		Existing->Definition.CooldownRounds = 7;
		Existing->Definition.Effects.Reset();

		FString PreserveError;
		if (TestTrue(
				*FString::Printf(TEXT("%s existing same-id asset rematerializes safely"), *SpellId.ToString()),
				FGridSpellCatalogAuthoring::ConfigureSpellAsset(*Existing, Seed, PreserveError)))
		{
			TestTrue(
				*FString::Printf(TEXT("%s existing asset mechanics are rebuilt"), *SpellId.ToString()),
				FGridSpellCatalogAuthoring::IsCanonicalMechanics(*Existing, Seed));
			TestEqual(
				*FString::Printf(TEXT("%s preserves authored DisplayName"), *SpellId.ToString()),
				Existing->Definition.DisplayName.ToString(),
				AuthoredName);
			TestEqual(
				*FString::Printf(TEXT("%s preserves authored Description"), *SpellId.ToString()),
				Existing->Definition.Description.ToString(),
				AuthoredDescription);
			TestEqual(
				*FString::Printf(TEXT("%s preserves authored presentation feedback"), *SpellId.ToString()),
				Existing->Presentation.FeedbackDurationSeconds,
				AuthoredPresentation.FeedbackDurationSeconds);
			TestFalse(
				*FString::Printf(TEXT("%s preserves authored projectile enablement"), *SpellId.ToString()),
				Existing->Presentation.Projectile.bEnabled);
			++PreservedPresentationCount;
		}
		else if (!PreserveError.IsEmpty())
		{
			AddError(PreserveError);
		}

		UGridSpellDefinitionAsset* Mismatch =
			NewObject<UGridSpellDefinitionAsset>(GetTransientPackage());
		Mismatch->Definition = Seed.Definition;
		Mismatch->Definition.SpellId = TEXT("Spell_TEXT05_5_Different");
		Mismatch->Definition.DisplayName = FText::FromString(AuthoredName);
		Mismatch->Definition.Description = FText::FromString(AuthoredDescription);
		Mismatch->Presentation = AuthoredPresentation;

		FString MismatchError;
		TestTrue(
			*FString::Printf(TEXT("%s mismatched asset rematerializes as requested SpellId"), *SpellId.ToString()),
			FGridSpellCatalogAuthoring::ConfigureSpellAsset(*Mismatch, Seed, MismatchError));
		TestEqual(
			*FString::Printf(TEXT("%s mismatched identity receives canonical SpellId"), *SpellId.ToString()),
			Mismatch->Definition.SpellId,
			SpellId);
		TestFalse(
			*FString::Printf(TEXT("%s mismatched identity does not preserve unrelated DisplayName"), *SpellId.ToString()),
			Mismatch->Definition.DisplayName.ToString() == AuthoredName);
	}

	TestEqual(TEXT("Fresh bootstrap covers all four production Spells"), FreshBootstrapCount, 4);
	TestEqual(TEXT("Same-id preservation covers all four production Spells"), PreservedPresentationCount, 4);

	// Empty authored DisplayName must not be silently replaced on rerun.
	const FGridProductionSpellAuthoringSeed& Representative = Seeds[0];
	UGridSpellDefinitionAsset* InvalidAuthored =
		NewObject<UGridSpellDefinitionAsset>(GetTransientPackage());
	InvalidAuthored->Definition = Representative.Definition;
	InvalidAuthored->Presentation = Representative.Presentation;
	InvalidAuthored->Definition.DisplayName = FText::GetEmpty();

	FString InvalidError;
	TestFalse(
		TEXT("Invalid authored DisplayName fails instead of being silently overwritten"),
		FGridSpellCatalogAuthoring::ConfigureSpellAsset(
			*InvalidAuthored,
			Representative,
			InvalidError));
	TestTrue(
		TEXT("Invalid authored presentation failure is explicit"),
		!InvalidError.IsEmpty());
	TestTrue(
		TEXT("Failed preservation still leaves authored empty DisplayName untouched"),
		InvalidAuthored->Definition.DisplayName.IsEmpty());

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT05.5][SUMMARY] Seeds=%d FreshBootstrap=%d PreservedPresentation=%d"),
		Seeds.Num(),
		FreshBootstrapCount,
		PreservedPresentationCount);

	return true;
}

#endif

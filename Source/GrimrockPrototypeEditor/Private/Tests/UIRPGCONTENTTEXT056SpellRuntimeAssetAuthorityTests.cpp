#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "Magic/GridProductionSpellLibrary.h"
#include "Magic/GridSpellDefinitionAsset.h"

namespace UIRPGCONTENTTEXT056
{
	struct FExpectedSpellAsset
	{
		FName SpellId;
		const TCHAR* ObjectPath;
	};

	const FExpectedSpellAsset ExpectedAssets[] = {
		{ FGridProductionSpellLibrary::ArcaneBoltId(),
		  TEXT("/Game/GrimrockPrototype/Core/DataAssets/Magic/Spells/DA_Spell_ArcaneBolt.DA_Spell_ArcaneBolt") },
		{ FGridProductionSpellLibrary::LesserHealId(),
		  TEXT("/Game/GrimrockPrototype/Core/DataAssets/Magic/Spells/DA_Spell_LesserHeal.DA_Spell_LesserHeal") },
		{ FGridProductionSpellLibrary::HasteId(),
		  TEXT("/Game/GrimrockPrototype/Core/DataAssets/Magic/Spells/DA_Spell_Haste.DA_Spell_Haste") },
		{ FGridProductionSpellLibrary::CurePoisonId(),
		  TEXT("/Game/GrimrockPrototype/Core/DataAssets/Magic/Spells/DA_Spell_CurePoison.DA_Spell_CurePoison") }
	};

	bool LoadProjectText(const TCHAR* RelativePath, FString& OutText)
	{
		return FFileHelper::LoadFileToString(
			OutText,
			*FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}

	const FGridSpellDefinition* FindDefinition(
		const TArray<FGridSpellDefinition>& Definitions,
		FName SpellId)
	{
		return Definitions.FindByPredicate(
			[SpellId](const FGridSpellDefinition& Definition)
			{
				return Definition.SpellId == SpellId;
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGCONTENTTEXT056SpellRuntimeAssetAuthorityTest,
	"Grimrock.UI.RPG.CONTENTTEXT05.SpellRuntimeAssetAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT056SpellRuntimeAssetAuthorityTest::RunTest(const FString&)
{
	using namespace UIRPGCONTENTTEXT056;

	TArray<FGridSpellDefinition> RuntimeDefinitions;
	FGridProductionSpellLibrary::BuildAll(RuntimeDefinitions);
	TestEqual(TEXT("Runtime facade resolves four serialized production Spells"), RuntimeDefinitions.Num(), 4);

	int32 VerifiedAssetCount = 0;
	for (const FExpectedSpellAsset& Expected : ExpectedAssets)
	{
		const UGridSpellDefinitionAsset* Asset =
			LoadObject<UGridSpellDefinitionAsset>(nullptr, Expected.ObjectPath);
		if (!TestNotNull(
				*FString::Printf(TEXT("%s serialized Spell asset loads"), *Expected.SpellId.ToString()),
				Asset))
		{
			continue;
		}

		TestEqual(
			*FString::Printf(TEXT("%s serialized asset keeps canonical SpellId"), *Expected.SpellId.ToString()),
			Asset->Definition.SpellId,
			Expected.SpellId);
		TestTrue(
			*FString::Printf(TEXT("%s serialized definition validates"), *Expected.SpellId.ToString()),
			Asset->ValidateDefinition() == EGridSpellValidationError::None);
		TestTrue(
			*FString::Printf(TEXT("%s serialized presentation validates"), *Expected.SpellId.ToString()),
			Asset->HasValidPresentation());

		const FGridSpellDefinition* Runtime =
			FindDefinition(RuntimeDefinitions, Expected.SpellId);
		if (!TestNotNull(
				*FString::Printf(TEXT("%s runtime definition resolves"), *Expected.SpellId.ToString()),
				Runtime))
		{
			continue;
		}

		TestTrue(
			*FString::Printf(TEXT("%s runtime DisplayName comes from serialized asset"), *Expected.SpellId.ToString()),
			Runtime->DisplayName.EqualTo(Asset->Definition.DisplayName));
		TestTrue(
			*FString::Printf(TEXT("%s runtime Description comes from serialized asset"), *Expected.SpellId.ToString()),
			Runtime->Description.EqualTo(Asset->Definition.Description));
		TestEqual(
			*FString::Printf(TEXT("%s runtime ManaCost comes from serialized asset"), *Expected.SpellId.ToString()),
			Runtime->ManaCost,
			Asset->Definition.ManaCost);
		TestEqual(
			*FString::Printf(TEXT("%s runtime effect count comes from serialized asset"), *Expected.SpellId.ToString()),
			Runtime->Effects.Num(),
			Asset->Definition.Effects.Num());

		FGridSpellPresentationProfile RuntimePresentation;
		if (TestTrue(
				*FString::Printf(TEXT("%s runtime presentation resolves"), *Expected.SpellId.ToString()),
				FGridProductionSpellLibrary::TryBuildPresentationProfile(
					Expected.SpellId,
					RuntimePresentation)))
		{
			TestEqual(
				*FString::Printf(TEXT("%s runtime feedback duration comes from serialized asset"), *Expected.SpellId.ToString()),
				RuntimePresentation.FeedbackDurationSeconds,
				Asset->Presentation.FeedbackDurationSeconds);
			TestEqual(
				*FString::Printf(TEXT("%s runtime projectile enablement comes from serialized asset"), *Expected.SpellId.ToString()),
				RuntimePresentation.Projectile.bEnabled,
				Asset->Presentation.Projectile.bEnabled);
		}

		++VerifiedAssetCount;
	}

	TestEqual(TEXT("All four serialized production Spell assets are runtime authority"), VerifiedAssetCount, 4);

	FString ProductionLibrarySource;
	FString PackagingConfig;
	TestTrue(
		TEXT("Production Spell library source loads for authority audit"),
		LoadProjectText(
			TEXT("Source/GrimrockPrototype/Private/Magic/GridProductionSpellLibrary.cpp"),
			ProductionLibrarySource));
	TestTrue(
		TEXT("DefaultGame.ini loads for Spell cook audit"),
		LoadProjectText(TEXT("Config/DefaultGame.ini"), PackagingConfig));

	TestTrue(
		TEXT("Runtime Spell library loads UGridSpellDefinitionAsset"),
		ProductionLibrarySource.Contains(TEXT("LoadObject<UGridSpellDefinitionAsset>")));
	TestFalse(
		TEXT("Runtime Spell library contains no authored FText::FromString content"),
		ProductionLibrarySource.Contains(TEXT("FText::FromString")));
	TestFalse(
		TEXT("Runtime Spell library contains no C++ ManaCost authoring"),
		ProductionLibrarySource.Contains(TEXT("Definition.ManaCost =")));
	TestFalse(
		TEXT("Runtime Spell library contains no C++ effect authoring helpers"),
		ProductionLibrarySource.Contains(TEXT("MakeMagnitudeEffect")) ||
		ProductionLibrarySource.Contains(TEXT("MakeStatusEffect")));
	TestTrue(
		TEXT("Spell DataAsset directory is explicitly cooked"),
		PackagingConfig.Contains(
			TEXT("+DirectoriesToAlwaysCook=(Path=\"/Game/GrimrockPrototype/Core/DataAssets/Magic/Spells\")")));

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT05.6][SUMMARY] RuntimeAssets=%d CxxSpellProse=0"),
		VerifiedAssetCount);

	return true;
}

#endif

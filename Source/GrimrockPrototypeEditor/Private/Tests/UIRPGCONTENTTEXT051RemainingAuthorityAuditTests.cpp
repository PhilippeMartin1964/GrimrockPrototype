#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "Magic/GridProductionSpellLibrary.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillCatalogAuthoring.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"

namespace UIRPGCONTENTTEXT051
{
	FString SkillObjectPath(FName SkillId)
	{
		const FString AssetName = FString::Printf(TEXT("DA_%s"), *SkillId.ToString());
		return FString::Printf(
			TEXT("%s/%s.%s"),
			FRPGSkillCatalogAuthoring::ProductionFolder(),
			*AssetName,
			*AssetName);
	}

	bool LoadProjectSource(const TCHAR* RelativePath, FString& OutSource)
	{
		return FFileHelper::LoadFileToString(
			OutSource,
			*FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}

	int32 CountOccurrences(const FString& Text, const TCHAR* Needle)
	{
		if (!Needle || !*Needle)
		{
			return 0;
		}

		int32 Count = 0;
		int32 SearchFrom = 0;
		for (;;)
		{
			const int32 Found = Text.Find(
				Needle,
				ESearchCase::CaseSensitive,
				ESearchDir::FromStart,
				SearchFrom);
			if (Found == INDEX_NONE)
			{
				break;
			}
			++Count;
			SearchFrom = Found + FCString::Strlen(Needle);
		}
		return Count;
	}
}

/**
 * UI-RPG-CONTENT-TEXT05.1 is a cross-domain characterization audit.
 *
 * It intentionally does not migrate any text authority. It identifies the
 * remaining player-facing C++ content boundaries after CONTENT-TEXT01..04 and
 * separates them from generic UI/system vocabulary and technical fallbacks.
 *
 * No package is saved and no production asset is modified.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGCONTENTTEXT051RemainingAuthorityAuditTest,
	"Grimrock.UI.RPG.CONTENTTEXT05.RemainingAuthorityAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT051RemainingAuthorityAuditTest::RunTest(const FString&)
{
	using namespace UIRPGCONTENTTEXT051;

	// ---------------------------------------------------------------------
	// Skills: 25 serialized DA_Skill_* assets exist, but the Editor catalog
	// still owns and reapplies DisplayName from C++ on every authoring pass.
	// Description is already preserved.
	// ---------------------------------------------------------------------
	TArray<FRPGCanonicalSkillDefinition> CanonicalSkills;
	FRPGSkillCatalogAuthoring::GetCanonicalDefinitions(CanonicalSkills);
	TestEqual(TEXT("Remaining-authority audit sees twenty-five canonical Skills"), CanonicalSkills.Num(), 25);

	int32 LoadedSkillCount = 0;
	int32 SerializedSkillDisplayNameCount = 0;
	int32 SkillDisplayNameOverwriteCount = 0;
	int32 SkillDescriptionPreservedCount = 0;

	for (const FRPGCanonicalSkillDefinition& Definition : CanonicalSkills)
	{
		URPGSkillAsset* Serialized =
			LoadObject<URPGSkillAsset>(nullptr, *SkillObjectPath(Definition.SkillId));
		if (!TestNotNull(
				*FString::Printf(TEXT("%s production DA_Skill loads"), *Definition.SkillId.ToString()),
				Serialized))
		{
			continue;
		}

		++LoadedSkillCount;
		TestEqual(
			*FString::Printf(TEXT("%s keeps canonical SkillId"), *Definition.SkillId.ToString()),
			Serialized->SkillId,
			Definition.SkillId);
		TestTrue(
			*FString::Printf(TEXT("%s production Skill is structurally valid"), *Definition.SkillId.ToString()),
			Serialized->IsValidDefinition());

		if (!Serialized->DisplayName.IsEmpty())
		{
			++SerializedSkillDisplayNameCount;
		}

		URPGSkillAsset* Working =
			DuplicateObject<URPGSkillAsset>(Serialized, GetTransientPackage());
		if (!TestNotNull(
				*FString::Printf(TEXT("%s transient Skill audit duplicate is created"), *Definition.SkillId.ToString()),
				Working))
		{
			continue;
		}

		const FText SentinelDisplayName = FText::FromString(
			FString::Printf(TEXT("TEXT05.1 Skill Name %s"), *Definition.SkillId.ToString()));
		const FText SentinelDescription = FText::FromString(
			FString::Printf(TEXT("TEXT05.1 Skill Description %s"), *Definition.SkillId.ToString()));
		Working->DisplayName = SentinelDisplayName;
		Working->Description = SentinelDescription;

		FRPGSkillCatalogAuthoring::ConfigureSkill(*Working, Definition);

		if (!Working->DisplayName.EqualTo(SentinelDisplayName))
		{
			++SkillDisplayNameOverwriteCount;
			UE_LOG(
				LogTemp,
				Display,
				TEXT("[UI-RPG-CONTENT-TEXT05.1][SKILL-TEXT-OVERWRITE] SkillId=%s Before=\"%s\" After=\"%s\""),
				*Definition.SkillId.ToString(),
				*SentinelDisplayName.ToString(),
				*Working->DisplayName.ToString());
		}

		if (Working->Description.EqualTo(SentinelDescription))
		{
			++SkillDescriptionPreservedCount;
		}

		TestTrue(
			*FString::Printf(TEXT("%s mechanical Skill authoring remains canonical"), *Definition.SkillId.ToString()),
			FRPGSkillCatalogAuthoring::IsCanonicalSkill(*Working, Definition));
	}

	TestEqual(TEXT("Audit loads all twenty-five production DA_Skill assets"), LoadedSkillCount, 25);
	TestEqual(TEXT("All twenty-five production Skills have serialized DisplayName"), SerializedSkillDisplayNameCount, 25);
	TestEqual(TEXT("Current Skill authoring preserves Description for all twenty-five Skills"), SkillDescriptionPreservedCount, 25);

	// ---------------------------------------------------------------------
	// Spells: TEXT05.6 keeps the production facade but resolves the four
	// serialized UGridSpellDefinitionAsset records as runtime authority.
	// ---------------------------------------------------------------------
	TArray<FGridSpellDefinition> Spells;
	FGridProductionSpellLibrary::BuildAll(Spells);
	TestEqual(TEXT("Production spell library exposes four spells"), Spells.Num(), 4);

	int32 SpellDisplayNameCount = 0;
	int32 SpellDescriptionCount = 0;
	for (const FGridSpellDefinition& Spell : Spells)
	{
		TestTrue(
			*FString::Printf(TEXT("%s production spell definition validates"), *Spell.SpellId.ToString()),
			FGridSpellContract::ValidateDefinition(Spell) == EGridSpellValidationError::None);
		if (!Spell.DisplayName.IsEmpty())
		{
			++SpellDisplayNameCount;
		}
		if (!Spell.Description.IsEmpty())
		{
			++SpellDescriptionCount;
		}

		UE_LOG(
			LogTemp,
			Display,
			TEXT("[UI-RPG-CONTENT-TEXT05.1][SPELL-SERIALIZED-AUTHORITY] SpellId=%s DisplayName=\"%s\" Description=\"%s\""),
			*Spell.SpellId.ToString(),
			*Spell.DisplayName.ToString(),
			*Spell.Description.ToString());
	}

	TestEqual(TEXT("All four production spells expose serialized DisplayName"), SpellDisplayNameCount, 4);
	TestEqual(TEXT("All four production spells expose serialized Description"), SpellDescriptionCount, 4);

	// ---------------------------------------------------------------------
	// Universal unarmed action: one built-in runtime action still owns its
	// player-facing name and description directly in C++.
	// ---------------------------------------------------------------------
	const FGridCombatActionDefinition Unarmed =
		FGridCombatActionCatalog::MakeUnarmedAttackDefinition(2);
	TestEqual(TEXT("Universal unarmed action keeps stable ActionId"), Unarmed.ActionId, FName(TEXT("Attack_Unarmed")));
	TestFalse(TEXT("Universal unarmed action has C++ DisplayName"), Unarmed.DisplayName.IsEmpty());
	TestFalse(TEXT("Universal unarmed action has C++ Description"), Unarmed.Description.IsEmpty());

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT05.1][UNARMED-CXX-AUTHORITY] ActionId=%s DisplayName=\"%s\" Description=\"%s\""),
		*Unarmed.ActionId.ToString(),
		*Unarmed.DisplayName.ToString(),
		*Unarmed.Description.ToString());

	// ---------------------------------------------------------------------
	// Static source characterization keeps the boundary explicit without
	// invoking any package-saving authoring path.
	// ---------------------------------------------------------------------
	FString SkillAuthoringSource;
	FString SpellLibrarySource;
	FString CombatCatalogSource;
	FString SkillsPageSource;

	TestTrue(
		TEXT("Skill catalog authoring source loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototypeEditor/Private/RPG/RPGSkillCatalogAuthoring.cpp"),
			SkillAuthoringSource));
	TestTrue(
		TEXT("Production spell library source loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototype/Private/Magic/GridProductionSpellLibrary.cpp"),
			SpellLibrarySource));
	TestTrue(
		TEXT("Combat action catalog source loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridCombatActionCatalog.cpp"),
			CombatCatalogSource));
	TestTrue(
		TEXT("Skills/Talents read-model source loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsPageService.cpp"),
			SkillsPageSource));

	const int32 SkillDisplayNameWriterCount = CountOccurrences(
		SkillAuthoringSource,
		TEXT("SkillAsset.DisplayName = Definition.DisplayName;"));
	const int32 SpellDisplayNameWriterCount = CountOccurrences(
		SpellLibrarySource,
		TEXT("Definition.DisplayName = FText::FromString(TEXT("));
	const int32 SpellDescriptionWriterCount = CountOccurrences(
		SpellLibrarySource,
		TEXT("Definition.Description = FText::FromString(TEXT("));
	TestEqual(TEXT("Spell C++ DisplayName writers are removed"), SpellDisplayNameWriterCount, 0);
	TestEqual(TEXT("Spell C++ Description writers are removed"), SpellDescriptionWriterCount, 0);

	const int32 UnarmedDisplayNameWriterCount = CountOccurrences(
		CombatCatalogSource,
		TEXT("Definition.DisplayName = LOCTEXT(\"UnarmedName\""));
	const int32 UnarmedDescriptionWriterCount = CountOccurrences(
		CombatCatalogSource,
		TEXT("Definition.Description = LOCTEXT(\"UnarmedDescription\""));
	const bool bRecipeUsesStableTextIds =
		SkillsPageSource.Contains(TEXT("Talent.Recipe."));
	const bool bRecipeHumanizeFallback =
		SkillsPageSource.Contains(TEXT("HumanizeId(RecipeId)"));

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT05.1][SOURCE] SkillDisplayNameWriters=%d SpellDisplayNameWriters=%d SpellDescriptionWriters=%d UnarmedDisplayNameWriters=%d UnarmedDescriptionWriters=%d RecipeStableTextIds=%d RecipeHumanizeFallback=%d"),
		SkillDisplayNameWriterCount,
		SpellDisplayNameWriterCount,
		SpellDescriptionWriterCount,
		UnarmedDisplayNameWriterCount,
		UnarmedDescriptionWriterCount,
		bRecipeUsesStableTextIds ? 1 : 0,
		bRecipeHumanizeFallback ? 1 : 0);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT05.1][SUMMARY] SkillAssets=%d SkillDisplayNameOverwrites=%d SkillDescriptionsPreserved=%d SerializedSpells=%d SpellDisplayNames=%d SpellDescriptions=%d CxxUniversalActions=1"),
		LoadedSkillCount,
		SkillDisplayNameOverwriteCount,
		SkillDescriptionPreservedCount,
		Spells.Num(),
		SpellDisplayNameCount,
		SpellDescriptionCount);

	return true;
}


/**
 * UI-RPG-CONTENT-TEXT05.2 proves that DA_Skill_* DisplayName is preserved
 * during catalog rematerialization once the stable SkillId already matches.
 *
 * The twenty-five C++ labels remain as fresh-record bootstrap until TEXT05.3.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGCONTENTTEXT052SkillTextPreservationTest,
	"Grimrock.UI.RPG.CONTENTTEXT05.SkillPreservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT052SkillTextPreservationTest::RunTest(const FString&)
{
	using namespace UIRPGCONTENTTEXT051;

	TArray<FRPGCanonicalSkillDefinition> Definitions;
	FRPGSkillCatalogAuthoring::GetCanonicalDefinitions(Definitions);
	TestEqual(TEXT("Skill preservation covers twenty-five canonical definitions"), Definitions.Num(), 25);

	int32 PreservedSkillCount = 0;
	for (const FRPGCanonicalSkillDefinition& Definition : Definitions)
	{
		URPGSkillAsset* Serialized =
			LoadObject<URPGSkillAsset>(nullptr, *SkillObjectPath(Definition.SkillId));
		if (!TestNotNull(
				*FString::Printf(TEXT("%s production DA_Skill loads for preservation"), *Definition.SkillId.ToString()),
				Serialized))
		{
			continue;
		}

		URPGSkillAsset* Working =
			DuplicateObject<URPGSkillAsset>(Serialized, GetTransientPackage());
		if (!TestNotNull(
				*FString::Printf(TEXT("%s transient preservation duplicate is created"), *Definition.SkillId.ToString()),
				Working))
		{
			continue;
		}

		const FString DisplayName =
			FString::Printf(TEXT("TEXT05.2 Skill Name %s"), *Definition.SkillId.ToString());
		const FString Description =
			FString::Printf(TEXT("TEXT05.2 Skill Description %s"), *Definition.SkillId.ToString());
		Working->DisplayName = FText::FromString(DisplayName);
		Working->Description = FText::FromString(Description);

		// Poison mechanics to prove rematerialization still occurs.
		Working->GoverningAttribute = ERPGSkillGoverningAttribute::None;
		Working->MaxRank = 99;
		Working->bAllowUntrainedChecks = !Definition.bAllowUntrainedChecks;

		FRPGSkillCatalogAuthoring::ConfigureSkill(*Working, Definition);

		TestEqual(
			*FString::Printf(TEXT("%s preserves serialized DisplayName"), *Definition.SkillId.ToString()),
			Working->DisplayName.ToString(),
			DisplayName);
		TestEqual(
			*FString::Printf(TEXT("%s preserves serialized Description"), *Definition.SkillId.ToString()),
			Working->Description.ToString(),
			Description);
		TestTrue(
			*FString::Printf(TEXT("%s rematerializes canonical mechanical fields"), *Definition.SkillId.ToString()),
			FRPGSkillCatalogAuthoring::IsCanonicalSkill(*Working, Definition));
		TestTrue(
			*FString::Printf(TEXT("%s remains structurally valid"), *Definition.SkillId.ToString()),
			Working->IsValidDefinition());

		URPGSkillAsset* Fresh = NewObject<URPGSkillAsset>(GetTransientPackage());
		FRPGSkillCatalogAuthoring::ConfigureSkill(*Fresh, Definition);
		TestEqual(
			*FString::Printf(TEXT("%s fresh record receives technical SkillId bootstrap DisplayName"), *Definition.SkillId.ToString()),
			Fresh->DisplayName.ToString(),
			Definition.SkillId.ToString());
		TestTrue(
			*FString::Printf(TEXT("%s fresh record is mechanically canonical"), *Definition.SkillId.ToString()),
			FRPGSkillCatalogAuthoring::IsCanonicalSkill(*Fresh, Definition));

		URPGSkillAsset* Mismatch =
			DuplicateObject<URPGSkillAsset>(Serialized, GetTransientPackage());
		if (TestNotNull(
				*FString::Printf(TEXT("%s mismatched identity probe is created"), *Definition.SkillId.ToString()),
				Mismatch))
		{
			Mismatch->SkillId = TEXT("Skill_TEXT05_2_Different");
			Mismatch->DisplayName = FText::FromString(DisplayName);
			FRPGSkillCatalogAuthoring::ConfigureSkill(*Mismatch, Definition);
			TestFalse(
				*FString::Printf(TEXT("%s mismatched identity does not preserve unrelated DisplayName"), *Definition.SkillId.ToString()),
				Mismatch->DisplayName.ToString() == DisplayName);
		}

		++PreservedSkillCount;
	}

	TestEqual(TEXT("Serialized DisplayName preservation covers all twenty-five production Skills"), PreservedSkillCount, 25);

	// Empty is still authored data: preservation is exact, not a non-empty-only policy.
	FRPGCanonicalSkillDefinition Representative;
	if (TestTrue(
			TEXT("Representative Skill definition resolves for empty-text preservation"),
			FRPGSkillCatalogAuthoring::TryGetCanonicalDefinition(TEXT("Skill_HeavyWeapons"), Representative)))
	{
		URPGSkillAsset* EmptyProbe = NewObject<URPGSkillAsset>(GetTransientPackage());
		EmptyProbe->SkillId = Representative.SkillId;
		EmptyProbe->DisplayName = FText::GetEmpty();
		FRPGSkillCatalogAuthoring::ConfigureSkill(*EmptyProbe, Representative);
		TestTrue(TEXT("Empty serialized DisplayName is preserved exactly"), EmptyProbe->DisplayName.IsEmpty());
		TestTrue(
			TEXT("Empty-text preservation does not prevent mechanical rematerialization"),
			FRPGSkillCatalogAuthoring::IsCanonicalSkill(*EmptyProbe, Representative));
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT05.2][SUMMARY] PreservedSkills=%d"),
		PreservedSkillCount);

	return true;
}


/**
 * UI-RPG-CONTENT-TEXT05.3 proves that the canonical Skill catalog no longer
 * owns player-facing presentation. Fresh records use SkillId only as technical
 * bootstrap; production DA_Skill_* records keep serialized presentation.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGCONTENTTEXT053SkillBootstrapOnlyTest,
	"Grimrock.UI.RPG.CONTENTTEXT05.SkillBootstrapOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT053SkillBootstrapOnlyTest::RunTest(const FString&)
{
	using namespace UIRPGCONTENTTEXT051;

	TArray<FRPGCanonicalSkillDefinition> Definitions;
	FRPGSkillCatalogAuthoring::GetCanonicalDefinitions(Definitions);
	TestEqual(TEXT("Bootstrap-only audit covers twenty-five canonical Skills"), Definitions.Num(), 25);

	int32 BootstrapCount = 0;
	int32 ProductionPresentationCount = 0;

	for (const FRPGCanonicalSkillDefinition& Definition : Definitions)
	{
		URPGSkillAsset* Fresh = NewObject<URPGSkillAsset>(GetTransientPackage());
		if (!TestNotNull(
				*FString::Printf(TEXT("%s fresh Skill bootstrap object is created"), *Definition.SkillId.ToString()),
				Fresh))
		{
			continue;
		}

		FRPGSkillCatalogAuthoring::ConfigureSkill(*Fresh, Definition);
		TestEqual(
			*FString::Printf(TEXT("%s bootstrap DisplayName is technical SkillId only"), *Definition.SkillId.ToString()),
			Fresh->DisplayName.ToString(),
			Definition.SkillId.ToString());
		TestTrue(
			*FString::Printf(TEXT("%s bootstrap remains mechanically canonical"), *Definition.SkillId.ToString()),
			FRPGSkillCatalogAuthoring::IsCanonicalSkill(*Fresh, Definition));
		++BootstrapCount;

		URPGSkillAsset* Serialized =
			LoadObject<URPGSkillAsset>(nullptr, *SkillObjectPath(Definition.SkillId));
		if (!TestNotNull(
				*FString::Printf(TEXT("%s production DA_Skill loads for presentation audit"), *Definition.SkillId.ToString()),
				Serialized))
		{
			continue;
		}

		TestFalse(
			*FString::Printf(TEXT("%s production DisplayName is authored, not SkillId bootstrap"), *Definition.SkillId.ToString()),
			Serialized->DisplayName.ToString() == Definition.SkillId.ToString());
		TestFalse(
			*FString::Printf(TEXT("%s production DisplayName is non-empty"), *Definition.SkillId.ToString()),
			Serialized->DisplayName.IsEmpty());
		++ProductionPresentationCount;
	}

	TestEqual(TEXT("Technical bootstrap covers all twenty-five Skills"), BootstrapCount, 25);
	TestEqual(TEXT("Serialized production presentation covers all twenty-five Skills"), ProductionPresentationCount, 25);

	FString SkillAuthoringSource;
	TestTrue(
		TEXT("Skill authoring source loads for final C++ text-authority check"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototypeEditor/Private/RPG/RPGSkillCatalogAuthoring.cpp"),
			SkillAuthoringSource));

	TestFalse(
		TEXT("Canonical Skill authoring no longer contains Definition.DisplayName"),
		SkillAuthoringSource.Contains(TEXT("Definition.DisplayName")));
	TestFalse(
		TEXT("Canonical Skill authoring no longer accepts a DisplayName parameter"),
		SkillAuthoringSource.Contains(TEXT("const TCHAR* DisplayName")));
	TestTrue(
		TEXT("Fresh Skill presentation is derived only from SkillId"),
		SkillAuthoringSource.Contains(TEXT("SkillAsset.DisplayName = FText::FromName(Definition.SkillId);")));

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT05.3][SUMMARY] BootstrapSkills=%d SerializedProductionSkills=%d"),
		BootstrapCount,
		ProductionPresentationCount);

	return true;
}

#endif

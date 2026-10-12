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
	// Spells: current production definitions are built directly in C++.
	// UGridSpellDefinitionAsset exists as a data-driven container, but this
	// production library remains the live text authority for the four spells.
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
			TEXT("[UI-RPG-CONTENT-TEXT05.1][SPELL-CXX-AUTHORITY] SpellId=%s DisplayName=\"%s\" Description=\"%s\""),
			*Spell.SpellId.ToString(),
			*Spell.DisplayName.ToString(),
			*Spell.Description.ToString());
	}

	TestEqual(TEXT("All four production spells have C++ DisplayName"), SpellDisplayNameCount, 4);
	TestEqual(TEXT("All four production spells have C++ Description"), SpellDescriptionCount, 4);

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
		TEXT("[UI-RPG-CONTENT-TEXT05.1][SUMMARY] SkillAssets=%d SkillDisplayNameOverwrites=%d SkillDescriptionsPreserved=%d CxxSpells=%d CxxSpellDisplayNames=%d CxxSpellDescriptions=%d CxxUniversalActions=1"),
		LoadedSkillCount,
		SkillDisplayNameOverwriteCount,
		SkillDescriptionPreservedCount,
		Spells.Num(),
		SpellDisplayNameCount,
		SpellDescriptionCount);

	return true;
}

#endif

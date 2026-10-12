#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "RPG/RPGRangerAuthoring.h"
#include "Runtime/Monsters/GridMonsterCategoryAsset.h"

namespace UIRPGCONTENTTEXT041
{
	struct FCategoryAuditSpec
	{
		FName CategoryId = NAME_None;
	};

	void CollectProductionCategorySpecs(TArray<FCategoryAuditSpec>& OutSpecs)
	{
		OutSpecs = {
			{ TEXT("Goblin") },
			{ TEXT("Vermin") }
		};
	}

	FString CategoryObjectPath(FName CategoryId)
	{
		const FString AssetName = FString::Printf(TEXT("DA_MONCAT_%s"), *CategoryId.ToString());
		return FString::Printf(
			TEXT("%s/%s.%s"),
			UGridMonsterCategoryAsset::ProductionFolder(),
			*AssetName,
			*AssetName);
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
			const int32 Found = Text.Find(Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
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
 * UI-RPG-CONTENT-TEXT04.1 characterizes the current DA_MONCAT_* text authority.
 *
 * This is an audit only:
 * - production category assets are loaded read-only;
 * - the Ranger authoring source is inspected without invoking the package-saving
 *   AuthorProductionAssets path;
 * - runtime/category resolution is exercised read-only;
 * - no package is saved and no binary asset is modified.
 *
 * Source observations are logged rather than asserted as permanent ownership
 * rules so later TEXT04 tickets can remove the overwrite path without breaking
 * this characterization test.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGCONTENTTEXT041MonsterCategoryOwnershipAuditTest,
	"Grimrock.UI.RPG.CONTENTTEXT04.MonsterCategoryOwnershipAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT041MonsterCategoryOwnershipAuditTest::RunTest(const FString&)
{
	using namespace UIRPGCONTENTTEXT041;

	TArray<FCategoryAuditSpec> Specs;
	CollectProductionCategorySpecs(Specs);
	TestEqual(TEXT("MONCAT audit covers the two current production categories"), Specs.Num(), 2);

	int32 LoadedCategoryCount = 0;
	int32 SerializedDisplayNameCount = 0;
	int32 SerializedDescriptionCount = 0;

	for (const FCategoryAuditSpec& Spec : Specs)
	{
		const FString ObjectPath = CategoryObjectPath(Spec.CategoryId);
		UGridMonsterCategoryAsset* Category =
			LoadObject<UGridMonsterCategoryAsset>(nullptr, *ObjectPath);
		if (!TestNotNull(
				*FString::Printf(TEXT("%s production DA_MONCAT loads"), *Spec.CategoryId.ToString()),
				Category))
		{
			continue;
		}

		++LoadedCategoryCount;
		TestEqual(
			*FString::Printf(TEXT("%s keeps canonical CategoryId"), *Spec.CategoryId.ToString()),
			Category->CategoryId,
			Spec.CategoryId);
		TestTrue(
			*FString::Printf(TEXT("%s production category is structurally valid"), *Spec.CategoryId.ToString()),
			Category->IsValidDefinition());

		if (!Category->DisplayName.IsEmpty())
		{
			++SerializedDisplayNameCount;
		}
		if (!Category->Description.IsEmpty())
		{
			++SerializedDescriptionCount;
		}

		const UGridMonsterCategoryAsset* Resolved =
			UGridMonsterCategoryAsset::ResolveByCategoryId(Spec.CategoryId);
		if (TestNotNull(
				*FString::Printf(TEXT("%s resolves through canonical CategoryId lookup"), *Spec.CategoryId.ToString()),
				Resolved))
		{
			TestEqual(
				*FString::Printf(TEXT("%s runtime resolution uses serialized DisplayName"), *Spec.CategoryId.ToString()),
				Resolved->DisplayName.ToString(),
				Category->DisplayName.ToString());
		}

		UE_LOG(
			LogTemp,
			Display,
			TEXT("[UI-RPG-CONTENT-TEXT04.1][MONCAT] CategoryId=%s DisplayName=\"%s\" Description=\"%s\" ObjectPath=%s"),
			*Category->CategoryId.ToString(),
			*Category->DisplayName.ToString(),
			*Category->Description.ToString(),
			*ObjectPath);
	}

	TestEqual(TEXT("Audit loads both current production DA_MONCAT assets"), LoadedCategoryCount, 2);
	TestEqual(TEXT("Both production categories have serialized DisplayName"), SerializedDisplayNameCount, 2);

	TestTrue(
		TEXT("Unknown monster category has no canonical production presentation fallback"),
		UGridMonsterCategoryAsset::ResolveByCategoryId(TEXT("TEXT04_1_MissingCategory")) == nullptr);

	TArray<FRPGRangerFavoredEnemyCategoryDefinition> FavoredEnemyCategories;
	FString CollectError;
	if (TestTrue(
			TEXT("Ranger production discovery resolves categories through the bestiary authority"),
			FRPGRangerAuthoring::CollectProductionFavoredEnemyCategories(FavoredEnemyCategories, CollectError)))
	{
		TestEqual(
			TEXT("Favored Enemy production discovery sees the same two categories"),
			FavoredEnemyCategories.Num(),
			2);

		for (const FCategoryAuditSpec& Spec : Specs)
		{
			const FRPGRangerFavoredEnemyCategoryDefinition* Projection =
				FavoredEnemyCategories.FindByPredicate(
					[&Spec](const FRPGRangerFavoredEnemyCategoryDefinition& Candidate)
					{
						return Candidate.CategoryId == Spec.CategoryId;
					});
			if (!TestNotNull(
					*FString::Printf(TEXT("%s is projected to Ranger authoring"), *Spec.CategoryId.ToString()),
					Projection))
			{
				continue;
			}

			const UGridMonsterCategoryAsset* Category =
				UGridMonsterCategoryAsset::ResolveByCategoryId(Spec.CategoryId);
			if (TestNotNull(
					*FString::Printf(TEXT("%s canonical category resolves for projection comparison"), *Spec.CategoryId.ToString()),
					Category))
			{
				TestEqual(
					*FString::Printf(TEXT("%s Ranger projection uses the serialized bestiary DisplayName"), *Spec.CategoryId.ToString()),
					Projection->DisplayName.ToString(),
					Category->DisplayName.ToString());
			}
		}
	}
	else if (!CollectError.IsEmpty())
	{
		AddError(CollectError);
	}

	FString RangerAuthoringSource;
	const FString RangerSourcePath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("Source/GrimrockPrototypeEditor/Private/RPG/RPGRangerAuthoring.cpp"));
	TestTrue(
		TEXT("Ranger authoring source loads for overwrite-path characterization"),
		FFileHelper::LoadFileToString(RangerAuthoringSource, *RangerSourcePath));

	FString SkillsPageSource;
	const FString SkillsSourcePath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsPageService.cpp"));
	TestTrue(
		TEXT("Skills read-model source loads for runtime-fallback characterization"),
		FFileHelper::LoadFileToString(SkillsPageSource, *SkillsSourcePath));

	const int32 PlayerDisplayNameWrites = CountOccurrences(
		RangerAuthoringSource,
		TEXT("CategoryAsset.DisplayName = FText::FromString("));
	const int32 TechnicalDisplayNameWrites = CountOccurrences(
		RangerAuthoringSource,
		TEXT("CategoryAsset.DisplayName = FText::FromName(CategoryId);"));
	const int32 CategoryDescriptionWrites = CountOccurrences(
		RangerAuthoringSource,
		TEXT("Category->Description ="));
	const bool bHasProductionSpecTable =
		RangerAuthoringSource.Contains(TEXT("ProductionCategorySpecs[]"));
	const bool bHasPlayerCategoryLiterals =
		RangerAuthoringSource.Contains(TEXT("Gobelins")) ||
		RangerAuthoringSource.Contains(TEXT("Vermine"));
	const bool bHasCategorySavePath =
		RangerAuthoringSource.Contains(TEXT("SaveAuthoredAsset(Category"));
	const bool bReadModelResolvesCategoryAsset =
		SkillsPageSource.Contains(TEXT("UGridMonsterCategoryAsset::ResolveByCategoryId(CategoryId)"));
	const bool bReadModelFallsBackToEmpty =
		SkillsPageSource.Contains(TEXT("return FText::GetEmpty();"));

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT04.1][AUTHORING] ProductionSpecTable=%d PlayerDisplayNameWrites=%d TechnicalDisplayNameWrites=%d DescriptionWrites=%d SavesCategoryAsset=%d PlayerCategoryLiterals=%d"),
		bHasProductionSpecTable ? 1 : 0,
		PlayerDisplayNameWrites,
		TechnicalDisplayNameWrites,
		CategoryDescriptionWrites,
		bHasCategorySavePath ? 1 : 0,
		bHasPlayerCategoryLiterals ? 1 : 0);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT04.1][RUNTIME] ResolveByCategoryId=%d EmptyFallback=%d"),
		bReadModelResolvesCategoryAsset ? 1 : 0,
		bReadModelFallsBackToEmpty ? 1 : 0);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT04.1][SUMMARY] Categories=%d SerializedDisplayNames=%d SerializedDescriptions=%d PlayerDisplayNameWrites=%d TechnicalDisplayNameWrites=%d DescriptionOverwritePaths=%d"),
		LoadedCategoryCount,
		SerializedDisplayNameCount,
		SerializedDescriptionCount,
		PlayerDisplayNameWrites,
		TechnicalDisplayNameWrites,
		CategoryDescriptionWrites);

	return true;
}


/**
 * UI-RPG-CONTENT-TEXT04.2 proves that Ranger category rematerialization keeps
 * serialized DA_MONCAT DisplayName for an existing stable CategoryId.
 *
 * Description is not written by the Ranger authoring path and is checked as an
 * invariant rather than introducing a redundant snapshot/restore layer.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGCONTENTTEXT042MonsterCategoryTextPreservationTest,
	"Grimrock.UI.RPG.CONTENTTEXT04.Preservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT042MonsterCategoryTextPreservationTest::RunTest(const FString&)
{
	using namespace UIRPGCONTENTTEXT041;

	TArray<FCategoryAuditSpec> Specs;
	CollectProductionCategorySpecs(Specs);

	int32 PreservedCategoryCount = 0;
	for (const FCategoryAuditSpec& Spec : Specs)
	{
		UGridMonsterCategoryAsset* Serialized =
			LoadObject<UGridMonsterCategoryAsset>(nullptr, *CategoryObjectPath(Spec.CategoryId));
		if (!TestNotNull(
				*FString::Printf(TEXT("%s serialized DA_MONCAT loads for preservation test"), *Spec.CategoryId.ToString()),
				Serialized))
		{
			continue;
		}

		UGridMonsterCategoryAsset* Working =
			DuplicateObject<UGridMonsterCategoryAsset>(Serialized, GetTransientPackage());
		if (!TestNotNull(
				*FString::Printf(TEXT("%s preservation duplicate is created"), *Spec.CategoryId.ToString()),
				Working))
		{
			continue;
		}

		const FString DisplayName =
			FString::Printf(TEXT("TEXT04.2 Category Name %s"), *Spec.CategoryId.ToString());
		const FString Description =
			FString::Printf(TEXT("TEXT04.2 Category Description %s"), *Spec.CategoryId.ToString());
		Working->DisplayName = FText::FromString(DisplayName);
		Working->Description = FText::FromString(Description);

		if (!TestTrue(
				*FString::Printf(TEXT("%s existing category rematerializes"), *Spec.CategoryId.ToString()),
				FRPGRangerAuthoring::ConfigureMonsterCategory(*Working, Spec.CategoryId)))
		{
			continue;
		}

		TestEqual(
			*FString::Printf(TEXT("%s preserves serialized DisplayName"), *Spec.CategoryId.ToString()),
			Working->DisplayName.ToString(),
			DisplayName);
		TestEqual(
			*FString::Printf(TEXT("%s leaves serialized Description untouched"), *Spec.CategoryId.ToString()),
			Working->Description.ToString(),
			Description);
		TestEqual(
			*FString::Printf(TEXT("%s keeps canonical CategoryId"), *Spec.CategoryId.ToString()),
			Working->CategoryId,
			Spec.CategoryId);
		TestTrue(
			*FString::Printf(TEXT("%s remains structurally valid after text restoration"), *Spec.CategoryId.ToString()),
			Working->IsValidDefinition());

		UGridMonsterCategoryAsset* Fresh =
			NewObject<UGridMonsterCategoryAsset>(GetTransientPackage());
		if (TestNotNull(
				*FString::Printf(TEXT("%s fresh category bootstrap object is created"), *Spec.CategoryId.ToString()),
				Fresh))
		{
			TestTrue(
				*FString::Printf(TEXT("%s fresh category still receives a valid bootstrap"), *Spec.CategoryId.ToString()),
				FRPGRangerAuthoring::ConfigureMonsterCategory(*Fresh, Spec.CategoryId));
			TestEqual(
				*FString::Printf(TEXT("%s fresh category keeps canonical CategoryId"), *Spec.CategoryId.ToString()),
				Fresh->CategoryId,
				Spec.CategoryId);
			TestFalse(
				*FString::Printf(TEXT("%s fresh category bootstrap DisplayName is non-empty"), *Spec.CategoryId.ToString()),
				Fresh->DisplayName.IsEmpty());
		}

		UGridMonsterCategoryAsset* Mismatch =
			DuplicateObject<UGridMonsterCategoryAsset>(Serialized, GetTransientPackage());
		if (TestNotNull(
				*FString::Printf(TEXT("%s mismatched identity probe is created"), *Spec.CategoryId.ToString()),
				Mismatch))
		{
			Mismatch->CategoryId = TEXT("TEXT04_2_DifferentCategory");
			Mismatch->DisplayName = FText::FromString(DisplayName);
			TestTrue(
				*FString::Printf(TEXT("%s mismatched category rematerializes as requested identity"), *Spec.CategoryId.ToString()),
				FRPGRangerAuthoring::ConfigureMonsterCategory(*Mismatch, Spec.CategoryId));
			TestFalse(
				*FString::Printf(TEXT("%s mismatched identity does not preserve unrelated DisplayName"), *Spec.CategoryId.ToString()),
				Mismatch->DisplayName.ToString() == DisplayName);
		}

		++PreservedCategoryCount;
	}

	TestEqual(TEXT("DisplayName preservation covers both current production categories"), PreservedCategoryCount, 2);

	UGridMonsterCategoryAsset* DynamicCategory =
		NewObject<UGridMonsterCategoryAsset>(GetTransientPackage());
	TestTrue(
		TEXT("A new bestiary CategoryId can bootstrap without a C++ presentation registry"),
		FRPGRangerAuthoring::ConfigureMonsterCategory(*DynamicCategory, TEXT("TEXT04_2_NewCategory")));
	TestEqual(
		TEXT("Dynamic category bootstrap uses the stable CategoryId as technical DisplayName"),
		DynamicCategory->DisplayName.ToString(),
		FString(TEXT("TEXT04_2_NewCategory")));

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT04.2][SUMMARY] PreservedCategories=%d"),
		PreservedCategoryCount);

	return true;
}


/**
 * UI-RPG-CONTENT-TEXT04.3 proves that fresh monster-category records receive
 * technical text only and that production Ranger authoring no longer contains
 * a category presentation registry.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGCONTENTTEXT043MonsterCategoryBootstrapOnlyTest,
	"Grimrock.UI.RPG.CONTENTTEXT04.BootstrapOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT043MonsterCategoryBootstrapOnlyTest::RunTest(const FString&)
{
	using namespace UIRPGCONTENTTEXT041;

	TArray<FCategoryAuditSpec> Specs;
	CollectProductionCategorySpecs(Specs);

	int32 BootstrapCategoryCount = 0;
	for (const FCategoryAuditSpec& Spec : Specs)
	{
		UGridMonsterCategoryAsset* Fresh =
			NewObject<UGridMonsterCategoryAsset>(GetTransientPackage());
		if (!TestNotNull(
				*FString::Printf(TEXT("%s fresh category object is created"), *Spec.CategoryId.ToString()),
				Fresh))
		{
			continue;
		}

		if (!TestTrue(
				*FString::Printf(TEXT("%s fresh category bootstraps without player-facing C++ text"), *Spec.CategoryId.ToString()),
				FRPGRangerAuthoring::ConfigureMonsterCategory(*Fresh, Spec.CategoryId)))
		{
			continue;
		}

		TestEqual(
			*FString::Printf(TEXT("%s bootstrap keeps canonical CategoryId"), *Spec.CategoryId.ToString()),
			Fresh->CategoryId,
			Spec.CategoryId);
		TestEqual(
			*FString::Printf(TEXT("%s bootstrap DisplayName is technical CategoryId only"), *Spec.CategoryId.ToString()),
			Fresh->DisplayName.ToString(),
			Spec.CategoryId.ToString());
		TestTrue(
			*FString::Printf(TEXT("%s bootstrap category remains structurally valid"), *Spec.CategoryId.ToString()),
			Fresh->IsValidDefinition());

		++BootstrapCategoryCount;
	}

	TestEqual(TEXT("Bootstrap-only audit covers both current production categories"), BootstrapCategoryCount, 2);

	UGridMonsterCategoryAsset* Dynamic =
		NewObject<UGridMonsterCategoryAsset>(GetTransientPackage());
	TestTrue(
		TEXT("An arbitrary non-empty CategoryId can bootstrap without a C++ category registry"),
		FRPGRangerAuthoring::ConfigureMonsterCategory(*Dynamic, TEXT("TEXT04_3_DynamicCategory")));
	TestEqual(
		TEXT("Dynamic category uses its stable id as technical DisplayName"),
		Dynamic->DisplayName.ToString(),
		FString(TEXT("TEXT04_3_DynamicCategory")));

	UGridMonsterCategoryAsset* Invalid =
		NewObject<UGridMonsterCategoryAsset>(GetTransientPackage());
	TestFalse(
		TEXT("None remains invalid as a monster CategoryId"),
		FRPGRangerAuthoring::ConfigureMonsterCategory(*Invalid, NAME_None));

	FString RangerAuthoringSource;
	const FString RangerSourcePath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("Source/GrimrockPrototypeEditor/Private/RPG/RPGRangerAuthoring.cpp"));
	TestTrue(
		TEXT("Ranger source loads for bootstrap-only audit"),
		FFileHelper::LoadFileToString(RangerAuthoringSource, *RangerSourcePath));
	TestFalse(
		TEXT("Ranger production authoring no longer has a category presentation registry"),
		RangerAuthoringSource.Contains(TEXT("ProductionCategorySpecs")));
	TestFalse(
		TEXT("Ranger production authoring no longer contains Gobelins player text"),
		RangerAuthoringSource.Contains(TEXT("Gobelins")));
	TestFalse(
		TEXT("Ranger production authoring no longer contains Vermine player text"),
		RangerAuthoringSource.Contains(TEXT("Vermine")));
	TestTrue(
		TEXT("Ranger category bootstrap is derived from CategoryId"),
		RangerAuthoringSource.Contains(TEXT("CategoryAsset.DisplayName = FText::FromName(CategoryId);")));

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT04.3][SUMMARY] BootstrapCategories=%d DynamicCategoryBootstrap=1"),
		BootstrapCategoryCount);

	return true;
}

#endif

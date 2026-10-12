#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "Magic/GridProductionSpellLibrary.h"
#include "Magic/GridSpellDefinitionAsset.h"

namespace UIRPGCONTENTTEXT054
{
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
 * UI-RPG-CONTENT-TEXT05.4 characterizes the production Spell materialization
 * gap before any DataAsset migration.
 *
 * Audit only:
 * - no package save;
 * - no runtime ownership change;
 * - no .uasset creation;
 * - source/runtime paths are measured so a later migration can preserve the
 *   existing FGridProductionSpellLibrary consumer surface.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGCONTENTTEXT054SpellMaterializationAuditTest,
	"Grimrock.UI.RPG.CONTENTTEXT05.SpellMaterializationAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGCONTENTTEXT054SpellMaterializationAuditTest::RunTest(const FString&)
{
	using namespace UIRPGCONTENTTEXT054;

	TArray<FGridSpellDefinition> Definitions;
	FGridProductionSpellLibrary::BuildAll(Definitions);
	TestEqual(TEXT("Spell materialization audit sees four production definitions"), Definitions.Num(), 4);

	const FName ExpectedSpellIds[] = {
		FGridProductionSpellLibrary::ArcaneBoltId(),
		FGridProductionSpellLibrary::LesserHealId(),
		FGridProductionSpellLibrary::HasteId(),
		FGridProductionSpellLibrary::CurePoisonId()
	};

	TSet<FName> SeenSpellIds;
	int32 ValidDefinitionCount = 0;
	int32 ValidTransientAssetCount = 0;
	for (const FGridSpellDefinition& Definition : Definitions)
	{
		TestFalse(TEXT("Production SpellId is not None"), Definition.SpellId.IsNone());
		TestFalse(
			*FString::Printf(TEXT("%s production SpellId is unique"), *Definition.SpellId.ToString()),
			SeenSpellIds.Contains(Definition.SpellId));
		SeenSpellIds.Add(Definition.SpellId);

		if (TestTrue(
				*FString::Printf(TEXT("%s production definition validates"), *Definition.SpellId.ToString()),
				FGridSpellContract::ValidateDefinition(Definition) == EGridSpellValidationError::None))
		{
			++ValidDefinitionCount;
		}

		TestFalse(
			*FString::Printf(TEXT("%s production DisplayName is non-empty"), *Definition.SpellId.ToString()),
			Definition.DisplayName.IsEmpty());
		TestFalse(
			*FString::Printf(TEXT("%s production Description is non-empty"), *Definition.SpellId.ToString()),
			Definition.Description.IsEmpty());

		UGridSpellDefinitionAsset* TransientAsset =
			NewObject<UGridSpellDefinitionAsset>(GetTransientPackage());
		if (!TestNotNull(
				*FString::Printf(TEXT("%s transient UGridSpellDefinitionAsset is created"), *Definition.SpellId.ToString()),
				TransientAsset))
		{
			continue;
		}

		TransientAsset->Definition = Definition;
		FGridSpellPresentationProfile Presentation;
		if (FGridProductionSpellLibrary::TryBuildPresentationProfile(Definition.SpellId, Presentation))
		{
			TransientAsset->Presentation = Presentation;
		}

		if (TestTrue(
				*FString::Printf(TEXT("%s definition fits existing DataAsset contract"), *Definition.SpellId.ToString()),
				TransientAsset->ValidateDefinition() == EGridSpellValidationError::None) &&
			TestTrue(
				*FString::Printf(TEXT("%s presentation fits existing DataAsset contract"), *Definition.SpellId.ToString()),
				TransientAsset->HasValidPresentation()))
		{
			++ValidTransientAssetCount;
		}
	}

	TestEqual(TEXT("All four production spell definitions validate"), ValidDefinitionCount, 4);
	TestEqual(TEXT("All four production spells fit UGridSpellDefinitionAsset without a new data model"), ValidTransientAssetCount, 4);

	for (const FName ExpectedSpellId : ExpectedSpellIds)
	{
		TestTrue(
			*FString::Printf(TEXT("%s remains part of the current production Spell set"), *ExpectedSpellId.ToString()),
			SeenSpellIds.Contains(ExpectedSpellId));
	}

	// The repository currently has no DA_Spell_* production assets. Keep this
	// count informational so the audit remains useful once materialization starts.
	TArray<FString> SpellAssetFiles;
	IFileManager::Get().FindFilesRecursive(
		SpellAssetFiles,
		*FPaths::ProjectContentDir(),
		TEXT("DA_Spell_*.uasset"),
		true,
		false,
		false);

	FString ProductionLibrarySource;
	FString SpellAssetHeaderSource;
	FString SpellbookUISource;
	FString SpellbookPersistenceSource;
	FString PartySpellbookSource;
	FString TurnManagerSource;
	FString SpellbookWidgetSource;

	TestTrue(
		TEXT("Production Spell library source loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototype/Private/Magic/GridProductionSpellLibrary.cpp"),
			ProductionLibrarySource));
	TestTrue(
		TEXT("Spell DataAsset header source loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototype/Public/Magic/GridSpellDefinitionAsset.h"),
			SpellAssetHeaderSource));
	TestTrue(
		TEXT("Spellbook UI source loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototype/Private/Magic/GridSpellbookUI.cpp"),
			SpellbookUISource));
	TestTrue(
		TEXT("Spellbook persistence source loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototype/Private/Magic/GridSpellbookPersistence.cpp"),
			SpellbookPersistenceSource));
	TestTrue(
		TEXT("Party Spellbook source loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototype/Private/Magic/GridPartySpellbookComponent.cpp"),
			PartySpellbookSource));
	TestTrue(
		TEXT("TurnManager Spell source loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerPlayerActionCatalog.cpp"),
			TurnManagerSource));
	TestTrue(
		TEXT("Spellbook widget source loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototype/Private/UI/GridSpellbookWidget.cpp"),
			SpellbookWidgetSource));

	const int32 CxxDisplayNameWriterCount = CountOccurrences(
		ProductionLibrarySource,
		TEXT("Definition.DisplayName = FText::FromString(TEXT("));
	const int32 CxxDescriptionWriterCount = CountOccurrences(
		ProductionLibrarySource,
		TEXT("Definition.Description = FText::FromString(TEXT("));

	int32 BuildAllRuntimeConsumerCount = 0;
	BuildAllRuntimeConsumerCount += CountOccurrences(SpellbookUISource, TEXT("FGridProductionSpellLibrary::BuildAll("));
	BuildAllRuntimeConsumerCount += CountOccurrences(SpellbookPersistenceSource, TEXT("FGridProductionSpellLibrary::BuildAll("));
	BuildAllRuntimeConsumerCount += CountOccurrences(PartySpellbookSource, TEXT("FGridProductionSpellLibrary::BuildAll("));
	BuildAllRuntimeConsumerCount += CountOccurrences(TurnManagerSource, TEXT("FGridProductionSpellLibrary::BuildAll("));
	BuildAllRuntimeConsumerCount += CountOccurrences(SpellbookWidgetSource, TEXT("FGridProductionSpellLibrary::BuildAll("));

	const int32 PresentationRuntimeConsumerCount =
		CountOccurrences(TurnManagerSource, TEXT("FGridProductionSpellLibrary::TryBuildPresentationProfile("));

	const bool bSpellAssetHasPrimaryAssetOverride =
		SpellAssetHeaderSource.Contains(TEXT("GetPrimaryAssetId() const override"));
	const bool bRuntimeConsumersLoadSpellAssets =
		SpellbookUISource.Contains(TEXT("UGridSpellDefinitionAsset")) ||
		SpellbookPersistenceSource.Contains(TEXT("UGridSpellDefinitionAsset")) ||
		PartySpellbookSource.Contains(TEXT("UGridSpellDefinitionAsset")) ||
		TurnManagerSource.Contains(TEXT("UGridSpellDefinitionAsset")) ||
		SpellbookWidgetSource.Contains(TEXT("UGridSpellDefinitionAsset"));

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT05.4][MATERIALIZATION] DA_Spell_Files=%d DataAssetPrimaryIdOverride=%d ValidTransientAssets=%d"),
		SpellAssetFiles.Num(),
		bSpellAssetHasPrimaryAssetOverride ? 1 : 0,
		ValidTransientAssetCount);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT05.4][OWNERSHIP] CxxDisplayNames=%d CxxDescriptions=%d BuildAllRuntimeConsumers=%d PresentationRuntimeConsumers=%d RuntimeLoadsSpellAssets=%d"),
		CxxDisplayNameWriterCount,
		CxxDescriptionWriterCount,
		BuildAllRuntimeConsumerCount,
		PresentationRuntimeConsumerCount,
		bRuntimeConsumersLoadSpellAssets ? 1 : 0);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT05.4][SUMMARY] ProductionSpells=%d ValidDefinitions=%d DA_Spell_Files=%d BuildAllRuntimeConsumers=%d"),
		Definitions.Num(),
		ValidDefinitionCount,
		SpellAssetFiles.Num(),
		BuildAllRuntimeConsumerCount);

	return true;
}

#endif

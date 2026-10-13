#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/RPGTalentDetailTextSet.h"

namespace UILOCALIZATION011
{
	const TCHAR* TalentDetailTextSetPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_RPGTalentDetailTextSet.DA_RPGTalentDetailTextSet");

	FString FormatEntryIndices(const TArray<int32>& Indices)
	{
		TArray<FString> IndexStrings;
		IndexStrings.Reserve(Indices.Num());
		for (const int32 Index : Indices)
		{
			IndexStrings.Add(FString::FromInt(Index));
		}
		return FString::Join(IndexStrings, TEXT(", "));
	}
}

/**
 * UI-LOCALIZATION01.1
 *
 * Production integrity guard for the Talent text authority.
 *
 * The test is intentionally independent from the current entry count. It
 * rejects empty definitions and duplicate TextIds, and reports the exact
 * Entries[] indices so the Data Asset can be corrected without guesswork.
 *
 * No package is saved and no production asset is modified.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUILOCALIZATION011TalentTextSetIntegrityTest,
	"Grimrock.UI.Localization.LOCALIZATION01.TalentTextSetIntegrity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUILOCALIZATION011TalentTextSetIntegrityTest::RunTest(const FString&)
{
	using namespace UILOCALIZATION011;

	const URPGTalentDetailTextSet* TextSet =
		LoadObject<URPGTalentDetailTextSet>(nullptr, TalentDetailTextSetPath);
	if (!TestNotNull(TEXT("Production Talent detail text set loads"), TextSet))
	{
		return false;
	}

	TestTrue(TEXT("Production Talent detail text set is not empty"), !TextSet->Entries.IsEmpty());

	int32 InvalidEntryCount = 0;
	TMap<FName, TArray<int32>> IndicesByTextId;

	for (int32 Index = 0; Index < TextSet->Entries.Num(); ++Index)
	{
		const FRPGTalentDetailTextEntry& Entry = TextSet->Entries[Index];

		if (Entry.TextId.IsNone())
		{
			++InvalidEntryCount;
			AddError(FString::Printf(
				TEXT("DA_RPGTalentDetailTextSet Entries[%d] has an empty TextId."),
				Index));
			continue;
		}

		IndicesByTextId.FindOrAdd(Entry.TextId).Add(Index);

		if (Entry.Text.IsEmpty())
		{
			++InvalidEntryCount;
			AddError(FString::Printf(
				TEXT("DA_RPGTalentDetailTextSet Entries[%d] ('%s') has empty Text."),
				Index,
				*Entry.TextId.ToString()));
		}
	}

	TArray<FName> SortedTextIds;
	IndicesByTextId.GetKeys(SortedTextIds);
	SortedTextIds.Sort(
		[](const FName& A, const FName& B)
		{
			return A.ToString() < B.ToString();
		});

	int32 DuplicateTextIdCount = 0;
	for (const FName TextId : SortedTextIds)
	{
		const TArray<int32>& Indices = IndicesByTextId.FindChecked(TextId);
		if (Indices.Num() <= 1)
		{
			continue;
		}

		++DuplicateTextIdCount;
		AddError(FString::Printf(
			TEXT("DA_RPGTalentDetailTextSet duplicate TextId '%s' at Entries[%s]."),
			*TextId.ToString(),
			*FormatEntryIndices(Indices)));
	}

	TestEqual(TEXT("Production Talent detail text set has no invalid entries"), InvalidEntryCount, 0);
	TestEqual(TEXT("Production Talent detail text set has no duplicate TextIds"), DuplicateTextIdCount, 0);
	TestTrue(TEXT("Production Talent detail text set satisfies its runtime definition contract"), TextSet->IsValidDefinition());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

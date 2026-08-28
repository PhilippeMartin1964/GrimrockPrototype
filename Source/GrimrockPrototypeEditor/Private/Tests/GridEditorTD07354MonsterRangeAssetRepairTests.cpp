#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/PackageName.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace GridTD07354RangeRepair
{
	struct FTarget
	{
		const TCHAR* Label;
		const TCHAR* AssetPath;
	};

	const FTarget Targets[] = {
		{ TEXT("RatGiant"), TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.DA_MON_RatGiant") },
		{ TEXT("GoblinThrower"), TEXT("/Game/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower.DA_MON_GoblinThrower") }
	};

	bool SaveAssetPackage(UGridMonsterDefinitionAsset* Asset)
	{
		if (!IsValid(Asset))
		{
			return false;
		}

		UPackage* Package = Asset->GetOutermost();
		if (!Package)
		{
			return false;
		}

		const FString Filename =
			FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		SaveArgs.Error = GError;
		return UPackage::SavePackage(Package, Asset, *Filename, SaveArgs);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07354MonsterRangeAssetRepairTest,
	"Grimrock.TechnicalDebt.TD07_3_5_4.AssetRepair.MonsterRangeAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07354MonsterRangeAssetRepairTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07354RangeRepair;

	int32 LoadedCount = 0;
	int32 ChangedAssetCount = 0;

	for (const FTarget& Target : Targets)
	{
		UGridMonsterDefinitionAsset* Definition =
			LoadObject<UGridMonsterDefinitionAsset>(nullptr, Target.AssetPath);
		TestNotNull(*FString::Printf(TEXT("%s definition loads"), Target.Label), Definition);
		if (!Definition)
		{
			continue;
		}
		++LoadedCount;

		bool bChanged = false;
		for (FGridMonsterAttackDefinition& Attack : Definition->Attacks)
		{
			const int32 SerializedMaximum = Attack.RangeCells;
			TestTrue(*FString::Printf(TEXT("%s attack %s has a legal serialized range"),
				Target.Label, *Attack.AttackId.ToString()), SerializedMaximum >= Attack.MinRangeCells);

			if (Attack.MaxRangeCells != SerializedMaximum)
			{
				Definition->Modify();
				Attack.MaxRangeCells = SerializedMaximum;
				bChanged = true;
			}

			TestEqual(*FString::Printf(TEXT("%s attack %s copies RangeCells to MaxRangeCells"),
				Target.Label, *Attack.AttackId.ToString()), Attack.MaxRangeCells, SerializedMaximum);
			TestTrue(*FString::Printf(TEXT("%s attack %s target range is valid"),
				Target.Label, *Attack.AttackId.ToString()), Attack.MaxRangeCells >= Attack.MinRangeCells);
		}

		TestTrue(*FString::Printf(TEXT("%s remains a valid monster definition"), Target.Label),
			Definition->IsValidDefinition());

		if (bChanged)
		{
			Definition->MarkPackageDirty();
			TestTrue(*FString::Printf(TEXT("%s repaired package saves"), Target.Label),
				SaveAssetPackage(Definition));
			++ChangedAssetCount;
		}
	}

	TestEqual(TEXT("Both monster DataAssets load"), LoadedCount, 2);
	AddInfo(ChangedAssetCount > 0
		? FString::Printf(TEXT("TD07.3.5.4 copied serialized maximum range into MaxRangeCells for %d monster asset(s)."), ChangedAssetCount)
		: TEXT("TD07.3.5.4 monster range assets were already current; no package mutation was required."));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Engine/DataAsset.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

namespace GridTD0737Characterization
{
	struct FCandidate
	{
		FString Code;
		FString AssetPath;
		FString Context;
		FString Detail;
	};

	void AddCandidate(TArray<FCandidate>& Candidates, const TCHAR* Code, const FString& AssetPath, const FString& Context, const FString& Detail)
	{
		FCandidate& Candidate = Candidates.AddDefaulted_GetRef();
		Candidate.Code = Code;
		Candidate.AssetPath = AssetPath;
		Candidate.Context = Context;
		Candidate.Detail = Detail;
	}

	void AuditDefinitionPair(const FString& AssetPath, const FString& Context, const UObject* DefinitionObject, FName DefinitionId, FName StoredId,
		const TCHAR* Domain, TArray<FCandidate>& Candidates)
	{
		if (DefinitionObject && DefinitionId.IsNone())
		{
			AddCandidate(Candidates, TEXT("AUTHORING.DEFINITION_WITHOUT_ID"), AssetPath, Context,
				FString::Printf(TEXT("%s definition asset '%s' has no canonical id."), Domain, *DefinitionObject->GetPathName()));
		}

		if (StoredId.IsNone())
		{
			return;
		}

		if (!DefinitionObject)
		{
			AddCandidate(Candidates, TEXT("AUTHORING.ID_ONLY"), AssetPath, Context,
				FString::Printf(TEXT("%s is authored only through id '%s'; current schema requires the definition asset reference."),
					Domain, *StoredId.ToString()));
			return;
		}

		if (DefinitionId != StoredId)
		{
			AddCandidate(Candidates, TEXT("AUTHORING.ASSET_ID_CONFLICT"), AssetPath, Context,
				FString::Printf(TEXT("%s asset id '%s' conflicts with stored id '%s'."),
					Domain, *DefinitionId.ToString(), *StoredId.ToString()));
			return;
		}

		AddCandidate(Candidates, TEXT("AUTHORING.ASSET_ID_DUPLICATE"), AssetPath, Context,
			FString::Printf(TEXT("%s stores both asset reference and mirrored id '%s'."),
				Domain, *StoredId.ToString()));
	}

	void AuditBehavior(const FString& AssetPath, const FString& Context, const FGridObjectBehaviorParams& Behavior, TArray<FCandidate>& Candidates)
	{
		const UGridItemDefinitionAsset* ItemDefinition = Behavior.Item.ItemDefinitionAsset.Get();
		AuditDefinitionPair(AssetPath, Context + TEXT(".Item"), ItemDefinition,
			ItemDefinition ? ItemDefinition->ItemDefinitionId : NAME_None,
			Behavior.Item.ItemDefinitionId, TEXT("Item"), Candidates);

		const UGridReadableContentAsset* ReadableDefinition = Behavior.Item.DefaultReadableContentAsset.Get();
		AuditDefinitionPair(AssetPath, Context + TEXT(".DefaultReadableContent"), ReadableDefinition,
			ReadableDefinition ? ReadableDefinition->ReadableContentId : NAME_None,
			Behavior.Item.DefaultReadableContentId, TEXT("ReadableContent"), Candidates);

		if (!Behavior.Lock.AcceptedKeyIds.IsEmpty())
		{
			AddCandidate(Candidates, TEXT("AUTHORING.LOCK_KEY_IDS"), AssetPath, Context + TEXT(".Lock"),
				FString::Printf(TEXT("AcceptedKeyIds contains %d id(s); current schema keeps AcceptedKeyItems only."),
					Behavior.Lock.AcceptedKeyIds.Num()));
		}
	}

	void AuditDataAsset(const UDataAsset& DataAsset, TArray<FCandidate>& Candidates)
	{
		const FString AssetPath = DataAsset.GetPathName();

		if (const UGridLevelAsset* Level = Cast<UGridLevelAsset>(&DataAsset))
		{
			for (int32 ObjectIndex = 0; ObjectIndex < Level->Objects.Num(); ++ObjectIndex)
			{
				const FGridLevelObjectData& Object = Level->Objects[ObjectIndex];
				const FString Context = FString::Printf(TEXT("Objects[%d] ObjectId=%s"),
					ObjectIndex, *Object.ObjectId.ToString(EGuidFormats::Digits));

				const UGridItemDefinitionAsset* ItemDefinition = Object.ItemDefinitionAsset.Get();
				AuditDefinitionPair(AssetPath, Context + TEXT(".ItemDefinition"), ItemDefinition,
					ItemDefinition ? ItemDefinition->ItemDefinitionId : NAME_None,
					Object.ItemDefinitionId, TEXT("Item"), Candidates);

				const UGridReadableContentAsset* ReadableDefinition = Object.ReadableContentAsset.Get();
				AuditDefinitionPair(AssetPath, Context + TEXT(".ReadableContent"), ReadableDefinition,
					ReadableDefinition ? ReadableDefinition->ReadableContentId : NAME_None,
					Object.ReadableContentId, TEXT("ReadableContent"), Candidates);

				const UGridMonsterDefinitionAsset* MonsterDefinition = Object.MonsterDefinitionAsset.Get();
				AuditDefinitionPair(AssetPath, Context + TEXT(".MonsterDefinition"), MonsterDefinition,
					MonsterDefinition ? MonsterDefinition->MonsterId : NAME_None,
					Object.MonsterDefinitionId, TEXT("Monster"), Candidates);

				AuditBehavior(AssetPath, Context + TEXT(".Behavior"), Object.Behavior, Candidates);
			}
			return;
		}

		if (const UGridObjectArchetypeAsset* Archetype = Cast<UGridObjectArchetypeAsset>(&DataAsset))
		{
			AuditBehavior(AssetPath, TEXT("DefaultBehavior"), Archetype->DefaultBehavior, Candidates);
			return;
		}

		if (const UGridItemDefinitionAsset* Item = Cast<UGridItemDefinitionAsset>(&DataAsset))
		{
			if (!Item->HasValidCombatActions())
			{
				AddCandidate(Candidates, TEXT("ITEM.INVALID_COMBAT_ACTIONS"), AssetPath, TEXT("Equipment|CombatActions"),
					TEXT("Item contains an invalid or duplicate CombatActions definition."));
			}
			return;
		}

		if (const UGridMonsterDefinitionAsset* Monster = Cast<UGridMonsterDefinitionAsset>(&DataAsset))
		{
			for (int32 LootIndex = 0; LootIndex < Monster->LootTable.Num(); ++LootIndex)
			{
				const FGridMonsterLootEntry& Loot = Monster->LootTable[LootIndex];
				const UGridItemDefinitionAsset* ItemDefinition = Loot.ItemDefinitionAsset.Get();
				AuditDefinitionPair(AssetPath, FString::Printf(TEXT("LootTable[%d]"), LootIndex), ItemDefinition,
					ItemDefinition ? ItemDefinition->ItemDefinitionId : NAME_None,
					Loot.ItemDefinitionId, TEXT("LootItem"), Candidates);
			}
		}
	}

	FString BuildReport(int32 ScannedAssets, const TArray<FCandidate>& Candidates)
	{
		TArray<FString> Lines;
		Lines.Add(TEXT("GrimrockPrototype TD07.3.7 - Current Asset Repair / Recreation Characterization"));
		Lines.Add(TEXT("Policy: current prototype assets only; no backward migration."));
		Lines.Add(FString::Printf(TEXT("Scanned DataAssets: %d"), ScannedAssets));
		Lines.Add(FString::Printf(TEXT("Repair candidates: %d"), Candidates.Num()));
		Lines.Add(TEXT(""));

		TMap<FString, int32> CodeCounts;
		for (const FCandidate& Candidate : Candidates)
		{
			CodeCounts.FindOrAdd(Candidate.Code)++;
		}

		Lines.Add(TEXT("Candidate codes:"));
		TArray<FString> Codes;
		CodeCounts.GetKeys(Codes);
		Codes.Sort();
		for (const FString& Code : Codes)
		{
			Lines.Add(FString::Printf(TEXT("  %-44s %d"), *Code, CodeCounts.FindRef(Code)));
		}

		Lines.Add(TEXT(""));
		Lines.Add(TEXT("Details:"));
		for (const FCandidate& Candidate : Candidates)
		{
			Lines.Add(FString::Printf(TEXT("%s | %s | %s | %s"),
				*Candidate.Code, *Candidate.AssetPath, *Candidate.Context, *Candidate.Detail));
		}

		return FString::Join(Lines, TEXT("\n"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0737CurrentAssetRepairCharacterizationTest,
	"Grimrock.TechnicalDebt.TD07_3_7.Characterization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0737CurrentAssetRepairCharacterizationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0737Characterization;

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game")));
	Filter.ClassPaths.Add(UDataAsset::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);
	Assets.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.PackageName.LexicalLess(Right.PackageName);
	});

	int32 LoadedAssets = 0;
	TArray<FCandidate> Candidates;
	for (const FAssetData& Entry : Assets)
	{
		UDataAsset* DataAsset = Cast<UDataAsset>(Entry.GetAsset());
		TestNotNull(*FString::Printf(TEXT("%s loads"), *Entry.PackageName.ToString()), DataAsset);
		if (!DataAsset)
		{
			continue;
		}

		++LoadedAssets;
		AuditDataAsset(*DataAsset, Candidates);
	}

	TestEqual(TEXT("Every discovered DataAsset can be loaded"), LoadedAssets, Assets.Num());

	const FString ReportDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Diagnostics"), TEXT("TD07"));
	const FString ReportPath = FPaths::Combine(ReportDirectory, TEXT("TD07_3_7_CurrentAssetRepairCandidates.txt"));
	IFileManager::Get().MakeDirectory(*ReportDirectory, true);
	const FString Report = BuildReport(LoadedAssets, Candidates);
	TestTrue(TEXT("TD07.3.7 characterization report is written"),
		FFileHelper::SaveStringToFile(Report, *ReportPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));

	AddInfo(FString::Printf(TEXT("TD07.3.7 scanned %d DataAssets and found %d current-asset repair candidate(s)."),
		LoadedAssets, Candidates.Num()));
	AddInfo(FString::Printf(TEXT("TD07.3.7 report: %s"), *ReportPath));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridTypedPlacementAuditTestUtils.h"
#include "GridEditorTD073ApprovedCanonicalLockAcceptance.h"
#include "GridEditorTD073ApprovedCanonicalStoneLoot.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/DataAsset.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalInventoryCategoryTags.h"
#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

namespace GridTD0737Normalization
{
	struct FCandidate
	{
		FString Code;
		FString AssetPath;
		FString Context;
	};

	void AddCandidate(TArray<FCandidate>& Candidates, const TCHAR* Code, const FString& AssetPath, const FString& Context)
	{
		FCandidate& Candidate = Candidates.AddDefaulted_GetRef();
		Candidate.Code = Code;
		Candidate.AssetPath = AssetPath;
		Candidate.Context = Context;
	}

	void AuditPair(
		const FString& AssetPath, const FString& Context, const UObject* DefinitionObject, FName CanonicalId, FName StoredId, TArray<FCandidate>& Candidates)
	{
		if (DefinitionObject && CanonicalId.IsNone())
		{
			AddCandidate(Candidates, TEXT("AUTHORING.DEFINITION_WITHOUT_ID"), AssetPath, Context);
		}

		if (StoredId.IsNone())
		{
			return;
		}

		if (!DefinitionObject)
		{
			AddCandidate(Candidates, TEXT("AUTHORING.ID_ONLY"), AssetPath, Context);
			return;
		}

		AddCandidate(Candidates, CanonicalId == StoredId ? TEXT("AUTHORING.ASSET_ID_DUPLICATE") : TEXT("AUTHORING.ASSET_ID_CONFLICT"), AssetPath, Context);
	}

	void AuditBehavior(const FString& AssetPath, const FString& Context, const FGridObjectBehaviorParams& Behavior, TArray<FCandidate>& Candidates)
	{
		const UGridReadableContentAsset* Readable = Behavior.Item.DefaultReadableContentAsset.Get();
		AuditPair(AssetPath, Context + TEXT(".DefaultReadableContent"), Readable, Readable ? Readable->ReadableContentId : NAME_None,
			Behavior.Item.DefaultReadableContentId, Candidates);

		if (!GridTD073ApprovedCanonicalLockAcceptance::IsHistoricalOrApproved(AssetPath, Behavior.Lock))
		{
			AddCandidate(Candidates, TEXT("AUTHORING.LOCK_KEY_IDS"), AssetPath, Context + TEXT(".Lock"));
		}
	}

	void AuditDataAsset(const UDataAsset& DataAsset, TArray<FCandidate>& Candidates)
	{
		const FString AssetPath = DataAsset.GetPathName();

		if (const UGridLevelAsset* Level = Cast<UGridLevelAsset>(&DataAsset))
		{
			GridTypedPlacementAuditTestUtils::AuditDefinitionReferences(*Level,
				[&](const FString& Context, const UObject* Definition, FName CanonicalId, FName StoredId, const TCHAR*)
				{
					AuditPair(AssetPath, Context, Definition, CanonicalId, StoredId, Candidates);
				});
			return;
		}

		if (const UGridWorldObjectDefinitionAsset* Definition = Cast<UGridWorldObjectDefinitionAsset>(&DataAsset))
		{
			AuditBehavior(AssetPath, TEXT("DefaultBehavior"), Definition->DefaultBehavior, Candidates);
			return;
		}

		if (const UGridItemDefinitionAsset* Item = Cast<UGridItemDefinitionAsset>(&DataAsset))
		{
			if (!Item->HasValidCombatActions())
			{
				AddCandidate(Candidates, TEXT("ITEM.INVALID_COMBAT_ACTIONS"), AssetPath, TEXT("CombatActions"));
			}
			return;
		}

		if (const UGridMonsterDefinitionAsset* Monster = Cast<UGridMonsterDefinitionAsset>(&DataAsset))
		{
			for (int32 LootIndex = 0; LootIndex < Monster->LootTable.Num(); ++LootIndex)
			{
				const FGridMonsterLootEntry& Loot = Monster->LootTable[LootIndex];
				if (GridTD073ApprovedCanonicalStoneLoot::IsExact(AssetPath, LootIndex, Loot)) continue;
				AuditPair(AssetPath, FString::Printf(TEXT("LootTable[%d]"), LootIndex), nullptr, NAME_None, Loot.ItemDefinitionId,
					Candidates);
			}
		}
	}

	void GatherCandidates(FAutomationTestBase& Test, TArray<FCandidate>& OutCandidates, int32& OutLoadedAssets)
	{
		OutCandidates.Reset();
		OutLoadedAssets = 0;

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
		AssetRegistry.SearchAllAssets(true);

		FARFilter Filter;
		Filter.PackagePaths.Add(FName(TEXT("/Game")));
		Filter.ClassPaths.Add(UDataAsset::StaticClass()->GetClassPathName());
		Filter.bRecursivePaths = true;
		Filter.bRecursiveClasses = true;

		TArray<FAssetData> Assets;
		AssetRegistry.GetAssets(Filter, Assets);
		Assets.Sort(
			[](const FAssetData& Left, const FAssetData& Right)
			{
				return Left.PackageName.LexicalLess(Right.PackageName);
			});

		for (const FAssetData& Entry : Assets)
		{
			UDataAsset* DataAsset = Cast<UDataAsset>(Entry.GetAsset());
			Test.TestNotNull(*FString::Printf(TEXT("%s loads"), *Entry.PackageName.ToString()), DataAsset);
			if (!DataAsset)
			{
				continue;
			}

			++OutLoadedAssets;
			AuditDataAsset(*DataAsset, OutCandidates);
		}
	}

	const FGridMonsterLootEntry* FindLootEntry(const UGridMonsterDefinitionAsset* Monster, FName ItemId)
	{
		if (!Monster)
		{
			return nullptr;
		}

		return Monster->LootTable.FindByPredicate(
			[ItemId](const FGridMonsterLootEntry& Entry)
			{
				return Entry.ItemDefinitionId == ItemId;
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0737CurrentAssetsNormalizedTest, "Grimrock.TechnicalDebt.TD07_3_7.Normalization.CurrentAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0737CurrentAssetsNormalizedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0737Normalization;

	TArray<FCandidate> Candidates;
	int32 LoadedAssets = 0;
	GatherCandidates(*this, Candidates, LoadedAssets);

	for (const FCandidate& Candidate : Candidates)
	{
		AddError(FString::Printf(TEXT("%s | %s | %s"), *Candidate.Code, *Candidate.AssetPath, *Candidate.Context));
	}

	AddInfo(FString::Printf(TEXT("TD07.3.7 normalization scanned %d DataAssets."), LoadedAssets));
	TestEqual(TEXT("Current schema contains zero TD07.3.7 repair candidates"), Candidates.Num(), 0);
	return Candidates.IsEmpty();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0737RatLootDefinitionsNormalizedTest, "Grimrock.TechnicalDebt.TD07_3_7.Normalization.RatLootDefinitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0737RatLootDefinitionsNormalizedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0737Normalization;

	// H.21: rat loot is now authorized by DA_ItemSystem. Do not require the
	// retired DA_Item_RatMeat/DA_Item_RatTooth in a production test.
	UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem"));
	UGridMonsterDefinitionAsset* Rat = LoadObject<UGridMonsterDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.DA_MON_RatGiant"));
	if (!TestNotNull(TEXT("Canonical item registry exists"), Root) ||
		!TestNotNull(TEXT("Rat loot authoring exists"), Rat))
	{
		return false;
	}
	const UGridCanonicalItemDefinitionAsset* RatMeat = nullptr;
	const UGridCanonicalItemDefinitionAsset* RatTooth = nullptr;
	for (const TObjectPtr<UGridCanonicalItemDefinitionAsset>& Definition : Root->ItemDefinitions)
	{
		if (!IsValid(Definition)) continue;
		if (Definition->ItemId == FName(TEXT("Item_RatMeat"))) RatMeat = Definition;
		if (Definition->ItemId == FName(TEXT("Item_RatTooth"))) RatTooth = Definition;
	}
	if (!TestNotNull(TEXT("Canonical rat meat exists"), RatMeat) ||
		!TestNotNull(TEXT("Canonical rat tooth exists"), RatTooth))
	{
		return false;
	}
	TestTrue(TEXT("Rat meat retains the canonical consumable class"),
		RatMeat->IsA<UGridConsumableDefinitionAsset>());
	TestTrue(TEXT("Rat meat retains Food category"),
		RatMeat->Tags.Contains(GridCanonicalInventoryCategoryTags::Food()));
	TestTrue(TEXT("Rat meat validates canonical identity"), RatMeat->HasValidIdentity());
	TestTrue(TEXT("Rat tooth retains the canonical stackable class"),
		RatTooth->IsA<UGridGenericStackableDefinitionAsset>());
	TestTrue(TEXT("Rat tooth retains Component category"),
		RatTooth->Tags.Contains(GridCanonicalInventoryCategoryTags::Component()));
	TestTrue(TEXT("Rat tooth validates canonical identity"), RatTooth->HasValidIdentity());

	const FGridMonsterLootEntry* MeatLoot = FindLootEntry(Rat, TEXT("Item_RatMeat"));
	const FGridMonsterLootEntry* ToothLoot = FindLootEntry(Rat, TEXT("Item_RatTooth"));
	if (!TestNotNull(TEXT("Rat Giant references rat meat loot"), MeatLoot) || !TestNotNull(TEXT("Rat Giant references rat tooth loot"), ToothLoot))
	{
		return false;
	}

	const bool bAfter = GridTD073ApprovedCanonicalStoneLoot::IsExact(Rat->GetPathName(), 0, *MeatLoot) &&
		GridTD073ApprovedCanonicalStoneLoot::IsExact(Rat->GetPathName(), 1, *ToothLoot);
	TestTrue(TEXT("Both rat loot entries are in one exact native authoring state"), bAfter);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0737LockAuthoringNormalizedTest, "Grimrock.TechnicalDebt.TD07_3_7.Normalization.LockAuthoring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0737LockAuthoringNormalizedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridWorldObjectDefinitionAsset* CopperLock =
		LoadObject<UGridWorldObjectDefinitionAsset>(nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall.DA_Lock_CopperWall"));
	UGridWorldObjectDefinitionAsset* IronLock =
		LoadObject<UGridWorldObjectDefinitionAsset>(nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall.DA_Lock_IronWall"));

	if (!TestNotNull(TEXT("Copper wall lock exists"), CopperLock) || !TestNotNull(TEXT("Iron wall lock exists"), IronLock))
	{
		return false;
	}

	// X.52: real Copper/Iron shared definitions must accept only their
	// exact canonical ID after native cutover; the old field no longer exists.
	const auto Check = [this](const TCHAR* Label,
		const UGridWorldObjectDefinitionAsset* LockAsset, FName Expected) -> bool
	{
		const FGridLockBehaviorParams& Rule = LockAsset->DefaultBehavior.Lock;
		if (!TestTrue(*FString::Printf(TEXT("%s exact native accepted keys"), Label),
			GridTD073ApprovedCanonicalLockAcceptance::IsExactApprovedPair(
				LockAsset->GetPathName(), Rule)) ||
			!TestEqual(*FString::Printf(TEXT("%s exactly one canonical key"), Label),
				Rule.AcceptedKeyIds.Num(), 1) ||
			!TestEqual(*FString::Printf(TEXT("%s accepted ID unchanged"), Label),
				Rule.AcceptedKeyIds[0], Expected))
		{
			return false;
		}
		return true;
	};
	return Check(TEXT("Copper"), CopperLock, TEXT("Key_Copper")) &&
		Check(TEXT("Iron"), IronLock, TEXT("Key_Iron"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0737MonsterSpawnAuthoringAuthorityTest, "Grimrock.TechnicalDebt.TD07_3_7.Normalization.MonsterSpawnAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0737MonsterSpawnAuthoringAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridLevelAsset* Level =
		LoadObject<UGridLevelAsset>(nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00"));
	if (!TestNotNull(TEXT("DA_GridLevel_00 exists"), Level))
	{
		return false;
	}

	int32 MonsterSpawnCount = 0;
	for (const FGridMonsterSpawnInstance& Object : Level->MonsterSpawns)
	{
		++MonsterSpawnCount;
		TestNotNull(*FString::Printf(TEXT("MonsterSpawn %s has definition asset"), *Object.SpawnId.ToString()), Object.MonsterDefinition.Get());
	}
	TestNull(TEXT("Typed monster placement has no mirrored definition id"), FGridMonsterSpawnInstance::StaticStruct()->FindPropertyByName(TEXT("MonsterDefinitionId")));
	TestTrue(TEXT("Production level contains MonsterSpawn authoring coverage"), MonsterSpawnCount > 0);

	FString LevelSource;
	const FString LevelSourcePath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/GrimrockPrototype/Private/Core/GridLevelAsset.cpp"));
	TestTrue(TEXT("GridLevelAsset source can be read"), FFileHelper::LoadFileToString(LevelSource, *LevelSourcePath));
	TestFalse(TEXT("GridLevelAsset no longer repopulates MonsterDefinitionId from MonsterDefinitionAsset"),
		LevelSource.Contains(TEXT("ObjectData.MonsterDefinitionId = ObjectData.MonsterDefinitionAsset->MonsterId")));

	FString EditorSource;
	const FString EditorSourcePath = FPaths::Combine(FPaths::ProjectDir(),
		TEXT(
			"Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEditorActorParts/EditingObjectsLinks/GridLevelEditorActor_EditingObjectsLinks_02.inl"));
	TestTrue(TEXT("Grid editor placement source can be read"), FFileHelper::LoadFileToString(EditorSource, *EditorSourcePath));
	TestFalse(TEXT("Grid editor no longer writes MonsterDefinitionId mirror during placement"),
		EditorSource.Contains(TEXT("NewObject.MonsterDefinitionId = NewObject.MonsterDefinitionAsset->MonsterId")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

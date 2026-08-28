#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Engine/DataAsset.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Runtime/GridItemDefinitionAsset.h"
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

	void AuditPair(const FString& AssetPath, const FString& Context, const UObject* DefinitionObject, FName CanonicalId, FName StoredId,
		TArray<FCandidate>& Candidates)
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

		AddCandidate(
			Candidates,
			CanonicalId == StoredId ? TEXT("AUTHORING.ASSET_ID_DUPLICATE") : TEXT("AUTHORING.ASSET_ID_CONFLICT"),
			AssetPath,
			Context);
	}

	void AuditBehavior(const FString& AssetPath, const FString& Context, const FGridObjectBehaviorParams& Behavior, TArray<FCandidate>& Candidates)
	{
		const UGridItemDefinitionAsset* Item = Behavior.Item.ItemDefinitionAsset.Get();
		AuditPair(AssetPath, Context + TEXT(".Item"), Item, Item ? Item->ItemDefinitionId : NAME_None, Behavior.Item.ItemDefinitionId, Candidates);

		const UGridReadableContentAsset* Readable = Behavior.Item.DefaultReadableContentAsset.Get();
		AuditPair(
			AssetPath,
			Context + TEXT(".DefaultReadableContent"),
			Readable,
			Readable ? Readable->ReadableContentId : NAME_None,
			Behavior.Item.DefaultReadableContentId,
			Candidates);

		if (!Behavior.Lock.AcceptedKeyIds.IsEmpty())
		{
			AddCandidate(Candidates, TEXT("AUTHORING.LOCK_KEY_IDS"), AssetPath, Context + TEXT(".Lock"));
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
				const FString Context = FString::Printf(TEXT("Objects[%d]"), ObjectIndex);

				const UGridItemDefinitionAsset* Item = Object.ItemDefinitionAsset.Get();
				AuditPair(AssetPath, Context + TEXT(".ItemDefinition"), Item, Item ? Item->ItemDefinitionId : NAME_None, Object.ItemDefinitionId, Candidates);

				const UGridReadableContentAsset* Readable = Object.ReadableContentAsset.Get();
				AuditPair(
					AssetPath,
					Context + TEXT(".ReadableContent"),
					Readable,
					Readable ? Readable->ReadableContentId : NAME_None,
					Object.ReadableContentId,
					Candidates);

				const UGridMonsterDefinitionAsset* Monster = Object.MonsterDefinitionAsset.Get();
				AuditPair(
					AssetPath,
					Context + TEXT(".MonsterDefinition"),
					Monster,
					Monster ? Monster->MonsterId : NAME_None,
					Object.MonsterDefinitionId,
					Candidates);

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
				AddCandidate(Candidates, TEXT("ITEM.INVALID_COMBAT_ACTIONS"), AssetPath, TEXT("CombatActions"));
			}
			return;
		}

		if (const UGridMonsterDefinitionAsset* Monster = Cast<UGridMonsterDefinitionAsset>(&DataAsset))
		{
			for (int32 LootIndex = 0; LootIndex < Monster->LootTable.Num(); ++LootIndex)
			{
				const FGridMonsterLootEntry& Loot = Monster->LootTable[LootIndex];
				const UGridItemDefinitionAsset* Item = Loot.ItemDefinitionAsset.Get();
				AuditPair(
					AssetPath,
					FString::Printf(TEXT("LootTable[%d]"), LootIndex),
					Item,
					Item ? Item->ItemDefinitionId : NAME_None,
					Loot.ItemDefinitionId,
					Candidates);
			}
		}
	}

	void GatherCandidates(FAutomationTestBase& Test, TArray<FCandidate>& OutCandidates, int32& OutLoadedAssets)
	{
		OutCandidates.Reset();
		OutLoadedAssets = 0;

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
				const UGridItemDefinitionAsset* ItemDefinition = Entry.ItemDefinitionAsset.Get();
				const FName ResolvedItemId =
					(ItemDefinition && !ItemDefinition->ItemDefinitionId.IsNone())
						? ItemDefinition->ItemDefinitionId
						: Entry.ItemDefinitionId;
				return ResolvedItemId == ItemId;
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0737CurrentAssetsNormalizedTest,
	"Grimrock.TechnicalDebt.TD07_3_7.Normalization.CurrentAssets",
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0737RatLootDefinitionsNormalizedTest,
	"Grimrock.TechnicalDebt.TD07_3_7.Normalization.RatLootDefinitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0737RatLootDefinitionsNormalizedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0737Normalization;

	UGridItemDefinitionAsset* RatMeat = LoadObject<UGridItemDefinitionAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_RatMeat.DA_Item_RatMeat"));
	UGridItemDefinitionAsset* RatTooth = LoadObject<UGridItemDefinitionAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_RatTooth.DA_Item_RatTooth"));
	UGridMonsterDefinitionAsset* Rat = LoadObject<UGridMonsterDefinitionAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.DA_MON_RatGiant"));

	if (!TestNotNull(TEXT("DA_Item_RatMeat exists"), RatMeat) ||
		!TestNotNull(TEXT("DA_Item_RatTooth exists"), RatTooth) ||
		!TestNotNull(TEXT("DA_MON_RatGiant exists"), Rat))
	{
		return false;
	}

	TestEqual(TEXT("Rat meat id"), RatMeat->ItemDefinitionId, FName(TEXT("Item_RatMeat")));
	TestEqual(TEXT("Rat meat type"), RatMeat->ItemType, EGridItemType::Food);
	TestTrue(TEXT("Rat meat definition validates"), RatMeat->IsValidDefinition());

	TestEqual(TEXT("Rat tooth id"), RatTooth->ItemDefinitionId, FName(TEXT("Item_RatTooth")));
	TestEqual(TEXT("Rat tooth type"), RatTooth->ItemType, EGridItemType::Component);
	TestTrue(TEXT("Rat tooth definition validates"), RatTooth->IsValidDefinition());

	const FGridMonsterLootEntry* MeatLoot = FindLootEntry(Rat, TEXT("Item_RatMeat"));
	const FGridMonsterLootEntry* ToothLoot = FindLootEntry(Rat, TEXT("Item_RatTooth"));
	if (!TestNotNull(TEXT("Rat Giant references rat meat loot"), MeatLoot) ||
		!TestNotNull(TEXT("Rat Giant references rat tooth loot"), ToothLoot))
	{
		return false;
	}

	TestEqual(TEXT("Rat meat loot uses the current asset"), MeatLoot->ItemDefinitionAsset.Get(), RatMeat);
	TestTrue(TEXT("Rat meat loot mirror id is empty"), MeatLoot->ItemDefinitionId.IsNone());
	TestEqual(TEXT("Rat tooth loot uses the current asset"), ToothLoot->ItemDefinitionAsset.Get(), RatTooth);
	TestTrue(TEXT("Rat tooth loot mirror id is empty"), ToothLoot->ItemDefinitionId.IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0737LockAuthoringNormalizedTest,
	"Grimrock.TechnicalDebt.TD07_3_7.Normalization.LockAuthoring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0737LockAuthoringNormalizedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridObjectArchetypeAsset* CopperLock = LoadObject<UGridObjectArchetypeAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall.DA_Lock_CopperWall"));
	UGridObjectArchetypeAsset* IronLock = LoadObject<UGridObjectArchetypeAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall.DA_Lock_IronWall"));

	if (!TestNotNull(TEXT("Copper wall lock exists"), CopperLock) ||
		!TestNotNull(TEXT("Iron wall lock exists"), IronLock))
	{
		return false;
	}

	TestTrue(TEXT("Copper lock legacy key ids are empty"), CopperLock->DefaultBehavior.Lock.AcceptedKeyIds.IsEmpty());
	TestEqual(TEXT("Copper lock has one accepted key asset"), CopperLock->DefaultBehavior.Lock.AcceptedKeyItems.Num(), 1);
	if (CopperLock->DefaultBehavior.Lock.AcceptedKeyItems.Num() == 1 && CopperLock->DefaultBehavior.Lock.AcceptedKeyItems[0])
	{
		TestEqual(
			TEXT("Copper lock references Key_Copper"),
			CopperLock->DefaultBehavior.Lock.AcceptedKeyItems[0]->ItemDefinitionId,
			FName(TEXT("Key_Copper")));
	}

	TestTrue(TEXT("Iron lock legacy key ids are empty"), IronLock->DefaultBehavior.Lock.AcceptedKeyIds.IsEmpty());
	TestEqual(TEXT("Iron lock has one accepted key asset"), IronLock->DefaultBehavior.Lock.AcceptedKeyItems.Num(), 1);
	if (IronLock->DefaultBehavior.Lock.AcceptedKeyItems.Num() == 1 && IronLock->DefaultBehavior.Lock.AcceptedKeyItems[0])
	{
		TestEqual(
			TEXT("Iron lock references Key_Iron"),
			IronLock->DefaultBehavior.Lock.AcceptedKeyItems[0]->ItemDefinitionId,
			FName(TEXT("Key_Iron")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0737MonsterSpawnAuthoringAuthorityTest,
	"Grimrock.TechnicalDebt.TD07_3_7.Normalization.MonsterSpawnAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0737MonsterSpawnAuthoringAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridLevelAsset* Level = LoadObject<UGridLevelAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00"));
	if (!TestNotNull(TEXT("DA_GridLevel_00 exists"), Level))
	{
		return false;
	}

	int32 MonsterSpawnCount = 0;
	for (const FGridLevelObjectData& Object : Level->Objects)
	{
		if (Object.Type != EGridLevelObjectType::MonsterSpawn)
		{
			continue;
		}

		++MonsterSpawnCount;
		TestNotNull(*FString::Printf(TEXT("MonsterSpawn %s has definition asset"), *Object.ObjectId.ToString()), Object.MonsterDefinitionAsset.Get());
		TestTrue(*FString::Printf(TEXT("MonsterSpawn %s mirror id is empty"), *Object.ObjectId.ToString()), Object.MonsterDefinitionId.IsNone());
	}
	TestTrue(TEXT("Production level contains MonsterSpawn authoring coverage"), MonsterSpawnCount > 0);

	FString LevelSource;
	const FString LevelSourcePath =
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/GrimrockPrototype/Private/Core/GridLevelAsset.cpp"));
	TestTrue(TEXT("GridLevelAsset source can be read"), FFileHelper::LoadFileToString(LevelSource, *LevelSourcePath));
	TestFalse(
		TEXT("GridLevelAsset no longer repopulates MonsterDefinitionId from MonsterDefinitionAsset"),
		LevelSource.Contains(TEXT("ObjectData.MonsterDefinitionId = ObjectData.MonsterDefinitionAsset->MonsterId")));

	FString EditorSource;
	const FString EditorSourcePath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEditorActorParts/EditingObjectsLinks/GridLevelEditorActor_EditingObjectsLinks_02.inl"));
	TestTrue(TEXT("Grid editor placement source can be read"), FFileHelper::LoadFileToString(EditorSource, *EditorSourcePath));
	TestFalse(
		TEXT("Grid editor no longer writes MonsterDefinitionId mirror during placement"),
		EditorSource.Contains(TEXT("NewObject.MonsterDefinitionId = NewObject.MonsterDefinitionAsset->MonsterId")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

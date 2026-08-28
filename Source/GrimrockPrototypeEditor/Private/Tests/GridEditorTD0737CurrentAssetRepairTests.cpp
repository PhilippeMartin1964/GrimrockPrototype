#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Engine/DataAsset.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace GridTD0737AssetRepair
{
	FName GetCanonicalId(const UGridItemDefinitionAsset* Asset)
	{
		return Asset ? Asset->ItemDefinitionId : NAME_None;
	}

	FName GetCanonicalId(const UGridReadableContentAsset* Asset)
	{
		return Asset ? Asset->ReadableContentId : NAME_None;
	}

	FName GetCanonicalId(const UGridMonsterDefinitionAsset* Asset)
	{
		return Asset ? Asset->MonsterId : NAME_None;
	}

	bool SaveDataAsset(UDataAsset* Asset)
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

	template <typename TAsset>
	bool RegisterAssetById(FAutomationTestBase& Test, TMap<FName, TAsset*>& Registry, TAsset* Asset, FName CanonicalId, const TCHAR* Domain)
	{
		if (!Asset || CanonicalId.IsNone())
		{
			return true;
		}

		if (TAsset** Existing = Registry.Find(CanonicalId))
		{
			if (*Existing != Asset)
			{
				Test.AddError(FString::Printf(TEXT("Duplicate %s canonical id '%s': %s and %s."),
					Domain, *CanonicalId.ToString(), *GetPathNameSafe(*Existing), *GetPathNameSafe(Asset)));
				return false;
			}
			return true;
		}

		Registry.Add(CanonicalId, Asset);
		return true;
	}

	UGridItemDefinitionAsset* EnsurePlaceholderItemDefinition(
		FAutomationTestBase& Test,
		TMap<FName, UGridItemDefinitionAsset*>& ItemById,
		FName ItemId,
		const TCHAR* AssetName,
		const TCHAR* DisplayName,
		EGridItemType ItemType)
	{
		if (UGridItemDefinitionAsset** Existing = ItemById.Find(ItemId))
		{
			return *Existing;
		}

		const FString PackageName = FString::Printf(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/%s"), AssetName);
		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			Test.AddError(FString::Printf(TEXT("Could not create package %s."), *PackageName));
			return nullptr;
		}
		Package->FullyLoad();

		UGridItemDefinitionAsset* Asset = FindObject<UGridItemDefinitionAsset>(Package, AssetName);
		if (!Asset)
		{
			Asset = NewObject<UGridItemDefinitionAsset>(
				Package, UGridItemDefinitionAsset::StaticClass(), AssetName, RF_Public | RF_Standalone | RF_Transactional);
			if (!Asset)
			{
				Test.AddError(FString::Printf(TEXT("Could not create item asset %s."), AssetName));
				return nullptr;
			}
			FAssetRegistryModule::AssetCreated(Asset);
		}

		Asset->Modify();
		Asset->ItemDefinitionId = ItemId;
		Asset->DisplayName = FText::FromString(DisplayName);
		Asset->Description = FText::FromString(TEXT("Current-schema placeholder loot definition created by TD07.3.7; presentation/balance may be refined later."));
		Asset->ItemType = ItemType;
		Asset->Weight = 0.0f;
		Asset->bStackable = true;
		Asset->MaxStackSize = 99;
		Asset->MarkPackageDirty();

		Test.TestTrue(*FString::Printf(TEXT("%s is a valid item definition"), AssetName), Asset->IsValidDefinition());
		Test.TestTrue(*FString::Printf(TEXT("%s saves"), AssetName), SaveDataAsset(Asset));
		ItemById.Add(ItemId, Asset);
		return Asset;
	}

	template <typename TAsset>
	bool RepairPair(
		FAutomationTestBase& Test,
		TObjectPtr<TAsset>& AssetRef,
		FName& MirroredId,
		const TMap<FName, TAsset*>& Registry,
		const FString& Context,
		bool& bChanged)
	{
		if (AssetRef)
		{
			const FName CanonicalId = GetCanonicalId(AssetRef.Get());
			if (CanonicalId.IsNone())
			{
				Test.AddError(FString::Printf(TEXT("%s references an asset without canonical id: %s."), *Context, *GetPathNameSafe(AssetRef.Get())));
				return false;
			}

			if (!MirroredId.IsNone())
			{
				MirroredId = NAME_None;
				bChanged = true;
			}
			return true;
		}

		if (MirroredId.IsNone())
		{
			return true;
		}

		TAsset* const* Resolved = Registry.Find(MirroredId);
		if (!Resolved || !*Resolved)
		{
			Test.AddError(FString::Printf(TEXT("%s cannot resolve id-only authoring value '%s'."), *Context, *MirroredId.ToString()));
			return false;
		}

		AssetRef = *Resolved;
		MirroredId = NAME_None;
		bChanged = true;
		return true;
	}

	bool RepairBehavior(
		FAutomationTestBase& Test,
		FGridObjectBehaviorParams& Behavior,
		const TMap<FName, UGridItemDefinitionAsset*>& ItemById,
		const TMap<FName, UGridReadableContentAsset*>& ReadableById,
		const FString& Context,
		bool& bChanged)
	{
		bool bOk = true;
		bOk &= RepairPair(Test, Behavior.Item.ItemDefinitionAsset, Behavior.Item.ItemDefinitionId, ItemById, Context + TEXT(".Item"), bChanged);
		bOk &= RepairPair(
			Test,
			Behavior.Item.DefaultReadableContentAsset,
			Behavior.Item.DefaultReadableContentId,
			ReadableById,
			Context + TEXT(".DefaultReadableContent"),
			bChanged);

		if (!Behavior.Lock.AcceptedKeyIds.IsEmpty())
		{
			for (const FName KeyId : Behavior.Lock.AcceptedKeyIds)
			{
				if (KeyId.IsNone())
				{
					continue;
				}
				UGridItemDefinitionAsset* const* KeyDefinition = ItemById.Find(KeyId);
				if (!KeyDefinition || !*KeyDefinition)
				{
					Test.AddError(FString::Printf(TEXT("%s.Lock cannot resolve AcceptedKeyId '%s'."), *Context, *KeyId.ToString()));
					bOk = false;
					continue;
				}
				Behavior.Lock.AcceptedKeyItems.AddUnique(*KeyDefinition);
			}
			Behavior.Lock.AcceptedKeyIds.Reset();
			bChanged = true;
		}

		return bOk;
	}

	int32 CountResidualCandidates(const UDataAsset& DataAsset)
	{
		int32 Count = 0;
		auto CountPair = [&Count](const UObject* Asset, FName StoredId, FName CanonicalId)
		{
			if (!StoredId.IsNone())
			{
				++Count;
			}
			if (Asset && CanonicalId.IsNone())
			{
				++Count;
			}
		};

		auto CountBehavior = [&Count, &CountPair](const FGridObjectBehaviorParams& Behavior)
		{
			CountPair(
				Behavior.Item.ItemDefinitionAsset.Get(),
				Behavior.Item.ItemDefinitionId,
				GetCanonicalId(Behavior.Item.ItemDefinitionAsset.Get()));
			CountPair(
				Behavior.Item.DefaultReadableContentAsset.Get(),
				Behavior.Item.DefaultReadableContentId,
				GetCanonicalId(Behavior.Item.DefaultReadableContentAsset.Get()));
			Count += Behavior.Lock.AcceptedKeyIds.Num();
		};

		if (const UGridLevelAsset* Level = Cast<UGridLevelAsset>(&DataAsset))
		{
			for (const FGridLevelObjectData& Object : Level->Objects)
			{
				CountPair(Object.ItemDefinitionAsset.Get(), Object.ItemDefinitionId, GetCanonicalId(Object.ItemDefinitionAsset.Get()));
				CountPair(Object.ReadableContentAsset.Get(), Object.ReadableContentId, GetCanonicalId(Object.ReadableContentAsset.Get()));
				CountPair(Object.MonsterDefinitionAsset.Get(), Object.MonsterDefinitionId, GetCanonicalId(Object.MonsterDefinitionAsset.Get()));
				CountBehavior(Object.Behavior);
			}
		}
		else if (const UGridObjectArchetypeAsset* Archetype = Cast<UGridObjectArchetypeAsset>(&DataAsset))
		{
			CountBehavior(Archetype->DefaultBehavior);
		}
		else if (const UGridItemDefinitionAsset* Item = Cast<UGridItemDefinitionAsset>(&DataAsset))
		{
			if (!Item->HasValidCombatActions())
			{
				++Count;
			}
		}
		else if (const UGridMonsterDefinitionAsset* Monster = Cast<UGridMonsterDefinitionAsset>(&DataAsset))
		{
			for (const FGridMonsterLootEntry& Loot : Monster->LootTable)
			{
				CountPair(Loot.ItemDefinitionAsset.Get(), Loot.ItemDefinitionId, GetCanonicalId(Loot.ItemDefinitionAsset.Get()));
			}
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0737CurrentAssetRepairTest,
	"Grimrock.TechnicalDebt.TD07_3_7.AssetRepair",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0737CurrentAssetRepairTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0737AssetRepair;

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game")));
	Filter.ClassPaths.Add(UDataAsset::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> AssetData;
	AssetRegistry.GetAssets(Filter, AssetData);
	AssetData.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.PackageName.LexicalLess(Right.PackageName);
	});

	TArray<UDataAsset*> Assets;
	TMap<FName, UGridItemDefinitionAsset*> ItemById;
	TMap<FName, UGridReadableContentAsset*> ReadableById;
	TMap<FName, UGridMonsterDefinitionAsset*> MonsterById;

	for (const FAssetData& Entry : AssetData)
	{
		UDataAsset* DataAsset = Cast<UDataAsset>(Entry.GetAsset());
		TestNotNull(*FString::Printf(TEXT("%s loads"), *Entry.PackageName.ToString()), DataAsset);
		if (!DataAsset)
		{
			continue;
		}
		Assets.Add(DataAsset);

		if (UGridItemDefinitionAsset* Item = Cast<UGridItemDefinitionAsset>(DataAsset))
		{
			RegisterAssetById(*this, ItemById, Item, Item->ItemDefinitionId, TEXT("Item"));
		}
		else if (UGridReadableContentAsset* Readable = Cast<UGridReadableContentAsset>(DataAsset))
		{
			RegisterAssetById(*this, ReadableById, Readable, Readable->ReadableContentId, TEXT("ReadableContent"));
		}
		else if (UGridMonsterDefinitionAsset* Monster = Cast<UGridMonsterDefinitionAsset>(DataAsset))
		{
			RegisterAssetById(*this, MonsterById, Monster, Monster->MonsterId, TEXT("Monster"));
		}
	}

	UGridItemDefinitionAsset* RatMeat = EnsurePlaceholderItemDefinition(
		*this, ItemById, TEXT("Item_RatMeat"), TEXT("DA_Item_RatMeat"), TEXT("Rat Meat"), EGridItemType::Food);
	UGridItemDefinitionAsset* RatTooth = EnsurePlaceholderItemDefinition(
		*this, ItemById, TEXT("Item_RatTooth"), TEXT("DA_Item_RatTooth"), TEXT("Rat Tooth"), EGridItemType::Component);
	if (!RatMeat || !RatTooth)
	{
		return false;
	}

	int32 ModifiedAssetCount = 0;
	bool bAllRepairsValid = true;

	for (UDataAsset* DataAsset : Assets)
	{
		if (!DataAsset)
		{
			continue;
		}

		bool bChanged = false;
		bool bAssetOk = true;
		DataAsset->Modify();

		if (UGridLevelAsset* Level = Cast<UGridLevelAsset>(DataAsset))
		{
			for (int32 ObjectIndex = 0; ObjectIndex < Level->Objects.Num(); ++ObjectIndex)
			{
				FGridLevelObjectData& Object = Level->Objects[ObjectIndex];
				const FString Context = FString::Printf(TEXT("%s Objects[%d]"), *Level->GetPathName(), ObjectIndex);
				bAssetOk &= RepairPair(*this, Object.ItemDefinitionAsset, Object.ItemDefinitionId, ItemById, Context + TEXT(".ItemDefinition"), bChanged);
				bAssetOk &= RepairPair(
					*this, Object.ReadableContentAsset, Object.ReadableContentId, ReadableById, Context + TEXT(".ReadableContent"), bChanged);
				bAssetOk &= RepairPair(
					*this, Object.MonsterDefinitionAsset, Object.MonsterDefinitionId, MonsterById, Context + TEXT(".MonsterDefinition"), bChanged);
				bAssetOk &= RepairBehavior(*this, Object.Behavior, ItemById, ReadableById, Context + TEXT(".Behavior"), bChanged);
			}
		}
		else if (UGridObjectArchetypeAsset* Archetype = Cast<UGridObjectArchetypeAsset>(DataAsset))
		{
			bAssetOk &= RepairBehavior(
				*this, Archetype->DefaultBehavior, ItemById, ReadableById, Archetype->GetPathName() + TEXT(".DefaultBehavior"), bChanged);
		}
		else if (UGridItemDefinitionAsset* Item = Cast<UGridItemDefinitionAsset>(DataAsset))
		{
			if (!Item->HasValidCombatActions())
			{
				AddError(FString::Printf(TEXT("%s has invalid CombatActions and requires manual recreation."), *Item->GetPathName()));
				bAssetOk = false;
			}
		}
		else if (UGridMonsterDefinitionAsset* Monster = Cast<UGridMonsterDefinitionAsset>(DataAsset))
		{
			for (int32 LootIndex = 0; LootIndex < Monster->LootTable.Num(); ++LootIndex)
			{
				FGridMonsterLootEntry& Loot = Monster->LootTable[LootIndex];
				bAssetOk &= RepairPair(
					*this,
					Loot.ItemDefinitionAsset,
					Loot.ItemDefinitionId,
					ItemById,
					FString::Printf(TEXT("%s LootTable[%d]"), *Monster->GetPathName(), LootIndex),
					bChanged);
			}
		}

		bAllRepairsValid &= bAssetOk;
		if (bChanged)
		{
			DataAsset->MarkPackageDirty();
			TestTrue(*FString::Printf(TEXT("%s saves repaired current-schema data"), *DataAsset->GetPathName()), SaveDataAsset(DataAsset));
			++ModifiedAssetCount;
		}
	}

	int32 ResidualCandidateCount = 0;
	for (UDataAsset* DataAsset : Assets)
	{
		if (DataAsset)
		{
			ResidualCandidateCount += CountResidualCandidates(*DataAsset);
		}
	}
	ResidualCandidateCount += CountResidualCandidates(*RatMeat);
	ResidualCandidateCount += CountResidualCandidates(*RatTooth);

	TestTrue(TEXT("Every repair operation resolves against a current definition asset"), bAllRepairsValid);
	TestEqual(TEXT("No TD07.3.7 current-asset candidate remains after repair"), ResidualCandidateCount, 0);
	AddInfo(FString::Printf(TEXT("TD07.3.7 repaired %d existing DataAsset(s) and ensured 2 Rat loot definitions."), ModifiedAssetCount));
	return bAllRepairsValid && ResidualCandidateCount == 0;
}

#endif // WITH_DEV_AUTOMATION_TESTS

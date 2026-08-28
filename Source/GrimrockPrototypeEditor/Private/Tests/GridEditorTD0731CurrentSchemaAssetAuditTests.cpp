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

DEFINE_LOG_CATEGORY_STATIC(LogGridTD0731SchemaAudit, Log, All);

namespace GridTD0731SchemaAuditPrivate
{
	enum class EGridTD0731FindingKind : uint8
	{
		Conflict,
		LegacyOnly,
		DuplicateAuthority,
		LegacyField,
		SchemaRename
	};

	struct FGridTD0731Finding
	{
		EGridTD0731FindingKind Kind = EGridTD0731FindingKind::LegacyField;
		FString Code;
		FString AssetPath;
		FString Context;
		FString Detail;
	};

	const TCHAR* ToFindingKindText(EGridTD0731FindingKind Kind)
	{
		switch (Kind)
		{
			case EGridTD0731FindingKind::Conflict:
				return TEXT("Conflict");
			case EGridTD0731FindingKind::LegacyOnly:
				return TEXT("LegacyOnly");
			case EGridTD0731FindingKind::DuplicateAuthority:
				return TEXT("DuplicateAuthority");
			case EGridTD0731FindingKind::LegacyField:
				return TEXT("LegacyField");
			case EGridTD0731FindingKind::SchemaRename:
				return TEXT("SchemaRename");
			default:
				return TEXT("Unknown");
		}
	}

	void AddFinding(TArray<FGridTD0731Finding>& Findings, EGridTD0731FindingKind Kind, const TCHAR* Code, const FString& AssetPath,
		const FString& Context, const FString& Detail)
	{
		FGridTD0731Finding& Finding = Findings.AddDefaulted_GetRef();
		Finding.Kind = Kind;
		Finding.Code = Code;
		Finding.AssetPath = AssetPath;
		Finding.Context = Context;
		Finding.Detail = Detail;
	}

	void AuditDefinitionPair(const FString& AssetPath, const FString& Context, const UObject* DefinitionObject, FName DefinitionId, FName StoredId,
		const TCHAR* Domain, TArray<FGridTD0731Finding>& Findings)
	{
		if (DefinitionObject && DefinitionId.IsNone())
		{
			AddFinding(Findings, EGridTD0731FindingKind::Conflict, TEXT("AUTHORING.DEFINITION_WITHOUT_ID"), AssetPath, Context,
				FString::Printf(TEXT("%s definition asset '%s' has no canonical id."), Domain, *DefinitionObject->GetPathName()));
		}

		if (StoredId.IsNone())
		{
			return;
		}

		if (!DefinitionObject)
		{
			AddFinding(Findings, EGridTD0731FindingKind::LegacyOnly, TEXT("AUTHORING.ID_ONLY"), AssetPath, Context,
				FString::Printf(TEXT("%s is authored only through id '%s'; target schema requires the definition asset reference."), Domain,
					*StoredId.ToString()));
			return;
		}

		if (DefinitionId != StoredId)
		{
			AddFinding(Findings, EGridTD0731FindingKind::Conflict, TEXT("AUTHORING.ASSET_ID_CONFLICT"), AssetPath, Context,
				FString::Printf(TEXT("%s asset id '%s' conflicts with stored id '%s'."), Domain, *DefinitionId.ToString(), *StoredId.ToString()));
			return;
		}

		AddFinding(Findings, EGridTD0731FindingKind::DuplicateAuthority, TEXT("AUTHORING.ASSET_ID_DUPLICATE"), AssetPath, Context,
			FString::Printf(TEXT("%s stores both asset reference and mirrored id '%s'."), Domain, *StoredId.ToString()));
	}

	void AuditBehavior(const FString& AssetPath, const FString& Context, const FGridObjectBehaviorParams& Behavior, TArray<FGridTD0731Finding>& Findings)
	{
		const UGridItemDefinitionAsset* ItemDefinition = Behavior.Item.ItemDefinitionAsset.Get();
		AuditDefinitionPair(AssetPath, Context + TEXT(".Item"), ItemDefinition, ItemDefinition ? ItemDefinition->ItemDefinitionId : NAME_None,
			Behavior.Item.ItemDefinitionId, TEXT("Item"), Findings);

		const UGridReadableContentAsset* ReadableDefinition = Behavior.Item.DefaultReadableContentAsset.Get();
		AuditDefinitionPair(AssetPath, Context + TEXT(".DefaultReadableContent"), ReadableDefinition,
			ReadableDefinition ? ReadableDefinition->ReadableContentId : NAME_None, Behavior.Item.DefaultReadableContentId, TEXT("ReadableContent"), Findings);

		if (!Behavior.Lock.AcceptedKeyIds.IsEmpty())
		{
			AddFinding(Findings, EGridTD0731FindingKind::LegacyField, TEXT("AUTHORING.LOCK_KEY_IDS"), AssetPath, Context + TEXT(".Lock"),
				FString::Printf(TEXT("AcceptedKeyIds contains %d id(s); target authoring schema keeps AcceptedKeyItems only."),
					Behavior.Lock.AcceptedKeyIds.Num()));
		}
	}

	bool IsCardinalFacing(EGridEdge Facing)
	{
		return Facing == EGridEdge::North || Facing == EGridEdge::East || Facing == EGridEdge::South || Facing == EGridEdge::West;
	}

	float GetYawForFacing(EGridEdge Facing)
	{
		switch (Facing)
		{
			case EGridEdge::East:
				return 90.0f;
			case EGridEdge::South:
				return 180.0f;
			case EGridEdge::West:
				return 270.0f;
			case EGridEdge::North:
			default:
				return 0.0f;
		}
	}

	void AuditLevelAsset(const UGridLevelAsset& Level, const FString& AssetPath, TArray<FGridTD0731Finding>& Findings)
	{
		for (int32 ObjectIndex = 0; ObjectIndex < Level.Objects.Num(); ++ObjectIndex)
		{
			const FGridLevelObjectData& Object = Level.Objects[ObjectIndex];
			const FString Context = FString::Printf(TEXT("Objects[%d] ObjectId=%s"), ObjectIndex, *Object.ObjectId.ToString(EGuidFormats::Digits));

			const UGridItemDefinitionAsset* ItemDefinition = Object.ItemDefinitionAsset.Get();
			AuditDefinitionPair(AssetPath, Context + TEXT(".ItemDefinition"), ItemDefinition,
				ItemDefinition ? ItemDefinition->ItemDefinitionId : NAME_None, Object.ItemDefinitionId, TEXT("Item"), Findings);

			const UGridReadableContentAsset* ReadableDefinition = Object.ReadableContentAsset.Get();
			AuditDefinitionPair(AssetPath, Context + TEXT(".ReadableContent"), ReadableDefinition,
				ReadableDefinition ? ReadableDefinition->ReadableContentId : NAME_None, Object.ReadableContentId, TEXT("ReadableContent"), Findings);

			const UGridMonsterDefinitionAsset* MonsterDefinition = Object.MonsterDefinitionAsset.Get();
			AuditDefinitionPair(AssetPath, Context + TEXT(".MonsterDefinition"), MonsterDefinition,
				MonsterDefinition ? MonsterDefinition->MonsterId : NAME_None, Object.MonsterDefinitionId, TEXT("Monster"), Findings);

			AuditBehavior(AssetPath, Context + TEXT(".Behavior"), Object.Behavior, Findings);

			if (Object.Type == EGridLevelObjectType::MonsterSpawn)
			{
				if (!IsCardinalFacing(Object.InitialFacing))
				{
					AddFinding(Findings, EGridTD0731FindingKind::LegacyOnly, TEXT("MONSTERSPAWN.LEGACY_YAW_FACING"), AssetPath, Context,
						TEXT("InitialFacing is not cardinal; current PostLoad can reconstruct it from LocalYaw. Target schema rejects this legacy fallback."));
				}
				else
				{
					const float ExpectedYaw = GetYawForFacing(Object.InitialFacing);
					const float DeltaYaw = FMath::FindDeltaAngleDegrees(ExpectedYaw, Object.LocalYaw);
					if (!FMath::IsNearlyZero(DeltaYaw, 0.1f))
					{
						AddFinding(Findings, EGridTD0731FindingKind::DuplicateAuthority, TEXT("MONSTERSPAWN.FACING_YAW_MISMATCH"), AssetPath, Context,
							FString::Printf(TEXT("InitialFacing is authoritative but LocalYaw=%g differs from expected %g."), Object.LocalYaw, ExpectedYaw));
					}
				}
			}
		}
	}

	void AuditArchetypeAsset(const UGridObjectArchetypeAsset& Archetype, const FString& AssetPath, TArray<FGridTD0731Finding>& Findings)
	{
		const bool bExpectedEdge = Archetype.PlacementKind == EGridObjectPlacementKind::Edge || Archetype.PlacementKind == EGridObjectPlacementKind::Wall;
		const bool bExpectedCenter = Archetype.PlacementKind == EGridObjectPlacementKind::Center || Archetype.PlacementKind == EGridObjectPlacementKind::Floor ||
			Archetype.PlacementKind == EGridObjectPlacementKind::Ceiling;
		if (Archetype.bPlaceOnEdge != bExpectedEdge || Archetype.bPlaceAtCellCenter != bExpectedCenter)
		{
			AddFinding(Findings, EGridTD0731FindingKind::LegacyField, TEXT("ARCHETYPE.LEGACY_PLACEMENT_MIRROR"), AssetPath, TEXT("Placement"),
				FString::Printf(TEXT("PlacementKind=%s but legacy mirrors are bPlaceOnEdge=%s bPlaceAtCellCenter=%s."),
					*UEnum::GetValueAsString(Archetype.PlacementKind), Archetype.bPlaceOnEdge ? TEXT("true") : TEXT("false"),
					Archetype.bPlaceAtCellCenter ? TEXT("true") : TEXT("false")));
		}

		AuditBehavior(AssetPath, TEXT("DefaultBehavior"), Archetype.DefaultBehavior, Findings);
	}

	void AuditItemDefinition(const UGridItemDefinitionAsset& Item, const FString& AssetPath, TArray<FGridTD0731Finding>& Findings)
	{
		if (!Item.HasValidCombatActions())
		{
			AddFinding(Findings, EGridTD0731FindingKind::Conflict, TEXT("ITEM.INVALID_COMBAT_ACTIONS"), AssetPath, TEXT("Equipment|CombatActions"),
				TEXT("Item contains an invalid or duplicate CombatActions definition."));
		}
	}

	void AuditMonsterDefinition(const UGridMonsterDefinitionAsset& Monster, const FString& AssetPath, TArray<FGridTD0731Finding>& Findings)
	{
		if (!Monster.Attacks.IsEmpty())
		{
			AddFinding(Findings, EGridTD0731FindingKind::SchemaRename, TEXT("MONSTER.RANGE_FIELD_RENAME"), AssetPath, TEXT("Attacks"),
				FString::Printf(TEXT("%d attack(s) store maximum range in legacy-named RangeCells; target property name is MaxRangeCells."), Monster.Attacks.Num()));
		}

		for (int32 LootIndex = 0; LootIndex < Monster.LootTable.Num(); ++LootIndex)
		{
			const FGridMonsterLootEntry& Loot = Monster.LootTable[LootIndex];
			const UGridItemDefinitionAsset* ItemDefinition = Loot.ItemDefinitionAsset.Get();
			AuditDefinitionPair(AssetPath, FString::Printf(TEXT("LootTable[%d]"), LootIndex), ItemDefinition,
				ItemDefinition ? ItemDefinition->ItemDefinitionId : NAME_None, Loot.ItemDefinitionId, TEXT("LootItem"), Findings);
		}
	}

	void AuditDataAsset(const UDataAsset& DataAsset, TArray<FGridTD0731Finding>& Findings)
	{
		const FString AssetPath = DataAsset.GetPathName();
		if (const UGridLevelAsset* Level = Cast<UGridLevelAsset>(&DataAsset))
		{
			AuditLevelAsset(*Level, AssetPath, Findings);
		}
		else if (const UGridObjectArchetypeAsset* Archetype = Cast<UGridObjectArchetypeAsset>(&DataAsset))
		{
			AuditArchetypeAsset(*Archetype, AssetPath, Findings);
		}
		else if (const UGridItemDefinitionAsset* Item = Cast<UGridItemDefinitionAsset>(&DataAsset))
		{
			AuditItemDefinition(*Item, AssetPath, Findings);
		}
		else if (const UGridMonsterDefinitionAsset* Monster = Cast<UGridMonsterDefinitionAsset>(&DataAsset))
		{
			AuditMonsterDefinition(*Monster, AssetPath, Findings);
		}
	}

	FString BuildReport(const TMap<FString, int32>& ClassCounts, const TArray<FGridTD0731Finding>& Findings, int32 ScannedAssets)
	{
		TArray<FString> Lines;
		Lines.Add(TEXT("GrimrockPrototype TD07.3.1 - Current Schema Asset Audit"));
		Lines.Add(TEXT("Policy: prototype data has NO backward-compatibility requirement."));
		Lines.Add(FString::Printf(TEXT("Scanned DataAssets: %d"), ScannedAssets));
		Lines.Add(FString::Printf(TEXT("Findings: %d"), Findings.Num()));
		Lines.Add(TEXT(""));
		Lines.Add(TEXT("DataAsset classes:"));

		TArray<FString> ClassNames;
		ClassCounts.GetKeys(ClassNames);
		ClassNames.Sort();
		for (const FString& ClassName : ClassNames)
		{
			Lines.Add(FString::Printf(TEXT("  %-48s %d"), *ClassName, ClassCounts.FindRef(ClassName)));
		}

		TMap<FString, int32> FindingKindCounts;
		TMap<FString, int32> FindingCodeCounts;
		for (const FGridTD0731Finding& Finding : Findings)
		{
			FindingKindCounts.FindOrAdd(ToFindingKindText(Finding.Kind))++;
			FindingCodeCounts.FindOrAdd(Finding.Code)++;
		}

		Lines.Add(TEXT(""));
		Lines.Add(TEXT("Finding kinds:"));
		TArray<FString> KindNames;
		FindingKindCounts.GetKeys(KindNames);
		KindNames.Sort();
		for (const FString& KindName : KindNames)
		{
			Lines.Add(FString::Printf(TEXT("  %-24s %d"), *KindName, FindingKindCounts.FindRef(KindName)));
		}

		Lines.Add(TEXT(""));
		Lines.Add(TEXT("Finding codes:"));
		TArray<FString> Codes;
		FindingCodeCounts.GetKeys(Codes);
		Codes.Sort();
		for (const FString& Code : Codes)
		{
			Lines.Add(FString::Printf(TEXT("  %-44s %d"), *Code, FindingCodeCounts.FindRef(Code)));
		}

		Lines.Add(TEXT(""));
		Lines.Add(TEXT("Details:"));
		for (const FGridTD0731Finding& Finding : Findings)
		{
			Lines.Add(FString::Printf(TEXT("[%s] %s | %s | %s | %s"), ToFindingKindText(Finding.Kind), *Finding.Code, *Finding.AssetPath,
				*Finding.Context, *Finding.Detail));
		}

		Lines.Add(TEXT(""));
		Lines.Add(TEXT("NOTE: Findings are characterization output, not backward-compatibility requirements. TD07.3 will repair or recreate current assets, then remove the legacy schema."));
		return FString::Join(Lines, TEXT("\n"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0731CurrentSchemaAssetAuditTest, "Grimrock.TechnicalDebt.TD07_3_1.CurrentSchemaAssetAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0731CurrentSchemaAssetAuditTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0731SchemaAuditPrivate;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game")));
	Filter.ClassPaths.Add(UDataAsset::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> AssetData;
	AssetRegistry.GetAssets(Filter, AssetData);
	AssetData.Sort(
		[](const FAssetData& Left, const FAssetData& Right)
		{
			return Left.PackageName.ToString() < Right.PackageName.ToString();
		});

	TestTrue(TEXT("The project contains at least one DataAsset to audit"), !AssetData.IsEmpty());
	if (AssetData.IsEmpty())
	{
		return false;
	}

	TMap<FString, int32> ClassCounts;
	TArray<FGridTD0731Finding> Findings;
	int32 LoadedAssetCount = 0;
	for (const FAssetData& Entry : AssetData)
	{
		UDataAsset* DataAsset = Cast<UDataAsset>(Entry.GetAsset());
		if (!DataAsset)
		{
			AddFinding(Findings, EGridTD0731FindingKind::Conflict, TEXT("ASSET.LOAD_FAILURE"), Entry.PackageName.ToString(), TEXT("AssetRegistry"),
				TEXT("AssetRegistry returned a DataAsset entry that could not be loaded."));
			continue;
		}

		++LoadedAssetCount;
		ClassCounts.FindOrAdd(DataAsset->GetClass()->GetName())++;
		AuditDataAsset(*DataAsset, Findings);
	}

	TestEqual(TEXT("Every discovered DataAsset can be loaded"), LoadedAssetCount, AssetData.Num());

	const FString ReportDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Diagnostics"), TEXT("TD07"));
	const FString ReportPath = FPaths::Combine(ReportDirectory, TEXT("TD07_3_1_CurrentSchemaAssetAudit.txt"));
	IFileManager::Get().MakeDirectory(*ReportDirectory, true);
	const FString Report = BuildReport(ClassCounts, Findings, LoadedAssetCount);
	const bool bSavedReport = FFileHelper::SaveStringToFile(Report, *ReportPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	TestTrue(TEXT("The TD07.3.1 current-schema audit report is written"), bSavedReport);

	AddInfo(FString::Printf(TEXT("TD07.3.1 scanned %d DataAssets and found %d current-schema cleanup candidates."), LoadedAssetCount, Findings.Num()));
	AddInfo(FString::Printf(TEXT("TD07.3.1 report: %s"), *ReportPath));
	UE_LOG(LogGridTD0731SchemaAudit, Display, TEXT("Scanned=%d Findings=%d Report=%s"), LoadedAssetCount, Findings.Num(), *ReportPath);

	return true;
}

#endif

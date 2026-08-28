#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UObject/UnrealType.h"

namespace GridTD0736Normalization
{
	bool LoadProjectFile(const TCHAR* RelativePath, FString& OutText)
	{
		return FFileHelper::LoadFileToString(OutText, *FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}

	bool IsCardinalFacing(EGridEdge Facing)
	{
		return Facing == EGridEdge::North || Facing == EGridEdge::East || Facing == EGridEdge::South || Facing == EGridEdge::West;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0736LegacySymbolsAbsentTest, "Grimrock.TechnicalDebt.TD07_3_6.Normalization.LegacySymbolsAbsent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0736LegacySymbolsAbsentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* ArchetypeClass = UGridObjectArchetypeAsset::StaticClass();
	TestNull(TEXT("bPlaceOnEdge is removed"), ArchetypeClass->FindPropertyByName(TEXT("bPlaceOnEdge")));
	TestNull(TEXT("bPlaceAtCellCenter is removed"), ArchetypeClass->FindPropertyByName(TEXT("bPlaceAtCellCenter")));

	TestNull(TEXT("HasCharacterCommittedAttackThisPhase is removed"),
		UGridTurnManagerComponent::StaticClass()->FindFunctionByName(TEXT("HasCharacterCommittedAttackThisPhase")));

	UClass* PawnClass = AGrimrockPartyPawn::StaticClass();
	TestNull(TEXT("bEnableLegacyKeyboardUseAction is removed"), PawnClass->FindPropertyByName(TEXT("bEnableLegacyKeyboardUseAction")));
	TestNull(TEXT("UseAction is removed"), PawnClass->FindPropertyByName(TEXT("UseAction")));

	const UEnum* RebuildEnum = StaticEnum<EGridRuntimeRebuildMode>();
	TestNotNull(TEXT("Runtime rebuild enum exists"), RebuildEnum);
	if (RebuildEnum)
	{
		TestTrue(TEXT("ObjectsOnly is removed"), RebuildEnum->GetValueByNameString(TEXT("ObjectsOnly")) == INDEX_NONE);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0736MonsterSpawnFacingAuthorityTest, "Grimrock.TechnicalDebt.TD07_3_6.Normalization.MonsterSpawnFacingAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0736MonsterSpawnFacingAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0736Normalization;

	FString LevelAssetSource;
	TestTrue(TEXT("GridLevelAsset source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Core/GridLevelAsset.cpp"), LevelAssetSource));

	TestFalse(TEXT("Legacy yaw-to-facing converter is gone"), LevelAssetSource.Contains(TEXT("GetFacingForLegacyYaw")));
	TestFalse(TEXT("InitialFacing is never recovered from LocalYaw"), LevelAssetSource.Contains(TEXT("InitialFacing = GetFacingForLegacyYaw")));
	TestTrue(TEXT("InitialFacing remains authoritative for the generic preview yaw mirror"),
		LevelAssetSource.Contains(TEXT("ObjectData.LocalYaw = GetYawForFacing(ObjectData.InitialFacing)")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0736CurrentMonsterSpawnAssetsTest, "Grimrock.TechnicalDebt.TD07_3_6.Normalization.CurrentMonsterSpawnAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0736CurrentMonsterSpawnAssetsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0736Normalization;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game")));
	Filter.ClassPaths.Add(UGridLevelAsset::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	int32 MonsterSpawnCount = 0;
	for (const FAssetData& AssetData : Assets)
	{
		UGridLevelAsset* Level = Cast<UGridLevelAsset>(AssetData.GetAsset());
		TestNotNull(*FString::Printf(TEXT("%s loads"), *AssetData.PackageName.ToString()), Level);
		if (!Level)
		{
			continue;
		}

		for (const FGridLevelObjectData& Object : Level->Objects)
		{
			if (Object.Type != EGridLevelObjectType::MonsterSpawn)
			{
				continue;
			}

			++MonsterSpawnCount;
			TestTrue(*FString::Printf(TEXT("%s MonsterSpawn has durable cardinal InitialFacing"), *AssetData.PackageName.ToString()),
				IsCardinalFacing(Object.InitialFacing));
		}

		TArray<FString> SpawnErrors;
		TestTrue(
			*FString::Printf(TEXT("%s current MonsterSpawn schema validates"), *AssetData.PackageName.ToString()), Level->ValidateMonsterSpawns(SpawnErrors));
		for (const FString& Error : SpawnErrors)
		{
			AddError(FString::Printf(TEXT("%s: %s"), *AssetData.PackageName.ToString(), *Error));
		}
	}

	TestTrue(TEXT("At least one current MonsterSpawn is covered by the repaired asset set"), MonsterSpawnCount > 0);
	AddInfo(FString::Printf(TEXT("TD07.3.6 validated %d current MonsterSpawn(s) without legacy yaw fallback."), MonsterSpawnCount));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0736LegacyInfrastructureRemovedTest, "Grimrock.TechnicalDebt.TD07_3_6.Normalization.LegacyInfrastructureRemoved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0736LegacyInfrastructureRemovedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0736Normalization;

	FString AuditSource;
	TestTrue(TEXT("TD07.3.1 audit source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototypeEditor/Private/Tests/GridEditorTD0731CurrentSchemaAssetAuditTests.cpp"), AuditSource));
	TestFalse(TEXT("Legacy MonsterSpawn yaw finding is removed"), AuditSource.Contains(TEXT("MONSTERSPAWN.LEGACY_YAW_FACING")));
	TestFalse(TEXT("MonsterSpawn yaw duplicate-authority finding is removed"), AuditSource.Contains(TEXT("MONSTERSPAWN.FACING_YAW_MISMATCH")));

	TestFalse(TEXT("One-shot PowerShell repair is removed"),
		FPaths::FileExists(FPaths::Combine(FPaths::ProjectDir(), TEXT("Scripts/RepairTD0736MonsterSpawnFacing.ps1"))));
	TestFalse(TEXT("One-shot Editor repair test is removed"),
		FPaths::FileExists(FPaths::Combine(
			FPaths::ProjectDir(), TEXT("Source/GrimrockPrototypeEditor/Private/Tests/GridEditorTD0736MonsterSpawnFacingAssetRepairTests.cpp"))));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

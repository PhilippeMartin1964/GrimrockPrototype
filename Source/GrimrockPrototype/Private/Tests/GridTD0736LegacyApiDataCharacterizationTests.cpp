#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridObjectArchetypeAsset.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UObject/UnrealType.h"

namespace GridTD0736Characterization
{
	bool LoadProjectFile(const TCHAR* RelativePath, FString& OutText)
	{
		return FFileHelper::LoadFileToString(OutText, *FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0736LegacyPlacementMirrorsTest, "Grimrock.TechnicalDebt.TD07_3_6.Characterization.LegacyPlacementMirrors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0736LegacyPlacementMirrorsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0736Characterization;

	UClass* ArchetypeClass = UGridObjectArchetypeAsset::StaticClass();
	TestNotNull(TEXT("Archetype class exists"), ArchetypeClass);
	if (!ArchetypeClass)
	{
		return false;
	}

	TestNotNull(TEXT("PlacementKind current authority exists"), ArchetypeClass->FindPropertyByName(TEXT("PlacementKind")));
	TestNull(TEXT("Legacy bPlaceOnEdge mirror is removed"), ArchetypeClass->FindPropertyByName(TEXT("bPlaceOnEdge")));
	TestNull(TEXT("Legacy bPlaceAtCellCenter mirror is removed"), ArchetypeClass->FindPropertyByName(TEXT("bPlaceAtCellCenter")));

	FString HeaderSource;
	FString ValidationSource;
	FString EditorSource;
	TestTrue(TEXT("Archetype header loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Core/GridObjectArchetypeAsset.h"), HeaderSource));
	TestTrue(TEXT("Archetype validation source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Core/GridObjectArchetypeAsset.cpp"), ValidationSource));
	TestTrue(TEXT("Grid editor core-dungeon source loads"),
		LoadProjectFile(
			TEXT("Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEditorActorParts/CoreDungeon/GridLevelEditorActor_CoreDungeon_03.inl"),
			EditorSource));

	TestTrue(
		TEXT("PlacementKind is documented as current source of truth"), HeaderSource.Contains(TEXT("Current source of truth for editor/runtime placement")));
	TestFalse(
		TEXT("Legacy mirror validation is removed"), ValidationSource.Contains(TEXT("bPlaceOnEdge")) || ValidationSource.Contains(TEXT("bPlaceAtCellCenter")));
	TestTrue(TEXT("Current editor authoring explicitly sets PlacementKind"),
		EditorSource.Contains(TEXT("Archetype.PlacementKind = EGridObjectPlacementKind::Floor")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0736MonsterSpawnYawFallbackTest, "Grimrock.TechnicalDebt.TD07_3_6.Characterization.MonsterSpawnYawFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0736MonsterSpawnYawFallbackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0736Characterization;

	UScriptStruct* ObjectStruct = FGridLevelObjectData::StaticStruct();
	TestNotNull(TEXT("Grid level object struct exists"), ObjectStruct);
	if (!ObjectStruct)
	{
		return false;
	}

	TestNotNull(TEXT("Current InitialFacing field exists"), ObjectStruct->FindPropertyByName(TEXT("InitialFacing")));
	TestNotNull(TEXT("Generic LocalYaw field exists"), ObjectStruct->FindPropertyByName(TEXT("LocalYaw")));

	FString LevelAssetSource;
	FString AuditSource;
	TestTrue(TEXT("GridLevelAsset source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Core/GridLevelAsset.cpp"), LevelAssetSource));
	TestTrue(TEXT("Current schema audit source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototypeEditor/Private/Tests/GridEditorTD0731CurrentSchemaAssetAuditTests.cpp"), AuditSource));

	TestFalse(TEXT("Legacy yaw converter is removed"), LevelAssetSource.Contains(TEXT("GetFacingForLegacyYaw")));
	TestFalse(TEXT("InitialFacing is never reconstructed from LocalYaw"),
		LevelAssetSource.Contains(TEXT("ObjectData.InitialFacing = GetFacingForLegacyYaw(ObjectData.LocalYaw)")));
	TestTrue(TEXT("InitialFacing then rewrites the preview LocalYaw mirror"),
		LevelAssetSource.Contains(TEXT("ObjectData.LocalYaw = GetYawForFacing(ObjectData.InitialFacing)")));
	TestFalse(TEXT("TD07.3.1 legacy MonsterSpawn facing findings are removed"),
		AuditSource.Contains(TEXT("MONSTERSPAWN.LEGACY_YAW_FACING")) || AuditSource.Contains(TEXT("MONSTERSPAWN.FACING_YAW_MISMATCH")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0736DeprecatedCombatAndKeyboardUseTest,
	"Grimrock.TechnicalDebt.TD07_3_6.Characterization.DeprecatedCombatAndKeyboardUse", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0736DeprecatedCombatAndKeyboardUseTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0736Characterization;

	UFunction* DeprecatedQuery = UGridTurnManagerComponent::StaticClass()->FindFunctionByName(TEXT("HasCharacterCommittedAttackThisPhase"));
	TestNull(TEXT("Deprecated combat query is removed"), DeprecatedQuery);

	UClass* PawnClass = AGrimrockPartyPawn::StaticClass();
	TestNotNull(TEXT("Party pawn class exists"), PawnClass);
	if (!PawnClass)
	{
		return false;
	}
	TestNull(TEXT("Legacy keyboard use flag is removed"), PawnClass->FindPropertyByName(TEXT("bEnableLegacyKeyboardUseAction")));
	TestNull(TEXT("Legacy UseAction property is removed"), PawnClass->FindPropertyByName(TEXT("UseAction")));

	FString PawnSource;
	FString BufferSource;
	FString MON11Source;
	TestTrue(TEXT("Party pawn source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GrimrockPartyPawn.cpp"), PawnSource));
	TestTrue(TEXT("Party pawn input-buffer source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GrimrockPartyPawnInputBuffer.cpp"), BufferSource));
	TestTrue(TEXT("MON11 source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Tests/GridMonsterMON11Tests.cpp"), MON11Source));

	TestFalse(TEXT("Legacy keyboard Use path is removed"),
		PawnSource.Contains(TEXT("bEnableLegacyKeyboardUseAction")) || PawnSource.Contains(TEXT("HandleUse")) ||
			PawnSource.Contains(TEXT("TryUseFrontInteraction")));
	TestFalse(TEXT("Legacy Use command is removed from the input buffer"),
		BufferSource.Contains(TEXT("EBufferedCommandType::Use")) || BufferSource.Contains(TEXT("BufferUseCommand")));
	TestFalse(TEXT("Historical MON11 tests use the current turn-state authority"), MON11Source.Contains(TEXT("HasCharacterCommittedAttackThisPhase")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0736LegacyRebuildModeTest, "Grimrock.TechnicalDebt.TD07_3_6.Characterization.LegacyRebuildMode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0736LegacyRebuildModeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0736Characterization;

	const UEnum* RebuildEnum = StaticEnum<EGridRuntimeRebuildMode>();
	TestNotNull(TEXT("Runtime rebuild enum exists"), RebuildEnum);
	if (!RebuildEnum)
	{
		return false;
	}

	TestTrue(TEXT("Full rebuild mode exists"), RebuildEnum->GetValueByNameString(TEXT("Full")) != INDEX_NONE);
	TestTrue(TEXT("GeometryOnly rebuild mode exists"), RebuildEnum->GetValueByNameString(TEXT("GeometryOnly")) != INDEX_NONE);
	TestTrue(TEXT("Legacy ObjectsOnly rebuild mode is removed"), RebuildEnum->GetValueByNameString(TEXT("ObjectsOnly")) == INDEX_NONE);

	FString HeaderSource;
	FString RuntimeSource;
	FString StartupSource;
	FString EditorSource;
	TestTrue(TEXT("Runtime actor header loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Runtime/GridLevelRuntimeActor.h"), HeaderSource));
	TestTrue(TEXT("Runtime actor source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp"), RuntimeSource));
	TestTrue(TEXT("Startup source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GrimrockStartupModeComponent.cpp"), StartupSource));
	TestTrue(TEXT("Editor interaction source loads"),
		LoadProjectFile(
			TEXT("Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEditorActorParts/GridLevelEditorActor_InteractionViewport.inl"), EditorSource));

	TestFalse(TEXT("ObjectsOnly declaration is removed"), HeaderSource.Contains(TEXT("ObjectsOnly")));
	TestFalse(TEXT("Runtime implementation has no ObjectsOnly call site"), RuntimeSource.Contains(TEXT("EGridRuntimeRebuildMode::ObjectsOnly")));
	TestFalse(TEXT("Startup has no ObjectsOnly call site"), StartupSource.Contains(TEXT("EGridRuntimeRebuildMode::ObjectsOnly")));
	TestFalse(TEXT("Editor preview has no ObjectsOnly call site"), EditorSource.Contains(TEXT("EGridRuntimeRebuildMode::ObjectsOnly")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0736LegacyPlacementMirrorsTest,
	"Grimrock.TechnicalDebt.TD07_3_6.Characterization.LegacyPlacementMirrors",
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
	TestNotNull(TEXT("Legacy bPlaceOnEdge mirror still exists"), ArchetypeClass->FindPropertyByName(TEXT("bPlaceOnEdge")));
	TestNotNull(TEXT("Legacy bPlaceAtCellCenter mirror still exists"), ArchetypeClass->FindPropertyByName(TEXT("bPlaceAtCellCenter")));

	FString HeaderSource;
	FString ValidationSource;
	FString EditorSource;
	TestTrue(TEXT("Archetype header loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Core/GridObjectArchetypeAsset.h"), HeaderSource));
	TestTrue(TEXT("Archetype validation source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Core/GridObjectArchetypeAsset.cpp"), ValidationSource));
	TestTrue(TEXT("Grid editor core-dungeon source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEditorActorParts/CoreDungeon/GridLevelEditorActor_CoreDungeon_03.inl"), EditorSource));

	TestTrue(TEXT("PlacementKind is documented as current source of truth"),
		HeaderSource.Contains(TEXT("Current source of truth for editor/runtime placement")));
	TestTrue(TEXT("Legacy mirrors are used only as compatibility validation in archetype validation"),
		ValidationSource.Contains(TEXT("Legacy bPlaceOnEdge=true")) &&
		ValidationSource.Contains(TEXT("Legacy bPlaceAtCellCenter=true")));
	TestTrue(TEXT("Current editor authoring explicitly sets PlacementKind"),
		EditorSource.Contains(TEXT("Archetype.PlacementKind = EGridObjectPlacementKind::Floor")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0736MonsterSpawnYawFallbackTest,
	"Grimrock.TechnicalDebt.TD07_3_6.Characterization.MonsterSpawnYawFallback",
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
	TestTrue(TEXT("GridLevelAsset source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Core/GridLevelAsset.cpp"), LevelAssetSource));
	TestTrue(TEXT("Current schema audit source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototypeEditor/Private/Tests/GridEditorTD0731CurrentSchemaAssetAuditTests.cpp"), AuditSource));

	TestTrue(TEXT("Legacy yaw converter still exists"),
		LevelAssetSource.Contains(TEXT("GetFacingForLegacyYaw")));
	TestTrue(TEXT("Invalid InitialFacing still falls back to LocalYaw"),
		LevelAssetSource.Contains(TEXT("ObjectData.InitialFacing = GetFacingForLegacyYaw(ObjectData.LocalYaw)")));
	TestTrue(TEXT("InitialFacing then rewrites the preview LocalYaw mirror"),
		LevelAssetSource.Contains(TEXT("ObjectData.LocalYaw = GetYawForFacing(ObjectData.InitialFacing)")));
	TestTrue(TEXT("TD07.3.1 still tracks the legacy facing fallback"),
		AuditSource.Contains(TEXT("MONSTERSPAWN.LEGACY_YAW_FACING")) &&
		AuditSource.Contains(TEXT("MONSTERSPAWN.FACING_YAW_MISMATCH")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0736DeprecatedCombatAndKeyboardUseTest,
	"Grimrock.TechnicalDebt.TD07_3_6.Characterization.DeprecatedCombatAndKeyboardUse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0736DeprecatedCombatAndKeyboardUseTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0736Characterization;

	UFunction* DeprecatedQuery =
		UGridTurnManagerComponent::StaticClass()->FindFunctionByName(TEXT("HasCharacterCommittedAttackThisPhase"));
	TestNotNull(TEXT("Deprecated combat query still exists"), DeprecatedQuery);
	if (DeprecatedQuery)
	{
		TestTrue(TEXT("Combat query is explicitly deprecated"),
			DeprecatedQuery->HasMetaData(TEXT("DeprecatedFunction")));
	}

	UClass* PawnClass = AGrimrockPartyPawn::StaticClass();
	TestNotNull(TEXT("Party pawn class exists"), PawnClass);
	if (!PawnClass)
	{
		return false;
	}
	TestNotNull(TEXT("Legacy keyboard use flag still exists"),
		PawnClass->FindPropertyByName(TEXT("bEnableLegacyKeyboardUseAction")));

	const AGrimrockPartyPawn* DefaultPawn = GetDefault<AGrimrockPartyPawn>();
	TestNotNull(TEXT("Party pawn default object exists"), DefaultPawn);
	if (DefaultPawn)
	{
		TestFalse(TEXT("Legacy keyboard use is disabled by default"), DefaultPawn->bEnableLegacyKeyboardUseAction);
	}

	FString PawnSource;
	FString BufferSource;
	FString MON11Source;
	TestTrue(TEXT("Party pawn source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GrimrockPartyPawn.cpp"), PawnSource));
	TestTrue(TEXT("Party pawn input-buffer source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GrimrockPartyPawnInputBuffer.cpp"), BufferSource));
	TestTrue(TEXT("MON11 source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Tests/GridMonsterMON11Tests.cpp"), MON11Source));

	TestTrue(TEXT("Legacy UseAction binding is opt-in"),
		PawnSource.Contains(TEXT("if (bEnableLegacyKeyboardUseAction && UseAction)")));
	TestTrue(TEXT("Legacy keyboard path still reaches front interaction"),
		PawnSource.Contains(TEXT("HandleUse")) && PawnSource.Contains(TEXT("TryUseFrontInteraction")));
	TestTrue(TEXT("Legacy Use command is still part of the input buffer"),
		BufferSource.Contains(TEXT("EBufferedCommandType::Use")) &&
		BufferSource.Contains(TEXT("BufferUseCommand")));
	TestTrue(TEXT("Only historical MON11 tests still exercise the deprecated combat query"),
		MON11Source.Contains(TEXT("HasCharacterCommittedAttackThisPhase")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0736LegacyRebuildModeTest,
	"Grimrock.TechnicalDebt.TD07_3_6.Characterization.LegacyRebuildMode",
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

	TestTrue(TEXT("Full rebuild mode exists"),
		RebuildEnum->GetValueByNameString(TEXT("Full")) != INDEX_NONE);
	TestTrue(TEXT("GeometryOnly rebuild mode exists"),
		RebuildEnum->GetValueByNameString(TEXT("GeometryOnly")) != INDEX_NONE);
	TestTrue(TEXT("Legacy ObjectsOnly rebuild mode still exists"),
		RebuildEnum->GetValueByNameString(TEXT("ObjectsOnly")) != INDEX_NONE);

	FString HeaderSource;
	FString RuntimeSource;
	FString StartupSource;
	FString EditorSource;
	TestTrue(TEXT("Runtime actor header loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Runtime/GridLevelRuntimeActor.h"), HeaderSource));
	TestTrue(TEXT("Runtime actor source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp"), RuntimeSource));
	TestTrue(TEXT("Startup source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GrimrockStartupModeComponent.cpp"), StartupSource));
	TestTrue(TEXT("Editor interaction source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEditorActorParts/GridLevelEditorActor_InteractionViewport.inl"), EditorSource));

	TestTrue(TEXT("ObjectsOnly is documented as reserved legacy mode"),
		HeaderSource.Contains(TEXT("Reserved legacy mode. No current call site uses it")));
	TestFalse(TEXT("Runtime implementation has no ObjectsOnly call site"),
		RuntimeSource.Contains(TEXT("EGridRuntimeRebuildMode::ObjectsOnly")));
	TestFalse(TEXT("Startup has no ObjectsOnly call site"),
		StartupSource.Contains(TEXT("EGridRuntimeRebuildMode::ObjectsOnly")));
	TestFalse(TEXT("Editor preview has no ObjectsOnly call site"),
		EditorSource.Contains(TEXT("EGridRuntimeRebuildMode::ObjectsOnly")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

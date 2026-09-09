#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Core/GridLevelAsset.h"
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

	UClass* DefinitionClass = UGridWorldObjectDefinitionAsset::StaticClass();
	TestNotNull(TEXT("Definition class exists"), DefinitionClass);
	if (!DefinitionClass)
	{
		return false;
	}

	// The authored surface is persistent; the old placement projection has been removed.
	const FProperty* SurfaceProperty = DefinitionClass->FindPropertyByName(TEXT("PlacementSurface"));
	const FProperty* KindProperty = DefinitionClass->FindPropertyByName(TEXT("PlacementKind"));
	TestTrue(TEXT("PlacementSurface is editable and persistent"),
		SurfaceProperty && SurfaceProperty->HasAnyPropertyFlags(CPF_Edit) && !SurfaceProperty->HasAnyPropertyFlags(CPF_Transient));
	TestNull(TEXT("PlacementKind is removed"), KindProperty);
	TestNull(TEXT("Legacy bPlaceOnEdge mirror is removed"), DefinitionClass->FindPropertyByName(TEXT("bPlaceOnEdge")));
	TestNull(TEXT("Legacy bPlaceAtCellCenter mirror is removed"), DefinitionClass->FindPropertyByName(TEXT("bPlaceAtCellCenter")));

	FString HeaderSource;
	FString ValidationSource;
	FString EditorSource;
	TestTrue(TEXT("Definition header loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Core/GridWorldObjectDefinitionAsset.h"), HeaderSource));
	TestTrue(TEXT("Definition validation source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Core/GridWorldObjectDefinitionAsset.cpp"), ValidationSource));
	TestTrue(TEXT("Grid editor core-dungeon source loads"),
		LoadProjectFile(
			TEXT("Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEditorActorParts/CoreDungeon/GridLevelEditorActor_CoreDungeon_07.inl"),
			EditorSource));

	TestTrue(
		TEXT("PlacementSurface is documented as current source of truth"), HeaderSource.Contains(TEXT("Current source of truth for editor/runtime placement")));
	TestFalse(
		TEXT("Legacy mirror validation is removed"), ValidationSource.Contains(TEXT("bPlaceOnEdge")) || ValidationSource.Contains(TEXT("bPlaceAtCellCenter")));
	TestTrue(TEXT("Current editor authoring sets the surface and local coordinates"),
		EditorSource.Contains(TEXT("Definition.PlacementSurface = EGridObjectPlacementKind::Floor")) &&
		EditorSource.Contains(TEXT("Definition.DefaultLocalPosition = FGridSurfaceLocalPosition()")));

	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>();
	Definition->DefinitionId = TEXT("TD0736_WallPlacement");
	Definition->PlacementSurface = EGridObjectPlacementKind::Wall;
	TestEqual(TEXT("Authored wall placement uses the permanent surface"), Definition->PlacementSurface, EGridObjectPlacementKind::Wall);
	UGridLevelAsset* Level = NewObject<UGridLevelAsset>();
	FGridWorldObjectInstance Instance;
	Instance.InstanceId = FGuid::NewGuid();
	Instance.Type = EGridLevelObjectType::Decoration;
	Instance.WorldObjectDefinitionId = Definition->DefinitionId;
	Instance.CellX = 1;
	Instance.CellY = 2;
	Instance.WallSide = EGridEdge::East;
	Instance.bHasLocalTransformOverride = true;
	Instance.LocalTransformOverride = FTransform(FRotator(0.f, 30.f, 0.f), FVector(10.f, 20.f, 40.f));
	Level->WorldObjectInstances.Add(Instance);
	const FGridWorldObjectInstance* Stored = Level->FindWorldObjectInstanceById(Instance.InstanceId);
	if (!TestNotNull(TEXT("Native collection owns the placed instance"), Stored))
	{
		return false;
	}
	int32 CellX = INDEX_NONE;
	int32 CellY = INDEX_NONE;
	EGridEdge Edge = EGridEdge::None;
	TestTrue(TEXT("Native location lookup succeeds"), Level->TryGetTypedPlacementLocation(Instance.InstanceId, CellX, CellY, Edge));
	TestEqual(TEXT("Cell X is preserved"), CellX, 1);
	TestEqual(TEXT("Cell Y is preserved"), CellY, 2);
	TestEqual(TEXT("Wall side is preserved"), Edge, EGridEdge::East);
	TestEqual(TEXT("Definition identity is preserved"), Stored->WorldObjectDefinitionId, Definition->DefinitionId);
	TestTrue(TEXT("Explicit local transform is preserved"),
		Stored->bHasLocalTransformOverride && Stored->LocalTransformOverride.Equals(Instance.LocalTransformOverride));
	TestEqual(TEXT("The native instance is counted exactly once"), Level->GetTypedPlacementCount(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0736MonsterSpawnYawFallbackTest, "Grimrock.TechnicalDebt.TD07_3_6.Characterization.MonsterSpawnYawFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0736MonsterSpawnYawFallbackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0736Characterization;

	UScriptStruct* ObjectStruct = FGridMonsterSpawnInstance::StaticStruct();
	TestNotNull(TEXT("Grid level object struct exists"), ObjectStruct);
	if (!ObjectStruct)
	{
		return false;
	}

	TestNotNull(TEXT("Typed Facing field exists"), ObjectStruct->FindPropertyByName(TEXT("Facing")));
	TestNull(TEXT("Monster placement has no redundant yaw mirror"), ObjectStruct->FindPropertyByName(TEXT("LocalYaw")));

	FString LevelAssetSource;
	FString AuditSource;
	TestTrue(TEXT("GridLevelAsset source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Core/GridLevelAsset.cpp"), LevelAssetSource));
	TestTrue(TEXT("Current schema audit source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototypeEditor/Private/Tests/GridEditorTD0731CurrentSchemaAssetAuditTests.cpp"), AuditSource));

	TestFalse(TEXT("Legacy yaw converter is removed"), LevelAssetSource.Contains(TEXT("GetFacingForLegacyYaw")));
	TestFalse(TEXT("InitialFacing is never reconstructed from LocalYaw"),
		LevelAssetSource.Contains(TEXT("ObjectData.InitialFacing = GetFacingForLegacyYaw(ObjectData.LocalYaw)")));
	FGridMonsterSpawnInstance Spawn;
	Spawn.Facing = EGridEdge::West;
	TestEqual(TEXT("Typed facing is the sole persisted monster orientation"), Spawn.Facing, EGridEdge::West);
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

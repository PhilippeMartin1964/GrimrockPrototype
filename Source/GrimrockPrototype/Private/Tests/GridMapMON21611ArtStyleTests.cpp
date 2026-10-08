#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridSecretDoorActor.h"
#include "Runtime/Map/GridMapReadModel.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UI/GridMapWidget.h"
#include "UObject/UnrealType.h"

namespace GridMapMON21611Tests
{
	UGridLevelAsset* MakeOpenLevel(UObject* Outer)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Outer);
		Level->Width = FGridMapExplorationState::GridSize;
		Level->Height = FGridMapExplorationState::GridSize;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.NorthWall = EGridWallType::None;
			Cell.EastWall = EGridWallType::None;
			Cell.SouthWall = EGridWallType::None;
			Cell.WestWall = EGridWallType::None;
		}
		return Level;
	}

	UGridWorldObjectDefinitionAsset* MakeSecretDoorDefinition(UObject* Outer, FName Id)
	{
		UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(Outer);
		Definition->DefinitionId = Id;
		Definition->SupportedType = EGridLevelObjectType::Door;
		Definition->RuntimeActorClass = AGridSecretDoorActor::StaticClass();
		return Definition;
	}

	FGridWorldObjectInstance AddSecretDoor(UGridLevelAsset* Level, FName DefinitionId)
	{
		FGridWorldObjectInstance Door;
		Door.InstanceId = FGuid::NewGuid();
		Door.Type = EGridLevelObjectType::Door;
		Door.WorldObjectDefinitionId = DefinitionId;
		Door.CellX = 10;
		Door.CellY = 10;
		Door.WallSide = EGridEdge::North;
		Door.InstanceConfig.bDoorInitiallyOpen = false;
		Level->WorldObjectInstances.Add(Door);
		return Door;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON21611NoiseTest, "Grimrock.Map.MON21_6_11.ArtStyle.DeterministicNoise",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON21611NoiseTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FIntPoint Cell(17, -9);
	const float First = UGridMapWidget::ComputeDeterministicArtNoise(Cell, EGridEdge::North, 123);
	const float Second = UGridMapWidget::ComputeDeterministicArtNoise(Cell, EGridEdge::North, 123);
	const float OtherSalt = UGridMapWidget::ComputeDeterministicArtNoise(Cell, EGridEdge::North, 124);
	const float OtherCell = UGridMapWidget::ComputeDeterministicArtNoise(FIntPoint(18, -9), EGridEdge::North, 123);

	TestEqual(TEXT("Same primitive and salt produce exactly the same presentation noise"), First, Second);
	TestTrue(TEXT("Deterministic noise stays within [-1,1]"), First >= -1.0f && First <= 1.0f);
	TestTrue(TEXT("Changing salt changes the deterministic stroke sample"), !FMath::IsNearlyEqual(First, OtherSalt));
	TestTrue(TEXT("Changing map coordinate changes the deterministic stroke sample"), !FMath::IsNearlyEqual(First, OtherCell));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON21611DefaultsTest, "Grimrock.Map.MON21_6_11.ArtStyle.ParchmentDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON21611DefaultsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridMapWidget* Widget = NewObject<UGridMapWidget>(GetTransientPackage());
	if (!TestNotNull(TEXT("Map widget exists"), Widget))
	{
		return false;
	}

	TestTrue(TEXT("Parchment style is enabled by default"), Widget->bEnableParchmentStyle);
	TestTrue(TEXT("Hand-drawn jitter is non-zero"), Widget->HandDrawnJitterPixels > 0.0f);
	TestTrue(TEXT("Secondary ink stroke remains subtle"), Widget->SecondaryStrokeAlpha > 0.0f && Widget->SecondaryStrokeAlpha < 0.5f);
	TestTrue(TEXT("Procedural parchment grain is enabled"), Widget->ParchmentGrainLineCount > 0);
	TestTrue(TEXT("Explored-cell hatching is enabled"), Widget->CellHatchLineCount > 0);
	TestTrue(TEXT("Parchment background has visible alpha"), Widget->ParchmentColor.A > 0.0f);
	TestTrue(TEXT("Fog feather remains deliberately subtle"), Widget->FogFeatherColor.A > 0.0f && Widget->FogFeatherColor.A < 0.3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON21611TransientTest, "Grimrock.Map.MON21_6_11.ArtStyle.PresentationOnlyContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON21611TransientTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* MapClass = UGridMapWidget::StaticClass();
	for (const TCHAR* PropertyName : {
		TEXT("bEnableParchmentStyle"),
		TEXT("ParchmentColor"),
		TEXT("ParchmentGrainColor"),
		TEXT("CellHatchColor"),
		TEXT("FogFeatherColor"),
		TEXT("HandDrawnJitterPixels"),
		TEXT("SecondaryStrokeAlpha"),
		TEXT("ParchmentGrainLineCount"),
		TEXT("CellHatchLineCount") })
	{
		const FProperty* Property = MapClass->FindPropertyByName(PropertyName);
		TestNotNull(*FString::Printf(TEXT("%s exists"), PropertyName), Property);
		TestFalse(*FString::Printf(TEXT("%s is presentation config, never SaveGame"), PropertyName),
			Property && Property->HasAnyPropertyFlags(CPF_SaveGame));
	}

	TestEqual(TEXT("Artistic pass does not change exact-match SaveGame v24"), UGrimrockPartySaveGame::CurrentSaveVersion, 24);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON21611SecretRegressionTest, "Grimrock.Map.MON21_6_11.ArtStyle.HiddenSecretStillNormalWall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON21611SecretRegressionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON21611Tests;

	UGridLevelAsset* Level = MakeOpenLevel(GetTransientPackage());
	const FName DefinitionId(TEXT("MON21611_SecretDoor"));
	UGridWorldObjectDefinitionAsset* Definition = MakeSecretDoorDefinition(Level, DefinitionId);
	const FGridWorldObjectInstance Door = AddSecretDoor(Level, DefinitionId);

	FGridLevelRuntimeState State;
	State.LevelId = TEXT("ArtSecretTile");
	bool bNewlyExplored = false;
	State.MapExploration.TryMarkExplored(FIntPoint(10, 10), bNewlyExplored);

	FGridRuntimeDoorState& PersistedDoor = State.Doors.Add(Door.InstanceId);
	PersistedDoor.ObjectId = Door.InstanceId;
	PersistedDoor.bIsOpen = false;
	PersistedDoor.bBlocksMovement = true;

	const TArray<TObjectPtr<UGridWorldObjectDefinitionAsset>> Definitions = { Definition };
	FGridMapTileView View;
	TestTrue(TEXT("Hidden secret projection builds"),
		FGridMapReadModelBuilder::BuildTileView(State.LevelId, *Level, State, Definitions, nullptr, View));
	TestEqual(TEXT("Hidden secret exposes one observed boundary"), View.Boundaries.Num(), 1);
	TestEqual(TEXT("Art renderer receives hidden secret only as Wall"), View.Boundaries[0].Kind, EGridMapBoundaryKind::Wall);
	TestFalse(TEXT("Hidden secret does not expose an open-door state"), View.Boundaries[0].bDoorOpen);

	UScriptStruct* BoundaryStruct = FGridMapBoundaryView::StaticStruct();
	TestNull(TEXT("Boundary view still exposes no ObjectId"), BoundaryStruct->FindPropertyByName(TEXT("ObjectId")));
	TestNull(TEXT("Boundary view still exposes no DefinitionId"), BoundaryStruct->FindPropertyByName(TEXT("WorldObjectDefinitionId")));
	TestNull(TEXT("Boundary view still exposes no hidden-secret flag"), BoundaryStruct->FindPropertyByName(TEXT("bIsSecret")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

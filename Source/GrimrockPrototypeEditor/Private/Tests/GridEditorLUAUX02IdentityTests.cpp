#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "EditorTools/GridEditorLuaService.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
	struct FLuaUX02EditorWorld
	{
		UWorld* World = nullptr;

		FLuaUX02EditorWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::EditorPreview, false,
				FName(*FString::Printf(TEXT("LUAUX02_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
				Context.SetCurrentWorld(World);
			}
		}

		~FLuaUX02EditorWorld()
		{
			if (!World)
			{
				return;
			}
			World->DestroyWorld(false);
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridLuaUX02LogicIdentityContractTest,
	"Grimrock.LUAUX02.LogicIdentityContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridLuaUX02LogicIdentityContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FLuaUX02EditorWorld TestWorld;
	TestNotNull(TEXT("Editor world exists"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelEditorActor* Editor = TestWorld.World->SpawnActor<AGridLevelEditorActor>();
	TestNotNull(TEXT("Grid editor actor exists"), Editor);
	if (!Editor)
	{
		return false;
	}

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Editor);
	Level->Width = 2;
	Level->Height = 2;
	Level->EnsureCellCount();
	Editor->LevelAsset = Level;

	FGridWorldObjectInstance Guardian;
	Guardian.InstanceId = FGuid(20, 18, 1, 1);
	Guardian.Type = EGridLevelObjectType::Receptacle;
	Guardian.CellX = 0;
	Guardian.CellY = 0;
	Guardian.Tag = TEXT("LegacyGuardianTag");
	Guardian.WorldObjectDefinitionId = TEXT("Guardian");
	Level->WorldObjectInstances.Add(Guardian);

	FGridWorldObjectInstance Door;
	Door.InstanceId = FGuid(20, 18, 1, 2);
	Door.Type = EGridLevelObjectType::Door;
	Door.CellX = 1;
	Door.CellY = 0;
	Door.Tag = TEXT("LegacyDoorTag");
	Door.WorldObjectDefinitionId = TEXT("DoorWood");
	Level->WorldObjectInstances.Add(Door);

	FString Error;
	Editor->LastSelectedObjectId = Guardian.InstanceId;
	TestTrue(TEXT("Guardian LogicId is authored through the canonical service"),
		GridEditorLuaService::SetSelectedObjectLogicId(*Editor, TEXT("Guardian"), Error));
	Editor->LastSelectedObjectId = Door.InstanceId;
	TestTrue(TEXT("Door LogicId is authored through the canonical service"),
		GridEditorLuaService::SetSelectedObjectLogicId(*Editor, TEXT("GuardianDoor"), Error));

	TestEqual(TEXT("Guardian LogicId is authoritative"), Level->GetTypedPlacementLogicId(Guardian.InstanceId), FName(TEXT("Guardian")));
	TestEqual(TEXT("Door LogicId is authoritative"), Level->GetTypedPlacementLogicId(Door.InstanceId), FName(TEXT("GuardianDoor")));

	// LUA-UX02 deliberately preserves legacy serialized Tag data until a
	// dedicated asset migration removes the field. Authoring no longer depends
	// on it, so setting LogicId must not rewrite that historical payload.
	TestEqual(TEXT("Legacy Guardian Tag is preserved for asset compatibility"),
		Level->WorldObjectInstances[0].Tag, FName(TEXT("LegacyGuardianTag")));
	TestEqual(TEXT("Legacy Door Tag is preserved for asset compatibility"),
		Level->WorldObjectInstances[1].Tag, FName(TEXT("LegacyDoorTag")));

	TArray<FGuid> Matches;
	TestEqual(TEXT("GuardianDoor resolves uniquely by LogicId"), Level->FindTypedPlacementIdsByLogicId(TEXT("GuardianDoor"), Matches), 1);
	TestTrue(TEXT("GuardianDoor resolution returns the door"), Matches.Num() == 1 && Matches[0] == Door.InstanceId);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

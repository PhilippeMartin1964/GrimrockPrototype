#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace
{
	struct FGridALIGNA4EditorTestWorld
	{
		UWorld* World = nullptr;

		FGridALIGNA4EditorTestWorld()
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
				FName(*FString::Printf(TEXT("ALIGNA4PreviewSyncWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridALIGNA4EditorTestWorld()
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorALIGNA4PreviewDefinitionSyncTest, "Grimrock.Editor.ALIGN_A4.PreviewDefinitionSync",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorALIGNA4PreviewDefinitionSyncTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridALIGNA4EditorTestWorld TestWorld;
	TestNotNull(TEXT("ALIGN-A4 editor preview world is created"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelEditorActor* EditorActor = TestWorld.World->SpawnActor<AGridLevelEditorActor>();
	AGridLevelRuntimeActor* RuntimeActor = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	TestNotNull(TEXT("ALIGN-A4 editor actor is spawned"), EditorActor);
	TestNotNull(TEXT("ALIGN-A4 runtime actor is spawned"), RuntimeActor);
	if (!EditorActor || !RuntimeActor)
	{
		return false;
	}

	UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>(EditorActor);
	UGridWorldObjectDefinitionAsset* ActiveDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Palette);
	UGridWorldObjectDefinitionAsset* StaleDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Palette);
	ActiveDefinition->DefinitionId = TEXT("ALIGN_A4_Active");
	ActiveDefinition->SupportedType = EGridLevelObjectType::Decoration;
	StaleDefinition->DefinitionId = TEXT("ALIGN_A4_Stale");
	StaleDefinition->SupportedType = EGridLevelObjectType::Decoration;

	FGridObjectPaletteEntry& ActiveEntry = Palette->Entries.AddDefaulted_GetRef();
	ActiveEntry.EntryId = TEXT("ALIGN_A4_Active");
	ActiveEntry.DefaultWorldObjectDefinition = ActiveDefinition;

	RuntimeActor->WorldObjectDefinitions.Add(StaleDefinition);
	RuntimeActor->WorldObjectDefinitions.Add(ActiveDefinition);

	EditorActor->ObjectPalette = Palette;
	EditorActor->PreviewRuntimeActor = RuntimeActor;
	EditorActor->RebuildPreview();

	TestEqual(TEXT("ALIGN-A4 preview definitions are projected exactly from the palette"), RuntimeActor->WorldObjectDefinitions.Num(), 1);
	TestTrue(TEXT("ALIGN-A4 active palette definition is retained"), RuntimeActor->WorldObjectDefinitions.Contains(ActiveDefinition));
	TestFalse(TEXT("ALIGN-A4 stale runtime definition is removed"), RuntimeActor->WorldObjectDefinitions.Contains(StaleDefinition));

	EditorActor->RebuildPreview();
	TestEqual(TEXT("ALIGN-A4 repeated sync remains stable"), RuntimeActor->WorldObjectDefinitions.Num(), 1);
	TestTrue(TEXT("ALIGN-A4 repeated sync keeps the active definition"), RuntimeActor->WorldObjectDefinitions.Contains(ActiveDefinition));

	return true;
}

#endif

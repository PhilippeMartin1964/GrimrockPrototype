#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridTypes.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace RPGStoryCompanionMON204EventCommandTests
{
	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
															.AllowAudioPlayback(false)
															.RequiresHitProxies(false)
															.CreatePhysicsScene(false)
															.CreateNavigation(false)
															.CreateAISystem(false)
															.ShouldSimulatePhysics(false)
															.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false, FName(*FString::Printf(TEXT("MON2044_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (!World || !GEngine)
			{
				return;
			}

			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
		}

		~FTestWorld()
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

using namespace RPGStoryCompanionMON204EventCommandTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON2044RecruitmentEventCommandContractTest, "Grimrock.MON20.4.RecruitmentUI.EventCommandContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON2044RecruitmentEventCommandContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(TEXT("OfferRecruitment keeps the serialized command value 23"), static_cast<uint8>(EGridObjectCommand::OfferRecruitment), static_cast<uint8>(23));

	const UEnum* CommandEnum = StaticEnum<EGridObjectCommand>();
	TestNotNull(TEXT("Grid command enum exists"), CommandEnum);
	if (CommandEnum)
	{
		TestEqual(TEXT("Lua-style enum lookup resolves OfferRecruitment"), CommandEnum->GetValueByNameString(TEXT("OfferRecruitment")),
			static_cast<int64>(EGridObjectCommand::OfferRecruitment));
	}

	const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType>();
	TestNotNull(TEXT("Grid object type enum exists"), TypeEnum);
	if (TypeEnum)
	{
		TestEqual(TEXT("StoryCompanion is exposed by the object type enum"), TypeEnum->GetValueByNameString(TEXT("StoryCompanion")),
			static_cast<int64>(EGridLevelObjectType::StoryCompanion));
	}

	UScriptStruct* ObjectDataStruct = FGridLevelObjectData::StaticStruct();
	TestNotNull(TEXT("Grid level object struct exists"), ObjectDataStruct);
	if (ObjectDataStruct)
	{
		TestNotNull(TEXT("Story companion definition is part of level object data"),
			ObjectDataStruct->FindPropertyByName(GET_MEMBER_NAME_CHECKED(FGridLevelObjectData, StoryCompanionDefinition)));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON2044RecruitmentMissingDefinitionTest, "Grimrock.MON20.4.RecruitmentUI.EventCommandMissingDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON2044RecruitmentMissingDefinitionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	AddExpectedError(TEXT("missing or invalid story companion definition"), EAutomationExpectedErrorFlags::Contains, 1);

	FTestWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("Unable to create MON20.4.4 test world."));
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	TestNotNull(TEXT("Runtime actor spawns"), Runtime);
	if (!Runtime)
	{
		return false;
	}

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
	Level->Width = 1;
	Level->Height = 1;
	Level->EnsureCellCount();
	Level->Cells[0].CellType = EGridCellType::Floor;

	const FGuid SourceId(20, 4, 4, 1);
	FGridLevelObjectData Source;
	Source.ObjectId = SourceId;
	Source.Type = EGridLevelObjectType::Trigger;
	Level->Objects.Add(Source);

	const FGuid CompanionId(20, 4, 4, 2);
	FGridLevelObjectData Companion;
	Companion.ObjectId = CompanionId;
	Companion.Type = EGridLevelObjectType::StoryCompanion;
	Companion.StoryCompanionDefinition = nullptr;
	Level->Objects.Add(Companion);

	FGridObjectLink Link;
	Link.SourceObjectId = SourceId;
	Link.SourceEvent = EGridObjectEvent::Activated;
	Link.TargetObjectId = CompanionId;
	Link.Command = EGridObjectCommand::OfferRecruitment;
	Level->Links.Add(Link);

	Runtime->LevelAsset = Level;
	UGridActivationComponent* Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
	TestNotNull(TEXT("Activation component exists"), Activation);
	if (!Activation)
	{
		return false;
	}

	Activation->Initialize(Runtime);
	Activation->RebuildIndexes();

	TestFalse(TEXT("OfferRecruitment rejects a story companion without a definition"),
		Activation->ExecuteLinksFromObjectForEvent(SourceId, EGridObjectEvent::Activated));
	TestFalse(TEXT("Rejected recruitment does not mark the data-only target active"), Activation->GetActiveObjectIds().Contains(CompanionId));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

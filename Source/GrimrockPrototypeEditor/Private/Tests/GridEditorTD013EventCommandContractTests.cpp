#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridTypes.h"
#include "EditorTools/GridEditorLinkPolicy.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
	FGridLevelObjectData MakeTD013Object(EGridLevelObjectType Type, int32 CellX = 0, int32 CellY = 0)
	{
		FGridLevelObjectData Object;
		Object.ObjectId = FGuid::NewGuid();
		Object.Type = Type;
		Object.CellX = CellX;
		Object.CellY = CellY;
		Object.Edge = EGridEdge::None;
		Object.bInitiallyEnabled = true;
		return Object;
	}

	FGridObjectLink MakeTD013Link(const FGridLevelObjectData& Source, const FGridLevelObjectData& Target, EGridObjectCommand Command)
	{
		FGridObjectLink Link;
		Link.SourceObjectId = Source.ObjectId;
		Link.TargetObjectId = Target.ObjectId;
		Link.SourceEvent = EGridObjectEvent::Activated;
		Link.Command = Command;
		return Link;
	}

	bool HasUnsupportedCommandDiagnostic(const TArray<FGridLevelValidationMessage>& Messages, int32 LinkIndex)
	{
		const FString Prefix = FString::Printf(TEXT("Link %d command "), LinkIndex);
		for (const FGridLevelValidationMessage& Message : Messages)
		{
			if (Message.Severity == EGridLevelValidationSeverity::Error && Message.Message.StartsWith(Prefix) &&
				Message.Message.Contains(TEXT("is not supported by the current runtime")))
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD013EventCommandPolicyTest, "Grimrock.TechnicalDebt.TD01_3.EventCommandContract.Policy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD013EventCommandPolicyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGridLevelObjectData Teleporter = MakeTD013Object(EGridLevelObjectType::Teleporter);
	const FGridLevelObjectData Light = MakeTD013Object(EGridLevelObjectType::Light);
	const FGridLevelObjectData ItemSpawn = MakeTD013Object(EGridLevelObjectType::ItemSpawn);
	const FGridLevelObjectData Logic = MakeTD013Object(EGridLevelObjectType::Logic);
	const FGridLevelObjectData StoryCompanion = MakeTD013Object(EGridLevelObjectType::StoryCompanion);
	const FGridLevelObjectData CustomRecruiter = MakeTD013Object(EGridLevelObjectType::CustomRecruiter);

	TestFalse(
		TEXT("Teleporter is not an authorable command target until specialized gameplay exists"), GridEditorLinkPolicy::CanObjectReceiveCommands(Teleporter));
	TestFalse(TEXT("Light is not an authorable command target while it only stores generic state"), GridEditorLinkPolicy::CanObjectReceiveCommands(Light));
	TestFalse(TEXT("ItemSpawn is not an authorable command target until commanded spawning exists"), GridEditorLinkPolicy::CanObjectReceiveCommands(ItemSpawn));

	TestTrue(TEXT("Teleporter legacy activation remains classified StateOnly"),
		GridEditorLinkPolicy::GetCommandRuntimeSupport(Teleporter, EGridObjectCommand::Activate) == EGridEditorCommandRuntimeSupport::StateOnly);
	TestTrue(TEXT("Light legacy activation remains classified StateOnly"),
		GridEditorLinkPolicy::GetCommandRuntimeSupport(Light, EGridObjectCommand::Activate) == EGridEditorCommandRuntimeSupport::StateOnly);
	TestTrue(TEXT("ItemSpawn legacy activation remains classified StateOnly"),
		GridEditorLinkPolicy::GetCommandRuntimeSupport(ItemSpawn, EGridObjectCommand::Activate) == EGridEditorCommandRuntimeSupport::StateOnly);

	TestTrue(TEXT("LogicExecute remains real gameplay"),
		GridEditorLinkPolicy::GetCommandRuntimeSupport(Logic, EGridObjectCommand::LogicExecute) == EGridEditorCommandRuntimeSupport::Gameplay);
	TestTrue(TEXT("OfferRecruitment remains real gameplay"),
		GridEditorLinkPolicy::GetCommandRuntimeSupport(StoryCompanion, EGridObjectCommand::OfferRecruitment) == EGridEditorCommandRuntimeSupport::Gameplay);
	TestTrue(TEXT("OpenCustomRecruit remains real gameplay"),
		GridEditorLinkPolicy::GetCommandRuntimeSupport(CustomRecruiter, EGridObjectCommand::OpenCustomRecruit) == EGridEditorCommandRuntimeSupport::Gameplay);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD013EventCommandValidationTest, "Grimrock.TechnicalDebt.TD01_3.EventCommandContract.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD013EventCommandValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const UWorld::InitializationValues InitializationValues = UWorld::InitializationValues()
																  .AllowAudioPlayback(false)
																  .RequiresHitProxies(false)
																  .CreatePhysicsScene(false)
																  .CreateNavigation(false)
																  .CreateAISystem(false)
																  .ShouldSimulatePhysics(false)
																  .SetTransactional(false);
	UWorld* World =
		UWorld::CreateWorld(EWorldType::Editor, false, TEXT("TD013EventCommandValidationWorld"), nullptr, true, ERHIFeatureLevel::Num, &InitializationValues);
	TestNotNull(TEXT("TD01.3 editor validation world is created"), World);
	if (!World || !GEngine)
	{
		return false;
	}

	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Editor);
	Context.SetCurrentWorld(World);

	AGridLevelEditorActor* EditorActor = World->SpawnActor<AGridLevelEditorActor>();
	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(EditorActor);
	Level->Width = 4;
	Level->Height = 2;
	Level->EnsureCellCount();
	for (FGridLevelCellData& Cell : Level->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.bBlocksOccupancy = false;
	}
	Level->StartCellX = 0;
	Level->StartCellY = 0;
	Level->StartFacing = EGridEdge::North;
	EditorActor->LevelAsset = Level;

	const FGridLevelObjectData Trigger = MakeTD013Object(EGridLevelObjectType::Trigger, 0, 0);
	const FGridLevelObjectData Teleporter = MakeTD013Object(EGridLevelObjectType::Teleporter, 1, 0);
	const FGridLevelObjectData ItemSpawn = MakeTD013Object(EGridLevelObjectType::ItemSpawn, 2, 0);
	const FGridLevelObjectData Logic = MakeTD013Object(EGridLevelObjectType::Logic, 3, 0);
	const FGridLevelObjectData StoryCompanion = MakeTD013Object(EGridLevelObjectType::StoryCompanion, 0, 1);
	const FGridLevelObjectData CustomRecruiter = MakeTD013Object(EGridLevelObjectType::CustomRecruiter, 1, 1);
	Level->Objects = { Trigger, Teleporter, ItemSpawn, Logic, StoryCompanion, CustomRecruiter };

	Level->Links.Add(MakeTD013Link(Trigger, Teleporter, EGridObjectCommand::Activate));
	Level->Links.Add(MakeTD013Link(Trigger, ItemSpawn, EGridObjectCommand::Activate));
	Level->Links.Add(MakeTD013Link(Trigger, Logic, EGridObjectCommand::LogicExecute));
	Level->Links.Add(MakeTD013Link(Trigger, StoryCompanion, EGridObjectCommand::OfferRecruitment));
	Level->Links.Add(MakeTD013Link(Trigger, CustomRecruiter, EGridObjectCommand::OpenCustomRecruit));

	const TArray<FGridLevelValidationMessage> Messages = EditorActor->ValidateCurrentLevel();
	TestTrue(TEXT("Teleporter StateOnly command is rejected by level validation"), HasUnsupportedCommandDiagnostic(Messages, 0));
	TestTrue(TEXT("ItemSpawn StateOnly command is rejected by level validation"), HasUnsupportedCommandDiagnostic(Messages, 1));
	TestFalse(TEXT("LogicExecute gameplay command is not rejected by level validation"), HasUnsupportedCommandDiagnostic(Messages, 2));
	TestFalse(TEXT("OfferRecruitment gameplay command is not rejected by level validation"), HasUnsupportedCommandDiagnostic(Messages, 3));
	TestFalse(TEXT("OpenCustomRecruit gameplay command is not rejected by level validation"), HasUnsupportedCommandDiagnostic(Messages, 4));

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridLevelVariableTypes.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLevelVariableStore.h"

namespace
{
	FGridLevelVariableDefinition MakeLegacyBoolVariable1924(FName Id, bool bDefault)
	{
		FGridLevelVariableDefinition Definition;
		Definition.VariableId = Id;
		Definition.Type = EGridLevelVariableType::Bool;
		Definition.bDefaultBoolValue = bDefault;
		return Definition;
	}

	FGridLevelVariableDefinition MakeLegacyIntVariable1924(FName Id, int32 DefaultValue)
	{
		FGridLevelVariableDefinition Definition;
		Definition.VariableId = Id;
		Definition.Type = EGridLevelVariableType::Int32;
		Definition.DefaultInt32Value = DefaultValue;
		return Definition;
	}

	struct FLegacyConditionRuntime1924
	{
		UWorld* World = nullptr;
		AGridLevelRuntimeActor* Runtime = nullptr;
		UGridLevelAsset* Level = nullptr;
		UGridActivationComponent* Activation = nullptr;
		FGuid SourceId;
		FGuid TargetId;

		FLegacyConditionRuntime1924()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("LUAUX03_MON1924_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (!World)
			{
				return;
			}
			if (GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}

			Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
			if (!Runtime)
			{
				return;
			}

			Level = NewObject<UGridLevelAsset>(Runtime);
			Level->Width = 1;
			Level->Height = 1;
			Level->EnsureCellCount();
			Level->Cells[0].CellType = EGridCellType::Floor;
			Level->LevelVariables = {
				MakeLegacyBoolVariable1924(TEXT("Gate"), true),
				MakeLegacyIntVariable1924(TEXT("Count"), 99),
				MakeLegacyIntVariable1924(TEXT("Hits"), 0)
			};

			SourceId = FGuid(19, 2, 4, 1);
			FGridWorldObjectInstance Source;
			Source.InstanceId = SourceId;
			Source.Type = EGridLevelObjectType::Trigger;
			Level->WorldObjectInstances.Add(Source);

			TargetId = FGuid(19, 2, 4, 2);
			FGridLogicObjectInstance Target;
			Target.InstanceId = TargetId;
			Target.Type = EGridLevelObjectType::Logic;
			Target.Logic.NodeType = EGridLogicNodeType::AddInt;
			Target.Logic.VariableId = TEXT("Hits");
			Target.Logic.IntValue = 1;
			Level->LogicObjects.Add(Target);

			Runtime->LevelAsset = Level;
			Runtime->CurrentDungeonLevelId = TEXT("LUAUX03_MON1924");
			Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
			if (Activation)
			{
				Activation->Initialize(Runtime);
			}
		}

		~FLegacyConditionRuntime1924()
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

		bool IsValid() const
		{
			return World && Runtime && Level && Activation;
		}

		int32 ReadHits(FAutomationTestBase& Test) const
		{
			FGridLevelRuntimeState* State = Runtime ? Runtime->GetOrCreateRuntimeStateForCurrentLevel() : nullptr;
			if (!State || !Level)
			{
				Test.AddError(TEXT("Missing LUA-UX03 legacy-condition runtime state."));
				return INDEX_NONE;
			}
			FString Error;
			int32 Hits = 0;
			if (!GridLevelVariableStore::TryGetInt32(*Level, *State, TEXT("Hits"), Hits, Error))
			{
				Test.AddError(FString::Printf(TEXT("Unable to read Hits: %s"), *Error));
				return INDEX_NONE;
			}
			return Hits;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON1924LegacyVariableConditionRejectedTest,
	"Grimrock.MON19.2.Runtime.VariableConditions.LegacyConditionsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON1924LegacyVariableConditionRejectedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FLegacyConditionRuntime1924 Fixture;
	if (!Fixture.IsValid())
	{
		AddError(TEXT("Unable to build LUA-UX03 legacy-condition runtime."));
		return false;
	}

	FGridObjectLink BoolLink;
	BoolLink.SourceObjectId = Fixture.SourceId;
	BoolLink.SourceEvent = EGridObjectEvent::Activated;
	BoolLink.TargetObjectId = Fixture.TargetId;
	BoolLink.Command = EGridObjectCommand::LogicExecute;
	BoolLink.Condition = EGridObjectCondition::LevelVariableBoolEquals;
	BoolLink.ConditionVariableId = TEXT("Gate");
	BoolLink.ConditionBoolValue = true;
	Fixture.Level->Links.Add(BoolLink);
	Fixture.Activation->RebuildIndexes();

	TestFalse(TEXT("Legacy Bool LevelVariable connector condition never executes"),
		Fixture.Runtime->ExecuteLinksFromRuntimeObject(Fixture.SourceId, EGridObjectEvent::Activated));
	TestEqual(TEXT("Rejected legacy Bool condition leaves target untouched"), Fixture.ReadHits(*this), 0);

	Fixture.Level->Links.Reset();
	FGridObjectLink IntLink = BoolLink;
	IntLink.Condition = EGridObjectCondition::LevelVariableIntCompare;
	IntLink.ConditionVariableId = TEXT("Count");
	IntLink.ConditionIntComparison = EGridLogicIntComparison::GreaterOrEqual;
	IntLink.ConditionIntValue = 1;
	Fixture.Level->Links.Add(IntLink);
	Fixture.Activation->RebuildIndexes();

	TestFalse(TEXT("Legacy Int LevelVariable connector condition never executes"),
		Fixture.Runtime->ExecuteLinksFromRuntimeObject(Fixture.SourceId, EGridObjectEvent::Activated));
	TestEqual(TEXT("Rejected legacy Int condition leaves target untouched"), Fixture.ReadHits(*this), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

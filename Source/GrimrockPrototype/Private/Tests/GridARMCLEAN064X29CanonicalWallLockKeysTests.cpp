#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridRuntimeWorldObjectData.h"
#include "Runtime/GridWallLockActor.h"
#include "../Runtime/GridWallLockAcceptedKeyReadModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X29CanonicalWallLockKeys,
	"Grimrock.RPG.ARM.CLEAN06_4X29.CanonicalWallLockAcceptedIds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X29CanonicalWallLockKeys::RunTest(const FString&)
{
	FGridLockBehaviorParams Params;
	Params.AcceptedKeyIds.Add(TEXT("Key_Copper"));
	Params.AcceptedKeyIds.Add(TEXT("Key_Copper"));

	TArray<FName> Effective = FGridWallLockAcceptedKeyReadModel::Resolve(Params);
	if (!TestEqual(TEXT("Explicit canonical ids deduplicated"), Effective.Num(), 1) ||
		!TestTrue(TEXT("Only authored copper key is accepted"), Effective.Contains(TEXT("Key_Copper"))) ||
		!TestFalse(TEXT("Unlisted iron key is not accepted"),
			Effective.Contains(TEXT("Key_Iron"))))
	{
		return false;
	}

	const UWorld::InitializationValues Values = UWorld::InitializationValues()
		.AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(false).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false)
		.SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
		FName(*FString::Printf(TEXT("ARM_CLEAN064X29_%s"),
			*FGuid::NewGuid().ToString(EGuidFormats::Digits))),
		nullptr, true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Transient runtime world"), World)) return false;
	if (GEngine)
	{
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	}
	AGridWallLockActor* Lock = World->SpawnActor<AGridWallLockActor>();
	if (!TestNotNull(TEXT("Real runtime wall lock"), Lock))
	{
		World->DestroyWorld(false);
		if (GEngine) GEngine->DestroyWorldContext(World);
		return false;
	}

	FGridRuntimeWorldObjectData Data;
	Data.ObjectId = FGuid::NewGuid();
	Data.Type = EGridLevelObjectType::Receptacle;
	Data.CellX = 1;
	Data.CellY = 1;
	Data.Edge = EGridEdge::North;
	Data.Behavior.Lock = Params;
	Lock->InitializeRuntimeWorldObject(Data, nullptr, FTransform::Identity);
	bool bOk = TestTrue(TEXT("Runtime lock accepts canonical copper ID"),
		Lock->CanAcceptKeyDefinition(TEXT("Key_Copper"))) &&
		TestFalse(TEXT("Runtime lock rejects unlisted iron ID"),
			Lock->CanAcceptKeyDefinition(TEXT("Key_Iron")));

	// An empty explicit list must not inherit any key.
	Data.Behavior.Lock.AcceptedKeyIds.Reset();
	Lock->InitializeRuntimeWorldObject(Data, nullptr, FTransform::Identity);
	bOk &= TestFalse(TEXT("Empty lock rejects iron"),
		Lock->CanAcceptKeyDefinition(TEXT("Key_Iron")));
	bOk &= TestFalse(TEXT("Empty lock rejects copper"),
		Lock->CanAcceptKeyDefinition(TEXT("Key_Copper")));

	// A nonempty but invalid authored list never turns into an implicit
	// implicit acceptance of any other key.
	Data.Behavior.Lock.AcceptedKeyIds.Add(NAME_None);
	Lock->InitializeRuntimeWorldObject(Data, nullptr, FTransform::Identity);
	bOk &= TestFalse(TEXT("Malformed explicit list fails closed for iron"),
		Lock->CanAcceptKeyDefinition(TEXT("Key_Iron")));
	bOk &= TestFalse(TEXT("Malformed explicit list fails closed for copper"),
		Lock->CanAcceptKeyDefinition(TEXT("Key_Copper")));

	World->DestroyWorld(false);
	if (GEngine) GEngine->DestroyWorldContext(World);
	return bOk && !HasAnyErrors();
}
#endif

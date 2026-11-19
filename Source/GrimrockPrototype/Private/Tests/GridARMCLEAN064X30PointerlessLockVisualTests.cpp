#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridWallLockActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "../Runtime/GridLegacyItemLookup.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridItemVisualEffectsComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X30PointerlessLockVisual,
	"Grimrock.RPG.ARM.CLEAN06_4X30.PointerlessCanonicalWallLockInsertedKeyVisual",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X30PointerlessLockVisual::RunTest(const FString&)
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues()
		.AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(false).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false)
		.SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
		FName(*FString::Printf(TEXT("ARM_CLEAN064X30_%s"),
			*FGuid::NewGuid().ToString(EGuidFormats::Digits))),
		nullptr, true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Transient runtime world"), World)) return false;
	if (GEngine)
	{
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	}
	UGameInstance* GI = NewObject<UGameInstance>(GetTransientPackage());
	GI->Init();
	World->SetGameInstance(GI);
	UGridItemSubsystem* Registry = GI->GetSubsystem<UGridItemSubsystem>();
	bool bOk = TestNotNull(TEXT("Configured real registry"), Registry);
	const FName CopperId(TEXT("Key_Copper"));
	FGridResolvedItem Expected;
	bOk &= TestTrue(TEXT("Actual copper key N0 resolves"),
		Registry && Registry->ResolveItem(CopperId, 0, Expected));
	if (!bOk)
	{
		World->SetGameInstance(nullptr);
		GI->Shutdown();
		World->DestroyWorld(false);
		if (GEngine) GEngine->DestroyWorldContext(World);
		return false;
	}
	AGridLevelRuntimeActor* Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
	Level->Width = 2;
	Level->Height = 2;
	Level->EnsureCellCount();
	for (FGridLevelCellData& Cell : Level->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.bBlocksOccupancy = false;
	}
	Runtime->LevelAsset = Level;
	AGrimrockPartyPawn* Party = World->SpawnActor<AGrimrockPartyPawn>();
	Party->LevelRuntimeActor = Runtime;

	FActorSpawnParameters Params;
	Params.Owner = Runtime;
	AGridWallLockActor* Lock = World->SpawnActor<AGridWallLockActor>(
		AGridWallLockActor::StaticClass(), FTransform::Identity, Params);
	bOk &= TestNotNull(TEXT("Runtime wall lock"), Lock);
	FGridRuntimeWorldObjectData Data;
	Data.ObjectId = FGuid::NewGuid();
	Data.Type = EGridLevelObjectType::Receptacle;
	Data.CellX = 1;
	Data.CellY = 1;
	Data.Edge = EGridEdge::North;
	Data.Behavior.Lock.AcceptedKeyIds.Add(CopperId);
	if (Lock) Lock->InitializeRuntimeWorldObject(Data, nullptr, FTransform::Identity);

	bOk &= TestNull(TEXT("No historical copper key definition is discoverable"),
		FGridLegacyItemLookup::Resolve(Party, CopperId));
	FGridItemInstance Key;
	Key.RuntimeObjectId = FGuid::NewGuid();
	Key.ItemDefinitionId = CopperId;
	Key.ItemLevel = 0;
	Key.Quantity = 1;
	Key.OwnerType = EGridItemOwnerType::Cursor;
	Key.bLightsEnabled = Expected.LightEmitter.bDefaultEnabled;

	if (Lock)
	{
		bOk &= TestTrue(TEXT("Pointerless canonical key unlocks a matching lock"),
			Lock->TryUnlockWithContextItem(Party, Key));
		bOk &= TestTrue(TEXT("Lock is unlocked"), Lock->bIsUnlocked);
		bOk &= TestEqual(TEXT("Exactly one inserted key"), Lock->GetContainedItemCount(), 1);
		AGridItemActor* Visual = Lock->GetContainedItemActor(0);
		bOk &= TestNotNull(TEXT("Inserted key actor exists"), Visual);
		if (Visual)
		{
			bOk &= TestNull(TEXT("Inserted key has no historical asset"),
				Visual->GetItemDefinitionAsset());
			bOk &= TestTrue(TEXT("Inserted key has full canonical presentation"),
				Visual->CanonicalWorldPresentation.IsValid());
			bOk &= TestEqual(TEXT("Inserted item keeps real ItemId"),
				Visual->GetItemDefinitionId(), CopperId);
			bOk &= TestEqual(TEXT("Inserted item keeps original physical GUID"),
				Lock->ContainedItems[0].RuntimeObjectId, Key.RuntimeObjectId);
			bOk &= TestTrue(TEXT("Canonical mesh is used"),
				Visual->MeshComponent->GetStaticMesh() == Expected.WorldMesh.LoadSynchronous());
			bOk &= TestEqual(TEXT("Niagara configuration comes from ItemSystem"),
				Visual->VisualEffectsComponent->GetConfiguredEffectCount(),
				Expected.VisualEffects.Effects.Num());
			bOk &= TestEqual(TEXT("Point-light configuration comes from ItemSystem"),
				Visual->LightEmitterComponent->RuntimeConfig.bUsePointLight,
				Expected.LightEmitter.bUsePointLight);
			bOk &= TestFalse(TEXT("Inserted key has no Chaos simulation"),
				Visual->MeshComponent->IsSimulatingPhysics());
			bOk &= TestEqual(TEXT("Inserted key is not a world pickup"),
				Visual->MeshComponent->GetCollisionEnabled(),
				ECollisionEnabled::NoCollision);
		}
	}

	// Invalid registered physical levels fail before changing the lock or
	// creating any visual. Real non-ARM keys can never become N1.
	AGridWallLockActor* InvalidLock = World->SpawnActor<AGridWallLockActor>(
		AGridWallLockActor::StaticClass(), FTransform::Identity, Params);
	if (InvalidLock)
	{
		Data.ObjectId = FGuid::NewGuid();
		InvalidLock->InitializeRuntimeWorldObject(Data, nullptr, FTransform::Identity);
		Key.ItemLevel = 1;
		bOk &= TestFalse(TEXT("Invalid copper key N1 is rejected"),
			InvalidLock->TryUnlockWithContextItem(Party, Key));
		bOk &= TestFalse(TEXT("Failed attempt does not unlock"), InvalidLock->bIsUnlocked);
		bOk &= TestEqual(TEXT("Failed attempt adds no logical key"),
			InvalidLock->GetContainedItemCount(), 0);
	}

	World->SetGameInstance(nullptr);
	GI->Shutdown();
	World->DestroyWorld(false);
	if (GEngine) GEngine->DestroyWorldContext(World);
	return bOk && !HasAnyErrors();
}
#endif

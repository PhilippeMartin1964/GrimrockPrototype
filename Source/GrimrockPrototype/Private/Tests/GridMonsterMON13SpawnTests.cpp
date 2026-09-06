#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/AnimInstance.h"
#include "Core/GridDirectionUtils.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterCombatComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Save/GrimrockPartySaveGame.h"

namespace
{
	UGridLevelAsset* MakeMON13Level(UObject* Outer)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Outer);
		Level->Width = 4;
		Level->Height = 4;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		return Level;
	}

	UGridMonsterDefinitionAsset* MakeMON13Definition(UObject* Outer, FName MonsterId = TEXT("MON13_Rat"))
	{
		UGridMonsterDefinitionAsset* Definition = NewObject<UGridMonsterDefinitionAsset>(Outer);
		Definition->MonsterId = MonsterId;
		Definition->DisplayName = FText::FromString(TEXT("Rat MON13"));
		Definition->CategoryId = TEXT("Vermin");
		Definition->DangerLevel = 1;
		Definition->MaxHealth = 12;
		Definition->ActionPointsPerTurn = 2;
		Definition->GridFootprint = FIntPoint(1, 1);
		Definition->DeathExpectedDuration = 1.0f;
		return Definition;
	}

	UGridMonsterDefinitionAsset* MakeMON13RuntimeDefinition(FAutomationTestBase& Test, UObject* Outer, FName MonsterId)
	{
		USkeletalMesh* SkeletalMesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Meshes/SK_RatGiant.SK_RatGiant"));
		UClass* AnimationClass =
			LoadClass<UAnimInstance>(nullptr, TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Animation/ABP_MON_RatGiant.ABP_MON_RatGiant_C"));
		UClass* MonsterActorClass =
			LoadClass<AGridMonsterActor>(nullptr, TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Blueprints/BP_MON_RatGiant.BP_MON_RatGiant_C"));

		Test.TestNotNull(TEXT("MON13.2 runtime fixture loads the Rat Giant skeletal mesh"), SkeletalMesh);
		Test.TestNotNull(TEXT("MON13.2 runtime fixture loads the Rat Giant animation class"), AnimationClass);
		Test.TestNotNull(TEXT("MON13 runtime fixture loads the combat-ready Rat Giant Actor class"), MonsterActorClass);
		if (!SkeletalMesh || !AnimationClass || !MonsterActorClass)
		{
			return nullptr;
		}

		UGridMonsterDefinitionAsset* Definition = MakeMON13Definition(Outer, MonsterId);
		Definition->SkeletalMesh = TSoftObjectPtr<USkeletalMesh>(SkeletalMesh);
		Definition->AnimationClass = AnimationClass;
		Definition->MonsterActorClass = MonsterActorClass;
		return Definition;
	}

	FGridLevelObjectData MakeMON13Spawn(UGridMonsterDefinitionAsset* Definition, FGuid SpawnId, FIntPoint Cell = FIntPoint(1, 1))
	{
		FGridLevelObjectData Spawn;
		Spawn.ObjectId = SpawnId;
		Spawn.Type = EGridLevelObjectType::MonsterSpawn;
		Spawn.CellX = Cell.X;
		Spawn.CellY = Cell.Y;
		Spawn.Edge = EGridEdge::None;
		Spawn.InitialFacing = EGridEdge::North;
		Spawn.MonsterDefinitionAsset = Definition;
		Spawn.MonsterDefinitionId = Definition ? Definition->MonsterId : NAME_None;
		Spawn.EncounterGroupId = TEXT("Encounter_MON13");
		Spawn.bInitiallyEnabled = true;
		return Spawn;
	}

	UGridObjectArchetypeAsset* GridMonsterMON13MakeLeverMarkerArchetype(UObject* Outer, FName ArchetypeId)
	{
		UGridObjectArchetypeAsset* Archetype = NewObject<UGridObjectArchetypeAsset>(Outer);
		Archetype->ArchetypeId = ArchetypeId;
		Archetype->SupportedType = EGridLevelObjectType::Lever;
		Archetype->Category = TEXT("Mechanisms");
		Archetype->ObjectCategory = EGridObjectCategory::Mechanism;
		Archetype->PlacementKind = EGridObjectPlacementKind::Wall;
		Archetype->RuntimeActorClass = AGridLeverActor::StaticClass();
		Archetype->MovingParts.Part0.Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		Archetype->bIsInteractable = true;
		return Archetype;
	}

	bool HasErrorContaining(const TArray<FString>& Errors, const TCHAR* ExpectedText)
	{
		return Errors.ContainsByPredicate(
			[ExpectedText](const FString& Error)
			{
				return Error.Contains(ExpectedText);
			});
	}

	struct FGridMON132TestWorld
	{
		UWorld* World = nullptr;

		explicit FGridMON132TestWorld(EWorldType::Type WorldType)
		{
			const UWorld::InitializationValues InitializationValues = UWorld::InitializationValues()
																		  .AllowAudioPlayback(false)
																		  .RequiresHitProxies(false)
																		  .CreatePhysicsScene(false)
																		  .CreateNavigation(false)
																		  .CreateAISystem(false)
																		  .ShouldSimulatePhysics(false)
																		  .SetTransactional(false);
			World = UWorld::CreateWorld(WorldType, false, FName(*FString::Printf(TEXT("MON132TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &InitializationValues);
			if (!World || !GEngine)
			{
				return;
			}

			FWorldContext& Context = GEngine->CreateNewWorldContext(WorldType);
			Context.SetCurrentWorld(World);
		}

		~FGridMON132TestWorld()
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

	int32 CountMON132WorldMonsters(UWorld* World)
	{
		int32 Count = 0;
		if (!World)
		{
			return Count;
		}
		for (TActorIterator<AGridMonsterActor> It(World); It; ++It)
		{
			Count += IsValid(*It) ? 1 : 0;
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON131PersistentModelTest, "Grimrock.Monsters.MON13.1.PersistentModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON131PersistentModelTest::RunTest(const FString& Parameters)
{
	UGridLevelAsset* Level = MakeMON13Level(GetTransientPackage());
	UGridMonsterDefinitionAsset* Definition = MakeMON13Definition(Level);

	TestEqual(TEXT("MonsterDefinition defaults to the native monster actor"), Definition->MonsterActorClass.Get(), AGridMonsterActor::StaticClass());

	FGridLevelObjectData Spawn = MakeMON13Spawn(Definition, FGuid(), FIntPoint(2, 1));
	Spawn.MonsterDefinitionId = NAME_None;
	Spawn.InitialFacing = EGridEdge::East;
	Spawn.LocalYaw = 0.0f;

	const FGuid SpawnId = Level->AddObject(Spawn);
	const FGridLevelObjectData* StoredSpawn = Level->FindMonsterSpawnById(SpawnId);

	TestTrue(TEXT("AddObject creates a stable SpawnId"), SpawnId.IsValid());
	TestNotNull(TEXT("Spawn is found by its persistent id"), StoredSpawn);
	if (!StoredSpawn)
	{
		return false;
	}

	TestEqual(TEXT("Explicit InitialFacing remains authoritative"), StoredSpawn->InitialFacing, EGridEdge::East);
	TestEqual(TEXT("Preview LocalYaw is synchronized from InitialFacing"), StoredSpawn->LocalYaw, 90.0f);
	TestTrue(TEXT("Authoring MonsterDefinitionId mirror remains empty"), StoredSpawn->MonsterDefinitionId.IsNone());
	TestEqual(TEXT("Encounter id remains persistent"), StoredSpawn->EncounterGroupId, FName(TEXT("Encounter_MON13")));
	TestTrue(TEXT("Initial enabled state remains persistent"), StoredSpawn->bInitiallyEnabled);

	TArray<FString> Errors;
	TestTrue(TEXT("Complete MonsterSpawn model validates"), Level->ValidateMonsterSpawns(Errors));
	TestEqual(TEXT("Valid model has no errors"), Errors.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON131ValidationTest, "Grimrock.Monsters.MON13.1.Validation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON131ValidationTest::RunTest(const FString& Parameters)
{
	UGridLevelAsset* Level = MakeMON13Level(GetTransientPackage());
	UGridMonsterDefinitionAsset* Definition = MakeMON13Definition(Level);
	const FGuid SharedId(13, 1, 1, 1);

	FGridLevelObjectData Valid = MakeMON13Spawn(Definition, SharedId, FIntPoint(1, 1));
	Level->Objects.Add(Valid);

	FGridLevelObjectData DuplicateId = MakeMON13Spawn(Definition, SharedId, FIntPoint(2, 1));
	DuplicateId.bInitiallyEnabled = false;
	Level->Objects.Add(DuplicateId);

	FGridLevelObjectData DuplicateCell = MakeMON13Spawn(Definition, FGuid(13, 1, 1, 2), FIntPoint(1, 1));
	Level->Objects.Add(DuplicateCell);

	FGridLevelObjectData InvalidPlacement = MakeMON13Spawn(nullptr, FGuid(13, 1, 1, 3), FIntPoint(3, 3));
	InvalidPlacement.Edge = EGridEdge::North;
	InvalidPlacement.InitialFacing = EGridEdge::None;
	InvalidPlacement.MonsterDefinitionId = NAME_None;
	Level->Cells[Level->GetIndex(3, 3)].bBlocksOccupancy = true;
	Level->Objects.Add(InvalidPlacement);

	FGridLevelObjectData MismatchedDefinition = MakeMON13Spawn(Definition, FGuid(13, 1, 1, 4), FIntPoint(0, 1));
	MismatchedDefinition.MonsterDefinitionId = TEXT("MON13_Other");
	Level->Objects.Add(MismatchedDefinition);

	FGridLevelObjectData Outside = MakeMON13Spawn(Definition, FGuid(13, 1, 1, 5), FIntPoint(8, 8));
	Level->Objects.Add(Outside);

	TArray<FString> Errors;
	TestFalse(TEXT("Invalid MonsterSpawn set is rejected"), Level->ValidateMonsterSpawns(Errors));
	TestTrue(TEXT("Duplicate SpawnId is reported"), HasErrorContaining(Errors, TEXT("unique ObjectId/SpawnId")));
	TestTrue(TEXT("Duplicate enabled cell is reported"), HasErrorContaining(Errors, TEXT("shares initial cell")));
	TestTrue(TEXT("Blocked cell is reported"), HasErrorContaining(Errors, TEXT("allows occupancy")));
	TestTrue(TEXT("Edge placement is rejected"), HasErrorContaining(Errors, TEXT("requires Edge=None")));
	TestTrue(TEXT("Non-cardinal facing is rejected"), HasErrorContaining(Errors, TEXT("cardinal InitialFacing")));
	TestTrue(TEXT("Missing definition is reported"), HasErrorContaining(Errors, TEXT("requires MonsterDefinitionAsset")));
	TestTrue(TEXT("Definition id mismatch is reported"), HasErrorContaining(Errors, TEXT("asset resolves")));
	TestTrue(TEXT("Out-of-bounds spawn is reported"), HasErrorContaining(Errors, TEXT("outside grid bounds")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON131PaletteContractTest, "Grimrock.Monsters.MON13.1.PaletteContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON131PaletteContractTest::RunTest(const FString& Parameters)
{
	UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>(GetTransientPackage());
	UGridObjectArchetypeAsset* Archetype = NewObject<UGridObjectArchetypeAsset>(Palette);
	Archetype->ArchetypeId = TEXT("Monster_RatGiant");
	Archetype->SupportedType = EGridLevelObjectType::MonsterSpawn;
	Archetype->PlacementKind = EGridObjectPlacementKind::Center;

	FGridObjectPaletteEntry Entry;
	Entry.EntryId = TEXT("MON_RatGiant");
	Entry.DefaultArchetype = Archetype;
	Palette->Entries.Add(Entry);

	TArray<FGridArchetypeValidationMessage> Messages;
	TestFalse(TEXT("Monster palette entry requires a definition"), Palette->ValidatePalette(Messages));

	Palette->Entries[0].DefaultMonsterDefinition = MakeMON13Definition(Palette, TEXT("MON_RatGiant"));
	TestTrue(TEXT("Complete monster palette entry validates"), Palette->ValidatePalette(Messages));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON132RuntimePipelineTest, "Grimrock.Monsters.MON13.2.RuntimePipeline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON132RuntimePipelineTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON132TestWorld TestWorld(EWorldType::Game);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	TestNotNull(TEXT("Runtime actor is created"), Runtime);
	if (!Runtime)
	{
		return false;
	}

	UGridLevelAsset* Level = MakeMON13Level(Runtime);
	Runtime->LevelAsset = Level;
	UGridMonsterDefinitionAsset* Definition = MakeMON13RuntimeDefinition(*this, Runtime, TEXT("MON132_Rat"));
	if (!Definition)
	{
		return false;
	}
	const FGuid SpawnId(13, 2, 1, 1);
	FGridLevelObjectData Spawn = MakeMON13Spawn(Definition, SpawnId, FIntPoint(2, 1));
	Spawn.InitialFacing = EGridEdge::East;
	Spawn.LocalYaw = 0.0f;
	Spawn.EncounterGroupId = TEXT("Encounter_MON132");
	Level->Objects.Add(Spawn);

	Runtime->RebuildLevel();
	AGridMonsterActor* FirstMonster = Runtime->FindSpawnedMonsterActor(SpawnId);
	TestNotNull(TEXT("MonsterSpawn creates its Actor"), FirstMonster);
	if (!FirstMonster)
	{
		return false;
	}

	TestEqual(TEXT("Exactly one placement Actor is tracked"), Runtime->GetSpawnedMonsterActorCount(), 1);
	TestEqual(TEXT("Valid spawn reports no failure"), Runtime->GetMonsterSpawnFailureCount(), 0);
	TestEqual(TEXT("Definition is transmitted"), FirstMonster->MonsterDefinition.Get(), Definition);
	TestEqual(TEXT("Definition chooses the spawned Actor class"), FirstMonster->GetClass(), Definition->MonsterActorClass.Get());
	TestNotNull(TEXT("Spawned Actor has MonsterMovement"), FirstMonster->FindComponentByClass<UGridMonsterMovementComponent>());
	TestNotNull(TEXT("Spawned Actor has MonsterBehavior"), FirstMonster->FindComponentByClass<UGridMonsterBehaviorComponent>());
	TestNotNull(TEXT("Spawned Actor has MonsterCombat"), FirstMonster->FindComponentByClass<UGridMonsterCombatComponent>());
	TestEqual(TEXT("SpawnId remains the persistence id"), FirstMonster->ResolvePersistenceId(), SpawnId);
	TestTrue(TEXT("Actor identity comes from MonsterSpawn"), FirstMonster->HasMonsterSpawnIdentity());
	TestEqual(TEXT("Cell is transmitted"), FirstMonster->CurrentCell, FIntPoint(2, 1));
	TestEqual(TEXT("InitialFacing is authoritative"), FirstMonster->Facing, EGridEdge::East);
	TestEqual(TEXT("Encounter group is transmitted"), FirstMonster->EncounterGroupId, FName(TEXT("Encounter_MON132")));
	TestEqual(TEXT("Combat health is initialized"), FirstMonster->CurrentHealth, Definition->MaxHealth);
	TestTrue(TEXT("Combat stats are initialized"), FirstMonster->bCombatStatsInitialized);
	TestTrue(TEXT("Actor is centered on its cell"), FirstMonster->GetActorLocation().Equals(Runtime->GetCellCenterWorld(2, 1), KINDA_SMALL_NUMBER));
	TestTrue(
		TEXT("Actor rotation follows InitialFacing"), FMath::IsNearlyEqual(FirstMonster->GetActorRotation().Yaw, GridDirectionUtils::ToYaw(EGridEdge::East)));

	Runtime->RebuildLevel();
	AGridMonsterActor* RebuiltMonster = Runtime->FindSpawnedMonsterActor(SpawnId);
	TestNotNull(TEXT("Rebuild recreates the placement Actor"), RebuiltMonster);
	TestTrue(TEXT("Rebuild replaces rather than duplicates"), RebuiltMonster != FirstMonster);
	TestFalse(TEXT("Previous generated Actor is destroyed"), IsValid(FirstMonster));
	TestEqual(TEXT("Rebuild still tracks one Actor"), Runtime->GetSpawnedMonsterActorCount(), 1);
	TestEqual(TEXT("Only one live monster remains in the world"), CountMON132WorldMonsters(TestWorld.World), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON135ProductionAssetContractTest, "Grimrock.Monsters.MON13.5.ProductionAssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON135ProductionAssetContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridMonsterDefinitionAsset* RatDefinition =
		LoadObject<UGridMonsterDefinitionAsset>(nullptr, TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.DA_MON_RatGiant"));
	UClass* ExpectedRatClass =
		LoadClass<AGridMonsterActor>(nullptr, TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Blueprints/BP_MON_RatGiant.BP_MON_RatGiant_C"));

	TestNotNull(TEXT("Production Rat definition loads"), RatDefinition);
	TestNotNull(TEXT("Production Rat Actor class loads"), ExpectedRatClass);
	if (!RatDefinition || !ExpectedRatClass)
	{
		return false;
	}

	TestEqual(TEXT("DA_MON_RatGiant uses BP_MON_RatGiant_C"), RatDefinition->MonsterActorClass.Get(), ExpectedRatClass);
	TestNotEqual(TEXT("Production Rat never falls back to the native base Actor"), RatDefinition->MonsterActorClass.Get(), AGridMonsterActor::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON132AtomicFailureTest, "Grimrock.Monsters.MON13.2.AtomicFailure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON132AtomicFailureTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON132TestWorld TestWorld(EWorldType::Game);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	UGridLevelAsset* Level = MakeMON13Level(Runtime);
	Runtime->LevelAsset = Level;
	UGridMonsterDefinitionAsset* Definition = MakeMON13RuntimeDefinition(*this, Runtime, TEXT("MON132_AtomicRat"));
	if (!Definition)
	{
		return false;
	}

	const FGuid ValidId(13, 2, 2, 1);
	Level->Objects.Add(MakeMON13Spawn(Definition, ValidId, FIntPoint(0, 0)));

	FGridLevelObjectData Mismatched = MakeMON13Spawn(Definition, FGuid(13, 2, 2, 2), FIntPoint(1, 0));
	Mismatched.MonsterDefinitionId = TEXT("MON132_WrongId");
	Level->Objects.Add(Mismatched);

	Level->Objects.Add(MakeMON13Spawn(Definition, ValidId, FIntPoint(2, 0)));

	FGridLevelObjectData DuplicateCell = MakeMON13Spawn(Definition, FGuid(13, 2, 2, 5), FIntPoint(0, 0));
	Level->Objects.Add(DuplicateCell);

	FGridLevelObjectData Blocked = MakeMON13Spawn(Definition, FGuid(13, 2, 2, 3), FIntPoint(3, 0));
	Level->Cells[Level->GetIndex(3, 0)].bBlocksOccupancy = true;
	Level->Objects.Add(Blocked);

	FGridLevelObjectData Disabled = MakeMON13Spawn(nullptr, FGuid(13, 2, 2, 4), FIntPoint(0, 1));
	Disabled.bInitiallyEnabled = false;
	Level->Objects.Add(Disabled);

	AddExpectedError(TEXT("[GridMonsterSpawn] Skipped"), EAutomationExpectedErrorFlags::Contains, 4, false);
	Runtime->RebuildLevel();
	TestEqual(TEXT("Only the valid placement creates an Actor"), Runtime->GetSpawnedMonsterActorCount(), 1);
	TestEqual(TEXT("Every enabled invalid placement is counted"), Runtime->GetMonsterSpawnFailureCount(), 4);
	TestNotNull(TEXT("Valid placement remains available"), Runtime->FindSpawnedMonsterActor(ValidId));
	TestNull(TEXT("Mismatched definition creates no partial Actor"), Runtime->FindSpawnedMonsterActor(Mismatched.ObjectId));
	TestNull(TEXT("Blocked placement creates no partial Actor"), Runtime->FindSpawnedMonsterActor(Blocked.ObjectId));
	TestNull(TEXT("Occupied placement creates no partial Actor"), Runtime->FindSpawnedMonsterActor(DuplicateCell.ObjectId));
	TestNull(TEXT("Disabled placement creates no Actor"), Runtime->FindSpawnedMonsterActor(Disabled.ObjectId));
	TestEqual(TEXT("Invalid placements leak no world Actor"), CountMON132WorldMonsters(TestWorld.World), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON132EditorPreviewTest, "Grimrock.Monsters.MON13.2.EditorPreview", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON132EditorPreviewTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON132TestWorld TestWorld(EWorldType::Editor);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	UGridLevelAsset* Level = MakeMON13Level(Runtime);
	Runtime->LevelAsset = Level;
	UGridMonsterDefinitionAsset* Definition = MakeMON13RuntimeDefinition(*this, Runtime, TEXT("MON132_PreviewRat"));
	if (!Definition)
	{
		return false;
	}
	USkeletalMesh* PreviewMesh = Definition->SkeletalMesh.Get();
	Definition->VisualOffset = FVector(1.0f, 2.0f, 3.0f);
	Definition->VisualScale = FVector(1.5f);

	const FGuid SpawnId(13, 2, 3, 1);
	FGridLevelObjectData Spawn = MakeMON13Spawn(Definition, SpawnId, FIntPoint(1, 2));
	Spawn.InitialFacing = EGridEdge::South;
	Level->Objects.Add(Spawn);

	Runtime->RebuildLevel();
	AGridEditorPreviewObjectActor* PreviewActor = nullptr;
	for (TActorIterator<AGridEditorPreviewObjectActor> It(TestWorld.World); It; ++It)
	{
		if (IsValid(*It) && It->GetOwner() == Runtime && It->ObjectId == SpawnId)
		{
			PreviewActor = *It;
			break;
		}
	}

	TestNotNull(TEXT("Editor rebuild creates a monster preview"), PreviewActor);
	if (!PreviewActor)
	{
		return false;
	}
	TestNotNull(TEXT("Monster preview has a skeletal component"), PreviewActor->SkeletalMeshComponent);
	TestEqual(TEXT("Preview resolves the definition skeletal mesh"), PreviewActor->SkeletalMeshComponent->GetSkeletalMeshAsset(), PreviewMesh);
	TestTrue(TEXT("Preview is centered on the placement cell"), PreviewActor->GetActorLocation().Equals(Runtime->GetCellCenterWorld(1, 2), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Preview rotation follows InitialFacing"),
		FMath::IsNearlyEqual(PreviewActor->GetActorRotation().Yaw, GridDirectionUtils::ToYaw(EGridEdge::South)));
	TestEqual(TEXT("Preview receives the visual offset"), PreviewActor->SkeletalMeshComponent->GetRelativeLocation(), Definition->VisualOffset);
	TestEqual(TEXT("Preview receives the visual scale"), PreviewActor->SkeletalMeshComponent->GetRelativeScale3D(), Definition->VisualScale);
	Runtime->SetEditorSelectedObject(SpawnId);
	TestTrue(TEXT("Selection stencil reaches the skeletal preview"), PreviewActor->SkeletalMeshComponent->bRenderCustomDepth);
	TestEqual(TEXT("Editor preview creates no gameplay monster"), CountMON132WorldMonsters(TestWorld.World), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON133DeferredSpawnLinksTest, "Grimrock.Monsters.MON13.3.DeferredSpawnLinks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON133DeferredSpawnLinksTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON132TestWorld TestWorld(EWorldType::PIE);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	UGridLevelAsset* Level = MakeMON13Level(Runtime);
	Runtime->LevelAsset = Level;
	UGridActivationComponent* Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
	TestNotNull(TEXT("Runtime activation component exists"), Activation);
	if (!Activation)
	{
		return false;
	}
	Level->StartCellX = 0;
	Level->StartCellY = 0;
	Level->StartFacing = EGridEdge::North;

	UGridMonsterDefinitionAsset* FirstDefinition = MakeMON13RuntimeDefinition(*this, Runtime, TEXT("MON133_FirstRat"));
	UGridMonsterDefinitionAsset* SecondDefinition = MakeMON13RuntimeDefinition(*this, Runtime, TEXT("MON133_SecondRat"));
	if (!FirstDefinition || !SecondDefinition)
	{
		return false;
	}

	const FGuid TriggerId(13, 3, 1, 1);
	const FGuid FirstSpawnId(13, 3, 1, 2);
	const FGuid SecondSpawnId(13, 3, 1, 3);
	const FGuid SpawnEventMarkerId(13, 3, 1, 4);
	const FName SpawnEventMarkerArchetypeId(TEXT("MON133_EventMarkerLever"));

	FGridLevelObjectData Trigger;
	Trigger.ObjectId = TriggerId;
	Trigger.Type = EGridLevelObjectType::Trigger;
	Trigger.CellX = 3;
	Trigger.CellY = 3;
	Trigger.Edge = EGridEdge::None;
	Trigger.bInitiallyEnabled = true;
	Level->Objects.Add(Trigger);

	FGridLevelObjectData FirstSpawn = MakeMON13Spawn(FirstDefinition, FirstSpawnId, FIntPoint(0, 1));
	FirstSpawn.bInitiallyEnabled = false;
	FirstSpawn.EncounterGroupId = TEXT("Encounter_MON133_Rats");
	Level->Objects.Add(FirstSpawn);

	FGridLevelObjectData SecondSpawn = MakeMON13Spawn(SecondDefinition, SecondSpawnId, FIntPoint(1, 0));
	SecondSpawn.bInitiallyEnabled = false;
	SecondSpawn.EncounterGroupId = TEXT("Encounter_MON133_Rats");
	Level->Objects.Add(SecondSpawn);

	FGridLevelObjectData SpawnEventMarker;
	SpawnEventMarker.ObjectId = SpawnEventMarkerId;
	SpawnEventMarker.ArchetypeId = SpawnEventMarkerArchetypeId;
	SpawnEventMarker.Type = EGridLevelObjectType::Lever;
	SpawnEventMarker.CellX = 2;
	SpawnEventMarker.CellY = 3;
	SpawnEventMarker.Edge = EGridEdge::North;
	SpawnEventMarker.bInitiallyEnabled = true;
	SpawnEventMarker.bInitiallyActive = false;
	Level->Objects.Add(SpawnEventMarker);
	Runtime->ObjectArchetypes.Add(GridMonsterMON13MakeLeverMarkerArchetype(Runtime, SpawnEventMarkerArchetypeId));

	FGridObjectLink TriggerLink;
	TriggerLink.SourceObjectId = TriggerId;
	TriggerLink.TargetObjectId = FirstSpawnId;
	TriggerLink.SourceEvent = EGridObjectEvent::Activated;
	TriggerLink.Command = EGridObjectCommand::Spawn;
	Level->Links.Add(TriggerLink);

	FGridObjectLink CascadeLink;
	CascadeLink.SourceObjectId = FirstSpawnId;
	CascadeLink.TargetObjectId = SpawnEventMarkerId;
	CascadeLink.SourceEvent = EGridObjectEvent::MonsterSpawned;
	CascadeLink.Command = EGridObjectCommand::Toggle;
	Level->Links.Add(CascadeLink);

	FGridObjectLink TeleportCascadeLink;
	TeleportCascadeLink.SourceObjectId = FirstSpawnId;
	TeleportCascadeLink.TargetObjectId = SecondSpawnId;
	TeleportCascadeLink.SourceEvent = EGridObjectEvent::MonsterTeleported;
	TeleportCascadeLink.Command = EGridObjectCommand::Despawn;
	Level->Links.Add(TeleportCascadeLink);

	FGridObjectLink DespawnCascadeLink;
	DespawnCascadeLink.SourceObjectId = FirstSpawnId;
	DespawnCascadeLink.TargetObjectId = SecondSpawnId;
	DespawnCascadeLink.SourceEvent = EGridObjectEvent::MonsterDespawned;
	DespawnCascadeLink.Command = EGridObjectCommand::Spawn;
	Level->Links.Add(DespawnCascadeLink);

	Runtime->RebuildLevel();
	TestEqual(TEXT("Deferred-spawn fixture uses a PIE world"), TestWorld.World->WorldType, EWorldType::PIE);
	TestEqual(TEXT("Disabled placements start absent"), Runtime->GetSpawnedMonsterActorCount(), 0);
	TestNull(TEXT("The first SpawnId is absent after RebuildLevel"), Runtime->FindSpawnedMonsterActor(FirstSpawnId));
	Runtime->HandlePartyCellChanged(Level->StartCellX, Level->StartCellY, Level->StartCellX, Level->StartCellY);
	TestEqual(TEXT("Startup notification away from the trigger spawns nothing"), Runtime->GetSpawnedMonsterActorCount(), 0);
	TestNull(TEXT("The first SpawnId remains absent on StartCell"), Runtime->FindSpawnedMonsterActor(FirstSpawnId));

	Runtime->HandlePartyCellChanged(Level->StartCellX, Level->StartCellY, Trigger.CellX, Trigger.CellY);
	TestEqual(TEXT("Entering the trigger cell creates one Actor"), Runtime->GetSpawnedMonsterActorCount(), 1);

	AGridMonsterActor* FirstMonster = Runtime->FindSpawnedMonsterActor(FirstSpawnId);
	AGridMonsterActor* SecondMonster = Runtime->FindSpawnedMonsterActor(SecondSpawnId);
	TestNotNull(TEXT("First deferred monster exists"), FirstMonster);
	TestNull(TEXT("Unrelated disabled placement remains absent"), SecondMonster);
	TestTrue(TEXT("MonsterSpawned executes its linked marker command"), Activation->GetActiveObjectIds().Contains(SpawnEventMarkerId));
	if (!FirstMonster)
	{
		return false;
	}
	TestEqual(TEXT("Encounter group reaches the first monster"), FirstMonster->EncounterGroupId, FName(TEXT("Encounter_MON133_Rats")));

	const FGridLevelRuntimeState* SpawnedState = Runtime->DungeonRuntimeState.LevelStates.Find(TEXT("SingleLevel"));
	const FGridRuntimeMonsterPlacementState* SpawnedPlacement = SpawnedState ? SpawnedState->MonsterPlacements.Find(FirstSpawnId) : nullptr;
	TestNotNull(TEXT("A genuine Spawn stores MonsterPlacements state"), SpawnedPlacement);
	if (SpawnedPlacement)
	{
		TestTrue(TEXT("Persisted placement is spawned"), SpawnedPlacement->bIsSpawned);
		TestTrue(TEXT("Persisted placement has monster state"), SpawnedPlacement->bHasMonsterState);
	}

	Runtime->HandlePartyCellChanged(Level->StartCellX, Level->StartCellY, Trigger.CellX, Trigger.CellY);
	TestEqual(TEXT("A second trigger notification creates no duplicate"), Runtime->GetSpawnedMonsterActorCount(), 1);
	TestTrue(TEXT("Idempotent Spawn does not emit MonsterSpawned twice"), Activation->GetActiveObjectIds().Contains(SpawnEventMarkerId));

	TestTrue(TEXT("Teleport emits its lifecycle event"), Runtime->TeleportSpawnedMonster(FirstSpawnId, 2, 2, EGridEdge::South));
	TestNull(TEXT("MonsterTeleported executes the linked Despawn"), Runtime->FindSpawnedMonsterActor(SecondSpawnId));
	TestTrue(TEXT("Teleport command returns to the placement pose"), Runtime->ExecuteMonsterSpawnCommand(FirstSpawnId, EGridObjectCommand::Teleport));
	TestEqual(TEXT("Teleport command restores the placement cell"), FirstMonster->CurrentCell, FIntPoint(0, 1));
	TestEqual(TEXT("Teleport command restores the placement facing"), FirstMonster->Facing, EGridEdge::North);
	TestTrue(TEXT("Despawn emits its lifecycle event"), Runtime->ExecuteMonsterSpawnCommand(FirstSpawnId, EGridObjectCommand::Despawn));
	TestNull(TEXT("The first placement remains despawned"), Runtime->FindSpawnedMonsterActor(FirstSpawnId));
	TestNotNull(TEXT("MonsterDespawned executes the linked Spawn"), Runtime->FindSpawnedMonsterActor(SecondSpawnId));
	SecondMonster = Runtime->FindSpawnedMonsterActor(SecondSpawnId);
	if (SecondMonster)
	{
		TestEqual(TEXT("Encounter group reaches the lifecycle-spawned monster"), SecondMonster->EncounterGroupId, FName(TEXT("Encounter_MON133_Rats")));
	}

	Runtime->RebuildLevel();
	TestEqual(TEXT("Cascaded lifecycle presence survives a rebuild"), Runtime->GetSpawnedMonsterActorCount(), 1);
	TestNull(TEXT("Rebuild preserves the first despawn"), Runtime->FindSpawnedMonsterActor(FirstSpawnId));
	TestNotNull(TEXT("Rebuild preserves the second spawn"), Runtime->FindSpawnedMonsterActor(SecondSpawnId));

	const FGridDungeonRuntimeState ContinueState = Runtime->DungeonRuntimeState;
	Runtime->DungeonRuntimeState = FGridDungeonRuntimeState();
	Runtime->RebuildLevel();
	TestEqual(TEXT("A fresh playtest does not restore prior placements"), Runtime->GetSpawnedMonsterActorCount(), 0);
	TestNull(TEXT("Fresh playtest keeps the first SpawnId absent"), Runtime->FindSpawnedMonsterActor(FirstSpawnId));
	TestNull(TEXT("Fresh playtest keeps the second SpawnId absent"), Runtime->FindSpawnedMonsterActor(SecondSpawnId));

	Runtime->DungeonRuntimeState = ContinueState;
	Runtime->RebuildLevel();
	TestEqual(TEXT("Continue restores persisted placement presence"), Runtime->GetSpawnedMonsterActorCount(), 1);
	TestNotNull(TEXT("Continue restores the lifecycle-spawned monster"), Runtime->FindSpawnedMonsterActor(SecondSpawnId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON133LifecyclePersistenceTest, "Grimrock.Monsters.MON13.3.LifecyclePersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON133LifecyclePersistenceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON132TestWorld TestWorld(EWorldType::Game);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	UGridLevelAsset* Level = MakeMON13Level(Runtime);
	Runtime->LevelAsset = Level;
	UGridMonsterDefinitionAsset* Definition = MakeMON13RuntimeDefinition(*this, Runtime, TEXT("MON133_PersistentRat"));
	if (!Definition)
	{
		return false;
	}

	const FGuid SpawnId(13, 3, 2, 1);
	FGridLevelObjectData Spawn = MakeMON13Spawn(Definition, SpawnId, FIntPoint(0, 0));
	Spawn.EncounterGroupId = TEXT("Encounter_MON133_Persistent");
	Level->Objects.Add(Spawn);

	Runtime->RebuildLevel();
	AGridMonsterActor* InitialMonster = Runtime->FindSpawnedMonsterActor(SpawnId);
	TestNotNull(TEXT("Initial placement is spawned"), InitialMonster);
	if (!InitialMonster)
	{
		return false;
	}
	InitialMonster->CurrentHealth = 7;

	TestTrue(TEXT("Teleport accepts a free walkable cell"), Runtime->TeleportSpawnedMonster(SpawnId, 2, 2, EGridEdge::South));
	TestTrue(TEXT("Despawn stores and removes the Actor"), Runtime->ExecuteMonsterSpawnCommand(SpawnId, EGridObjectCommand::Despawn));
	TestEqual(TEXT("Despawn leaves no generated Actor"), Runtime->GetSpawnedMonsterActorCount(), 0);

	Runtime->RebuildLevel();
	TestEqual(TEXT("A rebuild preserves the despawned state"), Runtime->GetSpawnedMonsterActorCount(), 0);

	TestTrue(TEXT("Spawn restores the saved placement state"), Runtime->ExecuteMonsterSpawnCommand(SpawnId, EGridObjectCommand::Spawn));
	AGridMonsterActor* RestoredMonster = Runtime->FindSpawnedMonsterActor(SpawnId);
	TestNotNull(TEXT("Spawn recreates the Actor"), RestoredMonster);
	if (!RestoredMonster)
	{
		return false;
	}
	TestEqual(TEXT("Teleport cell survives despawn and spawn"), RestoredMonster->CurrentCell, FIntPoint(2, 2));
	TestEqual(TEXT("Teleport facing survives despawn and spawn"), RestoredMonster->Facing, EGridEdge::South);
	TestEqual(TEXT("Health survives despawn and spawn"), RestoredMonster->CurrentHealth, 7);
	TestEqual(TEXT("Encounter group survives despawn and spawn"), RestoredMonster->EncounterGroupId, FName(TEXT("Encounter_MON133_Persistent")));

	TestTrue(TEXT("Lifecycle state can be captured for persistence"), Runtime->CaptureCurrentLevelRuntimeState());
	UGrimrockPartySaveGame* SourceSave = NewObject<UGrimrockPartySaveGame>(Runtime);
	SourceSave->DungeonRuntimeState = Runtime->DungeonRuntimeState;
	TArray<uint8> SaveBytes;
	TestTrue(TEXT("Lifecycle state serializes through SaveGame"), UGameplayStatics::SaveGameToMemory(SourceSave, SaveBytes));
	UGrimrockPartySaveGame* LoadedSave = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(SaveBytes));
	TestNotNull(TEXT("Lifecycle SaveGame deserializes"), LoadedSave);
	if (!LoadedSave)
	{
		return false;
	}

	Runtime->ClearVisuals();
	Runtime->DungeonRuntimeState = LoadedSave->DungeonRuntimeState;
	Runtime->RebuildLevel();
	Runtime->ApplyCurrentLevelRuntimeState();
	AGridMonsterActor* RebuiltMonster = Runtime->FindSpawnedMonsterActor(SpawnId);
	TestNotNull(TEXT("Spawned state survives a runtime-state round trip"), RebuiltMonster);
	if (RebuiltMonster)
	{
		TestEqual(TEXT("Runtime state restores the teleported cell"), RebuiltMonster->CurrentCell, FIntPoint(2, 2));
		TestEqual(TEXT("Runtime state restores health"), RebuiltMonster->CurrentHealth, 7);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON133AtomicCommandsTest, "Grimrock.Monsters.MON13.3.AtomicCommands", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON133AtomicCommandsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON132TestWorld TestWorld(EWorldType::Game);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	UGridLevelAsset* Level = MakeMON13Level(Runtime);
	Runtime->LevelAsset = Level;
	UGridMonsterDefinitionAsset* FirstDefinition = MakeMON13RuntimeDefinition(*this, Runtime, TEXT("MON133_AtomicFirst"));
	UGridMonsterDefinitionAsset* SecondDefinition = MakeMON13RuntimeDefinition(*this, Runtime, TEXT("MON133_AtomicSecond"));
	if (!FirstDefinition || !SecondDefinition)
	{
		return false;
	}

	const FGuid FirstSpawnId(13, 3, 3, 1);
	const FGuid SecondSpawnId(13, 3, 3, 2);
	Level->Objects.Add(MakeMON13Spawn(FirstDefinition, FirstSpawnId, FIntPoint(0, 0)));
	Level->Objects.Add(MakeMON13Spawn(SecondDefinition, SecondSpawnId, FIntPoint(1, 0)));

	Runtime->RebuildLevel();
	AGridMonsterActor* FirstMonster = Runtime->FindSpawnedMonsterActor(FirstSpawnId);
	TestNotNull(TEXT("First atomic fixture monster exists"), FirstMonster);
	if (!FirstMonster)
	{
		return false;
	}

	TestFalse(TEXT("Teleport rejects an occupied monster cell"), Runtime->TeleportSpawnedMonster(FirstSpawnId, 1, 0, EGridEdge::West));
	TestEqual(TEXT("Rejected teleport preserves the source cell"), FirstMonster->CurrentCell, FIntPoint(0, 0));
	TestEqual(TEXT("Rejected teleport preserves both Actors"), Runtime->GetSpawnedMonsterActorCount(), 2);

	TestTrue(TEXT("Second monster despawns before cell reuse"), Runtime->ExecuteMonsterSpawnCommand(SecondSpawnId, EGridObjectCommand::Despawn));
	TestTrue(TEXT("Freed cell accepts the first monster"), Runtime->TeleportSpawnedMonster(FirstSpawnId, 1, 0, EGridEdge::West));

	AddExpectedError(TEXT("[GridMonsterSpawn] Skipped"), EAutomationExpectedErrorFlags::Contains, 1, false);
	TestFalse(TEXT("Spawn rejects its occupied saved cell"), Runtime->ExecuteMonsterSpawnCommand(SecondSpawnId, EGridObjectCommand::Spawn));
	TestNull(TEXT("Rejected spawn leaves no partial Actor"), Runtime->FindSpawnedMonsterActor(SecondSpawnId));
	TestEqual(TEXT("Rejected spawn leaves one world monster"), CountMON132WorldMonsters(TestWorld.World), 1);
	TestEqual(TEXT("Occupying monster remains at the destination"), FirstMonster->CurrentCell, FIntPoint(1, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON134EncounterWavesTest, "Grimrock.Monsters.MON13.4.EncounterWaves", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON134EncounterWavesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON132TestWorld TestWorld(EWorldType::PIE);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	UGridLevelAsset* Level = MakeMON13Level(Runtime);
	Runtime->LevelAsset = Level;
	UGridActivationComponent* Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
	TestNotNull(TEXT("Runtime activation component exists"), Activation);
	if (!Activation)
	{
		return false;
	}

	UGridMonsterDefinitionAsset* Definition = MakeMON13RuntimeDefinition(*this, Runtime, TEXT("MON134_WaveRat"));
	if (!Definition)
	{
		return false;
	}

	const FName EncounterId(TEXT("Encounter_MON134_Rats"));
	const FGuid TriggerId(13, 4, 1, 1);
	const FGuid Wave0AnchorId(13, 4, 1, 2);
	const FGuid Wave0SecondId(13, 4, 1, 3);
	const FGuid Wave1Id(13, 4, 1, 4);
	const FGuid WaveStartedMarkerId(13, 4, 1, 5);
	const FGuid CompletedMarkerId(13, 4, 1, 6);
	const FName EncounterMarkerArchetypeId(TEXT("MON134_EventMarkerLever"));

	FGridLevelObjectData Trigger;
	Trigger.ObjectId = TriggerId;
	Trigger.Type = EGridLevelObjectType::Trigger;
	Trigger.CellX = 3;
	Trigger.CellY = 3;
	Trigger.Edge = EGridEdge::None;
	Trigger.bInitiallyEnabled = true;
	Level->Objects.Add(Trigger);

	FGridLevelObjectData Wave0Anchor = MakeMON13Spawn(Definition, Wave0AnchorId, FIntPoint(0, 1));
	Wave0Anchor.EncounterGroupId = EncounterId;
	Wave0Anchor.EncounterWaveIndex = 0;
	Wave0Anchor.bInitiallyEnabled = false;
	Level->Objects.Add(Wave0Anchor);

	FGridLevelObjectData Wave0Second = MakeMON13Spawn(Definition, Wave0SecondId, FIntPoint(1, 1));
	Wave0Second.EncounterGroupId = EncounterId;
	Wave0Second.EncounterWaveIndex = 0;
	Wave0Second.bInitiallyEnabled = false;
	Level->Objects.Add(Wave0Second);

	FGridLevelObjectData Wave1 = MakeMON13Spawn(Definition, Wave1Id, FIntPoint(2, 1));
	Wave1.EncounterGroupId = EncounterId;
	Wave1.EncounterWaveIndex = 1;
	Wave1.bInitiallyEnabled = false;
	Level->Objects.Add(Wave1);

	FGridLevelObjectData WaveStartedMarker;
	WaveStartedMarker.ObjectId = WaveStartedMarkerId;
	WaveStartedMarker.ArchetypeId = EncounterMarkerArchetypeId;
	WaveStartedMarker.Type = EGridLevelObjectType::Lever;
	WaveStartedMarker.CellX = 0;
	WaveStartedMarker.CellY = 3;
	WaveStartedMarker.Edge = EGridEdge::North;
	WaveStartedMarker.bInitiallyEnabled = true;
	WaveStartedMarker.bInitiallyActive = false;
	Level->Objects.Add(WaveStartedMarker);

	FGridLevelObjectData CompletedMarker = WaveStartedMarker;
	CompletedMarker.ObjectId = CompletedMarkerId;
	CompletedMarker.CellX = 1;
	Level->Objects.Add(CompletedMarker);
	Runtime->ObjectArchetypes.Add(GridMonsterMON13MakeLeverMarkerArchetype(Runtime, EncounterMarkerArchetypeId));

	FGridObjectLink StartLink;
	StartLink.SourceObjectId = TriggerId;
	StartLink.TargetObjectId = Wave0AnchorId;
	StartLink.SourceEvent = EGridObjectEvent::Activated;
	StartLink.Command = EGridObjectCommand::StartEncounter;
	Level->Links.Add(StartLink);

	FGridObjectLink WaveStartedLink;
	WaveStartedLink.SourceObjectId = Wave0AnchorId;
	WaveStartedLink.TargetObjectId = WaveStartedMarkerId;
	WaveStartedLink.SourceEvent = EGridObjectEvent::EncounterWaveStarted;
	WaveStartedLink.Command = EGridObjectCommand::Toggle;
	Level->Links.Add(WaveStartedLink);

	FGridObjectLink CompletedLink;
	CompletedLink.SourceObjectId = Wave0AnchorId;
	CompletedLink.TargetObjectId = CompletedMarkerId;
	CompletedLink.SourceEvent = EGridObjectEvent::EncounterCompleted;
	CompletedLink.Command = EGridObjectCommand::Toggle;
	Level->Links.Add(CompletedLink);

	Runtime->RebuildLevel();
	TestEqual(TEXT("Encounter members start absent"), Runtime->GetSpawnedMonsterActorCount(), 0);

	Runtime->HandlePartyCellChanged(0, 0, 3, 3);
	TestNotNull(TEXT("Wave zero spawns its anchor"), Runtime->FindSpawnedMonsterActor(Wave0AnchorId));
	TestNotNull(TEXT("Wave zero spawns every member"), Runtime->FindSpawnedMonsterActor(Wave0SecondId));
	TestNull(TEXT("Future wave remains absent"), Runtime->FindSpawnedMonsterActor(Wave1Id));
	TestEqual(TEXT("Wave zero becomes active"), Runtime->GetMonsterEncounterActiveWave(EncounterId), 0);
	TestFalse(TEXT("Encounter is not complete after start"), Runtime->IsMonsterEncounterCompleted(EncounterId));
	TestTrue(TEXT("Wave-start event is emitted"), Activation->GetActiveObjectIds().Contains(WaveStartedMarkerId));

	Runtime->HandlePartyCellChanged(0, 0, 3, 3);
	TestEqual(TEXT("Repeated start creates no duplicate"), Runtime->GetSpawnedMonsterActorCount(), 2);
	TestTrue(TEXT("Repeated start emits no duplicate wave event"), Activation->GetActiveObjectIds().Contains(WaveStartedMarkerId));

	Activation->SetActiveObjectIds(TSet<FGuid>());

	TestTrue(TEXT("A wave member can be despawned"), Runtime->ExecuteMonsterSpawnCommand(Wave0AnchorId, EGridObjectCommand::Despawn));
	AGridMonsterActor* Wave0SecondMonster = Runtime->FindSpawnedMonsterActor(Wave0SecondId);
	TestNotNull(TEXT("Second wave-zero member remains"), Wave0SecondMonster);
	if (!Wave0SecondMonster)
	{
		return false;
	}
	Wave0SecondMonster->MarkDead();
	TestEqual(TEXT("Despawn does not count as an encounter death"), Runtime->GetMonsterEncounterActiveWave(EncounterId), 0);
	TestNull(TEXT("Despawn cannot advance the encounter"), Runtime->FindSpawnedMonsterActor(Wave1Id));

	TestTrue(TEXT("Despawned member can rejoin its active wave"), Runtime->ExecuteMonsterSpawnCommand(Wave0AnchorId, EGridObjectCommand::Spawn));
	AGridMonsterActor* RespawnedAnchor = Runtime->FindSpawnedMonsterActor(Wave0AnchorId);
	TestNotNull(TEXT("Wave-zero anchor respawns"), RespawnedAnchor);
	if (!RespawnedAnchor)
	{
		return false;
	}
	RespawnedAnchor->MarkDead();

	AGridMonsterActor* Wave1Monster = Runtime->FindSpawnedMonsterActor(Wave1Id);
	TestNotNull(TEXT("Completing wave zero spawns wave one"), Wave1Monster);
	TestEqual(TEXT("Wave one becomes active"), Runtime->GetMonsterEncounterActiveWave(EncounterId), 1);
	TestTrue(TEXT("Second wave emits its wave-start event"), Activation->GetActiveObjectIds().Contains(WaveStartedMarkerId));
	if (!Wave1Monster)
	{
		return false;
	}

	Wave1Monster->MarkDead();
	TestTrue(TEXT("Final committed death completes encounter"), Runtime->IsMonsterEncounterCompleted(EncounterId));
	TestEqual(TEXT("Completed encounter has no active wave"), Runtime->GetMonsterEncounterActiveWave(EncounterId), INDEX_NONE);
	TestTrue(TEXT("Completion event is emitted once"), Activation->GetActiveObjectIds().Contains(CompletedMarkerId));

	const FGridLevelRuntimeState* LevelState = Runtime->FindRuntimeStateForCurrentLevel();
	const FGridRuntimeMonsterEncounterState* EncounterState = LevelState ? LevelState->MonsterEncounters.Find(EncounterId) : nullptr;
	TestNotNull(TEXT("Encounter progress is persisted"), EncounterState);
	if (!EncounterState)
	{
		return false;
	}
	TestEqual(TEXT("All encounter members are defeated"), EncounterState->DefeatedSpawnIds.Num(), 3);

	TestTrue(TEXT("Encounter runtime state can be captured"), Runtime->CaptureCurrentLevelRuntimeState());
	UGrimrockPartySaveGame* SourceSave = NewObject<UGrimrockPartySaveGame>(Runtime);
	SourceSave->DungeonRuntimeState = Runtime->DungeonRuntimeState;
	TArray<uint8> SaveBytes;
	TestTrue(TEXT("Encounter state serializes through SaveGame"), UGameplayStatics::SaveGameToMemory(SourceSave, SaveBytes));
	const UGrimrockPartySaveGame* LoadedSave = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(SaveBytes));
	TestNotNull(TEXT("Encounter SaveGame deserializes"), LoadedSave);
	if (!LoadedSave)
	{
		return false;
	}
	const FGridLevelRuntimeState* LoadedLevelState = LoadedSave->DungeonRuntimeState.LevelStates.Find(TEXT("SingleLevel"));
	const FGridRuntimeMonsterEncounterState* LoadedEncounterState = LoadedLevelState ? LoadedLevelState->MonsterEncounters.Find(EncounterId) : nullptr;
	TestTrue(TEXT("Completed encounter survives SaveGame round trip"), LoadedEncounterState && LoadedEncounterState->bCompleted);
	TestEqual(TEXT("Defeated members survive SaveGame round trip"), LoadedEncounterState ? LoadedEncounterState->DefeatedSpawnIds.Num() : 0, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON134AtomicWaveFailureTest, "Grimrock.Monsters.MON13.4.AtomicWaveFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON134AtomicWaveFailureTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON132TestWorld TestWorld(EWorldType::Game);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	UGridLevelAsset* Level = MakeMON13Level(Runtime);
	Runtime->LevelAsset = Level;
	UGridMonsterDefinitionAsset* Definition = MakeMON13RuntimeDefinition(*this, Runtime, TEXT("MON134_AtomicRat"));
	if (!Definition)
	{
		return false;
	}

	const FName EncounterId(TEXT("Encounter_MON134_Atomic"));
	const FGuid AnchorId(13, 4, 2, 1);
	FGridLevelObjectData Anchor = MakeMON13Spawn(Definition, AnchorId, FIntPoint(1, 1));
	Anchor.EncounterGroupId = EncounterId;
	Anchor.bInitiallyEnabled = false;
	Level->Objects.Add(Anchor);

	FGridLevelObjectData Conflict = MakeMON13Spawn(Definition, FGuid(13, 4, 2, 2), FIntPoint(1, 1));
	Conflict.EncounterGroupId = EncounterId;
	Conflict.bInitiallyEnabled = false;
	Level->Objects.Add(Conflict);

	Runtime->RebuildLevel();
	AddExpectedError(TEXT("Reason=GeneratedMonsterCellConflict"), EAutomationExpectedErrorFlags::Contains, 1, false);
	AddExpectedError(TEXT("[GridMonsterEncounter] WaveRejected"), EAutomationExpectedErrorFlags::Contains, 1, false);
	TestFalse(TEXT("Invalid wave is rejected atomically"), Runtime->StartMonsterEncounter(AnchorId));
	TestEqual(TEXT("Rejected wave leaves no tracked Actor"), Runtime->GetSpawnedMonsterActorCount(), 0);
	TestEqual(TEXT("Rejected wave leaks no world Actor"), CountMON132WorldMonsters(TestWorld.World), 0);
	TestEqual(TEXT("Rejected wave has no active index"), Runtime->GetMonsterEncounterActiveWave(EncounterId), INDEX_NONE);

	const FGridLevelRuntimeState* LevelState = Runtime->FindRuntimeStateForCurrentLevel();
	const FGridRuntimeMonsterEncounterState* EncounterState = LevelState ? LevelState->MonsterEncounters.Find(EncounterId) : nullptr;
	TestNotNull(TEXT("Rejected encounter keeps retry metadata"), EncounterState);
	TestTrue(TEXT("Rejected wave is not marked started"), EncounterState && !EncounterState->bStarted);
	TestEqual(TEXT("Rejected wave restores placement state"), LevelState ? LevelState->MonsterPlacements.Num() : -1, 0);
	TestEqual(TEXT("Rejected wave restores monster state"), LevelState ? LevelState->Monsters.Num() : -1, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON134ValidationTest, "Grimrock.Monsters.MON13.4.Validation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON134ValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridLevelAsset* Level = MakeMON13Level(GetTransientPackage());
	UGridMonsterDefinitionAsset* Definition = MakeMON13Definition(Level, TEXT("MON134_ValidationRat"));

	FGridLevelObjectData NegativeWave = MakeMON13Spawn(Definition, FGuid(13, 4, 3, 1), FIntPoint(0, 0));
	NegativeWave.EncounterWaveIndex = -1;
	Level->Objects.Add(NegativeWave);

	FGridLevelObjectData UngroupedFutureWave = MakeMON13Spawn(Definition, FGuid(13, 4, 3, 2), FIntPoint(1, 0));
	UngroupedFutureWave.EncounterGroupId = NAME_None;
	UngroupedFutureWave.EncounterWaveIndex = 1;
	UngroupedFutureWave.bInitiallyEnabled = false;
	Level->Objects.Add(UngroupedFutureWave);

	FGridLevelObjectData EnabledFutureWave = MakeMON13Spawn(Definition, FGuid(13, 4, 3, 3), FIntPoint(2, 0));
	EnabledFutureWave.EncounterGroupId = TEXT("Encounter_MON134_Future");
	EnabledFutureWave.EncounterWaveIndex = 1;
	EnabledFutureWave.bInitiallyEnabled = true;
	Level->Objects.Add(EnabledFutureWave);

	FGridLevelObjectData SharedWaveCellA = MakeMON13Spawn(Definition, FGuid(13, 4, 3, 4), FIntPoint(0, 1));
	SharedWaveCellA.EncounterGroupId = TEXT("Encounter_MON134_Shared");
	SharedWaveCellA.EncounterWaveIndex = 2;
	SharedWaveCellA.bInitiallyEnabled = false;
	Level->Objects.Add(SharedWaveCellA);

	FGridLevelObjectData SharedWaveCellB = SharedWaveCellA;
	SharedWaveCellB.ObjectId = FGuid(13, 4, 3, 5);
	Level->Objects.Add(SharedWaveCellB);

	TArray<FString> Errors;
	TestFalse(TEXT("Invalid encounter wave data is rejected"), Level->ValidateMonsterSpawns(Errors));
	TestTrue(TEXT("Negative wave index is reported"), HasErrorContaining(Errors, TEXT("EncounterWaveIndex >= 0")));
	TestTrue(TEXT("Future wave requires an encounter group"), HasErrorContaining(Errors, TEXT("requires EncounterGroupId")));
	TestTrue(TEXT("Future wave must start disabled"), HasErrorContaining(Errors, TEXT("must be disabled at start")));
	TestTrue(TEXT("Same-wave cell conflict is reported"), HasErrorContaining(Errors, TEXT("shares encounter wave")));
	return true;
}

#endif

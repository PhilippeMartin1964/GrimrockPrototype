#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorClean064X27CanonicalSelectedObject,
	"Grimrock.RPG.ARM.CLEAN06_4X27.SelectedObjectCanonicalItemId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorClean064X27CanonicalSelectedObject::RunTest(const FString&)
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues()
		.AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(false).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false)
		.SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false,
		FName(*FString::Printf(TEXT("ARM_CLEAN064X27_%s"),
			*FGuid::NewGuid().ToString(EGuidFormats::Digits))),
		nullptr, true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Transient editor preview world"), World)) return false;
	if (GEngine)
	{
		GEngine->CreateNewWorldContext(EWorldType::EditorPreview)
			.SetCurrentWorld(World);
	}
	AGridLevelEditorActor* Editor = World->SpawnActor<AGridLevelEditorActor>();
	if (!TestNotNull(TEXT("Real level editor actor"), Editor))
	{
		World->DestroyWorld(false);
		if (GEngine) GEngine->DestroyWorldContext(World);
		return false;
	}

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Editor);
	Level->Width = 3;
	Level->Height = 3;
	Level->EnsureCellCount();
	for (FGridLevelCellData& Cell : Level->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
		Cell.bBlocksOccupancy = false;
	}
	Editor->LevelAsset = Level;

	const FName Dagger(TEXT("Weapon_Dagger_01"));
	const FName Sword(TEXT("Weapon_StraightSword_01"));
	const FName Shield(TEXT("Weapon_RoundShield_01"));
	const FName Arrows(TEXT("Item_Arrows_01"));
	UGridItemDefinitionAsset* HistoricDagger = NewObject<UGridItemDefinitionAsset>(Level);
	HistoricDagger->ItemDefinitionId = Dagger;
	FGridLooseItemInstance& Loose = Level->LooseItemInstances.AddDefaulted_GetRef();
	Loose.InstanceId = FGuid::NewGuid();
	Loose.CellX = 1;
	Loose.CellY = 1;
	Loose.ItemDefinition = HistoricDagger;
	Loose.ItemLevel = 20;
	Loose.Quantity = 1;
	const FGuid StableLooseId = Loose.InstanceId;
	Editor->LastSelectedObjectId = StableLooseId;

	// Simply re-entering the ID shown by the inspector is a strict no-op.
	// It must NOT drop an old definition before native visual parity.
	bool bOk = TestTrue(TEXT("Recommit historical ID succeeds"),
		Editor->SetSelectedObjectItemDefinitionId(Dagger)) &&
		TestTrue(TEXT("Recommit does not drop legacy presentation"),
			Loose.ItemDefinition == HistoricDagger) &&
		TestTrue(TEXT("Recommit does not invent canonical authoring"),
			Loose.CanonicalItemId.IsNone());

	// Explicitly changing the item family is new authoring: old source is
	// dropped, but GUID, quantity and a valid physical level stay stable.
	bOk &= TestTrue(TEXT("Select canonical sword by ID"),
		Editor->SetSelectedObjectItemDefinitionId(Sword));
	bOk &= TestEqual(TEXT("Canonical sword ID persisted"), Loose.CanonicalItemId, Sword);
	bOk &= TestEqual(TEXT("Valid N20 kept on sword"), Loose.ItemLevel, 20);
	bOk &= TestNull(TEXT("Old asset removed from new canonical selection"),
		Loose.ItemDefinition.Get());
	bOk &= TestFalse(TEXT("Legacy picker cannot contradict persisted canonical ID"),
		Editor->SetSelectedObjectItemDefinitionAsset(HistoricDagger));
	bOk &= TestEqual(TEXT("Conflicting picker has not changed canonical ID"),
		Loose.CanonicalItemId, Sword);
	bOk &= TestEqual(TEXT("Conflicting picker has not changed physical level"),
		Loose.ItemLevel, 20);

	bOk &= TestTrue(TEXT("Select unlevelled registered arrows"),
		Editor->SetSelectedObjectItemDefinitionId(Arrows));
	bOk &= TestEqual(TEXT("Family changes to arrows"), Loose.CanonicalItemId, Arrows);
	bOk &= TestEqual(TEXT("New non-ARM family is physically N0"), Loose.ItemLevel, 0);
	bOk &= TestTrue(TEXT("No legacy definition recreated"),
		Loose.ItemDefinition == nullptr);

	bOk &= TestTrue(TEXT("Select shield from N0 creates NEW physical N1"),
		Editor->SetSelectedObjectItemDefinitionId(Shield));
	bOk &= TestEqual(TEXT("New registered ARM defaults to authored N1"), Loose.ItemLevel, 1);
	bOk &= TestFalse(TEXT("Unknown canonical ID cannot change selected identity"),
		Editor->SetSelectedObjectItemDefinitionId(TEXT("ARM_CLEAN064X27_Unknown")));
	bOk &= TestEqual(TEXT("Rejected identity leaves real item unchanged"),
		Loose.CanonicalItemId, Shield);
	bOk &= TestEqual(TEXT("Rejected identity leaves physical N1 unchanged"),
		Loose.ItemLevel, 1);
	bOk &= TestEqual(TEXT("Changing ID retains original level instance GUID"),
		Loose.InstanceId, StableLooseId);
	bOk &= TestEqual(TEXT("Changing ID retains quantity"), Loose.Quantity, 1);

	// ItemSpawns are edited through the SAME Selected Object command; they
	// must also retain the authored spawn GUID and stack quantity.
	FGridItemSpawnInstance& Spawn = Level->ItemSpawns.AddDefaulted_GetRef();
	Spawn.SpawnId = FGuid::NewGuid();
	Spawn.CellX = 2;
	Spawn.CellY = 1;
	Spawn.CanonicalItemId = Arrows;
	Spawn.ItemLevel = 0;
	Spawn.Quantity = 5;
	const FGuid StableSpawnId = Spawn.SpawnId;
	Editor->LastSelectedObjectId = StableSpawnId;
	bOk &= TestFalse(TEXT("Stack of five arrows cannot silently become five daggers"),
		Editor->SetSelectedObjectItemDefinitionId(Dagger));
	bOk &= TestEqual(TEXT("Rejected conversion leaves ammo identity"),
		Spawn.CanonicalItemId, Arrows);
	bOk &= TestEqual(TEXT("Rejected conversion leaves quantity"),
		Spawn.Quantity, 5);
	// Author explicitly changes quantity before changing from a stack to
	// an indivisible weapon (no implicit consumption or duplication).
	Spawn.Quantity = 1;
	bOk &= TestTrue(TEXT("One item spawn changes from arrows N0 to dagger N1"),
		Editor->SetSelectedObjectItemDefinitionId(Dagger));
	bOk &= TestEqual(TEXT("Spawn canonical ID updated"), Spawn.CanonicalItemId, Dagger);
	bOk &= TestEqual(TEXT("New ARM spawn has physical N1"), Spawn.ItemLevel, 1);
	bOk &= TestNull(TEXT("Generator schema has no old pointer"), FGridItemSpawnInstance::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition")));
	bOk &= TestEqual(TEXT("Spawn GUID retained"), Spawn.SpawnId, StableSpawnId);
	bOk &= TestEqual(TEXT("Spawn authored quantity retained"), Spawn.Quantity, 1);
	bOk &= TestTrue(TEXT("Clearing an already-null legacy picker is a no-op"),
		Editor->SetSelectedObjectItemDefinitionAsset(nullptr));
	bOk &= TestEqual(TEXT("Canonical identity survives empty picker"),
		Spawn.CanonicalItemId, Dagger);

	World->DestroyWorld(false);
	if (GEngine) GEngine->DestroyWorldContext(World);
	return bOk && !HasAnyErrors();
}
#endif

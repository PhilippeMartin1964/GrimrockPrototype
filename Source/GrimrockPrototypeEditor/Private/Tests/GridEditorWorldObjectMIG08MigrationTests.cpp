#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "EditorTools/GridWorldObjectMIG08MigrationService.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG08LevelAssetMigrationTest,
	"Grimrock.WorldObjects.MIG08.LevelAssetMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG08LevelAssetMigrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>();
	UGridItemDefinitionAsset* Item = NewObject<UGridItemDefinitionAsset>();
	Item->ItemDefinitionId = TEXT("MIG08_Item");
	UGridMonsterDefinitionAsset* Monster = NewObject<UGridMonsterDefinitionAsset>();
	Monster->MonsterId = TEXT("MIG08_Monster");

	FGridLevelObjectData Door;
	Door.ObjectId = FGuid::NewGuid();
	Door.Type = EGridLevelObjectType::Door;
	Door.ArchetypeId = TEXT("MIG08_Door");
	Door.CellX = 1;
	Door.CellY = 2;
	Door.Behavior.Transition.bIsTransition = true;
	Door.Behavior.Transition.TargetLevelId = TEXT("MIG08_Target");
	Level->Objects.Add(Door);

	FGridLevelObjectData LooseItem;
	LooseItem.ObjectId = FGuid::NewGuid();
	LooseItem.Type = EGridLevelObjectType::Item;
	LooseItem.ItemDefinitionAsset = Item;
	LooseItem.CellX = 3;
	LooseItem.CellY = 4;
	Level->Objects.Add(LooseItem);

	FGridLevelObjectData MonsterSpawn;
	MonsterSpawn.ObjectId = FGuid::NewGuid();
	MonsterSpawn.Type = EGridLevelObjectType::MonsterSpawn;
	MonsterSpawn.MonsterDefinitionAsset = Monster;
	MonsterSpawn.InitialFacing = EGridEdge::North;
	MonsterSpawn.CellX = 5;
	MonsterSpawn.CellY = 6;
	Level->Objects.Add(MonsterSpawn);

	FGridLevelObjectData ItemSpawn;
	ItemSpawn.ObjectId = FGuid::NewGuid();
	ItemSpawn.Type = EGridLevelObjectType::ItemSpawn;
	ItemSpawn.ItemDefinitionAsset = Item;
	ItemSpawn.CellX = 7;
	ItemSpawn.CellY = 8;
	Level->Objects.Add(ItemSpawn);

	FGridLevelObjectData Logic;
	Logic.ObjectId = FGuid::NewGuid();
	Logic.Type = EGridLevelObjectType::Logic;
	Logic.LogicId = TEXT("MIG08_Logic");
	Level->Objects.Add(Logic);

	const FGridWorldObjectMIG08MigrationResult Result = FGridWorldObjectMIG08MigrationService::MigrateLevelAsset(*Level);
	TestTrue(TEXT("Legacy level is changed by MIG08"), Result.bChanged);
	TestEqual(TEXT("Level migration has no errors"), Result.Errors.Num(), 0);
	TestTrue(TEXT("Typed storage becomes authoritative"), Level->bTypedPlacementStorageAuthoritative);
	TestEqual(TEXT("One world-object instance"), Level->WorldObjectInstances.Num(), 1);
	TestEqual(TEXT("One loose item instance"), Level->LooseItemInstances.Num(), 1);
	TestEqual(TEXT("One monster spawn"), Level->MonsterSpawns.Num(), 1);
	TestEqual(TEXT("One item spawn"), Level->ItemSpawns.Num(), 1);
	TestEqual(TEXT("One logic object"), Level->LogicObjects.Num(), 1);
	TestEqual(TEXT("Compatibility mirror preserves all placements"), Level->Objects.Num(), 5);
	TestEqual(TEXT("Transition instance data is preserved"), Level->WorldObjectInstances[0].InstanceConfig.Transition.TargetLevelId, FName(TEXT("MIG08_Target")));
	TestEqual(TEXT("Loose item keeps direct definition"), Level->LooseItemInstances[0].ItemDefinition.Get(), Item);
	TestEqual(TEXT("Monster spawn keeps direct definition"), Level->MonsterSpawns[0].MonsterDefinition.Get(), Monster);
	TestEqual(TEXT("Item spawn keeps direct definition"), Level->ItemSpawns[0].ItemDefinition.Get(), Item);

	const FGridWorldObjectMIG08MigrationResult SecondPass = FGridWorldObjectMIG08MigrationService::MigrateLevelAsset(*Level);
	TestFalse(TEXT("MIG08 level migration is idempotent"), SecondPass.bChanged);
	TestEqual(TEXT("Second pass has no errors"), SecondPass.Errors.Num(), 0);
	TestEqual(TEXT("Second pass preserves placement count"), Level->Objects.Num(), 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG08PaletteMigrationTest,
	"Grimrock.WorldObjects.MIG08.PaletteItemMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG08PaletteMigrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridItemDefinitionAsset* Item = NewObject<UGridItemDefinitionAsset>();
	Item->ItemDefinitionId = TEXT("MIG08_Gem");
	UStaticMesh* LegacyMesh = NewObject<UStaticMesh>();
	UTexture2D* LegacyIcon = NewObject<UTexture2D>();

	UGridObjectArchetypeAsset* LegacyItemArchetype = NewObject<UGridObjectArchetypeAsset>();
	LegacyItemArchetype->ArchetypeId = TEXT("Item_MIG08_Gem");
	LegacyItemArchetype->SupportedType = EGridLevelObjectType::Item;
	LegacyItemArchetype->DefaultBehavior.Item.ItemDefinitionAsset = Item;
	LegacyItemArchetype->StaticPart.Mesh = LegacyMesh;

	UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>();
	FGridObjectPaletteEntry& Entry = Palette->Entries.AddDefaulted_GetRef();
	Entry.EntryId = TEXT("MIG08_Gem");
	Entry.DefaultArchetype = LegacyItemArchetype;
	Entry.Icon = LegacyIcon;

	const FGridWorldObjectMIG08MigrationResult Result = FGridWorldObjectMIG08MigrationService::MigratePaletteAsset(*Palette);
	TestTrue(TEXT("Legacy item palette entry changes"), Result.bChanged);
	TestEqual(TEXT("Palette migration has no errors"), Result.Errors.Num(), 0);
	TestEqual(TEXT("Palette references ItemDefinition directly"), Entry.DefaultItemDefinition.Get(), Item);
	TestNull(TEXT("Companion item archetype is removed from palette entry"), Entry.DefaultArchetype.Get());
	TestNull(TEXT("Palette icon duplication is removed"), Entry.Icon.Get());
	TestEqual(TEXT("Legacy palette icon is promoted into ItemDefinition"), Item->Icon.Get(), LegacyIcon);
	TestEqual(TEXT("Legacy StaticPart mesh is promoted into ItemDefinition"), Item->WorldMesh.Get(), LegacyMesh);
	TestTrue(TEXT("Migrated direct item palette entry is valid"), Entry.IsValidEntry());

	const FGridWorldObjectMIG08MigrationResult SecondPass = FGridWorldObjectMIG08MigrationService::MigratePaletteAsset(*Palette);
	TestFalse(TEXT("MIG08 palette migration is idempotent"), SecondPass.bChanged);
	TestEqual(TEXT("Second palette pass has no errors"), SecondPass.Errors.Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

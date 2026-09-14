#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridRelocationUtils.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"

namespace
{
	bool HasCLASS01Error(const TArray<FGridLevelValidationMessage>& Messages, const TCHAR* Fragment)
	{
		return Messages.ContainsByPredicate(
			[Fragment](const FGridLevelValidationMessage& Message)
			{
				return Message.Severity == EGridLevelValidationSeverity::Error && Message.Message.Contains(Fragment);
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridWorldObjectCLASS01Test, "Grimrock.WorldObjects.CLASS01",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectCLASS01Test::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const UEnum* ObjectType = StaticEnum<EGridLevelObjectType>();
	const UEnum* CellType = StaticEnum<EGridCellType>();
	TestNotNull(TEXT("Gameplay Type enum"), ObjectType);
	TestNotNull(TEXT("Cell Type enum"), CellType);
	if (ObjectType)
	{
		TestTrue(TEXT("Gameplay Type contains Relocation"), ObjectType->GetValueByNameString(TEXT("Relocation")) != INDEX_NONE);
		TestTrue(TEXT("Gameplay Type has no legacy relocation alias"),
			ObjectType->GetValueByNameString(TEXT("Tele"
											 "porter")) == INDEX_NONE);
	}
	if (CellType)
	{
		TestTrue(TEXT("Cell Type has no relocation classification"),
			CellType->GetValueByNameString(TEXT("Tele"
										 "porter")) == INDEX_NONE);
	}
	TestNull(TEXT("Functional category enum is removed"),
		FindObject<UEnum>(nullptr, TEXT("/Script/GrimrockPrototype.EGridObject"
										"Category")));

	UClass* DefinitionClass = UGridWorldObjectDefinitionAsset::StaticClass();
	TestNull(TEXT("Definition has no functional category"), FindFProperty<FProperty>(DefinitionClass, TEXT("Object"
																							 "Category")));
	TestNull(TEXT("Definition has no palette category"), FindFProperty<FProperty>(DefinitionClass, TEXT("Category")));
	UScriptStruct* PaletteEntryStruct = FGridObjectPaletteEntry::StaticStruct();
	TestNotNull(TEXT("Palette entry owns PaletteCategory"), FindFProperty<FProperty>(PaletteEntryStruct, TEXT("PaletteCategory")));
	TestNull(TEXT("Palette entry has no override category"), FindFProperty<FProperty>(PaletteEntryStruct, TEXT("Category"
																							 "Override")));
	UScriptStruct* ConfigStruct = FGridWorldObjectInstanceConfig::StaticStruct();
	TestNotNull(TEXT("Instance has relocation initial state"), FindFProperty<FProperty>(ConfigStruct, TEXT("bRelocationInitiallyEnabled")));
	TestNull(TEXT("Instance has no old initial state"), FindFProperty<FProperty>(ConfigStruct, TEXT("bTeleporter"
																							 "InitiallyEnabled")));

	FGridWorldObjectInstance Object;
	Object.Type = EGridLevelObjectType::Relocation;
	TestFalse(TEXT("Unconfigured Relocation is not a runtime candidate"), GridRelocation::IsCandidate(Object));
	Object.InstanceConfig.Relocation.TargetCellX = 1;
	Object.InstanceConfig.Relocation.TargetCellY = 1;
	TestTrue(TEXT("Configured Relocation is a runtime candidate"), GridRelocation::IsCandidate(Object));
	Object.Type = EGridLevelObjectType::Decoration;
	TestFalse(TEXT("Decoration with relocation data is not a runtime candidate"), GridRelocation::IsCandidate(Object));
	Object.Type = EGridLevelObjectType::Pit;
	TestFalse(TEXT("Pit is not a generic relocation candidate"), GridRelocation::IsCandidate(Object));

	const UWorld::InitializationValues Values = UWorld::InitializationValues()
		.AllowAudioPlayback(false)
		.RequiresHitProxies(false)
		.CreatePhysicsScene(false)
		.CreateNavigation(false)
		.CreateAISystem(false)
		.ShouldSimulatePhysics(false)
		.SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Editor, false, TEXT("CLASS01ValidationWorld"), nullptr, true, ERHIFeatureLevel::Num, &Values);
	if (TestNotNull(TEXT("Validation world"), World) && GEngine)
	{
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Editor);
		Context.SetCurrentWorld(World);
		AGridLevelEditorActor* Editor = World->SpawnActor<AGridLevelEditorActor>();
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Editor);
		Level->Width = 3;
		Level->Height = 3;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
		}
		Editor->LevelAsset = Level;

		FGridWorldObjectInstance Placement;
		Placement.InstanceId = FGuid::NewGuid();
		Placement.Type = EGridLevelObjectType::Relocation;
		Level->WorldObjectInstances = { Placement };
		TestTrue(TEXT("Unconfigured Relocation is a validation error"),
			HasCLASS01Error(Editor->ValidateCurrentLevel(), TEXT("requires Destination Cell X")));

		Placement.Type = EGridLevelObjectType::Decoration;
		Placement.InstanceConfig.Relocation.TargetCellX = 1;
		Placement.InstanceConfig.Relocation.TargetCellY = 1;
		Level->WorldObjectInstances = { Placement };
		TestTrue(TEXT("Relocation data on Decoration is a validation error"),
			HasCLASS01Error(Editor->ValidateCurrentLevel(), TEXT("Gameplay Type is not Relocation")));

		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	}

	auto* StairsUp = LoadObject<UGridWorldObjectDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Stairs_Up.DA_Stairs_Up"));
	auto* StairsDown = LoadObject<UGridWorldObjectDefinitionAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Stairs_Down.DA_Stairs_Down"));
	for (const UGridWorldObjectDefinitionAsset* Stairs : { StairsUp, StairsDown })
	{
		if (TestNotNull(TEXT("Stairs definition"), Stairs))
		{
			TestEqual(TEXT("Stairs Gameplay Type"), Stairs->SupportedType, EGridLevelObjectType::Relocation);
			TestEqual(TEXT("Stairs Placement Surface"), Stairs->PlacementSurface, EGridObjectPlacementKind::Floor);
			TestEqual(TEXT("Stairs default destination X is unset"), Stairs->DefaultBehavior.Relocation.TargetCellX, INDEX_NONE);
			TestEqual(TEXT("Stairs default destination Y is unset"), Stairs->DefaultBehavior.Relocation.TargetCellY, INDEX_NONE);
		}
	}

	auto* Level01 = LoadObject<UGridLevelAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_01.DA_GridLevel_01"));
	if (TestNotNull(TEXT("Unprotected level asset"), Level01))
	{
		const FGridWorldObjectInstance* StairsPlacement = Level01->WorldObjectInstances.FindByPredicate(
			[](const FGridWorldObjectInstance& Placement)
			{
				return Placement.WorldObjectDefinitionId == FName(TEXT("Stairs_Up"));
			});
		if (TestNotNull(TEXT("Level 01 Stairs_Up placement"), StairsPlacement))
		{
			TestEqual(TEXT("Level 01 stairs Gameplay Type"), StairsPlacement->Type, EGridLevelObjectType::Relocation);
		}
	}

	auto* Palette = LoadObject<UGridObjectPaletteAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default"));
	if (TestNotNull(TEXT("Official palette"), Palette))
	{
		for (const FGridObjectPaletteEntry& Entry : Palette->Entries)
		{
			TestFalse(*FString::Printf(TEXT("Palette entry %s is classified"), *Entry.EntryId.ToString()), Entry.PaletteCategory.IsNone());
		}
		for (const FName StairsEntryId : { FName(TEXT("Stairs_Up")), FName(TEXT("Stairs_Down")) })
		{
			const FGridObjectPaletteEntry* Entry = Palette->FindEntryById(StairsEntryId);
			if (TestNotNull(*FString::Printf(TEXT("Palette entry %s"), *StairsEntryId.ToString()), Entry))
			{
				TestEqual(TEXT("Stairs palette category"), Entry->PaletteCategory, FName(TEXT("Navigation")));
			}
		}
	}

	return true;
}

#endif

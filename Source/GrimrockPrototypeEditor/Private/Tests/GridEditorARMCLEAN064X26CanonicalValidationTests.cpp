#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "EditorTools/GridEditorItemLevelPolicy.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorClean064X26CanonicalAuthoring,
	"Grimrock.RPG.ARM.CLEAN06_4X26.PointerlessCanonicalEditorValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorClean064X26CanonicalAuthoring::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/"
					  "DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real ItemSystem"), Root.Get())) { return false; }
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register actual ItemSystem"),
			Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& E : Errors) { AddError(E); }
		return false;
	}

	const FName Dagger(TEXT("Weapon_Dagger_01"));
	const FName Arrows(TEXT("Item_Arrows_01"));
	if (!TestTrue(TEXT("Persisted ARM N1 legal"),
			FGridEditorItemLevelPolicy::CanAuthorItemLevel(Registry.Get(), Dagger, 1)) ||
		!TestTrue(TEXT("Persisted ARM N20 legal"),
			FGridEditorItemLevelPolicy::CanAuthorItemLevel(Registry.Get(), Dagger, 20)) ||
		!TestFalse(TEXT("Persisted ARM N0 forbidden"),
			FGridEditorItemLevelPolicy::CanAuthorItemLevel(Registry.Get(), Dagger, 0)) ||
		!TestTrue(TEXT("Unlevelled ammo N0 legal"),
			FGridEditorItemLevelPolicy::CanAuthorItemLevel(Registry.Get(), Arrows, 0)) ||
		!TestFalse(TEXT("Unlevelled ammo N1 forbidden"),
			FGridEditorItemLevelPolicy::CanAuthorItemLevel(Registry.Get(), Arrows, 1)) ||
		!TestFalse(TEXT("Unregistered identity forbidden"),
			FGridEditorItemLevelPolicy::CanAuthorItemLevel(
				Registry.Get(), TEXT("ARM_CLEAN064X26_Unknown"), 1)) ||
		!TestFalse(TEXT("Missing registry is never an authority"),
			FGridEditorItemLevelPolicy::CanAuthorItemLevel(nullptr, Dagger, 1)))
	{
		return false;
	}

	const UWorld::InitializationValues Values = UWorld::InitializationValues()
		.AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(false).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false)
		.SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false,
		FName(*FString::Printf(TEXT("ARM_CLEAN064X26_%s"),
			*FGuid::NewGuid().ToString(EGuidFormats::Digits))),
		nullptr, true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Editor preview world"), World)) { return false; }
	if (GEngine) { GEngine->CreateNewWorldContext(EWorldType::EditorPreview).SetCurrentWorld(World); }
	AGridLevelEditorActor* Editor = World->SpawnActor<AGridLevelEditorActor>();
	if (!TestNotNull(TEXT("Editor actor"), Editor))
	{
		World->DestroyWorld(false);
		if (GEngine) { GEngine->DestroyWorldContext(World); }
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
	FGridLooseItemInstance& Loose = Level->LooseItemInstances.AddDefaulted_GetRef();
	Loose.InstanceId = FGuid::NewGuid();
	Loose.CellX = 1;
	Loose.CellY = 1;
	Loose.CanonicalItemId = Dagger;
	Loose.ItemLevel = 20;
	Loose.Quantity = 1;
	const FGuid LooseId = Loose.InstanceId;

	FGridItemSpawnInstance& Spawn = Level->ItemSpawns.AddDefaulted_GetRef();
	Spawn.SpawnId = FGuid::NewGuid();
	Spawn.CellX = 2;
	Spawn.CellY = 1;
	Spawn.CanonicalItemId = Arrows;
	Spawn.ItemLevel = 0;
	Spawn.Quantity = 3;
	const FGuid SpawnId = Spawn.SpawnId;

	const auto HasSpecificError = [](const TArray<FGridLevelValidationMessage>& Messages,
		FGuid Id, const TCHAR* Fragment)
	{
		return Messages.ContainsByPredicate(
			[Id, Fragment](const FGridLevelValidationMessage& Msg)
			{
				return Msg.OptionalObjectId == Id &&
					Msg.Severity == EGridLevelValidationSeverity::Error &&
					Msg.Message.Contains(Fragment);
			});
	};

	const TArray<FGridLevelValidationMessage> Initially = Editor->ValidateCurrentLevel();
	if (!TestFalse(TEXT("Valid pointerless ARM has no definition error"),
			HasSpecificError(Initially, LooseId, TEXT("ItemDefinition"))) ||
		!TestFalse(TEXT("Valid pointerless spawn has no definition error"),
			HasSpecificError(Initially, SpawnId, TEXT("ItemDefinition"))) ||
		!TestFalse(TEXT("Valid physical ARM N20 has no canonical error"),
			HasSpecificError(Initially, LooseId, TEXT("Canonical item placement"))) ||
		!TestFalse(TEXT("Valid physical arrows N0 have no canonical error"),
			HasSpecificError(Initially, SpawnId, TEXT("Canonical item placement"))))
	{
		World->DestroyWorld(false);
		if (GEngine) { GEngine->DestroyWorldContext(World); }
		return false;
	}

	Editor->LastSelectedObjectId = LooseId;
	if (!TestTrue(TEXT("Pointerless ARM editor level changes to N1"),
			Editor->SetSelectedObjectItemLevel(1)) ||
		!TestEqual(TEXT("Real loose level edited"), Loose.ItemLevel, 1) ||
		!TestFalse(TEXT("Pointerless ARM editor refuses N0"),
			Editor->SetSelectedObjectItemLevel(0)) ||
		!TestEqual(TEXT("Invalid edit preserves N1"), Loose.ItemLevel, 1))
	{
		World->DestroyWorld(false);
		if (GEngine) { GEngine->DestroyWorldContext(World); }
		return false;
	}

	Editor->LastSelectedObjectId = SpawnId;
	if (!TestFalse(TEXT("Pointerless non-ARM editor refuses N1"),
			Editor->SetSelectedObjectItemLevel(1)) ||
		!TestEqual(TEXT("Rejected spawn keeps N0"), Spawn.ItemLevel, 0))
	{
		World->DestroyWorld(false);
		if (GEngine) { GEngine->DestroyWorldContext(World); }
		return false;
	}

	// Force a malformed native payload in memory and ensure validation catches
	// illegal physical levels and absent or unknown canonical identities.
	Loose.ItemLevel = 0;
	TArray<FGridLevelValidationMessage> Messages = Editor->ValidateCurrentLevel();
	const bool bRejectN0 = HasSpecificError(
		Messages, LooseId, TEXT("Canonical item placement"));
	Loose.ItemLevel = 20;
	const bool bSchemaClean = TestNull(TEXT("Contradictory source is no longer representable"), FGridLooseItemInstance::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition")));
	const FName SavedId = Loose.CanonicalItemId;
	Loose.CanonicalItemId = NAME_None;
	Messages = Editor->ValidateCurrentLevel();
	const bool bRejectMissing = HasSpecificError(Messages, LooseId, TEXT("Canonical item placement"));
	Loose.CanonicalItemId = SavedId;
	Spawn.CanonicalItemId = TEXT("ARM_CLEAN064X26_Unknown");
	Messages = Editor->ValidateCurrentLevel();
	const bool bRejectUnknown = HasSpecificError(
		Messages, SpawnId, TEXT("Canonical item placement"));

	World->DestroyWorld(false);
	if (GEngine) { GEngine->DestroyWorldContext(World); }
	return TestTrue(TEXT("Editor validator rejects persisted ARM N0"), bRejectN0) &&
		bSchemaClean &&
		TestTrue(TEXT("Editor validator rejects missing ItemId"), bRejectMissing) &&
		TestTrue(TEXT("Editor validator rejects unregistered ItemId"), bRejectUnknown);
}
#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "EditorTools/GridEditorItemLevelPolicy.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063ANAuthoring,
	"Grimrock.RPG.ARM.CLEAN06_3.EditorCanonicalAuthoredItemLevels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063ANAuthoring::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real canonical root"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register persisted ItemSystem"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}

	const FName Sword(TEXT("Weapon_StraightSword_01"));
	const FName Ammo(TEXT("Item_Arrows_01"));
	const FName Unknown(TEXT("Legacy_Editor_063AN"));
	for (int32 Authored : {1, 20})
	{
		TestTrue(TEXT("Canonical ARM permits actual N1-N20"),
			FGridEditorItemLevelPolicy::CanAuthorItemLevel(Registry.Get(), Sword, Authored));
		FGridResolvedItem Canonical;
		TestTrue(TEXT("Same fixed level resolves in actual canonical system"),
			Registry->ResolveItem(Sword, Authored, Canonical) && Canonical.bIsArmament &&
			Canonical.ItemLevel == Authored);
	}
	TestFalse(TEXT("N21 never accepted"),
		FGridEditorItemLevelPolicy::CanAuthorItemLevel(Registry.Get(), Sword, 21));
	TestFalse(TEXT("ARM N0 is not an inherited default"),
		FGridEditorItemLevelPolicy::CanAuthorItemLevel(Registry.Get(), Sword, 0));
	TestTrue(TEXT("Canonical ammo permits only true N0"),
		FGridEditorItemLevelPolicy::CanAuthorItemLevel(Registry.Get(), Ammo, 0));
	TestFalse(TEXT("Canonical ammo N1 is rejected"),
		FGridEditorItemLevelPolicy::CanAuthorItemLevel(Registry.Get(), Ammo, 1));
	TestFalse(TEXT("Unknown ID cannot authorize even N0"),
		FGridEditorItemLevelPolicy::CanAuthorItemLevel(Registry.Get(), Unknown, 0));
	TestFalse(TEXT("Unknown ID cannot authorize N1"),
		FGridEditorItemLevelPolicy::CanAuthorItemLevel(Registry.Get(), Unknown, 1));
	TestFalse(TEXT("Missing editor registry fails closed"),
		FGridEditorItemLevelPolicy::CanAuthorItemLevel(nullptr, Sword, 1));

	const UWorld::InitializationValues Settings = UWorld::InitializationValues()
		.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
		.SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false,
		FName(*FString::Printf(TEXT("ARMCLEAN063AN_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
		nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("EditorPreview world"), World)) return false;
	if (GEngine)
	{
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
		Context.SetCurrentWorld(World);
	}
	AGridLevelEditorActor* Editor = World->SpawnActor<AGridLevelEditorActor>();
	if (TestNotNull(TEXT("Editor actor"), Editor))
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Editor);
		Level->Width = 2;
		Level->Height = 2;
		Level->EnsureCellCount();
		Editor->LevelAsset = Level;
		FGridLooseItemInstance& Loose = Level->LooseItemInstances.AddDefaulted_GetRef();
		Loose.InstanceId = FGuid::NewGuid();
		Loose.ItemId = Sword;
		Loose.ItemLevel = 1;
		Editor->LastSelectedObjectId = Loose.InstanceId;
		TestTrue(TEXT("Actor can author explicit canonical ARM"),
			Editor->SetSelectedObjectItemLevel(20));
		TestEqual(TEXT("Loose item fixed N20 persisted"), Loose.ItemLevel, 20);
		TestFalse(TEXT("Actor cannot restore obsolete default sentinel"),
			Editor->SetSelectedObjectItemLevel(0));
		TestEqual(TEXT("Loose item physical N20 remains"), Loose.ItemLevel, 20);
		TestFalse(TEXT("Repeated invalid N0 rejected before mutation"),
			Editor->SetSelectedObjectItemLevel(0));
		TestEqual(TEXT("Rejected edit preserves prior loose item authoring"), Loose.ItemLevel, 20);

		FGridItemSpawnInstance& Spawn = Level->ItemSpawns.AddDefaulted_GetRef();
		Spawn.SpawnId = FGuid::NewGuid();
		Spawn.ItemId = Sword;
		Spawn.ItemLevel = 1;
		Editor->LastSelectedObjectId = Spawn.SpawnId;
		TestTrue(TEXT("Generator uses same canonical authored gate"), Editor->SetSelectedObjectItemLevel(13));
		TestEqual(TEXT("Generator stores its N13"), Spawn.ItemLevel, 13);
		Spawn.ItemId = Ammo;
		TestFalse(TEXT("Ammo generator rejects invented N1"), Editor->SetSelectedObjectItemLevel(1));
		TestEqual(TEXT("Rejected generator edit does not overwrite prior authored level"), Spawn.ItemLevel, 13);
		TestTrue(TEXT("Ammo generator may set the real N0"), Editor->SetSelectedObjectItemLevel(0));
		TestEqual(TEXT("Ammo generator N0 persisted"), Spawn.ItemLevel, 0);
	}
	World->DestroyWorld(false);
	if (GEngine) GEngine->DestroyWorldContext(World);
	return !HasAnyErrors();
}
#endif

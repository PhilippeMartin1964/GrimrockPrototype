#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "EditorTools/GridEditorCanonicalItemLevelPolicy.h"
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

	TStrongObjectPtr<UGridItemDefinitionAsset> Sword(NewObject<UGridItemDefinitionAsset>());
	Sword->ItemDefinitionId = TEXT("Weapon_StraightSword_01");
	Sword->DefaultWeaponItemLevel = 7;
	Sword->bUseWeaponCatalogue = false; // The stale flag must NOT disable editing.
	TestTrue(TEXT("Canonical ARM editing enabled despite old false flag"),
		FGridEditorCanonicalItemLevelPolicy::IsLevelEditable(Registry.Get(), Sword.Get()));
	for (int32 Authored : {0, 1, 20})
	{
		TestTrue(TEXT("Canonical ARM permits inherited or actual N1-N20"),
			FGridEditorCanonicalItemLevelPolicy::CanAuthorLevel(Registry.Get(), Sword.Get(), Authored));
		FGridResolvedItem Canonical;
		const int32 Fixed = Authored == 0 ? 7 : Authored;
		TestTrue(TEXT("Same fixed level resolves in actual canonical system"),
			Registry->ResolveItem(Sword->ItemDefinitionId, Fixed, Canonical) && Canonical.bIsArmament);
	}
	TestFalse(TEXT("N21 never accepted"),
		FGridEditorCanonicalItemLevelPolicy::CanAuthorLevel(Registry.Get(), Sword.Get(), 21));
	Sword->DefaultWeaponItemLevel = 0;
	TestFalse(TEXT("ARM N0 without valid inherited default is invalid"),
		FGridEditorCanonicalItemLevelPolicy::CanAuthorLevel(Registry.Get(), Sword.Get(), 0));
	Sword->DefaultWeaponItemLevel = 7;

	TStrongObjectPtr<UGridItemDefinitionAsset> Ammo(NewObject<UGridItemDefinitionAsset>());
	Ammo->ItemDefinitionId = TEXT("Item_Arrows_01");
	Ammo->bUseWeaponCatalogue = true; // A stale flag must NOT invent an ammo level.
	TestFalse(TEXT("Canonical ammo has no level spinner"),
		FGridEditorCanonicalItemLevelPolicy::IsLevelEditable(Registry.Get(), Ammo.Get()));
	TestTrue(TEXT("Canonical ammo permits only true N0"),
		FGridEditorCanonicalItemLevelPolicy::CanAuthorLevel(Registry.Get(), Ammo.Get(), 0));
	TestFalse(TEXT("Canonical ammo N1 is rejected despite legacy flag"),
		FGridEditorCanonicalItemLevelPolicy::CanAuthorLevel(Registry.Get(), Ammo.Get(), 1));

	TStrongObjectPtr<UGridItemDefinitionAsset> Legacy(NewObject<UGridItemDefinitionAsset>());
	Legacy->ItemDefinitionId = TEXT("Legacy_Editor_063AN");
	Legacy->bUseWeaponCatalogue = false;
	TestFalse(TEXT("Unregistered non-ARM has no progressive level spinner"),
		FGridEditorCanonicalItemLevelPolicy::IsLevelEditable(Registry.Get(), Legacy.Get()));
	TestTrue(TEXT("Unknown non-levelled item keeps authored N0"),
		FGridEditorCanonicalItemLevelPolicy::CanAuthorLevel(Registry.Get(), Legacy.Get(), 0));
	TestFalse(TEXT("Unknown non-levelled item cannot invent N1"),
		FGridEditorCanonicalItemLevelPolicy::CanAuthorLevel(Registry.Get(), Legacy.Get(), 1));
	TestFalse(TEXT("Missing editor registry fails closed"),
		FGridEditorCanonicalItemLevelPolicy::CanAuthorLevel(nullptr, Sword.Get(), 1));

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
		Loose.ItemDefinition = Sword.Get();
		Loose.ItemLevel = 1;
		Editor->LastSelectedObjectId = Loose.InstanceId;
		TestTrue(TEXT("Actor can author canonical ARM despite old false flag"),
			Editor->SetSelectedObjectItemLevel(20));
		TestEqual(TEXT("Loose item fixed N20 persisted"), Loose.ItemLevel, 20);
		TestTrue(TEXT("Actor can restore valid authored default"),
			Editor->SetSelectedObjectItemLevel(0));
		TestEqual(TEXT("Loose item N0 request persisted"), Loose.ItemLevel, 0);
		Sword->DefaultWeaponItemLevel = 0;
		TestFalse(TEXT("Invalid inherited ARM default rejected before mutation"),
			Editor->SetSelectedObjectItemLevel(0));
		TestEqual(TEXT("Rejected edit preserves prior loose item authoring"), Loose.ItemLevel, 0);
		Sword->DefaultWeaponItemLevel = 7;

		FGridItemSpawnInstance& Spawn = Level->ItemSpawns.AddDefaulted_GetRef();
		Spawn.SpawnId = FGuid::NewGuid();
		Spawn.ItemDefinition = Sword.Get();
		Spawn.ItemLevel = 1;
		Editor->LastSelectedObjectId = Spawn.SpawnId;
		TestTrue(TEXT("Generator uses same canonical authored gate"), Editor->SetSelectedObjectItemLevel(13));
		TestEqual(TEXT("Generator stores its N13"), Spawn.ItemLevel, 13);
		Spawn.ItemDefinition = Ammo.Get();
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

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Runtime/GridItemActor.h"
#include "GridCanonicalLootTestScope.h"

namespace
{
	struct FGridItemSparkleTestWorld
	{
		UWorld* World = nullptr;

		FGridItemSparkleTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(
				EWorldType::Game, false,
				FName(*FString::Printf(TEXT("ITEM_SPARKLE01_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);

			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridItemSparkleTestWorld()
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemWorldSparklePresentationTest,
	"Grimrock.Items.ITEM_SPARKLE01.WorldPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemWorldSparklePresentationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridItemSparkleTestWorld TestWorld;
	if (!TestNotNull(TEXT("Sparkle test world exists"), TestWorld.World))
	{
		return false;
	}

	AGridItemActor* ItemActor = TestWorld.World->SpawnActor<AGridItemActor>();
	if (!TestNotNull(TEXT("Item actor exists"), ItemActor))
	{
		return false;
	}

	UStaticMesh* WorldMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* SparkleMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
	if (!TestNotNull(TEXT("Fixture mesh"), WorldMesh) || !TestNotNull(TEXT("Fixture material"), SparkleMaterial))
		return false;
	FGridCanonicalLootTestScope Scope;
	if (!TestTrue(TEXT("Isolated item system registered"), Scope.AttachWithWeights(TestWorld.World, {},
		[&](UGridItemSystemAsset& Root)
		{
			auto* Definition = NewObject<UGridNonArmamentDefinitionAsset>(&Root);
			Definition->ItemId = TEXT("Item_Gem_Sparkle_Test");
			Definition->bEnableWorldSparkle = true;
			Definition->WorldSparkleColor = FLinearColor(0.78f, 0.38f, 0.12f, 1.0f);
			Definition->WorldSparkleIntensity = 2.0f;
			Definition->WorldSparkleSpeed = 1.2f;
			Definition->WorldSparkleVariation = 0.35f;
			Definition->WorldMesh = TSoftObjectPtr<UStaticMesh>(WorldMesh);
			Definition->WorldSparkleMaterial = TSoftObjectPtr<UMaterialInterface>(SparkleMaterial);
			Definition->DisplayName = FText::FromName(Definition->ItemId);
			Root.ItemDefinitions.Add(Definition);
			auto* DisabledDefinition = NewObject<UGridNonArmamentDefinitionAsset>(&Root);
			DisabledDefinition->ItemId = TEXT("Item_Quest_NoSparkle_Test");
			DisabledDefinition->bEnableWorldSparkle = false;
			DisabledDefinition->WorldMesh = TSoftObjectPtr<UStaticMesh>(WorldMesh);
			DisabledDefinition->WorldSparkleMaterial = TSoftObjectPtr<UMaterialInterface>(SparkleMaterial);
			DisabledDefinition->DisplayName = FText::FromName(DisabledDefinition->ItemId);
			Root.ItemDefinitions.Add(DisabledDefinition);

			auto* MissingMaterial = NewObject<UGridNonArmamentDefinitionAsset>(&Root);
			MissingMaterial->ItemId = TEXT("Item_Key_Iron_InvalidSparkle");
			MissingMaterial->DisplayName = FText::FromName(MissingMaterial->ItemId);
			MissingMaterial->bEnableWorldSparkle = true;
			MissingMaterial->WorldMesh = WorldMesh;
			Root.ItemDefinitions.Add(MissingMaterial);
		})))
	{
		return false;
	}

	const FGuid RuntimeId = FGuid::NewGuid();
	FGridItemInstance Instance;
	Instance.ItemDefinitionId = TEXT("Item_Gem_Sparkle_Test");
	Instance.RuntimeObjectId = RuntimeId;
	Instance.Quantity = 1;
	Instance.OwnerType = EGridItemOwnerType::World;
	if (!TestTrue(TEXT("Sparkle item initializes"), ItemActor->InitializeFromItemInstance(Instance, Scope.Registry))) return false;

	TestNotNull(TEXT("Sparkle component exists"), ItemActor->SparkleMeshComponent.Get());
	TestTrue(TEXT("Sparkle component mirrors the item world mesh"),
		ItemActor->SparkleMeshComponent && ItemActor->SparkleMeshComponent->GetStaticMesh() == WorldMesh);
	TestFalse(TEXT("Sparkle is inactive immediately after initialization"), ItemActor->IsWorldSparkleActive());
	TestFalse(TEXT("Sparkle mesh starts hidden"),
		ItemActor->SparkleMeshComponent && ItemActor->SparkleMeshComponent->IsVisible());
	const float InitialPhase = ItemActor->BuildWorldSparklePhase();
	TestTrue(TEXT("Sparkle phase is normalized"), InitialPhase >= 0.0f && InitialPhase < 1.0f);

	AGridItemActor* SameRuntimeIdActor = TestWorld.World->SpawnActor<AGridItemActor>();
	if (!TestNotNull(TEXT("Second actor"), SameRuntimeIdActor) ||
		!TestTrue(TEXT("Same identity initializes"), SameRuntimeIdActor->InitializeFromItemInstance(Instance, Scope.Registry))) return false;
	TestEqual(TEXT("Sparkle phase is deterministic for the same runtime id"), SameRuntimeIdActor->BuildWorldSparklePhase(), InitialPhase);

	ItemActor->ConfigureAsWorldPickup();
	TestTrue(TEXT("World pickup enables configured sparkle"), ItemActor->IsWorldSparkleActive());
	TestTrue(TEXT("World pickup shows sparkle mesh"),
		ItemActor->SparkleMeshComponent && ItemActor->SparkleMeshComponent->IsVisible());
	TestNotNull(TEXT("World pickup creates sparkle MID"), ItemActor->WorldSparkleMaterialInstance.Get());
	TestEqual(TEXT("Sparkle mesh never gains collision"),
		ItemActor->SparkleMeshComponent->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	TestFalse(TEXT("Sparkle mesh never simulates physics"), ItemActor->SparkleMeshComponent->IsSimulatingPhysics());
	TestFalse(TEXT("Sparkle mesh never uses gravity"), ItemActor->SparkleMeshComponent->IsGravityEnabled());

	ItemActor->ConfigureAsAttachedItem();
	TestFalse(TEXT("Attached item disables world sparkle"), ItemActor->IsWorldSparkleActive());
	TestFalse(TEXT("Attached item hides sparkle mesh"), ItemActor->SparkleMeshComponent->IsVisible());
	ItemActor->OnPlacedInWorld();
	TestFalse(TEXT("OnPlacedInWorld does not re-enable sparkle for an attached item"), ItemActor->IsWorldSparkleActive());
	TestFalse(TEXT("Attached item remains visually non-sparkling after OnPlacedInWorld"), ItemActor->SparkleMeshComponent->IsVisible());

	ItemActor->ConfigureAsWorldPickup();
	TestTrue(TEXT("Returning item to world re-enables sparkle"), ItemActor->IsWorldSparkleActive());
	ItemActor->OnRemovedFromWorld();
	TestFalse(TEXT("Removing item from world disables sparkle"), ItemActor->IsWorldSparkleActive());
	TestFalse(TEXT("Removing item from world hides sparkle mesh"), ItemActor->SparkleMeshComponent->IsVisible());

	AGridItemActor* DisabledActor = TestWorld.World->SpawnActor<AGridItemActor>();
	Instance.ItemDefinitionId = TEXT("Item_Quest_NoSparkle_Test");
	Instance.RuntimeObjectId = FGuid::NewGuid();
	if (!TestNotNull(TEXT("Non-sparkling actor"), DisabledActor) ||
		!TestTrue(TEXT("Non-sparkling item initializes"), DisabledActor->InitializeFromItemInstance(Instance, Scope.Registry))) return false;
	DisabledActor->ConfigureAsWorldPickup();
	TestFalse(TEXT("Disabled world sparkle remains inactive on a world pickup"), DisabledActor->IsWorldSparkleActive());
	TestFalse(TEXT("Disabled world sparkle keeps overlay hidden"), DisabledActor->SparkleMeshComponent->IsVisible());

	AGridItemActor* MissingMaterialActor = TestWorld.World->SpawnActor<AGridItemActor>();
	Instance.ItemDefinitionId = TEXT("Item_Key_Iron_InvalidSparkle");
	Instance.RuntimeObjectId = FGuid::NewGuid();
	if (!TestNotNull(TEXT("Actor without a sparkle material"), MissingMaterialActor) ||
		!TestTrue(TEXT("Missing sparkle material does not prevent materialization"),
			MissingMaterialActor->InitializeFromItemInstance(Instance, Scope.Registry))) return false;
	MissingMaterialActor->ConfigureAsWorldPickup();
	TestFalse(TEXT("Missing sparkle material never activates presentation"), MissingMaterialActor->IsWorldSparkleActive());
	TestFalse(TEXT("Missing sparkle material keeps overlay hidden"), MissingMaterialActor->SparkleMeshComponent->IsVisible());

	const UGridItemSubsystem* Registry = Scope.Registry;
	if (TestNotNull(TEXT("Configured registry for sparkle cutover"), Registry))
	{
		TestTrue(TEXT("Real registry configured"), Registry->IsConfigured());
		ItemActor->ItemDefinitionId = TEXT("Unregistered_Sparkle_Identity");
		ItemActor->SetWorldSparkleEnabled(true);
		TestFalse(TEXT("Unknown ID cannot enable historical sparkle"), ItemActor->IsWorldSparkleActive());
		TestFalse(TEXT("Unknown overlay hidden"), ItemActor->SparkleMeshComponent->IsVisible());
		TestNull(TEXT("Unknown overlay owns no MID"), ItemActor->WorldSparkleMaterialInstance.Get());

		FGridItemInstance Key;
		Key.RuntimeObjectId = FGuid::NewGuid();
		Key.ItemDefinitionId = TEXT("Key_Copper");
		Key.ItemLevel = 0;
		Key.Quantity = 1;
		FGridResolvedItem Canonical;
		if (TestTrue(TEXT("Actual copper key resolves"), Registry->ResolveItem(Key.ItemDefinitionId, 0, Canonical)))
		{
			AGridItemActor* CanonicalActor = TestWorld.World->SpawnActor<AGridItemActor>();
			if (TestNotNull(TEXT("Canonical key actor"), CanonicalActor) &&
				TestTrue(TEXT("Canonical key initialized"), CanonicalActor->InitializeFromItemInstance(Key, Registry)))
			{
				const bool Expected = Canonical.bEnableWorldSparkle &&
					Canonical.WorldSparkleMaterial.LoadSynchronous() != nullptr &&
					CanonicalActor->MeshComponent->GetStaticMesh() != nullptr;
				CanonicalActor->SetWorldSparkleEnabled(true);
				TestEqual(TEXT("Canonical sparkle policy wins"), CanonicalActor->IsWorldSparkleActive(), Expected);
				TestEqual(TEXT("Canonical overlay visibility"), CanonicalActor->SparkleMeshComponent->IsVisible(), Expected);
				CanonicalActor->WorldItemLevel = 1;
				CanonicalActor->SetWorldSparkleEnabled(true);
				TestFalse(TEXT("Invalid non-ARM level refuses sparkle"), CanonicalActor->IsWorldSparkleActive());
				TestFalse(TEXT("Invalid level clears overlay"), CanonicalActor->SparkleMeshComponent->IsVisible());
			}
		}
	}
	return !HasAnyErrors();
}

#endif

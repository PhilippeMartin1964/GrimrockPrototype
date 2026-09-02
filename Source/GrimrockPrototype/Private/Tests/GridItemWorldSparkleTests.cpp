#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"

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

	UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(ItemActor);
	Definition->ItemDefinitionId = TEXT("Item_Gem_Sparkle_Test");
	Definition->ItemType = EGridItemType::Gem;
	Definition->bEnableWorldSparkle = true;
	Definition->WorldSparkleColor = FLinearColor(0.78f, 0.38f, 0.12f, 1.0f);
	Definition->WorldSparkleIntensity = 2.0f;
	Definition->WorldSparkleSpeed = 1.2f;
	Definition->WorldSparkleVariation = 0.35f;

	UStaticMesh* WorldMesh = NewObject<UStaticMesh>(Definition, TEXT("SM_Gem_Sparkle_Test"));
	UMaterial* SparkleMaterial = NewObject<UMaterial>(Definition, TEXT("M_ItemWorldSparkle_Test"));
	Definition->WorldMesh = TSoftObjectPtr<UStaticMesh>(WorldMesh);
	Definition->WorldSparkleMaterial = TSoftObjectPtr<UMaterialInterface>(SparkleMaterial);

	TestTrue(TEXT("Enabled sparkle definition with material is valid"), Definition->IsValidDefinition());

	const FGuid RuntimeId = FGuid::NewGuid();
	ItemActor->InitializeFromItemDefinition(Definition, RuntimeId);

	TestNotNull(TEXT("Sparkle component exists"), ItemActor->SparkleMeshComponent.Get());
	TestTrue(TEXT("Sparkle component mirrors the item world mesh"),
		ItemActor->SparkleMeshComponent && ItemActor->SparkleMeshComponent->GetStaticMesh() == WorldMesh);
	TestFalse(TEXT("Sparkle is inactive immediately after initialization"), ItemActor->IsWorldSparkleActive());
	TestFalse(TEXT("Sparkle mesh starts hidden"),
		ItemActor->SparkleMeshComponent && ItemActor->SparkleMeshComponent->IsVisible());
	const float InitialPhase = ItemActor->BuildWorldSparklePhase();
	TestTrue(TEXT("Sparkle phase is normalized"), InitialPhase >= 0.0f && InitialPhase < 1.0f);

	AGridItemActor* SameRuntimeIdActor = TestWorld.World->SpawnActor<AGridItemActor>();
	SameRuntimeIdActor->InitializeFromItemDefinition(Definition, RuntimeId);
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

	UGridItemDefinitionAsset* DisabledDefinition = NewObject<UGridItemDefinitionAsset>(ItemActor);
	DisabledDefinition->ItemDefinitionId = TEXT("Item_Quest_NoSparkle_Test");
	DisabledDefinition->ItemType = EGridItemType::Quest;
	DisabledDefinition->bEnableWorldSparkle = false;
	DisabledDefinition->WorldMesh = TSoftObjectPtr<UStaticMesh>(WorldMesh);
	DisabledDefinition->WorldSparkleMaterial = TSoftObjectPtr<UMaterialInterface>(SparkleMaterial);
	AGridItemActor* DisabledActor = TestWorld.World->SpawnActor<AGridItemActor>();
	DisabledActor->InitializeFromItemDefinition(DisabledDefinition, FGuid::NewGuid());
	DisabledActor->ConfigureAsWorldPickup();
	TestFalse(TEXT("Disabled world sparkle remains inactive on a world pickup"), DisabledActor->IsWorldSparkleActive());
	TestFalse(TEXT("Disabled world sparkle keeps overlay hidden"), DisabledActor->SparkleMeshComponent->IsVisible());

	UGridItemDefinitionAsset* MissingMaterialDefinition = NewObject<UGridItemDefinitionAsset>(ItemActor);
	MissingMaterialDefinition->ItemDefinitionId = TEXT("Item_Key_Iron_InvalidSparkle");
	MissingMaterialDefinition->ItemType = EGridItemType::Key;
	MissingMaterialDefinition->bEnableWorldSparkle = true;
	MissingMaterialDefinition->WorldMesh = TSoftObjectPtr<UStaticMesh>(WorldMesh);
	TestFalse(TEXT("Enabled sparkle without material is invalid"), MissingMaterialDefinition->IsValidDefinition());

	AGridItemActor* MissingMaterialActor = TestWorld.World->SpawnActor<AGridItemActor>();
	MissingMaterialActor->InitializeFromItemDefinition(MissingMaterialDefinition, FGuid::NewGuid());
	MissingMaterialActor->ConfigureAsWorldPickup();
	TestFalse(TEXT("Missing sparkle material never activates presentation"), MissingMaterialActor->IsWorldSparkleActive());
	TestFalse(TEXT("Missing sparkle material keeps overlay hidden"), MissingMaterialActor->SparkleMeshComponent->IsVisible());

	return true;
}

#endif

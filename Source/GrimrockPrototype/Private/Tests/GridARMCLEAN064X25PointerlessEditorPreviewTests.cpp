#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/GridEditorPreviewComponent.h"
#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridEditorLooseItemPreviewReadModel.h"

namespace ARMCLEAN064X25
{
	struct FFixture
	{
		UWorld* World = nullptr;
		UGameInstance* GI = nullptr;
		UGridItemSubsystem* Registry = nullptr;
		AGridLevelRuntimeActor* Runtime = nullptr;
		UGridEditorPreviewComponent* Preview = nullptr;

		FFixture()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false)
				.CreatePhysicsScene(false).CreateNavigation(false)
				.CreateAISystem(false).ShouldSimulatePhysics(false)
				.SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("ARM_CLEAN064X25_%s"),
					*FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (!World || !GEngine) { return; }
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			GI = NewObject<UGameInstance>(GetTransientPackage());
			if (!GI) { return; }
			GI->Init();
			World->SetGameInstance(GI);
			Registry = GI->GetSubsystem<UGridItemSubsystem>();
			Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
			if (!Runtime) { return; }
			UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
			Level->Width = 2;
			Level->Height = 2;
			Level->CellSize = 200.f;
			Level->EnsureCellCount();
			Runtime->LevelAsset = Level;
			Preview = NewObject<UGridEditorPreviewComponent>(Runtime);
			Preview->Initialize(Runtime);
			Preview->RegisterComponent();
		}
		~FFixture()
		{
			if (World) { World->SetGameInstance(nullptr); }
			if (GI) { GI->Shutdown(); }
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine) { GEngine->DestroyWorldContext(World); }
			}
		}

		AGridEditorPreviewObjectActor* Find(FGuid Id) const
		{
			if (!World) { return nullptr; }
			for (TActorIterator<AGridEditorPreviewObjectActor> It(World); It; ++It)
			{
				if (IsValid(*It) && It->ObjectId == Id) { return *It; }
			}
			return nullptr;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X25PointerlessEditorPreview,
	"Grimrock.RPG.ARM.CLEAN06_4X25.PointerlessCanonicalEditorPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X25PointerlessEditorPreview::RunTest(const FString&)
{
	ARMCLEAN064X25::FFixture F;
	if (!TestNotNull(TEXT("World"), F.World) ||
		!TestNotNull(TEXT("Canonical registry"), F.Registry) ||
		!TestNotNull(TEXT("Runtime"), F.Runtime) ||
		!TestNotNull(TEXT("Preview component"), F.Preview))
	{
		return false;
	}
	FGridLooseItemInstance Placement;
	Placement.InstanceId = FGuid::NewGuid();
	Placement.CellX = 0;
	Placement.CellY = 0;
	Placement.ItemId = TEXT("Weapon_Dagger_01");
	Placement.ItemLevel = 20;
	Placement.Quantity = 1;

	FGridResolvedItem Expected;
	if (!TestTrue(TEXT("Real N20 ARM resolves"), F.Registry->ResolveItem(
			Placement.ItemId, Placement.ItemLevel, Expected)))
	{
		return false;
	}
	const auto Preflight = FGridEditorLooseItemPreviewReadModel::Resolve(
		F.Registry, Placement);
	if (!TestTrue(TEXT("Explicit canonical ItemId recognized without legacy asset"),
			Preflight.bRegisteredIdentity && Preflight.bResolved) ||
		!TestEqual(TEXT("Physical N20 level preserved"), Preflight.EffectiveItemLevel, 20) ||
		!TestTrue(TEXT("Preview mesh is canonical"), Preflight.WorldMesh == Expected.WorldMesh) ||
		!TestTrue(TEXT("Preview transform is canonical"),
			Preflight.WorldTransform.Equals(Expected.WorldVisualTransform)))
	{
		return false;
	}

	F.Runtime->LevelAsset->LooseItemInstances.Add(Placement);
	F.Preview->RebuildPreviewObjects();
	AGridEditorPreviewObjectActor* ItemPreview = F.Find(Placement.InstanceId);
	if (!TestNotNull(TEXT("Actual pointerless preview actor created"), ItemPreview) ||
		!TestNotNull(TEXT("Preview static mesh component"), ItemPreview->MeshComponent) ||
		!TestTrue(TEXT("Editor preview actor owns registered ARM mesh"),
			ItemPreview->MeshComponent->GetStaticMesh() ==
				Expected.WorldMesh.LoadSynchronous()) ||
		!TestTrue(TEXT("Preview actor uses canonical visual transform"),
			ItemPreview->MeshComponent->GetRelativeTransform().Equals(
				Expected.WorldVisualTransform)))
	{
		return false;
	}

	for (const int32 Level : {1, 0})
	{
		Placement.ItemLevel = Level;
		const auto Resolution = FGridEditorLooseItemPreviewReadModel::Resolve(
			F.Registry, Placement);
		if (!TestTrue(TEXT("Explicit ARM remains classified canonical"),
			Resolution.bRegisteredIdentity) ||
			!TestEqual(TEXT("ARM N0 rejected and N1 accepted"),
				Resolution.bResolved, Level == 1))
		{
			return false;
		}
	}

	Placement.ItemId = TEXT("Item_Arrows_01");
	Placement.ItemLevel = 0;
	FGridResolvedItem Arrows;
	if (!TestTrue(TEXT("Real non-ARM arrows resolve at N0"),
			F.Registry->ResolveItem(Placement.ItemId, 0, Arrows)) ||
		!TestTrue(TEXT("Pointerless arrows preview works at N0"),
			FGridEditorLooseItemPreviewReadModel::Resolve(
				F.Registry, Placement).bResolved))
	{
		return false;
	}
	Placement.ItemLevel = 1;
	if (!TestFalse(TEXT("Registered non-ARM N1 cannot be previewed"),
		FGridEditorLooseItemPreviewReadModel::Resolve(F.Registry, Placement).bResolved))
	{
		return false;
	}

	Placement.ItemLevel = 20;
	Placement.ItemId = TEXT("Weapon_Dagger_01");
	if (!TestNull(TEXT("Old placement source cannot be represented"), FGridLooseItemInstance::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition"))))
		return false;
	Placement.ItemId = NAME_None;
	if (!TestFalse(TEXT("Missing identity cannot preview"),
		FGridEditorLooseItemPreviewReadModel::Resolve(F.Registry, Placement).bResolved))
		return false;
	Placement.ItemId = TEXT("Weapon_Dagger_01");
	if (!TestTrue(TEXT("No registry cannot authorize legacy fallback"),
		FGridEditorLooseItemPreviewReadModel::Resolve(
			nullptr, Placement).bRegisteredIdentity) ||
		!TestFalse(TEXT("No registry means no resolved preview"),
			FGridEditorLooseItemPreviewReadModel::Resolve(
				nullptr, Placement).bResolved))
	{
		return false;
	}
	Placement.ItemId = TEXT("ARM_CLEAN064X25_Unknown");
	if (!TestFalse(TEXT("Unregistered persisted ID fails closed"),
		FGridEditorLooseItemPreviewReadModel::Resolve(
			F.Registry, Placement).bResolved))
	{
		return false;
	}

	AddInfo(TEXT("[ARM-CLEAN06.4X.25] pointerless registered ARM N20 editor preview actor, "
		"N1/N0 physical validation, non-ARM N0, missing registry and bad source."));
	return true;
}
#endif

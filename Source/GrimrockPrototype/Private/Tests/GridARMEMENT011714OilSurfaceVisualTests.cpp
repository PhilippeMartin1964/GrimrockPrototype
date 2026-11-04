#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Engine/StaticMesh.h"
#include "MaterialDomain.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Save/GrimrockPartySaveGame.h"

namespace ARMEMENT011714
{
	struct FWorld
	{
		UWorld* World = nullptr;
		FWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false)
				.CreatePhysicsScene(false).CreateNavigation(false)
				.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("OilVis_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}
		~FWorld()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine) { GEngine->DestroyWorldContext(World); }
			}
		}
	};

	AGridLevelRuntimeActor* MakeRuntime(FWorld& Fixture)
	{
		if (!Fixture.World) { return nullptr; }
		AGridLevelRuntimeActor* Runtime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
		if (!Runtime) { return nullptr; }
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
		Level->Width = 4;
		Level->Height = 4;
		Level->CellSize = 200.f;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		Runtime->LevelAsset = Level;
		Runtime->CurrentDungeonLevelId = TEXT("OilVis_Level");
		// Material testé indépendamment par le test de production Editor.
		Runtime->OilSurfaceMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
		return Runtime;
	}

	bool Apply(AGridLevelRuntimeActor* Runtime, int32 X, int32 Y, EGridCombatSurfaceType Type, int32 Rounds = 2)
	{
		FGridCombatSurfaceEffectProfile Profile;
		Profile.SurfaceType = Type;
		Profile.DurationRounds = Rounds;
		return Runtime->ApplyCombatSurfaceAtCell(
			X, Y, Profile, FGuid(17, 1, 14, 1), TEXT("Action_OilVis"), FGridResolvedCombatModifiers());
	}

	int32 Count(const AGridLevelRuntimeActor* Runtime)
	{
		const UInstancedStaticMeshComponent* C = Runtime ? Runtime->GetCombatOilSurfaceVisualComponent() : nullptr;
		return C ? C->GetInstanceCount() : INDEX_NONE;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOilVisCells,
	"Grimrock.RPG.ARMEMENT01.17.1.4.OilCellProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOilVisCells::RunTest(const FString&)
{
	ARMEMENT011714::FWorld World;
	AGridLevelRuntimeActor* Runtime = ARMEMENT011714::MakeRuntime(World);
	if (!TestNotNull(TEXT("Monde et acteur"), Runtime)) return false;
	if (!TestNotNull(TEXT("Mesh Engine Plane"), Runtime->SurfaceOverlayMesh.Get())) return false;
	TestTrue(TEXT("Huile 1,1"), ARMEMENT011714::Apply(Runtime, 1, 1, EGridCombatSurfaceType::Oil));
	TestTrue(TEXT("Huile 2,1"), ARMEMENT011714::Apply(Runtime, 2, 1, EGridCombatSurfaceType::Oil));
	TestTrue(TEXT("Eau indépendante"), ARMEMENT011714::Apply(Runtime, 0, 0, EGridCombatSurfaceType::Water));
	TestFalse(TEXT("Hors grille"), ARMEMENT011714::Apply(Runtime, 4, 4, EGridCombatSurfaceType::Oil));
	TestEqual(TEXT("Deux instances, pas trois"), ARMEMENT011714::Count(Runtime), 2);
	const UInstancedStaticMeshComponent* Visual = Runtime->GetCombatOilSurfaceVisualComponent();
	TestTrue(TEXT("Le mesh du plan est réellement affecté"), Visual && Visual->GetStaticMesh() == Runtime->SurfaceOverlayMesh);
	TestTrue(TEXT("Le matériau de test est lié au slot 0"), Visual && Visual->GetMaterial(0) == UMaterial::GetDefaultMaterial(MD_Surface));
	TestTrue(TEXT("Composant visible en présence d'huile"), Visual && Visual->IsVisible());
	TSet<FIntPoint> Cells;
	for (int32 I = 0; Visual && I < Visual->GetInstanceCount(); ++I)
	{
		FTransform Local;
		if (!TestTrue(TEXT("Transformation locale valide"), Visual->GetInstanceTransform(I, Local, false))) return false;
		const FVector P = Local.GetLocation() - Runtime->GridOrigin;
		TestTrue(TEXT("Décalage Z"), FMath::IsNearlyEqual(P.Z, Runtime->OilSurfaceHeightOffset, 0.1f));
		Cells.Add(FIntPoint(FMath::RoundToInt(P.X / 200.f - .5f), FMath::RoundToInt(P.Y / 200.f - .5f)));
	}
	TestTrue(TEXT("Case 1,1"), Cells.Contains(FIntPoint(1, 1)));
	TestTrue(TEXT("Case 2,1"), Cells.Contains(FIntPoint(2, 1)));
	TestFalse(TEXT("Pas d'huile sur l'eau"), Cells.Contains(FIntPoint(0, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOilVisFloorThickness,
	"Grimrock.RPG.ARMEMENT01.17.1.4.OverlayAboveFloorThickness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOilVisFloorThickness::RunTest(const FString&)
{
	ARMEMENT011714::FWorld World;
	AGridLevelRuntimeActor* Runtime = ARMEMENT011714::MakeRuntime(World);
	if (!TestNotNull(TEXT("Acteur runtime"), Runtime)) return false;
	UStaticMesh* ThickFloor = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Sol de référence épais"), ThickFloor)) return false;
	Runtime->FloorMesh = ThickFloor;
	const float Top = ThickFloor->GetBoundingBox().Max.Z;
	TestTrue(TEXT("Sol de test plus épais que le décalage historique"), Top > Runtime->OilSurfaceHeightOffset + 1.f);
	if (!TestTrue(TEXT("Surface d'huile autoritaire"), ARMEMENT011714::Apply(Runtime, 1, 2, EGridCombatSurfaceType::Oil))) return false;
	const UInstancedStaticMeshComponent* Oil = Runtime->GetCombatOilSurfaceVisualComponent();
	if (!TestTrue(TEXT("Une instance créée"), Oil && Oil->GetInstanceCount() == 1)) return false;
	FTransform Local;
	if (!TestTrue(TEXT("Transform locale disponible"), Oil->GetInstanceTransform(0, Local, false))) return false;
	const float LocalZ = Local.GetLocation().Z - Runtime->GridOrigin.Z;
	TestTrue(TEXT("La flaque est au-dessus de la face haute du plancher"), LocalZ > Top);
	TestTrue(TEXT("La hauteur du plancher et la marge d'huile sont respectées"),
		FMath::IsNearlyEqual(LocalZ, Top + Runtime->OilSurfaceHeightOffset, 0.1f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOilVisReaction,
	"Grimrock.RPG.ARMEMENT01.17.1.4.ReactionsAndConversions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOilVisReaction::RunTest(const FString&)
{
	ARMEMENT011714::FWorld World;
	auto* Runtime = ARMEMENT011714::MakeRuntime(World);
	if (!TestNotNull(TEXT("Acteur"), Runtime)) return false;
	TestTrue(TEXT("Huile"), ARMEMENT011714::Apply(Runtime, 1, 1, EGridCombatSurfaceType::Oil, 3));
	TestTrue(TEXT("Autre huile"), ARMEMENT011714::Apply(Runtime, 2, 2, EGridCombatSurfaceType::Oil, 3));
	TestEqual(TEXT("Deux nappes"), ARMEMENT011714::Count(Runtime), 2);
	FGridCombatSurfaceReactionResult Reaction;
	TestTrue(TEXT("Huile + feu"), Runtime->InteractCombatSurfaceAtCell(
		1, 1, EGridCombatSurfaceInteraction::Fire, FGridResolvedCombatModifiers(), Reaction));
	const FGridCombatSurfaceState* Converted = Runtime->FindCombatSurfaceAtCell(1, 1);
	TestTrue(TEXT("État Fire autoritaire"), Converted && Converted->SurfaceType == EGridCombatSurfaceType::Fire);
	TestEqual(TEXT("Une seule nappe d'huile"), ARMEMENT011714::Count(Runtime), 1);
	FGridCombatSurfaceConversionProfile Profile;
	Profile.bAllowEmptyCell = true;
	Profile.OutputSurfaceType = EGridCombatSurfaceType::Oil;
	Profile.EmptyCellDurationRounds = 2;
	TestTrue(TEXT("Conversion -> huile"), Runtime->ConvertCombatSurfaceAtCell(
		3, 3, Profile, FGuid::NewGuid(), TEXT("Action_Conversion"), FGridResolvedCombatModifiers()));
	TestEqual(TEXT("Deux nappes après conversion"), ARMEMENT011714::Count(Runtime), 2);
	TestTrue(TEXT("Remplacement par eau"), ARMEMENT011714::Apply(Runtime, 3, 3, EGridCombatSurfaceType::Water));
	TestEqual(TEXT("Une nappe après remplacement"), ARMEMENT011714::Count(Runtime), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOilVisExpiry,
	"Grimrock.RPG.ARMEMENT01.17.1.4.Expiration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOilVisExpiry::RunTest(const FString&)
{
	ARMEMENT011714::FWorld World;
	auto* Runtime = ARMEMENT011714::MakeRuntime(World);
	if (!TestNotNull(TEXT("Acteur"), Runtime)) return false;
	TestTrue(TEXT("Huile à durée de deux rounds"), ARMEMENT011714::Apply(Runtime, 2, 2, EGridCombatSurfaceType::Oil, 2));
	TestEqual(TEXT("Nappe présente"), ARMEMENT011714::Count(Runtime), 1);
	TestEqual(TEXT("Premier round"), Runtime->AdvanceCombatSurfaceRound(), 0);
	TestEqual(TEXT("Nappe conservée"), ARMEMENT011714::Count(Runtime), 1);
	TestEqual(TEXT("Second round"), Runtime->AdvanceCombatSurfaceRound(), 1);
	TestNull(TEXT("Surface expirée"), Runtime->FindCombatSurfaceAtCell(2, 2));
	TestEqual(TEXT("Aucune instance orpheline"), ARMEMENT011714::Count(Runtime), 0);
	const UInstancedStaticMeshComponent* Visual = Runtime->GetCombatOilSurfaceVisualComponent();
	TestFalse(TEXT("Composant masqué après disparition de l'huile"), Visual && Visual->IsVisible());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOilVisPersistence,
	"Grimrock.RPG.ARMEMENT01.17.1.4.SaveLoadRebuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOilVisPersistence::RunTest(const FString&)
{
	ARMEMENT011714::FWorld World;
	auto* Runtime = ARMEMENT011714::MakeRuntime(World);
	if (!TestNotNull(TEXT("Acteur"), Runtime)) return false;
	TestTrue(TEXT("Huile A"), ARMEMENT011714::Apply(Runtime, 0, 0, EGridCombatSurfaceType::Oil, 4));
	TestTrue(TEXT("Huile B"), ARMEMENT011714::Apply(Runtime, 1, 1, EGridCombatSurfaceType::Oil, 2));
	TestTrue(TEXT("Poison indépendant"), ARMEMENT011714::Apply(Runtime, 2, 2, EGridCombatSurfaceType::Poison, 3));
	TestTrue(TEXT("Capture d'état"), Runtime->CaptureCurrentLevelRuntimeState());
	auto* Save = NewObject<UGrimrockPartySaveGame>();
	Save->DungeonRuntimeState = Runtime->DungeonRuntimeState;
	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("Sérialisation SaveGame"), UGameplayStatics::SaveGameToMemory(Save, Bytes))) return false;
	auto* Reloaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("Désérialisation SaveGame"), Reloaded)) return false;
	Runtime->DungeonRuntimeState = FGridDungeonRuntimeState();
	Runtime->RefreshCombatOilSurfaceVisuals();
	TestEqual(TEXT("Reset des instances"), ARMEMENT011714::Count(Runtime), 0);
	Runtime->DungeonRuntimeState = Reloaded->DungeonRuntimeState;
	Runtime->RebuildLevel(EGridRuntimeRebuildMode::GeometryOnly);
	TestEqual(TEXT("Rebuild reconstitue les deux nappes"), ARMEMENT011714::Count(Runtime), 2);
	TestTrue(TEXT("Restauration de l'état"), Runtime->ApplyCurrentLevelRuntimeState());
	TestEqual(TEXT("Deux nappes restaurées"), ARMEMENT011714::Count(Runtime), 2);
	const FGridCombatSurfaceState* Oil = Runtime->FindCombatSurfaceAtCell(0, 0);
	TestTrue(TEXT("Durée de quatre rounds conservée"), Oil &&
		Oil->SurfaceType == EGridCombatSurfaceType::Oil && Oil->RemainingRounds == 4);
	const FGridCombatSurfaceState* Poison = Runtime->FindCombatSurfaceAtCell(2, 2);
	TestTrue(TEXT("Poison non modifié"), Poison && Poison->SurfaceType == EGridCombatSurfaceType::Poison);
	return true;
}
#endif // WITH_DEV_AUTOMATION_TESTS

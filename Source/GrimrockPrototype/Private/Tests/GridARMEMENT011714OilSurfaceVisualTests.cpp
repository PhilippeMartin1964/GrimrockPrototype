#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridCatalystVfxProfileAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "MaterialDomain.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/Combat/GridCatalystAreaResolver.h"
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
				.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false)
				.ShouldSimulatePhysics(false).SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("OilDecal_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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
				if (GEngine) GEngine->DestroyWorldContext(World);
			}
		}
	};

	AGridLevelRuntimeActor* MakeRuntime(FWorld& Fixture)
	{
		if (!Fixture.World) return nullptr;
		AGridLevelRuntimeActor* Runtime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
		if (!Runtime) return nullptr;
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
		Runtime->CurrentDungeonLevelId = TEXT("DecalOilTest");
		Runtime->OilSurfaceMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
		// Explicit test authoring: no visual asset paths embedded in production runtime.
		UGridCatalystVfxProfileAsset* CatalystVfx = NewObject<UGridCatalystVfxProfileAsset>(Runtime);
		CatalystVfx->Settings.ProjectileMesh = LoadObject<UStaticMesh>(nullptr,
			TEXT("/Game/GrimrockPrototype/Meshes/Items/SM_Catalyst_01.SM_Catalyst_01"));
		CatalystVfx->Settings.RunePulseMaterial = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/GrimrockPrototype/Art/Materials/Catalyst/M_SurfaceCatalyst_RunePulse.M_SurfaceCatalyst_RunePulse"));
		CatalystVfx->Settings.EmberMaterial = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/GrimrockPrototype/Art/Materials/Catalyst/M_SurfaceCatalyst_Ember.M_SurfaceCatalyst_Ember"));
		CatalystVfx->Settings.SparkMesh = LoadObject<UStaticMesh>(nullptr,
			TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		Runtime->CatalystVfxProfile = CatalystVfx;
		return Runtime;
	}

	bool Apply(AGridLevelRuntimeActor* Runtime, int32 X, int32 Y,
		EGridCombatSurfaceType Type, int32 Rounds = 2)
	{
		FGridCombatSurfaceEffectProfile P;
		P.SurfaceType = Type;
		P.DurationRounds = Rounds;
		return Runtime->ApplyCombatSurfaceAtCell(
			X, Y, P, FGuid(17, 1, 14, 1), TEXT("OilDecalTest"), FGridResolvedCombatModifiers());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOilDecalProjection,
	"Grimrock.RPG.ARMEMENT01.17.1.4.OilCellProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOilDecalProjection::RunTest(const FString&)
{
	ARMEMENT011714::FWorld F;
	auto* Runtime = ARMEMENT011714::MakeRuntime(F);
	if (!TestNotNull(TEXT("Acteur"), Runtime)) return false;
	TestTrue(TEXT("Huile cellule 1"), ARMEMENT011714::Apply(Runtime, 1, 1, EGridCombatSurfaceType::Oil));
	TestTrue(TEXT("Huile voisine"), ARMEMENT011714::Apply(Runtime, 2, 1, EGridCombatSurfaceType::Oil));
	TestTrue(TEXT("Eau non concernée"), ARMEMENT011714::Apply(Runtime, 0, 0, EGridCombatSurfaceType::Water));
	TestFalse(TEXT("Cellule hors grille"), ARMEMENT011714::Apply(Runtime, 6, 6, EGridCombatSurfaceType::Oil));
	TestEqual(TEXT("Deux cases adjacentes forment un seul décalque continu"),
		Runtime->GetActiveOilDecalCount(), 1);
	const UDecalComponent* Decal = Runtime->GetActiveOilDecal(0);
	if (!TestNotNull(TEXT("Décalque présent"), Decal)) return false;
	TestTrue(TEXT("Matière affectée"), Decal->GetDecalMaterial() == UMaterial::GetDefaultMaterial(MD_Surface));
	TestTrue(TEXT("Aucune translation en Z, sol à zéro"),
		FMath::IsNearlyEqual(Decal->GetRelativeLocation().Z, Runtime->GridOrigin.Z, 0.01f));
	TestTrue(TEXT("Une projection traverse les deux cellules"), Decal->DecalSize.Z > 350.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOilDecalAtZero,
	"Grimrock.RPG.ARMEMENT01.17.1.4.NoFloatingOverlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOilDecalAtZero::RunTest(const FString&)
{
	ARMEMENT011714::FWorld F;
	auto* Runtime = ARMEMENT011714::MakeRuntime(F);
	if (!TestNotNull(TEXT("Acteur"), Runtime)) return false;
	TestTrue(TEXT("Huile"), ARMEMENT011714::Apply(Runtime, 1, 2, EGridCombatSurfaceType::Oil));
	const UDecalComponent* D = Runtime->GetActiveOilDecal(0);
	if (!TestNotNull(TEXT("Décalque, pas plan"), D)) return false;
	TestEqual(TEXT("Origine Z exactement à zéro"), D->GetRelativeLocation().Z, Runtime->GridOrigin.Z);
	TestTrue(TEXT("Volume englobant le sol"), D->DecalSize.X > 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOilDecalReaction,
	"Grimrock.RPG.ARMEMENT01.17.1.4.ReactionsAndConversions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOilDecalReaction::RunTest(const FString&)
{
	ARMEMENT011714::FWorld F;
	auto* Runtime = ARMEMENT011714::MakeRuntime(F);
	if (!TestNotNull(TEXT("Acteur"), Runtime)) return false;
	TestTrue(TEXT("Huile A"), ARMEMENT011714::Apply(Runtime, 1, 1, EGridCombatSurfaceType::Oil, 3));
	TestTrue(TEXT("Huile B"), ARMEMENT011714::Apply(Runtime, 3, 3, EGridCombatSurfaceType::Oil, 3));
	TestEqual(TEXT("Deux décalques"), Runtime->GetActiveOilDecalCount(), 2);
	FGridCombatSurfaceReactionResult Reaction;
	TestTrue(TEXT("Réaction au feu"), Runtime->InteractCombatSurfaceAtCell(
		1, 1, EGridCombatSurfaceInteraction::Fire, FGridResolvedCombatModifiers(), Reaction));
	TestEqual(TEXT("Un décalque après feu"), Runtime->GetActiveOilDecalCount(), 1);
	FGridCombatSurfaceConversionProfile P;
	P.bAllowEmptyCell = true;
	P.OutputSurfaceType = EGridCombatSurfaceType::Oil;
	P.EmptyCellDurationRounds = 2;
	TestTrue(TEXT("Conversion vers l'huile"), Runtime->ConvertCombatSurfaceAtCell(
		0, 0, P, FGuid::NewGuid(), TEXT("ConversionOil"), FGridResolvedCombatModifiers()));
	TestEqual(TEXT("Deux décalques restaurés"), Runtime->GetActiveOilDecalCount(), 2);
	TestTrue(TEXT("Eau remplace l'huile"), ARMEMENT011714::Apply(Runtime, 0, 0, EGridCombatSurfaceType::Water));
	TestEqual(TEXT("Un décalque après eau"), Runtime->GetActiveOilDecalCount(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOilDecalExpiry,
	"Grimrock.RPG.ARMEMENT01.17.1.4.Expiration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOilDecalExpiry::RunTest(const FString&)
{
	ARMEMENT011714::FWorld F;
	auto* Runtime = ARMEMENT011714::MakeRuntime(F);
	if (!TestNotNull(TEXT("Acteur"), Runtime)) return false;
	TestTrue(TEXT("Huile 2 rounds"), ARMEMENT011714::Apply(Runtime, 2, 2, EGridCombatSurfaceType::Oil, 2));
	TestEqual(TEXT("Flaque présente"), Runtime->GetActiveOilDecalCount(), 1);
	TestEqual(TEXT("Premier round"), Runtime->AdvanceCombatSurfaceRound(), 0);
	TestEqual(TEXT("Flaque conservée"), Runtime->GetActiveOilDecalCount(), 1);
	TestEqual(TEXT("Second round"), Runtime->AdvanceCombatSurfaceRound(), 1);
	TestEqual(TEXT("Aucun décalque orphelin"), Runtime->GetActiveOilDecalCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOilDecalSave,
	"Grimrock.RPG.ARMEMENT01.17.1.4.SaveLoadRebuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOilDecalSave::RunTest(const FString&)
{
	ARMEMENT011714::FWorld F;
	auto* Runtime = ARMEMENT011714::MakeRuntime(F);
	if (!TestNotNull(TEXT("Acteur"), Runtime)) return false;
	TestTrue(TEXT("Huile A"), ARMEMENT011714::Apply(Runtime, 0, 0, EGridCombatSurfaceType::Oil, 4));
	TestTrue(TEXT("Huile B"), ARMEMENT011714::Apply(Runtime, 2, 2, EGridCombatSurfaceType::Oil, 2));
	TestTrue(TEXT("Poison"), ARMEMENT011714::Apply(Runtime, 1, 3, EGridCombatSurfaceType::Poison, 3));
	TestTrue(TEXT("Capture de l'état"), Runtime->CaptureCurrentLevelRuntimeState());
	auto* Save = NewObject<UGrimrockPartySaveGame>();
	Save->DungeonRuntimeState = Runtime->DungeonRuntimeState;
	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("Sérialisation"), UGameplayStatics::SaveGameToMemory(Save, Bytes))) return false;
	auto* Loaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("Désérialisation"), Loaded)) return false;
	Runtime->DungeonRuntimeState = FGridDungeonRuntimeState();
	Runtime->RefreshCombatOilSurfaceVisuals();
	TestEqual(TEXT("Nettoyage"), Runtime->GetActiveOilDecalCount(), 0);
	Runtime->DungeonRuntimeState = Loaded->DungeonRuntimeState;
	Runtime->RebuildLevel(EGridRuntimeRebuildMode::GeometryOnly);
	TestEqual(TEXT("Reconstruction des deux flaques"), Runtime->GetActiveOilDecalCount(), 2);
	TestTrue(TEXT("Restauration"), Runtime->ApplyCurrentLevelRuntimeState());
	TestEqual(TEXT("Deux flaques après restauration"), Runtime->GetActiveOilDecalCount(), 2);
	const FGridCombatSurfaceState* Oil = Runtime->FindCombatSurfaceAtCell(0, 0);
	TestTrue(TEXT("Durée conservée"), Oil && Oil->RemainingRounds == 4);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOilExplorationExpiry,
	"Grimrock.RPG.ARMEMENT01.17.1.6.ExplorationTime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOilExplorationExpiry::RunTest(const FString&)
{
	ARMEMENT011714::FWorld F;
	auto* Runtime = ARMEMENT011714::MakeRuntime(F);
	if (!TestNotNull(TEXT("Runtime"), Runtime)) return false;
	TestTrue(TEXT("Huile 2 rounds"), ARMEMENT011714::Apply(Runtime, 1, 1, EGridCombatSurfaceType::Oil, 2));
	TestTrue(TEXT("Eau 2 rounds"), ARMEMENT011714::Apply(Runtime, 2, 2, EGridCombatSurfaceType::Water, 2));
	const FGridCombatSurfaceState* Oil = Runtime->FindCombatSurfaceAtCell(1, 1);
	TestTrue(TEXT("12 secondes initiales"), Oil && FMath::IsNearlyEqual(Oil->RemainingOilSeconds, 12.f));
	TestEqual(TEXT("5 secondes d'exploration"), Runtime->AdvanceExplorationOilTime(5.f), 0);
	Oil = Runtime->FindCombatSurfaceAtCell(1, 1);
	TestTrue(TEXT("7 secondes restantes"), Oil && FMath::IsNearlyEqual(Oil->RemainingOilSeconds, 7.f));
	TestTrue(TEXT("Eau vieillit en secondes hors combat"),
		Runtime->FindCombatSurfaceAtCell(2, 2) &&
		Runtime->FindCombatSurfaceAtCell(2, 2)->RemainingRounds == 2 &&
		FMath::IsNearlyEqual(Runtime->FindCombatSurfaceAtCell(2, 2)->RemainingSurfaceSeconds, 7.f));
	TestEqual(TEXT("1 round supplémentaire"), Runtime->AdvanceCombatSurfaceRound(), 0);
	Oil = Runtime->FindCombatSurfaceAtCell(1, 1);
	TestTrue(TEXT("1 seconde restante"), Oil && Oil->RemainingRounds == 1 &&
		FMath::IsNearlyEqual(Oil->RemainingOilSeconds, 1.f));
	TestEqual(TEXT("Expiration synchronisée huile + eau"), Runtime->AdvanceExplorationOilTime(1.f), 2);
	TestNull(TEXT("Etat d'huile effacé"), Runtime->FindCombatSurfaceAtCell(1, 1));
	TestNull(TEXT("Eau temporaire expirée"), Runtime->FindCombatSurfaceAtCell(2, 2));
	TestEqual(TEXT("Décalque effacé"), Runtime->GetActiveOilDecalCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOilExplorationSave,
	"Grimrock.RPG.ARMEMENT01.17.1.6.SaveLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOilExplorationSave::RunTest(const FString&)
{
	ARMEMENT011714::FWorld F;
	auto* Runtime = ARMEMENT011714::MakeRuntime(F);
	if (!TestNotNull(TEXT("Runtime"), Runtime)) return false;
	TestTrue(TEXT("Huile 4 rounds"), ARMEMENT011714::Apply(Runtime, 0, 0, EGridCombatSurfaceType::Oil, 4));
	TestEqual(TEXT("Décompte partiel"), Runtime->AdvanceExplorationOilTime(1.5f), 0);
	TestTrue(TEXT("Capture"), Runtime->CaptureCurrentLevelRuntimeState());
	auto* Save = NewObject<UGrimrockPartySaveGame>();
	Save->DungeonRuntimeState = Runtime->DungeonRuntimeState;
	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("Sérialisation"), UGameplayStatics::SaveGameToMemory(Save, Bytes))) return false;
	auto* Loaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("Désérialisation"), Loaded)) return false;
	Runtime->DungeonRuntimeState = Loaded->DungeonRuntimeState;
	Runtime->RefreshCombatOilSurfaceVisuals();
	const FGridCombatSurfaceState* Oil = Runtime->FindCombatSurfaceAtCell(0, 0);
	TestTrue(TEXT("22.5 secondes conservées"),
		Oil && Oil->RemainingRounds == 4 && FMath::IsNearlyEqual(Oil->RemainingOilSeconds, 22.5f) &&
		FMath::IsNearlyEqual(Oil->RemainingSurfaceSeconds, 22.5f));
	TestEqual(TEXT("Expiration précise après rechargement"), Runtime->AdvanceExplorationOilTime(22.5f), 1);
	TestNull(TEXT("Pas d'huile résiduelle"), Runtime->FindCombatSurfaceAtCell(0, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOilExplorationLegacy,
	"Grimrock.RPG.ARMEMENT01.17.1.6.LegacySave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOilExplorationLegacy::RunTest(const FString&)
{
	ARMEMENT011714::FWorld F;
	auto* Runtime = ARMEMENT011714::MakeRuntime(F);
	if (!TestNotNull(TEXT("Runtime"), Runtime)) return false;
	TestTrue(TEXT("Huile ancienne"), ARMEMENT011714::Apply(Runtime, 1, 1, EGridCombatSurfaceType::Oil, 2));
	auto* State = Runtime->GetOrCreateRuntimeStateForCurrentLevel();
	if (!TestNotNull(TEXT("Etat runtime"), State)) return false;
	auto* OldOil = State->Surfaces.Find(FIntPoint(1, 1));
	if (!TestNotNull(TEXT("Surface"), OldOil)) return false;
	OldOil->RemainingOilSeconds = 0.f;
	OldOil->RemainingSurfaceSeconds = 0.f;
	TestEqual(TEXT("Ancien SaveGame migré"), Runtime->AdvanceExplorationOilTime(6.f), 0);
	const FGridCombatSurfaceState* Oil = Runtime->FindCombatSurfaceAtCell(1, 1);
	TestTrue(TEXT("6 secondes restantes"), Oil && Oil->RemainingRounds == 1 &&
		FMath::IsNearlyEqual(Oil->RemainingOilSeconds, 6.f));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOilSplashVisibleBurst,
	"Grimrock.RPG.ARMEMENT01.17.1.5C.VisibleBurst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOilSplashVisibleBurst::RunTest(const FString&)
{
	ARMEMENT011714::FWorld F;
	auto* Runtime = ARMEMENT011714::MakeRuntime(F);
	if (!TestNotNull(TEXT("Runtime"), Runtime)) return false;
	TestTrue(TEXT("Flaque persistante"), ARMEMENT011714::Apply(Runtime, 1, 1, EGridCombatSurfaceType::Oil, 4));
	TestEqual(TEXT("Pas de splash au repos"), Runtime->GetActiveOilSplashCount(), 0);

	Runtime->PlayCombatOilSplashAtCell(1, 1);
	TestEqual(TEXT("Un seul événement de présentation"), Runtime->GetOilSplashPresentationCount(), 1);
	TestEqual(TEXT("Impact central visible"), Runtime->GetActiveOilSplashCount(), 1);
	TestEqual(TEXT("Huit gouttes 3D"), Runtime->GetActiveOilSplashDropletCount(), 8);
	TestEqual(TEXT("Huit projections satellites"), Runtime->GetActiveOilSplashSpatterCount(), 8);
	const UStaticMeshComponent* FirstDrop = Runtime->GetOilSplashDroplet(0);
	if (!TestNotNull(TEXT("Goutte 3D"), FirstDrop)) return false;
	const FVector Initial = FirstDrop->GetRelativeLocation();
	for (int32 Index = 0; Index < 5; ++Index)
	{
		Runtime->UpdateCombatOilSplash();
	}
	FirstDrop = Runtime->GetOilSplashDroplet(0);
	TestTrue(TEXT("Goutte réellement animée dans l'espace"),
		FirstDrop && FVector::DistSquared(FirstDrop->GetRelativeLocation(), Initial) > 100.f);
	TestTrue(TEXT("La flaque RPG03.6 reste intacte"),
		Runtime->FindCombatSurfaceAtCell(1, 1) != nullptr &&
		Runtime->FindCombatSurfaceAtCell(1, 1)->RemainingRounds == 4);
	Runtime->ClearCombatOilSplash();
	TestEqual(TEXT("Gouttes nettoyées"), Runtime->GetActiveOilSplashDropletCount(), 0);
	TestEqual(TEXT("Projections nettoyées"), Runtime->GetActiveOilSplashSpatterCount(), 0);
	TestEqual(TEXT("Flaque durable conservée"), Runtime->GetActiveOilDecalCount(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOilSplashLifetime,
	"Grimrock.RPG.ARMEMENT01.17.1.5C.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOilSplashLifetime::RunTest(const FString&)
{
	ARMEMENT011714::FWorld F;
	auto* Runtime = ARMEMENT011714::MakeRuntime(F);
	if (!TestNotNull(TEXT("Runtime"), Runtime)) return false;
	TestTrue(TEXT("Flaque initiale"), ARMEMENT011714::Apply(Runtime, 1, 1, EGridCombatSurfaceType::Oil));
	Runtime->PlayCombatOilSplashAtCell(1, 1);
	TestEqual(TEXT("Premier impact"), Runtime->GetOilSplashPresentationCount(), 1);
	Runtime->PlayCombatOilSplashAtCell(1, 1);
	TestEqual(TEXT("Second impact sans accumulation"), Runtime->GetOilSplashPresentationCount(), 2);
	TestEqual(TEXT("Huit gouttes et pas seize"), Runtime->GetActiveOilSplashDropletCount(), 8);
	TestEqual(TEXT("Huit projections et pas seize"), Runtime->GetActiveOilSplashSpatterCount(), 8);
	for (int32 Index = 0; Index < 45; ++Index)
	{
		Runtime->UpdateCombatOilSplash();
	}
	TestEqual(TEXT("Impact central terminé"), Runtime->GetActiveOilSplashCount(), 0);
	TestEqual(TEXT("Toutes les gouttes retirées"), Runtime->GetActiveOilSplashDropletCount(), 0);
	TestEqual(TEXT("Toutes les satellites retirées"), Runtime->GetActiveOilSplashSpatterCount(), 0);
	TestNotNull(TEXT("Huile persistante toujours autoritaire"), Runtime->FindCombatSurfaceAtCell(1, 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCatalystVfxClassification,
	"Grimrock.RPG.ARMEMENT01.17.2C.CanonicalVfxClassification",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCatalystVfxClassification::RunTest(const FString&)
{
	using K=EGridSurfaceCatalystVfxKind;
	FGridCombatSurfaceReactionResult R;
	TestTrue(TEXT("Invalid results fizzle"), AGridLevelRuntimeActor::ResolveCatalystVfxKind(EGridCombatSurfaceType::Water,R)==K::Fizzle);
	R.bReacted=true; R.OutputSurfaceType=EGridCombatSurfaceType::Fire;
	TestTrue(TEXT("Oil is flame"),AGridLevelRuntimeActor::ResolveCatalystVfxKind(EGridCombatSurfaceType::Oil,R)==K::OilIgnition);
	R.bExplosive=true;
	TestTrue(TEXT("Poison is burst"),AGridLevelRuntimeActor::ResolveCatalystVfxKind(EGridCombatSurfaceType::Poison,R)==K::PoisonBurst);
	TestTrue(TEXT("PoisonCloud is burst"),AGridLevelRuntimeActor::ResolveCatalystVfxKind(EGridCombatSurfaceType::PoisonCloud,R)==K::PoisonBurst);
	R.bExplosive=false; R.OutputSurfaceType=EGridCombatSurfaceType::Water;
	TestTrue(TEXT("Ice is melt"),AGridLevelRuntimeActor::ResolveCatalystVfxKind(EGridCombatSurfaceType::Ice,R)==K::IceMelt);
	TestTrue(TEXT("No invented Water ignition"),AGridLevelRuntimeActor::ResolveCatalystVfxKind(EGridCombatSurfaceType::Water,R)==K::Fizzle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCatalystVfxLifecycle,
	"Grimrock.RPG.ARMEMENT01.17.2C.TransientReactionLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCatalystVfxLifecycle::RunTest(const FString&)
{
	ARMEMENT011714::FWorld F;
	auto* Runtime=ARMEMENT011714::MakeRuntime(F);
	if(!TestNotNull(TEXT("Runtime"),Runtime)) return false;
	if(!TestTrue(TEXT("Oil"),ARMEMENT011714::Apply(Runtime,1,1,EGridCombatSurfaceType::Oil,2))) return false;
	FGridCombatSurfaceReactionResult R;
	if(!TestTrue(TEXT("Canonical Fire"),Runtime->InteractCombatSurfaceAtCell(1,1,
		EGridCombatSurfaceInteraction::Fire,FGridResolvedCombatModifiers(),R))) return false;
	TestEqual(TEXT("Oil decal removed"),Runtime->GetActiveOilDecalCount(),0);
	Runtime->PlaySurfaceCatalystReactionAtCell(1,1,EGridCombatSurfaceType::Oil,R);
	TestEqual(TEXT("One presentation"),Runtime->GetCatalystPresentationCount(),1);
	TestTrue(TEXT("Correct VFX"),Runtime->GetCatalystVisualKind()==EGridSurfaceCatalystVfxKind::OilIgnition);
	for(int32 I=0;I<40;++I) Runtime->UpdateCatalystReactionVfx();
	TestEqual(TEXT("Transient particles cleaned"),Runtime->GetActiveCatalystSparkCount(),0);
	TestNotNull(TEXT("Fire still in canonical state"),Runtime->FindCombatSurfaceAtCell(1,1));
	TestEqual(TEXT("One round remains"),Runtime->AdvanceCombatSurfaceRound(),0);
	TestEqual(TEXT("Fire expires"),Runtime->AdvanceCombatSurfaceRound(),1);
	TestEqual(TEXT("Fire VFX cleaned on expiry"),Runtime->GetActiveFireNiagaraCount(),0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCatalystWetSurface,
	"Grimrock.RPG.ARMEMENT01.17.2C.IceMeltWaterProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCatalystWetSurface::RunTest(const FString&)
{
	ARMEMENT011714::FWorld F;
	auto* Runtime=ARMEMENT011714::MakeRuntime(F);
	if(!TestNotNull(TEXT("Runtime"),Runtime)) return false;
	if(!TestTrue(TEXT("Ice is created"),ARMEMENT011714::Apply(Runtime,1,1,EGridCombatSurfaceType::Ice,2))) return false;
	FGridCombatSurfaceReactionResult R;
	if(!TestTrue(TEXT("Ice reacts with Fire"),Runtime->InteractCombatSurfaceAtCell(
		1,1,EGridCombatSurfaceInteraction::Fire,FGridResolvedCombatModifiers(),R))) return false;
	TestTrue(TEXT("Water remains canonical"),Runtime->FindCombatSurfaceAtCell(1,1)->SurfaceType==EGridCombatSurfaceType::Water);
	TestTrue(TEXT("Ice melt variation"),AGridLevelRuntimeActor::ResolveCatalystVfxKind(
		EGridCombatSurfaceType::Ice,R)==EGridSurfaceCatalystVfxKind::IceMelt);
	TestTrue(TEXT("Wet floor projected onto real stone"),Runtime->GetActiveWaterSurfaceDecalCount()>0);
	Runtime->AdvanceCombatSurfaceRound();
	Runtime->AdvanceCombatSurfaceRound();
	TestEqual(TEXT("Wet decal removed at surface expiration"),Runtime->GetActiveWaterSurfaceDecalCount(),0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCatalystAreaVfxWave,
	"Grimrock.RPG.ARMEMENT01.17.3C.AreaFlightImpactAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCatalystAreaVfxWave::RunTest(const FString&)
{
	ARMEMENT011714::FWorld Fixture;
	AGridLevelRuntimeActor* Runtime = ARMEMENT011714::MakeRuntime(Fixture);
	if (!TestNotNull(TEXT("Runtime ready"), Runtime)) return false;
	if (!TestTrue(TEXT("Real oil surface"), ARMEMENT011714::Apply(
		Runtime, 1, 1, EGridCombatSurfaceType::Oil, 3))) return false;
	const int32 OilDecalBefore = Runtime->GetActiveOilDecalCount();
	const int32 FirstImpactCount = Runtime->GetCatalystImpactCount();

	TArray<FGridCatalystCellOutcome> Cells;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		FGridCatalystCellOutcome& Cell = Cells.AddDefaulted_GetRef();
		Cell.Cell = Index == 0 ? FIntPoint(1, 1) :
			(Index == 1 ? FIntPoint(2, 1) :
				(Index == 2 ? FIntPoint(1, 2) : FIntPoint(0, 2)));
		Cell.InputType = Index == 0 ? EGridCombatSurfaceType::Oil :
			(Index == 1 ? EGridCombatSurfaceType::Poison :
				(Index == 2 ? EGridCombatSurfaceType::Ice : EGridCombatSurfaceType::None));
		Cell.Reaction.bReacted = Index < 3;
		Cell.Reaction.bExplosive = Index == 1;
		Cell.Reaction.OutputSurfaceType = Index == 2 ?
			EGridCombatSurfaceType::Water : EGridCombatSurfaceType::Fire;
	}
	Runtime->BeginCatalystSurfaceVisualBatch();
	FGridCombatSurfaceReactionResult Applied;
	if (!TestTrue(TEXT("Logical Oil -> Fire accepted"), Runtime->InteractCombatSurfaceAtCell(
			1, 1, EGridCombatSurfaceInteraction::Fire,
			FGridResolvedCombatModifiers(), Applied)))
	{
		Runtime->CancelCatalystSurfaceVisualBatch();
		return false;
	}
	TestEqual(TEXT("Old oil decal is held until impact"), Runtime->GetActiveOilDecalCount(), OilDecalBefore);
	TestTrue(TEXT("Gameplay already committed once"),
		Runtime->FindCombatSurfaceAtCell(1, 1) &&
		Runtime->FindCombatSurfaceAtCell(1, 1)->SurfaceType == EGridCombatSurfaceType::Fire);
	Runtime->PlaySurfaceCatalystArea(FIntPoint(0, 1), FIntPoint(1, 1), Cells);
	TestEqual(TEXT("One bottle presentation for four distinct cells"),
		Runtime->GetCatalystPresentationCount(), 1);
	if (!TestEqual(TEXT("Artist-authored bottle visible in flight"),
		Runtime->GetActiveCatalystProjectileCount(), 1)) return false;
	TestTrue(TEXT("Fiery trail accompanies the bottle"),
		Runtime->GetActiveCatalystTrailCount() > 0);
	TestEqual(TEXT("No impact before arc is finished"),
		Runtime->GetCatalystImpactCount(), FirstImpactCount);
	TestEqual(TEXT("No local pulses before impact"), Runtime->GetActiveCatalystCellPulseCount(), 0);
	for (int32 I = 0; I < 14; ++I) Runtime->UpdateCatalystReactionVfx();
	TestEqual(TEXT("Exactly one impact"), Runtime->GetCatalystImpactCount(), FirstImpactCount + 1);
	TestEqual(TEXT("Projectile disappears at the floor"), Runtime->GetActiveCatalystProjectileCount(), 0);
	TestEqual(TEXT("Trail is removed at impact"), Runtime->GetActiveCatalystTrailCount(), 0);
	TestEqual(TEXT("Four independent cell pulses include fizzle"), Runtime->GetActiveCatalystCellPulseCount(), 4);
	TestTrue(TEXT("Local sparks are spawned"), Runtime->GetActiveCatalystSparkCount() > 10);
	TestEqual(TEXT("Oil decal replaced after impact"), Runtime->GetActiveOilDecalCount(), 0);
	TestTrue(TEXT("Fire C6 state is not changed by visual wave"),
		Runtime->FindCombatSurfaceAtCell(1, 1) &&
		Runtime->FindCombatSurfaceAtCell(1, 1)->SurfaceType == EGridCombatSurfaceType::Fire);
	for (int32 I = 0; I < 45; ++I) Runtime->UpdateCatalystReactionVfx();
	TestEqual(TEXT("All transient local pulse decals cleaned"), Runtime->GetActiveCatalystCellPulseCount(), 0);
	TestEqual(TEXT("No temporary spark leaked"), Runtime->GetActiveCatalystSparkCount(), 0);
	TestEqual(TEXT("No bottle mesh leaked"), Runtime->GetActiveCatalystProjectileCount(), 0);
	TestNotNull(TEXT("Permanent Fire survives transient effect"), Runtime->FindCombatSurfaceAtCell(1, 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCatalystAreaVfxCancel,
	"Grimrock.RPG.ARMEMENT01.17.3C.CancelPendingVisualBatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCatalystAreaVfxCancel::RunTest(const FString&)
{
	ARMEMENT011714::FWorld Fixture;
	AGridLevelRuntimeActor* Runtime = ARMEMENT011714::MakeRuntime(Fixture);
	if (!TestNotNull(TEXT("Runtime ready"), Runtime)) return false;
	if (!TestTrue(TEXT("Oil"), ARMEMENT011714::Apply(
			Runtime, 1, 1, EGridCombatSurfaceType::Oil, 3))) return false;
	Runtime->BeginCatalystSurfaceVisualBatch();
	FGridCombatSurfaceReactionResult Reaction;
	if (!TestTrue(TEXT("Reaction committed"), Runtime->InteractCombatSurfaceAtCell(
			1, 1, EGridCombatSurfaceInteraction::Fire, FGridResolvedCombatModifiers(), Reaction)))
		return false;
	TestEqual(TEXT("Deferred old oil visual"), Runtime->GetActiveOilDecalCount(), 1);
	Runtime->CancelCatalystSurfaceVisualBatch();
	TestEqual(TEXT("Cancel restores current surface rendering"), Runtime->GetActiveOilDecalCount(), 0);
	TestNotNull(TEXT("Cancel never undoes authoritative gameplay"), Runtime->FindCombatSurfaceAtCell(1, 1));
	TestEqual(TEXT("No abandoned projectile"), Runtime->GetActiveCatalystProjectileCount(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

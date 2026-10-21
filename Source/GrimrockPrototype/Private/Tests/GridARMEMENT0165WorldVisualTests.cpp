#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/Combat/GridWorldItemVisualResolver.h"
#include "Runtime/GridEditorPreviewComponent.h"
#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace ARMEMENT0165
{
	const TCHAR* CataloguePath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue");

	struct FWorldTest
	{
		UWorld* World = nullptr;

		FWorldTest()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
				.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("ARM0165_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);

			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FWorldTest()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
			}
		}
	};

	UGridItemDefinitionAsset* MakeDefinition(UObject* Outer, UGridWeaponVisualCatalogueAsset* Catalogue,
		FName ItemId, FName ArchetypeId)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Outer);
		Definition->ItemDefinitionId = ItemId;
		Definition->ItemType = ArchetypeId == FName(TEXT("ARM075")) ? EGridItemType::Shield : EGridItemType::Weapon;
		Definition->WeaponArchetypeId = ArchetypeId;
		Definition->bUseWeaponVisualCatalogue = true;
		Definition->WeaponVisualCatalogue = Catalogue;
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0165RuntimeMeshTest,
	"Grimrock.RPG.ARMEMENT01.6.World.RuntimeMeshAndLegacy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0165RuntimeMeshTest::RunTest(const FString&)
{
	using namespace ARMEMENT0165;
	FWorldTest Fixture;
	if (!TestNotNull(TEXT("Transient world exists"), Fixture.World)) { return false; }
	UGridWeaponVisualCatalogueAsset* Catalogue = LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, CataloguePath);
	if (!TestNotNull(TEXT("Authoritative visual catalogue exists"), Catalogue)) { return false; }

	struct FPilot { const TCHAR* Archetype; const TCHAR* VisualId; };
	const FPilot Pilots[] =
	{
		{TEXT("ARM001"), TEXT("VIS_Dagger_01")},
		{TEXT("ARM045"), TEXT("VIS_ShortBow_01")},
		{TEXT("ARM075"), TEXT("VIS_RoundShield_01")}
	};

	for (const FPilot& Pilot : Pilots)
	{
		const FName Id(*FString::Printf(TEXT("ARM0165_Runtime_%s"), Pilot.Archetype));
		UGridItemDefinitionAsset* Definition = MakeDefinition(Fixture.World, Catalogue, Id, FName(Pilot.Archetype));
		const FGridWeaponVisualProfile* Profile = Catalogue->FindProfileForArchetype(FName(Pilot.Archetype));
		if (!TestNotNull(TEXT("Valid approved profile"), Profile)) { continue; }
		UStaticMesh* ExpectedMesh = Profile->WorldMesh.LoadSynchronous();
		if (!TestNotNull(TEXT("Approved realistic mesh loads"), ExpectedMesh)) { continue; }

		AGridItemActor* Actor = Fixture.World->SpawnActor<AGridItemActor>();
		if (!TestNotNull(TEXT("Item actor spawned"), Actor)) { continue; }
		const FGuid OriginalId = FGuid::NewGuid();
		Actor->InitializeFromItemDefinition(Definition, OriginalId);

		TestEqual(TEXT("Runtime mesh is from the visual catalogue"), Actor->MeshComponent->GetStaticMesh().Get(), ExpectedMesh);
		TestTrue(TEXT("Authored mesh-local transform matches the visual catalogue"),
			Actor->MeshComponent->GetRelativeTransform().Equals(Profile->WorldVisualTransform));
		TestEqual(TEXT("Runtime item identity remains unchanged"), Actor->GetRuntimeObjectId(), OriginalId);
		TestEqual(TEXT("Actor collision configuration is not rewritten by visual resolution"),
			Actor->MeshComponent->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		TestEqual(TEXT("StaticMesh retains ownership of its first material"),
			Actor->MeshComponent->GetMaterial(0), ExpectedMesh->GetMaterial(0));

		const FGridResolvedWorldItemMesh ResolvedAgain = FGridWorldItemVisualResolver::Resolve(*Definition);
		TestEqual(TEXT("World resolver matches runtime mesh"), ResolvedAgain.Mesh, ExpectedMesh);
		TestEqual(TEXT("World resolver uses numeric VisualId"), ResolvedAgain.VisualId, FName(Pilot.VisualId));
	}

	UGridItemDefinitionAsset* Legacy = NewObject<UGridItemDefinitionAsset>(Fixture.World);
	Legacy->ItemDefinitionId = TEXT("ARM0165_Legacy");
	UStaticMesh* LegacyMesh = NewObject<UStaticMesh>(Fixture.World);
	Legacy->WorldMesh = LegacyMesh;
	AGridItemActor* LegacyActor = Fixture.World->SpawnActor<AGridItemActor>();
	if (TestNotNull(TEXT("Legacy actor exists"), LegacyActor))
	{
		LegacyActor->InitializeFromItemDefinition(Legacy, FGuid::NewGuid());
		TestEqual(TEXT("Legacy mesh remains unchanged"), LegacyActor->MeshComponent->GetStaticMesh().Get(), LegacyMesh);
		TestTrue(TEXT("Legacy mesh keeps its identity transform"),
			LegacyActor->MeshComponent->GetRelativeTransform().Equals(FTransform::Identity));
	}

	Legacy->bUseWeaponVisualCatalogue = true;
	Legacy->WeaponArchetypeId = TEXT("ARM082"); // not mapped in the approved pilot
	Legacy->WeaponVisualCatalogue = Catalogue;
	const FGridResolvedWorldItemMesh MissingBinding = FGridWorldItemVisualResolver::Resolve(*Legacy);
	TestEqual(TEXT("Missing visual binding falls back to same item's Legacy mesh"), MissingBinding.Mesh, LegacyMesh);
	TestTrue(TEXT("Fallback does not apply another weapon's visual transform"),
		MissingBinding.RelativeTransform.Equals(FTransform::Identity));
	TestTrue(TEXT("Missing binding does not invent another VisualId"), MissingBinding.VisualId.IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0165EditorPreviewTest,
	"Grimrock.RPG.ARMEMENT01.6.World.EditorPreviewSameMesh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0165EditorPreviewTest::RunTest(const FString&)
{
	using namespace ARMEMENT0165;
	FWorldTest Fixture;
	if (!TestNotNull(TEXT("Transient editor preview test world exists"), Fixture.World)) { return false; }
	UGridWeaponVisualCatalogueAsset* Catalogue = LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, CataloguePath);
	if (!TestNotNull(TEXT("Visual catalogue exists"), Catalogue)) { return false; }
	AGridLevelRuntimeActor* Runtime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Runtime actor exists"), Runtime)) { return false; }
	UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
	Level->Width = 4;
	Level->Height = 4;
	Level->EnsureCellCount();
	for (FGridLevelCellData& Cell : Level->Cells)
	{
		Cell.CellType = EGridCellType::Floor;
	}
	Runtime->LevelAsset = Level;

	const TCHAR* Archetypes[] = {TEXT("ARM001"), TEXT("ARM045"), TEXT("ARM075")};
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FName Id(*FString::Printf(TEXT("ARM0165_Preview_%s"), Archetypes[Index]));
		UGridItemDefinitionAsset* Definition = MakeDefinition(Runtime, Catalogue, Id, FName(Archetypes[Index]));
		FGridLooseItemInstance Placed;
		Placed.InstanceId = FGuid::NewGuid();
		Placed.ItemDefinition = Definition;
		Placed.CellX = Index;
		Placed.CellY = 1;
		Level->LooseItemInstances.Add(Placed);
	}

	UGridEditorPreviewComponent* PreviewComponent = Runtime->FindComponentByClass<UGridEditorPreviewComponent>();
	if (!TestNotNull(TEXT("Existing preview component"), PreviewComponent)) { return false; }
	PreviewComponent->Initialize(Runtime);
	PreviewComponent->RebuildPreviewObjects();

	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FGridLooseItemInstance& Placed = Level->LooseItemInstances[Index];
		const FGridWeaponVisualProfile* Profile =
			Catalogue->FindProfileForArchetype(FName(Archetypes[Index]));
		if (!TestNotNull(TEXT("Matching profile"), Profile)) { continue; }

		AGridEditorPreviewObjectActor* Preview = nullptr;
		for (TActorIterator<AGridEditorPreviewObjectActor> It(Fixture.World); It; ++It)
		{
			if ((*It)->ObjectId == Placed.InstanceId)
			{
				Preview = *It;
				break;
			}
		}
		if (!TestNotNull(TEXT("Loose item preview was created"), Preview)) { continue; }
		TestEqual(TEXT("Editor preview uses exactly the runtime WorldMesh"), Preview->MeshComponent->GetStaticMesh().Get(),
			Profile->WorldMesh.LoadSynchronous());
		TestTrue(TEXT("Editor and runtime use the identical local visual transform"),
			Preview->MeshComponent->GetRelativeTransform().Equals(Profile->WorldVisualTransform));
		TestEqual(TEXT("Preview has no collision"), Preview->MeshComponent->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		TestEqual(TEXT("Preview retains typed placement identity"), Preview->ObjectId, Placed.InstanceId);
	}
	PreviewComponent->ClearPreviewObjects();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

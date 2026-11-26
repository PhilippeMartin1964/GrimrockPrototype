#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemVisualEffectsComponent.h"
#include "Runtime/Inspection/GridItemInspectionReadModel.h"
#include "Runtime/Inspection/GridItemPreviewActor.h"
#include "Tests/GridCanonicalRegistryFixture.h"

namespace
{
	struct FInspectionTestWorld
	{
		UWorld* World = nullptr;

		explicit FInspectionTestWorld(EWorldType::Type Type = EWorldType::EditorPreview)
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
				.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
			World = UWorld::CreateWorld(Type, false,
				FName(*FString::Printf(TEXT("Inspection_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				GEngine->CreateNewWorldContext(Type).SetCurrentWorld(World);
			}
		}

		~FInspectionTestWorld()
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

	FGridItemInstance MakePhysicalItem(FName ItemId, int32 Level)
	{
		FGridItemInstance Item;
		Item.ItemDefinitionId = ItemId;
		Item.ItemLevel = Level;
		Item.RuntimeObjectId = FGuid::NewGuid();
		return Item;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemInspectionResolveTest, "Grimrock.Items.Inspection.Resolve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemInspectionResolveTest::RunTest(const FString&)
{
	const FGridCanonicalRegistryFixture Fixture;
	if (!TestTrue(TEXT("Production registry is configured"), Fixture.bReady))
	{
		return false;
	}
	TArray<FName> Ids;
	Fixture.Registry->GetItemIds(Ids);
	TestEqual(TEXT("All materialized definitions are enumerable"), Ids.Num(), 48);
	int32 Armaments = 0;
	int32 ArmamentLevels = 0;
	int32 NonArmaments = 0;
	for (FName Id : Ids)
	{
		FGridItemInspectionView View;
		if (!TestTrue(*FString::Printf(TEXT("Default selection resolves %s"), *Id.ToString()),
				FGridItemInspectionReadModel::ResolveDefault(Fixture.Registry.Get(), Id, View)))
		{
			continue;
		}
		TestFalse(TEXT("Catalogue selection is never a physical instance"), View.bPhysicalInstance);
		TestFalse(TEXT("Catalogue selection never exposes a remaining reserve"), View.bHasPhysicalReserve);
		if (View.HasLevel())
		{
			++Armaments;
			TestEqual(TEXT("ARM default is N1"), View.Item.ItemLevel, 1);
			for (int32 N = 1; N <= 20; ++N)
			{
				FGridResolvedItem Canonical;
				if (TestTrue(TEXT("Exact level resolves"), FGridItemInspectionReadModel::ResolveExact(Fixture.Registry.Get(), Id, N, View)) &&
					TestTrue(TEXT("Canonical lookup agrees"), Fixture.Resolve(Id, N, Canonical)))
				{
					++ArmamentLevels;
					TestEqual(TEXT("Exact identity"), View.Item.ItemId, Id);
					TestEqual(TEXT("Exact level"), View.Item.ItemLevel, N);
					TestTrue(TEXT("Mesh and finish remain canonical"), View.Item.WorldMesh == Canonical.WorldMesh &&
						View.Item.WeaponFinish.FinishId == Canonical.WeaponFinish.FinishId);
				}
			}
			TestFalse(TEXT("ARM N0 is refused"), FGridItemInspectionReadModel::ResolveExact(Fixture.Registry.Get(), Id, 0, View));
			TestFalse(TEXT("ARM N21 is refused"), FGridItemInspectionReadModel::ResolveExact(Fixture.Registry.Get(), Id, 21, View));
		}
		else
		{
			++NonArmaments;
			TestEqual(TEXT("Non-ARM remains N0"), View.Item.ItemLevel, 0);
			TestFalse(TEXT("Non-ARM N1 is refused"), FGridItemInspectionReadModel::ResolveExact(Fixture.Registry.Get(), Id, 1, View));
		}
		TestFalse(TEXT("Invalid selection does not keep a previous image"), View.HasMeshReference());
	}
	TestEqual(TEXT("Twenty-two materialized ARM"), Armaments, 22);
	TestEqual(TEXT("440 exact ARM resolutions"), ArmamentLevels, 440);
	TestEqual(TEXT("26 non-ARM definitions"), NonArmaments, 26);

	FGridItemInspectionView Unknown;
	TestFalse(TEXT("Unknown identity is refused"), FGridItemInspectionReadModel::ResolveDefault(Fixture.Registry.Get(), TEXT("Inspection_Unknown"), Unknown));
	TestTrue(TEXT("Unknown identity is diagnosed"), Unknown.Item.Error == EGridItemResolveError::UnknownItemId);
	TestFalse(TEXT("Unconfigured registry is refused"), FGridItemInspectionReadModel::ResolveDefault(nullptr, TEXT("Stone"), Unknown));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemInspectionCapabilitiesTest, "Grimrock.Items.Inspection.Capabilities",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemInspectionCapabilitiesTest::RunTest(const FString&)
{
	const FGridCanonicalRegistryFixture Fixture;
	if (!TestTrue(TEXT("Production registry is configured"), Fixture.bReady))
	{
		return false;
	}
	FGridItemInspectionView View;
	if (TestTrue(TEXT("Sword resolves"), FGridItemInspectionReadModel::ResolveDefault(Fixture.Registry.Get(), TEXT("Weapon_StraightSword_01"), View)))
	{
		TestTrue(TEXT("Sword shows level, equipment and attack data"), View.HasLevel() && View.HasEquipment() && View.HasAttackStats() && View.HasActionBindings());
	}
	FGridItemInstance Shield = MakePhysicalItem(TEXT("Weapon_RoundShield_01"), 1);
	Shield.AbsorbedEquipmentPhysicalArmor = 3;
	const FGuid OriginalGuid = Shield.RuntimeObjectId;
	if (TestTrue(TEXT("Shield resolves at its physical level"), FGridItemInspectionReadModel::ResolvePhysical(Fixture.Registry.Get(), Shield, View)))
	{
		TestTrue(TEXT("Shield is passive, with no fake attack"), View.HasLevel() && View.HasProtection() && !View.HasAttackStats());
		TestTrue(TEXT("Remaining reserve belongs to this instance"), View.bHasPhysicalReserve);
		TestEqual(TEXT("Spent reserve retained"), View.PhysicalReserve.SpentPhysicalArmor, 3);
		TestEqual(TEXT("Remaining reserve matches the actual nominal maximum"), View.PhysicalReserve.RemainingPhysicalArmor,
			FMath::Max(0, View.GetNominalPhysicalArmor() - 3));
	}
	TestEqual(TEXT("Physical GUID unchanged"), Shield.RuntimeObjectId, OriginalGuid);
	TestEqual(TEXT("Physical level unchanged"), Shield.ItemLevel, 1);
	TestEqual(TEXT("Spent armor unchanged"), Shield.AbsorbedEquipmentPhysicalArmor, 3);
	TestEqual(TEXT("Quantity unchanged"), Shield.Quantity, 1);
	TestTrue(TEXT("Simulated shield resolves independently"), FGridItemInspectionReadModel::ResolveExact(Fixture.Registry.Get(), Shield.ItemDefinitionId, 20, View));
	TestFalse(TEXT("Simulation never shows actual remaining reserve"), View.bHasPhysicalReserve);

	if (TestTrue(TEXT("Stone resolves at N0"), FGridItemInspectionReadModel::ResolveDefault(Fixture.Registry.Get(), TEXT("Stone"), View)))
	{
		TestFalse(TEXT("Stone has no level control"), View.HasLevel());
		TestFalse(TEXT("Stone has no ARM attack data"), View.HasAttackStats());
	}
	// The existing instance-readable contract also applies to examination.
	FGridItemInstance Readable = MakePhysicalItem(TEXT("Stone"), 0);
	Readable.ReadTextOverride = FText::FromString(TEXT("Inspection test only"));
	if (TestTrue(TEXT("Instance with readable override resolves"), FGridItemInspectionReadModel::ResolvePhysical(Fixture.Registry.Get(), Readable, View)))
	{
		TestTrue(TEXT("Readable override is respected"), View.bCanRead);
	}
	Readable.RuntimeObjectId.Invalidate();
	TestFalse(TEXT("Invalid physical identity is rejected"), FGridItemInspectionReadModel::ResolvePhysical(Fixture.Registry.Get(), Readable, View));
	TestTrue(TEXT("Invalid physical instance is diagnosed"), View.bInvalidPhysicalInstance);
	TestFalse(TEXT("Failed read does not keep previous reading capability"), View.bCanRead);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemInspectionPresentationTest, "Grimrock.Items.Inspection.PresentationParity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemInspectionPresentationTest::RunTest(const FString&)
{
	const FGridCanonicalRegistryFixture Fixture;
	FInspectionTestWorld Host;
	if (!TestTrue(TEXT("Production registry is configured"), Fixture.bReady) || !TestNotNull(TEXT("Isolated preview world"), Host.World))
	{
		return false;
	}
	AGridItemPreviewActor* First = Host.World->SpawnActor<AGridItemPreviewActor>();
	AGridItemPreviewActor* Second = Host.World->SpawnActor<AGridItemPreviewActor>();
	AGridItemActor* Runtime = NewObject<AGridItemActor>();
	if (!TestNotNull(TEXT("First preview"), First) || !TestNotNull(TEXT("Second preview"), Second) || !TestNotNull(TEXT("Runtime projection"), Runtime))
	{
		return false;
	}
	const FName Sword(TEXT("Weapon_StraightSword_01"));
	FGridItemInspectionView View;
	if (!TestTrue(TEXT("N17 inspection"), FGridItemInspectionReadModel::ResolveExact(Fixture.Registry.Get(), Sword, 17, View)) ||
		!TestTrue(TEXT("N17 preview ready"), First->ApplyPreview(View) == EGridItemPreviewResult::Ready) ||
		!TestTrue(TEXT("Same runtime N17"), Runtime->InitializeFromItemInstance(MakePhysicalItem(Sword, 17), Fixture.Registry.Get())))
	{
		return false;
	}
	TestEqual(TEXT("Preview uses the exact runtime mesh"), First->GetPreviewMesh()->GetStaticMesh(), Runtime->MeshComponent->GetStaticMesh());
	TestTrue(TEXT("Authored transform preserved"), First->GetPreviewMesh()->GetRelativeTransform().Equals(Runtime->MeshComponent->GetRelativeTransform()));
	TestTrue(TEXT("Preview class is transient"), First->GetClass()->HasAnyClassFlags(CLASS_Transient));
	TestFalse(TEXT("Preview cannot participate in collision"), First->GetActorEnableCollision());
	TestFalse(TEXT("Preview is not an interactable pickup"), First->GetClass()->ImplementsInterface(UGridInteractableInterface::StaticClass()));
	if (!TestTrue(TEXT("N5 inspection"), FGridItemInspectionReadModel::ResolveExact(Fixture.Registry.Get(), Sword, 5, View)) ||
		!TestTrue(TEXT("Second preview ready"), Second->ApplyPreview(View) == EGridItemPreviewResult::Ready))
	{
		return false;
	}
	auto* FirstMID = Cast<UMaterialInstanceDynamic>(First->GetPreviewMesh()->GetMaterial(0));
	auto* SecondMID = Cast<UMaterialInstanceDynamic>(Second->GetPreviewMesh()->GetMaterial(0));
	auto* RuntimeMID = Cast<UMaterialInstanceDynamic>(Runtime->MeshComponent->GetMaterial(0));
	if (!TestNotNull(TEXT("First MID"), FirstMID) || !TestNotNull(TEXT("Second MID"), SecondMID) || !TestNotNull(TEXT("Runtime MID"), RuntimeMID))
	{
		return false;
	}
	const FName Tint(TEXT("BaseColorTint"));
	const FLinearColor RuntimeTint = RuntimeMID->K2_GetVectorParameterValue(Tint);
	const FLinearColor SecondTint = SecondMID->K2_GetVectorParameterValue(Tint);
	TestEqual(TEXT("Preview finish matches runtime"), FirstMID->K2_GetVectorParameterValue(Tint), RuntimeTint);
	TestTrue(TEXT("All MID instances are independent"), FirstMID != SecondMID && FirstMID != RuntimeMID && SecondMID != RuntimeMID);
	FirstMID->SetVectorParameterValue(Tint, FLinearColor::Blue);
	TestEqual(TEXT("Editing preview leaves runtime unchanged"), RuntimeMID->K2_GetVectorParameterValue(Tint), RuntimeTint);
	TestEqual(TEXT("Editing preview leaves A/B neighbor unchanged"), SecondMID->K2_GetVectorParameterValue(Tint), SecondTint);
	if (TestTrue(TEXT("N1 inspection"), FGridItemInspectionReadModel::ResolveExact(Fixture.Registry.Get(), Sword, 1, View)) &&
		TestTrue(TEXT("N1 preview ready"), First->ApplyPreview(View) == EGridItemPreviewResult::Ready))
	{
		TestFalse(TEXT("N1 restores the base material"), First->GetPreviewMesh()->GetMaterial(0)->IsA<UMaterialInstanceDynamic>());
	}
	View.Item.WorldMesh.Reset();
	TestTrue(TEXT("Missing mesh gives explicit fallback"), First->ApplyPreview(View) == EGridItemPreviewResult::MissingMesh);
	TestNull(TEXT("Missing mesh clears the prior image"), First->GetPreviewMesh()->GetStaticMesh());
	TestTrue(TEXT("Invalid selection clears safely"), First->ApplyPreview(FGridItemInspectionView()) == EGridItemPreviewResult::InvalidSelection);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemInspectionLifecycleTest, "Grimrock.Items.Inspection.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemInspectionLifecycleTest::RunTest(const FString&)
{
	const FGridCanonicalRegistryFixture Fixture;
	FInspectionTestWorld Host;
	FInspectionTestWorld Game(EWorldType::Game);
	if (!TestTrue(TEXT("Production registry is configured"), Fixture.bReady) || !TestNotNull(TEXT("Preview world"), Host.World) ||
		!TestNotNull(TEXT("Ordinary world"), Game.World))
	{
		return false;
	}
	AGridItemPreviewActor* Preview = Host.World->SpawnActor<AGridItemPreviewActor>();
	AGridItemPreviewActor* WrongHost = Game.World->SpawnActor<AGridItemPreviewActor>();
	if (!TestNotNull(TEXT("Preview"), Preview) || !TestNotNull(TEXT("Unintended host"), WrongHost))
	{
		return false;
	}
	FGridItemInspectionView View;
	if (!TestTrue(TEXT("N17 catalyst resolves"), FGridItemInspectionReadModel::ResolveExact(Fixture.Registry.Get(), TEXT("Weapon_FireWand_01"), 17, View)))
	{
		return false;
	}
	TestTrue(TEXT("Ordinary game world is refused"), WrongHost->ApplyPreview(View) == EGridItemPreviewResult::InvalidHost);
	TestNull(TEXT("No gameplay mesh created"), WrongHost->GetPreviewMesh()->GetStaticMesh());
	if (!TestTrue(TEXT("N17 preview ready"), Preview->ApplyPreview(View) == EGridItemPreviewResult::Ready))
	{
		return false;
	}
	TestEqual(TEXT("N17 config carries the existing persistent effect"), Preview->GetPreviewEffects()->GetConfiguredEffectCount(), 1);
	TWeakObjectPtr<UNiagaraComponent> Previous(Preview->GetPreviewEffects()->GetEffectComponent(0));
	if (TestTrue(TEXT("N16 catalyst resolves"), FGridItemInspectionReadModel::ResolveExact(Fixture.Registry.Get(), TEXT("Weapon_FireWand_01"), 16, View)))
	{
		TestTrue(TEXT("N16 preview ready"), Preview->ApplyPreview(View) == EGridItemPreviewResult::Ready);
		TestEqual(TEXT("N16 has no persistent effect"), Preview->GetPreviewEffects()->GetConfiguredEffectCount(), 0);
		TestTrue(TEXT("Former effect is no longer registered"), !Previous.IsValid() || !Previous->IsRegistered());
	}
	Preview->ClearPreview();
	Preview->ClearPreview();
	TestNull(TEXT("Repeated cleanup releases mesh"), Preview->GetPreviewMesh()->GetStaticMesh());
	TestEqual(TEXT("Repeated cleanup releases configured effects"), Preview->GetPreviewEffects()->GetConfiguredEffectCount(), 0);
	TestTrue(TEXT("Preview actor can be destroyed"), Preview->Destroy());
	return !HasAnyErrors();
}

#endif

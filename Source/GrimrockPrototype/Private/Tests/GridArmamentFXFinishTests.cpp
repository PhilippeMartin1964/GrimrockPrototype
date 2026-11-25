#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Runtime/GridItemActor.h"
#include "Tests/GridCanonicalRegistryFixture.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridArmamentFXFinishTest, "Grimrock.RPG.ARMEMENT.FX01.LevelFinishes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridArmamentFXFinishTest::RunTest(const FString&)
{
	const FGridCanonicalRegistryFixture Fixture;
	if (!TestTrue(TEXT("Production registry loaded"), Fixture.bReady))
	{
		return false;
	}
	const FName Names[] = { TEXT("Brut"), TEXT("Soigne"), TEXT("Grave"), TEXT("Magistral"), TEXT("Mythique") };
	TArray<FName> Items;
	Fixture.Registry->GetItemIds(Items);
	int32 Armaments = 0;
	for (FName Item : Items)
	{
		FGridResolvedItem ItemProfile;
		if (!Fixture.Resolve(Item, 1, ItemProfile) || !ItemProfile.bIsArmament)
		{
			continue;
		}
		++Armaments;
		for (int32 Level = 1; Level <= 20; ++Level)
		{
			if (TestTrue(TEXT("A physical level resolves"), Fixture.Resolve(Item, Level, ItemProfile)))
			{
				TestEqual(TEXT("The level selects the authored finish"), ItemProfile.WeaponFinish.FinishId, Names[(Level - 1) / 4]);
			}
		}
		TestFalse(TEXT("An armament at N0 never acquires a finish"), Fixture.Registry->ResolveItem(Item, 0, ItemProfile));
	}
	TestEqual(TEXT("All materialized armaments use five bands"), Armaments, 22);

	FGridItemInstance First;
	First.ItemDefinitionId = TEXT("Weapon_StraightSword_01");
	First.RuntimeObjectId = FGuid::NewGuid();
	First.Quantity = 1;
	First.ItemLevel = 5;
	FGridItemInstance Second = First;
	Second.RuntimeObjectId = FGuid::NewGuid();
	Second.ItemLevel = 17;
	auto* FirstActor = NewObject<AGridItemActor>();
	auto* SecondActor = NewObject<AGridItemActor>();
	if (!TestTrue(TEXT("Two physical swords initialize independently"),
			FirstActor->InitializeFromItemInstance(First, Fixture.Registry.Get()) && SecondActor->InitializeFromItemInstance(Second, Fixture.Registry.Get())))
	{
		return false;
	}
	TestEqual(TEXT("Both finishes use the same mesh"), FirstActor->MeshComponent->GetStaticMesh(), SecondActor->MeshComponent->GetStaticMesh());
	auto* FirstMaterial = Cast<UMaterialInstanceDynamic>(FirstActor->MeshComponent->GetMaterial(0));
	auto* SecondMaterial = Cast<UMaterialInstanceDynamic>(SecondActor->MeshComponent->GetMaterial(0));
	if (!TestNotNull(TEXT("N5 material instance"), FirstMaterial) || !TestNotNull(TEXT("N17 material instance"), SecondMaterial))
	{
		return false;
	}
	const FName Tint(TEXT("BaseColorTint"));
	const FLinearColor SecondTint = SecondMaterial->K2_GetVectorParameterValue(Tint);
	TestNotEqual(TEXT("The two levels have distinct tint"), FirstMaterial->K2_GetVectorParameterValue(Tint), SecondTint);
	FirstMaterial->SetVectorParameterValue(Tint, FLinearColor::Blue);
	TestEqual(TEXT("Editing one actor leaves the other material untouched"), SecondMaterial->K2_GetVectorParameterValue(Tint), SecondTint);

	First.ItemLevel = 1;
	if (TestTrue(TEXT("Reinitialization applies N1"), FirstActor->InitializeFromItemInstance(First, Fixture.Registry.Get())))
	{
		TestFalse(TEXT("N1 restores the material from the mesh slot"), FirstActor->MeshComponent->GetMaterial(0)->IsA<UMaterialInstanceDynamic>());
	}
	TestEqual(TEXT("World projection retains the source GUID"), SecondActor->GetRuntimeObjectId(), Second.RuntimeObjectId);
	TestEqual(TEXT("World projection retains the source level"), SecondActor->WorldItemLevel, Second.ItemLevel);
	return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063ARealRegistry,
	"Grimrock.RPG.ARM.CLEAN06_3.RealNativeRegistryAndActions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063ARealRegistry::RunTest(const FString&)
{
	TArray<FGridWeaponPipelineRow> Manifest;
	FString Error;
	if (!TestTrue(TEXT("Production manifest of 22 concrete ARM"),
		FGridWeaponPipelineManifest::Load(
			FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)) ||
		!TestEqual(TEXT("22 production ARM"), Manifest.Num(), 22))
	{
		AddError(Error);
		return false;
	}

	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/"
			"DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Canonical registry is a real saved .uasset"), Root.Get()))
	{
		return false;
	}
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Real registry validates without any old-item fallback"),
		Root->Validate(Errors)))
	{
		for (const FString& Message : Errors) { AddError(Message); }
		return false;
	}
	TestEqual(TEXT("22 ARM + 1 existing arrow source"), Root->ItemDefinitions.Num(), 23);
	TestEqual(TEXT("All four catalogues authored in their final C++ classes"),
		Root->ActionCatalogue->Actions.Num(), 19);
	TestEqual(TEXT("One neutral and four real presentation profiles"),
		Root->ActionPresentationCatalogue->Presentations.Num(), 5);
	TestEqual(TEXT("All N1-N20 weapon archetypes persisted"),
		Root->WeaponCatalogue->Archetypes.Num(), 82);

	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Items(
		NewObject<UGridItemSubsystem>(GameInstance.Get()));
	Errors.Reset();
	if (!TestTrue(TEXT("Register persistent root once in actual runtime resolver"),
		Items->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Message : Errors) { AddError(Message); }
		return false;
	}

	int32 AttackCount = 0;
	int32 PassiveCount = 0;
	int32 NeutralCount = 0;
	for (const FGridWeaponPipelineRow& Row : Manifest)
	{
		const FName ItemId(*Row.ItemDefinitionId);
		FGridResolvedItem N1, N20;
		if (!TestTrue(TEXT("Persisted ARM resolves at N1"),
				Items->ResolveItem(ItemId, 1, N1)) ||
			!TestTrue(TEXT("Persisted ARM resolves at N20"),
				Items->ResolveItem(ItemId, 20, N20)))
		{
			return false;
		}
		TestEqual(TEXT("Same immutable ItemId at both levels"), N20.ItemId, ItemId);
		TestEqual(TEXT("One and only one armament archetype identity"), N20.ArchetypeId,
			FName(*Row.ArchetypeId));
		TestEqual(TEXT("Weight belongs to item identity, not a mutable level"),
			N20.WeightKg, N1.WeightKg);
		if (Row.IsPassive())
		{
			++PassiveCount;
			TestTrue(TEXT("Passive ARM has no authored attack"), N20.AvailableActions.IsEmpty());
			continue;
		}
		++AttackCount;
		FGridResolvedBoundAction Bound;
		if (!TestTrue(TEXT("Real registered action and presentation resolve"),
			Items->ResolveBoundAttackForHand(
				ItemId, 20, FName(*Row.AttackId), EGridEquipmentSlot::MainHand, Bound)))
		{
			return false;
		}
		TestEqual(TEXT("No substituted action identity"), Bound.ActionId,
			FName(*Row.AttackId));
		if (Bound.PresentationId == FName(TEXT("PRES_AttackNoFX")))
		{
			++NeutralCount;
			TestTrue(TEXT("Neutral profile invents no attack audio or VFX"),
				Bound.Presentation.AttackAudio.Sounds.IsEmpty() &&
				Bound.Presentation.AttackVFX.Systems.IsEmpty());
		}
	}
	TestEqual(TEXT("19 real attacks"), AttackCount, 19);
	TestEqual(TEXT("3 real passives"), PassiveCount, 3);
	TestEqual(TEXT("15 authored links to the shared historical neutral presentation"),
		NeutralCount, 15);

	FGridResolvedItem Arrows;
	TestTrue(TEXT("Existing production arrows resolved from same canonical registry"),
		Items->ResolveItem(TEXT("Item_Arrows_01"), 1, Arrows));
	TestEqual(TEXT("Authoritative arrow type"), Arrows.AmmoKindId,
		FName(TEXT("Item_Arrows_01")));
	return AttackCount == 19 && PassiveCount == 3 && NeutralCount == 15;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Tests/GridCanonicalLootRegistryTestUtils.h"
#include "Engine/GameInstance.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/Combat/GridAttackPresentationReadModel.h"
#include "../ARMEMENT/GridEditorRetirementAudit.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

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

	// Loading the binary explicitly is insufficient: prove the actual
	// startup configuration points to the same saved registry.
	const UGridItemSubsystem* Defaults = GetDefault<UGridItemSubsystem>();
	const FSoftObjectProperty* Config = FindFProperty<FSoftObjectProperty>(
		UGridItemSubsystem::StaticClass(), TEXT("ConfiguredItemSystem"));
	if (!TestNotNull(TEXT("One config-backed canonical item-system reference"), Config))
	{
		return false;
	}
	const FSoftObjectPtr* ConfiguredPath =
		Config->ContainerPtrToValuePtr<FSoftObjectPtr>(Defaults);
	if (!TestNotNull(TEXT("Canonical registry config value is readable"), ConfiguredPath))
	{
		return false;
	}
	TestEqual(TEXT("DefaultGame.ini points at the only native ItemSystem root"),
		ConfiguredPath->ToSoftObjectPath().ToString(),
		FString(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));

	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
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
	// Migration creates additional canonical non-ARM definitions in this real
	// root. Retain the original 22 ARM + arrow baseline and require every
	// appended entry to remain canonical; do not hardcode a permanent size 23.
	TestTrue(TEXT("22 ARM + arrow baseline remains registered"),
		GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root.Get()));
	TSet<FName> UniqueIds;
	for (const UGridItemDefinitionAsset* Definition : Root->ItemDefinitions)
	{
		if (!TestNotNull(TEXT("Saved canonical item is materialized"), Definition))
		{
			return false;
		}
		TestFalse(TEXT("No duplicate migrated ItemId"), UniqueIds.Contains(Definition->ItemId));
		UniqueIds.Add(Definition->ItemId);
	}
	TestEqual(TEXT("All four catalogues authored in their final C++ classes"),
		Root->ActionCatalogue->Actions.Num(), 19);
	for (const FName ProfileId :
		{ FName(TEXT("PRES_AttackNoFX")), FName(TEXT("PRES_ARM001")), FName(TEXT("PRES_ARM013")), FName(TEXT("PRES_ARM014")), FName(TEXT("PRES_ARM015")) })
	{
		TestNotNull(FString::Printf(TEXT("Original attack presentation %s remains authored"), *ProfileId.ToString()),
			Root->ActionPresentationCatalogue->Find(ProfileId));
	}
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

	TestFalse(TEXT("Unknown item never enters the canonical registry"),
		Items->ContainsItem(TEXT("NonProductionItem")));
	// Pre-retirement authored sources are all present or all gone. After
	// retirement, the canonical runtime assertions below remain mandatory.
	IAssetRegistry& AR = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		TEXT("AssetRegistry")).Get();
	AR.SearchAllAssets(true);
	TArray<FAssetData> WeaponAssets;
	AR.GetAssetsByPath(
		FName(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons")),
		WeaponAssets, false);
	TSet<FString> HistoricalPaths;
	for (const FAssetData& Asset : WeaponAssets)
	{
		if (GridEditorRetirementAudit::IsRetiredItemPackage(Asset.PackageName))
			HistoricalPaths.Add(Asset.GetSoftObjectPath().ToString());
	}
	int32 HistoricSourceCount = 0;
	for (const FGridWeaponPipelineRow& Row : Manifest)
		HistoricSourceCount += HistoricalPaths.Contains(Row.ItemPath) ? 1 : 0;
	if (!TestEqual(TEXT("No historical ARM source package remains"), HistoricSourceCount, 0)) return false;

	int32 AttackCount = 0;
	int32 PassiveCount = 0;
	int32 NeutralCount = 0;
	for (const FGridWeaponPipelineRow& Row : Manifest)
	{
		const FName ItemId(*Row.ItemDefinitionId);
		TestTrue(TEXT("Each official ARM is known to the one runtime registry"),
			Items->ContainsItem(ItemId));
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
		// BuildKind is a production label, not evidence of the actual
		// authored hand policy. That policy was measured from the UE asset.
		TestEqual(TEXT("Real authored hand policy remains stable from N1 to N20"),
			N20.Weapon.HandPolicy, N1.Weapon.HandPolicy);
		TestTrue(TEXT("Canonical weapon accepts its authored equipment hand"),
			N20.Weapon.CanEquipInHand(Row.IsPassive()
				? EGridEquipmentSlot::OffHand : EGridEquipmentSlot::MainHand));
		TestFalse(TEXT("Canonical armament never equips outside a hand slot"),
			N20.Weapon.CanEquipInHand(EGridEquipmentSlot::None));
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
		TestTrue(TEXT("Canonical compact standard attack has no unprojected effects"),
			Bound.Action.Effects.IsEmpty() && Bound.Action.WeaponDamagePercent == 0);
		TestEqual(TEXT("No substituted action identity"), Bound.ActionId,
			FName(*Row.AttackId));
		TestTrue(TEXT("N20 canonical weapon dice produce a complete attack"),
			N20.Weapon.DamageDice.IsValid() &&
			N20.Weapon.DamageDice.GetMaximumDamage() > 0 &&
			N20.Weapon.ActionPointCost > 0 &&
			N20.Weapon.RangeCells > 0);
		// Same identity gate used by the live equipment action enumerator.
		FGridItemInstance Equipped;
		Equipped.RuntimeObjectId = FGuid::NewGuid();
		Equipped.ItemDefinitionId = ItemId;
		Equipped.ItemLevel = 20;
		Equipped.Quantity = 1;
		Equipped.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Equipped.OwnerGuid = FGuid::NewGuid();
		Equipped.OwnerCharacterIndex = 0;
		Equipped.EquipmentSlot = EGridEquipmentSlot::MainHand;
		FGridResolvedBoundAction VerifiedInstance;
		TestTrue(TEXT("Real source slot identity binds exact canonical attack"),
			Items->ResolveAttackFromEquippedInstance(
				Equipped, Bound.ActionId, EGridEquipmentSlot::MainHand,
				Equipped.RuntimeObjectId, Equipped.OwnerGuid,
				Equipped.OwnerCharacterIndex, VerifiedInstance));
		TestEqual(TEXT("Source instance attack matches registered ActionId"),
			VerifiedInstance.ActionId, Bound.ActionId);
		// Compare the actual execution view, not only the Editor forward
		// conversion. No persisted fields, media order or timings may be lost.
		FGridPlayerAttackPresentationProfile FirstNativePresentation;
		bool bHasFirstNativePresentation = false;
		for (const int32 Level : {1, 20})
		{
			Equipped.ItemLevel = Level;
			FGridPlayerAttackRequest Request;
			Request.OffensiveItemDefinitionId = ItemId;
			Request.OffensiveEquipmentSlot = EGridEquipmentSlot::MainHand;
			Request.AttackId = Bound.ActionId;
			Request.PresentationSource = FGridAttackPresentationReadModel::CaptureFromInstance(
				Items.Get(), Equipped, Equipped.OwnerGuid, 0, EGridEquipmentSlot::MainHand, Bound.ActionId, false);
			FGridPlayerAttackPresentationProfile Actual;
			const bool bPresentationResolved = FGridAttackPresentationReadModel::TryResolve(
				Items.Get(), Request, Actual);
			TestTrue(TEXT("Exact canonical runtime attack presentation"), bPresentationResolved);
			if (bPresentationResolved)
			{
				if (!bHasFirstNativePresentation)
				{
					FirstNativePresentation = Actual;
					bHasFirstNativePresentation = true;
				}
				else
				{
					TestTrue(TEXT("N1 and N20 maintain identical runtime attack presentation"),
						FGridPlayerAttackPresentationProfile::StaticStruct()->CompareScriptStruct(
							&Actual, &FirstNativePresentation, 0));
				}
			}
			TestEqual(TEXT("Real source level captured"), Request.PresentationSource.ItemLevel, Level);
			TestEqual(TEXT("Real source GUID captured"), Request.PresentationSource.RuntimeObjectId, Equipped.RuntimeObjectId);
			Request.PresentationSource.ItemLevel = 0;
			TestFalse(TEXT("Known ARM cannot synthesize N1 media"), FGridAttackPresentationReadModel::TryResolve(Items.Get(), Request, Actual));
		}
		Equipped.ItemLevel = 0;
		TestFalse(TEXT("Old Level=0 cannot read N1 implicitly"),
			Items->ResolveAttackFromEquippedInstance(
				Equipped, Bound.ActionId, EGridEquipmentSlot::MainHand,
				Equipped.RuntimeObjectId, Equipped.OwnerGuid,
				Equipped.OwnerCharacterIndex, VerifiedInstance));
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
		Items->ResolveItem(TEXT("Item_Arrows_01"), 0, Arrows));
	TestEqual(TEXT("Authoritative arrow type"), Arrows.AmmoKindId,
		FName(TEXT("Item_Arrows_01")));
	return AttackCount == 19 && PassiveCount == 3 && NeutralCount == 15;
}

#endif

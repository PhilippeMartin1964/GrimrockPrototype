#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridArmClean06ItemIdentityProjection.h"
#include "ARMEMENT/GridArmClean06SaveInstancePreview.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062ADSaveInstanceParity,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionARMInstanceSaveRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062ADSaveInstanceParity::RunTest(const FString&)
{
	TArray<FGridWeaponPipelineRow> Manifest;
	FString Error;
	if (!TestTrue(TEXT("Real production manifest loads"),
		FGridWeaponPipelineManifest::Load(
			FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
	{
		AddError(Error);
		return false;
	}
	if (!TestEqual(TEXT("Only 22 authored ARM items"), Manifest.Num(), 22))
	{
		return false;
	}
	using namespace GridArmClean06SaveInstancePreview;
	TSet<FName> SeenItems;
	int32 RoundTrips = 0;
	int32 PassiveItems = 0;
	int32 WithWorldLocation = 0;
	for (int32 Index = 0; Index < Manifest.Num(); ++Index)
	{
		const FGridWeaponPipelineRow& Row = Manifest[Index];
		const UGridItemDefinitionAsset* Historical =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
		const FName ItemId(*Row.ItemDefinitionId);
		if (!IsValid(Historical) ||
			Row.ProductionState != TEXT("Active") ||
			SeenItems.Contains(ItemId))
		{
			AddError(TEXT("Source ARM identity not uniquely loadable"));
			return false;
		}
		SeenItems.Add(ItemId);
		const bool bSourceDirty = Historical->GetOutermost()->IsDirty();
		TStrongObjectPtr<UGridArmamentDefinitionAsset> Canonical(
			NewObject<UGridArmamentDefinitionAsset>(GetTransientPackage()));
		if (!GridArmClean06ItemIdentityProjection::TryProject(
				*Historical, *Canonical))
		{
			AddError(TEXT("Concrete ARM identity cannot project exactly"));
			return false;
		}
		if (Row.IsPassive()) { ++PassiveItems; }
		for (const int32 Level : { 1, 20 })
		{
			FGridItemInstance Original;
			Original.RuntimeObjectId = FGuid(0xAD06, Index + 1, Level, 0x2026);
			Original.ItemDefinitionId = ItemId;
			Original.ItemLevel = Level;
			Original.Quantity = 1;
			Original.AbsorbedEquipmentPhysicalArmor = Row.IsPassive() ? 3 : 0;
			Original.AbsorbedEquipmentMagicalArmor = Row.IsFocus() ? 1 : 0;
			Original.OwnerType = (Index % 4 == 0) ? EGridItemOwnerType::EquipmentSlot :
				(Index % 4 == 1) ? EGridItemOwnerType::CharacterInventory :
				(Index % 4 == 2) ? EGridItemOwnerType::Cursor :
				EGridItemOwnerType::World;
			Original.OwnerGuid = FGuid(0x2026, Index, Level, 0xAD);
			Original.OwnerCharacterIndex = Index % 3;
			Original.EquipmentSlot = Row.IsPassive() ?
				EGridEquipmentSlot::OffHand : EGridEquipmentSlot::MainHand;
			Original.bLightsEnabled = Index % 2 == 0;
			Original.ReadableContentId = Index == 0 ?
				FName(TEXT("TestReadingReference")) : NAME_None;
			Original.ReadTitleOverride = Index == 0 ?
				FText::FromString(TEXT("Preserved personal title")) : FText();
			Original.ReadTextOverride = Index == 0 ?
				FText::FromString(TEXT("Preserved custom reading text")) : FText();
			Original.LastWorldTransform = FTransform(
				FRotator(0.0f, Index * 5.0f, 0.0f),
				FVector(Index * 10.0f, 100.0f + Level, 3.0f),
				FVector(1.0f));

			FRecord Preview;
			EError Failure = EError::InvalidInstance;
			if (!TestTrue(TEXT("Existing N1 or N20 item state projects without loss"),
				TryProject(Original, *Historical, *Canonical, Preview, Failure)))
			{
				return false;
			}
			TestEqual(TEXT("Projection succeeds without migration error"),
				Failure, EError::None);
			FGridItemInstance Rehydrated;
			if (!TestTrue(TEXT("Canonical read model recreates all durable item fields"),
				TryRehydrateForParity(Preview, *Canonical, Rehydrated)))
			{
				return false;
			}
			if (!TestTrue(TEXT("Every durable field round-trips exactly"),
				IsExactRoundTrip(Original, Rehydrated)))
			{
				return false;
			}
			TestEqual(TEXT("Stable GUID preserved"), Preview.RuntimeObjectId, Original.RuntimeObjectId);
			TestEqual(TEXT("Level independent of character level"), Preview.ItemLevel, Level);
			TestEqual(TEXT("Depleted physical armor survives"), Preview.AbsorbedEquipmentPhysicalArmor,
				Original.AbsorbedEquipmentPhysicalArmor);
			TestEqual(TEXT("Depleted magical armor survives"), Preview.AbsorbedEquipmentMagicalArmor,
				Original.AbsorbedEquipmentMagicalArmor);
			TestEqual(TEXT("World ownership preserved"), Preview.OwnerType, Original.OwnerType);
			if (Preview.OwnerType == EGridItemOwnerType::World) { ++WithWorldLocation; }
			++RoundTrips;
		}
		if (Historical->GetOutermost()->IsDirty() != bSourceDirty)
		{
			AddError(TEXT("Read-only migration preview dirtied a real source package"));
			return false;
		}
	}
	TestEqual(TEXT("Exactly 44 source-guid/level combinations"), RoundTrips, 44);
	TestEqual(TEXT("Exactly three passive definitions"), PassiveItems, 3);
	TestTrue(TEXT("World-owner records were included"), WithWorldLocation > 0);
	AddInfo(FString::Printf(TEXT(
		"ARM 06.2AD: %d real ARM identities; %d N1/N20 instance round trips; "
		"%d passive source items. GUID, level, quantity, armor debt, owner, "
		"reading and world transform preserved. No SaveGame or asset was written."),
		SeenItems.Num(), RoundTrips, PassiveItems));
	return SeenItems.Num() == 22 && RoundTrips == 44 && PassiveItems == 3;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062ADRejectLossySave,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.RejectLossySaveInstanceMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062ADRejectLossySave::RunTest(const FString&)
{
	using namespace GridArmClean06SaveInstancePreview;
	TStrongObjectPtr<UGridItemDefinitionAsset> Source(
		NewObject<UGridItemDefinitionAsset>(GetTransientPackage()));
	TStrongObjectPtr<UGridArmamentDefinitionAsset> Canonical(
		NewObject<UGridArmamentDefinitionAsset>(GetTransientPackage()));
	Source->ItemDefinitionId = TEXT("Weapon_TestCanonicalSave");
	Source->WeaponArchetypeId = TEXT("ARM014");
	Source->DisplayName = FText::FromString(TEXT("Sword"));
	Source->Weight = 2.5f;
	Source->bUseWeaponCatalogue = true;
	if (!TestTrue(TEXT("Source identity projected"),
		GridArmClean06ItemIdentityProjection::TryProject(*Source, *Canonical)))
	{
		return false;
	}
	FGridItemInstance Item;
	Item.RuntimeObjectId = FGuid(1, 2, 3, 4);
	Item.ItemDefinitionId = Source->ItemDefinitionId;
	Item.ItemLevel = 20;
	Item.Quantity = 2;
	Item.OwnerType = EGridItemOwnerType::Cursor;
	Item.AbsorbedEquipmentPhysicalArmor = 3;
	Item.AbsorbedEquipmentMagicalArmor = 1;
	Item.LastWorldTransform = FTransform(FRotator(0, 30, 0), FVector(10, 20, 30), FVector(1));
	FRecord Preview;
	EError Failure;
	if (!TestTrue(TEXT("Canonical baseline round-trips"),
		TryProject(Item, *Source, *Canonical, Preview, Failure)))
	{
		return false;
	}
	TestEqual(TEXT("Item owner type preserved"), Preview.OwnerType, EGridItemOwnerType::Cursor);
	TestEqual(TEXT("Physical armor debt is not restored for free"),
		Preview.AbsorbedEquipmentPhysicalArmor, 3);
	TestEqual(TEXT("Magical armor debt is not restored for free"),
		Preview.AbsorbedEquipmentMagicalArmor, 1);
	FGridItemInstance Rehydrated;
	TestTrue(TEXT("Preview can be precisely rebuilt"),
		TryRehydrateForParity(Preview, *Canonical, Rehydrated));
	TestTrue(TEXT("No field changed"), IsExactRoundTrip(Item, Rehydrated));

	Item.ItemLevel = 0;
	TestFalse(TEXT("Legacy level 0 must not become a made-up N1"),
		TryProject(Item, *Source, *Canonical, Preview, Failure));
	TestEqual(TEXT("Legacy zero has an explicit reject reason"),
		Failure, EError::LegacyLevelZero);
	TestTrue(TEXT("No stale preview survives a failed attempt"), Preview.ItemId.IsNone());
	Item.ItemLevel = 20;

	TestNull(TEXT("Static name no longer belongs to the reflected item schema"),
		FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("DisplayName")));
	TestNull(TEXT("Static mass no longer belongs to the reflected item schema"),
		FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("Weight")));
	Canonical->DisplayName = FText::FromString(TEXT("Contradictory definition name"));
	TestFalse(TEXT("Historical source name mismatch still rejects"),
		TryProject(Item, *Source, *Canonical, Preview, Failure));
	TestEqual(TEXT("Unexpected definition name requires reconciliation"), Failure,
		EError::StaticSnapshotMismatch);
	Canonical->DisplayName = Source->DisplayName;

	Canonical->WeightKg += 0.2f;
	TestFalse(TEXT("Historical source mass mismatch still rejects"),
		TryProject(Item, *Source, *Canonical, Preview, Failure));
	TestEqual(TEXT("Noncanonical definition mass cannot be ignored"), Failure,
		EError::StaticSnapshotMismatch);
	Canonical->WeightKg = Source->Weight;

	Item.AbsorbedEquipmentPhysicalArmor = -1;
	TestFalse(TEXT("Negative armor debt is rejected instead of clamped"),
		TryProject(Item, *Source, *Canonical, Preview, Failure));
	TestEqual(TEXT("Exact invalid debt classification"), Failure,
		EError::InvalidArmorDepletion);
	Item.AbsorbedEquipmentPhysicalArmor = 3;

	Item.ItemDefinitionId = TEXT("Weapon_WrongIdentity");
	TestFalse(TEXT("Another ItemId must never be remapped by name"),
		TryProject(Item, *Source, *Canonical, Preview, Failure));
	TestEqual(TEXT("Wrong item is rejected by identity"),
		Failure, EError::IdentityMismatch);
	return true;
}
#endif

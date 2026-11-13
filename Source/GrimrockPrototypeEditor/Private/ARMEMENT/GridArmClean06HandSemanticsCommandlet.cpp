#include "ARMEMENT/GridArmClean06HandSemanticsCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Engine/DataAsset.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	const TCHAR* CataloguePath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue");
	const FName WeaponFolder =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons");

	FString ResolveEffectiveHandPolicy(const UGridItemDefinitionAsset& Item)
	{
		const EGridItemHandUsage Usage = Item.GetEffectiveHandUsage();
		const bool bMain = Item.CanEquipToSlot(EGridEquipmentSlot::MainHand);
		const bool bOff = Item.CanEquipToSlot(EGridEquipmentSlot::OffHand);
		if (Usage == EGridItemHandUsage::TwoHanded && bMain && bOff)
		{
			return TEXT("TwoHanded");
		}
		if (Usage == EGridItemHandUsage::OneHanded)
		{
			if (bMain && bOff) { return TEXT("EitherHand"); }
			if (bMain) { return TEXT("MainHand"); }
			if (bOff) { return TEXT("OffHand"); }
		}
		// In particular, NotHandHeld and unresolved Auto are NOT silently
		// converted to MainHand: the future migration must fail closed.
		return TEXT("Unresolved");
	}

	FString ResolveItemArmamentKind(const UGridItemDefinitionAsset& Item,
		const FGridWeaponLevelProfile& Profile)
	{
		if (Item.ItemType == EGridItemType::Shield && !Profile.DamageDice.bEnabled)
		{
			return TEXT("Shield");
		}
		if (Item.ItemType == EGridItemType::Weapon && Profile.DamageDice.bEnabled)
		{
			return TEXT("Weapon");
		}
		// A nonoffensive Weapon may be a Focus, but damage dice alone cannot
		// distinguish a focus from a shield or an unconfigured asset.
		return TEXT("Unresolved");
	}
}

UGridArmClean06HandSemanticsCommandlet::UGridArmClean06HandSemanticsCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGridArmClean06HandSemanticsCommandlet::Main(const FString& Params)
{
	FString OutputPath;
	if (!FParse::Value(*Params, TEXT("Output="), OutputPath) || OutputPath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2E] Explicit -Output= required."));
		return 1;
	}

	UGridWeaponCatalogueAsset* Source =
		LoadObject<UGridWeaponCatalogueAsset>(nullptr, CataloguePath);
	if (!IsValid(Source) || !Source->IsValidCatalogue() ||
		!Source->IsValidClassAffinityData() ||
		Source->ArchetypeProgressions.Num() != 82 || !Source->Profiles.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2E] Source catalogue invalid."));
		return 2;
	}

	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		TEXT("AssetRegistry")).Get();
	Registry.SearchAllAssets(true);

	TSet<FName> CatalogueIds;
	TSet<FString> Families;
	TSet<FString> RawHandLabels;
	TArray<TSharedPtr<FJsonValue>> ArchetypeRows;
	for (const FGridWeaponArchetypeProgression& Entry : Source->ArchetypeProgressions)
	{
		const FGridWeaponLevelProfile& N1 = Entry.BaseProfile;
		if (N1.ArchetypeId.IsNone() || CatalogueIds.Contains(N1.ArchetypeId) ||
			N1.Family.IsEmpty() || N1.HandUsage.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2E] Invalid source ARM identity / semantic labels."));
			return 3;
		}
		CatalogueIds.Add(N1.ArchetypeId);
		Families.Add(N1.Family);
		RawHandLabels.Add(N1.HandUsage);
		TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
		Row->SetStringField(TEXT("archetype_id"), N1.ArchetypeId.ToString());
		Row->SetStringField(TEXT("source_family"), N1.Family);
		Row->SetStringField(TEXT("source_hand_usage_label"), N1.HandUsage);
		Row->SetBoolField(TEXT("passive_no_damage"), !N1.DamageDice.bEnabled);
		Row->SetBoolField(TEXT("hand_policy_proven_for_all_variants"), false);
		Row->SetBoolField(TEXT("passive_kind_proven_for_all_variants"), false);
		ArchetypeRows.Add(MakeShared<FJsonValueObject>(Row));
	}
	if (CatalogueIds.Num() != 82)
	{
		return 3;
	}

	TArray<FAssetData> FolderAssets;
	Registry.GetAssetsByPath(WeaponFolder, FolderAssets, false);
	// Important: a package is still part of the migration baseline even if
	// its UObject does not derive from UGridItemDefinitionAsset.
	const FString WeaponPackagePrefix = WeaponFolder.ToString() + TEXT("/DA_Weapon_");
	TArray<FAssetData> WeaponAssets;
	for (const FAssetData& Asset : FolderAssets)
	{
		if (Asset.PackageName.ToString().StartsWith(WeaponPackagePrefix))
		{
			WeaponAssets.Add(Asset);
		}
	}
	WeaponAssets.Sort([](const FAssetData& A, const FAssetData& B)
	{
		return A.PackageName.ToString() < B.PackageName.ToString();
	});
	TSet<FName> SeenPackages;
	int32 DuplicatePackages = 0;
	int32 LoadedExpectedClass = 0;
	int32 LoadedOtherClass = 0;
	int32 UnloadablePackages = 0;
	TSet<FName> ConcreteIds;
	int32 MissingItemIds = 0;
	int32 DuplicateItemIds = 0;
	TMap<FName, FString> HandByArchetype;
	TMap<FName, FString> KindByArchetype;
	TSet<FName> UnambiguousArchetypes;
	TArray<TSharedPtr<FJsonValue>> ConcreteRows;
	int32 ConcreteCount = 0;
	int32 Joined = 0;
	int32 HandProven = 0;
	int32 KindProven = 0;
	int32 SemanticConflicts = 0;
	for (const FAssetData& Asset : WeaponAssets)
	{
		// Every DA_Weapon_* registry record must survive classification;
		// otherwise a misleading 24-file / N-UGridItemDefinitionAsset count
		// stops the audit before the real class mismatch can be diagnosed.
		++ConcreteCount;
		const bool bDuplicatePackage = SeenPackages.Contains(Asset.PackageName);
		if (bDuplicatePackage) { ++DuplicatePackages; }
		SeenPackages.Add(Asset.PackageName);
		UObject* LoadedObject = Asset.GetAsset();
		const bool bLoaded = IsValid(LoadedObject);
		const UGridItemDefinitionAsset* Item = bLoaded ?
			Cast<UGridItemDefinitionAsset>(LoadedObject) : nullptr;
		if (!bLoaded) { ++UnloadablePackages; }
		else if (!Item) { ++LoadedOtherClass; }
		else { ++LoadedExpectedClass; }

		TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
		Row->SetStringField(TEXT("asset_package"), Asset.PackageName.ToString());
		Row->SetStringField(TEXT("registry_class"), Asset.AssetClassPath.ToString());
		Row->SetStringField(TEXT("loaded_class"),
			bLoaded ? LoadedObject->GetClass()->GetPathName() : TEXT(""));
		Row->SetStringField(TEXT("load_classification"),
			!bLoaded ? TEXT("unloadable") :
			(Item ? TEXT("expected_class") : TEXT("other_class")));
		// Structural facts only: an unfamiliar class must not be relabelled
		// as a weapon, obsolete asset or Blueprint based on its filename.
		Row->SetBoolField(TEXT("is_data_asset"),
			bLoaded && LoadedObject->IsA<UDataAsset>());
		Row->SetBoolField(TEXT("is_primary_data_asset"),
			bLoaded && LoadedObject->IsA<UPrimaryDataAsset>());
		Row->SetBoolField(TEXT("class_compiled_from_blueprint"),
			bLoaded && LoadedObject->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint));
		Row->SetBoolField(TEXT("duplicate_package"), bDuplicatePackage);
		if (!Item)
		{
			// Count the package, preserve its class, and leave every item
			// property unresolved instead of inventing ItemId or hand usage.
			Row->SetStringField(TEXT("item_id"), TEXT(""));
			Row->SetStringField(TEXT("source_archetype_id"), TEXT(""));
			Row->SetBoolField(TEXT("joined_to_source_n1"), false);
			Row->SetStringField(TEXT("provable_hand_policy"), TEXT("Unresolved"));
			Row->SetStringField(TEXT("provable_kind"), TEXT("Unresolved"));
			ConcreteRows.Add(MakeShared<FJsonValueObject>(Row));
			continue;
		}
		// The census must NOT discard a real UE asset because its ItemId is
		// empty or duplicates another asset (including ThrowingKnife variants).
		// Record it as a migration blocker, without merging or authoring data.
		const bool bMissingId = Item->ItemDefinitionId.IsNone();
		const bool bDuplicateId = !bMissingId && ConcreteIds.Contains(Item->ItemDefinitionId);
		if (bMissingId) { ++MissingItemIds; }
		if (bDuplicateId) { ++DuplicateItemIds; }
		if (!bMissingId && !bDuplicateId)
		{
			ConcreteIds.Add(Item->ItemDefinitionId);
		}

		const FGridWeaponLevelProfile* SourceN1 =
			Item->bUseWeaponCatalogue ?
			Source->FindProfile(Item->WeaponArchetypeId, 1) : nullptr;
		const bool bJoined = SourceN1 != nullptr &&
			CatalogueIds.Contains(Item->WeaponArchetypeId);
		const FString HandPolicy = bJoined ? ResolveEffectiveHandPolicy(*Item) :
			TEXT("Unresolved");
		const FString Kind = bJoined ? ResolveItemArmamentKind(*Item, *SourceN1) :
			TEXT("Unresolved");
		if (bJoined) { ++Joined; }
		if (HandPolicy != TEXT("Unresolved")) { ++HandProven; }
		if (Kind != TEXT("Unresolved")) { ++KindProven; }

		if (bJoined)
		{
			const FString* PriorHand = HandByArchetype.Find(Item->WeaponArchetypeId);
			const FString* PriorKind = KindByArchetype.Find(Item->WeaponArchetypeId);
			if ((PriorHand && *PriorHand != HandPolicy) ||
				(PriorKind && *PriorKind != Kind))
			{
				++SemanticConflicts;
				UnambiguousArchetypes.Remove(Item->WeaponArchetypeId);
			}
			else if (!PriorHand)
			{
				UnambiguousArchetypes.Add(Item->WeaponArchetypeId);
			}
			HandByArchetype.FindOrAdd(Item->WeaponArchetypeId) = HandPolicy;
			KindByArchetype.FindOrAdd(Item->WeaponArchetypeId) = Kind;
		}

		Row->SetStringField(TEXT("item_id"), Item->ItemDefinitionId.ToString());
		Row->SetBoolField(TEXT("missing_item_id"), bMissingId);
		Row->SetBoolField(TEXT("duplicate_item_id"), bDuplicateId);
		Row->SetStringField(TEXT("source_archetype_id"), Item->WeaponArchetypeId.ToString());
		Row->SetBoolField(TEXT("b_use_weapon_catalogue"), Item->bUseWeaponCatalogue);
		Row->SetBoolField(TEXT("joined_to_source_n1"), bJoined);
		Row->SetNumberField(TEXT("item_type_enum"), static_cast<int32>(Item->ItemType));
		Row->SetNumberField(TEXT("effective_hand_usage_enum"),
			static_cast<int32>(Item->GetEffectiveHandUsage()));
		Row->SetNumberField(TEXT("declared_hand_usage_enum"),
			static_cast<int32>(Item->HandUsage));
		Row->SetBoolField(TEXT("declared_slot_main"),
			Item->CompatibleEquipmentSlots.Contains(EGridEquipmentSlot::MainHand));
		Row->SetBoolField(TEXT("declared_slot_off"),
			Item->CompatibleEquipmentSlots.Contains(EGridEquipmentSlot::OffHand));
		Row->SetNumberField(TEXT("combat_action_count"), Item->CombatActions.Num());
		Row->SetBoolField(TEXT("combat_throw_weapon"), Item->bCombatThrowWeapon);
		Row->SetBoolField(TEXT("source_n1_has_damage_dice"),
			bJoined && SourceN1->DamageDice.bEnabled);
		Row->SetBoolField(TEXT("can_equip_main"), Item->CanEquipToSlot(EGridEquipmentSlot::MainHand));
		Row->SetBoolField(TEXT("can_equip_off"), Item->CanEquipToSlot(EGridEquipmentSlot::OffHand));
		Row->SetStringField(TEXT("provable_hand_policy"), HandPolicy);
		Row->SetStringField(TEXT("provable_kind"), Kind);
		Row->SetStringField(TEXT("raw_catalogue_family"), bJoined ? SourceN1->Family : TEXT(""));
		Row->SetStringField(TEXT("raw_catalogue_hand_usage"), bJoined ? SourceN1->HandUsage : TEXT(""));
		ConcreteRows.Add(MakeShared<FJsonValueObject>(Row));
	}

	// The count now refers to DA_Weapon_* packages, not to their C++ base
	// class. Classification mismatches are audited without being discarded.
	// A preflight can reject an unexpected package cardinality AFTER the
	// private report is written, so the diagnostic itself is not lost.
	if (ConcreteCount != WeaponAssets.Num() ||
		LoadedExpectedClass + LoadedOtherClass + UnloadablePackages != ConcreteCount ||
		ConcreteRows.Num() != ConcreteCount)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2E] Internal package classification error."));
		return 5;
	}
	TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.2E/semantic-census/read-only/v1"));
	Report->SetNumberField(TEXT("archetypes"), CatalogueIds.Num());
	Report->SetNumberField(TEXT("source_family_variants"), Families.Num());
	Report->SetNumberField(TEXT("source_hand_label_variants"), RawHandLabels.Num());
	Report->SetNumberField(TEXT("concrete_weapon_assets"), ConcreteCount);
	Report->SetNumberField(TEXT("weapon_packages"), WeaponAssets.Num());
	Report->SetNumberField(TEXT("unique_weapon_packages"), SeenPackages.Num());
	Report->SetNumberField(TEXT("duplicate_weapon_packages"), DuplicatePackages);
	Report->SetNumberField(TEXT("count_expected_class"), LoadedExpectedClass);
	Report->SetNumberField(TEXT("count_other_class"), LoadedOtherClass);
	Report->SetNumberField(TEXT("count_unloadable"), UnloadablePackages);
	Report->SetNumberField(TEXT("unique_nonempty_item_ids"), ConcreteIds.Num());
	Report->SetNumberField(TEXT("missing_item_ids"), MissingItemIds);
	Report->SetNumberField(TEXT("duplicate_item_ids"), DuplicateItemIds);
	Report->SetNumberField(TEXT("concrete_joined"), Joined);
	Report->SetNumberField(TEXT("concrete_hand_policy_proven"), HandProven);
	Report->SetNumberField(TEXT("concrete_armament_kind_proven"), KindProven);
	Report->SetNumberField(TEXT("semantic_conflicts"), SemanticConflicts);
	Report->SetNumberField(TEXT("source_archetypes_without_any_concrete_definition"),
		CatalogueIds.Num() - HandByArchetype.Num());
	Report->SetArrayField(TEXT("source_archetype_rows"), ArchetypeRows);
	Report->SetArrayField(TEXT("concrete_weapon_rows"), ConcreteRows);
	Report->SetBoolField(TEXT("binary_migration_authorized"), false);
	Report->SetBoolField(TEXT("any_uasset_saved"), false);

	FString Json;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
	if (!FJsonSerializer::Serialize(Report, Writer) ||
		!IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutputPath), true) ||
		!FFileHelper::SaveStringToFile(Json, *OutputPath,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2E] Private JSON output failed."));
		return 6;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[ARM-CLEAN06.2E] PASS read-only census: %d weapon packages, %d expected item class, %d other class, %d unloadable, %d joined, %d empty IDs, %d repeated IDs; ZERO binary writes."),
		ConcreteCount, LoadedExpectedClass, LoadedOtherClass, UnloadablePackages,
		Joined, MissingItemIds, DuplicateItemIds);
	return 0;
}

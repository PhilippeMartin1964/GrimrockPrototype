#include "ARMEMENT/GridArmClean06HandSemanticsCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
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

	TArray<FAssetData> Assets;
	Registry.GetAssetsByPath(WeaponFolder, Assets, false);
	Assets.Sort([](const FAssetData& A, const FAssetData& B)
	{
		return A.PackageName.ToString() < B.PackageName.ToString();
	});
	TSet<FName> ConcreteIds;
	TMap<FName, FString> HandByArchetype;
	TMap<FName, FString> KindByArchetype;
	TSet<FName> UnambiguousArchetypes;
	TArray<TSharedPtr<FJsonValue>> ConcreteRows;
	int32 ConcreteCount = 0;
	int32 Joined = 0;
	int32 HandProven = 0;
	int32 KindProven = 0;
	int32 SemanticConflicts = 0;
	for (const FAssetData& Asset : Assets)
	{
		const UGridItemDefinitionAsset* Item =
			Cast<UGridItemDefinitionAsset>(Asset.GetAsset());
		if (!Item) { continue; } // Two catalogue assets are not concrete definitions.
		++ConcreteCount;
		if (Item->ItemDefinitionId.IsNone() || ConcreteIds.Contains(Item->ItemDefinitionId))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2E] Missing/duplicate real ItemDefinitionId."));
			return 4;
		}
		ConcreteIds.Add(Item->ItemDefinitionId);

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

		TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
		Row->SetStringField(TEXT("asset_package"), Asset.PackageName.ToString());
		Row->SetStringField(TEXT("item_id"), Item->ItemDefinitionId.ToString());
		Row->SetStringField(TEXT("source_archetype_id"), Item->WeaponArchetypeId.ToString());
		Row->SetBoolField(TEXT("b_use_weapon_catalogue"), Item->bUseWeaponCatalogue);
		Row->SetBoolField(TEXT("joined_to_source_n1"), bJoined);
		Row->SetNumberField(TEXT("item_type_enum"), static_cast<int32>(Item->ItemType));
		Row->SetNumberField(TEXT("effective_hand_usage_enum"),
			static_cast<int32>(Item->GetEffectiveHandUsage()));
		Row->SetBoolField(TEXT("can_equip_main"), Item->CanEquipToSlot(EGridEquipmentSlot::MainHand));
		Row->SetBoolField(TEXT("can_equip_off"), Item->CanEquipToSlot(EGridEquipmentSlot::OffHand));
		Row->SetStringField(TEXT("provable_hand_policy"), HandPolicy);
		Row->SetStringField(TEXT("provable_kind"), Kind);
		Row->SetStringField(TEXT("raw_catalogue_family"), bJoined ? SourceN1->Family : TEXT(""));
		Row->SetStringField(TEXT("raw_catalogue_hand_usage"), bJoined ? SourceN1->HandUsage : TEXT(""));
		ConcreteRows.Add(MakeShared<FJsonValueObject>(Row));
	}

	if (ConcreteCount != 24 || ConcreteIds.Num() != 24)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2E] Concrete source count changed; abort audit."));
		return 5;
	}
	TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.2E/semantic-census/read-only/v1"));
	Report->SetNumberField(TEXT("archetypes"), CatalogueIds.Num());
	Report->SetNumberField(TEXT("source_family_variants"), Families.Num());
	Report->SetNumberField(TEXT("source_hand_label_variants"), RawHandLabels.Num());
	Report->SetNumberField(TEXT("concrete_weapon_assets"), ConcreteCount);
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
		TEXT("[ARM-CLEAN06.2E] PASS read-only: 82 source ARMs, 24 concrete assets, %d joined, %d hand policies proven, %d armament kinds proven, %d conflicts; ZERO binary writes."),
		Joined, HandProven, KindProven, SemanticConflicts);
	return 0;
}

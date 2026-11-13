#include "ARMEMENT/GridArmClean06CatalogueProjectionCommandlet.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "UObject/Package.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	const TCHAR* SourceCataloguePath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue");

	bool SameNumericProfile(const FGridWeaponLevelProfile& Source,
		const FGridCanonicalResolvedWeapon& Canonical)
	{
		return Source.DamageType == Canonical.DamageType &&
			Source.PhysicalSubtype == Canonical.PhysicalSubtype &&
			Source.ScalingAttribute == Canonical.ScalingAttribute &&
			Source.AccuracyBonus == Canonical.AccuracyBonus &&
			Source.ActionPointCost == Canonical.ActionPointCost &&
			Source.RangeCells == Canonical.RangeCells &&
			Source.PhysicalArmor == Canonical.PhysicalArmor &&
			Source.MagicalArmor == Canonical.MagicalArmor &&
			Source.DamageDice.bEnabled == Canonical.DamageDice.bEnabled &&
			Source.DamageDice.DiceCount == Canonical.DamageDice.DiceCount &&
			Source.DamageDice.DiceSides == Canonical.DamageDice.DiceSides &&
			Source.DamageDice.FlatBonus == Canonical.DamageDice.FlatBonus;
	}

	FGridCanonicalWeaponArchetype ProjectNumericOnly(
		const FGridWeaponArchetypeProgression& Legacy)
	{
		const FGridWeaponLevelProfile& Base = Legacy.BaseProfile;
		FGridCanonicalWeaponArchetype Canonical;
		Canonical.ArchetypeId = Base.ArchetypeId;
		// TRANSIENT COMPARISON PLACEHOLDERS. These semantic fields are NOT
		// inferred from the N1 spreadsheet text and are NOT saved as assets.
		Canonical.Kind = Base.DamageDice.bEnabled ?
			EGridCanonicalArmamentKind::Weapon : EGridCanonicalArmamentKind::Shield;
		Canonical.HandPolicy = EGridCanonicalHandPolicy::MainHand;
		Canonical.DamageType = Base.DamageType;
		Canonical.PhysicalSubtype = Base.PhysicalSubtype;
		Canonical.ScalingAttribute = Base.ScalingAttribute;
		Canonical.AccuracyBonus = Base.AccuracyBonus;
		Canonical.ActionPointCost = Base.ActionPointCost;
		Canonical.RangeCells = Base.RangeCells;
		Canonical.DamageDiceN1 = Base.DamageDice;
		Canonical.PhysicalArmorN1 = Base.PhysicalArmor;
		Canonical.MagicalArmorN1 = Base.MagicalArmor;
		Canonical.DiceCountGrowth = Legacy.DiceCountGrowth;
		Canonical.DiceSidesGrowth = Legacy.DiceSidesGrowth;
		Canonical.FlatBonusGrowth = Legacy.FlatBonusGrowth;
		Canonical.PhysicalArmorGrowth = Legacy.PhysicalArmorGrowth;
		Canonical.MagicalArmorGrowth = Legacy.MagicalArmorGrowth;
		return Canonical;
	}
}

UGridArmClean06CatalogueProjectionCommandlet::UGridArmClean06CatalogueProjectionCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGridArmClean06CatalogueProjectionCommandlet::Main(const FString& Params)
{
	FString OutputPath;
	if (!FParse::Value(*Params, TEXT("Output="), OutputPath) || OutputPath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2C] Explicit -Output= required."));
		return 1;
	}
	UGridWeaponCatalogueAsset* Legacy =
		LoadObject<UGridWeaponCatalogueAsset>(nullptr, SourceCataloguePath);
	if (!IsValid(Legacy) || !Legacy->IsValidCatalogue() || !Legacy->IsValidClassAffinityData() ||
		Legacy->ArchetypeProgressions.Num() != 82 ||
		!Legacy->Profiles.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2C] Source is not the 82-record compact catalogue."));
		return 2;
	}
	// No NewObject ever reaches an asset package or SavePackage: this preview
	// is the sole transient model for strict numeric comparison.
	UGridCanonicalWeaponCatalogueAsset* Candidate =
		NewObject<UGridCanonicalWeaponCatalogueAsset>(GetTransientPackage());
	Candidate->Archetypes.Reserve(82);
	TSet<FName> Seen;
	TSet<FString> HandUsageValues;
	TSet<FString> Families;
	TArray<TSharedPtr<FJsonValue>> UnmappedRows;
	int32 ComparedProfiles = 0;
	int32 PassiveProfiles = 0;
	for (const FGridWeaponArchetypeProgression& LegacyRow : Legacy->ArchetypeProgressions)
	{
		const FGridWeaponLevelProfile& Base = LegacyRow.BaseProfile;
		if (Base.ArchetypeId.IsNone() || Seen.Contains(Base.ArchetypeId))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2C] Duplicate or unknown ARM identity."));
			return 3;
		}
		Seen.Add(Base.ArchetypeId);
		HandUsageValues.Add(Base.HandUsage);
		Families.Add(Base.Family);
		FGridCanonicalWeaponArchetype& Canonical =
			Candidate->Archetypes.Add_GetRef(ProjectNumericOnly(LegacyRow));
		TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
		Row->SetStringField(TEXT("archetype_id"), Base.ArchetypeId.ToString());
		Row->SetStringField(TEXT("source_family"), Base.Family);
		Row->SetStringField(TEXT("source_hand_usage"), Base.HandUsage);
		Row->SetStringField(TEXT("name_to_move_to_concrete_definition"), Base.DisplayName.ToString());
		Row->SetNumberField(TEXT("weight_kg_to_move_to_concrete_definition"), Base.WeightKg);
		Row->SetBoolField(TEXT("passive_kind_needs_authoritative_mapping"),
			!Base.DamageDice.bEnabled);
		Row->SetBoolField(TEXT("hand_policy_needs_authoritative_mapping"), true);
		Row->SetBoolField(TEXT("ammo_kind_needs_definition_join"), true);
		Row->SetBoolField(TEXT("action_presentation_needs_source_join"), true);
		UnmappedRows.Add(MakeShared<FJsonValueObject>(Row));
		for (int32 Level = 1; Level <= 20; ++Level)
		{
			const FGridWeaponLevelProfile* Actual = Legacy->FindProfile(Base.ArchetypeId, Level);
			FGridCanonicalResolvedWeapon Projected;
			if (!Actual || !Canonical.TryResolve(Level, Projected) ||
				!SameNumericProfile(*Actual, Projected))
			{
				UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2C] Numeric mismatch %s N%d."),
					*Base.ArchetypeId.ToString(), Level);
				return 4;
			}
			++ComparedProfiles;
			if (!Actual->DamageDice.bEnabled)
			{
				++PassiveProfiles;
			}
		}
	}
	TArray<FString> Errors;
	if (ComparedProfiles != 1640 || PassiveProfiles != 260 ||
		!Candidate->Validate(Errors))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2C] Invalid transient numeric projection."));
		return 5;
	}
	auto SortedStrings = [](const TSet<FString>& Values)
	{
		TArray<FString> Sorted = Values.Array();
		Sorted.Sort();
		TArray<TSharedPtr<FJsonValue>> Entries;
		for (const FString& Value : Sorted)
		{
			Entries.Add(MakeShared<FJsonValueString>(Value));
		}
		return Entries;
	};
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.2C/transient-numeric-only/v1"));
	Root->SetNumberField(TEXT("source_compact_archetypes"), Legacy->ArchetypeProgressions.Num());
	Root->SetNumberField(TEXT("compared_profiles"), ComparedProfiles);
	Root->SetNumberField(TEXT("passive_profiles"), PassiveProfiles);
	Root->SetNumberField(TEXT("numeric_mismatches"), 0);
	Root->SetNumberField(TEXT("source_class_affinity_rows"), Legacy->ClassAffinities.Num());
	Root->SetNumberField(TEXT("source_class_preferences"), Legacy->ClassPreferences.Num());
	Root->SetNumberField(TEXT("distinct_hand_usages"), HandUsageValues.Num());
	Root->SetNumberField(TEXT("distinct_families"), Families.Num());
	Root->SetArrayField(TEXT("source_hand_usage_values"), SortedStrings(HandUsageValues));
	Root->SetArrayField(TEXT("source_family_values"), SortedStrings(Families));
	Root->SetArrayField(TEXT("unmapped_semantics"), UnmappedRows);
	Root->SetBoolField(TEXT("authoring_or_binary_migration_authorized"), false);
	Root->SetBoolField(TEXT("any_binary_writes"), false);
	FString Json;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
	if (!FJsonSerializer::Serialize(Root, Writer) ||
		!IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutputPath), true) ||
		!FFileHelper::SaveStringToFile(Json, *OutputPath,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.2C] Failed writing private JSON."));
		return 6;
	}
	UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.2C] PASS 82 archetypes, 1640 numeric profiles, 260 passives. All semantic mapping pending; zero assets written."));
	return 0;
}

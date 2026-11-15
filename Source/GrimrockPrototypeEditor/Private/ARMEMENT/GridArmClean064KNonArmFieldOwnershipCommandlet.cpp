#include "ARMEMENT/GridArmClean064KNonArmFieldOwnershipCommandlet.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* CanonicalPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem");
	const FString LegacyRoot(TEXT("/Game/GrimrockPrototype/Core/DataAssets/"));

	TSharedPtr<FJsonObject> ReadJson(const FString& Path)
	{
		FString Text;
		TSharedPtr<FJsonObject> Object;
		if (!FFileHelper::LoadFileToString(Text, *Path) ||
			!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Object))
		{
			return nullptr;
		}
		return Object;
	}

	FString GetText(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		FString Value;
		if (Object.IsValid()) { Object->TryGetStringField(Field, Value); }
		return Value;
	}

	FString ExportText(const FProperty& Property, const UObject& Owner)
	{
		FString Result;
		Property.ExportTextItem_Direct(Result, Property.ContainerPtrToValuePtr<void>(&Owner),
			nullptr, const_cast<UObject*>(&Owner), PPF_None);
		return Result;
	}

	/** Explicit, conservative domains: a domain is NOT approval of an owner.
	 * The canonical weapon projectile catalogue must not be used for
	 * generic non-weapon throws. */
	FString DomainOf(const FString& Name)
	{
		if (Name == TEXT("ReadText")) { return TEXT("READING"); }
		if (Name == TEXT("LightEmitter")) { return TEXT("LIGHT"); }
		if (Name == TEXT("VisualEffects") ||
			Name == TEXT("bEnableWorldSparkle") ||
			Name.StartsWith(TEXT("WorldSparkle")))
		{
			return TEXT("PERSISTENT_VISUAL");
		}
		if (Name == TEXT("bUseItemWeightAsWorldPhysicsMass") ||
			Name == TEXT("WorldPhysicsInitialTiltDegrees"))
		{
			return TEXT("WORLD_CHAOS");
		}
		if (Name == TEXT("bCombatThrowWeapon") ||
			Name.StartsWith(TEXT("Throw")) ||
			Name == TEXT("ProjectileMeshForwardAxis"))
		{
			return TEXT("GENERIC_THROW");
		}
		if (Name == TEXT("CombatActions") ||
			Name == TEXT("bProvidesQuickItemCombatAction") ||
			Name == TEXT("QuickItemCombatAction") ||
			Name == TEXT("QuickItemActionIdOverride") ||
			Name == TEXT("bProvidesAttackPresentation") ||
			Name == TEXT("PlayerAttackPresentationProfile"))
		{
			return TEXT("ITEM_ACTION");
		}
		if (Name == TEXT("HandUsage") ||
			Name == TEXT("CompatibleEquipmentSlots") ||
			Name == TEXT("EquipmentStatBonus") ||
			Name == TEXT("EquipmentResistanceBonus") ||
			Name == TEXT("EquippedMesh"))
		{
			return TEXT("NONARM_EQUIPMENT");
		}
		if (Name == TEXT("bStackable") || Name == TEXT("MaxStackSize"))
		{
			return TEXT("NONARM_STACK");
		}
		if (Name == TEXT("bUseWeaponCatalogue") ||
			Name == TEXT("WeaponArchetypeId") ||
			Name == TEXT("RequiredAmmunitionItemDefinitionId") ||
			Name == TEXT("DefaultWeaponItemLevel") ||
			Name == TEXT("WeaponCatalogue") ||
			Name == TEXT("bUseWeaponVisualCatalogue") ||
			Name == TEXT("WeaponVisualCatalogue"))
		{
			return TEXT("LEGACY_ARMAMENT_CATALOGUE");
		}
		return TEXT("UNCLASSIFIED");
	}

	FString ReviewContract(const FString& Domain)
	{
		if (Domain == TEXT("READING")) {return TEXT("READABLE_TEXT_OWNER_AND_RUNTIME_PARITY");}
		if (Domain == TEXT("LIGHT")) {return TEXT("NONARM_LIGHT_EMITTER_AND_STATE_CONTRACT");}
		if (Domain == TEXT("PERSISTENT_VISUAL")) {return TEXT("NONARM_PERSISTENT_VISUAL_AND_NIAGARA_CONTRACT");}
		if (Domain == TEXT("WORLD_CHAOS")) {return TEXT("GENERIC_WORLD_PICKUP_MASS_AND_CHAOS_CONTRACT");}
		if (Domain == TEXT("GENERIC_THROW")) {return TEXT("GENERIC_NONARM_THROW_BALLISTICS_AND_VISUAL_CONTRACT");}
		if (Domain == TEXT("ITEM_ACTION")) {return TEXT("ACTION_CATALOGUE_BINDING_AND_EXECUTOR_PARITY");}
		if (Domain == TEXT("NONARM_EQUIPMENT")) {return TEXT("NONARM_EQUIPMENT_AND_RESISTANCE_PARITY");}
		if (Domain == TEXT("NONARM_STACK")) {return TEXT("NONCONSUMABLE_STACK_POLICY_CONTRACT");}
		if (Domain == TEXT("LEGACY_ARMAMENT_CATALOGUE")) {return TEXT("UNEXPECTED_WEAPON_LEGACY_FIELD_ON_NONARM");}
		return TEXT("UNKNOWN_FIELD_OWNER_REQUIRES_MANUAL_SPECIFICATION");
	}

	bool HasCompleteLegacyExports(
		const UGridItemDefinitionAsset& Legacy, const TSharedPtr<FJsonObject>& Saved)
	{
		if (!Saved.IsValid()) { return false; }
		int32 Count = 0;
		for (TFieldIterator<FProperty> Field(Legacy.GetClass()); Field; ++Field)
		{
			FString Historical;
			if (!Saved->TryGetStringField(Field->GetName(), Historical) ||
				Historical != ExportText(**Field, Legacy))
			{
				return false;
			}
			++Count;
		}
		return Count >= 10 && Count == Saved->Values.Num();
	}
}

UGridArmClean064KNonArmFieldOwnershipCommandlet::UGridArmClean064KNonArmFieldOwnershipCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGridArmClean064KNonArmFieldOwnershipCommandlet::Main(const FString& Params)
{
	FString ProjectionPath;
	FString IdentityPath;
	FString OutputPath;
	if (!FParse::Value(*Params, TEXT("Projection="), ProjectionPath) ||
		!FParse::Value(*Params, TEXT("Identity="), IdentityPath) ||
		!FParse::Value(*Params, TEXT("Output="), OutputPath) ||
		ProjectionPath.IsEmpty() || IdentityPath.IsEmpty() || OutputPath.IsEmpty() ||
		FPaths::IsSamePath(ProjectionPath, OutputPath) ||
		FPaths::IsSamePath(IdentityPath, OutputPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4K] Explicit distinct private paths required."));
		return 1;
	}
	const TSharedPtr<FJsonObject> Projection = ReadJson(ProjectionPath);
	const TSharedPtr<FJsonObject> Identity = ReadJson(IdentityPath);
	const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* OldSources = nullptr;
	if (!Projection.IsValid() || !Identity.IsValid() ||
		GetText(Projection, TEXT("schema")) != TEXT("ARM-CLEAN06.4J/nonarm-dryrun/v1") ||
		GetText(Identity, TEXT("schema")) != TEXT("ARM-CLEAN06.4I/distinct-identity-gap/v1") ||
		!Projection->TryGetArrayField(TEXT("candidate_details_private"), Items) ||
		!Identity->TryGetArrayField(TEXT("missing_canonical_id_details"), OldSources) ||
		!Items || !OldSources || Items->Num() != 11 || OldSources->Num() != 11 ||
		Projection->GetIntegerField(TEXT("unmapped_nondefault_property_occurrences")) != 50 ||
		Projection->GetIntegerField(TEXT("unique_missing_item_ids")) != 11 ||
		Projection->GetIntegerField(TEXT("missing_world_occurrences")) != 40 ||
		Projection->GetBoolField(TEXT("migration_authorized")) ||
		Identity->GetIntegerField(TEXT("unique_missing_item_ids")) != 11 ||
		Identity->GetIntegerField(TEXT("missing_canonical_occurrences")) != 40)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4K] Source reports do not match 06.4I/06.4J."));
		return 2;
	}
	UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr, CanonicalPath);
	if (!IsValid(Root) || Root->ItemDefinitions.Num() != 23)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4K] Real canonical root inventory changed."));
		return 3;
	}
	TSet<FName> Registered;
	for (const UGridCanonicalItemDefinitionAsset* Definition : Root->ItemDefinitions)
	{
		if (!IsValid(Definition) || Definition->ItemId.IsNone() ||
			Registered.Contains(Definition->ItemId)) {return 3;}
		Registered.Add(Definition->ItemId);
	}
	const bool bRootInitiallyDirty = Root->GetOutermost()->IsDirty();
	TMap<FName, TSharedPtr<FJsonObject>> OldById;
	for (const TSharedPtr<FJsonValue>& Raw : *OldSources)
	{
		const TSharedPtr<FJsonObject> Object = Raw.IsValid() ? Raw->AsObject() : nullptr;
		const FName Id(*GetText(Object, TEXT("item_id")));
		if (!Object.IsValid() || Id.IsNone() || OldById.Contains(Id)) {return 2;}
		OldById.Add(Id, Object);
	}
	TSet<FName> SeenIds;
	TMap<FString, int32> Domains;
	TSet<FString> DistinctFields;
	int32 TotalProperties = 0;
	int32 TotalOccurrences = 0;
	int32 VerifiedSources = 0;
	int32 ChangedDirty = 0;
	int32 UnknownItems = 0;
	TArray<TSharedPtr<FJsonValue>> PrivateRows;
	for (const TSharedPtr<FJsonValue>& Item : *Items)
	{
		const TSharedPtr<FJsonObject> Row = Item.IsValid() ? Item->AsObject() : nullptr;
		const FName Id(*GetText(Row, TEXT("item_id")));
		const TSharedPtr<FJsonObject>* OldRow = OldById.Find(Id);
		const FString Path = GetText(Row, TEXT("legacy_source"));
		const TArray<TSharedPtr<FJsonValue>>* Nondefault = nullptr;
		if (!Row.IsValid() || Id.IsNone() || SeenIds.Contains(Id) ||
			Registered.Contains(Id) || !OldRow ||
			!Row->TryGetArrayField(TEXT("nondefault_unmapped_legacy_fields"), Nondefault) ||
			!Nondefault || Nondefault->IsEmpty() ||
			!Row->GetBoolField(TEXT("common_fields_exact")) ||
			Row->GetBoolField(TEXT("authoring_approved")) ||
			Row->GetBoolField(TEXT("binary_writes_allowed")) ||
			Path != GetText(*OldRow, TEXT("representative_source")) ||
			!Path.StartsWith(LegacyRoot, ESearchCase::CaseSensitive) ||
			Path.StartsWith(TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/")) ||
			Path.Contains(TEXT("..")) || Path.Contains(TEXT("\\")) ||
			!Path.Contains(TEXT(".")) ||
			Row->GetIntegerField(TEXT("world_occurrences")) !=
				(*OldRow)->GetIntegerField(TEXT("world_occurrences")))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4K] Private item projection scope invalid."));
			return 4;
		}
		const UGridItemDefinitionAsset* Legacy =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *Path);
		const TSharedPtr<FJsonObject>* SavedProperties = nullptr;
		if (!IsValid(Legacy) || Legacy->ItemDefinitionId != Id ||
			!Legacy->WeaponArchetypeId.IsNone() ||
			!(*OldRow)->TryGetObjectField(TEXT("source_fields_exact"), SavedProperties) ||
			!SavedProperties || !HasCompleteLegacyExports(*Legacy, *SavedProperties))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4K] Real UE legacy UPROPERTY drift."));
			return 4;
		}
		SeenIds.Add(Id);
		++VerifiedSources;
		TotalOccurrences += Row->GetIntegerField(TEXT("world_occurrences"));
		const bool bDirty = Legacy->GetOutermost()->IsDirty();
		const UObject* Defaults = Legacy->GetClass()->GetDefaultObject();
		TSet<FString> SeenPropertyNames;
		TArray<TSharedPtr<FJsonValue>> Fields;
		bool bHasUnknown = false;
		for (const TSharedPtr<FJsonValue>& RawField : *Nondefault)
		{
			if (!RawField.IsValid() || RawField->Type != EJson::String) {return 4;}
			const FString Name = RawField->AsString();
			if (Name.IsEmpty() || SeenPropertyNames.Contains(Name)) {return 4;}
			SeenPropertyNames.Add(Name);
			const FProperty* Property = Legacy->GetClass()->FindPropertyByName(FName(*Name));
			if (!Property || Property->Identical_InContainer(Legacy, Defaults))
			{
				UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4K] Field is not active in current UE."));
				return 4;
			}
			const FString Domain = DomainOf(Name);
			Domains.FindOrAdd(Domain)++;
			DistinctFields.Add(Name);
			bHasUnknown |= Domain == TEXT("UNCLASSIFIED");
			TSharedRef<FJsonObject> Detail = MakeShared<FJsonObject>();
			Detail->SetStringField(TEXT("property"), Name);
			Detail->SetStringField(TEXT("source_value_exact"), ExportText(*Property, *Legacy));
			Detail->SetStringField(TEXT("domain"), Domain);
			Detail->SetStringField(TEXT("required_contract"), ReviewContract(Domain));
			Detail->SetStringField(TEXT("status"), TEXT("BLOCKED_REQUIRES_SEMANTIC_PARITY"));
			Fields.Add(MakeShared<FJsonValueObject>(Detail));
			++TotalProperties;
		}
		if (bHasUnknown) {++UnknownItems;}
		TSharedRef<FJsonObject> ItemDetail = MakeShared<FJsonObject>();
		ItemDetail->SetStringField(TEXT("item_id"), Id.ToString());
		ItemDetail->SetStringField(TEXT("source_asset"), Path);
		ItemDetail->SetStringField(TEXT("candidate_family"), GetText(Row, TEXT("candidate_family")));
		ItemDetail->SetNumberField(TEXT("affected_world_occurrences"),
			Row->GetIntegerField(TEXT("world_occurrences")));
		ItemDetail->SetNumberField(TEXT("active_unmapped_fields"), Fields.Num());
		ItemDetail->SetArrayField(TEXT("field_reviews"), Fields);
		ItemDetail->SetBoolField(TEXT("conversion_authorized"), false);
		PrivateRows.Add(MakeShared<FJsonValueObject>(ItemDetail));
		if (Legacy->GetOutermost()->IsDirty() != bDirty) {++ChangedDirty;}
	}
	if (VerifiedSources != 11 || SeenIds.Num() != 11 ||
		TotalProperties != 50 || TotalOccurrences != 40 ||
		ChangedDirty != 0 || Root->GetOutermost()->IsDirty() != bRootInitiallyDirty)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4K] 11 IDs / 50 fields not reconciled."));
		return 5;
	}
	TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4K/field-ownership/v1"));
	Report->SetNumberField(TEXT("verified_nonarm_item_ids"), VerifiedSources);
	Report->SetNumberField(TEXT("verified_world_occurrences"), TotalOccurrences);
	Report->SetNumberField(TEXT("active_unmapped_property_occurrences"), TotalProperties);
	Report->SetNumberField(TEXT("distinct_active_unmapped_property_names"), DistinctFields.Num());
	Report->SetNumberField(TEXT("item_ids_with_unknown_field_domains"), UnknownItems);
	Report->SetNumberField(TEXT("source_dirty_state_changes"), ChangedDirty);
	Report->SetNumberField(TEXT("field_semantic_parity_approved"), 0);
	Report->SetBoolField(TEXT("migration_authorized"), false);
	Report->SetBoolField(TEXT("savegame_files_opened"), false);
	TSharedRef<FJsonObject> GroupCounts = MakeShared<FJsonObject>();
	for (const TPair<FString, int32>& Group : Domains)
	{
		GroupCounts->SetNumberField(Group.Key, Group.Value);
	}
	Report->SetObjectField(TEXT("domain_counts"), GroupCounts);
	Report->SetArrayField(TEXT("private_item_ownership"), PrivateRows);
	FString Serialized;
	if (!FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Serialized)) ||
		!FFileHelper::SaveStringToFile(Serialized, *OutputPath,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4K] Private audit write failed."));
		return 6;
	}
	UE_LOG(LogTemp, Display,
		TEXT("[ARM-CLEAN06.4K] All 50 active legacy field occurrences assigned a REVIEW domain."));
	return 0;
}

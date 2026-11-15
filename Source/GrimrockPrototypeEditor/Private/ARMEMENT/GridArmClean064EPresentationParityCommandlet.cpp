#include "ARMEMENT/GridArmClean064EPresentationParityCommandlet.h"

#include "ARMEMENT/GridArmClean06PresentationProjection.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* RootPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem");
	const FString AuthorizedPrefix(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/"));

	FString ReadField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		FString Result;
		if (Object.IsValid())
		{
			Object->TryGetStringField(Field, Result);
		}
		return Result;
	}

	TSharedRef<FJsonObject> MakeResult(const FString& Status)
	{
		TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
		Row->SetStringField(TEXT("status"), Status);
		return Row;
	}

	FString ExportProperty(const FProperty& Property, const UObject& Source)
	{
		FString Exported;
		Property.ExportTextItem_Direct(Exported,
			Property.ContainerPtrToValuePtr<void>(&Source), nullptr,
			const_cast<UObject*>(&Source), PPF_None);
		return Exported;
	}

	/** Reflection compares full UE values, not their displayed text. In
	 * particular ordered sound/Niagara soft paths, pitch, all transforms,
	 * timing and the canonical extra channels all participate. */
	TArray<FString> ComparePresentation(const FGridCanonicalActionPresentation& Expected,
		const FGridCanonicalActionPresentation& Actual)
	{
		TArray<FString> Different;
		for (TFieldIterator<FProperty> Property(FGridCanonicalActionPresentation::StaticStruct());
			Property; ++Property)
		{
			if (!Property->Identical_InContainer(&Expected, &Actual))
			{
				Different.Add(Property->GetName());
			}
		}
		return Different;
	}
}

UGridArmClean064EPresentationParityCommandlet::UGridArmClean064EPresentationParityCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGridArmClean064EPresentationParityCommandlet::Main(const FString& Params)
{
	FString InputPath;
	FString OutputPath;
	if (!FParse::Value(*Params, TEXT("Input="), InputPath) || InputPath.IsEmpty() ||
		!FParse::Value(*Params, TEXT("Output="), OutputPath) || OutputPath.IsEmpty() ||
		FPaths::IsSamePath(InputPath, OutputPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4E] Explicit distinct -Input and -Output required."));
		return 1;
	}
	FString InputText;
	if (!FFileHelper::LoadFileToString(InputText, *InputPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4E] Private dry-run input missing."));
		return 2;
	}
	TSharedPtr<FJsonObject> Input;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(InputText), Input) ||
		!Input.IsValid() ||
		ReadField(Input, TEXT("schema")) != TEXT("ARM-CLEAN06.4D/live-dry-run/v1"))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4E] Private dry-run format invalid."));
		return 2;
	}
	const TArray<TSharedPtr<FJsonValue>>* Sources = nullptr;
	if (!Input->TryGetArrayField(TEXT("affected_assets"), Sources) ||
		!Sources || Sources->Num() != 4)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4E] Exactly four verified source rows required."));
		return 2;
	}

	UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr, RootPath);
	if (!IsValid(Root) || !IsValid(Root->ActionPresentationCatalogue))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4E] Canonical root or presentation catalogue missing."));
		return 3;
	}
	const bool bRootWasDirty = Root->GetOutermost()->IsDirty();
	const bool bPresentationsWereDirty =
		Root->ActionPresentationCatalogue->GetOutermost()->IsDirty();

	int32 CheckedProperties = 0;
	int32 CheckedLinks = 0;
	int32 EqualLinks = 0;
	int32 DifferentLinks = 0;
	int32 UnresolvedLinks = 0;
	int32 EqualSources = 0;
	int32 IncompleteSources = 0;
	int32 InvalidSources = 0;
	TSet<FString> SeenPackages;
	TArray<TSharedPtr<FJsonValue>> Rows;

	for (const TSharedPtr<FJsonValue>& SourceJson : *Sources)
	{
		const TSharedPtr<FJsonObject> SourceRecord = SourceJson.IsValid() ?
			SourceJson->AsObject() : nullptr;
		const FString PackageName = ReadField(SourceRecord, TEXT("package"));
		const TArray<TSharedPtr<FJsonValue>>* Properties = nullptr;
		if (!PackageName.StartsWith(AuthorizedPrefix, ESearchCase::CaseSensitive) ||
			SeenPackages.Contains(PackageName) ||
			!SourceRecord.IsValid() ||
			!SourceRecord->TryGetArrayField(TEXT("property_proposals"), Properties) ||
			!Properties || Properties->IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4E] Source package/field scope invalid."));
			return 4;
		}
		SeenPackages.Add(PackageName);
		TSharedRef<FJsonObject> Row = MakeResult(TEXT("UNRESOLVED"));
		Row->SetStringField(TEXT("package"), PackageName);
		const FString ObjectPath = PackageName + TEXT(".") +
			FPackageName::GetShortName(PackageName);
		const UGridItemDefinitionAsset* Item =
			LoadObject<UGridItemDefinitionAsset>(nullptr, *ObjectPath);
		if (!IsValid(Item))
		{
			Row->SetStringField(TEXT("status"), TEXT("NO_LEGACY_DEFINITION"));
			++InvalidSources;
			Rows.Add(MakeShared<FJsonValueObject>(Row));
			continue;
		}
		const bool bItemWasDirty = Item->GetOutermost()->IsDirty();
		for (const TSharedPtr<FJsonValue>& FieldJson : *Properties)
		{
			const TSharedPtr<FJsonObject> Field = FieldJson.IsValid() ?
				FieldJson->AsObject() : nullptr;
			const FString FieldName = ReadField(Field, TEXT("field"));
			const FString Destination = ReadField(Field, TEXT("candidate_destination"));
			const FString ExpectedValue = ReadField(Field, TEXT("current_export"));
			const FProperty* Authored = Item->GetClass()->FindPropertyByName(FName(*FieldName));
			if (FieldName.IsEmpty() ||
				Destination != TEXT("DA_ActionPresentationCatalogue") ||
				!Authored || ExportProperty(*Authored, *Item) != ExpectedValue)
			{
				UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4E] Source UPROPERTY diverged from 06.4D."));
				return 4;
			}
			++CheckedProperties;
		}

		const FGridCanonicalWeaponArchetype* Archetype =
			Root->WeaponCatalogue ?
			Root->WeaponCatalogue->Find(Item->WeaponArchetypeId) : nullptr;
		if (!Archetype || Item->ItemDefinitionId.IsNone() ||
			Item->WeaponArchetypeId.IsNone() ||
			(Item->bProvidesAttackPresentation &&
			 !Item->HasValidPlayerAttackPresentation()))
		{
			Row->SetStringField(TEXT("status"), TEXT("INVALID_SOURCE_OR_ARCHETYPE"));
			++InvalidSources;
			Rows.Add(MakeShared<FJsonValueObject>(Row));
			continue;
		}
		const FGridPlayerAttackPresentationProfile Historical =
			Item->bProvidesAttackPresentation ?
				Item->PlayerAttackPresentationProfile :
				FGridPlayerAttackPresentationProfile();

		TArray<TSharedPtr<FJsonValue>> ActionRows;
		int32 SourceEqual = 0;
		int32 SourceUnresolved = 0;
		int32 SourceDifferent = 0;
		TSet<FName> SeenActions;
		for (const FGridCombatActionDefinition& SourceAction : Item->CombatActions)
		{
			if (SourceAction.ResolutionProfile != EGridCombatActionResolutionProfile::Attack)
			{
				continue;
			}
			++CheckedLinks;
			TSharedRef<FJsonObject> ActionRow = MakeResult(TEXT("UNBOUND"));
			ActionRow->SetStringField(TEXT("action_id"), SourceAction.ActionId.ToString());
			const FGridItemActionBinding* Link = Archetype->Actions.FindByPredicate(
				[&SourceAction](const FGridItemActionBinding& Candidate)
				{
					return Candidate.ActionId == SourceAction.ActionId;
				});
			if (SourceAction.ActionId.IsNone() ||
				SeenActions.Contains(SourceAction.ActionId) || !Link ||
				Link->PresentationId.IsNone() || !Root->ActionCatalogue ||
				!Root->ActionCatalogue->Find(SourceAction.ActionId))
			{
				++SourceUnresolved;
				++UnresolvedLinks;
				ActionRows.Add(MakeShared<FJsonValueObject>(ActionRow));
				continue;
			}
			SeenActions.Add(SourceAction.ActionId);
			ActionRow->SetStringField(TEXT("presentation_id"), Link->PresentationId.ToString());
			const FGridCanonicalActionPresentation* Actual =
				Root->ActionPresentationCatalogue->Find(Link->PresentationId);
			FGridCanonicalActionPresentation Projected;
			if (!Actual ||
				!GridArmClean06PresentationProjection::TryProject(
					Link->PresentationId, Historical, Projected))
			{
				ActionRow->SetStringField(TEXT("status"), TEXT("NO_CANONICAL_PRESENTATION"));
				++SourceUnresolved;
				++UnresolvedLinks;
				ActionRows.Add(MakeShared<FJsonValueObject>(ActionRow));
				continue;
			}
			const TArray<FString> Differences = ComparePresentation(Projected, *Actual);
			if (Differences.IsEmpty())
			{
				ActionRow->SetStringField(TEXT("status"), TEXT("EQUAL_ALL_FIELDS"));
				++SourceEqual;
				++EqualLinks;
			}
			else
			{
				ActionRow->SetStringField(TEXT("status"), TEXT("DIFFERENT_FIELDS"));
				++SourceDifferent;
				++DifferentLinks;
				TArray<TSharedPtr<FJsonValue>> FieldNames;
				for (const FString& Name : Differences)
				{
					FieldNames.Add(MakeShared<FJsonValueString>(Name));
				}
				ActionRow->SetArrayField(TEXT("different_fields"), FieldNames);
			}
			ActionRows.Add(MakeShared<FJsonValueObject>(ActionRow));
		}
		Row->SetArrayField(TEXT("action_comparisons"), ActionRows);
		Row->SetNumberField(TEXT("source_equal"), SourceEqual);
		Row->SetNumberField(TEXT("source_unresolved"), SourceUnresolved);
		Row->SetNumberField(TEXT("source_different"), SourceDifferent);
		if (SourceEqual > 0 && SourceUnresolved == 0 && SourceDifferent == 0)
		{
			Row->SetStringField(TEXT("status"), TEXT("MATCHED_PRESENTATIONS"));
			++EqualSources;
		}
		else
		{
			Row->SetStringField(TEXT("status"), TEXT("REVIEW_REQUIRED"));
			++IncompleteSources;
		}
		if (Item->GetOutermost()->IsDirty() != bItemWasDirty)
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4E] Legacy asset dirty state changed."));
			return 5;
		}
		Rows.Add(MakeShared<FJsonValueObject>(Row));
	}
	if (CheckedProperties != 8 || Rows.Num() != 4 ||
		Root->GetOutermost()->IsDirty() != bRootWasDirty ||
		Root->ActionPresentationCatalogue->GetOutermost()->IsDirty() != bPresentationsWereDirty)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4E] Parity scope or binary dirtiness invalid."));
		return 5;
	}
	TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4E/presentation-parity/v1"));
	Report->SetNumberField(TEXT("verified_source_assets"), Rows.Num());
	Report->SetNumberField(TEXT("verified_changed_properties"), CheckedProperties);
	Report->SetNumberField(TEXT("checked_attack_bindings"), CheckedLinks);
	Report->SetNumberField(TEXT("equal_attack_bindings"), EqualLinks);
	Report->SetNumberField(TEXT("different_attack_bindings"), DifferentLinks);
	Report->SetNumberField(TEXT("unresolved_attack_bindings"), UnresolvedLinks);
	Report->SetNumberField(TEXT("fully_matching_assets"), EqualSources);
	Report->SetNumberField(TEXT("review_required_assets"), IncompleteSources);
	Report->SetNumberField(TEXT("invalid_source_assets"), InvalidSources);
	Report->SetBoolField(TEXT("migration_authorized"), false);
	Report->SetArrayField(TEXT("sources"), Rows);
	FString Json;
	if (!FJsonSerializer::Serialize(Report,
		TJsonWriterFactory<>::Create(&Json)) ||
		!FFileHelper::SaveStringToFile(Json, *OutputPath,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4E] Private report write failed."));
		return 6;
	}
	UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.4E] Read-only parity report complete."));
	return 0;
}

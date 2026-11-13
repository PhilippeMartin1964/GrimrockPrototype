#include "ARMEMENT/GridArmClean06BaselineCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/UnrealType.h"

namespace
{
	const FString DataRoot = TEXT("/Game/GrimrockPrototype/Core/DataAssets");
	const TCHAR* CataloguePath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue");

	TSharedRef<FJsonObject> ExportProperties(const UStruct* StructType, void* Instance, UObject* Owner)
	{
		TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
		for (TFieldIterator<FProperty> Property(StructType); Property; ++Property)
		{
			if (Property->HasAnyPropertyFlags(CPF_Transient | CPF_DuplicateTransient))
			{
				continue;
			}
			FString Value;
			Property->ExportTextItem_Direct(Value, Property->ContainerPtrToValuePtr<void>(Instance),
				nullptr, Owner, PPF_None);
			Result->SetStringField(Property->GetName(), Value);
		}
		return Result;
	}

	TArray<TSharedPtr<FJsonValue>> ExportNames(const TArray<FName>& Names)
	{
		TArray<FName> Sorted = Names;
		Sorted.Sort([](FName Left, FName Right) { return Left.ToString() < Right.ToString(); });
		TArray<TSharedPtr<FJsonValue>> Result;
		for (FName Name : Sorted)
		{
			Result.Add(MakeShared<FJsonValueString>(Name.ToString()));
		}
		return Result;
	}

	FString GetKind(const FString& Package)
	{
		if (Package.EndsWith(TEXT("/DA_WeaponCatalogue"))) { return TEXT("combat_catalogue"); }
		if (Package.EndsWith(TEXT("/DA_WeaponVisualCatalogue"))) { return TEXT("visual_catalogue"); }
		if (Package.StartsWith(DataRoot + TEXT("/Weapons/"))) { return TEXT("weapon"); }
		if (Package.StartsWith(DataRoot + TEXT("/Items/"))) { return TEXT("item"); }
		if (Package.StartsWith(DataRoot + TEXT("/GrimrockLevels/"))) { return TEXT("level"); }
		if (Package.StartsWith(DataRoot + TEXT("/GridDungeons/"))) { return TEXT("dungeon"); }
		return TEXT("palette");
	}
}

UGridArmClean06BaselineCommandlet::UGridArmClean06BaselineCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGridArmClean06BaselineCommandlet::Main(const FString& Params)
{
	FString OutputPath;
	if (!FParse::Value(*Params, TEXT("Output="), OutputPath) || OutputPath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.0] Explicit -Output= path is required."));
		return 1;
	}

	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		TEXT("AssetRegistry")).Get();
	Registry.SearchAllAssets(true);

	const TArray<FString> Roots = {
		DataRoot + TEXT("/Weapons"), DataRoot + TEXT("/Items"),
		DataRoot + TEXT("/GrimrockLevels"), DataRoot + TEXT("/GridDungeons")
	};

	TArray<FAssetData> Assets;
	for (const FString& Root : Roots)
	{
		Registry.GetAssetsByPath(FName(*Root), Assets, true);
	}
	TArray<FAssetData> ParentAssets;
	Registry.GetAssetsByPath(FName(*DataRoot), ParentAssets, false);
	for (const FAssetData& Asset : ParentAssets)
	{
		if (Asset.PackageName.ToString().EndsWith(TEXT("/DA_ObjectPalette_Default")))
		{
			Assets.Add(Asset);
		}
	}
	Assets.Sort([](const FAssetData& A, const FAssetData& B)
	{
		return A.PackageName.ToString() < B.PackageName.ToString();
	});

	TMap<FString, int32> Counts;
	TSet<FName> SeenPackages;
	TArray<TSharedPtr<FJsonValue>> AssetRecords;
	for (const FAssetData& Asset : Assets)
	{
		if (SeenPackages.Contains(Asset.PackageName))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.0] Duplicate package %s"), *Asset.PackageName.ToString());
			return 2;
		}
		SeenPackages.Add(Asset.PackageName);
		UObject* Object = Asset.GetAsset();
		if (!IsValid(Object))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.0] Unloadable asset %s"), *Asset.PackageName.ToString());
			return 2;
		}
		const FString Kind = GetKind(Asset.PackageName.ToString());
		Counts.FindOrAdd(Kind)++;
		TArray<FName> Dependencies;
		TArray<FName> Referencers;
		Registry.GetDependencies(Asset.PackageName, Dependencies,
			UE::AssetRegistry::EDependencyCategory::All, UE::AssetRegistry::FDependencyQuery());
		Registry.GetReferencers(Asset.PackageName, Referencers,
			UE::AssetRegistry::EDependencyCategory::All, UE::AssetRegistry::FDependencyQuery());

		TSharedRef<FJsonObject> Record = MakeShared<FJsonObject>();
		Record->SetStringField(TEXT("package"), Asset.PackageName.ToString());
		Record->SetStringField(TEXT("object"), Asset.GetSoftObjectPath().ToString());
		Record->SetStringField(TEXT("class"), Object->GetClass()->GetPathName());
		Record->SetStringField(TEXT("kind"), Kind);
		Record->SetObjectField(TEXT("properties"), ExportProperties(Object->GetClass(), Object, Object));
		Record->SetArrayField(TEXT("dependencies"), ExportNames(Dependencies));
		Record->SetArrayField(TEXT("referencers"), ExportNames(Referencers));
		AssetRecords.Add(MakeShared<FJsonValueObject>(Record));
	}

	UGridWeaponCatalogueAsset* Catalogue = LoadObject<UGridWeaponCatalogueAsset>(nullptr, CataloguePath);
	if (!IsValid(Catalogue) || !Catalogue->IsValidCatalogue())
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.0] Invalid or missing weapon catalogue."));
		return 3;
	}

	TArray<TSharedPtr<FJsonValue>> Profiles;
	int32 PassiveCount = 0;
	for (int32 Index = 1; Index <= 82; ++Index)
	{
		const FName ArchetypeId(*FString::Printf(TEXT("ARM%03d"), Index));
		for (int32 Level = 1; Level <= 20; ++Level)
		{
			const FGridWeaponLevelProfile* Profile = Catalogue->FindProfile(ArchetypeId, Level);
			if (!Profile || !Profile->IsValidProfile())
			{
				UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.0] Missing/invalid profile %s N%d"),
					*ArchetypeId.ToString(), Level);
				return 3;
			}
			if (!Profile->DamageDice.bEnabled) { ++PassiveCount; }
			Profiles.Add(MakeShared<FJsonValueObject>(
				ExportProperties(FGridWeaponLevelProfile::StaticStruct(),
					const_cast<FGridWeaponLevelProfile*>(Profile), nullptr)));
		}
	}

	TSharedRef<FJsonObject> CountObject = MakeShared<FJsonObject>();
	for (const TPair<FString, int32>& Count : Counts)
	{
		CountObject->SetNumberField(Count.Key, Count.Value);
	}
	CountObject->SetNumberField(TEXT("profiles"), Profiles.Num());
	CountObject->SetNumberField(TEXT("passive_profiles"), PassiveCount);
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.0/read-only/v1"));
	Root->SetObjectField(TEXT("counts"), CountObject);
	Root->SetArrayField(TEXT("assets"), AssetRecords);
	Root->SetArrayField(TEXT("expanded_profiles"), Profiles);

	FString Json;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
	if (!FJsonSerializer::Serialize(Root, Writer))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.0] JSON serialization failed."));
		return 4;
	}
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutputPath), true);
	if (!FFileHelper::SaveStringToFile(Json, *OutputPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.0] Unable to write audit JSON."));
		return 4;
	}
	const bool bComplete =
		Counts.FindRef(TEXT("weapon")) == 24 && Counts.FindRef(TEXT("item")) == 24 &&
		Counts.FindRef(TEXT("level")) == 4 && Counts.FindRef(TEXT("dungeon")) == 1 &&
		Counts.FindRef(TEXT("palette")) == 1 &&
		Counts.FindRef(TEXT("combat_catalogue")) == 1 &&
		Counts.FindRef(TEXT("visual_catalogue")) == 1 &&
		Profiles.Num() == 1640 && PassiveCount == 260;
	UE_LOG(LogTemp, Display,
		TEXT("[ARM-CLEAN06.0] %s weapons=%d items=%d levels=%d profiles=%d passives=%d total=%d"),
		bComplete ? TEXT("COMPLETE") : TEXT("INCOMPLETE"),
		Counts.FindRef(TEXT("weapon")), Counts.FindRef(TEXT("item")),
		Counts.FindRef(TEXT("level")), Profiles.Num(), PassiveCount, Assets.Num());
	return bComplete ? 0 : 5;
}

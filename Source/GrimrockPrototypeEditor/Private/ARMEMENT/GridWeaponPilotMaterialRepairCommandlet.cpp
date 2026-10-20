#include "ARMEMENT/GridWeaponPilotMaterialRepairCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"

namespace ARMEMENT0163MaterialRepair
{
	const TCHAR* const MeshFolder = TEXT("/Game/GrimrockPrototype/Meshes/Weapons");
	const TCHAR* const PilotNames[] =
	{
		TEXT("SM_Weapon_Dagger_A"),
		TEXT("SM_Weapon_ShortBow_A"),
		TEXT("SM_Weapon_RoundShield_A")
	};

	struct FMaterialRecipe
	{
		FString Name;
		FLinearColor BaseColor = FLinearColor::White;
		float Shininess = 25.f;
		bool bHasBaseColor = false;
	};

	bool IsSafeMaterialName(const FString& Name)
	{
		if (Name.IsEmpty() || Name.Len() > 64)
		{
			return false;
		}
		for (const TCHAR C : Name)
		{
			if (!FChar::IsAlnum(C) && C != TEXT('_'))
			{
				return false;
			}
		}
		return true;
	}

	bool ParseTriplet(const FString& Value, FLinearColor& OutColor)
	{
		TArray<FString> Parts;
		Value.ParseIntoArrayWS(Parts);
		if (Parts.Num() != 3 || !Parts[0].IsNumeric() || !Parts[1].IsNumeric() || !Parts[2].IsNumeric())
		{
			return false;
		}
		const float R = FCString::Atof(*Parts[0]);
		const float G = FCString::Atof(*Parts[1]);
		const float B = FCString::Atof(*Parts[2]);
		if (!FMath::IsFinite(R) || !FMath::IsFinite(G) || !FMath::IsFinite(B) ||
			R < 0.f || G < 0.f || B < 0.f || R > 1.f || G > 1.f || B > 1.f)
		{
			return false;
		}
		OutColor = FLinearColor(R, G, B);
		return true;
	}

	bool ParseMtl(const FString& Path, TMap<FString, FMaterialRecipe>& Recipes, FString& OutError)
	{
		FString Content;
		if (!FFileHelper::LoadFileToString(Content, *Path))
		{
			OutError = FString::Printf(TEXT("Cannot load source MTL: %s"), *Path);
			return false;
		}

		TArray<FString> Lines;
		Content.ParseIntoArrayLines(Lines);
		FMaterialRecipe Current;
		bool bHasMaterial = false;

		auto Commit = [&]() -> bool
		{
			if (!bHasMaterial)
			{
				return true;
			}
			if (!Current.bHasBaseColor)
			{
				OutError = FString::Printf(TEXT("Missing Kd base color for %s"), *Current.Name);
				return false;
			}
			if (const FMaterialRecipe* Existing = Recipes.Find(Current.Name))
			{
				if (!Existing->BaseColor.Equals(Current.BaseColor, KINDA_SMALL_NUMBER) ||
					!FMath::IsNearlyEqual(Existing->Shininess, Current.Shininess))
				{
					OutError = FString::Printf(
						TEXT("Conflicting MTL palette for shared material %s"), *Current.Name);
					return false;
				}
			}
			else
			{
				Recipes.Add(Current.Name, Current);
			}
			return true;
		};

		for (FString Line : Lines)
		{
			Line.TrimStartAndEndInline();
			if (Line.IsEmpty() || Line.StartsWith(TEXT("#")))
			{
				continue;
			}

			FString Key;
			FString Rest;
			if (!Line.Split(TEXT(" "), &Key, &Rest))
			{
				continue;
			}
			Rest.TrimStartAndEndInline();
			if (Key == TEXT("newmtl"))
			{
				if (!Commit())
				{
					return false;
				}
				if (!IsSafeMaterialName(Rest))
				{
					OutError = FString::Printf(TEXT("Unsafe MTL identifier: %s"), *Rest);
					return false;
				}
				Current = FMaterialRecipe();
				Current.Name = Rest;
				bHasMaterial = true;
			}
			else if (bHasMaterial && Key == TEXT("Kd"))
			{
				if (!ParseTriplet(Rest, Current.BaseColor))
				{
					OutError = FString::Printf(TEXT("Invalid Kd value for %s"), *Current.Name);
					return false;
				}
				Current.bHasBaseColor = true;
			}
			else if (bHasMaterial && Key == TEXT("Ns"))
			{
				if (!Rest.IsNumeric())
				{
					OutError = FString::Printf(TEXT("Invalid Ns for %s"), *Current.Name);
					return false;
				}
				Current.Shininess = FCString::Atof(*Rest);
				if (!FMath::IsFinite(Current.Shininess) || Current.Shininess < 0.f ||
					Current.Shininess > 1000.f)
				{
					OutError = FString::Printf(TEXT("Out-of-range Ns for %s"), *Current.Name);
					return false;
				}
			}
		}
		return Commit();
	}

	FString PackagePath(const FString& Name)
	{
		return FString::Printf(TEXT("%s/%s"), MeshFolder, *Name);
	}

	FString ObjectPath(const FString& Name)
	{
		return FString::Printf(TEXT("%s.%s"), *PackagePath(Name), *Name);
	}

	bool SaveAsset(UObject* Object)
	{
		if (!IsValid(Object))
		{
			return false;
		}
		UPackage* Package = Object->GetOutermost();
		const FString File = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(File), true);
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Object, *File, SaveArgs);
	}

	bool ConnectColor(UMaterial* Material, const FLinearColor& Color)
	{
		UMaterialExpressionConstant3Vector* Node = Cast<UMaterialExpressionConstant3Vector>(
			UMaterialEditingLibrary::CreateMaterialExpression(
				Material, UMaterialExpressionConstant3Vector::StaticClass(), -440, -180));
		if (!Node)
		{
			return false;
		}
		Node->Constant = Color;
		return UMaterialEditingLibrary::ConnectMaterialProperty(Node, TEXT(""), MP_BaseColor);
	}

	bool ConnectScalar(UMaterial* Material, EMaterialProperty Property, float Value, int32 Y)
	{
		UMaterialExpressionConstant* Node = Cast<UMaterialExpressionConstant>(
			UMaterialEditingLibrary::CreateMaterialExpression(
				Material, UMaterialExpressionConstant::StaticClass(), -440, Y));
		if (!Node)
		{
			return false;
		}
		Node->R = Value;
		return UMaterialEditingLibrary::ConnectMaterialProperty(Node, TEXT(""), Property);
	}

	UMaterialInterface* CreateOrFindMaterial(const FMaterialRecipe& Recipe)
	{
		const FString AssetPath = ObjectPath(Recipe.Name);
		const FString File = FPackageName::LongPackageNameToFilename(
			PackagePath(Recipe.Name), FPackageName::GetAssetPackageExtension());
		if (FPaths::FileExists(File))
		{
			// Do not overwrite an artist-authored material.
			UMaterialInterface* Existing = LoadObject<UMaterialInterface>(nullptr, *AssetPath);
			if (!IsValid(Existing))
			{
				UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3] Non-material asset occupies %s"), *AssetPath);
			}
			return Existing;
		}

		UPackage* Package = CreatePackage(*PackagePath(Recipe.Name));
		UMaterial* Material = NewObject<UMaterial>(
			Package, *Recipe.Name, RF_Public | RF_Standalone | RF_Transactional);
		if (!IsValid(Material))
		{
			return nullptr;
		}
		const bool bMetal = Recipe.Name.Contains(TEXT("Steel")) ||
			Recipe.Name.Contains(TEXT("Bronze")) ||
			Recipe.Name.Contains(TEXT("Iron")) ||
			Recipe.Name.Contains(TEXT("Brass"));
		const float Roughness = FMath::Clamp(
			1.f - FMath::Sqrt(Recipe.Shininess / 1000.f), 0.20f, 0.94f);

		if (!ConnectColor(Material, Recipe.BaseColor) ||
			!ConnectScalar(Material, MP_Metallic, bMetal ? 0.78f : 0.f, 10) ||
			!ConnectScalar(Material, MP_Roughness, Roughness, 190))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3] Unable to connect PBR nodes for %s"), *Recipe.Name);
			return nullptr;
		}
		Material->PostEditChange();
		FAssetRegistryModule::AssetCreated(Material);
		Package->MarkPackageDirty();
		if (!SaveAsset(Material))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3] Unable to save material %s"), *AssetPath);
			return nullptr;
		}
		UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.6.3] Created material %s"), *AssetPath);
		return Material;
	}
}

UGridWeaponPilotMaterialRepairCommandlet::UGridWeaponPilotMaterialRepairCommandlet()
{
	IsEditor = true;
	IsClient = false;
	IsServer = false;
	LogToConsole = true;
}

int32 UGridWeaponPilotMaterialRepairCommandlet::Main(const FString& Params)
{
	using namespace ARMEMENT0163MaterialRepair;
	FString SourceRoot;
	if (!FParse::Value(*Params, TEXT("SourceRoot="), SourceRoot) || SourceRoot.IsEmpty())
	{
		SourceRoot = FPaths::Combine(FPaths::ProjectDir(), TEXT("ArtSource/ARMEMENT01_6_3/Meshes"));
	}
	SourceRoot = FPaths::ConvertRelativePathToFull(SourceRoot);

	// Collect and verify the full palette BEFORE creating any package.
	TMap<FString, FMaterialRecipe> Recipes;
	for (const TCHAR* PilotName : PilotNames)
	{
		FString Error;
		const FString MtlFile = FPaths::Combine(SourceRoot, FString(PilotName) + TEXT(".mtl"));
		if (!ParseMtl(MtlFile, Recipes, Error))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3] %s"), *Error);
			return 1;
		}
	}
	if (Recipes.Num() != 14)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3] Expected 14 shared palette materials, found %d"),
			Recipes.Num());
		return 1;
	}

	// First save every material so loading the original meshes will not
	// produce missing-dependent-package warnings.
	TMap<FName, UMaterialInterface*> Materials;
	for (const TPair<FString, FMaterialRecipe>& Entry : Recipes)
	{
		UMaterialInterface* Material = CreateOrFindMaterial(Entry.Value);
		if (!IsValid(Material))
		{
			return 1;
		}
		Materials.Add(FName(*Entry.Key), Material);
	}

	for (const TCHAR* PilotName : PilotNames)
	{
		const FString MeshName(PilotName);
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath(MeshName));
		if (!IsValid(Mesh))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3] Missing pilot mesh %s"), PilotName);
			return 1;
		}
		const TArray<FStaticMaterial>& Slots = Mesh->GetStaticMaterials();
		if (Slots.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3] Mesh without material slots: %s"), PilotName);
			return 1;
		}

		int32 Corrected = 0;
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			// OBJ importer preserves MTL material identifiers in slot names.
			FName SlotName = Slots[Index].ImportedMaterialSlotName;
			if (SlotName.IsNone())
			{
				SlotName = Slots[Index].MaterialSlotName;
			}
			UMaterialInterface** Material = Materials.Find(SlotName);
			if (!Material)
			{
				UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3] Unknown MTL slot %s on %s"),
					*SlotName.ToString(), PilotName);
				return 1;
			}
			if (Slots[Index].MaterialInterface != *Material)
			{
				Mesh->SetMaterial(Index, *Material);
				++Corrected;
			}
		}
		if (Corrected > 0)
		{
			Mesh->MarkPackageDirty();
			if (!SaveAsset(Mesh))
			{
				UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3] Mesh save failed: %s"), PilotName);
				return 1;
			}
		}
		UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.6.3] %s: %d material slot(s), %d repaired."),
			PilotName, Slots.Num(), Corrected);
	}
	UE_LOG(LogTemp, Display,
		TEXT("[ARMEMENT01.6.3] All 14 material packages are saved and all three meshes use their own Material Slots."));
	return 0;
}

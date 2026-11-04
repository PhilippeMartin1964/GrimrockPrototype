#include "ARMEMENT/GridOilSurfaceMaterialAuthoringCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSphereMask.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace OilMaterialAuthoring
{
	const TCHAR* Folder = TEXT("/Game/GrimrockPrototype/Art/Materials/Surfaces");
	const TCHAR* MasterName = TEXT("M_Surface_Oil");
	const TCHAR* InstanceName = TEXT("MI_Surface_Oil");

	bool SaveAsset(UObject* Asset)
	{
		if (!IsValid(Asset) || !Asset->GetOutermost())
		{
			return false;
		}
		UPackage* Package = Asset->GetOutermost();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		Package->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Asset, *Filename, Args);
	}

	template <typename T>
	T* Add(UMaterial* Material, int32 X, int32 Y)
	{
		return Cast<T>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material, T::StaticClass(), X, Y));
	}

	bool Connect(UMaterialExpression* From, UMaterialExpression* To, const TCHAR* Pin)
	{
		return IsValid(From) && IsValid(To) &&
			UMaterialEditingLibrary::ConnectMaterialExpressions(From, TEXT(""), To, Pin);
	}

	bool ConnectOutput(UMaterialExpression* From, EMaterialProperty Property)
	{
		return IsValid(From) &&
			UMaterialEditingLibrary::ConnectMaterialProperty(From, TEXT(""), Property);
	}

	bool BuildMaster(UMaterial& Material)
	{
		Material.MaterialDomain = MD_Surface;
		Material.BlendMode = BLEND_Translucent;
		Material.TwoSided = true; // Le plan /Engine/BasicShapes/Plane est visible des deux côtés.

		auto* Tint = Add<UMaterialExpressionVectorParameter>(&Material, -700, -450);
		auto* Roughness = Add<UMaterialExpressionScalarParameter>(&Material, -700, -200);
		auto* Specular = Add<UMaterialExpressionScalarParameter>(&Material, -700, -50);
		auto* Opacity = Add<UMaterialExpressionScalarParameter>(&Material, -700, 100);
		auto* UV = Add<UMaterialExpressionTextureCoordinate>(&Material, -900, 320);
		auto* Center = Add<UMaterialExpressionConstant2Vector>(&Material, -900, 480);
		auto* Mask = Add<UMaterialExpressionSphereMask>(&Material, -520, 330);
		auto* Multiply = Add<UMaterialExpressionMultiply>(&Material, -240, 190);
		if (!Tint || !Roughness || !Specular || !Opacity ||
			!UV || !Center || !Mask || !Multiply)
		{
			return false;
		}

		Tint->ParameterName = TEXT("OilTint");
		Tint->Group = TEXT("Huile");
		Tint->DefaultValue = FLinearColor(0.09f, 0.055f, 0.024f, 1.f);

		Roughness->ParameterName = TEXT("OilRoughness");
		Roughness->Group = TEXT("Huile");
		Roughness->DefaultValue = 0.09f;

		Specular->ParameterName = TEXT("OilSpecular");
		Specular->Group = TEXT("Huile");
		Specular->DefaultValue = 0.8f;

		Opacity->ParameterName = TEXT("OilOpacity");
		Opacity->Group = TEXT("Huile");
		Opacity->DefaultValue = 0.63f;

		Center->R = .5f;
		Center->G = .5f;
		Mask->AttenuationRadius = .61f;
		Mask->HardnessPercent = 38.f;

		return ConnectOutput(Tint, MP_BaseColor) &&
			ConnectOutput(Roughness, MP_Roughness) &&
			ConnectOutput(Specular, MP_Specular) &&
			Connect(UV, Mask, TEXT("A")) &&
			Connect(Center, Mask, TEXT("B")) &&
			Connect(Mask, Multiply, TEXT("A")) &&
			Connect(Opacity, Multiply, TEXT("B")) &&
			ConnectOutput(Multiply, MP_Opacity);
	}
}

UGridOilSurfaceMaterialAuthoringCommandlet::UGridOilSurfaceMaterialAuthoringCommandlet()
{
	IsEditor = true;
	IsClient = false;
	IsServer = false;
	LogToConsole = true;
}

int32 UGridOilSurfaceMaterialAuthoringCommandlet::Main(const FString& Params)
{
	(void)Params;
	using namespace OilMaterialAuthoring;

	const FString MasterPackage = FString(Folder) / MasterName;
	const FString InstancePackage = FString(Folder) / InstanceName;
	const FString MasterPath = MasterPackage + TEXT(".") + MasterName;
	const FString InstancePath = InstancePackage + TEXT(".") + InstanceName;

	UMaterial* Master = LoadObject<UMaterial>(nullptr, *MasterPath);
	if (!IsValid(Master))
	{
		if (FPackageName::DoesPackageExist(MasterPackage))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.17.1.4] Asset maître existant incompatible."));
			return 1;
		}
		UPackage* Package = CreatePackage(*MasterPackage);
		Master = NewObject<UMaterial>(Package, FName(MasterName), RF_Public | RF_Standalone);
		if (!IsValid(Master) || !BuildMaster(*Master))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.17.1.4] Création du matériau impossible."));
			return 1;
		}
		FAssetRegistryModule::AssetCreated(Master);
		Master->PostEditChange();
		const TArray<FString> Errors = UMaterialEditingLibrary::RecompileMaterial(Master);
		if (!Errors.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.17.1.4] Erreurs du graphe de matériau : %d."), Errors.Num());
			return 1;
		}
		if (!SaveAsset(Master))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.17.1.4] Échec de sauvegarde du matériau."));
			return 1;
		}
	}

	UMaterialInstanceConstant* Instance = LoadObject<UMaterialInstanceConstant>(nullptr, *InstancePath);
	if (!IsValid(Instance))
	{
		if (FPackageName::DoesPackageExist(InstancePackage))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.17.1.4] Instance de matériau existante incompatible."));
			return 1;
		}
		UPackage* Package = CreatePackage(*InstancePackage);
		Instance = NewObject<UMaterialInstanceConstant>(
			Package, FName(InstanceName), RF_Public | RF_Standalone);
		if (!IsValid(Instance))
		{
			return 1;
		}
		UMaterialEditingLibrary::SetMaterialInstanceParent(Instance, Master);
		UMaterialEditingLibrary::UpdateMaterialInstance(Instance);
		FAssetRegistryModule::AssetCreated(Instance);
		if (!SaveAsset(Instance))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.17.1.4] Échec de sauvegarde de l'instance."));
			return 1;
		}
	}
	if (Instance->GetMaterial() != Master)
	{
		UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.17.1.4] Le matériau parent est inattendu."));
		return 1;
	}
	UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.17.1.4] M_Surface_Oil et MI_Surface_Oil prêts."));
	return 0;
}

#include "ARMEMENT/GridOilSurfaceMaterialAuthoringCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "MaterialEditingLibrary.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNoise.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSphereMask.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace OilDecalAuthoring
{
	const TCHAR* Folder = TEXT("/Game/GrimrockPrototype/Art/Materials/Surfaces");
	const TCHAR* MasterName = TEXT("M_Surface_Oil");
	const TCHAR* InstanceName = TEXT("MI_Surface_Oil");

	bool SaveAsset(UObject* Asset)
	{
		if (!IsValid(Asset) || !Asset->GetOutermost()) return false;
		UPackage* Package = Asset->GetOutermost();
		const FString Path = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
		Package->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Asset, *Path, Args);
	}

	template<typename T> T* Add(UMaterial* M, int32 X, int32 Y)
	{
		return Cast<T>(UMaterialEditingLibrary::CreateMaterialExpression(M, T::StaticClass(), X, Y));
	}
	bool Wire(UMaterialExpression* A, UMaterialExpression* B, const TCHAR* Input)
	{
		return IsValid(A) && IsValid(B) &&
			UMaterialEditingLibrary::ConnectMaterialExpressions(A, TEXT(""), B, Input);
	}
	bool Output(UMaterialExpression* A, EMaterialProperty Property)
	{
		return IsValid(A) && UMaterialEditingLibrary::ConnectMaterialProperty(A, TEXT(""), Property);
	}
	bool BuildMaster(UMaterial* M)
	{
		M->MaterialDomain = MD_DeferredDecal;
		M->BlendMode = BLEND_Translucent;
		M->TwoSided = false;
		// Le décalque modifie la pierre réellement rendue ; aucun mesh ISM flottant.
		auto* Tint = Add<UMaterialExpressionVectorParameter>(M, -850, -450);
		auto* Rough = Add<UMaterialExpressionScalarParameter>(M, -850, -230);
		auto* Opacity = Add<UMaterialExpressionScalarParameter>(M, -850, -90);
		auto* UV = Add<UMaterialExpressionTextureCoordinate>(M, -1150, 100);
		auto* Center = Add<UMaterialExpressionConstant2Vector>(M, -1100, 350);
		auto* Zero = Add<UMaterialExpressionConstant>(M, -1150, 550);
		auto* Position = Add<UMaterialExpressionAppendVector>(M, -870, 330);
		auto* Noise = Add<UMaterialExpressionNoise>(M, -680, 330);
		auto* Amplitude = Add<UMaterialExpressionConstant>(M, -680, 550);
		auto* Warp = Add<UMaterialExpressionMultiply>(M, -400, 360);
		auto* BaseRadius = Add<UMaterialExpressionConstant>(M, -400, 520);
		auto* Radius = Add<UMaterialExpressionAdd>(M, -190, 370);
		auto* Mask = Add<UMaterialExpressionSphereMask>(M, 50, 200);
		auto* Alpha = Add<UMaterialExpressionMultiply>(M, 300, 180);
		if (!Tint || !Rough || !Opacity || !UV || !Center || !Zero ||
			!Position || !Noise || !Amplitude || !Warp || !BaseRadius || !Radius || !Mask || !Alpha)
		{
			return false;
		}
		Tint->ParameterName = TEXT("OilTint");
		Tint->Group = TEXT("Huile");
		Tint->DefaultValue = FLinearColor(0.026f, 0.019f, 0.012f, 1.f);
		Rough->ParameterName = TEXT("OilRoughness");
		Rough->Group = TEXT("Huile");
		Rough->DefaultValue = .07f;
		Opacity->ParameterName = TEXT("OilOpacity");
		Opacity->Group = TEXT("Huile");
		Opacity->DefaultValue = .82f;
		Center->R = .5f;
		Center->G = .5f;
		Zero->R = 0.f;
		Amplitude->R = .06f;
		BaseRadius->R = .415f;
		Noise->Scale = 5.f;
		Noise->Levels = 2;
		Noise->Quality = 1;
		Noise->OutputMin = -1.f;
		Noise->OutputMax = 1.f;
		Mask->HardnessPercent = 35.f;
		return Wire(UV, Position, TEXT("A")) &&
			Wire(Zero, Position, TEXT("B")) &&
			Wire(Position, Noise, TEXT("Position")) &&
			Wire(Noise, Warp, TEXT("A")) &&
			Wire(Amplitude, Warp, TEXT("B")) &&
			Wire(Warp, Radius, TEXT("A")) &&
			Wire(BaseRadius, Radius, TEXT("B")) &&
			Wire(UV, Mask, TEXT("A")) &&
			Wire(Center, Mask, TEXT("B")) &&
			Wire(Radius, Mask, TEXT("Radius")) &&
			Wire(Mask, Alpha, TEXT("A")) &&
			Wire(Opacity, Alpha, TEXT("B")) &&
			Output(Tint, MP_BaseColor) &&
			Output(Rough, MP_Roughness) &&
			Output(Alpha, MP_Opacity);
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
	using namespace OilDecalAuthoring;
	const bool bConvert = FParse::Param(*Params, TEXT("ConvertToDecal"));
	const FString MPkg = FString(Folder) / MasterName;
	const FString IPkg = FString(Folder) / InstanceName;
	const FString MPath = MPkg + TEXT(".") + MasterName;
	const FString IPath = IPkg + TEXT(".") + InstanceName;
	UMaterial* Master = LoadObject<UMaterial>(nullptr, *MPath);
	if (bConvert)
	{
		// Refaire le seul matériau maître : l'instance existante garde son parent.
		if (!IsValid(Master) || !FPackageName::DoesPackageExist(IPkg))
		{
			UE_LOG(LogTemp, Error, TEXT("Matériaux d'huile existants requis."));
			return 1;
		}
		if (Master->MaterialDomain == MD_DeferredDecal)
		{
			UE_LOG(LogTemp, Display, TEXT("Matériau déjà converti en décalque."));
			return 0;
		}
		Master->Modify();
		UMaterialEditingLibrary::DeleteAllMaterialExpressions(Master);
		if (!BuildMaster(Master))
		{
			UE_LOG(LogTemp, Error, TEXT("Impossible de construire le décalque d'huile."));
			return 1;
		}
		Master->PostEditChange();
		UMaterialEditingLibrary::RecompileMaterial(Master);
		return SaveAsset(Master) ? 0 : 1;
	}
	if (!IsValid(Master))
	{
		if (FPackageName::DoesPackageExist(MPkg)) return 1;
		UPackage* Package = CreatePackage(*MPkg);
		Master = NewObject<UMaterial>(Package, FName(MasterName), RF_Public | RF_Standalone);
		if (!IsValid(Master) || !BuildMaster(Master)) return 1;
		FAssetRegistryModule::AssetCreated(Master);
		Master->PostEditChange();
		UMaterialEditingLibrary::RecompileMaterial(Master);
		if (!SaveAsset(Master)) return 1;
	}
	UMaterialInstanceConstant* Instance = LoadObject<UMaterialInstanceConstant>(nullptr, *IPath);
	if (!IsValid(Instance))
	{
		if (FPackageName::DoesPackageExist(IPkg)) return 1;
		UPackage* Package = CreatePackage(*IPkg);
		Instance = NewObject<UMaterialInstanceConstant>(Package,
			FName(InstanceName), RF_Public | RF_Standalone);
		if (!IsValid(Instance)) return 1;
		UMaterialEditingLibrary::SetMaterialInstanceParent(Instance, Master);
		UMaterialEditingLibrary::UpdateMaterialInstance(Instance);
		FAssetRegistryModule::AssetCreated(Instance);
		if (!SaveAsset(Instance)) return 1;
	}
	return Instance->GetMaterial() == Master ? 0 : 1;
}

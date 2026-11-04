#include "RPG/RPGSurfaceCatalystVisualAuthoring.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "MaterialEditingLibrary.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSphereMask.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/PackageName.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace SurfaceCatalystArt
{
	const TCHAR* ItemPath = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_SurfaceCatalyst_01.DA_Item_SurfaceCatalyst_01");
	const TCHAR* FlaskItemPath = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Oil.DA_Item_Flask_Oil");
	const TCHAR* Root = TEXT("/Game/GrimrockPrototype/Art/Materials/Catalyst");
	const TCHAR* MeshRoot = TEXT("/Game/GrimrockPrototype/Art/Meshes/Catalyst");
	const TCHAR* IconRoot = TEXT("/Game/GrimrockPrototype/Art/Textures/Catalyst");
	const TCHAR* Names[] = { TEXT("M_SurfaceCatalyst_Flask"), TEXT("M_SurfaceCatalyst_Ember"),
		TEXT("M_SurfaceCatalyst_RunePulse") };
	const TCHAR* MeshName = TEXT("SM_SurfaceCatalyst_01");
	const TCHAR* IconName = TEXT("T_UI_Item_SurfaceCatalyst_01");

	FString Pkg(const TCHAR* Folder,const TCHAR* Name) { return FString(Folder)/Name; }
	FString Path(const TCHAR* Folder,const TCHAR* Name) { return Pkg(Folder,Name)+TEXT(".")+Name; }

	bool Save(UObject* Asset)
	{
		if (!IsValid(Asset) || !IsValid(Asset->GetOutermost())) return false;
		UPackage* Package=Asset->GetOutermost();
		const FString Filename=FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename),true);
		Package->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags=RF_Public|RF_Standalone;
		Args.SaveFlags=SAVE_NoError;
		return UPackage::SavePackage(Package,Asset,*Filename,Args);
	}

	template<typename T>
	T* Add(UMaterial* Material,int32 X,int32 Y)
	{
		return Cast<T>(UMaterialEditingLibrary::CreateMaterialExpression(
			Material,T::StaticClass(),X,Y));
	}
	bool Wire(UMaterialExpression* A,UMaterialExpression* B,const TCHAR* Input)
	{
		return IsValid(A) && IsValid(B) &&
			UMaterialEditingLibrary::ConnectMaterialExpressions(A,TEXT(""),B,Input);
	}
	bool Output(UMaterialExpression* A, EMaterialProperty Property)
	{
		return IsValid(A) && UMaterialEditingLibrary::ConnectMaterialProperty(A,TEXT(""),Property);
	}

	bool BuildMaterial(UMaterial* Material,int32 Kind)
	{
		Material->MaterialDomain=Kind==2?MD_DeferredDecal:MD_Surface;
		Material->BlendMode=Kind==0?BLEND_Opaque:BLEND_Translucent;
		Material->TwoSided=Kind==1;
		auto* Color=Add<UMaterialExpressionVectorParameter>(Material,-800,-200);
		auto* Strength=Add<UMaterialExpressionScalarParameter>(Material,-800,70);
		auto* Emissive=Add<UMaterialExpressionMultiply>(Material,-480,-100);
		auto* Roughness=Add<UMaterialExpressionConstant>(Material,-480,180);
		if (!Color || !Strength || !Emissive || !Roughness) return false;
		Color->ParameterName=TEXT("CatalystGlowColor");
		Color->Group=TEXT("Catalyseur");
		Color->DefaultValue=Kind==0 ? FLinearColor(.65f,.22f,.045f) :
			FLinearColor(1.f,.37f,.025f);
		Strength->ParameterName=TEXT("CatalystGlowStrength");
		Strength->Group=TEXT("Catalyseur");
		Strength->DefaultValue=Kind==0?1.45f:(Kind==1?3.6f:1.2f);
		Roughness->R=Kind==0?.18f:.35f;
		if (!Wire(Color,Emissive,TEXT("A")) || !Wire(Strength,Emissive,TEXT("B")) ||
			!Output(Emissive,MP_EmissiveColor) || !Output(Color,MP_BaseColor) ||
			!Output(Roughness,MP_Roughness)) return false;
		if (Kind==0)
		{
			auto* Metallic=Add<UMaterialExpressionConstant>(Material,-450,360);
			if (!IsValid(Metallic)) return false;
			Metallic->R=.38f;
			return Output(Metallic,MP_Metallic);
		}
		auto* Opacity=Add<UMaterialExpressionScalarParameter>(Material,-800,310);
		if (!IsValid(Opacity)) return false;
		Opacity->ParameterName=TEXT("CatalystOpacity");
		Opacity->Group=TEXT("Catalyseur");
		Opacity->DefaultValue=Kind==1?.72f:.96f;
		if (Kind==1) return Output(Opacity,MP_Opacity);
		// An actual hollow rune-ring decal, not another full oil stain.
		auto* UV=Add<UMaterialExpressionTextureCoordinate>(Material,-1200,540);
		auto* Center=Add<UMaterialExpressionConstant2Vector>(Material,-1200,740);
		auto* OutRadius=Add<UMaterialExpressionConstant>(Material,-1200,900);
		auto* InRadius=Add<UMaterialExpressionConstant>(Material,-1200,1040);
		auto* Outer=Add<UMaterialExpressionSphereMask>(Material,-840,590);
		auto* Inner=Add<UMaterialExpressionSphereMask>(Material,-840,800);
		auto* Ring=Add<UMaterialExpressionSubtract>(Material,-530,650);
		auto* Alpha=Add<UMaterialExpressionMultiply>(Material,-250,540);
		if (!UV || !Center || !OutRadius || !InRadius || !Outer ||
			!Inner || !Ring || !Alpha) return false;
		Center->R=.5f; Center->G=.5f;
		OutRadius->R=.46f; InRadius->R=.365f;
		Outer->HardnessPercent=22.f;
		Inner->HardnessPercent=24.f;
		return Wire(UV,Outer,TEXT("A")) && Wire(Center,Outer,TEXT("B")) &&
			Wire(OutRadius,Outer,TEXT("Radius")) &&
			Wire(UV,Inner,TEXT("A")) && Wire(Center,Inner,TEXT("B")) &&
			Wire(InRadius,Inner,TEXT("Radius")) &&
			Wire(Outer,Ring,TEXT("A")) && Wire(Inner,Ring,TEXT("B")) &&
			Wire(Ring,Alpha,TEXT("A")) && Wire(Opacity,Alpha,TEXT("B")) &&
			Output(Alpha,MP_Opacity);
	}

	/** Antialiased RGBA vial: orange core, aged-bronze cage and central rune.
	 * 256px UI texture is self-contained; no copyrighted reference image is embedded.
	 */
	FColor Pixel(int32 Px,int32 Py)
	{
		const float X=(Px-127.5f)/127.5f;
		const float Y=(127.5f-Py)/127.5f;
		const float E=FMath::Sqrt(FMath::Square(X/.48f)+FMath::Square((Y+.27f)/.54f));
		const float Body=FMath::Clamp((1.f-E)*38.f+.48f,0.f,1.f);
		const float Neck=FMath::Clamp((.17f-FMath::Abs(X))*42.f,0.f,1.f) *
			FMath::Clamp((Y-.03f)*40.f,0.f,1.f)*FMath::Clamp((.81f-Y)*40.f,0.f,1.f);
		const float Cap=FMath::Clamp((.255f-FMath::Abs(X))*40.f,0.f,1.f) *
			FMath::Clamp((Y-.70f)*36.f,0.f,1.f)*FMath::Clamp((.89f-Y)*36.f,0.f,1.f);
		const float Glass=FMath::Max(Body,Neck);
		const float Sil=FMath::Max(Glass,Cap);
		const float Halo=FMath::Exp(-FMath::Max(0.f,E-1.f)*18.f) * .22f;
		const float Fuel=FMath::Clamp((.2f-Y)*4.f,0.f,1.f)*
			FMath::Clamp((.94f-E)*12.f,0.f,1.f)*Body;
		const float BronzeEdge=Body*(1.f-FMath::Clamp(FMath::Abs(E-.83f)*28.f,0.f,1.f));
		const float Cage=Body*FMath::Clamp((.047f-FMath::Abs(X-.36f*(1.f-FMath::Abs(Y+.25f))/.8f))*95.f,0.f,1.f);
		const float CageRight=Body*FMath::Clamp((.047f-FMath::Abs(X+.36f*(1.f-FMath::Abs(Y+.25f))/.8f))*95.f,0.f,1.f);
		const float CenterBand=Glass*FMath::Clamp((.03f-FMath::Abs(X))*90.f,0.f,1.f);
		const float RuneRadius=FMath::Sqrt(FMath::Square(X/.16f)+FMath::Square((Y+.35f)/.16f));
		const float Rune=Body*FMath::Clamp((.11f-FMath::Abs(RuneRadius-.74f))*25.f,0.f,1.f);
		const float Metal=FMath::Clamp(FMath::Max(FMath::Max(BronzeEdge,Cage),FMath::Max(CageRight,CenterBand)),0.f,1.f);
		const float RuneGlow=FMath::Max(Rune,CenterBand*.45f)*Body;
		const float A=FMath::Clamp(Sil+Halo*(1.f-Sil),0.f,1.f);
		const float Flame=(.64f+.36f*FMath::Sin(X*12.f+Y*19.f))*Fuel;
		FLinearColor C(.015f,.016f,.021f, A);
		C+=FLinearColor(.95f,.20f,.006f,0.f)*Flame;
		C+=FLinearColor(.20f,.09f,.036f,0.f)*Glass;
		C+=FLinearColor(.62f,.34f,.14f,0.f)*Metal;
		C+=FLinearColor(1.5f,.65f,.09f,0.f)*RuneGlow;
		C+=FLinearColor(.08f,.035f,.007f,0.f)*Halo;
		if (Cap>.5f) C=FMath::Lerp(C,FLinearColor(.24f,.16f,.10f,A),Cap*.86f);
		C.R=FMath::Clamp(C.R,0.f,1.f);
		C.G=FMath::Clamp(C.G,0.f,1.f);
		C.B=FMath::Clamp(C.B,0.f,1.f);
		return C.ToFColor(true);
	}
}

bool FRPGSurfaceCatalystVisualAuthoring::MatchesProductionAssets(FString* OutProblem)
{
	using namespace SurfaceCatalystArt;
	const UGridItemDefinitionAsset* Item=LoadObject<UGridItemDefinitionAsset>(nullptr,ItemPath);
	UTexture2D* Icon=LoadObject<UTexture2D>(nullptr,*Path(IconRoot,IconName));
	UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*Path(MeshRoot,MeshName));
	UMaterial* Flask=LoadObject<UMaterial>(nullptr,*Path(Root,Names[0]));
	UMaterial* Ember=LoadObject<UMaterial>(nullptr,*Path(Root,Names[1]));
	UMaterial* Ring=LoadObject<UMaterial>(nullptr,*Path(Root,Names[2]));
	const bool Valid=IsValid(Item)&&IsValid(Icon)&&IsValid(Mesh)&&IsValid(Flask)&&
		IsValid(Ember)&&IsValid(Ring)&&Item->IsValidDefinition() &&
		Item->Icon.ToSoftObjectPath().ToString()==Path(IconRoot,IconName) &&
		Item->WorldMesh.ToSoftObjectPath().ToString()==Path(MeshRoot,MeshName) &&
		Icon->GetSizeX()==256 && Icon->GetSizeY()==256 &&
		Mesh->GetStaticMaterials().Num()>0 && Mesh->GetMaterial(0)==Flask &&
		Flask->MaterialDomain==MD_Surface && Ember->MaterialDomain==MD_Surface &&
		Ring->MaterialDomain==MD_DeferredDecal &&
		Ember->BlendMode==BLEND_Translucent && Ring->BlendMode==BLEND_Translucent;
	if (OutProblem) *OutProblem=Valid?TEXT(""):TEXT("Native Catalyst visual contract is incomplete.");
	return Valid;
}

bool FRPGSurfaceCatalystVisualAuthoring::Run(bool bApply,FString& OutError)
{
	using namespace SurfaceCatalystArt;
	OutError.Reset();
	if (MatchesProductionAssets()) return true;
	UGridItemDefinitionAsset* Item=LoadObject<UGridItemDefinitionAsset>(nullptr,ItemPath);
	const UGridItemDefinitionAsset* Oil=LoadObject<UGridItemDefinitionAsset>(nullptr,FlaskItemPath);
	// The legacy oil DataAsset may have no world mesh. The approved starter
	// catalyst was authored with an oil-then-fire-bomb fallback; reuse the
	// exact already-resolved production mesh rather than assuming oil owns one.
	UStaticMesh* SourceMesh=IsValid(Item) ? Item->WorldMesh.LoadSynchronous() : nullptr;
	if (!IsValid(SourceMesh) && IsValid(Oil))
		SourceMesh=Oil->WorldMesh.LoadSynchronous();
	if (!IsValid(Item) || !IsValid(Oil) || !IsValid(SourceMesh))
	{
		OutError=TEXT("Missing approved item or original visual source.");
		return false;
	}
	for (const TCHAR* Name : Names)
	{
		if (FPackageName::DoesPackageExist(Pkg(Root,Name)))
		{
			OutError=TEXT("Unexpected partial material asset set; refuse overwrite.");
			return false;
		}
	}
	if (FPackageName::DoesPackageExist(Pkg(MeshRoot,MeshName)) ||
		FPackageName::DoesPackageExist(Pkg(IconRoot,IconName)))
	{
		OutError=TEXT("Unexpected partial mesh/icon set; refuse overwrite.");
		return false;
	}
	if (!bApply)
	{
		UE_LOG(LogTemp,Display,TEXT("[ARMEMENT01.17.2C] DRY_RUN READY"));
		return true;
	}
	UMaterial* BuiltMaterials[3]={};
	for (int32 I=0;I<3;++I)
	{
		UPackage* Package=CreatePackage(*Pkg(Root,Names[I]));
		UMaterial* M=Package ? NewObject<UMaterial>(Package,FName(Names[I]),RF_Public|RF_Standalone) : nullptr;
		if (!IsValid(M) || !BuildMaterial(M,I))
		{
			OutError=FString::Printf(TEXT("Material graph %d failed."),I);
			return false;
		}
		FAssetRegistryModule::AssetCreated(M);
		M->PostEditChange();
		UMaterialEditingLibrary::RecompileMaterial(M);
		if (!Save(M))
		{
			OutError=FString::Printf(TEXT("SaveMaterial %d failed."),I);
			return false;
		}
		BuiltMaterials[I]=M;
	}
	UPackage* MeshPackage=CreatePackage(*Pkg(MeshRoot,MeshName));
	UStaticMesh* Mesh=MeshPackage ? DuplicateObject<UStaticMesh>(SourceMesh,MeshPackage,FName(MeshName)) : nullptr;
	if (!IsValid(Mesh))
	{
		OutError=TEXT("DuplicateMesh failed.");
		return false;
	}
	Mesh->SetFlags(RF_Public|RF_Standalone);
	Mesh->SetMaterial(0,BuiltMaterials[0]);
	FAssetRegistryModule::AssetCreated(Mesh);
	Mesh->PostEditChange();
	if (!Save(Mesh))
	{
		OutError=TEXT("SaveMesh failed.");
		return false;
	}
	UPackage* TexturePackage=CreatePackage(*Pkg(IconRoot,IconName));
	UTexture2D* Icon=TexturePackage ? NewObject<UTexture2D>(TexturePackage,FName(IconName),
		RF_Public|RF_Standalone) : nullptr;
	if (!IsValid(Icon))
	{
		OutError=TEXT("CreateIcon failed.");
		return false;
	}
	TArray<FColor> Pixels;
	Pixels.SetNumUninitialized(256*256);
	for(int32 Y=0;Y<256;++Y)
		for(int32 X=0;X<256;++X)
			Pixels[Y*256+X]=Pixel(X,Y);
	Icon->Source.Init(256,256,1,1,TSF_BGRA8,(const uint8*)Pixels.GetData());
	Icon->SRGB=true;
	Icon->CompressionSettings=TC_Default;
	Icon->LODGroup=TEXTUREGROUP_UI;
	Icon->MipGenSettings=TMGS_NoMipmaps;
	Icon->NeverStream=true;
	FAssetRegistryModule::AssetCreated(Icon);
	Icon->PostEditChange();
	if (!Save(Icon))
	{
		OutError=TEXT("SaveIcon failed.");
		return false;
	}
	Item->Modify();
	Item->Icon=Icon;
	Item->WorldMesh=Mesh;
	FAssetRegistryModule::AssetCreated(Item);
	Item->MarkPackageDirty();
	if (!Save(Item) || !MatchesProductionAssets(&OutError))
	{
		if (OutError.IsEmpty()) OutError=TEXT("Final visual asset validation failed.");
		return false;
	}
	UE_LOG(LogTemp,Display,TEXT("[ARMEMENT01.17.2C] NATIVE_ASSETS_SAVED"));
	return true;
}

#include "RPG/RPGSurfaceCatalystVisualAuthoring.h"

#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Runtime/GridItemDefinitionAsset.h"

// 17.3D.4: read-only validation. The artist assets are NEVER generated here.
bool FRPGSurfaceCatalystVisualAuthoring::MatchesProductionAssets(FString* OutProblem)
{
	// ARMEMENT01.17.3A: the artist approved new mesh/icon names on d7b8fdc0.
	// These assets are authoritative: never regenerate the retired variants.
	const UGridItemDefinitionAsset* Item = LoadObject<UGridItemDefinitionAsset>(nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_SurfaceCatalyst_01.DA_Item_SurfaceCatalyst_01"));
	const TCHAR* MeshPath = TEXT("/Game/GrimrockPrototype/Meshes/Items/SM_Catalyst_01.SM_Catalyst_01");
	const TCHAR* IconPath = TEXT("/Game/GrimrockPrototype/UI/Icons/Items/T_Catalyst_Icon.T_Catalyst_Icon");
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
	UTexture2D* Icon = LoadObject<UTexture2D>(nullptr, IconPath);
	UMaterial* Ember = LoadObject<UMaterial>(nullptr,
		TEXT("/Game/GrimrockPrototype/Art/Materials/Catalyst/M_SurfaceCatalyst_Ember.M_SurfaceCatalyst_Ember"));
	UMaterial* Ring = LoadObject<UMaterial>(nullptr,
		TEXT("/Game/GrimrockPrototype/Art/Materials/Catalyst/M_SurfaceCatalyst_RunePulse.M_SurfaceCatalyst_RunePulse"));
	const bool bValid = IsValid(Item) && IsValid(Mesh) && IsValid(Icon) &&
		IsValid(Ember) && IsValid(Ring) && Item->IsValidDefinition() &&
		Item->WorldMesh.ToSoftObjectPath().ToString() == FString(MeshPath) &&
		Item->Icon.ToSoftObjectPath().ToString() == FString(IconPath) &&
		Mesh->GetStaticMaterials().Num() > 0 && IsValid(Mesh->GetMaterial(0)) &&
		Ember->MaterialDomain == MD_Surface && Ring->MaterialDomain == MD_DeferredDecal &&
		Ember->BlendMode == BLEND_Translucent && Ring->BlendMode == BLEND_Translucent;
	if (OutProblem)
	{
		*OutProblem = bValid ? TEXT("") :
			TEXT("Approved artist catalyst mesh/icon or reaction VFX materials missing.");
	}
	return bValid;
}


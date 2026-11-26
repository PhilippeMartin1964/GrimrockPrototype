#include "Runtime/Inspection/GridItemMeshPresentation.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Runtime/Items/GridItemCatalogues.h"

void FGridItemMeshPresentation::SetMesh(UStaticMeshComponent* MeshComponent, UStaticMesh* Mesh,
	const FGridWeaponFinish& Finish, TArray<TObjectPtr<UMaterialInstanceDynamic>>& FinishMaterials)
{
	if (!MeshComponent)
	{
		return;
	}
	ClearFinish(MeshComponent, FinishMaterials);
	MeshComponent->SetStaticMesh(Mesh);
	ApplyFinish(MeshComponent, Finish, FinishMaterials);
}

void FGridItemMeshPresentation::ClearFinish(UStaticMeshComponent* MeshComponent, TArray<TObjectPtr<UMaterialInstanceDynamic>>& FinishMaterials)
{
	for (int32 Slot = 0; MeshComponent && Slot < FinishMaterials.Num(); ++Slot)
	{
		if (FinishMaterials[Slot] && MeshComponent->GetMaterial(Slot) == FinishMaterials[Slot])
		{
			MeshComponent->SetMaterial(Slot, nullptr);
		}
	}
	FinishMaterials.Reset();
}

void FGridItemMeshPresentation::ApplyFinish(UStaticMeshComponent* MeshComponent, const FGridWeaponFinish& Finish,
	TArray<TObjectPtr<UMaterialInstanceDynamic>>& FinishMaterials)
{
	if (Finish.FinishId.IsNone() || (Finish.RoughnessScale == 1.0f && Finish.NormalScale == 1.0f && Finish.Tint == FLinearColor::White))
	{
		return;
	}

	static const FMaterialParameterInfo RoughnessMultiplier(TEXT("RoughnessMultiplier"));
	static const FMaterialParameterInfo RoughnessOverride(TEXT("RoughnessOverride"));
	static const FMaterialParameterInfo NormalStrength(TEXT("NormalStrength"));
	static const FMaterialParameterInfo BaseColorTint(TEXT("BaseColorTint"));
	FinishMaterials.SetNum(MeshComponent->GetNumMaterials());
	for (int32 Slot = 0; Slot < FinishMaterials.Num(); ++Slot)
	{
		// An explicit override owned by a Blueprint or another feature keeps its slot.
		if (MeshComponent->OverrideMaterials.IsValidIndex(Slot) && MeshComponent->OverrideMaterials[Slot])
		{
			continue;
		}
		const UMaterialInterface* Base = MeshComponent->GetMaterial(Slot);
		float RoughnessScale;
		float RoughnessAbsolute;
		float Normal;
		FLinearColor Tint;
		if (!Base || !Base->GetScalarParameterValue(RoughnessMultiplier, RoughnessScale) ||
			!Base->GetScalarParameterValue(RoughnessOverride, RoughnessAbsolute) || !Base->GetScalarParameterValue(NormalStrength, Normal) ||
			!Base->GetVectorParameterValue(BaseColorTint, Tint))
		{
			continue;
		}
		if (UMaterialInstanceDynamic* Material = MeshComponent->CreateDynamicMaterialInstance(Slot))
		{
			Material->SetScalarParameterValue(RoughnessMultiplier.Name, RoughnessScale * Finish.RoughnessScale);
			Material->SetScalarParameterValue(RoughnessOverride.Name, RoughnessAbsolute * Finish.RoughnessScale);
			Material->SetScalarParameterValue(NormalStrength.Name, Normal * Finish.NormalScale);
			Material->SetVectorParameterValue(BaseColorTint.Name, Tint * Finish.Tint);
			FinishMaterials[Slot] = Material;
		}
	}
}

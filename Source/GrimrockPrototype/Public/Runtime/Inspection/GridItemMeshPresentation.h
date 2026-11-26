#pragma once

#include "CoreMinimal.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
struct FGridWeaponFinish;

/** One implementation of the existing finish application, shared by world and preview.
 * The owner retains its own transient MID array; explicit material overrides are preserved.
 */
struct GRIMROCKPROTOTYPE_API FGridItemMeshPresentation
{
	static void SetMesh(UStaticMeshComponent* MeshComponent, UStaticMesh* Mesh, const FGridWeaponFinish& Finish,
		TArray<TObjectPtr<UMaterialInstanceDynamic>>& FinishMaterials);
	static void ClearFinish(UStaticMeshComponent* MeshComponent, TArray<TObjectPtr<UMaterialInstanceDynamic>>& FinishMaterials);

private:
	static void ApplyFinish(UStaticMeshComponent* MeshComponent, const FGridWeaponFinish& Finish,
		TArray<TObjectPtr<UMaterialInstanceDynamic>>& FinishMaterials);
};

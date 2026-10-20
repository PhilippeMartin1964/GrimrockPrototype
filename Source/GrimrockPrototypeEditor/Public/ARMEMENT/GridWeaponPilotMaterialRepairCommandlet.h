#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridWeaponPilotMaterialRepairCommandlet.generated.h"

/**
 * ARMEMENT01.6.3 material repair for previously imported pilot OBJ meshes.
 * Creates missing assets from the source MTL palettes and corrects mesh
 * material slots, without ever reimporting or replacing the pilot meshes.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridWeaponPilotMaterialRepairCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGridWeaponPilotMaterialRepairCommandlet();
	virtual int32 Main(const FString& Params) override;
};

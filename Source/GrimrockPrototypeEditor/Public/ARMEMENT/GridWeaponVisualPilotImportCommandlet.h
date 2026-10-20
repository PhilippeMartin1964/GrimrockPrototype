#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridWeaponVisualPilotImportCommandlet.generated.h"

/** ARMEMENT01.6.3: import three pilot textures/OBJ meshes, then build the visual DataAsset. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridWeaponVisualPilotImportCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGridWeaponVisualPilotImportCommandlet();
	virtual int32 Main(const FString& Params) override;
};

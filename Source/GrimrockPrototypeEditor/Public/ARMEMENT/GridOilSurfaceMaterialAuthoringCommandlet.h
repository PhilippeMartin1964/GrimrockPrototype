#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridOilSurfaceMaterialAuthoringCommandlet.generated.h"

/** ARMEMENT01.17.1.4 : matérialise les deux assets de rendu d'huile. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridOilSurfaceMaterialAuthoringCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGridOilSurfaceMaterialAuthoringCommandlet();
	virtual int32 Main(const FString& Params) override;
};

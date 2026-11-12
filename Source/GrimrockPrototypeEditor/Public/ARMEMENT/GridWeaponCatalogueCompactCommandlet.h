#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridWeaponCatalogueCompactCommandlet.generated.h"

/** Convert the full weapon catalogue to lossless compact N1 growth rules. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridWeaponCatalogueCompactCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGridWeaponCatalogueCompactCommandlet();
	virtual int32 Main(const FString& Params) override;
};

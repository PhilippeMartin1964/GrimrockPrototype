#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridWeaponCatalogueImportCommandlet.generated.h"

/** ARMEMENT01.4 - materialize a single validated cooked asset from normalized XLSX export. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridWeaponCatalogueImportCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UGridWeaponCatalogueImportCommandlet();
	virtual int32 Main(const FString& Params) override;
};

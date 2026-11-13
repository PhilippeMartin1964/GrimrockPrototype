#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean06CatalogueProjectionCommandlet.generated.h"

/** ARM-CLEAN06.2C: native, read-only, transient catalogue parity proof. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean06CatalogueProjectionCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGridArmClean06CatalogueProjectionCommandlet();
	virtual int32 Main(const FString& Params) override;
};

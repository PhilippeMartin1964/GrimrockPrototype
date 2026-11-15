#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064KNonArmFieldOwnershipCommandlet.generated.h"

/** ARM-CLEAN06.4K: no SavePackage, no canonical authoring. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064KNonArmFieldOwnershipCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGridArmClean064KNonArmFieldOwnershipCommandlet();
	virtual int32 Main(const FString& Params) override;
};

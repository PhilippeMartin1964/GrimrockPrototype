#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean06HandSemanticsCommandlet.generated.h"

/**
 * ARM-CLEAN06.2E: read-only inspection of source hand rules and actual
 * UGridItemDefinitionAsset instances. Never authors or saves a package.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean06HandSemanticsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGridArmClean06HandSemanticsCommandlet();
	virtual int32 Main(const FString& Params) override;
};

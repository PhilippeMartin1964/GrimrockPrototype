#pragma once

#include "Commandlets/Commandlet.h"
#include "GridWorldObjectMIG08Commandlet.generated.h"

/**
 * WORLDOBJ-MIG08 editor commandlet.
 *
 * Dry-run is the default. Pass -Apply to persist converted .uasset packages.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridWorldObjectMIG08Commandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGridWorldObjectMIG08Commandlet();
	virtual int32 Main(const FString& Params) override;
};

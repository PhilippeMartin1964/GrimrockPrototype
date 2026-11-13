#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean06CanonicalStageCommandlet.generated.h"

/**
 * One-shot Editor-only ARM-CLEAN06 binary staging.
 * No package write without explicit -Apply and complete source parity.
 * Remove this conversion commandlet after the verified 06.4/06.5 cutover.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean06CanonicalStageCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGridArmClean06CanonicalStageCommandlet();
	virtual int32 Main(const FString& Params) override;
};

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064NNonArmGenericThrowCommandlet.generated.h"

/** ARM-CLEAN06.4N real UE generic (non-weapon) throw-data audit, no writes. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064NNonArmGenericThrowCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064NNonArmGenericThrowCommandlet();
    virtual int32 Main(const FString& Params) override;
};

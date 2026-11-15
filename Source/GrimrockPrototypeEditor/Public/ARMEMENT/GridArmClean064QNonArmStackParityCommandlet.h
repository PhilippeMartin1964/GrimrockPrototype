#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064QNonArmStackParityCommandlet.generated.h"

/** 06.4Q: read-only real UE stack policy parity for non-ARM source assets. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064QNonArmStackParityCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGridArmClean064QNonArmStackParityCommandlet();
    virtual int32 Main(const FString& Params) override;
};

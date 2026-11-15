#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064RNonArmFullActionBridgeCommandlet.generated.h"

/** One-shot Editor-only lossless non-ARM action/presentation staging audit.
 * Does not write packages, mutate runtime or authorize a binary migration. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064RNonArmFullActionBridgeCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064RNonArmFullActionBridgeCommandlet();
    virtual int32 Main(const FString& Params) override;
};

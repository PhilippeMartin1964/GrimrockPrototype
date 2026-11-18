#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064X60NativeStoneLootCommandlet.generated.h"

/** X.60 protected Stone-only native loot transaction. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064X60NativeStoneLootCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064X60NativeStoneLootCommandlet();
    virtual int32 Main(const FString& Params) override;
};

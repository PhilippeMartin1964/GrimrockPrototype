#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064X58NativeInitialContentCommandlet.generated.h"

/** X.58 initial-content-only transaction, two explicit native levels.
 * Preflight/Verify read only. Apply is operator-only, through the runner,
 * with a private byte-equal snapshot of both files and an exact SHA receipt.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064X58NativeInitialContentCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064X58NativeInitialContentCommandlet();
    virtual int32 Main(const FString& Params) override;
};

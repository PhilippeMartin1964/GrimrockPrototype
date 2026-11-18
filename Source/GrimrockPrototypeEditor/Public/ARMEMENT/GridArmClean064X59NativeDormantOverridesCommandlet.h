#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064X59NativeDormantOverridesCommandlet.generated.h"

/** X.59 dormant-override-only transaction in level 00.
 * Read-only by default; Apply requires private byte-equal backup and exact SHA.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064X59NativeDormantOverridesCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064X59NativeDormantOverridesCommandlet();
    virtual int32 Main(const FString& Params) override;
};

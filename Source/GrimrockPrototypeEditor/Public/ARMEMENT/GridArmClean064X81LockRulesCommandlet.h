#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064X81LockRulesCommandlet.generated.h"

/** ARM-CLEAN06.4X.16.8.1: exact two-lock native migration commandlet.
 * Automation tests run -Audit only. -Apply is guarded by a later isolated
 * Git LFS/SHA-256 transaction; never run it from this milestone.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064X81LockRulesCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064X81LockRulesCommandlet();
    virtual int32 Main(const FString& Params) override;
};

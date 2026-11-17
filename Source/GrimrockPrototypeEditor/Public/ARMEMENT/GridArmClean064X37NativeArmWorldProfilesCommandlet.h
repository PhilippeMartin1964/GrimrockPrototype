#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064X37NativeArmWorldProfilesCommandlet.generated.h"

/** Native, one-shot migration of persistent world presentation for 22 ARM.
 * -Preflight and -Verify are read-only; -Apply requires a verified backup
 * manifest generated outside the repository by the gated Windows runner. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064X37NativeArmWorldProfilesCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064X37NativeArmWorldProfilesCommandlet();
    virtual int32 Main(const FString& Params) override;
};

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064ONonArmActionSemanticsCommandlet.generated.h"

/** Read-only real UE audit of active non-arm action payloads; no conversion. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064ONonArmActionSemanticsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064ONonArmActionSemanticsCommandlet();
    virtual int32 Main(const FString& Params) override;
};

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064TNonArmMaterializeCommandlet.generated.h"

/** One-shot native UE materialization of ACTION-FREE non-ARM item definitions.
 * Editor-only legacy reader, never a runtime fallback or gameplay switch. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064TNonArmMaterializeCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064TNonArmMaterializeCommandlet();
    virtual int32 Main(const FString& Params) override;
};

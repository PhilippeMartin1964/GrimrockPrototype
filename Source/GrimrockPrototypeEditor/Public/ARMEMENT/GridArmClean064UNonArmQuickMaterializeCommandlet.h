#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064UNonArmQuickMaterializeCommandlet.generated.h"

/** Editor-only, single-use conversion of proven non-ARM QuickItem actions.
 * No historical definition lookup is added to the runtime. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064UNonArmQuickMaterializeCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064UNonArmQuickMaterializeCommandlet();
    virtual int32 Main(const FString& Params) override;
};

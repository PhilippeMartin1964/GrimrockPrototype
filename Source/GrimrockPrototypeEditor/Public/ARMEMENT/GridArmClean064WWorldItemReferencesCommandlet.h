#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064WWorldItemReferencesCommandlet.generated.h"

/** One-shot real UE authoring of canonical IDs for existing world items.
 * The original source pointers are never used as a runtime fallback here. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064WWorldItemReferencesCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064WWorldItemReferencesCommandlet();
    virtual int32 Main(const FString& Params) override;
};

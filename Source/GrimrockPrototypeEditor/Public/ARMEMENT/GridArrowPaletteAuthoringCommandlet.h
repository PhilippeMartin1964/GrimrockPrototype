#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArrowPaletteAuthoringCommandlet.generated.h"

/** LOT02.13.2.2: register the existing arrow stack as a direct collectible. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArrowPaletteAuthoringCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArrowPaletteAuthoringCommandlet();
    virtual int32 Main(const FString& Params) override;
};

#pragma once
#include "Commandlets/Commandlet.h"
#include "GridArmClean065FIconPaletteCommandlet.generated.h"

UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean065FIconPaletteCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean065FIconPaletteCommandlet();
    virtual int32 Main(const FString& Params) override;
};

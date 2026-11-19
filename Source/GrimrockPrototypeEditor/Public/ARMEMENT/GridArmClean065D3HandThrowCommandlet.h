#pragma once
#include "Commandlets/Commandlet.h"
#include "GridArmClean065D3HandThrowCommandlet.generated.h"

UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean065D3HandThrowCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean065D3HandThrowCommandlet();
    virtual int32 Main(const FString& Params) override;
};

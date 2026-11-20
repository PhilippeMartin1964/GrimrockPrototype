#pragma once
#include "Commandlets/Commandlet.h"
#include "GridArmClean065G71PuzzleCategoryCommandlet.generated.h"

UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean065G71PuzzleCategoryCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean065G71PuzzleCategoryCommandlet();
    virtual int32 Main(const FString& Params) override;
};

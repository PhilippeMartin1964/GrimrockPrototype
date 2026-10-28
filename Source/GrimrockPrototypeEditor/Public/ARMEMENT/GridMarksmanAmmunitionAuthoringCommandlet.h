#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridMarksmanAmmunitionAuthoringCommandlet.generated.h"

/** LOT02.5: create the starter arrow stack definition and configure ARM045. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridMarksmanAmmunitionAuthoringCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridMarksmanAmmunitionAuthoringCommandlet();
    virtual int32 Main(const FString& Params) override;
};

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064PNonArmEquipmentParityCommandlet.generated.h"

/** Editor-only, read-only real UE validation of non-armament equipment data. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064PNonArmEquipmentParityCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGridArmClean064PNonArmEquipmentParityCommandlet();
    virtual int32 Main(const FString& Params) override;
};

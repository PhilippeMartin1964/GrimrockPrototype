#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064VNonArmEquipmentMaterializeCommandlet.generated.h"

/** Editor-only, one-shot materialization of last two non-ARM definitions.
 * Reads historical sources ONLY during binary authoring/verification. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064VNonArmEquipmentMaterializeCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064VNonArmEquipmentMaterializeCommandlet();
    virtual int32 Main(const FString& Params) override;
};

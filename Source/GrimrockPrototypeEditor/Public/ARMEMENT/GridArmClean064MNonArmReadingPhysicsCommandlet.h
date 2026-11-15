#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064MNonArmReadingPhysicsCommandlet.generated.h"

/** Real UE read-only verification of non-ARM reading and world pickup policies. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064MNonArmReadingPhysicsCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGridArmClean064MNonArmReadingPhysicsCommandlet();
    virtual int32 Main(const FString& Params) override;
};

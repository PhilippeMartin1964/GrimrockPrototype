#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridWarriorN1ItemAuthoringCommandlet.generated.h"

/**
 * ARMEMENT01.10.2 - Safely author one Warrior N1 item at a time.
 * Default invocation only audits all four candidates.
 * -Apply requires -Archetype=ARM014|ARM015|ARM025|ARM031.
 * Existing definitions or missing real art block writes.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridWarriorN1ItemAuthoringCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGridWarriorN1ItemAuthoringCommandlet();
    virtual int32 Main(const FString& Params) override;
};

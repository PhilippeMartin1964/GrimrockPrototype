#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridAssassinDaggerN1ItemAuthoringCommandlet.generated.h"

/**
 * ARMEMENT01.11.2.2 - Safely author one Assassin Dagger N1 item at a time.
 * Default invocation only audits the single ARM002 candidate.
 * -Apply requires -Archetype=ARM002.
 * Existing definitions or missing real art block writes.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridAssassinDaggerN1ItemAuthoringCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGridAssassinDaggerN1ItemAuthoringCommandlet();
    virtual int32 Main(const FString& Params) override;
};

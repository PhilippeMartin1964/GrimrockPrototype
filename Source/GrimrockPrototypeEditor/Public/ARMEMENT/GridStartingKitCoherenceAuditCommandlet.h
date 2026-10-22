#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridStartingKitCoherenceAuditCommandlet.generated.h"

/** Read-only production report. Only writes CSV under Saved/ARMEMENT/ARMEMENT01.9.4. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridStartingKitCoherenceAuditCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGridStartingKitCoherenceAuditCommandlet();
    virtual int32 Main(const FString& Params) override;
};

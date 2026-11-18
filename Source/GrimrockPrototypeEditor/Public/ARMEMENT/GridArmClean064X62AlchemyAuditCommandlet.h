#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064X62AlchemyAuditCommandlet.generated.h"

/** Read-only audit of the native alchemy sources and their resolved action identities. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064X62AlchemyAuditCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064X62AlchemyAuditCommandlet();
    virtual int32 Main(const FString& Params) override;
};

#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean065RetirementAuditCommandlet.generated.h"

UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean065RetirementAuditCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean065RetirementAuditCommandlet();
    virtual int32 Main(const FString& Params) override;
};

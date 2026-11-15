#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064HReferenceAuditCommandlet.generated.h"

/** Read-only 06.4H preflight of real UE level/item/receptacle references. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064HReferenceAuditCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGridArmClean064HReferenceAuditCommandlet();
	virtual int32 Main(const FString& Params) override;
};

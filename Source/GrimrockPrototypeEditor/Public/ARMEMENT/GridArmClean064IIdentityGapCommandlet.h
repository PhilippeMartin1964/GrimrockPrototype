#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064IIdentityGapCommandlet.generated.h"

/** 06.4I — classify actual distinct legacy ItemIds, never modify assets. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064IIdentityGapCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGridArmClean064IIdentityGapCommandlet();
	virtual int32 Main(const FString& Params) override;
};

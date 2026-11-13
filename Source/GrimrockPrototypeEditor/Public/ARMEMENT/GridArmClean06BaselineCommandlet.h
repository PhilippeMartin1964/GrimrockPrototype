#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean06BaselineCommandlet.generated.h"

/** ARM-CLEAN06.0: read-only snapshot of UE serialized assets. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean06BaselineCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGridArmClean06BaselineCommandlet();
	virtual int32 Main(const FString& Params) override;
};

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064JNonArmProjectionCommandlet.generated.h"

/** Editor-only transient projection of the 11 missing non-weapon ItemIds. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064JNonArmProjectionCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGridArmClean064JNonArmProjectionCommandlet();
	virtual int32 Main(const FString& Params) override;
};

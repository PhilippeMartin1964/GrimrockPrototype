#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064LNonArmPresentationParityCommandlet.generated.h"

/** ARM-CLEAN06.4L: real UE read-only projection of non-ARM light and persistent visuals. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064LNonArmPresentationParityCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGridArmClean064LNonArmPresentationParityCommandlet();
	virtual int32 Main(const FString& Params) override;
};

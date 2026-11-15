#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064EPresentationParityCommandlet.generated.h"

/** 06.4E: inspect real legacy and canonical UE assets, never save either. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064EPresentationParityCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGridArmClean064EPresentationParityCommandlet();
	virtual int32 Main(const FString& Params) override;
};

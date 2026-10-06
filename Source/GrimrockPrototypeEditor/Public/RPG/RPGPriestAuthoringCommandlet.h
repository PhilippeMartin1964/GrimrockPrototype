#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RPGPriestAuthoringCommandlet.generated.h"

UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API URPGPriestAuthoringCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URPGPriestAuthoringCommandlet();
	virtual int32 Main(const FString& Params) override;
};

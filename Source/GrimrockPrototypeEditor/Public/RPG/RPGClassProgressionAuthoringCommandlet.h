#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RPGClassProgressionAuthoringCommandlet.generated.h"

UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API URPGClassProgressionAuthoringCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URPGClassProgressionAuthoringCommandlet();
	virtual int32 Main(const FString& Params) override;
};

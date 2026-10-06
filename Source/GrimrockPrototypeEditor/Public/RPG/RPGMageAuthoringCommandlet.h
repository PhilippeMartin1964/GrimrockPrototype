#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RPGMageAuthoringCommandlet.generated.h"

UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API URPGMageAuthoringCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URPGMageAuthoringCommandlet();
	virtual int32 Main(const FString& Params) override;
};

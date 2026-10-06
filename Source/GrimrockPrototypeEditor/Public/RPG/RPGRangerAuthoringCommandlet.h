#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RPGRangerAuthoringCommandlet.generated.h"

UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API URPGRangerAuthoringCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URPGRangerAuthoringCommandlet();
	virtual int32 Main(const FString& Params) override;
};

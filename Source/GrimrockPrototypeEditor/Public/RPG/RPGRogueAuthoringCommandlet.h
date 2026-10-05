#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RPGRogueAuthoringCommandlet.generated.h"

UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API URPGRogueAuthoringCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URPGRogueAuthoringCommandlet();
	virtual int32 Main(const FString& Params) override;
};

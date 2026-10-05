#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RPGWarriorAuthoringCommandlet.generated.h"

UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API URPGWarriorAuthoringCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URPGWarriorAuthoringCommandlet();
	virtual int32 Main(const FString& Params) override;
};

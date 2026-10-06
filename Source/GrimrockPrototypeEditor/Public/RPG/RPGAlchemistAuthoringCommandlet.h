#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RPGAlchemistAuthoringCommandlet.generated.h"

UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API URPGAlchemistAuthoringCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URPGAlchemistAuthoringCommandlet();
	virtual int32 Main(const FString& Params) override;
};

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "UIStringTableAuthoringCommandlet.generated.h"

UCLASS()
class UUIStringTableAuthoringCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UUIStringTableAuthoringCommandlet();
	virtual int32 Main(const FString& Params) override;
};

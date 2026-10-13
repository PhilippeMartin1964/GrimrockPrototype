#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "UILocalizationEnglishSmokeCommandlet.generated.h"

UCLASS()
class UUILocalizationEnglishSmokeCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UUILocalizationEnglishSmokeCommandlet();
	virtual int32 Main(const FString& Params) override;
};

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "UITalentTextSetCentralizeCommandlet.generated.h"

UCLASS()
class UUITalentTextSetCentralizeCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UUITalentTextSetCentralizeCommandlet();
	virtual int32 Main(const FString& Params) override;
};

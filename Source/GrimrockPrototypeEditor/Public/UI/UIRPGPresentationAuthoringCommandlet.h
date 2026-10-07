#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "UIRPGPresentationAuthoringCommandlet.generated.h"

/** Creates or refreshes the canonical UI-RPG02 presentation DataAsset. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UUIRPGPresentationAuthoringCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UUIRPGPresentationAuthoringCommandlet();
	virtual int32 Main(const FString& Params) override;
};

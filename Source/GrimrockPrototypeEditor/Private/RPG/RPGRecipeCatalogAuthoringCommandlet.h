#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RPGRecipeCatalogAuthoringCommandlet.generated.h"

UCLASS()
class URPGRecipeCatalogAuthoringCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URPGRecipeCatalogAuthoringCommandlet();
	virtual int32 Main(const FString& Params) override;
};

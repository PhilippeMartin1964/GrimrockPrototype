#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064X66NativeAlchemyCutoverCommandlet.generated.h"

/** X.66 guarded twelve-package alchemy registry and action cutover. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064X66NativeAlchemyCutoverCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064X66NativeAlchemyCutoverCommandlet();
    virtual int32 Main(const FString& Params) override;
};

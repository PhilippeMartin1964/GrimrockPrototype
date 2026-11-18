#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064X61NativeLootCutoverCommandlet.generated.h"

/** X.61 guarded seven-package loot registry and reference cutover. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064X61NativeLootCutoverCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064X61NativeLootCutoverCommandlet();
    virtual int32 Main(const FString& Params) override;
};

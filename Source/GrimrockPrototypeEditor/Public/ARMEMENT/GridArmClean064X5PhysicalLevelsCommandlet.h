#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064X5PhysicalLevelsCommandlet.generated.h"

/** One-shot Editor commandlet, guarded by the exact five-package 06.4X.5 script.
 * It never opens or writes player SaveGames, maps, WBP or old definition assets. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064X5PhysicalLevelsCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064X5PhysicalLevelsCommandlet();
    virtual int32 Main(const FString& Params) override;
};

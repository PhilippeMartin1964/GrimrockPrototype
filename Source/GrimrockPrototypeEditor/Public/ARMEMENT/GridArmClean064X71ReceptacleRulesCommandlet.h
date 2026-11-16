#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064X71ReceptacleRulesCommandlet.generated.h"

/** Strict two-native-package receptacle migration; -Audit only is
 * exercised in X.16.7.1. -Apply remains uninvoked until a separate
 * guarded SHA-256/backups + independent-reopen transaction. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064X71ReceptacleRulesCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064X71ReceptacleRulesCommandlet();
    virtual int32 Main(const FString& Params) override;
};

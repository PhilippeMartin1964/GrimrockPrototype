#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064X54NativeSharedRulesCommandlet.generated.h"

/** X.54: four strictly allowlisted native shared acceptance definitions.
 *  Read-only -Preflight/-Verify. -Apply requires four byte-equal private
 *  backups outside the project, an exact SHA40 operator receipt, and is
 *  never run by CI. The Windows runner restores all four files on failure.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064X54NativeSharedRulesCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064X54NativeSharedRulesCommandlet();
    virtual int32 Main(const FString& Params) override;
};

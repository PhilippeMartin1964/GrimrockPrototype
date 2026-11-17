#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064X46NativeLoosePointersCommandlet.generated.h"

/** ARM-CLEAN06.4X.46: one-package, 44-item native pointer cleanup.
 *  Read-only -Preflight/-Verify. -Apply requires a byte-equal private backup
 *  outside the repository and an exact SHA40 receipt verified by the runner.
 *  Never updates receptacle contents, palette, other levels or SaveGames.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064X46NativeLoosePointersCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064X46NativeLoosePointersCommandlet();
    virtual int32 Main(const FString& Params) override;
};

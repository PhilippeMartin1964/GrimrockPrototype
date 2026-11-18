#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064X57NativePaletteCommandlet.generated.h"

/** X.57: the single allowlisted DA_ObjectPalette_Default package.
 *  Read-only Preflight/Verify; Apply requires a private byte-equal backup
 *  and exact SHA40 operator receipt. Only the protected Windows runner
 *  may request Apply. No SaveGame or other package may be modified.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064X57NativePaletteCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064X57NativePaletteCommandlet();
    virtual int32 Main(const FString& Params) override;
};

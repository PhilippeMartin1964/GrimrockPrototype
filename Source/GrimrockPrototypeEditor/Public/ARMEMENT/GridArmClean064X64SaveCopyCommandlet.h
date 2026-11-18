#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridArmClean064X64SaveCopyCommandlet.generated.h"

/** Convert only a named copy under SnapshotRoot/Original to a new file
 * under SnapshotRoot/Converted. No live slots, replacement or publication.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridArmClean064X64SaveCopyCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UGridArmClean064X64SaveCopyCommandlet();
    virtual int32 Main(const FString& Params) override;
};

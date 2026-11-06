#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridEffectsOrgTorchMoveCommandlet.generated.h"

/**
 * EFFECTS-ORG01.2D: one-time Unreal-native relocation of the torch Niagara.
 * Read-only unless -Apply is passed. Invoked only in the isolated Windows
 * agent worktree, after a full passing suite.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridEffectsOrgTorchMoveCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UGridEffectsOrgTorchMoveCommandlet();
    virtual int32 Main(const FString& Params) override;
};

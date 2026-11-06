#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridEffectsOrgPotionRenameCommandlet.generated.h"

/**
 * EFFECTS-ORG01.2B. Explicit, narrowly scoped Unreal-native rename of
 * the healing-potion bubble material. Read-only unless -Apply is passed.
 * Never touches the developer checkout; the agent invokes it in its
 * isolated, verified UE validation worktree.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridEffectsOrgPotionRenameCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGridEffectsOrgPotionRenameCommandlet();
	virtual int32 Main(const FString& Params) override;
};

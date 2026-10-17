#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "SkillAffinityAuthoringCommandlet.generated.h"

/** Materializes the canonical UI-SKILLS-AFFINITY01.3C DiscouragedClassIds matrix. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API USkillAffinityAuthoringCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	USkillAffinityAuthoringCommandlet();
	virtual int32 Main(const FString& Params) override;
};

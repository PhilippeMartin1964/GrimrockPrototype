#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "SkillAffinityAuditCommandlet.generated.h"

/**
 * UI-SKILLS-AFFINITY01.3A
 * Read-only production audit for DA_RPGSkillPresentation.
 * Exports the current Recommended / Useful / Discouraged / Neutral matrix.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API USkillAffinityAuditCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	USkillAffinityAuditCommandlet();
	virtual int32 Main(const FString& Params) override;
};

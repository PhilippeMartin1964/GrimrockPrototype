#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "UIRPGTalentTreeAuthoringCommandlet.generated.h"

/**
 * UI-RPG01.4 editor-only materialization commandlet.
 * Re-runs the six canonical class authoring sources and saves only DA_Class_*.
 */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UUIRPGTalentTreeAuthoringCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UUIRPGTalentTreeAuthoringCommandlet();
	virtual int32 Main(const FString& Params) override;
};

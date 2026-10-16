#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "UITalentTextSetAuditCommandlet.generated.h"

/**
 * UI-TEXT-CENTRAL01.2A
 *
 * Read-only audit of the legacy Talent presentation TextSet.
 * The commandlet never edits or saves production assets.
 */
UCLASS()
class UUITalentTextSetAuditCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UUITalentTextSetAuditCommandlet();
	virtual int32 Main(const FString& Params) override;
};

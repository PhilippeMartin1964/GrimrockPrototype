#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RPGSkillCatalogAuthoringCommandlet.generated.h"

/** Matérialise le catalogue canonique URPGSkillAsset défini par UI-RPG06.1. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API URPGSkillCatalogAuthoringCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	URPGSkillCatalogAuthoringCommandlet();
	virtual int32 Main(const FString& Params) override;
};

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GridSpellCatalogAuthoringCommandlet.generated.h"

/** Materializes the four production UGridSpellDefinitionAsset records for TEXT05.5. */
UCLASS()
class GRIMROCKPROTOTYPEEDITOR_API UGridSpellCatalogAuthoringCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGridSpellCatalogAuthoringCommandlet();
	virtual int32 Main(const FString& Params) override;
};

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Magic/GridSpellTypes.h"
#include "GridSpellDefinitionAsset.generated.h"

/**
 * Authorable data-driven container for the MON18 spell contract.
 * Runtime systems consume Definition; the asset itself owns no cast state.
 */
UCLASS (BlueprintType)
class GRIMROCKPROTOTYPE_API UGridSpellDefinitionAsset : public UPrimaryDataAsset
{
    GENERATED_BODY ()

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Spell")
    FGridSpellDefinition Definition;

    EGridSpellValidationError ValidateDefinition () const
    {
        return FGridSpellContract::ValidateDefinition (Definition);
    }
};

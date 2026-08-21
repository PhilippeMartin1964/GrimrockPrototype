#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Magic/GridSpellPresentation.h"
#include "Magic/GridSpellTypes.h"
#include "GridSpellDefinitionAsset.generated.h"

/**
 * Authorable data-driven container for the MON18 spell contract.
 * Runtime systems consume Definition/Presentation; the asset owns no cast state.
 */
UCLASS (BlueprintType)
class GRIMROCKPROTOTYPE_API UGridSpellDefinitionAsset : public UPrimaryDataAsset
{
    GENERATED_BODY ()

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Spell")
    FGridSpellDefinition Definition;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Spell|Presentation")
    FGridSpellPresentationProfile Presentation;

    EGridSpellValidationError ValidateDefinition () const
    {
        return FGridSpellContract::ValidateDefinition (Definition);
    }

    bool HasValidPresentation () const
    {
        return Presentation.IsValid ();
    }
};

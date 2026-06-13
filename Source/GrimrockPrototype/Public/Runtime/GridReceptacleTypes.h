#pragma once

#include "CoreMinimal.h"
#include "Core/GridObjectBehavior.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "GridReceptacleTypes.generated.h"

UENUM (BlueprintType)
enum class EGridReceptacleRejectReason : uint8
{
    None,
    InvalidItem,
    Full,
    ExplicitlyRejected,
    NoMatchingAcceptanceRule
};

USTRUCT (BlueprintType)
struct FGridReceptacleAcceptanceResult
{
    GENERATED_BODY ()

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    bool bAccepted = false;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    EGridReceptacleRejectReason RejectReason = EGridReceptacleRejectReason::None;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Receptacle")
    FName MatchedRule = NAME_None;
};

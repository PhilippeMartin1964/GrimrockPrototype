#pragma once

#include "CoreMinimal.h"
#include "GridLevelVariableTypes.generated.h"

/** Types de variables logiques persistantes supportés par MON19.2.2. */
UENUM (BlueprintType)
enum class EGridLevelVariableType : uint8
{
    Bool  UMETA (DisplayName = "Bool"),
    Int32 UMETA (DisplayName = "Int32")
};

/** Déclaration data-driven d'une variable logique appartenant à un niveau. */
USTRUCT (BlueprintType)
struct FGridLevelVariableDefinition
{
    GENERATED_BODY ()

    /** Identité stable dans le niveau et dans les sauvegardes. */
    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Logic")
    FName VariableId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Logic")
    EGridLevelVariableType Type = EGridLevelVariableType::Bool;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Logic|Default",
        meta = (EditCondition = "Type == EGridLevelVariableType::Bool", EditConditionHides))
    bool bDefaultBoolValue = false;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Logic|Default",
        meta = (EditCondition = "Type == EGridLevelVariableType::Int32", EditConditionHides))
    int32 DefaultInt32Value = 0;
};

#pragma once

#include "CoreMinimal.h"
#include "GridLuaScriptTypes.generated.h"

/**
 * MON19.3.1 source-only Lua script registered in one level.
 * ScriptId is the stable editor/runtime identity; bytecode is never persisted.
 */
USTRUCT (BlueprintType)
struct GRIMROCKLUA_API FGridLuaScriptSource
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Lua")
    FName ScriptId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Lua")
    bool bEnabled = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Lua", meta = (MultiLine = "true"))
    FString Source;
};

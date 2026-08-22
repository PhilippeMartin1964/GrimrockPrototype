#pragma once

#include "CoreMinimal.h"
#include "GridLuaScriptTypes.h"

/** Runtime limits applied independently to one Lua VM. */
struct GRIMROCKLUA_API FGridLuaVmConfig
{
    /** Hard allocator quota. MON19.3.1 requires at least 1 MiB. */
    SIZE_T MemoryLimitBytes = 8 * 1024 * 1024;

    /** Instruction budget reset for every chunk load/callback execution. */
    int32 InstructionBudgetPerCall = 100000;
};

/**
 * MON19.3.1 Lua 5.4.8 runtime foundation.
 *
 * The Lua C API is deliberately hidden behind this PIMPL wrapper so no
 * lua_State or Lua header crosses the GrimrockLua module boundary.
 */
class GRIMROCKLUA_API FGridLuaVm
{
public:
    FGridLuaVm ();
    ~FGridLuaVm ();

    FGridLuaVm (const FGridLuaVm&) = delete;
    FGridLuaVm& operator= (const FGridLuaVm&) = delete;

    /**
     * Atomically replaces the VM after all enabled scripts load successfully.
     * On failure, the previously valid VM remains untouched.
     */
    bool Reload (
        const TArray<FGridLuaScriptSource>& Scripts,
        const FGridLuaVmConfig& Config,
        FString& OutError);

    void Reset ();

    bool IsReady () const;
    int32 GetLoadedScriptCount () const;
    bool HasScript (FName ScriptId) const;

    /** Calls one no-argument/no-result callback in the selected script _ENV. */
    bool CallFunction (
        FName ScriptId,
        FName FunctionName,
        FString& OutError);

    /** Test/diagnostic access to script-local scalar globals. */
    bool TryGetScriptBool (
        FName ScriptId,
        FName GlobalName,
        bool& OutValue,
        FString& OutError) const;

    bool TryGetScriptInt32 (
        FName ScriptId,
        FName GlobalName,
        int32& OutValue,
        FString& OutError) const;

    FString GetVersionString () const;
    SIZE_T GetAllocatedBytes () const;

    static bool ValidateScriptDefinitions (
        const TArray<FGridLuaScriptSource>& Scripts,
        FString& OutError);

private:
    struct FImpl;
    TUniquePtr<FImpl> Impl;
};

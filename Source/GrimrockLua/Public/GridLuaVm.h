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

/** Minimal event context exposed to one MON19.4 callback. */
struct GRIMROCKLUA_API FGridLuaEventContext
{
    FString SourceObjectId;
    FString EventName;
};

/**
 * Host callbacks backing the safe `grid` Lua API.
 *
 * GrimrockLua deliberately knows nothing about UWorld, Actors or gameplay
 * enums. The owning runtime binds these functions for the duration of one
 * callback and remains the only authority allowed to mutate gameplay state.
 */
struct GRIMROCKLUA_API FGridLuaHostApi
{
    TFunction<bool(FName, bool&, FString&)> GetBool;
    TFunction<bool(FName, bool, FString&)> SetBool;
    TFunction<bool(FName, int32&, FString&)> GetInt32;
    TFunction<bool(FName, int32, FString&)> SetInt32;
    TFunction<bool(const FString&, const FString&, FString&)> Command;
    TFunction<void(const FString&)> Log;
};

/**
 * MON19.3.1 Lua 5.4.8 runtime foundation, extended by MON19.4 with a
 * host-controlled Event -> Lua callback bridge.
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

    /**
     * MON19.4 event callback. The callback receives one Lua table argument:
     *
     *   event.source_object_id
     *   event.event
     *
     * During this protected call only, the script-local `grid` API is backed
     * by HostApi. Nested host-bound Lua callbacks are rejected explicitly.
     */
    bool CallEventFunction (
        FName ScriptId,
        FName FunctionName,
        const FGridLuaEventContext& EventContext,
        const FGridLuaHostApi& HostApi,
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

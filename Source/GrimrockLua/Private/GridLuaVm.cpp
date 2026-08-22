#include "GridLuaVm.h"

#include "HAL/UnrealMemory.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

namespace
{
    constexpr SIZE_T MinimumLuaMemoryLimitBytes = 1024 * 1024;
    constexpr int32 MaximumHookStep = 1000;

    FString GetLuaStackError (lua_State* State, const TCHAR* Fallback)
    {
        if (!State)
        {
            return Fallback;
        }

        const char* ErrorText = lua_tostring (State, -1);
        return ErrorText
            ? FString (UTF8_TO_TCHAR (ErrorText))
            : FString (Fallback);
    }

    void CloneLuaTable (lua_State* State, int32 SourceIndex)
    {
        SourceIndex = lua_absindex (State, SourceIndex);
        lua_newtable (State);
        const int32 DestinationIndex = lua_absindex (State, -1);

        lua_pushnil (State);
        while (lua_next (State, SourceIndex) != 0)
        {
            // Stack: source ... key value. Duplicate key/value into destination.
            lua_pushvalue (State, -2);
            lua_pushvalue (State, -2);
            lua_rawset (State, DestinationIndex);
            lua_pop (State, 1); // original value; keep key for lua_next
        }
    }
}

struct FGridLuaVm::FImpl
{
    lua_State* State = nullptr;
    FGridLuaVmConfig Config;
    TMap<FName, int32> EnvironmentRefByScriptId;
    SIZE_T AllocatedBytes = 0;
    int32 RemainingInstructionBudget = 0;
    int32 HookStep = MaximumHookStep;

    ~FImpl ()
    {
        Close ();
    }

    static void* LuaAllocator (
        void* UserData,
        void* Ptr,
        size_t OldSize,
        size_t NewSize)
    {
        FImpl* Self = static_cast<FImpl*> (UserData);
        if (!Self)
        {
            return nullptr;
        }

        const SIZE_T CountedOldSize = Ptr ? static_cast<SIZE_T> (OldSize) : 0;
        const SIZE_T BaseBytes = Self->AllocatedBytes >= CountedOldSize
            ? Self->AllocatedBytes - CountedOldSize
            : 0;

        if (NewSize == 0)
        {
            if (Ptr)
            {
                FMemory::Free (Ptr);
            }
            Self->AllocatedBytes = BaseBytes;
            return nullptr;
        }

        const SIZE_T RequestedSize = static_cast<SIZE_T> (NewSize);
        if (BaseBytes > Self->Config.MemoryLimitBytes ||
            RequestedSize > Self->Config.MemoryLimitBytes - BaseBytes)
        {
            return nullptr;
        }

        void* NewPtr = FMemory::Realloc (Ptr, RequestedSize);
        if (!NewPtr)
        {
            return nullptr;
        }

        Self->AllocatedBytes = BaseBytes + RequestedSize;
        return NewPtr;
    }

    static void InstructionHook (lua_State* InState, lua_Debug*)
    {
        FImpl* const* SelfSlot =
            static_cast<FImpl* const*> (lua_getextraspace (InState));
        FImpl* Self = SelfSlot ? *SelfSlot : nullptr;
        if (!Self)
        {
            luaL_error (InState, "Grimrock Lua instruction hook lost VM context");
            return;
        }

        Self->RemainingInstructionBudget -= Self->HookStep;
        if (Self->RemainingInstructionBudget <= 0)
        {
            luaL_error (InState, "Grimrock Lua instruction budget exceeded");
        }
    }

    bool Initialize (FString& OutError)
    {
        State = lua_newstate (&FImpl::LuaAllocator, this);
        if (!State)
        {
            OutError = TEXT ("Unable to create Lua state within the configured memory quota.");
            return false;
        }

        *static_cast<FImpl**> (lua_getextraspace (State)) = this;

        // Open only the libraries used to construct script-local safe _ENV tables.
        // The actual Lua global table is never exposed to scripts.
        luaL_requiref (State, "_G", luaopen_base, 1);
        lua_pop (State, 1);
        luaL_requiref (State, LUA_MATHLIBNAME, luaopen_math, 1);
        lua_pop (State, 1);
        luaL_requiref (State, LUA_STRLIBNAME, luaopen_string, 1);
        lua_pop (State, 1);
        luaL_requiref (State, LUA_TABLIBNAME, luaopen_table, 1);
        lua_pop (State, 1);
        luaL_requiref (State, LUA_UTF8LIBNAME, luaopen_utf8, 1);
        lua_pop (State, 1);

        OutError.Reset ();
        return true;
    }

    void Close ()
    {
        EnvironmentRefByScriptId.Reset ();
        if (State)
        {
            lua_sethook (State, nullptr, 0, 0);
            lua_close (State);
            State = nullptr;
        }
        AllocatedBytes = 0;
        RemainingInstructionBudget = 0;
    }

    void PushSafeEnvironment ()
    {
        lua_newtable (State);
        const int32 EnvironmentIndex = lua_absindex (State, -1);

        static const char* SafeBaseNames[] = {
            "assert",
            "error",
            "ipairs",
            "next",
            "pairs",
            "pcall",
            "rawequal",
            "rawget",
            "rawlen",
            "rawset",
            "select",
            "tonumber",
            "tostring",
            "type",
            "xpcall",
            "_VERSION"
        };

        for (const char* Name : SafeBaseNames)
        {
            lua_getglobal (State, Name);
            if (!lua_isnil (State, -1))
            {
                lua_setfield (State, EnvironmentIndex, Name);
            }
            else
            {
                lua_pop (State, 1);
            }
        }

        static const char* SafeLibraryNames[] = {
            LUA_MATHLIBNAME,
            LUA_STRLIBNAME,
            LUA_TABLIBNAME,
            LUA_UTF8LIBNAME
        };

        for (const char* LibraryName : SafeLibraryNames)
        {
            lua_getglobal (State, LibraryName);
            if (lua_istable (State, -1))
            {
                CloneLuaTable (State, -1);
                lua_setfield (State, EnvironmentIndex, LibraryName);
            }
            lua_pop (State, 1); // source library or nil
        }

        // _G is script-local; scripts cannot reach Lua's hidden process globals.
        lua_pushvalue (State, EnvironmentIndex);
        lua_setfield (State, EnvironmentIndex, "_G");
    }

    bool ExecuteProtectedFunction (
        int32 ArgumentCount,
        int32 ResultCount,
        FString& OutError)
    {
        RemainingInstructionBudget = Config.InstructionBudgetPerCall;
        HookStep = FMath::Clamp (
            Config.InstructionBudgetPerCall,
            1,
            MaximumHookStep);
        lua_sethook (State, &FImpl::InstructionHook, LUA_MASKCOUNT, HookStep);

        const int32 Status = lua_pcall (
            State,
            ArgumentCount,
            ResultCount,
            0);

        lua_sethook (State, nullptr, 0, 0);
        RemainingInstructionBudget = 0;

        if (Status != LUA_OK)
        {
            OutError = GetLuaStackError (
                State,
                TEXT ("Lua protected call failed without an error string."));
            return false;
        }

        OutError.Reset ();
        return true;
    }

    bool LoadScript (
        const FGridLuaScriptSource& Script,
        FString& OutError)
    {
        const int32 BaseTop = lua_gettop (State);

        PushSafeEnvironment ();
        const int32 EnvironmentRef = luaL_ref (State, LUA_REGISTRYINDEX);

        const FString ScriptIdString = Script.ScriptId.ToString ();
        const FString ChunkName = FString::Printf (
            TEXT ("@Grimrock/%s.lua"),
            *ScriptIdString);
        FTCHARToUTF8 SourceUtf8 (*Script.Source);
        FTCHARToUTF8 ChunkNameUtf8 (*ChunkName);

        const int32 LoadStatus = luaL_loadbufferx (
            State,
            SourceUtf8.Get (),
            static_cast<size_t> (SourceUtf8.Length ()),
            ChunkNameUtf8.Get (),
            "t");
        if (LoadStatus != LUA_OK)
        {
            OutError = FString::Printf (
                TEXT ("Script '%s' failed to compile: %s"),
                *ScriptIdString,
                *GetLuaStackError (State, TEXT ("unknown compile error")));
            luaL_unref (State, LUA_REGISTRYINDEX, EnvironmentRef);
            lua_settop (State, BaseTop);
            return false;
        }

        lua_rawgeti (State, LUA_REGISTRYINDEX, EnvironmentRef);
        const char* UpvalueName = lua_setupvalue (State, -2, 1);
        if (!UpvalueName || FCStringAnsi::Strcmp (UpvalueName, "_ENV") != 0)
        {
            OutError = FString::Printf (
                TEXT ("Script '%s' has no assignable _ENV upvalue."),
                *ScriptIdString);
            luaL_unref (State, LUA_REGISTRYINDEX, EnvironmentRef);
            lua_settop (State, BaseTop);
            return false;
        }

        FString ExecutionError;
        if (!ExecuteProtectedFunction (0, 0, ExecutionError))
        {
            OutError = FString::Printf (
                TEXT ("Script '%s' failed during initialization: %s"),
                *ScriptIdString,
                *ExecutionError);
            luaL_unref (State, LUA_REGISTRYINDEX, EnvironmentRef);
            lua_settop (State, BaseTop);
            return false;
        }

        EnvironmentRefByScriptId.Add (Script.ScriptId, EnvironmentRef);
        lua_settop (State, BaseTop);
        OutError.Reset ();
        return true;
    }

    bool PushScriptGlobal (
        FName ScriptId,
        FName GlobalName,
        FString& OutError) const
    {
        if (!State)
        {
            OutError = TEXT ("Lua VM is not ready.");
            return false;
        }
        if (ScriptId.IsNone () || GlobalName.IsNone ())
        {
            OutError = TEXT ("ScriptId and global name must be non-empty.");
            return false;
        }

        const int32* EnvironmentRef = EnvironmentRefByScriptId.Find (ScriptId);
        if (!EnvironmentRef)
        {
            OutError = FString::Printf (
                TEXT ("Lua script '%s' is not loaded."),
                *ScriptId.ToString ());
            return false;
        }

        lua_rawgeti (State, LUA_REGISTRYINDEX, *EnvironmentRef);
        FTCHARToUTF8 GlobalNameUtf8 (*GlobalName.ToString ());
        lua_getfield (State, -1, GlobalNameUtf8.Get ());
        lua_remove (State, -2); // leave global value only
        OutError.Reset ();
        return true;
    }
};

FGridLuaVm::FGridLuaVm () = default;
FGridLuaVm::~FGridLuaVm () = default;

bool FGridLuaVm::Reload (
    const TArray<FGridLuaScriptSource>& Scripts,
    const FGridLuaVmConfig& Config,
    FString& OutError)
{
    if (!ValidateScriptDefinitions (Scripts, OutError))
    {
        return false;
    }
    if (Config.MemoryLimitBytes < MinimumLuaMemoryLimitBytes)
    {
        OutError = FString::Printf (
            TEXT ("Lua memory quota must be at least %llu bytes."),
            static_cast<uint64> (MinimumLuaMemoryLimitBytes));
        return false;
    }
    if (Config.InstructionBudgetPerCall <= 0)
    {
        OutError = TEXT ("Lua instruction budget must be greater than zero.");
        return false;
    }

    TUniquePtr<FImpl> Candidate = MakeUnique<FImpl> ();
    Candidate->Config = Config;
    if (!Candidate->Initialize (OutError))
    {
        return false;
    }

    for (const FGridLuaScriptSource& Script : Scripts)
    {
        if (!Script.bEnabled)
        {
            continue;
        }
        if (!Candidate->LoadScript (Script, OutError))
        {
            return false;
        }
    }

    Impl = MoveTemp (Candidate);
    OutError.Reset ();
    return true;
}

void FGridLuaVm::Reset ()
{
    Impl.Reset ();
}

bool FGridLuaVm::IsReady () const
{
    return Impl.IsValid () && Impl->State != nullptr;
}

int32 FGridLuaVm::GetLoadedScriptCount () const
{
    return IsReady () ? Impl->EnvironmentRefByScriptId.Num () : 0;
}

bool FGridLuaVm::HasScript (FName ScriptId) const
{
    return IsReady () && Impl->EnvironmentRefByScriptId.Contains (ScriptId);
}

bool FGridLuaVm::CallFunction (
    FName ScriptId,
    FName FunctionName,
    FString& OutError)
{
    if (!IsReady ())
    {
        OutError = TEXT ("Lua VM is not ready.");
        return false;
    }

    const int32 BaseTop = lua_gettop (Impl->State);
    if (!Impl->PushScriptGlobal (ScriptId, FunctionName, OutError))
    {
        lua_settop (Impl->State, BaseTop);
        return false;
    }

    if (!lua_isfunction (Impl->State, -1))
    {
        OutError = FString::Printf (
            TEXT ("Lua '%s.%s' is not a function."),
            *ScriptId.ToString (),
            *FunctionName.ToString ());
        lua_settop (Impl->State, BaseTop);
        return false;
    }

    FString ExecutionError;
    const bool bSuccess = Impl->ExecuteProtectedFunction (0, 0, ExecutionError);
    if (!bSuccess)
    {
        OutError = FString::Printf (
            TEXT ("Lua callback '%s.%s' failed: %s"),
            *ScriptId.ToString (),
            *FunctionName.ToString (),
            *ExecutionError);
    }

    lua_settop (Impl->State, BaseTop);
    if (bSuccess)
    {
        OutError.Reset ();
    }
    return bSuccess;
}

bool FGridLuaVm::TryGetScriptBool (
    FName ScriptId,
    FName GlobalName,
    bool& OutValue,
    FString& OutError) const
{
    if (!IsReady ())
    {
        OutError = TEXT ("Lua VM is not ready.");
        return false;
    }

    const int32 BaseTop = lua_gettop (Impl->State);
    if (!Impl->PushScriptGlobal (ScriptId, GlobalName, OutError))
    {
        lua_settop (Impl->State, BaseTop);
        return false;
    }
    if (!lua_isboolean (Impl->State, -1))
    {
        OutError = FString::Printf (
            TEXT ("Lua '%s.%s' is not a Bool."),
            *ScriptId.ToString (),
            *GlobalName.ToString ());
        lua_settop (Impl->State, BaseTop);
        return false;
    }

    OutValue = lua_toboolean (Impl->State, -1) != 0;
    lua_settop (Impl->State, BaseTop);
    OutError.Reset ();
    return true;
}

bool FGridLuaVm::TryGetScriptInt32 (
    FName ScriptId,
    FName GlobalName,
    int32& OutValue,
    FString& OutError) const
{
    if (!IsReady ())
    {
        OutError = TEXT ("Lua VM is not ready.");
        return false;
    }

    const int32 BaseTop = lua_gettop (Impl->State);
    if (!Impl->PushScriptGlobal (ScriptId, GlobalName, OutError))
    {
        lua_settop (Impl->State, BaseTop);
        return false;
    }
    if (!lua_isinteger (Impl->State, -1))
    {
        OutError = FString::Printf (
            TEXT ("Lua '%s.%s' is not an integer."),
            *ScriptId.ToString (),
            *GlobalName.ToString ());
        lua_settop (Impl->State, BaseTop);
        return false;
    }

    const lua_Integer LuaValue = lua_tointeger (Impl->State, -1);
    if (LuaValue < static_cast<lua_Integer> (MIN_int32) ||
        LuaValue > static_cast<lua_Integer> (MAX_int32))
    {
        OutError = FString::Printf (
            TEXT ("Lua '%s.%s' is outside the Int32 range."),
            *ScriptId.ToString (),
            *GlobalName.ToString ());
        lua_settop (Impl->State, BaseTop);
        return false;
    }

    OutValue = static_cast<int32> (LuaValue);
    lua_settop (Impl->State, BaseTop);
    OutError.Reset ();
    return true;
}

FString FGridLuaVm::GetVersionString () const
{
    return FString (UTF8_TO_TCHAR (LUA_RELEASE));
}

SIZE_T FGridLuaVm::GetAllocatedBytes () const
{
    return IsReady () ? Impl->AllocatedBytes : 0;
}

bool FGridLuaVm::ValidateScriptDefinitions (
    const TArray<FGridLuaScriptSource>& Scripts,
    FString& OutError)
{
    TSet<FName> SeenIds;
    for (int32 Index = 0; Index < Scripts.Num (); ++Index)
    {
        const FGridLuaScriptSource& Script = Scripts[Index];
        if (Script.ScriptId.IsNone ())
        {
            OutError = FString::Printf (
                TEXT ("Lua script entry %d has an empty ScriptId."),
                Index);
            return false;
        }
        if (SeenIds.Contains (Script.ScriptId))
        {
            OutError = FString::Printf (
                TEXT ("Lua ScriptId '%s' is duplicated."),
                *Script.ScriptId.ToString ());
            return false;
        }
        SeenIds.Add (Script.ScriptId);
    }

    OutError.Reset ();
    return true;
}

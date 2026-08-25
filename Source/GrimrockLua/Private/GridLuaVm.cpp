#include "GridLuaVm.h"

#include "HAL/UnrealMemory.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

namespace
{
	constexpr SIZE_T MinimumLuaMemoryLimitBytes = 1024 * 1024;
	constexpr int32 MaximumHookStep = 1000;

	FString GetLuaStackError(lua_State* State, const TCHAR* Fallback)
	{
		if (!State)
		{
			return Fallback;
		}

		const char* ErrorText = lua_tostring(State, -1);
		return ErrorText ? FString(UTF8_TO_TCHAR(ErrorText)) : FString(Fallback);
	}

	void CloneLuaTable(lua_State* State, int32 SourceIndex)
	{
		SourceIndex = lua_absindex(State, SourceIndex);
		lua_newtable(State);
		const int32 DestinationIndex = lua_absindex(State, -1);

		lua_pushnil(State);
		while (lua_next(State, SourceIndex) != 0)
		{
			lua_pushvalue(State, -2);
			lua_pushvalue(State, -2);
			lua_rawset(State, DestinationIndex);
			lua_pop(State, 1);
		}
	}

	bool ArePersistentDefinitionsCompatible(const FGridLuaPersistentVariableDefinition& A, const FGridLuaPersistentVariableDefinition& B)
	{
		if (A.VariableId != B.VariableId || A.Type != B.Type)
		{
			return false;
		}
		return A.Type == EGridLuaPersistentValueType::Bool ? A.bDefaultBoolValue == B.bDefaultBoolValue : A.DefaultInt32Value == B.DefaultInt32Value;
	}
}

struct FGridLuaVm::FImpl
{
	lua_State* State = nullptr;
	FGridLuaVmConfig Config;
	TMap<FName, int32> EnvironmentRefByScriptId;
	TMap<FName, TArray<FGridLuaPersistentVariableDefinition>> PersistentDefinitionsByScriptId;
	TMap<FName, FGridLuaPersistentVariableDefinition> PersistentDefinitionByVariableId;
	SIZE_T AllocatedBytes = 0;
	int32 RemainingInstructionBudget = 0;
	int32 HookStep = MaximumHookStep;
	const FGridLuaHostApi* ActiveHost = nullptr;

	~FImpl()
	{
		Close();
	}

	static FImpl* GetSelf(lua_State* InState)
	{
		if (!InState)
		{
			return nullptr;
		}
		FImpl* const* SelfSlot = static_cast<FImpl* const*>(lua_getextraspace(InState));
		return SelfSlot ? *SelfSlot : nullptr;
	}

	static void* LuaAllocator(void* UserData, void* Ptr, size_t OldSize, size_t NewSize)
	{
		FImpl* Self = static_cast<FImpl*>(UserData);
		if (!Self)
		{
			return nullptr;
		}

		const SIZE_T CountedOldSize = Ptr ? static_cast<SIZE_T>(OldSize) : 0;
		const SIZE_T BaseBytes = Self->AllocatedBytes >= CountedOldSize ? Self->AllocatedBytes - CountedOldSize : 0;

		if (NewSize == 0)
		{
			if (Ptr)
			{
				FMemory::Free(Ptr);
			}
			Self->AllocatedBytes = BaseBytes;
			return nullptr;
		}

		const SIZE_T RequestedSize = static_cast<SIZE_T>(NewSize);
		if (BaseBytes > Self->Config.MemoryLimitBytes || RequestedSize > Self->Config.MemoryLimitBytes - BaseBytes)
		{
			return nullptr;
		}

		void* NewPtr = FMemory::Realloc(Ptr, RequestedSize);
		if (!NewPtr)
		{
			return nullptr;
		}

		Self->AllocatedBytes = BaseBytes + RequestedSize;
		return NewPtr;
	}

	static void InstructionHook(lua_State* InState, lua_Debug*)
	{
		FImpl* Self = GetSelf(InState);
		if (!Self)
		{
			luaL_error(InState, "Grimrock Lua instruction hook lost VM context");
			return;
		}

		Self->RemainingInstructionBudget -= Self->HookStep;
		if (Self->RemainingInstructionBudget <= 0)
		{
			luaL_error(InState, "Grimrock Lua instruction budget exceeded");
		}
	}

	static int PushGetFailure(lua_State* InState, const FString& Error)
	{
		lua_pushnil(InState);
		FTCHARToUTF8 ErrorUtf8(*Error);
		lua_pushstring(InState, ErrorUtf8.Get());
		return 2;
	}

	static int PushActionResult(lua_State* InState, bool bSuccess, const FString& Error)
	{
		lua_pushboolean(InState, bSuccess ? 1 : 0);
		if (bSuccess)
		{
			lua_pushnil(InState);
		}
		else
		{
			FTCHARToUTF8 ErrorUtf8(*Error);
			lua_pushstring(InState, ErrorUtf8.Get());
		}
		return 2;
	}

	static bool TryReadNameArgument(lua_State* InState, int32 Index, FName& OutName, FString& OutError)
	{
		if (lua_type(InState, Index) != LUA_TSTRING)
		{
			OutError = TEXT("Expected a variable name string.");
			return false;
		}
		const char* Value = lua_tostring(InState, Index);
		if (!Value || Value[0] == '\0')
		{
			OutError = TEXT("Variable name must be non-empty.");
			return false;
		}
		OutName = FName(UTF8_TO_TCHAR(Value));
		OutError.Reset();
		return true;
	}

	static int LuaGetBool(lua_State* InState)
	{
		FImpl* Self = GetSelf(InState);
		if (!Self || !Self->ActiveHost || !Self->ActiveHost->GetBool)
		{
			return PushGetFailure(InState, TEXT("grid.vars.get_bool is unavailable outside a hosted callback."));
		}

		FName VariableId;
		FString Error;
		if (!TryReadNameArgument(InState, 1, VariableId, Error))
		{
			return PushGetFailure(InState, Error);
		}

		bool bValue = false;
		if (!Self->ActiveHost->GetBool(VariableId, bValue, Error))
		{
			return PushGetFailure(InState, Error);
		}

		lua_pushboolean(InState, bValue ? 1 : 0);
		lua_pushnil(InState);
		return 2;
	}

	static int LuaSetBool(lua_State* InState)
	{
		FImpl* Self = GetSelf(InState);
		if (!Self || !Self->ActiveHost || !Self->ActiveHost->SetBool)
		{
			return PushActionResult(InState, false, TEXT("grid.vars.set_bool is unavailable outside a hosted callback."));
		}

		FName VariableId;
		FString Error;
		if (!TryReadNameArgument(InState, 1, VariableId, Error))
		{
			return PushActionResult(InState, false, Error);
		}
		if (!lua_isboolean(InState, 2))
		{
			return PushActionResult(InState, false, TEXT("grid.vars.set_bool expects a Bool value."));
		}

		const bool bValue = lua_toboolean(InState, 2) != 0;
		const bool bSuccess = Self->ActiveHost->SetBool(VariableId, bValue, Error);
		return PushActionResult(InState, bSuccess, Error);
	}

	static int LuaGetInt(lua_State* InState)
	{
		FImpl* Self = GetSelf(InState);
		if (!Self || !Self->ActiveHost || !Self->ActiveHost->GetInt32)
		{
			return PushGetFailure(InState, TEXT("grid.vars.get_int is unavailable outside a hosted callback."));
		}

		FName VariableId;
		FString Error;
		if (!TryReadNameArgument(InState, 1, VariableId, Error))
		{
			return PushGetFailure(InState, Error);
		}

		int32 Value = 0;
		if (!Self->ActiveHost->GetInt32(VariableId, Value, Error))
		{
			return PushGetFailure(InState, Error);
		}

		lua_pushinteger(InState, static_cast<lua_Integer>(Value));
		lua_pushnil(InState);
		return 2;
	}

	static int LuaSetInt(lua_State* InState)
	{
		FImpl* Self = GetSelf(InState);
		if (!Self || !Self->ActiveHost || !Self->ActiveHost->SetInt32)
		{
			return PushActionResult(InState, false, TEXT("grid.vars.set_int is unavailable outside a hosted callback."));
		}

		FName VariableId;
		FString Error;
		if (!TryReadNameArgument(InState, 1, VariableId, Error))
		{
			return PushActionResult(InState, false, Error);
		}
		if (!lua_isinteger(InState, 2))
		{
			return PushActionResult(InState, false, TEXT("grid.vars.set_int expects an integer value."));
		}

		const lua_Integer LuaValue = lua_tointeger(InState, 2);
		if (LuaValue < static_cast<lua_Integer>(MIN_int32) || LuaValue > static_cast<lua_Integer>(MAX_int32))
		{
			return PushActionResult(InState, false, TEXT("grid.vars.set_int value is outside the Int32 range."));
		}

		const bool bSuccess = Self->ActiveHost->SetInt32(VariableId, static_cast<int32>(LuaValue), Error);
		return PushActionResult(InState, bSuccess, Error);
	}

	static int LuaCommand(lua_State* InState)
	{
		FImpl* Self = GetSelf(InState);
		if (!Self || !Self->ActiveHost || !Self->ActiveHost->Command)
		{
			return PushActionResult(InState, false, TEXT("grid.command is unavailable outside a hosted callback."));
		}
		if (lua_type(InState, 1) != LUA_TSTRING || lua_type(InState, 2) != LUA_TSTRING)
		{
			return PushActionResult(InState, false, TEXT("grid.command expects target ObjectId/LogicId and command name strings."));
		}

		const char* TargetUtf8 = lua_tostring(InState, 1);
		const char* CommandUtf8 = lua_tostring(InState, 2);
		const FString Target = TargetUtf8 ? FString(UTF8_TO_TCHAR(TargetUtf8)) : FString();
		const FString Command = CommandUtf8 ? FString(UTF8_TO_TCHAR(CommandUtf8)) : FString();

		FString Error;
		const bool bSuccess = Self->ActiveHost->Command(Target, Command, Error);
		return PushActionResult(InState, bSuccess, Error);
	}

	static int LuaLog(lua_State* InState)
	{
		FImpl* Self = GetSelf(InState);
		if (!Self || !Self->ActiveHost || !Self->ActiveHost->Log)
		{
			return PushActionResult(InState, false, TEXT("grid.log is unavailable outside a hosted callback."));
		}
		if (lua_type(InState, 1) != LUA_TSTRING)
		{
			return PushActionResult(InState, false, TEXT("grid.log expects a string."));
		}

		const char* MessageUtf8 = lua_tostring(InState, 1);
		Self->ActiveHost->Log(MessageUtf8 ? FString(UTF8_TO_TCHAR(MessageUtf8)) : FString());
		return PushActionResult(InState, true, FString());
	}

	bool Initialize(FString& OutError)
	{
		State = lua_newstate(&FImpl::LuaAllocator, this);
		if (!State)
		{
			OutError = TEXT("Unable to create Lua state within the configured memory quota.");
			return false;
		}

		*static_cast<FImpl**>(lua_getextraspace(State)) = this;

		luaL_requiref(State, "_G", luaopen_base, 1);
		lua_pop(State, 1);
		luaL_requiref(State, LUA_MATHLIBNAME, luaopen_math, 1);
		lua_pop(State, 1);
		luaL_requiref(State, LUA_STRLIBNAME, luaopen_string, 1);
		lua_pop(State, 1);
		luaL_requiref(State, LUA_TABLIBNAME, luaopen_table, 1);
		lua_pop(State, 1);
		luaL_requiref(State, LUA_UTF8LIBNAME, luaopen_utf8, 1);
		lua_pop(State, 1);

		OutError.Reset();
		return true;
	}

	void Close()
	{
		ActiveHost = nullptr;
		PersistentDefinitionsByScriptId.Reset();
		PersistentDefinitionByVariableId.Reset();
		EnvironmentRefByScriptId.Reset();
		if (State)
		{
			lua_sethook(State, nullptr, 0, 0);
			lua_close(State);
			State = nullptr;
		}
		AllocatedBytes = 0;
		RemainingInstructionBudget = 0;
	}

	void PushSafeEnvironment()
	{
		lua_newtable(State);
		const int32 EnvironmentIndex = lua_absindex(State, -1);

		static const char* SafeBaseNames[] = { "assert", "error", "ipairs", "next", "pairs", "pcall", "rawequal", "rawget", "rawlen", "rawset", "select",
			"tonumber", "tostring", "type", "xpcall", "_VERSION" };

		for (const char* Name : SafeBaseNames)
		{
			lua_getglobal(State, Name);
			if (!lua_isnil(State, -1))
			{
				lua_setfield(State, EnvironmentIndex, Name);
			}
			else
			{
				lua_pop(State, 1);
			}
		}

		static const char* SafeLibraryNames[] = { LUA_MATHLIBNAME, LUA_STRLIBNAME, LUA_TABLIBNAME, LUA_UTF8LIBNAME };

		for (const char* LibraryName : SafeLibraryNames)
		{
			lua_getglobal(State, LibraryName);
			if (lua_istable(State, -1))
			{
				CloneLuaTable(State, -1);
				if (FCStringAnsi::Strcmp(LibraryName, LUA_STRLIBNAME) == 0)
				{
					// Source-only sandbox: scripts cannot generate reusable
					// Lua bytecode even though binary chunks are also rejected
					// by luaL_loadbufferx(mode="t").
					lua_pushnil(State);
					lua_setfield(State, -2, "dump");
				}
				lua_setfield(State, EnvironmentIndex, LibraryName);
			}
			lua_pop(State, 1);
		}

		lua_pushvalue(State, EnvironmentIndex);
		lua_setfield(State, EnvironmentIndex, "_G");
	}

	void PushGridApi()
	{
		lua_newtable(State);
		const int32 GridIndex = lua_absindex(State, -1);

		lua_newtable(State);
		const int32 VarsIndex = lua_absindex(State, -1);
		lua_pushcfunction(State, &FImpl::LuaGetBool);
		lua_setfield(State, VarsIndex, "get_bool");
		lua_pushcfunction(State, &FImpl::LuaSetBool);
		lua_setfield(State, VarsIndex, "set_bool");
		lua_pushcfunction(State, &FImpl::LuaGetInt);
		lua_setfield(State, VarsIndex, "get_int");
		lua_pushcfunction(State, &FImpl::LuaSetInt);
		lua_setfield(State, VarsIndex, "set_int");
		lua_setfield(State, GridIndex, "vars");

		lua_pushcfunction(State, &FImpl::LuaCommand);
		lua_setfield(State, GridIndex, "command");
		lua_pushcfunction(State, &FImpl::LuaLog);
		lua_setfield(State, GridIndex, "log");
	}

	bool ExtractPersistentDefinitions(FName ScriptId, int32 EnvironmentRef, FString& OutError)
	{
		const int32 BaseTop = lua_gettop(State);
		TArray<FGridLuaPersistentVariableDefinition> ScriptDefinitions;

		lua_rawgeti(State, LUA_REGISTRYINDEX, EnvironmentRef);
		lua_getfield(State, -1, "persistent");
		if (lua_isnil(State, -1))
		{
			PersistentDefinitionsByScriptId.Add(ScriptId, MoveTemp(ScriptDefinitions));
			lua_settop(State, BaseTop);
			OutError.Reset();
			return true;
		}
		if (!lua_istable(State, -1))
		{
			OutError = FString::Printf(TEXT("Lua script '%s' declares 'persistent' but it is not a table."), *ScriptId.ToString());
			lua_settop(State, BaseTop);
			return false;
		}

		const int32 PersistentIndex = lua_absindex(State, -1);
		lua_pushnil(State);
		while (lua_next(State, PersistentIndex) != 0)
		{
			if (lua_type(State, -2) != LUA_TSTRING)
			{
				OutError = FString::Printf(TEXT("Lua script '%s' persistent keys must be strings."), *ScriptId.ToString());
				lua_settop(State, BaseTop);
				return false;
			}

			const char* KeyUtf8 = lua_tostring(State, -2);
			const FString VariableText = KeyUtf8 ? FString(UTF8_TO_TCHAR(KeyUtf8)).TrimStartAndEnd() : FString();
			if (VariableText.IsEmpty())
			{
				OutError = FString::Printf(TEXT("Lua script '%s' contains an empty persistent variable name."), *ScriptId.ToString());
				lua_settop(State, BaseTop);
				return false;
			}

			FGridLuaPersistentVariableDefinition Definition;
			Definition.VariableId = FName(*VariableText);
			if (lua_isboolean(State, -1))
			{
				Definition.Type = EGridLuaPersistentValueType::Bool;
				Definition.bDefaultBoolValue = lua_toboolean(State, -1) != 0;
			}
			else if (lua_isinteger(State, -1))
			{
				const lua_Integer LuaValue = lua_tointeger(State, -1);
				if (LuaValue < static_cast<lua_Integer>(MIN_int32) || LuaValue > static_cast<lua_Integer>(MAX_int32))
				{
					OutError = FString::Printf(TEXT("Lua persistent variable '%s' is outside the Int32 range."), *Definition.VariableId.ToString());
					lua_settop(State, BaseTop);
					return false;
				}
				Definition.Type = EGridLuaPersistentValueType::Int32;
				Definition.DefaultInt32Value = static_cast<int32>(LuaValue);
			}
			else
			{
				OutError = FString::Printf(TEXT("Lua persistent variable '%s' must have a Bool or Int32 literal default."), *Definition.VariableId.ToString());
				lua_settop(State, BaseTop);
				return false;
			}

			if (const FGridLuaPersistentVariableDefinition* Existing = PersistentDefinitionByVariableId.Find(Definition.VariableId))
			{
				if (!ArePersistentDefinitionsCompatible(*Existing, Definition))
				{
					OutError = FString::Printf(
						TEXT("Lua persistent variable '%s' is declared with conflicting type/default values."), *Definition.VariableId.ToString());
					lua_settop(State, BaseTop);
					return false;
				}
			}
			else
			{
				PersistentDefinitionByVariableId.Add(Definition.VariableId, Definition);
			}

			ScriptDefinitions.Add(Definition);
			lua_pop(State, 1);
		}

		ScriptDefinitions.Sort(
			[](const FGridLuaPersistentVariableDefinition& A, const FGridLuaPersistentVariableDefinition& B)
			{
				return A.VariableId.LexicalLess(B.VariableId);
			});
		PersistentDefinitionsByScriptId.Add(ScriptId, MoveTemp(ScriptDefinitions));
		lua_settop(State, BaseTop);
		OutError.Reset();
		return true;
	}

	bool SyncPersistentTableFromHost(
		FName ScriptId, const FGridLuaHostApi& HostApi, TMap<FName, bool>& OutInitialBoolValues, TMap<FName, int32>& OutInitialIntValues, FString& OutError)
	{
		OutInitialBoolValues.Reset();
		OutInitialIntValues.Reset();
		const TArray<FGridLuaPersistentVariableDefinition>* Definitions = PersistentDefinitionsByScriptId.Find(ScriptId);
		if (!Definitions || Definitions->IsEmpty())
		{
			OutError.Reset();
			return true;
		}

		const int32* EnvironmentRef = EnvironmentRefByScriptId.Find(ScriptId);
		if (!EnvironmentRef)
		{
			OutError = FString::Printf(TEXT("Lua script '%s' is not loaded."), *ScriptId.ToString());
			return false;
		}

		const int32 BaseTop = lua_gettop(State);
		lua_rawgeti(State, LUA_REGISTRYINDEX, *EnvironmentRef);
		lua_getfield(State, -1, "persistent");
		if (!lua_istable(State, -1))
		{
			OutError = FString::Printf(TEXT("Lua script '%s' persistent table disappeared after initialization."), *ScriptId.ToString());
			lua_settop(State, BaseTop);
			return false;
		}
		const int32 PersistentIndex = lua_absindex(State, -1);

		for (const FGridLuaPersistentVariableDefinition& Definition : *Definitions)
		{
			const FString VariableText = Definition.VariableId.ToString();
			FTCHARToUTF8 VariableUtf8(*VariableText);
			FString HostError;
			if (Definition.Type == EGridLuaPersistentValueType::Bool)
			{
				bool bValue = false;
				if (!HostApi.GetBool || !HostApi.GetBool(Definition.VariableId, bValue, HostError))
				{
					OutError = FString::Printf(
						TEXT("Cannot synchronize persistent Bool '%s': %s"), *VariableText, HostError.IsEmpty() ? TEXT("host getter unavailable") : *HostError);
					lua_settop(State, BaseTop);
					return false;
				}
				OutInitialBoolValues.Add(Definition.VariableId, bValue);
				lua_pushboolean(State, bValue ? 1 : 0);
			}
			else
			{
				int32 Value = 0;
				if (!HostApi.GetInt32 || !HostApi.GetInt32(Definition.VariableId, Value, HostError))
				{
					OutError = FString::Printf(TEXT("Cannot synchronize persistent Int32 '%s': %s"), *VariableText,
						HostError.IsEmpty() ? TEXT("host getter unavailable") : *HostError);
					lua_settop(State, BaseTop);
					return false;
				}
				OutInitialIntValues.Add(Definition.VariableId, Value);
				lua_pushinteger(State, static_cast<lua_Integer>(Value));
			}
			lua_setfield(State, PersistentIndex, VariableUtf8.Get());
		}

		lua_settop(State, BaseTop);
		OutError.Reset();
		return true;
	}

	bool CommitPersistentTableToHost(FName ScriptId, const FGridLuaHostApi& HostApi, const TMap<FName, bool>& InitialBoolValues,
		const TMap<FName, int32>& InitialIntValues, FString& OutError)
	{
		const TArray<FGridLuaPersistentVariableDefinition>* Definitions = PersistentDefinitionsByScriptId.Find(ScriptId);
		if (!Definitions || Definitions->IsEmpty())
		{
			OutError.Reset();
			return true;
		}

		const int32* EnvironmentRef = EnvironmentRefByScriptId.Find(ScriptId);
		if (!EnvironmentRef)
		{
			OutError = FString::Printf(TEXT("Lua script '%s' is not loaded."), *ScriptId.ToString());
			return false;
		}

		TSet<FName> DeclaredIds;
		for (const FGridLuaPersistentVariableDefinition& Definition : *Definitions)
		{
			DeclaredIds.Add(Definition.VariableId);
		}

		TMap<FName, bool> ChangedBoolValues;
		TMap<FName, int32> ChangedIntValues;
		const int32 BaseTop = lua_gettop(State);
		lua_rawgeti(State, LUA_REGISTRYINDEX, *EnvironmentRef);
		lua_getfield(State, -1, "persistent");
		if (!lua_istable(State, -1))
		{
			OutError = FString::Printf(TEXT("Lua script '%s' persistent table is no longer a table."), *ScriptId.ToString());
			lua_settop(State, BaseTop);
			return false;
		}
		const int32 PersistentIndex = lua_absindex(State, -1);

		lua_pushnil(State);
		while (lua_next(State, PersistentIndex) != 0)
		{
			if (lua_type(State, -2) != LUA_TSTRING)
			{
				OutError = TEXT("Lua persistent table gained a non-string key during callback execution.");
				lua_settop(State, BaseTop);
				return false;
			}
			const char* KeyUtf8 = lua_tostring(State, -2);
			const FName VariableId(KeyUtf8 ? UTF8_TO_TCHAR(KeyUtf8) : TEXT(""));
			if (VariableId.IsNone() || !DeclaredIds.Contains(VariableId))
			{
				OutError = FString::Printf(
					TEXT("Lua persistent variable '%s' was not declared in the top-level persistent table."), KeyUtf8 ? UTF8_TO_TCHAR(KeyUtf8) : TEXT(""));
				lua_settop(State, BaseTop);
				return false;
			}
			lua_pop(State, 1);
		}

		for (const FGridLuaPersistentVariableDefinition& Definition : *Definitions)
		{
			const FString VariableText = Definition.VariableId.ToString();
			FTCHARToUTF8 VariableUtf8(*VariableText);
			lua_getfield(State, PersistentIndex, VariableUtf8.Get());
			if (Definition.Type == EGridLuaPersistentValueType::Bool)
			{
				if (!lua_isboolean(State, -1))
				{
					OutError = FString::Printf(TEXT("Lua persistent variable '%s' must remain a Bool."), *VariableText);
					lua_settop(State, BaseTop);
					return false;
				}
				const bool bValue = lua_toboolean(State, -1) != 0;
				const bool* InitialValue = InitialBoolValues.Find(Definition.VariableId);
				if (!InitialValue || *InitialValue != bValue)
				{
					ChangedBoolValues.Add(Definition.VariableId, bValue);
				}
			}
			else
			{
				if (!lua_isinteger(State, -1))
				{
					OutError = FString::Printf(TEXT("Lua persistent variable '%s' must remain an Int32."), *VariableText);
					lua_settop(State, BaseTop);
					return false;
				}
				const lua_Integer LuaValue = lua_tointeger(State, -1);
				if (LuaValue < static_cast<lua_Integer>(MIN_int32) || LuaValue > static_cast<lua_Integer>(MAX_int32))
				{
					OutError = FString::Printf(TEXT("Lua persistent variable '%s' moved outside the Int32 range."), *VariableText);
					lua_settop(State, BaseTop);
					return false;
				}
				const int32 Value = static_cast<int32>(LuaValue);
				const int32* InitialValue = InitialIntValues.Find(Definition.VariableId);
				if (!InitialValue || *InitialValue != Value)
				{
					ChangedIntValues.Add(Definition.VariableId, Value);
				}
			}
			lua_pop(State, 1);
		}
		lua_settop(State, BaseTop);

		for (const TPair<FName, bool>& Pair : ChangedBoolValues)
		{
			FString HostError;
			if (!HostApi.SetBool || !HostApi.SetBool(Pair.Key, Pair.Value, HostError))
			{
				OutError = FString::Printf(
					TEXT("Cannot commit persistent Bool '%s': %s"), *Pair.Key.ToString(), HostError.IsEmpty() ? TEXT("host setter unavailable") : *HostError);
				return false;
			}
		}
		for (const TPair<FName, int32>& Pair : ChangedIntValues)
		{
			FString HostError;
			if (!HostApi.SetInt32 || !HostApi.SetInt32(Pair.Key, Pair.Value, HostError))
			{
				OutError = FString::Printf(
					TEXT("Cannot commit persistent Int32 '%s': %s"), *Pair.Key.ToString(), HostError.IsEmpty() ? TEXT("host setter unavailable") : *HostError);
				return false;
			}
		}

		OutError.Reset();
		return true;
	}

	bool ExecuteProtectedFunction(int32 ArgumentCount, int32 ResultCount, FString& OutError)
	{
		RemainingInstructionBudget = Config.InstructionBudgetPerCall;
		HookStep = FMath::Clamp(Config.InstructionBudgetPerCall, 1, MaximumHookStep);
		lua_sethook(State, &FImpl::InstructionHook, LUA_MASKCOUNT, HookStep);

		const int32 Status = lua_pcall(State, ArgumentCount, ResultCount, 0);

		lua_sethook(State, nullptr, 0, 0);
		RemainingInstructionBudget = 0;

		if (Status != LUA_OK)
		{
			OutError = GetLuaStackError(State, TEXT("Lua protected call failed without an error string."));
			return false;
		}

		OutError.Reset();
		return true;
	}

	bool LoadScript(const FGridLuaScriptSource& Script, FString& OutError)
	{
		const int32 BaseTop = lua_gettop(State);

		PushSafeEnvironment();
		const int32 EnvironmentRef = luaL_ref(State, LUA_REGISTRYINDEX);

		const FString ScriptIdString = Script.ScriptId.ToString();
		const FString ChunkName = FString::Printf(TEXT("@Grimrock/%s.lua"), *ScriptIdString);
		FTCHARToUTF8 SourceUtf8(*Script.Source);
		FTCHARToUTF8 ChunkNameUtf8(*ChunkName);

		const int32 LoadStatus = luaL_loadbufferx(State, SourceUtf8.Get(), static_cast<size_t>(SourceUtf8.Length()), ChunkNameUtf8.Get(), "t");
		if (LoadStatus != LUA_OK)
		{
			OutError = FString::Printf(TEXT("Script '%s' failed to compile: %s"), *ScriptIdString, *GetLuaStackError(State, TEXT("unknown compile error")));
			luaL_unref(State, LUA_REGISTRYINDEX, EnvironmentRef);
			lua_settop(State, BaseTop);
			return false;
		}

		lua_rawgeti(State, LUA_REGISTRYINDEX, EnvironmentRef);
		const char* UpvalueName = lua_setupvalue(State, -2, 1);
		if (!UpvalueName || FCStringAnsi::Strcmp(UpvalueName, "_ENV") != 0)
		{
			OutError = FString::Printf(TEXT("Script '%s' has no assignable _ENV upvalue."), *ScriptIdString);
			luaL_unref(State, LUA_REGISTRYINDEX, EnvironmentRef);
			lua_settop(State, BaseTop);
			return false;
		}

		FString ExecutionError;
		if (!ExecuteProtectedFunction(0, 0, ExecutionError))
		{
			OutError = FString::Printf(TEXT("Script '%s' failed during initialization: %s"), *ScriptIdString, *ExecutionError);
			luaL_unref(State, LUA_REGISTRYINDEX, EnvironmentRef);
			lua_settop(State, BaseTop);
			return false;
		}

		if (!ExtractPersistentDefinitions(Script.ScriptId, EnvironmentRef, OutError))
		{
			luaL_unref(State, LUA_REGISTRYINDEX, EnvironmentRef);
			lua_settop(State, BaseTop);
			return false;
		}

		// Install the safe host bridge only after top-level initialization.
		// Native functions remain inert unless CallEventFunction binds a host.
		lua_rawgeti(State, LUA_REGISTRYINDEX, EnvironmentRef);
		PushGridApi();
		lua_setfield(State, -2, "grid");
		lua_pop(State, 1);

		EnvironmentRefByScriptId.Add(Script.ScriptId, EnvironmentRef);
		lua_settop(State, BaseTop);
		OutError.Reset();
		return true;
	}

	bool PushScriptGlobal(FName ScriptId, FName GlobalName, FString& OutError) const
	{
		if (!State)
		{
			OutError = TEXT("Lua VM is not ready.");
			return false;
		}
		if (ScriptId.IsNone() || GlobalName.IsNone())
		{
			OutError = TEXT("ScriptId and global name must be non-empty.");
			return false;
		}

		const int32* EnvironmentRef = EnvironmentRefByScriptId.Find(ScriptId);
		if (!EnvironmentRef)
		{
			OutError = FString::Printf(TEXT("Lua script '%s' is not loaded."), *ScriptId.ToString());
			return false;
		}

		lua_rawgeti(State, LUA_REGISTRYINDEX, *EnvironmentRef);
		FTCHARToUTF8 GlobalNameUtf8(*GlobalName.ToString());
		lua_getfield(State, -1, GlobalNameUtf8.Get());
		lua_remove(State, -2);
		OutError.Reset();
		return true;
	}
};

FGridLuaVm::FGridLuaVm() = default;
FGridLuaVm::~FGridLuaVm() = default;

bool FGridLuaVm::Reload(const TArray<FGridLuaScriptSource>& Scripts, const FGridLuaVmConfig& Config, FString& OutError)
{
	if (!ValidateScriptDefinitions(Scripts, OutError))
	{
		return false;
	}
	if (Config.MemoryLimitBytes < MinimumLuaMemoryLimitBytes)
	{
		OutError = FString::Printf(TEXT("Lua memory quota must be at least %llu bytes."), static_cast<uint64>(MinimumLuaMemoryLimitBytes));
		return false;
	}
	if (Config.InstructionBudgetPerCall <= 0)
	{
		OutError = TEXT("Lua instruction budget must be greater than zero.");
		return false;
	}

	TUniquePtr<FImpl> Candidate = MakeUnique<FImpl>();
	Candidate->Config = Config;
	if (!Candidate->Initialize(OutError))
	{
		return false;
	}

	for (const FGridLuaScriptSource& Script : Scripts)
	{
		if (!Script.bEnabled)
		{
			continue;
		}
		if (!Candidate->LoadScript(Script, OutError))
		{
			return false;
		}
	}

	Impl = MoveTemp(Candidate);
	OutError.Reset();
	return true;
}

void FGridLuaVm::Reset()
{
	Impl.Reset();
}

bool FGridLuaVm::IsReady() const
{
	return Impl.IsValid() && Impl->State != nullptr;
}

int32 FGridLuaVm::GetLoadedScriptCount() const
{
	return IsReady() ? Impl->EnvironmentRefByScriptId.Num() : 0;
}

bool FGridLuaVm::HasScript(FName ScriptId) const
{
	return IsReady() && Impl->EnvironmentRefByScriptId.Contains(ScriptId);
}

TArray<FGridLuaPersistentVariableDefinition> FGridLuaVm::GetPersistentVariableDefinitions() const
{
	TArray<FGridLuaPersistentVariableDefinition> Result;
	if (!IsReady())
	{
		return Result;
	}

	Impl->PersistentDefinitionByVariableId.GenerateValueArray(Result);
	Result.Sort(
		[](const FGridLuaPersistentVariableDefinition& A, const FGridLuaPersistentVariableDefinition& B)
		{
			return A.VariableId.LexicalLess(B.VariableId);
		});
	return Result;
}

bool FGridLuaVm::CallFunction(FName ScriptId, FName FunctionName, FString& OutError)
{
	if (!IsReady())
	{
		OutError = TEXT("Lua VM is not ready.");
		return false;
	}

	const int32 BaseTop = lua_gettop(Impl->State);
	if (!Impl->PushScriptGlobal(ScriptId, FunctionName, OutError))
	{
		lua_settop(Impl->State, BaseTop);
		return false;
	}

	if (!lua_isfunction(Impl->State, -1))
	{
		OutError = FString::Printf(TEXT("Lua '%s.%s' is not a function."), *ScriptId.ToString(), *FunctionName.ToString());
		lua_settop(Impl->State, BaseTop);
		return false;
	}

	FString ExecutionError;
	const bool bSuccess = Impl->ExecuteProtectedFunction(0, 0, ExecutionError);
	if (!bSuccess)
	{
		OutError = FString::Printf(TEXT("Lua callback '%s.%s' failed: %s"), *ScriptId.ToString(), *FunctionName.ToString(), *ExecutionError);
	}

	lua_settop(Impl->State, BaseTop);
	if (bSuccess)
	{
		OutError.Reset();
	}
	return bSuccess;
}

bool FGridLuaVm::CallEventFunction(
	FName ScriptId, FName FunctionName, const FGridLuaEventContext& EventContext, const FGridLuaHostApi& HostApi, FString& OutError)
{
	if (!IsReady())
	{
		OutError = TEXT("Lua VM is not ready.");
		return false;
	}
	if (Impl->ActiveHost)
	{
		OutError = TEXT("Nested hosted Lua callbacks are not supported.");
		return false;
	}

	const int32 BaseTop = lua_gettop(Impl->State);
	TMap<FName, bool> InitialBoolValues;
	TMap<FName, int32> InitialIntValues;
	if (!Impl->SyncPersistentTableFromHost(ScriptId, HostApi, InitialBoolValues, InitialIntValues, OutError))
	{
		lua_settop(Impl->State, BaseTop);
		return false;
	}

	if (!Impl->PushScriptGlobal(ScriptId, FunctionName, OutError))
	{
		lua_settop(Impl->State, BaseTop);
		return false;
	}
	if (!lua_isfunction(Impl->State, -1))
	{
		OutError = FString::Printf(TEXT("Lua '%s.%s' is not a function."), *ScriptId.ToString(), *FunctionName.ToString());
		lua_settop(Impl->State, BaseTop);
		return false;
	}

	lua_newtable(Impl->State);
	FTCHARToUTF8 SourceUtf8(*EventContext.SourceObjectId);
	lua_pushstring(Impl->State, SourceUtf8.Get());
	lua_setfield(Impl->State, -2, "source_object_id");
	FTCHARToUTF8 EventUtf8(*EventContext.EventName);
	lua_pushstring(Impl->State, EventUtf8.Get());
	lua_setfield(Impl->State, -2, "event");

	Impl->ActiveHost = &HostApi;
	FString ExecutionError;
	bool bSuccess = Impl->ExecuteProtectedFunction(1, 0, ExecutionError);
	Impl->ActiveHost = nullptr;

	if (!bSuccess)
	{
		OutError = FString::Printf(TEXT("Lua event callback '%s.%s' failed: %s"), *ScriptId.ToString(), *FunctionName.ToString(), *ExecutionError);
	}
	else
	{
		FString CommitError;
		if (!Impl->CommitPersistentTableToHost(ScriptId, HostApi, InitialBoolValues, InitialIntValues, CommitError))
		{
			bSuccess = false;
			OutError =
				FString::Printf(TEXT("Lua event callback '%s.%s' persistent commit failed: %s"), *ScriptId.ToString(), *FunctionName.ToString(), *CommitError);
		}
	}

	lua_settop(Impl->State, BaseTop);
	if (bSuccess)
	{
		OutError.Reset();
	}
	return bSuccess;
}

bool FGridLuaVm::TryGetScriptBool(FName ScriptId, FName GlobalName, bool& OutValue, FString& OutError) const
{
	if (!IsReady())
	{
		OutError = TEXT("Lua VM is not ready.");
		return false;
	}

	const int32 BaseTop = lua_gettop(Impl->State);
	if (!Impl->PushScriptGlobal(ScriptId, GlobalName, OutError))
	{
		lua_settop(Impl->State, BaseTop);
		return false;
	}
	if (!lua_isboolean(Impl->State, -1))
	{
		OutError = FString::Printf(TEXT("Lua '%s.%s' is not a Bool."), *ScriptId.ToString(), *GlobalName.ToString());
		lua_settop(Impl->State, BaseTop);
		return false;
	}

	OutValue = lua_toboolean(Impl->State, -1) != 0;
	lua_settop(Impl->State, BaseTop);
	OutError.Reset();
	return true;
}

bool FGridLuaVm::TryGetScriptInt32(FName ScriptId, FName GlobalName, int32& OutValue, FString& OutError) const
{
	if (!IsReady())
	{
		OutError = TEXT("Lua VM is not ready.");
		return false;
	}

	const int32 BaseTop = lua_gettop(Impl->State);
	if (!Impl->PushScriptGlobal(ScriptId, GlobalName, OutError))
	{
		lua_settop(Impl->State, BaseTop);
		return false;
	}
	if (!lua_isinteger(Impl->State, -1))
	{
		OutError = FString::Printf(TEXT("Lua '%s.%s' is not an integer."), *ScriptId.ToString(), *GlobalName.ToString());
		lua_settop(Impl->State, BaseTop);
		return false;
	}

	const lua_Integer LuaValue = lua_tointeger(Impl->State, -1);
	if (LuaValue < static_cast<lua_Integer>(MIN_int32) || LuaValue > static_cast<lua_Integer>(MAX_int32))
	{
		OutError = FString::Printf(TEXT("Lua '%s.%s' is outside the Int32 range."), *ScriptId.ToString(), *GlobalName.ToString());
		lua_settop(Impl->State, BaseTop);
		return false;
	}

	OutValue = static_cast<int32>(LuaValue);
	lua_settop(Impl->State, BaseTop);
	OutError.Reset();
	return true;
}

FString FGridLuaVm::GetVersionString() const
{
	return FString(UTF8_TO_TCHAR(LUA_RELEASE));
}

SIZE_T FGridLuaVm::GetAllocatedBytes() const
{
	return IsReady() ? Impl->AllocatedBytes : 0;
}

bool FGridLuaVm::ValidateScriptDefinitions(const TArray<FGridLuaScriptSource>& Scripts, FString& OutError)
{
	if (Scripts.Num() > HardMaxScriptCount)
	{
		OutError = FString::Printf(TEXT("Lua level defines %d scripts; hard limit is %d."), Scripts.Num(), HardMaxScriptCount);
		return false;
	}

	TSet<FName> SeenIds;
	SIZE_T TotalSourceBytes = 0;
	for (int32 Index = 0; Index < Scripts.Num(); ++Index)
	{
		const FGridLuaScriptSource& Script = Scripts[Index];
		if (Script.ScriptId.IsNone())
		{
			OutError = FString::Printf(TEXT("Lua script entry %d has an empty ScriptId."), Index);
			return false;
		}
		if (SeenIds.Contains(Script.ScriptId))
		{
			OutError = FString::Printf(TEXT("Lua ScriptId '%s' is duplicated."), *Script.ScriptId.ToString());
			return false;
		}
		SeenIds.Add(Script.ScriptId);

		FTCHARToUTF8 SourceUtf8(*Script.Source);
		const SIZE_T SourceBytes = static_cast<SIZE_T>(SourceUtf8.Length());
		if (SourceBytes > HardMaxSourceBytesPerScript)
		{
			OutError = FString::Printf(TEXT("Lua script '%s' uses %llu UTF-8 source bytes; hard per-script limit is %llu."), *Script.ScriptId.ToString(),
				static_cast<uint64>(SourceBytes), static_cast<uint64>(HardMaxSourceBytesPerScript));
			return false;
		}
		if (SourceBytes > HardMaxTotalSourceBytes || TotalSourceBytes > HardMaxTotalSourceBytes - SourceBytes)
		{
			OutError =
				FString::Printf(TEXT("Lua level source exceeds the hard total limit of %llu UTF-8 bytes."), static_cast<uint64>(HardMaxTotalSourceBytes));
			return false;
		}
		TotalSourceBytes += SourceBytes;
	}

	OutError.Reset();
	return true;
}

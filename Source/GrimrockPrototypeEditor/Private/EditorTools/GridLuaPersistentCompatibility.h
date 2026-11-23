#pragma once

#include "Core/GridLevelVariableTypes.h"
#include "GridLuaVm.h"

// Comparison, not declaration validation: the existing Lua VM validates allowed
// identifiers/types before either Editor caller reaches these predicates.
namespace GridLuaPersistentCompatibility
{
	inline bool AreDefinitionsCompatible(const FGridLuaPersistentVariableDefinition& A, const FGridLuaPersistentVariableDefinition& B)
	{
		if (A.VariableId != B.VariableId || A.Type != B.Type)
		{
			return false;
		}
		return A.Type == EGridLuaPersistentValueType::Bool ? A.bDefaultBoolValue == B.bDefaultBoolValue : A.DefaultInt32Value == B.DefaultInt32Value;
	}

	inline bool MatchesLevelVariable(const FGridLuaPersistentVariableDefinition& LuaDefinition, const FGridLevelVariableDefinition& LevelDefinition)
	{
		if (LuaDefinition.VariableId != LevelDefinition.VariableId)
		{
			return false;
		}
		if (LuaDefinition.Type == EGridLuaPersistentValueType::Bool)
		{
			return LevelDefinition.Type == EGridLevelVariableType::Bool && LevelDefinition.bDefaultBoolValue == LuaDefinition.bDefaultBoolValue;
		}
		return LevelDefinition.Type == EGridLevelVariableType::Int32 && LevelDefinition.DefaultInt32Value == LuaDefinition.DefaultInt32Value;
	}
}

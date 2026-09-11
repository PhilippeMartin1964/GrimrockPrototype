#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"
#include "GridLuaScriptTypes.h"

class AGridLevelEditorActor;

/** LUA-COMP01 one authoring-compiler diagnostic. */
struct GRIMROCKPROTOTYPEEDITOR_API FGridLuaCompileDiagnostic
{
	FString Code;
	FName ScriptId = NAME_None;
	int32 Line = 1;
	int32 Column = 1;
	FString Message;

	FString ToDisplayString() const;
};

/** LUA-COMP01 compile result for one level/candidate script set. */
struct GRIMROCKPROTOTYPEEDITOR_API FGridLuaCompileResult
{
	int32 EnabledScriptCount = 0;
	TArray<FGridLuaCompileDiagnostic> Diagnostics;

	bool Succeeded() const
	{
		return Diagnostics.IsEmpty();
	}

	FString GetFirstErrorText() const;
	FString GetSummaryText(int32 MaxDiagnostics = 8) const;
};

/**
 * LUA-COMP01 — Grimrock Lua authoring compiler.
 *
 * Pass 1 delegates Lua grammar/initialization/persistent declaration checks to
 * FGridLuaVm. Pass 2 statically validates the level-facing API against the
 * current UGridLevelAsset and Grid Editor definition registry.
 */
class GRIMROCKPROTOTYPEEDITOR_API FGridLuaAuthoringCompiler
{
public:
	/** Compile the scripts and Lua bindings currently stored by the editor level. */
	static bool CompileLevel(const AGridLevelEditorActor& EditorActor, FGridLuaCompileResult& OutResult);

	/** Compile an arbitrary candidate script/link set against the current level objects. */
	static bool CompileCandidate(const AGridLevelEditorActor& EditorActor, const TArray<FGridLuaScriptSource>& CandidateScripts,
		const TArray<FGridObjectLink>& CandidateLinks, FGridLuaCompileResult& OutResult);

	/**
	 * Builds the exact candidate produced by renaming/editing one script, updates
	 * candidate binding ScriptIds in memory only, and compiles it without touching the asset.
	 */
	static bool CompileScriptDraft(const AGridLevelEditorActor& EditorActor, FName OldScriptId, FName NewScriptId,
		const FString& CandidateSource, FGridLuaCompileResult& OutResult);
};

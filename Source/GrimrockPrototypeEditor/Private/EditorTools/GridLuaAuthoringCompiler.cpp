#include "EditorTools/GridLuaAuthoringCompiler.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridLevelVariableTypes.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridEditorLinkPolicy.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/StaticMesh.h"
#include "GridLuaVm.h"
#include "Internationalization/Regex.h"
#include "Materials/MaterialInterface.h"

namespace
{
	enum class ELuaAuthoringTokenKind : uint8
	{
		Identifier,
		String,
		Number,
		Symbol
	};

	struct FLuaAuthoringToken
	{
		ELuaAuthoringTokenKind Kind = ELuaAuthoringTokenKind::Symbol;
		FString Text;
		int32 Line = 1;
		int32 Column = 1;
		bool bSimpleLiteral = true;
	};

	bool IsIdentifierStart(TCHAR Ch)
	{
		return FChar::IsAlpha(Ch) || Ch == TEXT('_');
	}

	bool IsIdentifierContinue(TCHAR Ch)
	{
		return FChar::IsAlnum(Ch) || Ch == TEXT('_');
	}

	bool IsSimpleLogicIdText(const FString& Text)
	{
		if (Text.IsEmpty() || !IsIdentifierStart(Text[0]))
		{
			return false;
		}
		for (int32 Index = 1; Index < Text.Len(); ++Index)
		{
			if (!IsIdentifierContinue(Text[Index]))
			{
				return false;
			}
		}
		return true;
	}

	bool TryReadLongBracketHeader(const FString& Source, int32 StartIndex, int32& OutEqualsCount, int32& OutContentStart)
	{
		if (!Source.IsValidIndex(StartIndex) || Source[StartIndex] != TEXT('['))
		{
			return false;
		}

		int32 Cursor = StartIndex + 1;
		int32 EqualsCount = 0;
		while (Source.IsValidIndex(Cursor) && Source[Cursor] == TEXT('='))
		{
			++EqualsCount;
			++Cursor;
		}
		if (!Source.IsValidIndex(Cursor) || Source[Cursor] != TEXT('['))
		{
			return false;
		}

		OutEqualsCount = EqualsCount;
		OutContentStart = Cursor + 1;
		return true;
	}

	bool IsLongBracketCloseAt(const FString& Source, int32 Index, int32 EqualsCount, int32& OutAfterClose)
	{
		if (!Source.IsValidIndex(Index) || Source[Index] != TEXT(']'))
		{
			return false;
		}
		int32 Cursor = Index + 1;
		for (int32 EqualsIndex = 0; EqualsIndex < EqualsCount; ++EqualsIndex)
		{
			if (!Source.IsValidIndex(Cursor) || Source[Cursor] != TEXT('='))
			{
				return false;
			}
			++Cursor;
		}
		if (!Source.IsValidIndex(Cursor) || Source[Cursor] != TEXT(']'))
		{
			return false;
		}
		OutAfterClose = Cursor + 1;
		return true;
	}

	void AdvancePosition(TCHAR Ch, int32& Line, int32& Column)
	{
		if (Ch == TEXT('\n'))
		{
			++Line;
			Column = 1;
		}
		else
		{
			++Column;
		}
	}

	void TokenizeLuaSource(const FString& Source, TArray<FLuaAuthoringToken>& OutTokens)
	{
		OutTokens.Reset();
		int32 Index = 0;
		int32 Line = 1;
		int32 Column = 1;

		while (Index < Source.Len())
		{
			const TCHAR Ch = Source[Index];
			if (FChar::IsWhitespace(Ch))
			{
				AdvancePosition(Ch, Line, Column);
				++Index;
				continue;
			}

			// Lua comments: -- line comment or --[=[ long comment ]=].
			if (Ch == TEXT('-') && Source.IsValidIndex(Index + 1) && Source[Index + 1] == TEXT('-'))
			{
				AdvancePosition(Source[Index], Line, Column);
				AdvancePosition(Source[Index + 1], Line, Column);
				Index += 2;

				int32 EqualsCount = 0;
				int32 ContentStart = INDEX_NONE;
				if (TryReadLongBracketHeader(Source, Index, EqualsCount, ContentStart))
				{
					while (Index < ContentStart)
					{
						AdvancePosition(Source[Index], Line, Column);
						++Index;
					}
					while (Index < Source.Len())
					{
						int32 AfterClose = INDEX_NONE;
						if (IsLongBracketCloseAt(Source, Index, EqualsCount, AfterClose))
						{
							while (Index < AfterClose)
							{
								AdvancePosition(Source[Index], Line, Column);
								++Index;
							}
							break;
						}
						AdvancePosition(Source[Index], Line, Column);
						++Index;
					}
					continue;
				}

				while (Index < Source.Len() && Source[Index] != TEXT('\n'))
				{
					AdvancePosition(Source[Index], Line, Column);
					++Index;
				}
				continue;
			}

			const int32 TokenLine = Line;
			const int32 TokenColumn = Column;

			if (IsIdentifierStart(Ch))
			{
				const int32 Start = Index;
				while (Index < Source.Len() && IsIdentifierContinue(Source[Index]))
				{
					AdvancePosition(Source[Index], Line, Column);
					++Index;
				}
				FLuaAuthoringToken& Token = OutTokens.AddDefaulted_GetRef();
				Token.Kind = ELuaAuthoringTokenKind::Identifier;
				Token.Text = Source.Mid(Start, Index - Start);
				Token.Line = TokenLine;
				Token.Column = TokenColumn;
				continue;
			}

			if (Ch == TEXT('\'') || Ch == TEXT('"'))
			{
				const TCHAR Quote = Ch;
				FString Value;
				bool bSimple = true;
				AdvancePosition(Source[Index], Line, Column);
				++Index;
				while (Index < Source.Len() && Source[Index] != Quote)
				{
					if (Source[Index] == TEXT('\\') && Source.IsValidIndex(Index + 1))
					{
						bSimple = false;
						AdvancePosition(Source[Index], Line, Column);
						++Index;
						const TCHAR Escaped = Source[Index];
						switch (Escaped)
						{
							case TEXT('n'): Value.AppendChar(TEXT('\n')); break;
							case TEXT('r'): Value.AppendChar(TEXT('\r')); break;
							case TEXT('t'): Value.AppendChar(TEXT('\t')); break;
							default: Value.AppendChar(Escaped); break;
						}
						AdvancePosition(Source[Index], Line, Column);
						++Index;
						continue;
					}
					Value.AppendChar(Source[Index]);
					AdvancePosition(Source[Index], Line, Column);
					++Index;
				}
				if (Index < Source.Len() && Source[Index] == Quote)
				{
					AdvancePosition(Source[Index], Line, Column);
					++Index;
				}
				FLuaAuthoringToken& Token = OutTokens.AddDefaulted_GetRef();
				Token.Kind = ELuaAuthoringTokenKind::String;
				Token.Text = MoveTemp(Value);
				Token.Line = TokenLine;
				Token.Column = TokenColumn;
				Token.bSimpleLiteral = bSimple;
				continue;
			}

			int32 EqualsCount = 0;
			int32 ContentStart = INDEX_NONE;
			if (TryReadLongBracketHeader(Source, Index, EqualsCount, ContentStart))
			{
				while (Index < ContentStart)
				{
					AdvancePosition(Source[Index], Line, Column);
					++Index;
				}
				const int32 ValueStart = Index;
				while (Index < Source.Len())
				{
					int32 AfterClose = INDEX_NONE;
					if (IsLongBracketCloseAt(Source, Index, EqualsCount, AfterClose))
					{
						const FString Value = Source.Mid(ValueStart, Index - ValueStart);
						while (Index < AfterClose)
						{
							AdvancePosition(Source[Index], Line, Column);
							++Index;
						}
						FLuaAuthoringToken& Token = OutTokens.AddDefaulted_GetRef();
						Token.Kind = ELuaAuthoringTokenKind::String;
						Token.Text = Value;
						Token.Line = TokenLine;
						Token.Column = TokenColumn;
						break;
					}
					AdvancePosition(Source[Index], Line, Column);
					++Index;
				}
				continue;
			}

			if (FChar::IsDigit(Ch))
			{
				const int32 Start = Index;
				while (Index < Source.Len() && (FChar::IsDigit(Source[Index]) || Source[Index] == TEXT('.') || Source[Index] == TEXT('x') ||
					Source[Index] == TEXT('X') || (Source[Index] >= TEXT('a') && Source[Index] <= TEXT('f')) ||
					(Source[Index] >= TEXT('A') && Source[Index] <= TEXT('F'))))
				{
					AdvancePosition(Source[Index], Line, Column);
					++Index;
				}
				FLuaAuthoringToken& Token = OutTokens.AddDefaulted_GetRef();
				Token.Kind = ELuaAuthoringTokenKind::Number;
				Token.Text = Source.Mid(Start, Index - Start);
				Token.Line = TokenLine;
				Token.Column = TokenColumn;
				continue;
			}

			FLuaAuthoringToken& Token = OutTokens.AddDefaulted_GetRef();
			Token.Kind = ELuaAuthoringTokenKind::Symbol;
			Token.Text = FString::Chr(Ch);
			Token.Line = TokenLine;
			Token.Column = TokenColumn;
			AdvancePosition(Ch, Line, Column);
			++Index;
		}
	}

	bool TokenIs(const TArray<FLuaAuthoringToken>& Tokens, int32 Index, const TCHAR* Text)
	{
		return Tokens.IsValidIndex(Index) && Tokens[Index].Text == Text;
	}

	bool IdentifierIs(const TArray<FLuaAuthoringToken>& Tokens, int32 Index, const TCHAR* Text)
	{
		return Tokens.IsValidIndex(Index) && Tokens[Index].Kind == ELuaAuthoringTokenKind::Identifier && Tokens[Index].Text == Text;
	}

	void AddDiagnostic(FGridLuaCompileResult& Result, const TCHAR* Code, FName ScriptId, const FLuaAuthoringToken* Token, const FString& Message)
	{
		FGridLuaCompileDiagnostic& Diagnostic = Result.Diagnostics.AddDefaulted_GetRef();
		Diagnostic.Code = Code;
		Diagnostic.ScriptId = ScriptId;
		Diagnostic.Line = Token ? Token->Line : 1;
		Diagnostic.Column = Token ? Token->Column : 1;
		Diagnostic.Message = Message;
	}

	void AddDiagnostic(FGridLuaCompileResult& Result, const TCHAR* Code, FName ScriptId, int32 Line, int32 Column, const FString& Message)
	{
		FGridLuaCompileDiagnostic& Diagnostic = Result.Diagnostics.AddDefaulted_GetRef();
		Diagnostic.Code = Code;
		Diagnostic.ScriptId = ScriptId;
		Diagnostic.Line = FMath::Max(1, Line);
		Diagnostic.Column = FMath::Max(1, Column);
		Diagnostic.Message = Message;
	}

	int32 ExtractLuaErrorLine(const FString& Error)
	{
		static const FRegexPattern LinePattern(TEXT(":([0-9]+):"));
		FRegexMatcher Matcher(LinePattern, Error);
		return Matcher.FindNext() ? FMath::Max(1, FCString::Atoi(*Matcher.GetCaptureGroup(1))) : 1;
	}

	const FGridLevelVariableDefinition* FindLevelVariable(const UGridLevelAsset& LevelAsset, FName VariableId)
	{
		return LevelAsset.LevelVariables.FindByPredicate(
			[VariableId](const FGridLevelVariableDefinition& Definition)
			{
				return Definition.VariableId == VariableId;
			});
	}

	EGridLogicNodeType GetLogicNodeType(const UGridLevelAsset& LevelAsset, const FGuid& ObjectId)
	{
		if (const FGridLogicObjectInstance* Logic = LevelAsset.FindLogicObjectInstanceById(ObjectId))
		{
			return Logic->Logic.NodeType;
		}
		return EGridLogicNodeType::Relay;
	}

	bool ResolveLogicId(const UGridLevelAsset& LevelAsset, const FString& LogicIdText, FGuid& OutObjectId, FString& OutFailureCode, FString& OutFailure)
	{
		OutObjectId.Invalidate();
		if (!IsSimpleLogicIdText(LogicIdText))
		{
			OutFailureCode = TEXT("E200");
			OutFailure = FString::Printf(TEXT("'%s' is not a valid LogicId literal; expected [A-Za-z_][A-Za-z0-9_]*."), *LogicIdText);
			return false;
		}

		TArray<FGuid> MatchingIds;
		const int32 MatchCount = LevelAsset.FindTypedPlacementIdsByLogicId(FName(*LogicIdText), MatchingIds);
		if (MatchCount == 0)
		{
			OutFailureCode = TEXT("E201");
			OutFailure = FString::Printf(TEXT("unknown LogicId '%s'."), *LogicIdText);
			return false;
		}
		if (MatchCount != 1)
		{
			OutFailureCode = TEXT("E202");
			OutFailure = FString::Printf(TEXT("LogicId '%s' is ambiguous (%d objects)."), *LogicIdText, MatchCount);
			return false;
		}

		OutObjectId = MatchingIds[0];
		OutFailureCode.Reset();
		OutFailure.Reset();
		return OutObjectId.IsValid();
	}

	TSet<FName> DetectGlobalCallbackNames(const FString& Source)
	{
		static const FRegexPattern FunctionDeclaration(TEXT("(?m)^[ \\t]*function[ \\t]+([A-Za-z_][A-Za-z0-9_]*)[ \\t]*\\("));
		static const FRegexPattern FunctionAssignment(TEXT("(?m)^[ \\t]*([A-Za-z_][A-Za-z0-9_]*)[ \\t]*=[ \\t]*function[ \\t]*\\("));

		TSet<FName> Result;
		for (const FRegexPattern* Pattern : { &FunctionDeclaration, &FunctionAssignment })
		{
			FRegexMatcher Matcher(*Pattern, Source);
			while (Matcher.FindNext())
			{
				const FString Name = Matcher.GetCaptureGroup(1);
				if (!Name.IsEmpty())
				{
					Result.Add(FName(*Name));
				}
			}
		}
		return Result;
	}

	bool HasMaterialSlot(const UStaticMesh& Mesh, FName SlotName)
	{
		for (const FStaticMaterial& StaticMaterial : Mesh.GetStaticMaterials())
		{
			if (StaticMaterial.MaterialSlotName == SlotName)
			{
				return true;
			}
		}
		return false;
	}

	bool IsObviousPersistentTypeMismatch(EGridLuaPersistentValueType ExpectedType, const FLuaAuthoringToken& ValueToken)
	{
		if (ValueToken.Kind == ELuaAuthoringTokenKind::String)
		{
			return true;
		}
		if (ExpectedType == EGridLuaPersistentValueType::Bool)
		{
			return ValueToken.Kind == ELuaAuthoringTokenKind::Number || ValueToken.Text == TEXT("nil");
		}
		if (ValueToken.Text == TEXT("true") || ValueToken.Text == TEXT("false") || ValueToken.Text == TEXT("nil"))
		{
			return true;
		}
		return ValueToken.Kind == ELuaAuthoringTokenKind::Number && ValueToken.Text.Contains(TEXT("."));
	}

	void ValidatePersistentReferences(FName ScriptId, const TArray<FLuaAuthoringToken>& Tokens,
		const TArray<FGridLuaPersistentVariableDefinition>& Definitions, FGridLuaCompileResult& Result)
	{
		TMap<FName, EGridLuaPersistentValueType> Declared;
		for (const FGridLuaPersistentVariableDefinition& Definition : Definitions)
		{
			Declared.Add(Definition.VariableId, Definition.Type);
		}

		for (int32 Index = 0; Index < Tokens.Num(); ++Index)
		{
			if (!IdentifierIs(Tokens, Index, TEXT("persistent")))
			{
				continue;
			}

			if (TokenIs(Tokens, Index + 1, TEXT("[")))
			{
				AddDiagnostic(Result, TEXT("E100"), ScriptId, &Tokens[Index],
					TEXT("dynamic persistent indexing is not allowed; use persistent.Name so the compiler can validate the variable."));
				continue;
			}
			if (!TokenIs(Tokens, Index + 1, TEXT(".")) || !Tokens.IsValidIndex(Index + 2) ||
				Tokens[Index + 2].Kind != ELuaAuthoringTokenKind::Identifier)
			{
				continue;
			}

			const FName VariableId(*Tokens[Index + 2].Text);
			const EGridLuaPersistentValueType* Type = Declared.Find(VariableId);
			if (!Type)
			{
				AddDiagnostic(Result, TEXT("E101"), ScriptId, &Tokens[Index + 2],
					FString::Printf(TEXT("persistent variable '%s' is not declared in the top-level persistent table."), *VariableId.ToString()));
				continue;
			}

			if (TokenIs(Tokens, Index + 3, TEXT("=")) && Tokens.IsValidIndex(Index + 4) && IsObviousPersistentTypeMismatch(*Type, Tokens[Index + 4]))
			{
				const TCHAR* Expected = *Type == EGridLuaPersistentValueType::Bool ? TEXT("Bool") : TEXT("Int32");
				AddDiagnostic(Result, TEXT("E102"), ScriptId, &Tokens[Index + 4],
					FString::Printf(TEXT("persistent variable '%s' is %s and cannot be assigned this literal."), *VariableId.ToString(), Expected));
			}
		}
	}

	void ValidateGridApi(FName ScriptId, const AGridLevelEditorActor& EditorActor, const UGridLevelAsset& LevelAsset,
		const TArray<FLuaAuthoringToken>& Tokens, FGridLuaCompileResult& Result)
	{
		for (int32 Index = 0; Index < Tokens.Num(); ++Index)
		{
			if (!IdentifierIs(Tokens, Index, TEXT("grid")) || !TokenIs(Tokens, Index + 1, TEXT(".")) || !Tokens.IsValidIndex(Index + 2) ||
				Tokens[Index + 2].Kind != ELuaAuthoringTokenKind::Identifier)
			{
				continue;
			}

			const FString ApiName = Tokens[Index + 2].Text;
			if (ApiName == TEXT("command"))
			{
				const bool bShapeValid = TokenIs(Tokens, Index + 3, TEXT("(")) && Tokens.IsValidIndex(Index + 4) &&
					Tokens[Index + 4].Kind == ELuaAuthoringTokenKind::String && Tokens[Index + 4].bSimpleLiteral && TokenIs(Tokens, Index + 5, TEXT(",")) &&
					Tokens.IsValidIndex(Index + 6) && Tokens[Index + 6].Kind == ELuaAuthoringTokenKind::String && Tokens[Index + 6].bSimpleLiteral &&
					TokenIs(Tokens, Index + 7, TEXT(")"));
				if (!bShapeValid)
				{
					AddDiagnostic(Result, TEXT("E200"), ScriptId, &Tokens[Index],
						TEXT("grid.command requires exactly two plain string literals: grid.command(\"LogicId\", \"Command\")."));
					continue;
				}

				const FString& LogicIdText = Tokens[Index + 4].Text;
				const FString& CommandText = Tokens[Index + 6].Text;
				FGuid TargetId;
				FString FailureCode;
				FString Failure;
				if (!ResolveLogicId(LevelAsset, LogicIdText, TargetId, FailureCode, Failure))
				{
					AddDiagnostic(Result, *FailureCode, ScriptId, &Tokens[Index + 4], Failure);
					continue;
				}

				const UEnum* CommandEnum = StaticEnum<EGridObjectCommand>();
				const int64 RawCommand = CommandEnum ? CommandEnum->GetValueByNameString(CommandText) : INDEX_NONE;
				if (RawCommand == INDEX_NONE)
				{
					AddDiagnostic(Result, TEXT("E203"), ScriptId, &Tokens[Index + 6],
						FString::Printf(TEXT("unknown command '%s'."), *CommandText));
					continue;
				}

				const EGridObjectCommand Command = static_cast<EGridObjectCommand>(RawCommand);
				if (Command == EGridObjectCommand::LuaCallback)
				{
					AddDiagnostic(Result, TEXT("E205"), ScriptId, &Tokens[Index + 6], TEXT("LuaCallback cannot be invoked through grid.command."));
					continue;
				}

				const EGridLevelObjectType TargetType = LevelAsset.GetTypedPlacementType(TargetId);
				const EGridLogicNodeType LogicNodeType = GetLogicNodeType(LevelAsset, TargetId);
				if (!GridEditorLinkPolicy::GetSupportedCommandsForTarget(TargetType, LogicNodeType).Contains(Command))
				{
					const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType>();
					const FString TypeText = TypeEnum ? TypeEnum->GetNameStringByValue(static_cast<int64>(TargetType)) : TEXT("Unknown");
					AddDiagnostic(Result, TEXT("E204"), ScriptId, &Tokens[Index + 6],
						FString::Printf(TEXT("command '%s' is not supported by target '%s' (%s)."), *CommandText, *LogicIdText, *TypeText));
				}
				continue;
			}

			if (ApiName == TEXT("visual"))
			{
				if (!TokenIs(Tokens, Index + 3, TEXT(".")) || !IdentifierIs(Tokens, Index + 4, TEXT("set_material")))
				{
					AddDiagnostic(Result, TEXT("E400"), ScriptId, &Tokens[Index + 2],
						TEXT("unknown grid.visual API; only grid.visual.set_material is exposed."));
					continue;
				}

				const bool bShapeValid = TokenIs(Tokens, Index + 5, TEXT("(")) && Tokens.IsValidIndex(Index + 6) &&
					Tokens[Index + 6].Kind == ELuaAuthoringTokenKind::String && Tokens[Index + 6].bSimpleLiteral && TokenIs(Tokens, Index + 7, TEXT(",")) &&
					Tokens.IsValidIndex(Index + 8) && Tokens[Index + 8].Kind == ELuaAuthoringTokenKind::String && Tokens[Index + 8].bSimpleLiteral &&
					TokenIs(Tokens, Index + 9, TEXT(",")) && Tokens.IsValidIndex(Index + 10) &&
					Tokens[Index + 10].Kind == ELuaAuthoringTokenKind::String && Tokens[Index + 10].bSimpleLiteral && TokenIs(Tokens, Index + 11, TEXT(")"));
				if (!bShapeValid)
				{
					AddDiagnostic(Result, TEXT("E300"), ScriptId, &Tokens[Index],
						TEXT("grid.visual.set_material requires exactly three plain string literals: LogicId, slot and alias."));
					continue;
				}

				const FString& LogicIdText = Tokens[Index + 6].Text;
				const FString& SlotText = Tokens[Index + 8].Text;
				const FString& AliasText = Tokens[Index + 10].Text;
				FGuid TargetId;
				FString FailureCode;
				FString Failure;
				if (!ResolveLogicId(LevelAsset, LogicIdText, TargetId, FailureCode, Failure))
				{
					AddDiagnostic(Result, *FailureCode, ScriptId, &Tokens[Index + 6], Failure);
					continue;
				}

				const FGridWorldObjectInstance* Placement = LevelAsset.FindWorldObjectInstanceById(TargetId);
				const UGridWorldObjectDefinitionAsset* Definition = Placement ? EditorActor.FindWorldObjectDefinitionById(Placement->WorldObjectDefinitionId) : nullptr;
				if (!Placement || !Definition || !Definition->StaticPart.Mesh)
				{
					AddDiagnostic(Result, TEXT("E301"), ScriptId, &Tokens[Index + 6],
						FString::Printf(TEXT("target '%s' has no compiler-visible static visual definition."), *LogicIdText));
					continue;
				}

				if (!HasMaterialSlot(*Definition->StaticPart.Mesh, FName(*SlotText)))
				{
					AddDiagnostic(Result, TEXT("E302"), ScriptId, &Tokens[Index + 8],
						FString::Printf(TEXT("material slot '%s' does not exist on '%s'."), *SlotText, *LogicIdText));
				}
				const TObjectPtr<UMaterialInterface>* Material = Definition->RuntimeMaterialAliases.Find(FName(*AliasText));
				if (!Material || !Material->Get())
				{
					AddDiagnostic(Result, TEXT("E303"), ScriptId, &Tokens[Index + 10],
						FString::Printf(TEXT("material alias '%s' is not declared on '%s'."), *AliasText, *LogicIdText));
				}
				continue;
			}

			if (ApiName == TEXT("vars"))
			{
				if (!TokenIs(Tokens, Index + 3, TEXT(".")) || !Tokens.IsValidIndex(Index + 4) || Tokens[Index + 4].Kind != ELuaAuthoringTokenKind::Identifier ||
					!(Tokens[Index + 4].Text == TEXT("get_bool") || Tokens[Index + 4].Text == TEXT("set_bool") ||
						Tokens[Index + 4].Text == TEXT("get_int") || Tokens[Index + 4].Text == TEXT("set_int")))
				{
					AddDiagnostic(Result, TEXT("E400"), ScriptId, &Tokens[Index + 2], TEXT("unknown grid.vars API."));
				}
				continue;
			}

			if (ApiName == TEXT("log"))
			{
				continue;
			}

			AddDiagnostic(Result, TEXT("E400"), ScriptId, &Tokens[Index + 2],
				FString::Printf(TEXT("unknown grid API 'grid.%s'."), *ApiName));
		}
	}

	bool PersistentDefinitionsCompatible(const FGridLuaPersistentVariableDefinition& A, const FGridLuaPersistentVariableDefinition& B)
	{
		if (A.VariableId != B.VariableId || A.Type != B.Type)
		{
			return false;
		}
		return A.Type == EGridLuaPersistentValueType::Bool ? A.bDefaultBoolValue == B.bDefaultBoolValue : A.DefaultInt32Value == B.DefaultInt32Value;
	}

	bool PersistentMatchesLevelVariable(const FGridLuaPersistentVariableDefinition& LuaDefinition, const FGridLevelVariableDefinition& LevelDefinition)
	{
		if (LuaDefinition.VariableId != LevelDefinition.VariableId)
		{
			return false;
		}
		if (LuaDefinition.Type == EGridLuaPersistentValueType::Bool)
		{
			return LevelDefinition.Type == EGridLevelVariableType::Bool && LuaDefinition.bDefaultBoolValue == LevelDefinition.bDefaultBoolValue;
		}
		return LevelDefinition.Type == EGridLevelVariableType::Int32 && LuaDefinition.DefaultInt32Value == LevelDefinition.DefaultInt32Value;
	}

	bool CompileCandidateInternal(const AGridLevelEditorActor& EditorActor, const TArray<FGridLuaScriptSource>& CandidateScripts,
		const TArray<FGridObjectLink>& CandidateLinks, FGridLuaCompileResult& Result)
	{
		Result = FGridLuaCompileResult();
		const UGridLevelAsset* LevelAsset = EditorActor.LevelAsset;
		if (!LevelAsset)
		{
			AddDiagnostic(Result, TEXT("E900"), NAME_None, 1, 1, TEXT("Grid Editor has no UGridLevelAsset."));
			return false;
		}

		FString DefinitionError;
		if (!FGridLuaVm::ValidateScriptDefinitions(CandidateScripts, DefinitionError))
		{
			AddDiagnostic(Result, TEXT("E001"), NAME_None, ExtractLuaErrorLine(DefinitionError), 1, DefinitionError);
			return false;
		}

		TMap<FName, TSet<FName>> CallbacksByScript;
		TMap<FName, FGridLuaPersistentVariableDefinition> MergedPersistent;
		for (const FGridLuaScriptSource& Script : CandidateScripts)
		{
			if (!Script.bEnabled)
			{
				continue;
			}
			++Result.EnabledScriptCount;

			FGridLuaVm ScriptVm;
			FString LuaError;
			if (!ScriptVm.Reload({ Script }, FGridLuaVmConfig(), LuaError))
			{
				AddDiagnostic(Result, TEXT("E001"), Script.ScriptId, ExtractLuaErrorLine(LuaError), 1, LuaError);
				continue;
			}

			const TArray<FGridLuaPersistentVariableDefinition> PersistentDefinitions = ScriptVm.GetPersistentVariableDefinitions();
			for (const FGridLuaPersistentVariableDefinition& Definition : PersistentDefinitions)
			{
				if (const FGridLuaPersistentVariableDefinition* Existing = MergedPersistent.Find(Definition.VariableId))
				{
					if (!PersistentDefinitionsCompatible(*Existing, Definition))
					{
						AddDiagnostic(Result, TEXT("E103"), Script.ScriptId, 1, 1,
							FString::Printf(TEXT("persistent variable '%s' has conflicting declarations across scripts."), *Definition.VariableId.ToString()));
					}
				}
				else
				{
					MergedPersistent.Add(Definition.VariableId, Definition);
				}
			}

			TArray<FLuaAuthoringToken> Tokens;
			TokenizeLuaSource(Script.Source, Tokens);
			ValidatePersistentReferences(Script.ScriptId, Tokens, PersistentDefinitions, Result);
			ValidateGridApi(Script.ScriptId, EditorActor, *LevelAsset, Tokens, Result);
			CallbacksByScript.Add(Script.ScriptId, DetectGlobalCallbackNames(Script.Source));
		}

		FGridLuaVm FullVm;
		FString FullVmError;
		if (!FullVm.Reload(CandidateScripts, FGridLuaVmConfig(), FullVmError) &&
			!Result.Diagnostics.ContainsByPredicate([](const FGridLuaCompileDiagnostic& Diagnostic) { return Diagnostic.Code == TEXT("E001"); }))
		{
			AddDiagnostic(Result, TEXT("E001"), NAME_None, ExtractLuaErrorLine(FullVmError), 1, FullVmError);
		}

		for (const TPair<FName, FGridLuaPersistentVariableDefinition>& Pair : MergedPersistent)
		{
			if (const FGridLevelVariableDefinition* Existing = FindLevelVariable(*LevelAsset, Pair.Key); Existing && !PersistentMatchesLevelVariable(Pair.Value, *Existing))
			{
				AddDiagnostic(Result, TEXT("E103"), NAME_None, 1, 1,
					FString::Printf(TEXT("persistent variable '%s' conflicts with the existing LevelVariable type/default."), *Pair.Key.ToString()));
			}
		}

		for (const FGridObjectLink& Link : CandidateLinks)
		{
			if (Link.Command != EGridObjectCommand::LuaCallback)
			{
				continue;
			}
			const FGridLuaScriptSource* Script = CandidateScripts.FindByPredicate(
				[&Link](const FGridLuaScriptSource& Candidate)
				{
					return Candidate.ScriptId == Link.LuaScriptId;
				});
			if (!Script || !Script->bEnabled)
			{
				AddDiagnostic(Result, TEXT("E501"), Link.LuaScriptId, 1, 1,
					FString::Printf(TEXT("Lua binding references missing or disabled script '%s'."), *Link.LuaScriptId.ToString()));
				continue;
			}
			const TSet<FName>* Callbacks = CallbacksByScript.Find(Link.LuaScriptId);
			if (!Callbacks || !Callbacks->Contains(Link.LuaCallbackName))
			{
				AddDiagnostic(Result, TEXT("E502"), Link.LuaScriptId, 1, 1,
					FString::Printf(TEXT("Lua callback '%s.%s' is not declared as a global function."), *Link.LuaScriptId.ToString(), *Link.LuaCallbackName.ToString()));
			}
			if (!LevelAsset->ContainsTypedPlacementId(Link.SourceObjectId))
			{
				AddDiagnostic(Result, TEXT("E503"), Link.LuaScriptId, 1, 1, TEXT("Lua binding source object does not exist."));
			}
			if (Link.Condition != EGridObjectCondition::None)
			{
				AddDiagnostic(Result, TEXT("E504"), Link.LuaScriptId, 1, 1, TEXT("Lua bindings must be unconditional; put the condition in Lua."));
			}
		}

		return Result.Succeeded();
	}
}

FString FGridLuaCompileDiagnostic::ToDisplayString() const
{
	const FString ScriptText = ScriptId.IsNone() ? FString() : FString::Printf(TEXT(" [%s]"), *ScriptId.ToString());
	return FString::Printf(TEXT("LUA-COMPILE %s%s line %d:%d: %s"), *Code, *ScriptText, Line, Column, *Message);
}

FString FGridLuaCompileResult::GetFirstErrorText() const
{
	return Diagnostics.IsEmpty() ? FString() : Diagnostics[0].ToDisplayString();
}

FString FGridLuaCompileResult::GetSummaryText(int32 MaxDiagnostics) const
{
	if (Succeeded())
	{
		return FString::Printf(TEXT("Lua compile OK: %d enabled script(s)."), EnabledScriptCount);
	}

	TArray<FString> Lines;
	const int32 Count = FMath::Min(FMath::Max(1, MaxDiagnostics), Diagnostics.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Lines.Add(Diagnostics[Index].ToDisplayString());
	}
	if (Diagnostics.Num() > Count)
	{
		Lines.Add(FString::Printf(TEXT("... %d more compiler error(s)."), Diagnostics.Num() - Count));
	}
	return FString::Join(Lines, TEXT("\n"));
}

bool FGridLuaAuthoringCompiler::CompileLevel(const AGridLevelEditorActor& EditorActor, FGridLuaCompileResult& OutResult)
{
	if (!EditorActor.LevelAsset)
	{
		OutResult = FGridLuaCompileResult();
		AddDiagnostic(OutResult, TEXT("E900"), NAME_None, 1, 1, TEXT("Grid Editor has no UGridLevelAsset."));
		return false;
	}
	return CompileCandidateInternal(EditorActor, EditorActor.LevelAsset->LuaScripts, EditorActor.LevelAsset->Links, OutResult);
}

bool FGridLuaAuthoringCompiler::CompileCandidate(const AGridLevelEditorActor& EditorActor, const TArray<FGridLuaScriptSource>& CandidateScripts,
	const TArray<FGridObjectLink>& CandidateLinks, FGridLuaCompileResult& OutResult)
{
	return CompileCandidateInternal(EditorActor, CandidateScripts, CandidateLinks, OutResult);
}

bool FGridLuaAuthoringCompiler::CompileScriptDraft(const AGridLevelEditorActor& EditorActor, FName OldScriptId, FName NewScriptId,
	const FString& CandidateSource, FGridLuaCompileResult& OutResult)
{
	OutResult = FGridLuaCompileResult();
	if (!EditorActor.LevelAsset)
	{
		AddDiagnostic(OutResult, TEXT("E900"), NAME_None, 1, 1, TEXT("Grid Editor has no UGridLevelAsset."));
		return false;
	}
	if (OldScriptId.IsNone() || NewScriptId.IsNone())
	{
		AddDiagnostic(OutResult, TEXT("E900"), OldScriptId, 1, 1, TEXT("ScriptId cannot be empty."));
		return false;
	}

	TArray<FGridLuaScriptSource> CandidateScripts = EditorActor.LevelAsset->LuaScripts;
	const int32 ScriptIndex = CandidateScripts.IndexOfByPredicate(
		[OldScriptId](const FGridLuaScriptSource& Script)
		{
			return Script.ScriptId == OldScriptId;
		});
	if (ScriptIndex == INDEX_NONE)
	{
		AddDiagnostic(OutResult, TEXT("E900"), OldScriptId, 1, 1, TEXT("Script to edit was not found."));
		return false;
	}
	if (OldScriptId != NewScriptId && CandidateScripts.ContainsByPredicate(
			[NewScriptId](const FGridLuaScriptSource& Script)
			{
				return Script.ScriptId == NewScriptId;
			}))
	{
		AddDiagnostic(OutResult, TEXT("E900"), NewScriptId, 1, 1,
			FString::Printf(TEXT("ScriptId '%s' already exists."), *NewScriptId.ToString()));
		return false;
	}

	CandidateScripts[ScriptIndex].ScriptId = NewScriptId;
	CandidateScripts[ScriptIndex].Source = CandidateSource;

	TArray<FGridObjectLink> CandidateLinks = EditorActor.LevelAsset->Links;
	for (FGridObjectLink& Link : CandidateLinks)
	{
		if (Link.Command == EGridObjectCommand::LuaCallback && Link.LuaScriptId == OldScriptId)
		{
			Link.LuaScriptId = NewScriptId;
		}
	}

	return CompileCandidateInternal(EditorActor, CandidateScripts, CandidateLinks, OutResult);
}

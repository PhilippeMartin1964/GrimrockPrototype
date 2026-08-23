#include "EditorTools/GridEditorLuaService.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridLevelVariableTypes.h"
#include "EditorTools/GridEditorLinkPolicy.h"
#include "EditorTools/GridEditorLinkService.h"
#include "GridLuaVm.h"
#include "Internationalization/Regex.h"
#include "Runtime/GridLogicRuntime.h"

namespace
{
    const FGridLevelObjectData* FindObjectById (
        const UGridLevelAsset& LevelAsset,
        const FGuid& ObjectId)
    {
        return LevelAsset.Objects.FindByPredicate (
            [&ObjectId] (const FGridLevelObjectData& Object)
        {
            return Object.ObjectId == ObjectId;
        });
    }

    const FGridLuaScriptSource* FindScriptById (
        const UGridLevelAsset& LevelAsset,
        FName ScriptId)
    {
        return LevelAsset.LuaScripts.FindByPredicate (
            [ScriptId] (const FGridLuaScriptSource& Script)
        {
            return Script.ScriptId == ScriptId;
        });
    }

    FGridLuaScriptSource* FindScriptById (
        UGridLevelAsset& LevelAsset,
        FName ScriptId)
    {
        return LevelAsset.LuaScripts.FindByPredicate (
            [ScriptId] (const FGridLuaScriptSource& Script)
        {
            return Script.ScriptId == ScriptId;
        });
    }

    const FGridLevelVariableDefinition* FindVariableDefinition (
        const UGridLevelAsset& LevelAsset,
        FName VariableId)
    {
        return LevelAsset.LevelVariables.FindByPredicate (
            [VariableId] (const FGridLevelVariableDefinition& Definition)
        {
            return Definition.VariableId == VariableId;
        });
    }

    void AddDetectedCallbacksFromPattern (
        const FString& Source,
        const FRegexPattern& Pattern,
        TSet<FName>& OutCallbacks)
    {
        FRegexMatcher Matcher (Pattern, Source);
        while (Matcher.FindNext ())
        {
            const FString Name = Matcher.GetCaptureGroup (1);
            if (!Name.IsEmpty ())
            {
                OutCallbacks.Add (FName (*Name));
            }
        }
    }

    TArray<FName> DetectGlobalCallbackNames (const FString& Source)
    {
        // Only top-level global functions can be reached through the runtime
        // ScriptId + CallbackName contract. Local and table-member functions
        // are intentionally not reported.
        static const FRegexPattern FunctionDeclaration (
            TEXT ("(?m)^[ \\t]*function[ \\t]+([A-Za-z_][A-Za-z0-9_]*)[ \\t]*\\("));
        static const FRegexPattern FunctionAssignment (
            TEXT ("(?m)^[ \\t]*([A-Za-z_][A-Za-z0-9_]*)[ \\t]*=[ \\t]*function[ \\t]*\\("));

        TSet<FName> UniqueCallbacks;
        AddDetectedCallbacksFromPattern (
            Source,
            FunctionDeclaration,
            UniqueCallbacks);
        AddDetectedCallbacksFromPattern (
            Source,
            FunctionAssignment,
            UniqueCallbacks);

        TArray<FName> Result = UniqueCallbacks.Array ();
        Result.Sort ([] (FName Left, FName Right)
        {
            return Left.LexicalLess (Right);
        });
        return Result;
    }

    bool ValidateOneScript (
        const FGridLuaScriptSource& Script,
        FString& OutError)
    {
        FGridLuaScriptSource EnabledScript = Script;
        EnabledScript.bEnabled = true;

        FGridLuaVm Vm;
        return Vm.Reload (
            {EnabledScript},
            FGridLuaVmConfig (),
            OutError);
    }

    bool IsSupportedLuaCondition (EGridObjectCondition Condition)
    {
        return Condition == EGridObjectCondition::None ||
            Condition == EGridObjectCondition::LevelVariableBoolEquals ||
            Condition == EGridObjectCondition::LevelVariableIntCompare;
    }

    FGridObjectLink NormalizeLuaLink (const FGridObjectLink& Link)
    {
        FGridObjectLink Normalized =
            GridEditorLinkService::NormalizeLink (Link);
        Normalized.Command = EGridObjectCommand::LuaCallback;
        Normalized.TargetObjectId.Invalidate ();
        return Normalized;
    }

    bool TryGetLinkIndexFromMessage (
        const FString& Message,
        int32& OutLinkIndex)
    {
        OutLinkIndex = INDEX_NONE;
        if (!Message.StartsWith (TEXT ("Link ")))
        {
            return false;
        }

        int32 Cursor = 5;
        const int32 Start = Cursor;
        while (Cursor < Message.Len () && FChar::IsDigit (Message[Cursor]))
        {
            ++Cursor;
        }
        if (Cursor == Start)
        {
            return false;
        }

        OutLinkIndex = FCString::Atoi (*Message.Mid (Start, Cursor - Start));
        return OutLinkIndex >= 0;
    }

    bool IsExactLuaDuplicateAt (
        const UGridLevelAsset& LevelAsset,
        int32 LinkIndex)
    {
        if (!LevelAsset.Links.IsValidIndex (LinkIndex))
        {
            return false;
        }
        const FGridObjectLink& Link = LevelAsset.Links[LinkIndex];
        for (int32 PreviousIndex = 0; PreviousIndex < LinkIndex; ++PreviousIndex)
        {
            if (GridEditorLinkPolicy::AreLinksExactlyEquivalent (
                    LevelAsset.Links[PreviousIndex],
                    Link))
            {
                return true;
            }
        }
        return false;
    }

    void FillMessageLocation (
        const UGridLevelAsset& LevelAsset,
        FGridLevelValidationMessage& Message)
    {
        FGuid LocationId = Message.OptionalObjectId;
        if (!LocationId.IsValid ())
        {
            LocationId = Message.SourceObjectId.IsValid ()
                ? Message.SourceObjectId
                : Message.TargetObjectId;
        }
        if (const FGridLevelObjectData* Object =
                FindObjectById (LevelAsset, LocationId))
        {
            Message.CellX = Object->CellX;
            Message.CellY = Object->CellY;
            Message.Edge = Object->Edge;
        }
    }

    void AddValidationMessage (
        const UGridLevelAsset& LevelAsset,
        TArray<FGridLevelValidationMessage>& Messages,
        EGridLevelValidationSeverity Severity,
        FName Category,
        const FString& Text,
        FGuid SourceObjectId = FGuid ())
    {
        FGridLevelValidationMessage Message;
        Message.Severity = Severity;
        Message.Category = Category;
        Message.Message = Text;
        Message.OptionalObjectId = SourceObjectId;
        Message.SourceObjectId = SourceObjectId;
        FillMessageLocation (LevelAsset, Message);
        Messages.Add (MoveTemp (Message));
    }
}

namespace GridEditorLuaService
{
    bool AnalyzeLevel (
        const UGridLevelAsset& LevelAsset,
        FGridEditorLuaAnalysis& OutAnalysis)
    {
        OutAnalysis = FGridEditorLuaAnalysis ();

        FString DefinitionError;
        OutAnalysis.bDefinitionsValid =
            FGridLuaVm::ValidateScriptDefinitions (
                LevelAsset.LuaScripts,
                DefinitionError);
        if (!OutAnalysis.bDefinitionsValid)
        {
            OutAnalysis.GlobalError = DefinitionError;
        }

        for (const FGridLuaScriptSource& Script : LevelAsset.LuaScripts)
        {
            FGridEditorLuaScriptAnalysis ScriptAnalysis;
            ScriptAnalysis.ScriptId = Script.ScriptId;
            ScriptAnalysis.bEnabled = Script.bEnabled;
            ScriptAnalysis.CallbackNames =
                DetectGlobalCallbackNames (Script.Source);

            if (!Script.bEnabled)
            {
                ScriptAnalysis.bValid = true;
            }
            else
            {
                ScriptAnalysis.bValid =
                    ValidateOneScript (Script, ScriptAnalysis.Error);
            }
            OutAnalysis.Scripts.Add (MoveTemp (ScriptAnalysis));
        }

        FString FullVmError;
        FGridLuaVm FullVm;
        OutAnalysis.bFullVmValid =
            OutAnalysis.bDefinitionsValid &&
            FullVm.Reload (
                LevelAsset.LuaScripts,
                FGridLuaVmConfig (),
                FullVmError);
        if (!OutAnalysis.bFullVmValid &&
            OutAnalysis.GlobalError.IsEmpty ())
        {
            OutAnalysis.GlobalError = FullVmError;
        }

        return OutAnalysis.IsValid ();
    }

    bool GetCallbacksForScript (
        const UGridLevelAsset& LevelAsset,
        FName ScriptId,
        TArray<FName>& OutCallbacks,
        FString& OutError)
    {
        OutCallbacks.Reset ();
        const FGridLuaScriptSource* Script =
            FindScriptById (LevelAsset, ScriptId);
        if (!Script)
        {
            OutError = FString::Printf (
                TEXT ("Lua ScriptId '%s' does not exist."),
                *ScriptId.ToString ());
            return false;
        }
        if (!Script->bEnabled)
        {
            OutError = FString::Printf (
                TEXT ("Lua script '%s' is disabled."),
                *ScriptId.ToString ());
            return false;
        }
        if (!ValidateOneScript (*Script, OutError))
        {
            return false;
        }

        OutCallbacks = DetectGlobalCallbackNames (Script->Source);
        OutError.Reset ();
        return true;
    }

    bool IsLuaLinkSupported (
        const UGridLevelAsset& LevelAsset,
        const FGridObjectLink& Link,
        FString& OutError)
    {
        const FGridObjectLink Normalized = NormalizeLuaLink (Link);
        if (!Normalized.SourceObjectId.IsValid ())
        {
            OutError = TEXT ("Lua binding requires a valid SourceObjectId.");
            return false;
        }
        const FGridLevelObjectData* Source =
            FindObjectById (LevelAsset, Normalized.SourceObjectId);
        if (!Source)
        {
            OutError = TEXT ("Lua binding source object does not exist.");
            return false;
        }
        if (!GridEditorLinkPolicy::GetSupportedEventsForSource (*Source).Contains (
                Normalized.SourceEvent))
        {
            OutError = TEXT ("Lua binding source event is not emitted by this object type.");
            return false;
        }
        if (Normalized.LuaScriptId.IsNone () ||
            Normalized.LuaCallbackName.IsNone ())
        {
            OutError = TEXT ("Lua binding requires ScriptId and CallbackName.");
            return false;
        }
        if (!IsSupportedLuaCondition (Normalized.Condition) ||
            !GridEditorLinkService::IsConditionConfigurationValid (Normalized))
        {
            OutError = TEXT ("Lua bindings support only None or typed level-variable conditions.");
            return false;
        }

        if (Normalized.Condition == EGridObjectCondition::LevelVariableBoolEquals ||
            Normalized.Condition == EGridObjectCondition::LevelVariableIntCompare)
        {
            const FGridLevelVariableDefinition* Variable =
                FindVariableDefinition (
                    LevelAsset,
                    Normalized.ConditionVariableId);
            if (!Variable)
            {
                OutError = FString::Printf (
                    TEXT ("Lua binding references undeclared level variable '%s'."),
                    *Normalized.ConditionVariableId.ToString ());
                return false;
            }
            const EGridLevelVariableType RequiredType =
                Normalized.Condition == EGridObjectCondition::LevelVariableBoolEquals
                    ? EGridLevelVariableType::Bool
                    : EGridLevelVariableType::Int32;
            if (Variable->Type != RequiredType)
            {
                OutError = FString::Printf (
                    TEXT ("Lua binding variable '%s' has the wrong type."),
                    *Normalized.ConditionVariableId.ToString ());
                return false;
            }
        }

        TArray<FName> Callbacks;
        if (!GetCallbacksForScript (
                LevelAsset,
                Normalized.LuaScriptId,
                Callbacks,
                OutError))
        {
            return false;
        }
        if (!Callbacks.Contains (Normalized.LuaCallbackName))
        {
            OutError = FString::Printf (
                TEXT ("Lua callback '%s.%s' is not declared as a global function."),
                *Normalized.LuaScriptId.ToString (),
                *Normalized.LuaCallbackName.ToString ());
            return false;
        }

        OutError.Reset ();
        return true;
    }

    bool AddLuaLink (
        AGridLevelEditorActor& EditorActor,
        const FGridObjectLink& Link,
        FString& OutError)
    {
        UGridLevelAsset* LevelAsset = EditorActor.LevelAsset;
        if (!LevelAsset)
        {
            OutError = TEXT ("Grid Editor has no LevelAsset.");
            return false;
        }

        const FGridObjectLink Normalized = NormalizeLuaLink (Link);
        if (!IsLuaLinkSupported (*LevelAsset, Normalized, OutError))
        {
            return false;
        }
        if (GridEditorLinkService::ContainsExactLink (
                LevelAsset->Links,
                Normalized))
        {
            OutError = TEXT ("An identical Lua binding already exists.");
            return false;
        }

#if WITH_EDITOR
        LevelAsset->Modify ();
#endif
        LevelAsset->Links.Add (Normalized);
#if WITH_EDITOR
        LevelAsset->MarkPackageDirty ();
#endif
        EditorActor.LastSelectedObjectId = Normalized.SourceObjectId;
        EditorActor.RebuildPreview ();
        OutError.Reset ();
        return true;
    }

    bool RemoveLuaLink (
        AGridLevelEditorActor& EditorActor,
        const FGridObjectLink& Link)
    {
        return Link.Command == EGridObjectCommand::LuaCallback &&
            GridEditorLinkService::RemoveExactLink (
                EditorActor,
                Link);
    }

    FName MakeUniqueScriptId (const UGridLevelAsset& LevelAsset)
    {
        const FName BaseId (TEXT ("Script"));
        if (!FindScriptById (LevelAsset, BaseId))
        {
            return BaseId;
        }

        for (int32 Index = 2; Index < MAX_int32; ++Index)
        {
            const FName Candidate (*FString::Printf (
                TEXT ("Script_%d"),
                Index));
            if (!FindScriptById (LevelAsset, Candidate))
            {
                return Candidate;
            }
        }
        return NAME_None;
    }

    int32 CountScriptReferences (
        const UGridLevelAsset& LevelAsset,
        FName ScriptId)
    {
        int32 ReferenceCount = 0;
        for (const FGridObjectLink& Link : LevelAsset.Links)
        {
            if (Link.Command == EGridObjectCommand::LuaCallback &&
                Link.LuaScriptId == ScriptId)
            {
                ++ReferenceCount;
            }
        }
        return ReferenceCount;
    }

    bool AddScript (
        UGridLevelAsset& LevelAsset,
        FName ScriptId,
        const FString& Source,
        FString& OutError)
    {
        if (ScriptId.IsNone ())
        {
            OutError = TEXT ("ScriptId cannot be empty.");
            return false;
        }
        if (FindScriptById (LevelAsset, ScriptId))
        {
            OutError = FString::Printf (
                TEXT ("ScriptId '%s' already exists."),
                *ScriptId.ToString ());
            return false;
        }

#if WITH_EDITOR
        LevelAsset.Modify ();
#endif
        FGridLuaScriptSource Script;
        Script.ScriptId = ScriptId;
        Script.bEnabled = true;
        Script.Source = Source;
        LevelAsset.LuaScripts.Add (MoveTemp (Script));
#if WITH_EDITOR
        LevelAsset.MarkPackageDirty ();
#endif
        OutError.Reset ();
        return true;
    }

    bool RenameScript (
        UGridLevelAsset& LevelAsset,
        FName OldScriptId,
        FName NewScriptId,
        FString& OutError)
    {
        if (NewScriptId.IsNone ())
        {
            OutError = TEXT ("New ScriptId cannot be empty.");
            return false;
        }
        FGridLuaScriptSource* Script =
            FindScriptById (LevelAsset, OldScriptId);
        if (!Script)
        {
            OutError = TEXT ("Script to rename was not found.");
            return false;
        }
        if (OldScriptId == NewScriptId)
        {
            OutError.Reset ();
            return true;
        }
        if (FindScriptById (LevelAsset, NewScriptId))
        {
            OutError = FString::Printf (
                TEXT ("ScriptId '%s' already exists."),
                *NewScriptId.ToString ());
            return false;
        }

#if WITH_EDITOR
        LevelAsset.Modify ();
#endif
        Script->ScriptId = NewScriptId;
        for (FGridObjectLink& Link : LevelAsset.Links)
        {
            if (Link.Command == EGridObjectCommand::LuaCallback &&
                Link.LuaScriptId == OldScriptId)
            {
                Link.LuaScriptId = NewScriptId;
            }
        }
#if WITH_EDITOR
        LevelAsset.MarkPackageDirty ();
#endif
        OutError.Reset ();
        return true;
    }

    bool SetScriptEnabled (
        UGridLevelAsset& LevelAsset,
        FName ScriptId,
        bool bEnabled,
        FString& OutError)
    {
        FGridLuaScriptSource* Script =
            FindScriptById (LevelAsset, ScriptId);
        if (!Script)
        {
            OutError = TEXT ("Lua script was not found.");
            return false;
        }
        if (!bEnabled && CountScriptReferences (LevelAsset, ScriptId) > 0)
        {
            OutError = TEXT ("A referenced Lua script cannot be disabled; remove its bindings first.");
            return false;
        }

#if WITH_EDITOR
        LevelAsset.Modify ();
#endif
        Script->bEnabled = bEnabled;
#if WITH_EDITOR
        LevelAsset.MarkPackageDirty ();
#endif
        OutError.Reset ();
        return true;
    }

    bool SetScriptSource (
        UGridLevelAsset& LevelAsset,
        FName ScriptId,
        const FString& Source,
        FString& OutError)
    {
        FGridLuaScriptSource* Script =
            FindScriptById (LevelAsset, ScriptId);
        if (!Script)
        {
            OutError = TEXT ("Lua script was not found.");
            return false;
        }

#if WITH_EDITOR
        LevelAsset.Modify ();
#endif
        Script->Source = Source;
#if WITH_EDITOR
        LevelAsset.MarkPackageDirty ();
#endif
        OutError.Reset ();
        return true;
    }

    bool RemoveScript (
        UGridLevelAsset& LevelAsset,
        FName ScriptId,
        FString& OutError)
    {
        const int32 References =
            CountScriptReferences (LevelAsset, ScriptId);
        if (References > 0)
        {
            OutError = FString::Printf (
                TEXT ("Lua script '%s' is referenced by %d binding(s)."),
                *ScriptId.ToString (),
                References);
            return false;
        }
        const int32 Index = LevelAsset.LuaScripts.IndexOfByPredicate (
            [ScriptId] (const FGridLuaScriptSource& Script)
        {
            return Script.ScriptId == ScriptId;
        });
        if (Index == INDEX_NONE)
        {
            OutError = TEXT ("Lua script was not found.");
            return false;
        }

#if WITH_EDITOR
        LevelAsset.Modify ();
#endif
        LevelAsset.LuaScripts.RemoveAt (Index);
#if WITH_EDITOR
        LevelAsset.MarkPackageDirty ();
#endif
        OutError.Reset ();
        return true;
    }

    TArray<FGridLevelValidationMessage> ValidateCurrentLevelWithLua (
        AGridLevelEditorActor& EditorActor)
    {
        TArray<FGridLevelValidationMessage> Messages =
            EditorActor.ValidateCurrentLevel ();
        UGridLevelAsset* LevelAsset = EditorActor.LevelAsset;
        if (!LevelAsset)
        {
            return Messages;
        }

        // Remove legacy assumptions that predate MON19 data-only Logic and the
        // targetless LuaCallback contract.
        Messages.RemoveAll ([LevelAsset] (const FGridLevelValidationMessage& Message)
        {
            if (Message.Message ==
                    TEXT ("Placed object has no ArchetypeId. Preview and runtime archetype lookup cannot resolve it.") &&
                Message.OptionalObjectId.IsValid ())
            {
                const FGridLevelObjectData* Object =
                    FindObjectById (*LevelAsset, Message.OptionalObjectId);
                if (Object && Object->Type == EGridLevelObjectType::Logic)
                {
                    return true;
                }
            }

            int32 LinkIndex = INDEX_NONE;
            if (!TryGetLinkIndexFromMessage (
                    Message.Message,
                    LinkIndex) ||
                !LevelAsset->Links.IsValidIndex (LinkIndex))
            {
                return false;
            }

            const FGridObjectLink& Link = LevelAsset->Links[LinkIndex];
            if (Link.Command == EGridObjectCommand::LuaCallback)
            {
                if (Message.Message.Contains (TEXT ("TargetObjectId")) ||
                    Message.Message.Contains (TEXT ("targets its own source object")))
                {
                    return true;
                }
                if (Message.Message.Contains (TEXT ("duplicates an identical link")) &&
                    !IsExactLuaDuplicateAt (*LevelAsset, LinkIndex))
                {
                    return true;
                }
            }

            if (Message.Message.Contains (
                    TEXT ("is not emitted by the current C++ runtime")))
            {
                const FGridLevelObjectData* Source =
                    FindObjectById (*LevelAsset, Link.SourceObjectId);
                if (Source &&
                    GridEditorLinkPolicy::GetSupportedEventsForSource (*Source).Contains (
                        Link.SourceEvent))
                {
                    return true;
                }
            }
            return false;
        });

        for (const FGridLevelObjectData& Object : LevelAsset->Objects)
        {
            if (Object.Type != EGridLevelObjectType::Logic)
            {
                continue;
            }
            if (!Object.ArchetypeId.IsNone ())
            {
                AddValidationMessage (
                    *LevelAsset,
                    Messages,
                    EGridLevelValidationSeverity::Error,
                    TEXT ("Logic"),
                    TEXT ("Data-only Logic object must use ArchetypeId=None."),
                    Object.ObjectId);
            }

            FString LogicError;
            if (!GridLogicRuntime::ValidateNode (
                    *LevelAsset,
                    Object,
                    LogicError))
            {
                AddValidationMessage (
                    *LevelAsset,
                    Messages,
                    EGridLevelValidationSeverity::Error,
                    TEXT ("Logic"),
                    FString::Printf (
                        TEXT ("Logic object is invalid: %s"),
                        *LogicError),
                    Object.ObjectId);
            }
        }

        FGridEditorLuaAnalysis Analysis;
        AnalyzeLevel (*LevelAsset, Analysis);
        if (!Analysis.bDefinitionsValid && !Analysis.GlobalError.IsEmpty ())
        {
            AddValidationMessage (
                *LevelAsset,
                Messages,
                EGridLevelValidationSeverity::Error,
                TEXT ("Lua"),
                FString::Printf (
                    TEXT ("Lua definitions are invalid: %s"),
                    *Analysis.GlobalError));
        }

        for (const FGridEditorLuaScriptAnalysis& Script : Analysis.Scripts)
        {
            if (Script.bEnabled && !Script.bValid)
            {
                AddValidationMessage (
                    *LevelAsset,
                    Messages,
                    EGridLevelValidationSeverity::Error,
                    TEXT ("Lua"),
                    FString::Printf (
                        TEXT ("Lua script '%s' is invalid: %s"),
                        *Script.ScriptId.ToString (),
                        *Script.Error));
            }
        }

        if (Analysis.bDefinitionsValid &&
            !Analysis.bFullVmValid &&
            !Analysis.GlobalError.IsEmpty ())
        {
            AddValidationMessage (
                *LevelAsset,
                Messages,
                EGridLevelValidationSeverity::Error,
                TEXT ("Lua"),
                FString::Printf (
                    TEXT ("Lua level VM cannot be built: %s"),
                    *Analysis.GlobalError));
        }

        for (int32 LinkIndex = 0;
            LinkIndex < LevelAsset->Links.Num ();
            ++LinkIndex)
        {
            const FGridObjectLink& Link =
                LevelAsset->Links[LinkIndex];
            if (Link.Command != EGridObjectCommand::LuaCallback)
            {
                continue;
            }

            FString LinkError;
            if (!IsLuaLinkSupported (
                    *LevelAsset,
                    Link,
                    LinkError))
            {
                AddValidationMessage (
                    *LevelAsset,
                    Messages,
                    EGridLevelValidationSeverity::Error,
                    TEXT ("Lua"),
                    FString::Printf (
                        TEXT ("Lua binding %d is invalid: %s"),
                        LinkIndex,
                        *LinkError),
                    Link.SourceObjectId);
            }
            if (Link.TargetObjectId.IsValid ())
            {
                AddValidationMessage (
                    *LevelAsset,
                    Messages,
                    EGridLevelValidationSeverity::Error,
                    TEXT ("Lua"),
                    FString::Printf (
                        TEXT ("Lua binding %d must be targetless; TargetObjectId is ignored by runtime."),
                        LinkIndex),
                    Link.SourceObjectId);
            }
        }

        // A legacy validation run that had only now-filtered false positives
        // does not add its usual success Info. Restore a useful success row.
        if (Messages.IsEmpty ())
        {
            FGridLevelValidationMessage Success;
            Success.Severity = EGridLevelValidationSeverity::Info;
            Success.Category = TEXT ("Core");
            Success.Message = TEXT ("Validation complete: no issues found.");
            Messages.Add (MoveTemp (Success));
        }

        EditorActor.LastValidationMessages = Messages;
        return Messages;
    }
}

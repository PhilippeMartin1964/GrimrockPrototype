#include "EditorTools/Widgets/SGridEditorLuaScriptsPanel.h"

#if WITH_EDITOR

#include "Core/GridLevelAsset.h"
#include "Core/GridLevelVariableTypes.h"
#include "Core/GridTypes.h"
#include "Editor.h"
#include "EditorTools/GridEditorLinkPolicy.h"
#include "EditorTools/GridEditorLuaService.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "EngineUtils.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FGridLuaScriptSource* FindScript(const UGridLevelAsset* LevelAsset, FName ScriptId)
	{
		return LevelAsset ? LevelAsset->LuaScripts.FindByPredicate(
								[ScriptId](const FGridLuaScriptSource& Script)
								{
									return Script.ScriptId == ScriptId;
								})
						  : nullptr;
	}

	template <typename T> TSharedPtr<T> FindEnumOption(const TArray<TSharedPtr<T>>& Options, const TSharedPtr<T>& Current)
	{
		if (!Current.IsValid())
		{
			return nullptr;
		}
		for (const TSharedPtr<T>& Option : Options)
		{
			if (Option.IsValid() && *Option == *Current)
			{
				return Option;
			}
		}
		return nullptr;
	}

	TSharedPtr<FName> FindNameOption(const TArray<TSharedPtr<FName>>& Options, FName Value)
	{
		for (const TSharedPtr<FName>& Option : Options)
		{
			if (Option.IsValid() && *Option == Value)
			{
				return Option;
			}
		}
		return nullptr;
	}
}

void SGridEditorLuaScriptsPanel::Construct(const FArguments& InArgs)
{
	(void)InArgs;
	RebuildBindingOptions();
	Rebuild();
}

AGridLevelEditorActor* SGridEditorLuaScriptsPanel::FindEditorActor() const
{
	if (!GEditor)
	{
		return nullptr;
	}
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AGridLevelEditorActor> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

UGridLevelAsset* SGridEditorLuaScriptsPanel::GetLevelAsset() const
{
	AGridLevelEditorActor* EditorActor = FindEditorActor();
	return EditorActor ? EditorActor->LevelAsset.Get() : nullptr;
}

void SGridEditorLuaScriptsPanel::SetStatus(const FString& Text, bool bSuccess)
{
	StatusText = Text;
	bStatusSuccess = bSuccess;
}

void SGridEditorLuaScriptsPanel::LoadSelectedScriptDraft()
{
	const FGridLuaScriptSource* Script = FindScript(GetLevelAsset(), SelectedScriptId);
	if (!Script)
	{
		SelectedScriptId = NAME_None;
		DraftScriptId.Reset();
		DraftSource.Reset();
		LastValidatedCallbacks.Reset();
		return;
	}

	DraftScriptId = Script->ScriptId.ToString();
	DraftSource = Script->Source;
}

void SGridEditorLuaScriptsPanel::RebuildBindingOptions()
{
	const TSharedPtr<EGridObjectEvent> PreviousEvent = SelectedEvent;
	const FName PreviousScript = SelectedBindingScript.IsValid() ? *SelectedBindingScript : NAME_None;
	const FName PreviousCallback = SelectedCallback.IsValid() ? *SelectedCallback : NAME_None;
	const TSharedPtr<EGridObjectCondition> PreviousCondition = SelectedCondition;
	const FName PreviousVariable = SelectedVariable.IsValid() ? *SelectedVariable : NAME_None;
	const TSharedPtr<EGridLogicIntComparison> PreviousComparison = SelectedComparison;

	EventOptions.Reset();
	BindingScriptOptions.Reset();
	CallbackOptions.Reset();
	ConditionOptions.Reset();
	VariableOptions.Reset();
	ComparisonOptions.Reset();

	AGridLevelEditorActor* EditorActor = FindEditorActor();
	UGridLevelAsset* LevelAsset = EditorActor ? EditorActor->LevelAsset.Get() : nullptr;
	const FGridLevelObjectData* Source = EditorActor ? EditorActor->GetSelectedObjectData() : nullptr;
	if (Source)
	{
		for (EGridObjectEvent Event : GridEditorLinkPolicy::GetSupportedEventsForSource(*Source))
		{
			EventOptions.Add(MakeShared<EGridObjectEvent>(Event));
		}
	}

	if (LevelAsset)
	{
		for (const FGridLuaScriptSource& Script : LevelAsset->LuaScripts)
		{
			if (Script.bEnabled && !Script.ScriptId.IsNone())
			{
				BindingScriptOptions.Add(MakeShared<FName>(Script.ScriptId));
			}
		}
	}

	SelectedEvent = FindEnumOption(EventOptions, PreviousEvent);
	if (!SelectedEvent.IsValid() && !EventOptions.IsEmpty())
	{
		SelectedEvent = EventOptions[0];
	}

	SelectedBindingScript = FindNameOption(BindingScriptOptions, PreviousScript);
	if (!SelectedBindingScript.IsValid() && !BindingScriptOptions.IsEmpty())
	{
		SelectedBindingScript = BindingScriptOptions[0];
	}

	if (LevelAsset && SelectedBindingScript.IsValid())
	{
		TArray<FName> CallbackNames;
		FString CallbackError;
		if (GridEditorLuaService::GetCallbacksForScript(*LevelAsset, *SelectedBindingScript, CallbackNames, CallbackError))
		{
			for (FName Callback : CallbackNames)
			{
				CallbackOptions.Add(MakeShared<FName>(Callback));
			}
		}
	}
	SelectedCallback = FindNameOption(CallbackOptions, PreviousCallback);
	if (!SelectedCallback.IsValid() && !CallbackOptions.IsEmpty())
	{
		SelectedCallback = CallbackOptions[0];
	}

	ConditionOptions.Add(MakeShared<EGridObjectCondition>(EGridObjectCondition::None));
	ConditionOptions.Add(MakeShared<EGridObjectCondition>(EGridObjectCondition::LevelVariableBoolEquals));
	ConditionOptions.Add(MakeShared<EGridObjectCondition>(EGridObjectCondition::LevelVariableIntCompare));
	SelectedCondition = FindEnumOption(ConditionOptions, PreviousCondition);
	if (!SelectedCondition.IsValid())
	{
		SelectedCondition = ConditionOptions[0];
	}

	if (LevelAsset && SelectedCondition.IsValid())
	{
		const EGridLevelVariableType RequiredType =
			*SelectedCondition == EGridObjectCondition::LevelVariableIntCompare ? EGridLevelVariableType::Int32 : EGridLevelVariableType::Bool;
		if (*SelectedCondition != EGridObjectCondition::None)
		{
			for (const FGridLevelVariableDefinition& Variable : LevelAsset->LevelVariables)
			{
				if (!Variable.VariableId.IsNone() && Variable.Type == RequiredType)
				{
					VariableOptions.Add(MakeShared<FName>(Variable.VariableId));
				}
			}
		}
	}
	SelectedVariable = FindNameOption(VariableOptions, PreviousVariable);
	if (!SelectedVariable.IsValid() && !VariableOptions.IsEmpty())
	{
		SelectedVariable = VariableOptions[0];
	}

	const EGridLogicIntComparison Comparisons[] = { EGridLogicIntComparison::Equal, EGridLogicIntComparison::NotEqual, EGridLogicIntComparison::Less,
		EGridLogicIntComparison::LessOrEqual, EGridLogicIntComparison::Greater, EGridLogicIntComparison::GreaterOrEqual };
	for (EGridLogicIntComparison Comparison : Comparisons)
	{
		ComparisonOptions.Add(MakeShared<EGridLogicIntComparison>(Comparison));
	}
	SelectedComparison = FindEnumOption(ComparisonOptions, PreviousComparison);
	if (!SelectedComparison.IsValid())
	{
		SelectedComparison = ComparisonOptions[0];
	}

	if (!SelectedCondition.IsValid() || *SelectedCondition == EGridObjectCondition::None)
	{
		bInvertCondition = false;
	}
}

void SGridEditorLuaScriptsPanel::Rebuild()
{
	RebuildBindingOptions();
	ChildSlot[BuildRoot()];
}

TSharedRef<SWidget> SGridEditorLuaScriptsPanel::BuildRoot()
{
	return SNew(SBorder).Padding(8.f).BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))[SNew(SScrollBox)

		+
		SScrollBox::Slot()[SNew(SVerticalBox)

			+ SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)

				  + SHorizontalBox::Slot().FillWidth(1.f)
						[SNew(STextBlock).Text(FText::FromString(TEXT("GRIMROCK LUA — SCRIPTS & BINDINGS"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))]

				  + SHorizontalBox::Slot()
						.AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("Refresh"))).OnClicked(this, &SGridEditorLuaScriptsPanel::OnRefreshClicked)]]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[BuildStatusSection()]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)[BuildScriptsSection()]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 12.f, 0.f, 0.f)[BuildBindingsSection()]]];
}

TSharedRef<SWidget> SGridEditorLuaScriptsPanel::BuildStatusSection() const
{
	const UGridLevelAsset* LevelAsset = GetLevelAsset();
	const FString LevelText = LevelAsset ? LevelAsset->GetPathName() : TEXT("No UGridLevelAsset found on the current Grid Editor actor.");

	return SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(LevelText)).AutoWrapText(true)] +
		SVerticalBox::Slot().AutoHeight().Padding(0.f, StatusText.IsEmpty() ? 0.f : 4.f, 0.f, 0.f)[SNew(STextBlock)
				.Visibility(StatusText.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
				.Text(FText::FromString(StatusText))
				.ColorAndOpacity(bStatusSuccess ? FSlateColor(FLinearColor(0.35f, 0.85f, 0.45f, 1.f)) : FSlateColor(FLinearColor(1.f, 0.35f, 0.25f, 1.f)))
				.AutoWrapText(true)];
}

TSharedRef<SWidget> SGridEditorLuaScriptsPanel::BuildScriptsSection()
{
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);
	UGridLevelAsset* LevelAsset = GetLevelAsset();

	Root->AddSlot().AutoHeight()[SNew(SHorizontalBox) +
		SHorizontalBox::Slot().FillWidth(
			1.f)[SNew(STextBlock).Text(FText::FromString(TEXT("LEVEL LUA SCRIPTS"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))] +
		SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f)[SNew(SButton)
				.Text(FText::FromString(TEXT("+ Add Script")))
				.IsEnabled(LevelAsset != nullptr)
				.OnClicked(this, &SGridEditorLuaScriptsPanel::OnAddScriptClicked)] +
		SHorizontalBox::Slot().AutoWidth()[SNew(SButton)
				.Text(FText::FromString(TEXT("Validate Lua")))
				.IsEnabled(LevelAsset != nullptr)
				.OnClicked(this, &SGridEditorLuaScriptsPanel::OnValidateClicked)]];

	if (!LevelAsset)
	{
		return Root;
	}

	for (const FGridLuaScriptSource& Script : LevelAsset->LuaScripts)
	{
		const FName ScriptId = Script.ScriptId;
		Root->AddSlot ().AutoHeight ().Padding (0.f, 4.f, 0.f, 0.f)
        [
            SNew (SBorder)
                .Padding (5.f)
                .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
                [
                    SNew (SHorizontalBox)
                    + SHorizontalBox::Slot ().AutoWidth ().VAlign (VAlign_Center)
                    [
                        SNew (SCheckBox)
                            .IsChecked (Script.bEnabled
                                ? ECheckBoxState::Checked
                                : ECheckBoxState::Unchecked)
                            .OnCheckStateChanged_Lambda ([this, ScriptId] (ECheckBoxState State)
                            {
                                if (UGridLevelAsset* Asset = GetLevelAsset ())
                                {
                                    FString Error;
                                    const bool bOk = GridEditorLuaService::SetScriptEnabled (
                                        *Asset,
                                        ScriptId,
                                        State == ECheckBoxState::Checked,
                                        Error);
                                    SetStatus (bOk ? TEXT ("Script state updated.") : Error, bOk);
                                    Rebuild ();
                                }
                            })
                    ]
                    + SHorizontalBox::Slot ().FillWidth (1.f).Padding (6.f, 0.f)
                    [
                        SNew (SButton)
                            .Text (FText::FromName (ScriptId))
                            .HAlign (HAlign_Left)
                            .OnClicked_Lambda ([this, ScriptId] ()
                            {
                                return OnSelectScriptClicked (ScriptId);
                            })
                    ]
                    + SHorizontalBox::Slot ().AutoWidth ().VAlign (VAlign_Center)
                    [
                        SNew (STextBlock)
                            .Text (FText::FromString (
                                FString::Printf (
                                    TEXT ("%d binding(s)"),
                                    GridEditorLuaService::CountScriptReferences (*LevelAsset, ScriptId))))
                    ]
                ]
        ];
	}

	if (LevelAsset->LuaScripts.IsEmpty())
	{
		Root->AddSlot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)[SNew(STextBlock).Text(FText::FromString(TEXT("No Lua scripts in this level.")))];
	}

	const FGridLuaScriptSource* Selected = FindScript(LevelAsset, SelectedScriptId);
	if (!Selected)
	{
		return Root;
	}

	Root->AddSlot ().AutoHeight ().Padding (0.f, 8.f, 0.f, 0.f)
    [
        SNew (SBorder)
            .Padding (6.f)
            .BorderImage (FAppStyle::GetBrush ("ToolPanel.DarkGroupBorder"))
            [
                SNew (SVerticalBox)
                + SVerticalBox::Slot ().AutoHeight ()
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("SELECTED SCRIPT")))
                        .Font (FCoreStyle::GetDefaultFontStyle ("Bold", 9))
                ]
                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 5.f, 0.f, 0.f)
                [
                    SNew (SEditableTextBox)
                        .Text (FText::FromString (DraftScriptId))
                        .HintText (FText::FromString (TEXT ("ScriptId")))
                        .OnTextChanged_Lambda ([this] (const FText& Text)
                        {
                            DraftScriptId = Text.ToString ();
                        })
                ]
                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 5.f, 0.f, 0.f)
                [
                    SNew (SBox)
                        .MinDesiredHeight (220.f)
                        [
                            SNew (SMultiLineEditableTextBox)
                                .Text (FText::FromString (DraftSource))
                                .HintText (FText::FromString (TEXT ("Lua source")))
                                .AutoWrapText (false)
                                .OnTextChanged_Lambda ([this] (const FText& Text)
                                {
                                    DraftSource = Text.ToString ();
                                })
                        ]
                ]
                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 5.f, 0.f, 0.f)
                [
                    SNew (SHorizontalBox)
                    + SHorizontalBox::Slot ().AutoWidth ().Padding (0.f, 0.f, 4.f, 0.f)
                    [
                        SNew (SButton)
                            .Text (FText::FromString (TEXT ("Apply")))
                            .OnClicked (this, &SGridEditorLuaScriptsPanel::OnApplyScriptClicked)
                    ]
                    + SHorizontalBox::Slot ().AutoWidth ().Padding (0.f, 0.f, 4.f, 0.f)
                    [
                        SNew (SButton)
                            .Text (FText::FromString (TEXT ("Revert")))
                            .OnClicked (this, &SGridEditorLuaScriptsPanel::OnRevertScriptClicked)
                    ]
                    + SHorizontalBox::Slot ().AutoWidth ()
                    [
                        SNew (SButton)
                            .Text (FText::FromString (TEXT ("Remove")))
                            .OnClicked (this, &SGridEditorLuaScriptsPanel::OnRemoveScriptClicked)
                    ]
                ]
                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, LastValidatedCallbacks.IsEmpty () ? 0.f : 6.f, 0.f, 0.f)
                [
                    SNew (STextBlock)
                        .Visibility (LastValidatedCallbacks.IsEmpty () ? EVisibility::Collapsed : EVisibility::Visible)
                        .Text_Lambda ([this] ()
                        {
                            TArray<FString> Names;
                            for (FName Callback : LastValidatedCallbacks)
                            {
                                Names.Add (Callback.ToString ());
                            }
                            return FText::FromString (
                                FString::Printf (
                                    TEXT ("Detected global callbacks: %s"),
                                    *FString::Join (Names, TEXT (", "))));
                        })
                        .AutoWrapText (true)
                ]
            ]
    ];

	return Root;
}

TSharedRef<SWidget> SGridEditorLuaScriptsPanel::BuildBindingsSection()
{
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);
	AGridLevelEditorActor* EditorActor = FindEditorActor();
	UGridLevelAsset* LevelAsset = EditorActor ? EditorActor->LevelAsset.Get() : nullptr;
	const FGridLevelObjectData* Source = EditorActor ? EditorActor->GetSelectedObjectData() : nullptr;

	Root->AddSlot()
		.AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("LUA BINDINGS — SELECTED GRID OBJECT"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))];

	if (!LevelAsset || !Source)
	{
		Root->AddSlot().AutoHeight().Padding(0.f, 5.f, 0.f,
			0.f)[SNew(STextBlock).Text(FText::FromString(TEXT("Select a Grid object in Grimrock Grid Editor, then press Refresh."))).AutoWrapText(true)];
		return Root;
	}

	const FString SourceName =
		!Source->LogicId.IsNone() ? Source->LogicId.ToString() : (Source->Tag.IsNone() ? Source->ObjectId.ToString().Left(8) : Source->Tag.ToString());

	Root->AddSlot().AutoHeight().Padding(
		0.f, 4.f, 0.f, 0.f)[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("Source: %s @ (%d,%d)"), *SourceName, Source->CellX, Source->CellY)))];

	Root->AddSlot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)[SNew(SHorizontalBox) +
		SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)[SNew(STextBlock).Text(FText::FromString(TEXT("Logic Id")))] +
		SHorizontalBox::Slot().FillWidth(1.f)[SNew(SEditableTextBox)
				.Text(Source->LogicId.IsNone() ? FText::GetEmpty() : FText::FromName(Source->LogicId))
				.HintText(FText::FromString(TEXT("e.g. SecretDoor")))
				.ToolTipText(FText::FromString(TEXT("Stable readable alias used by grid.command(\"SecretDoor\", \"Open\"). Must be unique in the level.")))
				.OnTextCommitted_Lambda(
					[this](const FText& Text, ETextCommit::Type CommitType)
					{
						(void)CommitType;
						AGridLevelEditorActor* CurrentEditorActor = FindEditorActor();
						if (!CurrentEditorActor)
						{
							return;
						}
						const FString Trimmed = Text.ToString().TrimStartAndEnd();
						const FName LogicId = Trimmed.IsEmpty() ? NAME_None : FName(*Trimmed);
						FString Error;
						const bool bOk = GridEditorLuaService::SetSelectedObjectLogicId(*CurrentEditorActor, LogicId, Error);
						SetStatus(bOk ? TEXT("Logic Id updated. Lua can now reference this object by name.") : Error, bOk);
						Rebuild();
					})]];

	const TArray<EGridObjectEvent> SupportedEvents = GridEditorLinkPolicy::GetSupportedEventsForSource(*Source);
	if (SupportedEvents.IsEmpty())
	{
		Root->AddSlot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)[SNew(STextBlock)
				.Text(FText::FromString(TEXT("The selected object does not emit connector events. Its Logic Id can still be used as a Lua command target.")))
				.AutoWrapText(true)];
		return Root;
	}

	Root->AddSlot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 0.f)
    [
        SNew (SBorder)
            .Padding (6.f)
            .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
            [
                SNew (SVerticalBox)
                + SVerticalBox::Slot ().AutoHeight ()
                [
                    SNew (SHorizontalBox)
                    + SHorizontalBox::Slot ().FillWidth (0.5f).Padding (0.f, 0.f, 4.f, 0.f)
                    [
                        SNew (SComboBox<TSharedPtr<EGridObjectEvent>>)
                            .OptionsSource (&EventOptions)
                            .OnGenerateWidget (this, &SGridEditorLuaScriptsPanel::MakeEventOptionWidget)
                            .OnSelectionChanged (this, &SGridEditorLuaScriptsPanel::OnEventChanged)
                            [
                                SNew (STextBlock)
                                    .Text_Lambda ([this] ()
                                    {
                                        return SelectedEvent.IsValid ()
                                            ? GetEventText (*SelectedEvent)
                                            : FText::FromString (TEXT ("Event"));
                                    })
                            ]
                    ]
                    + SHorizontalBox::Slot ().FillWidth (0.5f).Padding (4.f, 0.f, 0.f, 0.f)
                    [
                        SNew (SComboBox<TSharedPtr<FName>>)
                            .OptionsSource (&BindingScriptOptions)
                            .OnGenerateWidget (this, &SGridEditorLuaScriptsPanel::MakeNameOptionWidget)
                            .OnSelectionChanged (this, &SGridEditorLuaScriptsPanel::OnBindingScriptChanged)
                            [
                                SNew (STextBlock)
                                    .Text_Lambda ([this] ()
                                    {
                                        return SelectedBindingScript.IsValid ()
                                            ? FText::FromName (*SelectedBindingScript)
                                            : FText::FromString (TEXT ("Script"));
                                    })
                            ]
                    ]
                ]
                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 4.f, 0.f, 0.f)
                [
                    SNew (SComboBox<TSharedPtr<FName>>)
                        .OptionsSource (&CallbackOptions)
                        .OnGenerateWidget (this, &SGridEditorLuaScriptsPanel::MakeNameOptionWidget)
                        .OnSelectionChanged (this, &SGridEditorLuaScriptsPanel::OnCallbackChanged)
                        [
                            SNew (STextBlock)
                                .Text_Lambda ([this] ()
                                {
                                    return SelectedCallback.IsValid ()
                                        ? FText::FromName (*SelectedCallback)
                                        : FText::FromString (TEXT ("Callback"));
                                })
                        ]
                ]
                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 4.f, 0.f, 0.f)
                [
                    SNew (SComboBox<TSharedPtr<EGridObjectCondition>>)
                        .OptionsSource (&ConditionOptions)
                        .OnGenerateWidget (this, &SGridEditorLuaScriptsPanel::MakeConditionOptionWidget)
                        .OnSelectionChanged (this, &SGridEditorLuaScriptsPanel::OnConditionChanged)
                        [
                            SNew (STextBlock)
                                .Text_Lambda ([this] ()
                                {
                                    return SelectedCondition.IsValid ()
                                        ? GetConditionText (*SelectedCondition)
                                        : FText::FromString (TEXT ("Condition"));
                                })
                        ]
                ]
                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 4.f, 0.f, 0.f)
                [
                    SNew (SBox)
                        .Visibility_Lambda ([this] ()
                        {
                            return SelectedCondition.IsValid () &&
                                *SelectedCondition != EGridObjectCondition::None
                                    ? EVisibility::Visible
                                    : EVisibility::Collapsed;
                        })
                        [
                            SNew (SComboBox<TSharedPtr<FName>>)
                                .OptionsSource (&VariableOptions)
                                .OnGenerateWidget (this, &SGridEditorLuaScriptsPanel::MakeNameOptionWidget)
                                .OnSelectionChanged (this, &SGridEditorLuaScriptsPanel::OnVariableChanged)
                                [
                                    SNew (STextBlock)
                                        .Text_Lambda ([this] ()
                                        {
                                            return SelectedVariable.IsValid ()
                                                ? FText::FromName (*SelectedVariable)
                                                : FText::FromString (TEXT ("Variable"));
                                        })
                                ]
                        ]
                ]
                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 4.f, 0.f, 0.f)
                [
                    SNew (SBox)
                        .Visibility_Lambda ([this] ()
                        {
                            return SelectedCondition.IsValid () &&
                                *SelectedCondition == EGridObjectCondition::LevelVariableBoolEquals
                                    ? EVisibility::Visible
                                    : EVisibility::Collapsed;
                        })
                        [
                            SNew (SCheckBox)
                                .IsChecked_Lambda ([this] ()
                                {
                                    return ConditionBoolValue
                                        ? ECheckBoxState::Checked
                                        : ECheckBoxState::Unchecked;
                                })
                                .OnCheckStateChanged_Lambda ([this] (ECheckBoxState State)
                                {
                                    ConditionBoolValue = State == ECheckBoxState::Checked;
                                })
                                [
                                    SNew (STextBlock)
                                        .Text (FText::FromString (TEXT ("Expected value = true")))
                                ]
                        ]
                ]
                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 4.f, 0.f, 0.f)
                [
                    SNew (SBox)
                        .Visibility_Lambda ([this] ()
                        {
                            return SelectedCondition.IsValid () &&
                                *SelectedCondition == EGridObjectCondition::LevelVariableIntCompare
                                    ? EVisibility::Visible
                                    : EVisibility::Collapsed;
                        })
                        [
                            SNew (SHorizontalBox)
                            + SHorizontalBox::Slot ().FillWidth (0.5f).Padding (0.f, 0.f, 4.f, 0.f)
                            [
                                SNew (SComboBox<TSharedPtr<EGridLogicIntComparison>>)
                                    .OptionsSource (&ComparisonOptions)
                                    .OnGenerateWidget (this, &SGridEditorLuaScriptsPanel::MakeComparisonOptionWidget)
                                    .OnSelectionChanged (this, &SGridEditorLuaScriptsPanel::OnComparisonChanged)
                                    [
                                        SNew (STextBlock)
                                            .Text_Lambda ([this] ()
                                            {
                                                return SelectedComparison.IsValid ()
                                                    ? GetComparisonText (*SelectedComparison)
                                                    : FText::FromString (TEXT ("Comparison"));
                                            })
                                    ]
                            ]
                            + SHorizontalBox::Slot ().FillWidth (0.5f).Padding (4.f, 0.f, 0.f, 0.f)
                            [
                                SNew (SNumericEntryBox<int32>)
                                    .Value_Lambda ([this] () -> TOptional<int32>
                                    {
                                        return ConditionIntValue;
                                    })
                                    .OnValueChanged_Lambda ([this] (int32 Value)
                                    {
                                        ConditionIntValue = Value;
                                    })
                            ]
                        ]
                ]
                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 4.f, 0.f, 0.f)
                [
                    SNew (SBox)
                        .Visibility_Lambda ([this] ()
                        {
                            return SelectedCondition.IsValid () &&
                                *SelectedCondition != EGridObjectCondition::None
                                    ? EVisibility::Visible
                                    : EVisibility::Collapsed;
                        })
                        [
                            SNew (SCheckBox)
                                .IsChecked_Lambda ([this] ()
                                {
                                    return bInvertCondition
                                        ? ECheckBoxState::Checked
                                        : ECheckBoxState::Unchecked;
                                })
                                .OnCheckStateChanged_Lambda ([this] (ECheckBoxState State)
                                {
                                    bInvertCondition = State == ECheckBoxState::Checked;
                                })
                                [
                                    SNew (STextBlock)
                                        .Text (FText::FromString (TEXT ("Invert condition")))
                                ]
                        ]
                ]
                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 0.f)
                [
                    SNew (SButton)
                        .Text (FText::FromString (TEXT ("Create Lua Binding")))
                        .IsEnabled_Lambda ([this] ()
                        {
                            const bool bVariableOk =
                                !SelectedCondition.IsValid () ||
                                *SelectedCondition == EGridObjectCondition::None ||
                                SelectedVariable.IsValid ();
                            return SelectedEvent.IsValid () &&
                                SelectedBindingScript.IsValid () &&
                                SelectedCallback.IsValid () &&
                                SelectedCondition.IsValid () &&
                                bVariableOk;
                        })
                        .OnClicked (this, &SGridEditorLuaScriptsPanel::OnCreateBindingClicked)
                ]
            ]
    ];

	int32 BindingCount = 0;
	for (int32 LinkIndex = 0; LinkIndex < LevelAsset->Links.Num(); ++LinkIndex)
	{
		const FGridObjectLink& Link = LevelAsset->Links[LinkIndex];
		if (Link.SourceObjectId != Source->ObjectId || Link.Command != EGridObjectCommand::LuaCallback)
		{
			continue;
		}
		++BindingCount;
		Root->AddSlot ().AutoHeight ().Padding (0.f, 5.f, 0.f, 0.f)
        [
            SNew (SBorder)
                .Padding (5.f)
                .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
                [
                    SNew (SHorizontalBox)
                    + SHorizontalBox::Slot ().FillWidth (1.f)
                    [
                        SNew (STextBlock)
                            .Text (FText::FromString (GetLuaBindingSummary (Link)))
                            .AutoWrapText (true)
                    ]
                    + SHorizontalBox::Slot ().AutoWidth ().Padding (5.f, 0.f, 0.f, 0.f)
                    [
                        SNew (SButton)
                            .Text (FText::FromString (TEXT ("Remove")))
                            .OnClicked_Lambda ([this, LinkIndex] ()
                            {
                                return OnRemoveBindingClicked (LinkIndex);
                            })
                    ]
                ]
        ];
	}

	if (BindingCount == 0)
	{
		Root->AddSlot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)[SNew(STextBlock).Text(FText::FromString(TEXT("No Lua bindings for the selected object.")))];
	}
	return Root;
}

FReply SGridEditorLuaScriptsPanel::OnRefreshClicked()
{
	Rebuild();
	return FReply::Handled();
}

FReply SGridEditorLuaScriptsPanel::OnAddScriptClicked()
{
	UGridLevelAsset* LevelAsset = GetLevelAsset();
	if (!LevelAsset)
	{
		return FReply::Handled();
	}
	const FName NewId = GridEditorLuaService::MakeUniqueScriptId(*LevelAsset);
	FString Error;
	const bool bOk = GridEditorLuaService::AddScript(*LevelAsset, NewId, TEXT("function on_event(event)\n    -- level script\nend\n"), Error);
	if (bOk)
	{
		SelectedScriptId = NewId;
		LoadSelectedScriptDraft();
	}
	SetStatus(bOk ? TEXT("Lua script added.") : Error, bOk);
	Rebuild();
	return FReply::Handled();
}

FReply SGridEditorLuaScriptsPanel::OnSelectScriptClicked(FName ScriptId)
{
	SelectedScriptId = ScriptId;
	LastValidatedCallbacks.Reset();
	LoadSelectedScriptDraft();
	Rebuild();
	return FReply::Handled();
}

FReply SGridEditorLuaScriptsPanel::OnApplyScriptClicked()
{
	UGridLevelAsset* LevelAsset = GetLevelAsset();
	if (!LevelAsset || SelectedScriptId.IsNone())
	{
		return FReply::Handled();
	}

	const FString TrimmedId = DraftScriptId.TrimStartAndEnd();
	const FName NewId = TrimmedId.IsEmpty() ? NAME_None : FName(*TrimmedId);
	FString Error;
	bool bOk = GridEditorLuaService::RenameScript(*LevelAsset, SelectedScriptId, NewId, Error);
	if (bOk)
	{
		SelectedScriptId = NewId;
		bOk = GridEditorLuaService::SetScriptSource(*LevelAsset, SelectedScriptId, DraftSource, Error);
	}
	SetStatus(bOk ? TEXT("Lua script applied. Persistent declarations synchronized; run Validate Lua before playtest.") : Error, bOk);
	LastValidatedCallbacks.Reset();
	if (bOk)
	{
		LoadSelectedScriptDraft();
	}
	Rebuild();
	return FReply::Handled();
}

FReply SGridEditorLuaScriptsPanel::OnRevertScriptClicked()
{
	LoadSelectedScriptDraft();
	SetStatus(TEXT("Lua draft reverted."), true);
	Rebuild();
	return FReply::Handled();
}

FReply SGridEditorLuaScriptsPanel::OnRemoveScriptClicked()
{
	UGridLevelAsset* LevelAsset = GetLevelAsset();
	if (!LevelAsset || SelectedScriptId.IsNone())
	{
		return FReply::Handled();
	}
	FString Error;
	const bool bOk = GridEditorLuaService::RemoveScript(*LevelAsset, SelectedScriptId, Error);
	if (bOk)
	{
		SelectedScriptId = NAME_None;
		DraftScriptId.Reset();
		DraftSource.Reset();
		LastValidatedCallbacks.Reset();
	}
	SetStatus(bOk ? TEXT("Lua script removed.") : Error, bOk);
	Rebuild();
	return FReply::Handled();
}

FReply SGridEditorLuaScriptsPanel::OnValidateClicked()
{
	UGridLevelAsset* LevelAsset = GetLevelAsset();
	if (!LevelAsset)
	{
		return FReply::Handled();
	}

	FGridEditorLuaAnalysis Analysis;
	const bool bValid = GridEditorLuaService::AnalyzeLevel(*LevelAsset, Analysis);
	LastValidatedCallbacks.Reset();
	for (const FGridEditorLuaScriptAnalysis& Script : Analysis.Scripts)
	{
		if (Script.ScriptId == SelectedScriptId)
		{
			LastValidatedCallbacks = Script.CallbackNames;
			break;
		}
	}

	if (bValid)
	{
		SetStatus(FString::Printf(TEXT("Lua validation OK: %d script(s), VM build accepted."), Analysis.Scripts.Num()), true);
	}
	else
	{
		FString Error = Analysis.GlobalError;
		if (Error.IsEmpty())
		{
			for (const FGridEditorLuaScriptAnalysis& Script : Analysis.Scripts)
			{
				if (Script.bEnabled && !Script.bValid)
				{
					Error = FString::Printf(TEXT("%s: %s"), *Script.ScriptId.ToString(), *Script.Error);
					break;
				}
			}
		}
		SetStatus(Error.IsEmpty() ? TEXT("Lua validation failed.") : Error, false);
	}
	Rebuild();
	return FReply::Handled();
}

FReply SGridEditorLuaScriptsPanel::OnCreateBindingClicked()
{
	AGridLevelEditorActor* EditorActor = FindEditorActor();
	const FGridLevelObjectData* Source = EditorActor ? EditorActor->GetSelectedObjectData() : nullptr;
	if (!EditorActor || !Source || !SelectedEvent.IsValid() || !SelectedBindingScript.IsValid() || !SelectedCallback.IsValid() || !SelectedCondition.IsValid())
	{
		return FReply::Handled();
	}

	FGridObjectLink Link;
	Link.SourceObjectId = Source->ObjectId;
	Link.SourceEvent = *SelectedEvent;
	Link.Command = EGridObjectCommand::LuaCallback;
	Link.LuaScriptId = *SelectedBindingScript;
	Link.LuaCallbackName = *SelectedCallback;
	Link.Condition = *SelectedCondition;
	Link.bInvertCondition = Link.Condition != EGridObjectCondition::None && bInvertCondition;
	if (Link.Condition == EGridObjectCondition::LevelVariableBoolEquals)
	{
		Link.ConditionVariableId = SelectedVariable.IsValid() ? *SelectedVariable : NAME_None;
		Link.ConditionBoolValue = ConditionBoolValue;
	}
	else if (Link.Condition == EGridObjectCondition::LevelVariableIntCompare)
	{
		Link.ConditionVariableId = SelectedVariable.IsValid() ? *SelectedVariable : NAME_None;
		Link.ConditionIntComparison = SelectedComparison.IsValid() ? *SelectedComparison : EGridLogicIntComparison::Equal;
		Link.ConditionIntValue = ConditionIntValue;
	}

	FString Error;
	const bool bOk = GridEditorLuaService::AddLuaLink(*EditorActor, Link, Error);
	SetStatus(bOk ? TEXT("Lua binding created.") : Error, bOk);
	Rebuild();
	return FReply::Handled();
}

FReply SGridEditorLuaScriptsPanel::OnRemoveBindingClicked(int32 LinkIndex)
{
	AGridLevelEditorActor* EditorActor = FindEditorActor();
	UGridLevelAsset* LevelAsset = EditorActor ? EditorActor->LevelAsset.Get() : nullptr;
	if (!EditorActor || !LevelAsset || !LevelAsset->Links.IsValidIndex(LinkIndex))
	{
		return FReply::Handled();
	}
	const FGridObjectLink Link = LevelAsset->Links[LinkIndex];
	const bool bOk = GridEditorLuaService::RemoveLuaLink(*EditorActor, Link);
	SetStatus(bOk ? TEXT("Lua binding removed.") : TEXT("Lua binding could not be removed."), bOk);
	Rebuild();
	return FReply::Handled();
}

TSharedRef<SWidget> SGridEditorLuaScriptsPanel::MakeEventOptionWidget(TSharedPtr<EGridObjectEvent> Item) const
{
	return SNew(STextBlock).Text(Item.IsValid() ? GetEventText(*Item) : FText::FromString(TEXT("Invalid")));
}

TSharedRef<SWidget> SGridEditorLuaScriptsPanel::MakeNameOptionWidget(TSharedPtr<FName> Item) const
{
	return SNew(STextBlock).Text(Item.IsValid() ? FText::FromName(*Item) : FText::FromString(TEXT("Invalid")));
}

TSharedRef<SWidget> SGridEditorLuaScriptsPanel::MakeConditionOptionWidget(TSharedPtr<EGridObjectCondition> Item) const
{
	return SNew(STextBlock).Text(Item.IsValid() ? GetConditionText(*Item) : FText::FromString(TEXT("Invalid")));
}

TSharedRef<SWidget> SGridEditorLuaScriptsPanel::MakeComparisonOptionWidget(TSharedPtr<EGridLogicIntComparison> Item) const
{
	return SNew(STextBlock).Text(Item.IsValid() ? GetComparisonText(*Item) : FText::FromString(TEXT("Invalid")));
}

void SGridEditorLuaScriptsPanel::OnEventChanged(TSharedPtr<EGridObjectEvent> Item, ESelectInfo::Type)
{
	SelectedEvent = Item;
}

void SGridEditorLuaScriptsPanel::OnBindingScriptChanged(TSharedPtr<FName> Item, ESelectInfo::Type)
{
	SelectedBindingScript = Item;
	SelectedCallback.Reset();
	Rebuild();
}

void SGridEditorLuaScriptsPanel::OnCallbackChanged(TSharedPtr<FName> Item, ESelectInfo::Type)
{
	SelectedCallback = Item;
}

void SGridEditorLuaScriptsPanel::OnConditionChanged(TSharedPtr<EGridObjectCondition> Item, ESelectInfo::Type)
{
	SelectedCondition = Item;
	SelectedVariable.Reset();
	if (SelectedCondition.IsValid() && *SelectedCondition == EGridObjectCondition::None)
	{
		bInvertCondition = false;
	}
	Rebuild();
}

void SGridEditorLuaScriptsPanel::OnVariableChanged(TSharedPtr<FName> Item, ESelectInfo::Type)
{
	SelectedVariable = Item;
}

void SGridEditorLuaScriptsPanel::OnComparisonChanged(TSharedPtr<EGridLogicIntComparison> Item, ESelectInfo::Type)
{
	SelectedComparison = Item;
}

FText SGridEditorLuaScriptsPanel::GetEventText(EGridObjectEvent Event) const
{
	const UEnum* Enum = StaticEnum<EGridObjectEvent>();
	return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Event)) : FText::FromString(TEXT("Unknown"));
}

FText SGridEditorLuaScriptsPanel::GetConditionText(EGridObjectCondition Condition) const
{
	const UEnum* Enum = StaticEnum<EGridObjectCondition>();
	return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Condition)) : FText::FromString(TEXT("Unknown"));
}

FText SGridEditorLuaScriptsPanel::GetComparisonText(EGridLogicIntComparison Comparison) const
{
	const UEnum* Enum = StaticEnum<EGridLogicIntComparison>();
	return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Comparison)) : FText::FromString(TEXT("Unknown"));
}

FString SGridEditorLuaScriptsPanel::GetLuaBindingSummary(const FGridObjectLink& Link) const
{
	FString Result =
		FString::Printf(TEXT("%s -> Lua %s.%s"), *GetEventText(Link.SourceEvent).ToString(), *Link.LuaScriptId.ToString(), *Link.LuaCallbackName.ToString());
	if (Link.Condition == EGridObjectCondition::LevelVariableBoolEquals)
	{
		Result += FString::Printf(TEXT(" if %s == %s"), *Link.ConditionVariableId.ToString(), Link.ConditionBoolValue ? TEXT("true") : TEXT("false"));
	}
	else if (Link.Condition == EGridObjectCondition::LevelVariableIntCompare)
	{
		Result += FString::Printf(
			TEXT(" if %s %s %d"), *Link.ConditionVariableId.ToString(), *GetComparisonText(Link.ConditionIntComparison).ToString(), Link.ConditionIntValue);
	}
	if (Link.bInvertCondition)
	{
		Result += TEXT(" [NOT]");
	}
	return Result;
}

#endif

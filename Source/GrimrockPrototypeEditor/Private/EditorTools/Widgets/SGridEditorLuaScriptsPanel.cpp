#include "EditorTools/Widgets/SGridEditorLuaScriptsPanel.h"

#if WITH_EDITOR

#include "Core/GridLevelAsset.h"
#include "Editor.h"
#include "EditorTools/GridEditorLuaService.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "EditorTools/GridLuaAuthoringCompiler.h"
#include "EditorTools/GridLuaSyntaxHighlighter.h"
#include "EngineUtils.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
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
			}) : nullptr;
	}

	FString GetPersistentDefinitionText(const FGridLuaPersistentVariableDefinition& Definition)
	{
		if (Definition.Type == EGridLuaPersistentValueType::Bool)
		{
			return FString::Printf(TEXT("%s : Bool = %s"), *Definition.VariableId.ToString(), Definition.bDefaultBoolValue ? TEXT("true") : TEXT("false"));
		}
		return FString::Printf(TEXT("%s : Int32 = %d"), *Definition.VariableId.ToString(), Definition.DefaultInt32Value);
	}
}

void SGridEditorLuaScriptsPanel::Construct(const FArguments& InArgs)
{
	(void)InArgs;
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
		PersistentDefinitions.Reset();
		return;
	}

	DraftScriptId = Script->ScriptId.ToString();
	DraftSource = Script->Source;
	RefreshSelectedScriptMetadata();
}

void SGridEditorLuaScriptsPanel::RefreshSelectedScriptMetadata()
{
	LastValidatedCallbacks.Reset();
	PersistentDefinitions.Reset();

	UGridLevelAsset* LevelAsset = GetLevelAsset();
	const FGridLuaScriptSource* Script = FindScript(LevelAsset, SelectedScriptId);
	if (!LevelAsset || !Script || !Script->bEnabled)
	{
		return;
	}

	FString CallbackError;
	GridEditorLuaService::GetCallbacksForScript(*LevelAsset, Script->ScriptId, LastValidatedCallbacks, CallbackError);

	FGridLuaVm Vm;
	FString VmError;
	if (Vm.Reload({ *Script }, FGridLuaVmConfig(), VmError))
	{
		PersistentDefinitions = Vm.GetPersistentVariableDefinitions();
	}
}

void SGridEditorLuaScriptsPanel::Rebuild()
{
	ChildSlot[BuildRoot()];
}

TSharedRef<SWidget> SGridEditorLuaScriptsPanel::BuildRoot()
{
	return SNew(SBorder)
		.Padding(8.f)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
						.Text(FText::FromString(TEXT("GRIMROCK LUA — SCRIPTS")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
				[
					BuildStatusSection()
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
				[
					BuildScriptsSection()
				]
			]
		];
}

TSharedRef<SWidget> SGridEditorLuaScriptsPanel::BuildStatusSection() const
{
	const UGridLevelAsset* LevelAsset = GetLevelAsset();
	const FString LevelText = LevelAsset ? LevelAsset->GetPathName() : TEXT("No UGridLevelAsset found on the current Grid Editor actor.");

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Text(FText::FromString(LevelText)).AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, StatusText.IsEmpty() ? 0.f : 4.f, 0.f, 0.f)
		[
			SNew(STextBlock)
				.Visibility(StatusText.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
				.Text(FText::FromString(StatusText))
				.ColorAndOpacity(bStatusSuccess ? FSlateColor(FLinearColor(0.35f, 0.85f, 0.45f, 1.f)) : FSlateColor(FLinearColor(1.f, 0.35f, 0.25f, 1.f)))
				.AutoWrapText(true)
		];
}

TSharedRef<SWidget> SGridEditorLuaScriptsPanel::BuildScriptsSection()
{
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);
	UGridLevelAsset* LevelAsset = GetLevelAsset();

	Root->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[
			SNew(STextBlock)
				.Text(FText::FromString(TEXT("LEVEL LUA SCRIPTS")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f)
		[
			SNew(SButton)
				.Text(FText::FromString(TEXT("+ Add Script")))
				.IsEnabled(LevelAsset != nullptr)
				.OnClicked(this, &SGridEditorLuaScriptsPanel::OnAddScriptClicked)
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SButton)
				.Text(FText::FromString(TEXT("Compile Lua")))
				.IsEnabled(LevelAsset != nullptr)
				.OnClicked(this, &SGridEditorLuaScriptsPanel::OnValidateClicked)
		]
	];

	if (!LevelAsset)
	{
		return Root;
	}

	for (const FGridLuaScriptSource& Script : LevelAsset->LuaScripts)
	{
		const FName ScriptId = Script.ScriptId;
		Root->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
		[
			SNew(SBorder)
				.Padding(5.f)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SCheckBox)
							.IsChecked(Script.bEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
							.OnCheckStateChanged_Lambda([this, ScriptId](ECheckBoxState State)
							{
								if (UGridLevelAsset* Asset = GetLevelAsset())
								{
									FString Error;
									const bool bOk = GridEditorLuaService::SetScriptEnabled(*Asset, ScriptId, State == ECheckBoxState::Checked, Error);
									SetStatus(bOk ? TEXT("Script state updated. Use Compile Lua to validate the enabled script set.") : Error, bOk);
									if (ScriptId == SelectedScriptId)
									{
										LoadSelectedScriptDraft();
									}
									Rebuild();
								}
							})
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(6.f, 0.f)
					[
						SNew(SButton)
							.Text(FText::FromName(ScriptId))
							.HAlign(HAlign_Left)
							.OnClicked_Lambda([this, ScriptId]()
							{
								return OnSelectScriptClicked(ScriptId);
							})
					]
				]
		];
	}

	if (LevelAsset->LuaScripts.IsEmpty())
	{
		Root->AddSlot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("No Lua scripts in this level.")))
		];
	}

	const FGridLuaScriptSource* Selected = FindScript(LevelAsset, SelectedScriptId);
	if (!Selected)
	{
		return Root;
	}

	Root->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
	[
		SNew(SBorder)
			.Padding(6.f)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
						.Text(FText::FromString(TEXT("SELECTED SCRIPT")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)
				[
					SNew(SEditableTextBox)
						.Text(FText::FromString(DraftScriptId))
						.HintText(FText::FromString(TEXT("Script Id")))
						.OnTextChanged_Lambda([this](const FText& Text)
						{
							DraftScriptId = Text.ToString();
						})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)
				[
					SNew(SBox)
						.MinDesiredHeight(320.f)
						[
							SNew(SMultiLineEditableTextBox)
								.Marshaller(FGridLuaSyntaxHighlighter::Create())
								.Font(FCoreStyle::GetDefaultFontStyle("Mono", 10))
								.Text(FText::FromString(DraftSource))
								.HintText(FText::FromString(TEXT("Lua source")))
								.AutoWrapText(false)
								.OnTextChanged_Lambda([this](const FText& Text)
								{
									DraftSource = Text.ToString();
								})
						]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
					[
						SNew(SButton).Text(FText::FromString(TEXT("Apply"))).OnClicked(this, &SGridEditorLuaScriptsPanel::OnApplyScriptClicked)
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
					[
						SNew(SButton).Text(FText::FromString(TEXT("Revert"))).OnClicked(this, &SGridEditorLuaScriptsPanel::OnRevertScriptClicked)
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton).Text(FText::FromString(TEXT("Remove"))).OnClicked(this, &SGridEditorLuaScriptsPanel::OnRemoveScriptClicked)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
				[
					SNew(STextBlock)
						.Text(FText::FromString(TEXT("DETECTED CALLBACKS")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f, 0.f, 0.f)
				[
					SNew(STextBlock)
						.Text_Lambda([this]()
						{
							if (LastValidatedCallbacks.IsEmpty())
							{
								return FText::FromString(TEXT("None"));
							}
							TArray<FString> Names;
							for (FName Callback : LastValidatedCallbacks)
							{
								Names.Add(Callback.ToString());
							}
							return FText::FromString(FString::Join(Names, TEXT("\n")));
						})
						.AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
				[
					BuildPersistentStateSection()
				]
			]
	];

	return Root;
}

TSharedRef<SWidget> SGridEditorLuaScriptsPanel::BuildPersistentStateSection() const
{
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
				.Text(FText::FromString(TEXT("PERSISTENT STATE")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 4.f)
		[
			SNew(STextBlock)
				.Text(FText::FromString(TEXT("Declared in Lua and automatically synchronized with the level save state.")))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.70f, 0.70f, 0.70f, 1.f)))
				.AutoWrapText(true)
		];

	if (PersistentDefinitions.IsEmpty())
	{
		Root->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("None")))];
		return Root;
	}

	for (const FGridLuaPersistentVariableDefinition& Definition : PersistentDefinitions)
	{
		Root->AddSlot().AutoHeight().Padding(0.f, 1.f)
		[
			SNew(STextBlock).Text(FText::FromString(GetPersistentDefinitionText(Definition)))
		];
	}
	return Root;
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
	const bool bOk = GridEditorLuaService::AddScript(*LevelAsset, NewId,
		TEXT("function on_event(event)\n    -- level script\nend\n"), Error);
	if (bOk)
	{
		SelectedScriptId = NewId;
		LoadSelectedScriptDraft();
	}
	SetStatus(bOk ? TEXT("Lua script added. Use Apply or Compile Lua after editing.") : Error, bOk);
	Rebuild();
	return FReply::Handled();
}

FReply SGridEditorLuaScriptsPanel::OnSelectScriptClicked(FName ScriptId)
{
	SelectedScriptId = ScriptId;
	LoadSelectedScriptDraft();
	Rebuild();
	return FReply::Handled();
}

FReply SGridEditorLuaScriptsPanel::OnApplyScriptClicked()
{
	AGridLevelEditorActor* EditorActor = FindEditorActor();
	UGridLevelAsset* LevelAsset = EditorActor ? EditorActor->LevelAsset.Get() : nullptr;
	if (!EditorActor || !LevelAsset || SelectedScriptId.IsNone())
	{
		return FReply::Handled();
	}

	const FString TrimmedId = DraftScriptId.TrimStartAndEnd();
	const FName NewId = TrimmedId.IsEmpty() ? NAME_None : FName(*TrimmedId);

	// LUA-COMP01: compile the exact in-memory candidate before RenameScript or
	// SetScriptSource is allowed to mutate the LevelAsset.
	FGridLuaCompileResult CompileResult;
	if (!FGridLuaAuthoringCompiler::CompileScriptDraft(*EditorActor, SelectedScriptId, NewId, DraftSource, CompileResult))
	{
		SetStatus(CompileResult.GetSummaryText(), false);
		Rebuild();
		return FReply::Handled();
	}

	FString Error;
	bool bOk = GridEditorLuaService::RenameScript(*LevelAsset, SelectedScriptId, NewId, Error);
	if (bOk)
	{
		SelectedScriptId = NewId;
		bOk = GridEditorLuaService::SetScriptSource(*LevelAsset, SelectedScriptId, DraftSource, Error);
	}
	SetStatus(bOk ? TEXT("Lua script compiled and applied. Persistent state synchronized automatically.") : Error, bOk);
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
		PersistentDefinitions.Reset();
	}
	SetStatus(bOk ? TEXT("Lua script removed.") : Error, bOk);
	Rebuild();
	return FReply::Handled();
}

FReply SGridEditorLuaScriptsPanel::OnValidateClicked()
{
	AGridLevelEditorActor* EditorActor = FindEditorActor();
	if (!EditorActor || !EditorActor->LevelAsset)
	{
		return FReply::Handled();
	}

	FGridLuaCompileResult CompileResult;
	const bool bValid = FGridLuaAuthoringCompiler::CompileLevel(*EditorActor, CompileResult);
	RefreshSelectedScriptMetadata();
	SetStatus(CompileResult.GetSummaryText(), bValid);
	Rebuild();
	return FReply::Handled();
}

#endif

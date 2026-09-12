#include "EditorTools/Widgets/SGridEditorLinksPanel.h"

#if WITH_EDITOR

#include "Core/GridLevelAsset.h"
#include "EditorTools/GridEditorLinkPolicy.h"
#include "EditorTools/GridEditorLinkService.h"
#include "EditorTools/GridEditorLuaService.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FName CommandActionType(TEXT("Command"));
	const FName LuaActionType(TEXT("Lua Callback"));

	template <typename T>
	TSharedPtr<T> FindEnumOption(const TArray<TSharedPtr<T>>& Options, const TSharedPtr<T>& Current)
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

	TSharedPtr<FGuid> FindGuidOption(const TArray<TSharedPtr<FGuid>>& Options, FGuid Value)
	{
		for (const TSharedPtr<FGuid>& Option : Options)
		{
			if (Option.IsValid() && *Option == Value)
			{
				return Option;
			}
		}
		return nullptr;
	}

	EGridLogicNodeType GetLogicNodeType(const UGridLevelAsset* LevelAsset, FGuid ObjectId)
	{
		if (LevelAsset)
		{
			if (const FGridLogicObjectInstance* Logic = LevelAsset->FindLogicObjectInstanceById(ObjectId))
			{
				return Logic->Logic.NodeType;
			}
		}
		return EGridLogicNodeType::Relay;
	}
}

void SGridEditorLinksPanel::Construct(const FArguments& InArgs)
{
	EditorActor = InArgs._EditorActor;
	OnGetEditorActor = InArgs._OnGetEditorActor;
	OnRequestRefresh = InArgs._OnRequestRefresh;
	ActionTypeOptions = { MakeShared<FName>(CommandActionType), MakeShared<FName>(LuaActionType) };
	SelectedActionType = ActionTypeOptions[0];
	RefreshOptions();
	ChildSlot[BuildRoot()];
}

AGridLevelEditorActor* SGridEditorLinksPanel::GetEditorActor() const
{
	if (EditorActor.IsValid())
	{
		return EditorActor.Get();
	}
	return OnGetEditorActor.IsBound() ? OnGetEditorActor.Execute() : nullptr;
}

UGridLevelAsset* SGridEditorLinksPanel::GetLevelAsset() const
{
	AGridLevelEditorActor* Actor = GetEditorActor();
	return Actor ? Actor->LevelAsset.Get() : nullptr;
}

void SGridEditorLinksPanel::RequestRefresh() const
{
	if (OnRequestRefresh.IsBound())
	{
		OnRequestRefresh.Execute();
	}
}

void SGridEditorLinksPanel::Rebuild()
{
	RefreshOptions();
	ChildSlot[BuildRoot()];
}

void SGridEditorLinksPanel::RefreshOptions()
{
	BuildEventOptions();
	BuildTargetOptions();
	BuildCommandOptions();
	BuildScriptOptions();
	BuildCallbackOptions();
}

void SGridEditorLinksPanel::BuildEventOptions()
{
	const TSharedPtr<EGridObjectEvent> Previous = SelectedEvent;
	EventOptions.Reset();

	AGridLevelEditorActor* Actor = GetEditorActor();
	UGridLevelAsset* LevelAsset = Actor ? Actor->LevelAsset.Get() : nullptr;
	const FGuid SourceId = Actor ? Actor->LastSelectedObjectId : FGuid();
	if (LevelAsset && LevelAsset->ContainsTypedPlacementId(SourceId))
	{
		const EGridLevelObjectType SourceType = LevelAsset->GetTypedPlacementType(SourceId);
		for (const EGridObjectEvent Event : GridEditorLinkPolicy::GetSupportedEventsForSource(SourceType, GetLogicNodeType(LevelAsset, SourceId)))
		{
			EventOptions.Add(MakeShared<EGridObjectEvent>(Event));
		}
	}

	SelectedEvent = FindEnumOption(EventOptions, Previous);
	if (!SelectedEvent.IsValid() && !EventOptions.IsEmpty())
	{
		SelectedEvent = EventOptions[0];
	}
}

void SGridEditorLinksPanel::BuildTargetOptions()
{
	const FGuid Previous = SelectedTarget.IsValid() ? *SelectedTarget : FGuid();
	TargetOptions.Reset();
	UGridLevelAsset* LevelAsset = GetLevelAsset();
	if (!LevelAsset)
	{
		SelectedTarget.Reset();
		return;
	}

	const auto AddTarget = [this, LevelAsset](FGuid ObjectId)
	{
		if (!ObjectId.IsValid())
		{
			return;
		}
		const EGridLevelObjectType Type = LevelAsset->GetTypedPlacementType(ObjectId);
		if (Type != EGridLevelObjectType::None && GridEditorLinkPolicy::CanObjectReceiveCommands(Type, GetLogicNodeType(LevelAsset, ObjectId)))
		{
			TargetOptions.Add(MakeShared<FGuid>(ObjectId));
		}
	};

	for (const FGridWorldObjectInstance& Instance : LevelAsset->WorldObjectInstances)
	{
		AddTarget(Instance.InstanceId);
	}
	for (const FGridLooseItemInstance& Instance : LevelAsset->LooseItemInstances)
	{
		AddTarget(Instance.InstanceId);
	}
	for (const FGridMonsterSpawnInstance& Spawn : LevelAsset->MonsterSpawns)
	{
		AddTarget(Spawn.SpawnId);
	}
	for (const FGridItemSpawnInstance& Spawn : LevelAsset->ItemSpawns)
	{
		AddTarget(Spawn.SpawnId);
	}
	for (const FGridLogicObjectInstance& Instance : LevelAsset->LogicObjects)
	{
		AddTarget(Instance.InstanceId);
	}

	TargetOptions.Sort([this](const TSharedPtr<FGuid>& A, const TSharedPtr<FGuid>& B)
	{
		return A.IsValid() && B.IsValid() ? GetObjectSummary(*A) < GetObjectSummary(*B) : A.IsValid();
	});

	SelectedTarget = FindGuidOption(TargetOptions, Previous);
	if (!SelectedTarget.IsValid() && !TargetOptions.IsEmpty())
	{
		SelectedTarget = TargetOptions[0];
	}
}

void SGridEditorLinksPanel::BuildCommandOptions()
{
	const TSharedPtr<EGridObjectCommand> Previous = SelectedCommand;
	CommandOptions.Reset();
	UGridLevelAsset* LevelAsset = GetLevelAsset();
	if (LevelAsset && SelectedTarget.IsValid())
	{
		const EGridLevelObjectType TargetType = LevelAsset->GetTypedPlacementType(*SelectedTarget);
		for (const EGridObjectCommand Command : GridEditorLinkPolicy::GetSupportedCommandsForTarget(TargetType, GetLogicNodeType(LevelAsset, *SelectedTarget)))
		{
			CommandOptions.Add(MakeShared<EGridObjectCommand>(Command));
		}
	}
	SelectedCommand = FindEnumOption(CommandOptions, Previous);
	if (!SelectedCommand.IsValid() && !CommandOptions.IsEmpty())
	{
		SelectedCommand = CommandOptions[0];
	}
}

void SGridEditorLinksPanel::BuildScriptOptions()
{
	const FName Previous = SelectedScript.IsValid() ? *SelectedScript : NAME_None;
	ScriptOptions.Reset();
	if (UGridLevelAsset* LevelAsset = GetLevelAsset())
	{
		for (const FGridLuaScriptSource& Script : LevelAsset->LuaScripts)
		{
			if (Script.bEnabled && !Script.ScriptId.IsNone())
			{
				ScriptOptions.Add(MakeShared<FName>(Script.ScriptId));
			}
		}
	}
	SelectedScript = FindNameOption(ScriptOptions, Previous);
	if (!SelectedScript.IsValid() && !ScriptOptions.IsEmpty())
	{
		SelectedScript = ScriptOptions[0];
	}
}

void SGridEditorLinksPanel::BuildCallbackOptions()
{
	const FName Previous = SelectedCallback.IsValid() ? *SelectedCallback : NAME_None;
	CallbackOptions.Reset();
	UGridLevelAsset* LevelAsset = GetLevelAsset();
	if (LevelAsset && SelectedScript.IsValid())
	{
		TArray<FName> Names;
		FString Error;
		if (GridEditorLuaService::GetCallbacksForScript(*LevelAsset, *SelectedScript, Names, Error))
		{
			for (const FName Name : Names)
			{
				CallbackOptions.Add(MakeShared<FName>(Name));
			}
		}
	}
	SelectedCallback = FindNameOption(CallbackOptions, Previous);
	if (!SelectedCallback.IsValid() && !CallbackOptions.IsEmpty())
	{
		SelectedCallback = CallbackOptions[0];
	}
}

TSharedRef<SWidget> SGridEditorLinksPanel::BuildRoot()
{
	AGridLevelEditorActor* Actor = GetEditorActor();
	UGridLevelAsset* LevelAsset = Actor ? Actor->LevelAsset.Get() : nullptr;
	const FGuid SourceId = Actor ? Actor->LastSelectedObjectId : FGuid();

	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
				.Text(FText::FromString(TEXT("EVENTS & ACTIONS")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
		];

	if (!LevelAsset || !LevelAsset->ContainsTypedPlacementId(SourceId))
	{
		Root->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("Select a Grid object to edit its events and actions."))).AutoWrapText(true)
		];
		return Root;
	}

	Root->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[BuildActionCreationSection()];
	Root->AddSlot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)[BuildActionsListSection()];
	return Root;
}

TSharedRef<SWidget> SGridEditorLinksPanel::BuildActionCreationSection()
{
	if (EventOptions.IsEmpty())
	{
		return SNew(SBorder)
			.Padding(6.f)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(STextBlock)
					.Text(FText::FromString(TEXT("This object does not emit authorable events.")))
					.AutoWrapText(true)
			];
	}

	return SNew(SBorder)
		.Padding(6.f)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
					.Text(FText::FromString(TEXT("ADD ACTION")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(0.f, 0.f, 4.f, 0.f)
				[
					SNew(SComboBox<TSharedPtr<EGridObjectEvent>>)
						.OptionsSource(&EventOptions)
						.OnGenerateWidget(this, &SGridEditorLinksPanel::MakeEventOptionWidget)
						.OnSelectionChanged(this, &SGridEditorLinksPanel::OnEventSelectionChanged)
						[
							SNew(STextBlock).Text(SelectedEvent.IsValid() ? GetEventText(*SelectedEvent) : FText::FromString(TEXT("Event")))
						]
				]
				+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(4.f, 0.f, 0.f, 0.f)
				[
					SNew(SComboBox<TSharedPtr<FName>>)
						.OptionsSource(&ActionTypeOptions)
						.OnGenerateWidget(this, &SGridEditorLinksPanel::MakeNameOptionWidget)
						.OnSelectionChanged(this, &SGridEditorLinksPanel::OnActionTypeSelectionChanged)
						[
							SNew(STextBlock).Text(SelectedActionType.IsValid() ? FText::FromName(*SelectedActionType) : FText::FromString(TEXT("Action")))
						]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)
			[
				SNew(SBox)
					.Visibility_Lambda([this]() { return IsLuaActionSelected() ? EVisibility::Collapsed : EVisibility::Visible; })
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(0.58f).Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(SComboBox<TSharedPtr<FGuid>>)
								.OptionsSource(&TargetOptions)
								.OnGenerateWidget(this, &SGridEditorLinksPanel::MakeObjectOptionWidget)
								.OnSelectionChanged(this, &SGridEditorLinksPanel::OnTargetSelectionChanged)
								[
									SNew(STextBlock).Text(FText::FromString(SelectedTarget.IsValid() ? GetObjectSummary(*SelectedTarget) : TEXT("Target object")))
								]
						]
						+ SHorizontalBox::Slot().FillWidth(0.42f).Padding(4.f, 0.f, 0.f, 0.f)
						[
							SNew(SComboBox<TSharedPtr<EGridObjectCommand>>)
								.OptionsSource(&CommandOptions)
								.OnGenerateWidget(this, &SGridEditorLinksPanel::MakeCommandOptionWidget)
								.OnSelectionChanged(this, &SGridEditorLinksPanel::OnCommandSelectionChanged)
								[
									SNew(STextBlock).Text(SelectedCommand.IsValid() ? GetCommandText(*SelectedCommand) : FText::FromString(TEXT("Command")))
								]
						]
					]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)
			[
				SNew(SBox)
					.Visibility_Lambda([this]() { return IsLuaActionSelected() ? EVisibility::Visible : EVisibility::Collapsed; })
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(SComboBox<TSharedPtr<FName>>)
								.OptionsSource(&ScriptOptions)
								.OnGenerateWidget(this, &SGridEditorLinksPanel::MakeNameOptionWidget)
								.OnSelectionChanged(this, &SGridEditorLinksPanel::OnScriptSelectionChanged)
								[
									SNew(STextBlock).Text(SelectedScript.IsValid() ? FText::FromName(*SelectedScript) : FText::FromString(TEXT("Script")))
								]
						]
						+ SHorizontalBox::Slot().FillWidth(0.5f).Padding(4.f, 0.f, 0.f, 0.f)
						[
							SNew(SComboBox<TSharedPtr<FName>>)
								.OptionsSource(&CallbackOptions)
								.OnGenerateWidget(this, &SGridEditorLinksPanel::MakeNameOptionWidget)
								.OnSelectionChanged(this, &SGridEditorLinksPanel::OnCallbackSelectionChanged)
								[
									SNew(STextBlock).Text(SelectedCallback.IsValid() ? FText::FromName(*SelectedCallback) : FText::FromString(TEXT("Function")))
								]
						]
					]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(TEXT("Event -> Command actions are unconditional. Use a Lua callback whenever the behavior needs a condition, counter, sequence or calculation.")))
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor(FLinearColor(0.70f, 0.70f, 0.70f, 1.f)))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(SButton)
					.Text(FText::FromString(TEXT("Create Action")))
					.IsEnabled_Lambda([this]() { return CanCreateAction(); })
					.OnClicked(this, &SGridEditorLinksPanel::OnCreateActionClicked)
			]
		];
}

TSharedRef<SWidget> SGridEditorLinksPanel::BuildActionsListSection()
{
	AGridLevelEditorActor* Actor = GetEditorActor();
	UGridLevelAsset* LevelAsset = Actor ? Actor->LevelAsset.Get() : nullptr;
	const FGuid SelectedId = Actor ? Actor->LastSelectedObjectId : FGuid();
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
				.Text(FText::FromString(TEXT("CURRENT ACTIONS")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		];

	if (!LevelAsset)
	{
		return Root;
	}

	int32 Count = 0;
	for (const FGridObjectLink& Link : LevelAsset->Links)
	{
		const bool bOutgoing = Link.SourceObjectId == SelectedId;
		const bool bIncoming = Link.TargetObjectId == SelectedId;
		if (!bOutgoing && !bIncoming)
		{
			continue;
		}
		++Count;
		const bool bBroken = IsActionBroken(Link);
		const FGuid Counterpart = bOutgoing ? Link.TargetObjectId : Link.SourceObjectId;

		Root->AddSlot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)
		[
			SNew(SBorder)
				.Padding(5.f)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(STextBlock)
							.Text(FText::FromString(GetActionSummary(Link, bOutgoing)))
							.AutoWrapText(true)
							.ColorAndOpacity(bBroken ? FSlateColor(FLinearColor(1.f, 0.30f, 0.20f, 1.f)) : FSlateColor::UseForeground())
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(5.f, 0.f, 0.f, 0.f)
					[
						SNew(SButton)
							.Visibility(Counterpart.IsValid() ? EVisibility::Visible : EVisibility::Collapsed)
							.Text(FText::FromString(TEXT("Go To")))
							.OnClicked_Lambda([this, Counterpart]() { return OnGoToObjectClicked(Counterpart); })
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(5.f, 0.f, 0.f, 0.f)
					[
						SNew(SButton)
							.Text(FText::FromString(TEXT("Remove")))
							.OnClicked_Lambda([this, Link]() { return OnRemoveActionClicked(Link); })
					]
				]
		];
	}

	if (Count == 0)
	{
		Root->AddSlot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("No actions reference the selected object.")))
		];
	}
	return Root;
}

TSharedRef<SWidget> SGridEditorLinksPanel::MakeEventOptionWidget(TSharedPtr<EGridObjectEvent> Item) const
{
	return SNew(STextBlock).Text(Item.IsValid() ? GetEventText(*Item) : FText::FromString(TEXT("Invalid")));
}

TSharedRef<SWidget> SGridEditorLinksPanel::MakeCommandOptionWidget(TSharedPtr<EGridObjectCommand> Item) const
{
	return SNew(STextBlock).Text(Item.IsValid() ? GetCommandText(*Item) : FText::FromString(TEXT("Invalid")));
}

TSharedRef<SWidget> SGridEditorLinksPanel::MakeNameOptionWidget(TSharedPtr<FName> Item) const
{
	return SNew(STextBlock).Text(Item.IsValid() ? FText::FromName(*Item) : FText::FromString(TEXT("Invalid")));
}

TSharedRef<SWidget> SGridEditorLinksPanel::MakeObjectOptionWidget(TSharedPtr<FGuid> Item) const
{
	return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? GetObjectSummary(*Item) : TEXT("Invalid")));
}

void SGridEditorLinksPanel::OnEventSelectionChanged(TSharedPtr<EGridObjectEvent> Item, ESelectInfo::Type)
{
	SelectedEvent = Item;
	Rebuild();
}

void SGridEditorLinksPanel::OnActionTypeSelectionChanged(TSharedPtr<FName> Item, ESelectInfo::Type)
{
	SelectedActionType = Item;
	Rebuild();
}

void SGridEditorLinksPanel::OnTargetSelectionChanged(TSharedPtr<FGuid> Item, ESelectInfo::Type)
{
	SelectedTarget = Item;
	BuildCommandOptions();
	Rebuild();
}

void SGridEditorLinksPanel::OnCommandSelectionChanged(TSharedPtr<EGridObjectCommand> Item, ESelectInfo::Type)
{
	SelectedCommand = Item;
	Rebuild();
}

void SGridEditorLinksPanel::OnScriptSelectionChanged(TSharedPtr<FName> Item, ESelectInfo::Type)
{
	SelectedScript = Item;
	SelectedCallback.Reset();
	BuildCallbackOptions();
	Rebuild();
}

void SGridEditorLinksPanel::OnCallbackSelectionChanged(TSharedPtr<FName> Item, ESelectInfo::Type)
{
	SelectedCallback = Item;
	Rebuild();
}

FReply SGridEditorLinksPanel::OnCreateActionClicked()
{
	AGridLevelEditorActor* Actor = GetEditorActor();
	UGridLevelAsset* LevelAsset = Actor ? Actor->LevelAsset.Get() : nullptr;
	if (!Actor || !LevelAsset || !SelectedEvent.IsValid())
	{
		return FReply::Handled();
	}

	FGridObjectLink Link;
	Link.SourceObjectId = Actor->LastSelectedObjectId;
	Link.SourceEvent = *SelectedEvent;
	Link.Condition = EGridObjectCondition::None;

	bool bCreated = false;
	if (IsLuaActionSelected())
	{
		if (!SelectedScript.IsValid() || !SelectedCallback.IsValid())
		{
			return FReply::Handled();
		}
		Link.Command = EGridObjectCommand::LuaCallback;
		Link.TargetObjectId.Invalidate();
		Link.LuaScriptId = *SelectedScript;
		Link.LuaCallbackName = *SelectedCallback;
		FString Error;
		bCreated = GridEditorLuaService::AddLuaLink(*Actor, Link, Error);
	}
	else
	{
		if (!SelectedTarget.IsValid() || !SelectedCommand.IsValid())
		{
			return FReply::Handled();
		}
		Link.TargetObjectId = *SelectedTarget;
		Link.Command = *SelectedCommand;
		bCreated = GridEditorLinkService::CreateLink(*Actor, Link);
	}

	if (bCreated)
	{
		RequestRefresh();
	}
	else
	{
		Rebuild();
	}
	return FReply::Handled();
}

FReply SGridEditorLinksPanel::OnRemoveActionClicked(FGridObjectLink Link)
{
	AGridLevelEditorActor* Actor = GetEditorActor();
	if (!Actor)
	{
		return FReply::Handled();
	}
	const bool bRemoved = Link.Command == EGridObjectCommand::LuaCallback
		? GridEditorLuaService::RemoveLuaLink(*Actor, Link)
		: GridEditorLinkService::RemoveExactLink(*Actor, Link);
	if (bRemoved)
	{
		RequestRefresh();
	}
	else
	{
		Rebuild();
	}
	return FReply::Handled();
}

FReply SGridEditorLinksPanel::OnGoToObjectClicked(FGuid ObjectId)
{
	if (AGridLevelEditorActor* Actor = GetEditorActor())
	{
		if (Actor->SelectObjectById(ObjectId))
		{
			RequestRefresh();
		}
	}
	return FReply::Handled();
}

bool SGridEditorLinksPanel::IsLuaActionSelected() const
{
	return SelectedActionType.IsValid() && *SelectedActionType == LuaActionType;
}

bool SGridEditorLinksPanel::CanCreateAction() const
{
	AGridLevelEditorActor* Actor = GetEditorActor();
	UGridLevelAsset* LevelAsset = Actor ? Actor->LevelAsset.Get() : nullptr;
	if (!Actor || !LevelAsset || !LevelAsset->ContainsTypedPlacementId(Actor->LastSelectedObjectId) || !SelectedEvent.IsValid())
	{
		return false;
	}

	FGridObjectLink Candidate;
	Candidate.SourceObjectId = Actor->LastSelectedObjectId;
	Candidate.SourceEvent = *SelectedEvent;
	Candidate.Condition = EGridObjectCondition::None;

	if (IsLuaActionSelected())
	{
		if (!SelectedScript.IsValid() || !SelectedCallback.IsValid())
		{
			return false;
		}
		Candidate.Command = EGridObjectCommand::LuaCallback;
		Candidate.TargetObjectId.Invalidate();
		Candidate.LuaScriptId = *SelectedScript;
		Candidate.LuaCallbackName = *SelectedCallback;
		FString Error;
		return GridEditorLuaService::IsLuaLinkSupported(*LevelAsset, Candidate, Error) && !GridEditorLinkService::ContainsExactLink(LevelAsset->Links, Candidate);
	}

	if (!SelectedTarget.IsValid() || !SelectedCommand.IsValid())
	{
		return false;
	}
	Candidate.TargetObjectId = *SelectedTarget;
	Candidate.Command = *SelectedCommand;
	const FGridObjectLink Normalized = GridEditorLinkService::NormalizeLink(Candidate);
	return GridEditorLinkService::IsLinkSupported(*LevelAsset, Normalized) && !GridEditorLinkService::ContainsExactLink(LevelAsset->Links, Normalized);
}

bool SGridEditorLinksPanel::IsActionBroken(const FGridObjectLink& Link) const
{
	UGridLevelAsset* LevelAsset = GetLevelAsset();
	if (!LevelAsset)
	{
		return true;
	}
	if (Link.Command == EGridObjectCommand::LuaCallback)
	{
		FString Error;
		return !GridEditorLuaService::IsLuaLinkSupported(*LevelAsset, Link, Error);
	}
	return !GridEditorLinkService::IsLinkSupported(*LevelAsset, Link);
}

FString SGridEditorLinksPanel::GetObjectSummary(FGuid ObjectId) const
{
	UGridLevelAsset* LevelAsset = GetLevelAsset();
	if (!LevelAsset || !LevelAsset->ContainsTypedPlacementId(ObjectId))
	{
		return TEXT("Missing object");
	}

	const FName LogicId = LevelAsset->GetTypedPlacementLogicId(ObjectId);
	const EGridLevelObjectType Type = LevelAsset->GetTypedPlacementType(ObjectId);
	const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType>();
	const FString TypeText = TypeEnum ? TypeEnum->GetDisplayNameTextByValue(static_cast<int64>(Type)).ToString() : TEXT("Object");
	int32 X = INDEX_NONE;
	int32 Y = INDEX_NONE;
	EGridEdge Edge = EGridEdge::None;
	LevelAsset->TryGetTypedPlacementLocation(ObjectId, X, Y, Edge);

	if (!LogicId.IsNone())
	{
		return FString::Printf(TEXT("%s — %s @ (%d,%d)"), *LogicId.ToString(), *TypeText, X, Y);
	}
	return FString::Printf(TEXT("%s @ (%d,%d) [%s]"), *TypeText, X, Y, *ObjectId.ToString().Left(8));
}

FString SGridEditorLinksPanel::GetActionSummary(const FGridObjectLink& Link, bool bOutgoing) const
{
	FString Result;
	if (bOutgoing)
	{
		if (Link.Command == EGridObjectCommand::LuaCallback)
		{
			Result = FString::Printf(TEXT("%s -> Run Lua %s.%s()"), *GetEventText(Link.SourceEvent).ToString(), *Link.LuaScriptId.ToString(), *Link.LuaCallbackName.ToString());
		}
		else
		{
			Result = FString::Printf(TEXT("%s -> %s : %s"), *GetEventText(Link.SourceEvent).ToString(), *GetObjectSummary(Link.TargetObjectId), *GetCommandText(Link.Command).ToString());
		}
	}
	else
	{
		Result = FString::Printf(TEXT("From %s / %s -> %s"), *GetObjectSummary(Link.SourceObjectId), *GetEventText(Link.SourceEvent).ToString(), *GetCommandText(Link.Command).ToString());
	}

	if (Link.Condition != EGridObjectCondition::None)
	{
		Result += TEXT("  [Legacy condition — move this logic to Lua]");
	}
	return Result;
}

FText SGridEditorLinksPanel::GetEventText(EGridObjectEvent Event) const
{
	const UEnum* Enum = StaticEnum<EGridObjectEvent>();
	return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Event)) : FText::FromString(TEXT("Unknown Event"));
}

FText SGridEditorLinksPanel::GetCommandText(EGridObjectCommand Command) const
{
	const UEnum* Enum = StaticEnum<EGridObjectCommand>();
	return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Command)) : FText::FromString(TEXT("Unknown Command"));
}

#endif

#include "EditorTools/Widgets/SGridEditorLinksPanel.h"

#if WITH_EDITOR

#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"
#include "EditorTools/GridEditorLinkPolicy.h"
#include "EditorTools/GridEditorLinkService.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridLevelPlacementTypes.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Core/GridLevelVariableTypes.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Core/GridTypes.h"

#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"

namespace
{
	const FLinearColor OutgoingConnectorColor(0.25f, 0.75f, 1.f, 1.f);
	const FLinearColor IncomingConnectorColor(0.70f, 0.55f, 1.f, 1.f);
	const FLinearColor BrokenConnectorColor(1.f, 0.18f, 0.16f, 1.f);

	EGridLogicNodeType GetLogicNodeTypeForPlacement(const UGridLevelAsset* LevelAsset, const FGuid& ObjectId)
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

	bool HasTypedPlacement(const UGridLevelAsset* LevelAsset, const FGuid& ObjectId)
	{
		return LevelAsset && ObjectId.IsValid() && LevelAsset->ContainsTypedPlacementId(ObjectId);
	}

	bool IsConnectorBroken(const FGridObjectLink& Link)
	{
		if (!Link.SourceObjectId.IsValid())
		{
			return true;
		}

		if (Link.Command == EGridObjectCommand::LuaCallback)
		{
			return Link.LuaScriptId.IsNone() || Link.LuaCallbackName.IsNone();
		}

		return !Link.TargetObjectId.IsValid();
	}

	bool IsConnectorBroken(const FGridObjectLink& Link, const UGridLevelAsset* LevelAsset)
	{
		if (IsConnectorBroken(Link) || !HasTypedPlacement(LevelAsset, Link.SourceObjectId))
		{
			return true;
		}

		if (Link.Command == EGridObjectCommand::LuaCallback)
		{
			// Lua callbacks are intentionally targetless. Script existence and
			// compilability are reported by the MON19.6 Lua validation service.
			return false;
		}

		return !HasTypedPlacement(LevelAsset, Link.TargetObjectId);
	}}

void SGridEditorLinksPanel::Construct(const FArguments& InArgs)
{
	EditorActor = InArgs._EditorActor;
	OnGetEditorActor = InArgs._OnGetEditorActor;
	OnRequestRefresh = InArgs._OnRequestRefresh;
	BuildLinkOptions();
	RefreshConnectorFormOptions();

	RebuildLinksSection();
}

AGridLevelEditorActor* SGridEditorLinksPanel::GetEditorActor() const
{
	if (EditorActor.IsValid())
	{
		return EditorActor.Get();
	}

	return OnGetEditorActor.IsBound() ? OnGetEditorActor.Execute() : nullptr;
}

void SGridEditorLinksPanel::RequestRefresh() const
{
	if (OnRequestRefresh.IsBound())
	{
		OnRequestRefresh.Execute();
	}
}

void SGridEditorLinksPanel::RebuildLinksSection()
{
	RefreshConnectorFormOptions();

	ChildSlot[BuildLinksSection()];
}

TSharedRef<SWidget> SGridEditorLinksPanel::BuildLinksSection()
{
	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();

	if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset)
	{
		return SNew(STextBlock).Text(FText::FromString(TEXT("No editor actor or level asset.")));
	}

	const UGridLevelAsset* LevelAsset = CurrentEditorActor->LevelAsset;
	const FGuid SelectedObjectId = CurrentEditorActor->LastSelectedObjectId;
	const EGridLevelObjectType SelectedObjectType = LevelAsset->GetTypedPlacementType(SelectedObjectId);
	if (SelectedObjectType == EGridLevelObjectType::None)
	{
		return SNew(STextBlock).Text(FText::FromString(TEXT("No selected object.")));
	}

	const EGridLogicNodeType SelectedLogicNodeType = GetLogicNodeTypeForPlacement(LevelAsset, SelectedObjectId);
	const bool bSelectedObjectSupportsConnectors =
		GridEditorLinkPolicy::CanObjectEmitEvents(SelectedObjectType, SelectedLogicNodeType) ||
		GridEditorLinkPolicy::CanObjectReceiveCommands(SelectedObjectType, SelectedLogicNodeType);

	if (!bSelectedObjectSupportsConnectors)
	{
		bAddConnectorVisible = false;
		return SNew(SVerticalBox)

			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[BuildConnectorsHeader(false)]

			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock)
				  .Text(FText::FromString(TEXT("This object has no connector behavior.")))
				  .AutoWrapText(true)
				  .ColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.72f, 0.72f, 1.f)))];
	}

	return SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[BuildConnectorsHeader(true)]

		+ SVerticalBox::Slot().AutoHeight().Padding(
			  0.f, 0.f, 0.f, bAddConnectorVisible ? 8.f : 0.f)[bAddConnectorVisible ? BuildLinkCreationSection() : SNullWidget::NullWidget]

		+ SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)

			  + SHorizontalBox::Slot().FillWidth(0.5f).Padding(
					0.f, 0.f, 4.f, 0.f)[SNew(SBorder).Padding(6.f).BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))[SNew(SVerticalBox)

					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)[SNew(STextBlock)
							  .Text(FText::FromString(TEXT("OUTGOING CONNECTORS")))
							  .ColorAndOpacity(FSlateColor(OutgoingConnectorColor))
							  .Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))]

					+ SVerticalBox::Slot().AutoHeight()[BuildObjectLinksList(SelectedObjectId, true)]]]

			  + SHorizontalBox::Slot().FillWidth(0.5f).Padding(
					4.f, 0.f, 0.f, 0.f)[SNew(SBorder).Padding(6.f).BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))[SNew(SVerticalBox)

					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)[SNew(STextBlock)
							  .Text(FText::FromString(TEXT("INCOMING CONNECTORS")))
							  .ColorAndOpacity(FSlateColor(IncomingConnectorColor))
							  .Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))]

					+ SVerticalBox::Slot().AutoHeight()[BuildObjectLinksList(SelectedObjectId, false)]]]];
}
TSharedRef<SWidget> SGridEditorLinksPanel::BuildConnectorsHeader(bool bAllowAddConnector)
{
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[BuildConnectorLegend()]

		+ SHorizontalBox::Slot().AutoWidth().VAlign(
			  VAlign_Center)[bAllowAddConnector ? GridEditorWidgetHelpers::BuildGridActionButton(FText::FromString(TEXT("+")),
													  FOnClicked::CreateSP(this, &SGridEditorLinksPanel::OnToggleAddConnectorClicked))
												: SNullWidget::NullWidget];
}

TSharedRef<SWidget> SGridEditorLinksPanel::BuildConnectorLegend()
{
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot().AutoWidth().Padding(
			  0.f, 0.f, 8.f, 0.f)[BuildConnectorLegendItem(FText::FromString(TEXT("Cyan = Outgoing")), FSlateColor(OutgoingConnectorColor))]

		+ SHorizontalBox::Slot().AutoWidth().Padding(
			  0.f, 0.f, 8.f, 0.f)[BuildConnectorLegendItem(FText::FromString(TEXT("Purple = Incoming")), FSlateColor(IncomingConnectorColor))]

		+ SHorizontalBox::Slot().AutoWidth()[BuildConnectorLegendItem(FText::FromString(TEXT("Red = Broken")), FSlateColor(BrokenConnectorColor))];
}

TSharedRef<SWidget> SGridEditorLinksPanel::BuildConnectorLegendItem(const FText& Label, const FSlateColor& Color) const
{
	return SNew(STextBlock).Text(Label).ColorAndOpacity(Color).Font(FCoreStyle::GetDefaultFontStyle("Regular", 8));
}

TSharedRef<SWidget> SGridEditorLinksPanel::BuildLinkCreationSection()
{
	return SNew (SBorder)
        .Padding (6.f)
        .BorderImage (FAppStyle::GetBrush ("ToolPanel.DarkGroupBorder"))
        [
            SNew (SVerticalBox)

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Source Object")),
                        BuildObjectCombo (
                            FText::FromString (TEXT ("Select source")),
                            true))
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Event")),
                        SNew (SComboBox<TSharedPtr<EGridObjectEvent>>)
                            .OptionsSource (&LinkSourceEventOptions)
                            .OnGenerateWidget (this, &SGridEditorLinksPanel::MakeLinkSourceEventComboWidget)
                            .OnSelectionChanged (this, &SGridEditorLinksPanel::OnLinkSourceEventSelectionChanged)
                            [
                                SNew (STextBlock)
                                    .Text_Lambda ([this] ()
                                {
                                    return GetSelectedLinkSourceEventText ();
                                })
                            ])
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Target Object")),
                        BuildObjectCombo (
                            FText::FromString (TEXT ("Select target")),
                            false))
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Command")),
                        SNew (SComboBox<TSharedPtr<EGridObjectCommand>>)
                            .OptionsSource (&LinkCommandOptions)
                            .OnGenerateWidget (this, &SGridEditorLinksPanel::MakeLinkCommandComboWidget)
                            .OnSelectionChanged (this, &SGridEditorLinksPanel::OnLinkCommandSelectionChanged)
                            [
                                SNew (STextBlock)
                                    .Text_Lambda ([this] ()
                                {
                                    return GetSelectedLinkCommandText ();
                                })
                            ])
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    BuildConditionCreationSection ()
                ]

                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 0.f)
                [
                    SNew (SHorizontalBox)

                    + SHorizontalBox::Slot ().AutoWidth ().Padding (0.f, 0.f, 4.f, 0.f)
                    [
                        SNew (SButton)
                            .Text (FText::FromString (TEXT ("Create")))
                            .HAlign (HAlign_Center)
                            .ContentPadding (FMargin (8.f, 3.f))
                            .IsEnabled (this, &SGridEditorLinksPanel::CanCreateConnector)
                            .OnClicked (this, &SGridEditorLinksPanel::OnCreateConnectorClicked)
                    ]

                    + SHorizontalBox::Slot ().AutoWidth ()
                    [
                        GridEditorWidgetHelpers::BuildGridActionButton (
                            FText::FromString (TEXT ("Cancel")),
                            FOnClicked::CreateSP (this, &SGridEditorLinksPanel::OnCancelAddConnectorClicked))
                    ]
                ]
        ];
}

TSharedRef<SWidget> SGridEditorLinksPanel::BuildConditionCreationSection()
{
	const auto VisibilityFor = [this](EGridObjectCondition Condition)
	{
		return IsConditionSelected(Condition) ? EVisibility::Visible : EVisibility::Collapsed;
	};

	const auto VariableVisibility = [this]()
	{
		return IsConditionSelected(EGridObjectCondition::LevelVariableBoolEquals) || IsConditionSelected(EGridObjectCondition::LevelVariableIntCompare)
			? EVisibility::Visible
			: EVisibility::Collapsed;
	};

	const auto InvertVisibility = [this]()
	{
		return SelectedCondition.IsValid() && *SelectedCondition != EGridObjectCondition::None ? EVisibility::Visible : EVisibility::Collapsed;
	};

	return SNew (SVerticalBox)

        + SVerticalBox::Slot ().AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridPropertyRow (
                FText::FromString (TEXT ("Condition")),
                SNew (SComboBox<TSharedPtr<EGridObjectCondition>>)
                    .OptionsSource (&LinkConditionOptions)
                    .OnGenerateWidget (this, &SGridEditorLinksPanel::MakeLinkConditionComboWidget)
                    .OnSelectionChanged (this, &SGridEditorLinksPanel::OnLinkConditionSelectionChanged)
                    [
                        SNew (STextBlock)
                            .Text_Lambda ([this] ()
                        {
                            return GetSelectedLinkConditionText ();
                        })
                    ])
        ]

        + SVerticalBox::Slot ().AutoHeight ()
        [
            SNew (SBox)
                .Visibility_Lambda (VariableVisibility)
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Variable")),
                        SNew (SComboBox<TSharedPtr<FName>>)
                            .OptionsSource (&VariableOptions)
                            .OnGenerateWidget (this, &SGridEditorLinksPanel::MakeVariableComboWidget)
                            .OnSelectionChanged (this, &SGridEditorLinksPanel::OnVariableSelectionChanged)
                            [
                                SNew (STextBlock)
                                    .Text_Lambda ([this] ()
                                {
                                    return GetSelectedVariableText ();
                                })
                            ])
                ]
        ]

        + SVerticalBox::Slot ().AutoHeight ()
        [
            SNew (SBox)
                .Visibility_Lambda ([VisibilityFor] ()
                {
                    return VisibilityFor (EGridObjectCondition::LevelVariableBoolEquals);
                })
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Expected Value")),
                        SNew (SHorizontalBox)

                        + SHorizontalBox::Slot ().AutoWidth ().VAlign (VAlign_Center)
                        [
                            SNew (SCheckBox)
                                .IsChecked_Lambda ([this] ()
                                {
                                    return ConditionBoolValue
                                        ? ECheckBoxState::Checked
                                        : ECheckBoxState::Unchecked;
                                })
                                .OnCheckStateChanged_Lambda ([this] (ECheckBoxState NewState)
                                {
                                    ConditionBoolValue = NewState == ECheckBoxState::Checked;
                                })
                        ]

                        + SHorizontalBox::Slot ().AutoWidth ().VAlign (VAlign_Center).Padding (5.f, 0.f, 0.f, 0.f)
                        [
                            SNew (STextBlock)
                                .Text_Lambda ([this] ()
                                {
                                    return FText::FromString (ConditionBoolValue ? TEXT ("true") : TEXT ("false"));
                                })
                        ])
                ]
        ]

        + SVerticalBox::Slot ().AutoHeight ()
        [
            SNew (SBox)
                .Visibility_Lambda ([VisibilityFor] ()
                {
                    return VisibilityFor (EGridObjectCondition::LevelVariableIntCompare);
                })
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Comparison")),
                        SNew (SComboBox<TSharedPtr<EGridLogicIntComparison>>)
                            .OptionsSource (&IntComparisonOptions)
                            .OnGenerateWidget (this, &SGridEditorLinksPanel::MakeIntComparisonComboWidget)
                            .OnSelectionChanged (this, &SGridEditorLinksPanel::OnIntComparisonSelectionChanged)
                            [
                                SNew (STextBlock)
                                    .Text_Lambda ([this] ()
                                {
                                    return GetSelectedIntComparisonText ();
                                })
                            ])
                ]
        ]

        + SVerticalBox::Slot ().AutoHeight ()
        [
            SNew (SBox)
                .Visibility_Lambda ([VisibilityFor] ()
                {
                    return VisibilityFor (EGridObjectCondition::LevelVariableIntCompare);
                })
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Compare Value")),
                        SNew (SNumericEntryBox<int32>)
                            .Value_Lambda ([this] () -> TOptional<int32>
                            {
                                return ConditionIntValue;
                            })
                            .OnValueChanged_Lambda ([this] (int32 NewValue)
                            {
                                ConditionIntValue = NewValue;
                            }))
                ]
        ]

        + SVerticalBox::Slot ().AutoHeight ()
        [
            SNew (SBox)
                .Visibility_Lambda ([VisibilityFor] ()
                {
                    return VisibilityFor (EGridObjectCondition::ReceptacleContainsItemDefinition);
                })
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Item Definition Id")),
                        SNew (SEditableTextBox)
                            .Text_Lambda ([this] ()
                            {
                                return ConditionItemDefinitionId.IsNone ()
                                    ? FText::GetEmpty ()
                                    : FText::FromName (ConditionItemDefinitionId);
                            })
                            .OnTextChanged_Lambda ([this] (const FText& NewText)
                            {
                                const FString Value = NewText.ToString ().TrimStartAndEnd ();
                                ConditionItemDefinitionId = Value.IsEmpty () ? NAME_None : FName (*Value);
                            }))
                ]
        ]

        + SVerticalBox::Slot ().AutoHeight ()
        [
            SNew (SBox)
                .Visibility_Lambda ([VisibilityFor] ()
                {
                    return VisibilityFor (EGridObjectCondition::ReceptacleContainsItemTag);
                })
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Item Tag")),
                        SNew (SEditableTextBox)
                            .Text_Lambda ([this] ()
                            {
                                return ConditionItemTag.IsNone ()
                                    ? FText::GetEmpty ()
                                    : FText::FromName (ConditionItemTag);
                            })
                            .OnTextChanged_Lambda ([this] (const FText& NewText)
                            {
                                const FString Value = NewText.ToString ().TrimStartAndEnd ();
                                ConditionItemTag = Value.IsEmpty () ? NAME_None : FName (*Value);
                            }))
                ]
        ]

        + SVerticalBox::Slot ().AutoHeight ()
        [
            SNew (SBox)
                .Visibility_Lambda ([VisibilityFor] ()
                {
                    return VisibilityFor (EGridObjectCondition::ReceptacleContainsItemType);
                })
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Item Type")),
                        SNew (SComboBox<TSharedPtr<EGridItemType>>)
                            .OptionsSource (&ItemTypeOptions)
                            .OnGenerateWidget (this, &SGridEditorLinksPanel::MakeItemTypeComboWidget)
                            .OnSelectionChanged (this, &SGridEditorLinksPanel::OnItemTypeSelectionChanged)
                            [
                                SNew (STextBlock)
                                    .Text_Lambda ([this] ()
                                {
                                    return GetSelectedItemTypeText ();
                                })
                            ])
                ]
        ]

        + SVerticalBox::Slot ().AutoHeight ()
        [
            SNew (SBox)
                .Visibility_Lambda ([VisibilityFor] ()
                {
                    return VisibilityFor (EGridObjectCondition::ReceptacleItemCountAtLeast);
                })
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Minimum Count")),
                        SNew (SNumericEntryBox<int32>)
                            .MinValue (1)
                            .MinSliderValue (1)
                            .Value_Lambda ([this] () -> TOptional<int32>
                            {
                                return ConditionCount;
                            })
                            .OnValueChanged_Lambda ([this] (int32 NewValue)
                            {
                                ConditionCount = FMath::Max (1, NewValue);
                            }))
                ]
        ]

        + SVerticalBox::Slot ().AutoHeight ()
        [
            SNew (SBox)
                .Visibility_Lambda ([VisibilityFor] ()
                {
                    return VisibilityFor (EGridObjectCondition::ReceptacleWeightAtLeast);
                })
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Minimum Weight")),
                        SNew (SNumericEntryBox<float>)
                            .MinValue (0.0f)
                            .MinSliderValue (0.0f)
                            .Value_Lambda ([this] () -> TOptional<float>
                            {
                                return ConditionWeight;
                            })
                            .OnValueChanged_Lambda ([this] (float NewValue)
                            {
                                ConditionWeight = FMath::Max (0.0f, NewValue);
                            }))
                ]
        ]

        + SVerticalBox::Slot ().AutoHeight ()
        [
            SNew (SBox)
                .Visibility_Lambda (InvertVisibility)
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Invert")),
                        SNew (SCheckBox)
                            .IsChecked_Lambda ([this] ()
                            {
                                return bInvertCondition
                                    ? ECheckBoxState::Checked
                                    : ECheckBoxState::Unchecked;
                            })
                            .OnCheckStateChanged_Lambda ([this] (ECheckBoxState NewState)
                            {
                                bInvertCondition = NewState == ECheckBoxState::Checked;
                            }))
                ]
        ];
}

TSharedRef<SWidget> SGridEditorLinksPanel::BuildObjectCombo(const FText& EmptyText, bool bSourceObject)
{
	if (bSourceObject)
	{
		return SNew(SComboBox<TSharedPtr<FGuid>>)
			.OptionsSource(&SourceObjectOptions)
			.OnGenerateWidget(this, &SGridEditorLinksPanel::MakeObjectComboWidget)
			.OnSelectionChanged(this, &SGridEditorLinksPanel::OnSourceObjectSelectionChanged)[SNew(STextBlock)
					.Text_Lambda(
						[this, EmptyText]()
						{
							return GetSelectedObjectOptionText(SelectedSourceObjectId, EmptyText);
						})];
	}

	return SNew(SComboBox<TSharedPtr<FGuid>>)
		.OptionsSource(&TargetObjectOptions)
		.OnGenerateWidget(this, &SGridEditorLinksPanel::MakeObjectComboWidget)
		.OnSelectionChanged(this, &SGridEditorLinksPanel::OnTargetObjectSelectionChanged)[SNew(STextBlock)
				.Text_Lambda(
					[this, EmptyText]()
					{
						return GetSelectedObjectOptionText(SelectedTargetObjectId, EmptyText);
					})];
}

TSharedRef<SWidget> SGridEditorLinksPanel::BuildObjectLinksList(const FGuid& SelectedObjectId, bool bOutgoing)
{
	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();

	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);

	if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset)
	{
		Root->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("No level asset.")))];
		return Root;
	}

	int32 Count = 0;

	const auto AddLinkRow = [this, CurrentEditorActor, &Root, &Count, bOutgoing](const FGridObjectLink& Link)
	{
		const FGuid SourceId = Link.SourceObjectId;
		const FGuid TargetId = Link.TargetObjectId;
		const bool bLuaCallback = Link.Command == EGridObjectCommand::LuaCallback;
		const FGuid OtherId = (bOutgoing && bLuaCallback) ? FGuid() : (bOutgoing ? TargetId : SourceId);
		const bool bBroken = IsConnectorBroken(Link, CurrentEditorActor->LevelAsset);
		const FText FlowText = bOutgoing && bLuaCallback
			? FText::Format(FText::FromString(TEXT("-> Lua {0}.{1}")), FText::FromName(Link.LuaScriptId), FText::FromName(Link.LuaCallbackName))
			: bOutgoing
			? FText::Format(FText::FromString(TEXT("-> {0} : {1}")),
				  HasTypedPlacement(CurrentEditorActor->LevelAsset, TargetId) ? GetObjectSummaryText(TargetId) : FText::FromString(TEXT("Missing object")),
				  GetLinkCommandText(Link.Command))
			: FText::Format(FText::FromString(TEXT("{0} : {1} -> {2}")),
				  HasTypedPlacement(CurrentEditorActor->LevelAsset, SourceId) ? GetObjectSummaryText(SourceId) : FText::FromString(TEXT("Missing object")),
				  GetLinkSourceEventText(Link.SourceEvent), GetLinkCommandText(Link.Command));

		Root->AddSlot ()
            .AutoHeight ()
            .Padding (bOutgoing ? FMargin (12.f, 2.f, 0.f, 4.f) : FMargin (0.f, 2.f, 0.f, 4.f))
            [
                SNew (SBorder)
                    .Padding (5.f)
                    .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
                    [
                        SNew (SVerticalBox)

                        + SVerticalBox::Slot ()
                        .AutoHeight ()
                        .Padding (0.f, 0.f, 0.f, 4.f)
                        [
                            SNew (STextBlock)
                                .Text (FlowText)
                                .ColorAndOpacity (bBroken
                                    ? FSlateColor (BrokenConnectorColor)
                                    : FSlateColor (bOutgoing ? OutgoingConnectorColor : IncomingConnectorColor))
                                .AutoWrapText (true)
                        ]

                        + SVerticalBox::Slot ()
                        .AutoHeight ()
                        .Padding (0.f, 0.f, 0.f, Link.Condition == EGridObjectCondition::None ? 0.f : 4.f)
                        [
                            SNew (STextBlock)
                                .Visibility (Link.Condition == EGridObjectCondition::None
                                    ? EVisibility::Collapsed
                                    : EVisibility::Visible)
                                .Text (GetLinkConditionSummaryText (Link))
                                .ColorAndOpacity (FSlateColor (FLinearColor (0.78f, 0.78f, 0.58f, 1.f)))
                                .AutoWrapText (true)
                        ]

                        + SVerticalBox::Slot ()
                        .AutoHeight ()
                        [
                            SNew (SHorizontalBox)

                            + SHorizontalBox::Slot ()
                            .AutoWidth ()
                            .Padding (0.f, 0.f, 4.f, 0.f)
                            [
                                (bOutgoing && bLuaCallback)
                                    ? SNullWidget::NullWidget
                                    : GridEditorWidgetHelpers::BuildGridActionButton (
                                        bOutgoing
                                            ? FText::FromString (TEXT ("Go To Target"))
                                            : FText::FromString (TEXT ("Go To Source")),
                                        FOnClicked::CreateLambda ([this, OtherId] () -> FReply
                                        {
                                            return OnSelectObjectFromLinkClicked (OtherId);
                                        }))
                            ]

                            + SHorizontalBox::Slot ()
                            .AutoWidth ()
                            .Padding (0.f, 0.f, 4.f, 0.f)
                            [
                                GridEditorWidgetHelpers::BuildGridActionButton (
                                    FText::FromString (TEXT ("Remove")),
                                    FOnClicked::CreateLambda ([this, Link] () -> FReply
                                    {
                                        return OnRemoveExactLinkClicked (Link);
                                    }))
                            ]
                        ]
                    ]
            ];

		++Count;
	};

	if (bOutgoing)
	{
		for (const EGridObjectEvent Event : GridEditorLinkPolicy::GetEventDisplayOrder())
		{
			bool bEventHeaderAdded = false;
			for (const FGridObjectLink& Link : CurrentEditorActor->LevelAsset->Links)
			{
				if (Link.SourceObjectId != SelectedObjectId || Link.SourceEvent != Event)
				{
					continue;
				}

				if (!bEventHeaderAdded)
				{
					Root->AddSlot().AutoHeight().Padding(
						0.f, 5.f, 0.f, 1.f)[SNew(STextBlock).Text(GetLinkSourceEventText(Event)).Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))];
					bEventHeaderAdded = true;
				}

				AddLinkRow(Link);
			}
		}
	}
	else
	{
		for (const FGridObjectLink& Link : CurrentEditorActor->LevelAsset->Links)
		{
			if (Link.TargetObjectId == SelectedObjectId)
			{
				AddLinkRow(Link);
			}
		}
	}

	if (Count == 0)
	{
		Root->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("None")))];
	}

	return Root;
}
FReply SGridEditorLinksPanel::OnRemoveExactLinkClicked(FGridObjectLink Link)
{
	if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor())
	{
		CurrentEditorActor->Modify();
		GridEditorLinkService::RemoveExactLink(*CurrentEditorActor, Link);
		RequestRefresh();
	}

	return FReply::Handled();
}

FReply SGridEditorLinksPanel::OnClearSelectedObjectLinksClicked()
{
	if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor())
	{
		CurrentEditorActor->Modify();
		CurrentEditorActor->RemoveAllLinksForSelectedObject();
		RequestRefresh();
	}

	return FReply::Handled();
}

FReply SGridEditorLinksPanel::OnSelectObjectFromLinkClicked(FGuid ObjectId)
{
	if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor())
	{
		CurrentEditorActor->Modify();

		if (CurrentEditorActor->SelectObjectById(ObjectId))
		{
			RequestRefresh();
		}
	}

	return FReply::Handled();
}

FReply SGridEditorLinksPanel::OnToggleAddConnectorClicked()
{
	bAddConnectorVisible = !bAddConnectorVisible;

	if (bAddConnectorVisible)
	{
		BuildObjectOptions();
		BuildEventOptions();
		BuildCommandOptions();
		BuildConditionOptions();
		BuildVariableOptions();
	}

	RebuildLinksSection();
	return FReply::Handled();
}

FReply SGridEditorLinksPanel::OnCreateConnectorClicked()
{
	if (!CanCreateConnector())
	{
		return FReply::Handled();
	}

	if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor())
	{
		CurrentEditorActor->Modify();
		const FGridObjectLink Link = BuildLinkFromForm();

		if (GridEditorLinkService::CreateLink(*CurrentEditorActor, Link))
		{
			SelectedTargetObjectId.Reset();
			SelectedCondition.Reset();
			SelectedConditionVariableId.Reset();
			bAddConnectorVisible = false;
			RequestRefresh();
		}
	}

	return FReply::Handled();
}

FReply SGridEditorLinksPanel::OnCancelAddConnectorClicked()
{
	SelectedTargetObjectId.Reset();
	SelectedCondition.Reset();
	SelectedConditionVariableId.Reset();
	bAddConnectorVisible = false;
	RebuildLinksSection();
	return FReply::Handled();
}

FText SGridEditorLinksPanel::GetObjectSummaryText(const FGuid& ObjectId) const
{
	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();

	if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset || !ObjectId.IsValid())
	{
		return FText::FromString(TEXT("Invalid object"));
	}

	const UGridLevelAsset* LevelAsset = CurrentEditorActor->LevelAsset;
	const EGridLevelObjectType ObjectType = LevelAsset->GetTypedPlacementType(ObjectId);
	if (ObjectType == EGridLevelObjectType::None)
	{
		return FText::FromString(TEXT("Missing object"));
	}

	int32 CellX = INDEX_NONE;
	int32 CellY = INDEX_NONE;
	EGridEdge Edge = EGridEdge::None;
	if (!LevelAsset->TryGetTypedPlacementLocation(ObjectId, CellX, CellY, Edge))
	{
		return FText::FromString(TEXT("Missing object"));
	}

	const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType>();
	const FString TypeText = TypeEnum ? TypeEnum->GetDisplayNameTextByValue(static_cast<int64>(ObjectType)).ToString() : TEXT("Object");
	FString NameText = TypeText;

	if (const FGridWorldObjectInstance* Instance = LevelAsset->FindWorldObjectInstanceById(ObjectId))
	{
		if (const UGridWorldObjectDefinitionAsset* Definition = CurrentEditorActor->FindWorldObjectDefinitionById(Instance->WorldObjectDefinitionId))
		{
			if (!Definition->DisplayName.IsEmpty())
			{
				NameText = Definition->DisplayName.ToString();
			}
		}
	}
	else if (const FGridLooseItemInstance* LooseItem = LevelAsset->FindLooseItemInstanceById(ObjectId))
	{
		if (LooseItem->ItemDefinition && !LooseItem->ItemDefinition->DisplayName.IsEmpty())
		{
			NameText = LooseItem->ItemDefinition->DisplayName.ToString();
		}
	}
	else if (const FGridMonsterSpawnInstance* MonsterSpawn = LevelAsset->FindMonsterSpawnInstanceById(ObjectId))
	{
		if (MonsterSpawn->MonsterDefinition && !MonsterSpawn->MonsterDefinition->DisplayName.IsEmpty())
		{
			NameText = MonsterSpawn->MonsterDefinition->DisplayName.ToString();
		}
	}
	else if (const FGridItemSpawnInstance* ItemSpawn = LevelAsset->FindItemSpawnInstanceById(ObjectId))
	{
		if (ItemSpawn->ItemDefinition && !ItemSpawn->ItemDefinition->DisplayName.IsEmpty())
		{
			NameText = ItemSpawn->ItemDefinition->DisplayName.ToString();
		}
	}
	else if (const FGridLogicObjectInstance* Logic = LevelAsset->FindLogicObjectInstanceById(ObjectId))
	{
		if (!Logic->LogicId.IsNone())
		{
			NameText = Logic->LogicId.ToString();
		}
	}

	const FString SummaryNameText =
		ObjectType == EGridLevelObjectType::ItemSpawn
			? FString::Printf(TEXT("%s Spawner"), *NameText.Replace(TEXT(" Spawn"), TEXT("")))
			: ObjectType == EGridLevelObjectType::MonsterSpawn
				? FString::Printf(TEXT("%s Spawner"), *NameText.Replace(TEXT(" Spawn"), TEXT("")))
				: NameText;
	const FString SuffixText =
		(ObjectType == EGridLevelObjectType::ItemSpawn || ObjectType == EGridLevelObjectType::MonsterSpawn) ? TEXT(" [Spawner]") : TEXT("");

	const UEnum* EdgeEnum = StaticEnum<EGridEdge>();
	const FString EdgeText = EdgeEnum ? EdgeEnum->GetDisplayNameTextByValue(static_cast<int64>(Edge)).ToString() : TEXT("Unknown");

	return FText::FromString(FString::Printf(TEXT("%s @ (%d,%d) %s%s"), *SummaryNameText, CellX, CellY, *EdgeText, *SuffixText));
}
FText SGridEditorLinksPanel::GetLinkSourceEventText(EGridObjectEvent SourceEvent) const
{
	const UEnum* Enum = StaticEnum<EGridObjectEvent>();

	return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(SourceEvent)) : FText::FromString(TEXT("Unknown"));
}

FText SGridEditorLinksPanel::GetLinkCommandText(EGridObjectCommand Command) const
{
	const UEnum* Enum = StaticEnum<EGridObjectCommand>();

	return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Command)) : FText::FromString(TEXT("Unknown"));
}

FText SGridEditorLinksPanel::GetLinkConditionText(EGridObjectCondition Condition) const
{
	const UEnum* Enum = StaticEnum<EGridObjectCondition>();

	return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Condition)) : FText::FromString(TEXT("Unknown"));
}

FText SGridEditorLinksPanel::GetItemTypeText(EGridItemType ItemType) const
{
	const UEnum* Enum = StaticEnum<EGridItemType>();

	return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(ItemType)) : FText::FromString(TEXT("Unknown"));
}

FText SGridEditorLinksPanel::GetIntComparisonText(EGridLogicIntComparison Comparison) const
{
	const UEnum* Enum = StaticEnum<EGridLogicIntComparison>();

	return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Comparison)) : FText::FromString(TEXT("Unknown"));
}

FText SGridEditorLinksPanel::GetLinkConditionSummaryText(const FGridObjectLink& Link) const
{
	if (Link.Condition == EGridObjectCondition::None)
	{
		return FText::GetEmpty();
	}

	FString Summary = GetLinkConditionText(Link.Condition).ToString();

	switch (Link.Condition)
	{
		case EGridObjectCondition::LevelVariableBoolEquals:
			Summary += FString::Printf(TEXT(" [%s == %s]"), *Link.ConditionVariableId.ToString(), Link.ConditionBoolValue ? TEXT("true") : TEXT("false"));
			break;

		case EGridObjectCondition::LevelVariableIntCompare:
			Summary += FString::Printf(TEXT(" [%s %s %d]"), *Link.ConditionVariableId.ToString(), *GetIntComparisonText(Link.ConditionIntComparison).ToString(),
				Link.ConditionIntValue);
			break;

		case EGridObjectCondition::ReceptacleContainsItemDefinition:
			Summary += FString::Printf(TEXT(" [%s]"), *Link.ConditionItemDefinitionId.ToString());
			break;

		case EGridObjectCondition::ReceptacleContainsItemTag:
			Summary += FString::Printf(TEXT(" [%s]"), *Link.ConditionItemTag.ToString());
			break;

		case EGridObjectCondition::ReceptacleContainsItemType:
			Summary += FString::Printf(TEXT(" [%s]"), *GetItemTypeText(Link.ConditionItemType).ToString());
			break;

		case EGridObjectCondition::ReceptacleItemCountAtLeast:
			Summary += FString::Printf(TEXT(" [%d]"), Link.ConditionCount);
			break;

		case EGridObjectCondition::ReceptacleWeightAtLeast:
			Summary += FString::Printf(TEXT(" [%.3g]"), Link.ConditionWeight);
			break;

		default:
			break;
	}

	if (Link.bInvertCondition)
	{
		Summary = FString(TEXT("NOT ")) + Summary;
	}

	return FText::FromString(FString(TEXT("Condition: ")) + Summary);
}

FText SGridEditorLinksPanel::GetSelectedObjectOptionText(const TSharedPtr<FGuid>& ObjectId, const FText& EmptyText) const
{
	return ObjectId.IsValid() ? GetObjectSummaryText(*ObjectId) : EmptyText;
}

FGridObjectLink SGridEditorLinksPanel::BuildLinkFromForm() const
{
	FGridObjectLink Link;

	if (SelectedSourceObjectId.IsValid())
	{
		Link.SourceObjectId = *SelectedSourceObjectId;
	}
	if (SelectedTargetObjectId.IsValid())
	{
		Link.TargetObjectId = *SelectedTargetObjectId;
	}
	if (SelectedSourceEvent.IsValid())
	{
		Link.SourceEvent = *SelectedSourceEvent;
	}
	if (SelectedCommand.IsValid())
	{
		Link.Command = *SelectedCommand;
	}
	if (SelectedCondition.IsValid())
	{
		Link.Condition = *SelectedCondition;
	}

	Link.bInvertCondition = Link.Condition != EGridObjectCondition::None && bInvertCondition;

	switch (Link.Condition)
	{
		case EGridObjectCondition::LevelVariableBoolEquals:
			Link.ConditionVariableId = SelectedConditionVariableId.IsValid() ? *SelectedConditionVariableId : NAME_None;
			Link.ConditionBoolValue = ConditionBoolValue;
			break;

		case EGridObjectCondition::LevelVariableIntCompare:
			Link.ConditionVariableId = SelectedConditionVariableId.IsValid() ? *SelectedConditionVariableId : NAME_None;
			Link.ConditionIntComparison = SelectedConditionIntComparison.IsValid() ? *SelectedConditionIntComparison : EGridLogicIntComparison::Equal;
			Link.ConditionIntValue = ConditionIntValue;
			break;

		case EGridObjectCondition::ReceptacleContainsItemDefinition:
			Link.ConditionItemDefinitionId = ConditionItemDefinitionId;
			break;

		case EGridObjectCondition::ReceptacleContainsItemTag:
			Link.ConditionItemTag = ConditionItemTag;
			break;

		case EGridObjectCondition::ReceptacleContainsItemType:
			Link.ConditionItemType = SelectedConditionItemType.IsValid() ? *SelectedConditionItemType : EGridItemType::None;
			break;

		case EGridObjectCondition::ReceptacleItemCountAtLeast:
			Link.ConditionCount = ConditionCount;
			break;

		case EGridObjectCondition::ReceptacleWeightAtLeast:
			Link.ConditionWeight = ConditionWeight;
			break;

		default:
			break;
	}

	return GridEditorLinkService::NormalizeLink(Link);
}

bool SGridEditorLinksPanel::CanCreateConnector() const
{
	const auto ContainsGuid = [](const TArray<TSharedPtr<FGuid>>& Options, const TSharedPtr<FGuid>& Value)
	{
		if (!Value.IsValid())
		{
			return false;
		}

		for (const TSharedPtr<FGuid>& Option : Options)
		{
			if (Option.IsValid() && *Option == *Value)
			{
				return true;
			}
		}

		return false;
	};

	const auto ContainsEvent = [](const TArray<TSharedPtr<EGridObjectEvent>>& Options, const TSharedPtr<EGridObjectEvent>& Value)
	{
		if (!Value.IsValid())
		{
			return false;
		}

		for (const TSharedPtr<EGridObjectEvent>& Option : Options)
		{
			if (Option.IsValid() && *Option == *Value)
			{
				return true;
			}
		}

		return false;
	};

	const auto ContainsCommand = [](const TArray<TSharedPtr<EGridObjectCommand>>& Options, const TSharedPtr<EGridObjectCommand>& Value)
	{
		if (!Value.IsValid())
		{
			return false;
		}

		for (const TSharedPtr<EGridObjectCommand>& Option : Options)
		{
			if (Option.IsValid() && *Option == *Value)
			{
				return true;
			}
		}

		return false;
	};

	const auto ContainsCondition = [](const TArray<TSharedPtr<EGridObjectCondition>>& Options, const TSharedPtr<EGridObjectCondition>& Value)
	{
		if (!Value.IsValid())
		{
			return false;
		}

		for (const TSharedPtr<EGridObjectCondition>& Option : Options)
		{
			if (Option.IsValid() && *Option == *Value)
			{
				return true;
			}
		}

		return false;
	};

	if (!ContainsGuid(SourceObjectOptions, SelectedSourceObjectId) || !SelectedSourceObjectId->IsValid() ||
		!ContainsEvent(LinkSourceEventOptions, SelectedSourceEvent) || !ContainsGuid(TargetObjectOptions, SelectedTargetObjectId) ||
		!SelectedTargetObjectId->IsValid() || !ContainsCommand(LinkCommandOptions, SelectedCommand) ||
		!ContainsCondition(LinkConditionOptions, SelectedCondition))
	{
		return false;
	}

	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset)
	{
		return false;
	}

	const FGridObjectLink Link = BuildLinkFromForm();
	return GridEditorLinkService::IsLinkSupported(*CurrentEditorActor->LevelAsset, Link) &&
		!GridEditorLinkService::ContainsExactLink(CurrentEditorActor->LevelAsset->Links, Link);
}

bool SGridEditorLinksPanel::IsConditionSelected(EGridObjectCondition Condition) const
{
	return SelectedCondition.IsValid() && *SelectedCondition == Condition;
}

void SGridEditorLinksPanel::BuildLinkOptions()
{
	BuildItemTypeOptions();
	BuildIntComparisonOptions();
	BuildEventOptions();
	BuildCommandOptions();
	BuildConditionOptions();
	BuildVariableOptions();
}

void SGridEditorLinksPanel::BuildObjectOptions()
{
	SourceObjectOptions.Reset();
	TargetObjectOptions.Reset();

	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset)
	{
		SelectedSourceObjectId.Reset();
		SelectedTargetObjectId.Reset();
		return;
	}

	const UGridLevelAsset* LevelAsset = CurrentEditorActor->LevelAsset;
	const auto AddPlacement = [this](const FGuid& ObjectId, EGridLevelObjectType ObjectType, EGridLogicNodeType LogicNodeType = EGridLogicNodeType::Relay)
	{
		if (GridEditorLinkPolicy::CanObjectEmitEvents(ObjectType, LogicNodeType))
		{
			SourceObjectOptions.Add(MakeShared<FGuid>(ObjectId));
		}
		if (GridEditorLinkPolicy::CanObjectReceiveCommands(ObjectType, LogicNodeType))
		{
			TargetObjectOptions.Add(MakeShared<FGuid>(ObjectId));
		}
	};

	for (const FGridWorldObjectInstance& Instance : LevelAsset->WorldObjectInstances)
	{
		AddPlacement(Instance.InstanceId, Instance.Type);
	}
	for (const FGridLooseItemInstance& Instance : LevelAsset->LooseItemInstances)
	{
		AddPlacement(Instance.InstanceId, EGridLevelObjectType::Item);
	}
	for (const FGridMonsterSpawnInstance& Spawn : LevelAsset->MonsterSpawns)
	{
		AddPlacement(Spawn.SpawnId, EGridLevelObjectType::MonsterSpawn);
	}
	for (const FGridItemSpawnInstance& Spawn : LevelAsset->ItemSpawns)
	{
		AddPlacement(Spawn.SpawnId, EGridLevelObjectType::ItemSpawn);
	}
	for (const FGridLogicObjectInstance& Instance : LevelAsset->LogicObjects)
	{
		AddPlacement(Instance.InstanceId, Instance.Type, Instance.Logic.NodeType);
	}

	const auto FindOption = [](const TArray<TSharedPtr<FGuid>>& Options, const FGuid& ObjectId) -> TSharedPtr<FGuid>
	{
		for (const TSharedPtr<FGuid>& Option : Options)
		{
			if (Option.IsValid() && *Option == ObjectId)
			{
				return Option;
			}
		}
		return nullptr;
	};

	if (!SelectedSourceObjectId.IsValid() && LevelAsset->ContainsTypedPlacementId(CurrentEditorActor->LastSelectedObjectId))
	{
		SelectedSourceObjectId = FindOption(SourceObjectOptions, CurrentEditorActor->LastSelectedObjectId);
	}
	else if (SelectedSourceObjectId.IsValid())
	{
		SelectedSourceObjectId = FindOption(SourceObjectOptions, *SelectedSourceObjectId);
	}

	if (SelectedTargetObjectId.IsValid())
	{
		SelectedTargetObjectId = FindOption(TargetObjectOptions, *SelectedTargetObjectId);
	}
}
void SGridEditorLinksPanel::BuildEventOptions()
{
	LinkSourceEventOptions.Reset();

	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	if (CurrentEditorActor && CurrentEditorActor->LevelAsset && SelectedSourceObjectId.IsValid())
	{
		const UGridLevelAsset* LevelAsset = CurrentEditorActor->LevelAsset;
		const EGridLevelObjectType SourceType = LevelAsset->GetTypedPlacementType(*SelectedSourceObjectId);
		if (SourceType != EGridLevelObjectType::None)
		{
			for (const EGridObjectEvent Event :
				GridEditorLinkPolicy::GetSupportedEventsForSource(SourceType, GetLogicNodeTypeForPlacement(LevelAsset, *SelectedSourceObjectId)))
			{
				LinkSourceEventOptions.Add(MakeShared<EGridObjectEvent>(Event));
			}
		}
	}

	bool bCurrentEventStillValid = false;
	for (const TSharedPtr<EGridObjectEvent>& Option : LinkSourceEventOptions)
	{
		if (Option.IsValid() && SelectedSourceEvent.IsValid() && *Option == *SelectedSourceEvent)
		{
			SelectedSourceEvent = Option;
			bCurrentEventStillValid = true;
			break;
		}
	}

	if (!bCurrentEventStillValid)
	{
		SelectedSourceEvent = LinkSourceEventOptions.Num() > 0 ? LinkSourceEventOptions[0] : nullptr;
	}
}
void SGridEditorLinksPanel::BuildCommandOptions()
{
	LinkCommandOptions.Reset();

	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	if (CurrentEditorActor && CurrentEditorActor->LevelAsset && SelectedTargetObjectId.IsValid())
	{
		const UGridLevelAsset* LevelAsset = CurrentEditorActor->LevelAsset;
		const EGridLevelObjectType TargetType = LevelAsset->GetTypedPlacementType(*SelectedTargetObjectId);
		if (TargetType != EGridLevelObjectType::None)
		{
			for (const EGridObjectCommand Command :
				GridEditorLinkPolicy::GetSupportedCommandsForTarget(TargetType, GetLogicNodeTypeForPlacement(LevelAsset, *SelectedTargetObjectId)))
			{
				LinkCommandOptions.Add(MakeShared<EGridObjectCommand>(Command));
			}
		}
	}

	bool bCurrentCommandStillValid = false;
	for (const TSharedPtr<EGridObjectCommand>& Option : LinkCommandOptions)
	{
		if (Option.IsValid() && SelectedCommand.IsValid() && *Option == *SelectedCommand)
		{
			SelectedCommand = Option;
			bCurrentCommandStillValid = true;
			break;
		}
	}

	if (!bCurrentCommandStillValid)
	{
		SelectedCommand = LinkCommandOptions.Num() > 0 ? LinkCommandOptions[0] : nullptr;
	}
}
void SGridEditorLinksPanel::BuildConditionOptions()
{
	LinkConditionOptions.Reset();

	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	if (CurrentEditorActor && CurrentEditorActor->LevelAsset && SelectedTargetObjectId.IsValid())
	{
		const EGridLevelObjectType TargetType = CurrentEditorActor->LevelAsset->GetTypedPlacementType(*SelectedTargetObjectId);
		if (TargetType != EGridLevelObjectType::None)
		{
			for (const EGridObjectCondition Condition : GridEditorLinkPolicy::GetSupportedConditionsForTarget(TargetType))
			{
				LinkConditionOptions.Add(MakeShared<EGridObjectCondition>(Condition));
			}
		}
	}

	bool bCurrentConditionStillValid = false;
	for (const TSharedPtr<EGridObjectCondition>& Option : LinkConditionOptions)
	{
		if (Option.IsValid() && SelectedCondition.IsValid() && *Option == *SelectedCondition)
		{
			SelectedCondition = Option;
			bCurrentConditionStillValid = true;
			break;
		}
	}

	if (!bCurrentConditionStillValid)
	{
		SelectedCondition = LinkConditionOptions.Num() > 0 ? LinkConditionOptions[0] : nullptr;
	}

	if (!SelectedCondition.IsValid() || *SelectedCondition == EGridObjectCondition::None)
	{
		bInvertCondition = false;
	}
}
void SGridEditorLinksPanel::BuildItemTypeOptions()
{
	ItemTypeOptions.Reset();

	const EGridItemType ItemTypes[] = { EGridItemType::Torch, EGridItemType::Weapon, EGridItemType::Shield, EGridItemType::Armor, EGridItemType::Jewelry,
		EGridItemType::Key, EGridItemType::Gem, EGridItemType::Potion, EGridItemType::Scroll, EGridItemType::Book, EGridItemType::Food,
		EGridItemType::Component, EGridItemType::Quest, EGridItemType::Misc };

	for (const EGridItemType ItemType : ItemTypes)
	{
		ItemTypeOptions.Add(MakeShared<EGridItemType>(ItemType));
	}

	bool bCurrentTypeStillValid = false;
	for (const TSharedPtr<EGridItemType>& Option : ItemTypeOptions)
	{
		if (Option.IsValid() && SelectedConditionItemType.IsValid() && *Option == *SelectedConditionItemType)
		{
			SelectedConditionItemType = Option;
			bCurrentTypeStillValid = true;
			break;
		}
	}

	if (!bCurrentTypeStillValid)
	{
		SelectedConditionItemType = ItemTypeOptions.Num() > 0 ? ItemTypeOptions[0] : nullptr;
	}
}

void SGridEditorLinksPanel::BuildVariableOptions()
{
	VariableOptions.Reset();

	EGridLevelVariableType RequiredType = EGridLevelVariableType::Bool;
	bool bUsesVariable = false;
	if (IsConditionSelected(EGridObjectCondition::LevelVariableBoolEquals))
	{
		RequiredType = EGridLevelVariableType::Bool;
		bUsesVariable = true;
	}
	else if (IsConditionSelected(EGridObjectCondition::LevelVariableIntCompare))
	{
		RequiredType = EGridLevelVariableType::Int32;
		bUsesVariable = true;
	}

	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	if (!bUsesVariable || !CurrentEditorActor || !CurrentEditorActor->LevelAsset)
	{
		SelectedConditionVariableId.Reset();
		return;
	}

	for (const FGridLevelVariableDefinition& Definition : CurrentEditorActor->LevelAsset->LevelVariables)
	{
		if (!Definition.VariableId.IsNone() && Definition.Type == RequiredType)
		{
			VariableOptions.Add(MakeShared<FName>(Definition.VariableId));
		}
	}

	bool bCurrentVariableStillValid = false;
	for (const TSharedPtr<FName>& Option : VariableOptions)
	{
		if (Option.IsValid() && SelectedConditionVariableId.IsValid() && *Option == *SelectedConditionVariableId)
		{
			SelectedConditionVariableId = Option;
			bCurrentVariableStillValid = true;
			break;
		}
	}

	if (!bCurrentVariableStillValid)
	{
		SelectedConditionVariableId = VariableOptions.Num() > 0 ? VariableOptions[0] : nullptr;
	}
}

void SGridEditorLinksPanel::BuildIntComparisonOptions()
{
	IntComparisonOptions.Reset();
	const EGridLogicIntComparison Comparisons[] = { EGridLogicIntComparison::Equal, EGridLogicIntComparison::NotEqual, EGridLogicIntComparison::Less,
		EGridLogicIntComparison::LessOrEqual, EGridLogicIntComparison::Greater, EGridLogicIntComparison::GreaterOrEqual };

	for (const EGridLogicIntComparison Comparison : Comparisons)
	{
		IntComparisonOptions.Add(MakeShared<EGridLogicIntComparison>(Comparison));
	}

	bool bCurrentComparisonStillValid = false;
	for (const TSharedPtr<EGridLogicIntComparison>& Option : IntComparisonOptions)
	{
		if (Option.IsValid() && SelectedConditionIntComparison.IsValid() && *Option == *SelectedConditionIntComparison)
		{
			SelectedConditionIntComparison = Option;
			bCurrentComparisonStillValid = true;
			break;
		}
	}

	if (!bCurrentComparisonStillValid)
	{
		SelectedConditionIntComparison = IntComparisonOptions.Num() > 0 ? IntComparisonOptions[0] : nullptr;
	}
}

void SGridEditorLinksPanel::RefreshConnectorFormOptions()
{
	BuildObjectOptions();
	BuildEventOptions();
	BuildCommandOptions();
	BuildConditionOptions();
	BuildVariableOptions();
}

TSharedRef<SWidget> SGridEditorLinksPanel::MakeObjectComboWidget(TSharedPtr<FGuid> Item) const
{
	if (!Item.IsValid())
	{
		return SNew(STextBlock).Text(FText::FromString(TEXT("Invalid")));
	}

	return SNew(STextBlock).Text(GetObjectSummaryText(*Item));
}

void SGridEditorLinksPanel::OnSourceObjectSelectionChanged(TSharedPtr<FGuid> NewValue, ESelectInfo::Type SelectInfo)
{
	(void)SelectInfo;
	SelectedSourceObjectId = NewValue;
	BuildEventOptions();
}

void SGridEditorLinksPanel::OnTargetObjectSelectionChanged(TSharedPtr<FGuid> NewValue, ESelectInfo::Type SelectInfo)
{
	(void)SelectInfo;
	SelectedTargetObjectId = NewValue;
	BuildCommandOptions();
	BuildConditionOptions();
	BuildVariableOptions();
	RebuildLinksSection();
}

TSharedRef<SWidget> SGridEditorLinksPanel::MakeLinkSourceEventComboWidget(TSharedPtr<EGridObjectEvent> Item) const
{
	if (!Item.IsValid())
	{
		return SNew(STextBlock).Text(FText::FromString(TEXT("Invalid")));
	}

	return SNew(STextBlock).Text(GetLinkSourceEventText(*Item));
}

void SGridEditorLinksPanel::OnLinkSourceEventSelectionChanged(TSharedPtr<EGridObjectEvent> NewValue, ESelectInfo::Type SelectInfo)
{
	(void)SelectInfo;
	if (NewValue.IsValid())
	{
		SelectedSourceEvent = NewValue;
	}
}

FText SGridEditorLinksPanel::GetSelectedLinkSourceEventText() const
{
	return SelectedSourceEvent.IsValid() ? GetLinkSourceEventText(*SelectedSourceEvent) : FText::FromString(TEXT("Select event"));
}

TSharedRef<SWidget> SGridEditorLinksPanel::MakeLinkCommandComboWidget(TSharedPtr<EGridObjectCommand> Item) const
{
	if (!Item.IsValid())
	{
		return SNew(STextBlock).Text(FText::FromString(TEXT("Invalid")));
	}

	return SNew(STextBlock).Text(GetLinkCommandText(*Item));
}

void SGridEditorLinksPanel::OnLinkCommandSelectionChanged(TSharedPtr<EGridObjectCommand> NewValue, ESelectInfo::Type SelectInfo)
{
	(void)SelectInfo;
	if (NewValue.IsValid())
	{
		SelectedCommand = NewValue;
	}
}

FText SGridEditorLinksPanel::GetSelectedLinkCommandText() const
{
	return SelectedCommand.IsValid() ? GetLinkCommandText(*SelectedCommand) : FText::FromString(TEXT("Select command"));
}

TSharedRef<SWidget> SGridEditorLinksPanel::MakeLinkConditionComboWidget(TSharedPtr<EGridObjectCondition> Item) const
{
	if (!Item.IsValid())
	{
		return SNew(STextBlock).Text(FText::FromString(TEXT("Invalid")));
	}

	return SNew(STextBlock).Text(GetLinkConditionText(*Item));
}

void SGridEditorLinksPanel::OnLinkConditionSelectionChanged(TSharedPtr<EGridObjectCondition> NewValue, ESelectInfo::Type SelectInfo)
{
	(void)SelectInfo;
	if (NewValue.IsValid())
	{
		SelectedCondition = NewValue;
		if (*SelectedCondition == EGridObjectCondition::None)
		{
			bInvertCondition = false;
		}
		BuildVariableOptions();
		RebuildLinksSection();
	}
}

FText SGridEditorLinksPanel::GetSelectedLinkConditionText() const
{
	return SelectedCondition.IsValid() ? GetLinkConditionText(*SelectedCondition) : FText::FromString(TEXT("Select condition"));
}

TSharedRef<SWidget> SGridEditorLinksPanel::MakeItemTypeComboWidget(TSharedPtr<EGridItemType> Item) const
{
	if (!Item.IsValid())
	{
		return SNew(STextBlock).Text(FText::FromString(TEXT("Invalid")));
	}

	return SNew(STextBlock).Text(GetItemTypeText(*Item));
}

void SGridEditorLinksPanel::OnItemTypeSelectionChanged(TSharedPtr<EGridItemType> NewValue, ESelectInfo::Type SelectInfo)
{
	(void)SelectInfo;
	if (NewValue.IsValid())
	{
		SelectedConditionItemType = NewValue;
	}
}

FText SGridEditorLinksPanel::GetSelectedItemTypeText() const
{
	return SelectedConditionItemType.IsValid() ? GetItemTypeText(*SelectedConditionItemType) : FText::FromString(TEXT("Select item type"));
}

TSharedRef<SWidget> SGridEditorLinksPanel::MakeVariableComboWidget(TSharedPtr<FName> Item) const
{
	if (!Item.IsValid())
	{
		return SNew(STextBlock).Text(FText::FromString(TEXT("Invalid")));
	}

	return SNew(STextBlock).Text(FText::FromName(*Item));
}

void SGridEditorLinksPanel::OnVariableSelectionChanged(TSharedPtr<FName> NewValue, ESelectInfo::Type SelectInfo)
{
	(void)SelectInfo;
	if (NewValue.IsValid())
	{
		SelectedConditionVariableId = NewValue;
	}
}

FText SGridEditorLinksPanel::GetSelectedVariableText() const
{
	return SelectedConditionVariableId.IsValid() ? FText::FromName(*SelectedConditionVariableId) : FText::FromString(TEXT("Select variable"));
}

TSharedRef<SWidget> SGridEditorLinksPanel::MakeIntComparisonComboWidget(TSharedPtr<EGridLogicIntComparison> Item) const
{
	if (!Item.IsValid())
	{
		return SNew(STextBlock).Text(FText::FromString(TEXT("Invalid")));
	}

	return SNew(STextBlock).Text(GetIntComparisonText(*Item));
}

void SGridEditorLinksPanel::OnIntComparisonSelectionChanged(TSharedPtr<EGridLogicIntComparison> NewValue, ESelectInfo::Type SelectInfo)
{
	(void)SelectInfo;
	if (NewValue.IsValid())
	{
		SelectedConditionIntComparison = NewValue;
	}
}

FText SGridEditorLinksPanel::GetSelectedIntComparisonText() const
{
	return SelectedConditionIntComparison.IsValid() ? GetIntComparisonText(*SelectedConditionIntComparison) : FText::FromString(TEXT("Select comparison"));
}

#endif

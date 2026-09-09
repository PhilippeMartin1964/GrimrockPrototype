#include "EditorTools/Widgets/SGridEditorObjectInspectorPanel.h"

#if WITH_EDITOR

#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectBehavior.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

#include "AssetRegistry/AssetData.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"

#include "Templates/Function.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"

namespace
{
	const FGridWorldObjectInstance* GetWorldObjectInstance(const AGridLevelEditorActor* Editor, FGuid ObjectId)
	{
		return Editor && Editor->LevelAsset ? Editor->LevelAsset->FindWorldObjectInstanceById(ObjectId) : nullptr;
	}

	const UGridWorldObjectDefinitionAsset* GetWorldObjectDefinition(const AGridLevelEditorActor* Editor, FGuid ObjectId)
	{
		const FGridWorldObjectInstance* WorldObjectInstance = GetWorldObjectInstance(Editor, ObjectId);
		return WorldObjectInstance ? Editor->FindWorldObjectDefinitionById(WorldObjectInstance->WorldObjectDefinitionId) : nullptr;
	}

	template <typename TValue, typename TRead>
	TValue ReadPlacementValue(const UGridLevelAsset& Level, FGuid ObjectId, TRead&& Read)
	{
		if (const FGridWorldObjectInstance* WorldObjectInstance = Level.FindWorldObjectInstanceById(ObjectId)) return Read(*WorldObjectInstance);
		if (const FGridLooseItemInstance* LooseItemInstance = Level.FindLooseItemInstanceById(ObjectId)) return Read(*LooseItemInstance);
		if (const FGridMonsterSpawnInstance* MonsterSpawn = Level.FindMonsterSpawnInstanceById(ObjectId)) return Read(*MonsterSpawn);
		if (const FGridItemSpawnInstance* ItemSpawn = Level.FindItemSpawnInstanceById(ObjectId)) return Read(*ItemSpawn);
		if (const FGridLogicObjectInstance* LogicInstance = Level.FindLogicObjectInstanceById(ObjectId)) return Read(*LogicInstance);
		return TValue();
	}

	FName GetObjectTagValue(const UGridLevelAsset& Level, FGuid ObjectId)
	{
		return ReadPlacementValue<FName>(Level, ObjectId, [](const auto& Placement) { return Placement.Tag; });
	}

	bool IsPlacementInitiallyActive(const UGridLevelAsset& Level, FGuid ObjectId)
	{
		if (const FGridWorldObjectInstance* WorldObjectInstance = Level.FindWorldObjectInstanceById(ObjectId)) return WorldObjectInstance->bInitiallyActive;
		if (const FGridLogicObjectInstance* LogicInstance = Level.FindLogicObjectInstanceById(ObjectId)) return LogicInstance->bInitiallyActive;
		return false;
	}

	float GetPlacementYaw(const UGridLevelAsset& Level, FGuid ObjectId)
	{
		if (const FGridWorldObjectInstance* WorldObjectInstance = Level.FindWorldObjectInstanceById(ObjectId))
			return WorldObjectInstance->bHasLocalTransformOverride ? WorldObjectInstance->LocalTransformOverride.Rotator().Yaw : 0.f;
		if (const FGridLooseItemInstance* LooseItemInstance = Level.FindLooseItemInstanceById(ObjectId)) return LooseItemInstance->LocalYaw;
		return 0.f;
	}

	FText GetInitialActiveStateText(const UGridLevelAsset& Level, FGuid ObjectId, const TCHAR* ActiveText, const TCHAR* InactiveText)
	{
		return FText::FromString(IsPlacementInitiallyActive(Level, ObjectId) ? ActiveText : InactiveText);
	}

	FText GetBoolText(bool bValue)
	{
		return FText::FromString(bValue ? TEXT("Yes") : TEXT("No"));
	}

	FText GetNameText(const FName& Name)
	{
		return Name.IsNone() ? FText::FromString(TEXT("None")) : FText::FromName(Name);
	}

	FName GetNameFromEditorText(const FText& Text)
	{
		const FString Value = Text.ToString().TrimStartAndEnd();
		return Value.IsEmpty() || Value.Equals(TEXT("None"), ESearchCase::IgnoreCase) ? NAME_None : FName(*Value);
	}

	FText GetObjectNameText(const UObject* Object)
	{
		return Object ? FText::FromString(Object->GetName()) : FText::FromString(TEXT("None"));
	}

	FText GetClassNameText(const UClass* Class)
	{
		return Class ? FText::FromString(Class->GetName()) : FText::FromString(TEXT("None"));
	}

	FText GetEdgeOrFacingText(const UGridLevelAsset& Level, FGuid ObjectId)
	{
		const UEnum* EdgeEnum = StaticEnum<EGridEdge>();
		if (const FGridMonsterSpawnInstance* MonsterSpawn = Level.FindMonsterSpawnInstanceById(ObjectId))
		{
			return FText::Format(
				FText::FromString(TEXT("Facing {0}")), GridEditorWidgetHelpers::GetGridEnumDisplayText(EdgeEnum, static_cast<int64>(MonsterSpawn->Facing)));
		}
		int32 CellX, CellY;
		EGridEdge Edge;
		if (Level.TryGetTypedPlacementLocation(ObjectId, CellX, CellY, Edge) && Edge != EGridEdge::None)
		{
			return GridEditorWidgetHelpers::GetGridEnumDisplayText(EdgeEnum, static_cast<int64>(Edge));
		}
		return FText::Format(FText::FromString(TEXT("Facing {0} deg")), FText::AsNumber(GetPlacementYaw(Level, ObjectId)));
	}

	FText GetHeaderPlacementText(const UGridLevelAsset& Level, FGuid ObjectId)
	{
		int32 CellX, CellY;
		EGridEdge Edge;
		if (!Level.TryGetTypedPlacementLocation(ObjectId, CellX, CellY, Edge)) return FText::GetEmpty();
		if (Edge != EGridEdge::None || !FMath::IsNearlyZero(GetPlacementYaw(Level, ObjectId)) || Level.FindMonsterSpawnInstanceById(ObjectId))
		{
			return FText::Format(FText::FromString(TEXT("@ ({0},{1}) {2}")), FText::AsNumber(CellX), FText::AsNumber(CellY), GetEdgeOrFacingText(Level, ObjectId));
		}
		return FText::Format(FText::FromString(TEXT("@ ({0},{1})")), FText::AsNumber(CellX), FText::AsNumber(CellY));
	}

	TSharedRef<SWidget> BuildExplicitConnectorSummary(const FText& EmitsText)
	{
		TSharedRef<SVerticalBox> Root = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Emits")), EmitsText)]
			+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
				FText::FromString(TEXT("Connector Rule")), FText::FromString(TEXT("Only explicit SourceEvent links execute.")))];
		return Root;
	}

	FText GetConnectorEventText(EGridObjectEvent Event)
	{
		const UEnum* EventEnum = StaticEnum<EGridObjectEvent>();
		const FText EventText = EventEnum ? EventEnum->GetDisplayNameTextByValue(static_cast<int64>(Event)) : FText::FromString(TEXT("Unknown"));
		return FText::Format(FText::FromString(TEXT("On {0}")), EventText);
	}

	FText GetConnectorCommandText(EGridObjectCommand Command)
	{
		const UEnum* CommandEnum = StaticEnum<EGridObjectCommand>();
		return CommandEnum ? CommandEnum->GetDisplayNameTextByValue(static_cast<int64>(Command)) : FText::FromString(TEXT("Unknown"));
	}

	TSharedRef<SWidget> BuildConnectorTextRow(const FText& Text, bool bWarning)
	{
		return SNew(STextBlock)
			.Text(Text)
			.AutoWrapText(true)
			.ColorAndOpacity(bWarning ? FSlateColor(FLinearColor(1.f, 0.55f, 0.18f, 1.f)) : FSlateColor::UseForeground());
	}

	bool IsObjectOrientationEditable(EGridLevelObjectType Type, const UGridWorldObjectDefinitionAsset* Definition)
	{
		if (Type == EGridLevelObjectType::MonsterSpawn || Type == EGridLevelObjectType::Item) return true;
		if (!Definition)
		{
			return false;
		}
		const bool bPlacementCanFace = Definition->PlacementSurface == EGridObjectPlacementKind::Edge ||
			Definition->PlacementSurface == EGridObjectPlacementKind::Wall || Definition->PlacementSurface == EGridObjectPlacementKind::Floor ||
			Definition->PlacementSurface == EGridObjectPlacementKind::Center;
		if (!bPlacementCanFace)
		{
			return false;
		}
		if (Type == EGridLevelObjectType::Trigger || Type == EGridLevelObjectType::ItemSpawn)
		{
			return Definition->HasAnyVisualPart();
		}
		return Definition->HasAnyVisualPart() || Definition->RuntimeActorClass || Definition->ItemActorClass;
	}

	TSharedRef<SWidget> BuildBehaviorFloatSpinBoxRow(const FText& Label, float Value, TFunction<void(float)> ApplyValue)
	{
		return GridEditorWidgetHelpers::BuildGridPropertyRow(Label,
			SNew(SSpinBox<float>)
				.Value(Value)
				.MinDesiredWidth(90.f)
				.OnValueCommitted_Lambda([ApplyValue](float NewValue, ETextCommit::Type CommitType)
				{
					ApplyValue(NewValue);
				}));
	}

	TSharedRef<SWidget> BuildItemDefinitionAssetPicker(UGridItemDefinitionAsset* CurrentAsset, TFunction<void(UGridItemDefinitionAsset*)> ApplyAsset)
	{
		return SNew(SObjectPropertyEntryBox)
			.AllowedClass(UGridItemDefinitionAsset::StaticClass())
			.ObjectPath(CurrentAsset ? CurrentAsset->GetPathName() : FString())
			.OnObjectChanged_Lambda([ApplyAsset](const FAssetData& AssetData)
			{
				ApplyAsset(Cast<UGridItemDefinitionAsset>(AssetData.GetAsset()));
			});
	}

	TSharedRef<SWidget> BuildReadableContentAssetPicker(UGridReadableContentAsset* CurrentAsset, TFunction<void(UGridReadableContentAsset*)> ApplyAsset)
	{
		return SNew(SObjectPropertyEntryBox)
			.AllowedClass(UGridReadableContentAsset::StaticClass())
			.ObjectPath(CurrentAsset ? CurrentAsset->GetPathName() : FString())
			.OnObjectChanged_Lambda([ApplyAsset](const FAssetData& AssetData)
			{
				ApplyAsset(Cast<UGridReadableContentAsset>(AssetData.GetAsset()));
			});
	}

	TSharedRef<SWidget> BuildMonsterDefinitionAssetPicker(UGridMonsterDefinitionAsset* CurrentAsset, TFunction<void(UGridMonsterDefinitionAsset*)> ApplyAsset)
	{
		return SNew(SObjectPropertyEntryBox)
			.AllowedClass(UGridMonsterDefinitionAsset::StaticClass())
			.ObjectPath(CurrentAsset ? CurrentAsset->GetPathName() : FString())
			.OnObjectChanged_Lambda([ApplyAsset](const FAssetData& AssetData)
			{
				ApplyAsset(Cast<UGridMonsterDefinitionAsset>(AssetData.GetAsset()));
			});
	}
}

void SGridEditorObjectInspectorPanel::Construct(const FArguments& InArgs)
{
	EditorActor = InArgs._EditorActor;
	OnGetEditorActor = InArgs._OnGetEditorActor;
	OnRequestRefresh = InArgs._OnRequestRefresh;
	ChildSlot[BuildObjectInspectorSection()];
}

AGridLevelEditorActor* SGridEditorObjectInspectorPanel::GetEditorActor() const
{
	if (EditorActor.IsValid())
	{
		return EditorActor.Get();
	}
	return OnGetEditorActor.IsBound() ? OnGetEditorActor.Execute() : nullptr;
}

void SGridEditorObjectInspectorPanel::RequestRefresh() const
{
	if (OnRequestRefresh.IsBound())
	{
		OnRequestRefresh.Execute();
	}
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildObjectInspectorSection()
{
	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);
	if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset)
	{
		Root->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("No editor actor or level asset.")))];
		return Root;
	}
	const FGuid Obj = CurrentEditorActor->LastSelectedObjectId;
	if (!CurrentEditorActor->LevelAsset->ContainsTypedPlacementId(Obj))
	{
		Root->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("No selected object.")))];
		return Root;
	}
	Root->AddSlot().AutoHeight()[BuildSelectedObjectCard(Obj)];
	const UGridWorldObjectDefinitionAsset* SelectedDefinition = GetWorldObjectDefinition(CurrentEditorActor, Obj);
	const bool bShowOrientationWidget = IsObjectOrientationEditable(CurrentEditorActor->LevelAsset->GetTypedPlacementType(Obj), SelectedDefinition);
	Root->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)[GridEditorWidgetHelpers::BuildGridActionButton(
			FText::FromString(TEXT("Move To Current Cell")), FOnClicked::CreateSP(this, &SGridEditorObjectInspectorPanel::OnMoveSelectedObjectToCurrentCellClicked))]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f, 0.f, 0.f)[bShowOrientationWidget ? BuildOrientationWidget(Obj) : SNullWidget::NullWidget]];
	return Root;
}

void SGridEditorObjectInspectorPanel::EditWorldObjectConfig(FGuid ObjectId, TFunctionRef<void(FGridWorldObjectInstanceConfig&)> Edit)
{
	AGridLevelEditorActor* Editor = GetEditorActor();
	if (!Editor || !Editor->LevelAsset || Editor->LastSelectedObjectId != ObjectId) return;
	FGridWorldObjectInstance* WorldObjectInstance = Editor->LevelAsset->FindWorldObjectInstanceById(ObjectId);
	if (!WorldObjectInstance) return;
	Editor->LevelAsset->Modify();
	Edit(WorldObjectInstance->InstanceConfig);
	Editor->ObjectBehavior = GridObjectInstanceBehavior::Resolve(*WorldObjectInstance, GetWorldObjectDefinition(Editor, ObjectId));
	Editor->LevelAsset->MarkPackageDirty();
	Editor->RebuildPreview();
	RequestRefresh();
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildOrientationWidget(FGuid ObjectId)
{
	const AGridLevelEditorActor* Editor = GetEditorActor();
	if (!Editor || !Editor->LevelAsset) return SNullWidget::NullWidget;
	const UGridLevelAsset& Level = *Editor->LevelAsset;
	int32 CellX, CellY;
	EGridEdge Edge;
	if (!Level.TryGetTypedPlacementLocation(ObjectId, CellX, CellY, Edge)) return SNullWidget::NullWidget;
	const FGridMonsterSpawnInstance* MonsterSpawn = Level.FindMonsterSpawnInstanceById(ObjectId);
	const float Yaw = FRotator::ClampAxis(GetPlacementYaw(Level, ObjectId));
	const EGridEdge CurrentOrientation = MonsterSpawn ? MonsterSpawn->Facing : Edge != EGridEdge::None ? Edge
		: (FMath::IsNearlyEqual(Yaw, 90.f) ? EGridEdge::East
			: FMath::IsNearlyEqual(Yaw, 180.f) ? EGridEdge::South
			: FMath::IsNearlyEqual(Yaw, 270.f) ? EGridEdge::West : EGridEdge::North);
	auto MakeButton = [this, CurrentOrientation](const TCHAR* Label, EGridEdge Orientation) -> TSharedRef<SWidget>
	{
		return SNew(SButton)
			.Text(FText::FromString(Label))
			.ContentPadding(FMargin(6.f, 3.f))
			.ButtonColorAndOpacity(CurrentOrientation == Orientation ? FLinearColor(0.25f, 0.45f, 0.75f, 1.f) : FLinearColor::White)
			.OnClicked(FOnClicked::CreateSP(this, &SGridEditorObjectInspectorPanel::OnSetSelectedObjectOrientationClicked, Orientation));
	};
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)[
			SNew(STextBlock).Text(FText::FromString(TEXT("Orientation"))).ColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.72f, 0.72f, 1.f)))]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 2.f, 0.f)[MakeButton(TEXT("North"), EGridEdge::North)]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)[MakeButton(TEXT("East"), EGridEdge::East)]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)[MakeButton(TEXT("South"), EGridEdge::South)]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f, 0.f, 0.f)[MakeButton(TEXT("West"), EGridEdge::West)];
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildSelectedObjectCard(FGuid Obj)
{
	const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType>();
	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset) return SNullWidget::NullWidget;
	const EGridLevelObjectType Type = CurrentEditorActor->LevelAsset->GetTypedPlacementType(Obj);
	const FText TypeText = GridEditorWidgetHelpers::GetGridEnumDisplayText(TypeEnum, static_cast<int64>(Type));
	const UGridWorldObjectDefinitionAsset* Definition = GetWorldObjectDefinition(CurrentEditorActor, Obj);
	const FGridWorldObjectInstance* WorldObjectInstance = GetWorldObjectInstance(CurrentEditorActor, Obj);
	const FText TitleText = Definition && !Definition->DisplayName.IsEmpty() ? Definition->DisplayName : TypeText;
	const bool bShowTransitionSection = WorldObjectInstance && (Type == EGridLevelObjectType::Pit || WorldObjectInstance->InstanceConfig.Transition.bIsTransition);
	return SNew(SBorder).Padding(8.f).BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))[SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 12.f, 0.f)[SNew(SBox).WidthOverride(88.f).HeightOverride(72.f)[
				SNew(SBorder).Padding(4.f).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))[SNew(STextBlock)
					.Text(GridEditorWidgetHelpers::GetGridObjectGlyph(Type)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 36)).Justification(ETextJustify::Center)]]]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(TitleText).Font(FCoreStyle::GetDefaultFontStyle("Regular", 20))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)[SNew(STextBlock)
					.Text(FText::Format(FText::FromString(TEXT("{0} {1}")), TypeText, GetHeaderPlacementText(*CurrentEditorActor->LevelAsset, Obj)))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.72f, 0.72f, 1.f)))]]]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[BuildGameObjectSection(Obj)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[BuildContextualComponentSection(Obj)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[bShowTransitionSection ? BuildTransitionDetailsSection(Obj) : SNullWidget::NullWidget]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[BuildAdvancedDebugSection(Obj)]];
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildGameObjectSection(FGuid Obj)
{
	const UEnum* PlacementKindEnum = StaticEnum<EGridObjectPlacementKind>();
	const UEnum* ObjectCategoryEnum = StaticEnum<EGridObjectCategory>();
	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset) return SNullWidget::NullWidget;
	const UGridLevelAsset& Level = *CurrentEditorActor->LevelAsset;
	const bool bIsMonsterSpawn = Level.GetTypedPlacementType(Obj) == EGridLevelObjectType::MonsterSpawn;
	const bool bHasActiveState = Level.FindWorldObjectInstanceById(Obj) || Level.FindLogicObjectInstanceById(Obj);
	const UGridWorldObjectDefinitionAsset* Definition = GetWorldObjectDefinition(CurrentEditorActor, Obj);
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);
	if (Definition && !bIsMonsterSpawn)
	{
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Placement Kind")),
			GridEditorWidgetHelpers::GetGridEnumDisplayText(PlacementKindEnum, static_cast<int64>(Definition->PlacementSurface)))];
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Palette Category")), GetNameText(Definition->Category))];
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Functional Category")),
			GridEditorWidgetHelpers::GetGridEnumDisplayText(ObjectCategoryEnum, static_cast<int64>(Definition->ObjectCategory)))];
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Runtime Interactable")), GetBoolText(Definition->bIsInteractable))];
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Runtime Readable")), GetBoolText(Definition->bIsReadable))];
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Runtime Light Source")), GetBoolText(Definition->bIsLightSource))];
	}
	Root->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 12.f, 0.f)[SNew(SCheckBox)
			.IsChecked(ReadPlacementValue<bool>(Level, Obj, [](const auto& Placement) { return Placement.bInitiallyEnabled; }) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
			{
				if (AGridLevelEditorActor* Editor = GetEditorActor())
				{
					Editor->SetSelectedObjectInitiallyEnabled(NewState == ECheckBoxState::Checked);
					RequestRefresh();
				}
			})[SNew(STextBlock).Text(FText::FromString(bIsMonsterSpawn ? TEXT("Present at Start") : TEXT("Enabled at Start")))]]
		+ SHorizontalBox::Slot().AutoWidth()[bHasActiveState ? StaticCastSharedRef<SWidget>(SNew(SCheckBox)
			.IsChecked(IsPlacementInitiallyActive(Level, Obj) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
			{
				if (AGridLevelEditorActor* Editor = GetEditorActor())
				{
					Editor->SetSelectedObjectInitiallyActive(NewState == ECheckBoxState::Checked);
					RequestRefresh();
				}
			})[SNew(STextBlock).Text(FText::FromString(TEXT("Active at Start")))]) : SNullWidget::NullWidget]];
	if (bIsMonsterSpawn)
	{
		Root->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[SNew(STextBlock)
			.Text(FText::FromString(TEXT("Present = the monster Actor exists when the level starts. Unchecked = absent until a Spawn command or encounter creates it.")))
			.AutoWrapText(true).ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f)))];
	}
	return GridEditorWidgetHelpers::BuildGridPanelSection(FText::FromString(bIsMonsterSpawn ? TEXT("Spawn Presence") : TEXT("Game Object")), Root);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildContextualComponentSection(FGuid Obj)
{
	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset) return SNullWidget::NullWidget;
	const UGridWorldObjectDefinitionAsset* Definition = GetWorldObjectDefinition(CurrentEditorActor, Obj);
	TSharedPtr<SWidget> PrimarySection;
	if (Definition && Definition->IsReadable())
	{
		PrimarySection = BuildReadableTextSection(Obj);
	}
	else
	{
		switch (CurrentEditorActor->LevelAsset->GetTypedPlacementType(Obj))
		{
			case EGridLevelObjectType::Door: PrimarySection = BuildDoorDetailsSection(Obj); break;
			case EGridLevelObjectType::Lever: PrimarySection = BuildLeverDetailsSection(Obj); break;
			case EGridLevelObjectType::Button: PrimarySection = BuildButtonDetailsSection(Obj); break;
			case EGridLevelObjectType::PressurePlate: PrimarySection = BuildPressurePlateDetailsSection(Obj); break;
			case EGridLevelObjectType::Pit: PrimarySection = BuildPitDetailsSection(Obj); break;
			case EGridLevelObjectType::Trigger: PrimarySection = BuildTriggerBehaviorSection(Obj); break;
			case EGridLevelObjectType::Receptacle: PrimarySection = BuildReceptacleBehaviorSection(Obj); break;
			case EGridLevelObjectType::Item: PrimarySection = BuildItemDefinitionSection(Obj); break;
			case EGridLevelObjectType::ItemSpawn: PrimarySection = BuildItemDefinitionSection(Obj); break;
			case EGridLevelObjectType::MonsterSpawn: PrimarySection = BuildMonsterSpawnSection(Obj); break;
			case EGridLevelObjectType::Teleporter: PrimarySection = BuildTeleporterDetailsSection(Obj); break;
			default:
				PrimarySection = GridEditorWidgetHelpers::BuildGridPanelSection(FText::FromString(TEXT("Component")),
					SNew(STextBlock).Text(FText::FromString(TEXT("No contextual component fields are exposed for this object type yet."))).AutoWrapText(true));
				break;
		}
	}
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[PrimarySection.ToSharedRef()];
	if (Definition && Definition->bIsLightSource)
	{
		Root->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)[BuildLightDetailsSection(*Definition)];
	}
	return Root;
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildAdvancedDebugSection(FGuid Obj)
{
	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset) return SNullWidget::NullWidget;
	const UGridLevelAsset& Level = *CurrentEditorActor->LevelAsset;
	const UGridWorldObjectDefinitionAsset* Definition = GetWorldObjectDefinition(CurrentEditorActor, Obj);
	const FGridWorldObjectInstance* WorldObjectInstance = GetWorldObjectInstance(CurrentEditorActor, Obj);
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("ObjectId")), FText::FromString(Obj.ToString()))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("WorldObjectDefinitionId")), FText::FromName(WorldObjectInstance ? WorldObjectInstance->WorldObjectDefinitionId : NAME_None))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(FText::FromString(TEXT("Tag")), SNew(SEditableTextBox).Text(FText::FromName(GetObjectTagValue(Level, Obj)))
			.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) { if (AGridLevelEditorActor* Editor = GetEditorActor()) { Editor->SetSelectedObjectTag(GetNameFromEditorText(NewText)); RequestRefresh(); } }))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[SNew(STextBlock).Text(FText::FromString(TEXT("Notes")))]
		+ SVerticalBox::Slot().AutoHeight()[SNew(SMultiLineEditableTextBox).Text(FText::FromString(ReadPlacementValue<FString>(Level, Obj, [](const auto& Placement) { return Placement.Notes; }))).AutoWrapText(true)
			.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type)
			{
				if (AGridLevelEditorActor* Editor = GetEditorActor())
				{
					Editor->SetSelectedObjectNotes(NewText.ToString());
					RequestRefresh();
				}
			})];
	if (Definition)
	{
		Root->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Runtime Actor Class")), GetClassNameText(Definition->RuntimeActorClass.Get()))];
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Item Actor Class")), GetClassNameText(Definition->ItemActorClass.Get()))];
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Static Part Mesh")), GetObjectNameText(Definition->StaticPart.Mesh.Get()))];
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Moving Part 0 Mesh")), GetObjectNameText(Definition->MovingParts.Part0.Mesh.Get()))];
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Moving Part 1 Mesh")), GetObjectNameText(Definition->MovingParts.Part1.Mesh.Get()))];
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Moving Part Count")), FText::AsNumber(Definition->GetDefinedMovingPartCount()))];
	}
	return GridEditorWidgetHelpers::BuildGridPanelSection(FText::FromString(TEXT("Advanced / Debug")), Root);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildDoorDetailsSection(FGuid ObjectId)
{
	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	const UGridWorldObjectDefinitionAsset* Definition = GetWorldObjectDefinition(CurrentEditorActor, ObjectId);
	if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset || !Definition) return SNullWidget::NullWidget;
	const FGridObjectBehaviorParams& Behavior = Definition->DefaultBehavior;
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
			FText::FromString(TEXT("Initial State")), GetInitialActiveStateText(*CurrentEditorActor->LevelAsset, ObjectId, TEXT("Open / Active"), TEXT("Closed / Inactive")))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
			FText::FromString(TEXT("Motion Source")), FText::FromString(TEXT("Definition > Moving Parts[].Motion")))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
			FText::FromString(TEXT("Blocks Movement (Generic Object)")),
			Definition ? GetBoolText(Definition->bBlocksMovement) : FText::FromString(TEXT("Runtime door blocking handled by door system")))];

	TSharedRef<SVerticalBox> ChainRoot = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Has Chain Mechanism")), GetBoolText(Behavior.DoorAnimation.bHasChainMechanism))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Chain Pull Distance")), FText::AsNumber(Behavior.DoorAnimation.ChainPullDistance))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Chain Pull Duration")), FText::AsNumber(Behavior.DoorAnimation.ChainPullDuration))];
	Root->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 3.f)[GridEditorWidgetHelpers::BuildGridPanelSection(FText::FromString(TEXT("Door Chain")), ChainRoot)];
	Root->AddSlot().AutoHeight().Padding(0.f, 1.f, 0.f, 3.f)[SNew(STextBlock)
		.Text(FText::FromString(TEXT("Door geometry and duration are authored once in the World Object Definition Moving Parts.")))
		.AutoWrapText(true).ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f)))];
	if (Definition)
	{
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Static Part")), GetBoolText(Definition->StaticPart.IsDefined()))];
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Moving Parts")), FText::AsNumber(Definition->GetDefinedMovingPartCount()))];
	}
	Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Supported Commands")), FText::FromString(TEXT("Open, Close, Toggle, Lock, Unlock")))];
	return GridEditorWidgetHelpers::BuildGridPanelSection(FText::FromString(TEXT("Door")), Root);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildLeverDetailsSection(FGuid ObjectId)
{
	const AGridLevelEditorActor* Editor = GetEditorActor();
	if (!Editor || !Editor->LevelAsset) return SNullWidget::NullWidget;
	return GridEditorWidgetHelpers::BuildGridPanelSection(FText::FromString(TEXT("Lever")), SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
			FText::FromString(TEXT("Initial State")), GetInitialActiveStateText(*Editor->LevelAsset, ObjectId, TEXT("Activated"), TEXT("Deactivated")))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
			FText::FromString(TEXT("Motion Source")), FText::FromString(TEXT("Definition > Moving Part[0].Motion")))]
		+ SVerticalBox::Slot().AutoHeight()[BuildExplicitConnectorSummary(FText::FromString(TEXT("Activated, Deactivated, Toggled")))]);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildButtonDetailsSection(FGuid ObjectId)
{
	const AGridLevelEditorActor* Editor = GetEditorActor();
	const UGridWorldObjectDefinitionAsset* Definition = GetWorldObjectDefinition(Editor, ObjectId);
	if (!Editor || !Editor->LevelAsset || !Definition) return SNullWidget::NullWidget;
	FString ButtonType = TEXT("Generic");
	const FString WorldObjectDefinitionIdText = Definition->DefinitionId.ToString();
	if (WorldObjectDefinitionIdText.Contains(TEXT("Button_Secret"), ESearchCase::IgnoreCase)) ButtonType = TEXT("Secret");
	else if (WorldObjectDefinitionIdText.Contains(TEXT("Button_Wall"), ESearchCase::IgnoreCase)) ButtonType = TEXT("Wall");
	else if (WorldObjectDefinitionIdText.Contains(TEXT("Button_Normal"), ESearchCase::IgnoreCase)) ButtonType = TEXT("Normal");
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Button Type")), FText::FromString(ButtonType))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Initial State")), GetInitialActiveStateText(*Editor->LevelAsset, ObjectId, TEXT("Pressed"), TEXT("Released")))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Motion Source")), FText::FromString(TEXT("Definition > Moving Part[0].Motion")))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Hold Time")), FText::AsNumber(Definition->DefaultBehavior.ButtonAnimation.ButtonHoldTime))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
			FText::FromString(TEXT("Emits")), FText::FromString(TEXT("Activated, Used")))];
	return GridEditorWidgetHelpers::BuildGridPanelSection(FText::FromString(TEXT("Button")), Root);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildPressurePlateDetailsSection(FGuid ObjectId)
{
	const AGridLevelEditorActor* Editor = GetEditorActor();
	const UGridWorldObjectDefinitionAsset* Definition = GetWorldObjectDefinition(Editor, ObjectId);
	if (!Editor || !Editor->LevelAsset || !Definition) return SNullWidget::NullWidget;
	const auto& Weight = Definition->DefaultBehavior.PressurePlateWeight;
	return GridEditorWidgetHelpers::BuildGridPanelSection(FText::FromString(TEXT("Pressure Plate / Floor Trigger")), SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Initial State")), GetInitialActiveStateText(*Editor->LevelAsset, ObjectId, TEXT("Activated"), TEXT("Deactivated")))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Motion Source")), FText::FromString(TEXT("Definition > Moving Part[0].Motion")))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Party Activates")), GetBoolText(Weight.bActivateWhenPartyPresent))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Use Item Weight")), GetBoolText(Weight.bUseItemWeight))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Required Item Weight")), FText::AsNumber(Weight.RequiredItemWeight))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Count Edge Items")), GetBoolText(Weight.bCountEdgeItems))]
		+ SVerticalBox::Slot().AutoHeight()[BuildExplicitConnectorSummary(FText::FromString(TEXT("Activated, Deactivated, Entered, Exited")))]
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock)
			.Text(FText::FromString(TEXT("Use explicit Activated and Deactivated connectors to control what happens on press and release."))).AutoWrapText(true)]);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildPitDetailsSection(FGuid ObjectId)
{
	const FGridWorldObjectInstance* WorldObjectInstance = GetWorldObjectInstance(GetEditorActor(), ObjectId);
	if (!WorldObjectInstance) return SNullWidget::NullWidget;
	const FGridPitBehaviorParams& Pit = WorldObjectInstance->InstanceConfig.Pit;
	return GridEditorWidgetHelpers::BuildGridPanelSection(FText::FromString(TEXT("Pit")), SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(FText::FromString(TEXT("Open at Start")), SNew(SCheckBox)
			.IsChecked(Pit.bInitiallyOpen ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged_Lambda([this, ObjectId](ECheckBoxState State)
			{
				EditWorldObjectConfig(ObjectId, [State](FGridWorldObjectInstanceConfig& Config) { Config.Pit.bInitiallyOpen = State == ECheckBoxState::Checked; });
			}))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(FText::FromString(TEXT("Use Same Cell Coordinates")), SNew(SCheckBox)
			.IsChecked(Pit.bUseSameCellCoordinates ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged_Lambda([this, ObjectId](ECheckBoxState State)
			{
				EditWorldObjectConfig(ObjectId, [State](FGridWorldObjectInstanceConfig& Config) { Config.Pit.bUseSameCellCoordinates = State == ECheckBoxState::Checked; });
			}))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Trapdoor Layout")), FText::FromString(TEXT("Definition > Moving Parts[0/1].Motion")))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[SNew(STextBlock)
			.Text(FText::FromString(TEXT("Trapdoor hinges, rotation angle and duration are authored once in the World Object Definition. The level instance stores only pit state and transition data.")))
			.AutoWrapText(true).ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f)))]);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildTeleporterDetailsSection(FGuid ObjectId)
{
	const FGridWorldObjectInstance* WorldObjectInstance = GetWorldObjectInstance(GetEditorActor(), ObjectId);
	if (!WorldObjectInstance) return SNullWidget::NullWidget;
	const FGridTeleporterBehaviorParams& Teleporter = WorldObjectInstance->InstanceConfig.Teleporter;
	auto BuildIntBehaviorRow = [this, ObjectId](const FText& Label, int32 CurrentValue, int32 MinValue, int32 MaxValue,
		TFunction<void(FGridWorldObjectInstanceConfig&, int32)> AssignValue) -> TSharedRef<SWidget>
	{
		return GridEditorWidgetHelpers::BuildGridPropertyRow(Label,
			SNew(SSpinBox<int32>).Value(CurrentValue).MinValue(MinValue).MaxValue(MaxValue).MinSliderValue(MinValue).MaxSliderValue(MaxValue).Delta(1)
			.OnValueCommitted_Lambda([this, ObjectId, AssignValue](int32 NewValue, ETextCommit::Type)
			{
				EditWorldObjectConfig(ObjectId, [AssignValue, NewValue](FGridWorldObjectInstanceConfig& Config) { AssignValue(Config, NewValue); });
			}));
	};
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[BuildIntBehaviorRow(FText::FromString(TEXT("Target Cell X")), Teleporter.TargetCellX, -1, 31,
			[](FGridWorldObjectInstanceConfig& Config, int32 NewValue){ Config.Teleporter.TargetCellX = NewValue; })]
		+ SVerticalBox::Slot().AutoHeight()[BuildIntBehaviorRow(FText::FromString(TEXT("Target Cell Y")), Teleporter.TargetCellY, -1, 31,
			[](FGridWorldObjectInstanceConfig& Config, int32 NewValue){ Config.Teleporter.TargetCellY = NewValue; })]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[SNew(STextBlock)
			.Text(FText::FromString(TEXT("Use -1 / -1 to mark an unset destination."))).AutoWrapText(true)
			.ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f)))];
	return GridEditorWidgetHelpers::BuildGridPanelSection(FText::FromString(TEXT("Teleporter")), Root);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildTransitionDetailsSection(FGuid ObjectId)
{
	const FGridWorldObjectInstance* WorldObjectInstance = GetWorldObjectInstance(GetEditorActor(), ObjectId);
	if (!WorldObjectInstance) return SNullWidget::NullWidget;
	const FGridWorldObjectInstance& Obj = *WorldObjectInstance;
	const FGridObjectTransitionParams& Transition = Obj.InstanceConfig.Transition;
	const bool bIsPit = Obj.Type == EGridLevelObjectType::Pit;
	const bool bPitUsesSameCellCoordinates = bIsPit && Obj.InstanceConfig.Pit.bUseSameCellCoordinates;
	const bool bTransitionFieldsEnabled = bIsPit || Transition.bIsTransition;
	auto BuildIntTransitionRow = [this, ObjectId, bTransitionFieldsEnabled, bPitUsesSameCellCoordinates](const FText& Label, int32 CurrentValue,
		TFunction<void(FGridObjectTransitionParams&, int32)> AssignValue) -> TSharedRef<SWidget>
	{
		return GridEditorWidgetHelpers::BuildGridPropertyRow(Label, SNew(SSpinBox<int32>).Value(CurrentValue).MinValue(0).MaxValue(31).MinSliderValue(0).MaxSliderValue(31).Delta(1)
			.IsEnabled(bTransitionFieldsEnabled && !bPitUsesSameCellCoordinates)
			.OnValueCommitted_Lambda([this, ObjectId, AssignValue](int32 NewValue, ETextCommit::Type)
			{
				EditWorldObjectConfig(ObjectId, [AssignValue, NewValue](FGridWorldObjectInstanceConfig& Config) { AssignValue(Config.Transition, NewValue); });
			}));
	};
	auto BuildFacingButton = [this, ObjectId, Transition, bTransitionFieldsEnabled](const TCHAR* Label, EGridEdge Facing) -> TSharedRef<SWidget>
	{
		const bool bSelected = Transition.TargetFacing == Facing;
		return SNew(SButton).Text(FText::FromString(Label)).IsEnabled(bTransitionFieldsEnabled)
			.ButtonColorAndOpacity(bSelected ? FLinearColor(0.32f, 0.46f, 0.72f, 1.f) : FLinearColor::White)
			.OnClicked_Lambda([this, ObjectId, Facing]()
			{
				EditWorldObjectConfig(ObjectId, [Facing](FGridWorldObjectInstanceConfig& Config) { Config.Transition.TargetFacing = Facing; });
				return FReply::Handled();
			});
	};
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[bIsPit ? StaticCastSharedRef<SWidget>(GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
			FText::FromString(TEXT("Transition Mode")), FText::FromString(TEXT("Intrinsic Pit Fall"))))
			: StaticCastSharedRef<SWidget>(SNew(SCheckBox).IsChecked(Transition.bIsTransition ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged_Lambda([this, ObjectId](ECheckBoxState State)
			{
				EditWorldObjectConfig(ObjectId, [State](FGridWorldObjectInstanceConfig& Config) { Config.Transition.bIsTransition = State == ECheckBoxState::Checked; });
			})[SNew(STextBlock).Text(FText::FromString(TEXT("Is Transition")))])]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(FText::FromString(TEXT("Target Level Id")), SNew(SEditableTextBox)
			.Text(GetNameText(Transition.TargetLevelId)).IsEnabled(bTransitionFieldsEnabled)
			.OnTextCommitted_Lambda([this, ObjectId](const FText& NewText, ETextCommit::Type)
			{
				EditWorldObjectConfig(ObjectId, [&NewText](FGridWorldObjectInstanceConfig& Config) { Config.Transition.TargetLevelId = GetNameFromEditorText(NewText); });
			}))]
		+ SVerticalBox::Slot().AutoHeight()[BuildIntTransitionRow(FText::FromString(TEXT("Target Cell X")), Transition.TargetCellX,
			[](FGridObjectTransitionParams& Params, int32 V){ Params.TargetCellX = V; })]
		+ SVerticalBox::Slot().AutoHeight()[BuildIntTransitionRow(FText::FromString(TEXT("Target Cell Y")), Transition.TargetCellY,
			[](FGridObjectTransitionParams& Params, int32 V){ Params.TargetCellY = V; })]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(FText::FromString(TEXT("Target Facing")), SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 2.f, 0.f)[BuildFacingButton(TEXT("North"), EGridEdge::North)]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)[BuildFacingButton(TEXT("East"), EGridEdge::East)]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)[BuildFacingButton(TEXT("South"), EGridEdge::South)]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f, 0.f, 0.f)[BuildFacingButton(TEXT("West"), EGridEdge::West)])]
		+ SVerticalBox::Slot().AutoHeight()[SNew(SCheckBox).IsEnabled(bTransitionFieldsEnabled && !bIsPit)
			.IsChecked(Transition.bRequireUseAction ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged_Lambda([this, ObjectId](ECheckBoxState State)
			{
				EditWorldObjectConfig(ObjectId, [State](FGridWorldObjectInstanceConfig& Config) { Config.Transition.bRequireUseAction = State == ECheckBoxState::Checked; });
			})[SNew(STextBlock).Text(FText::FromString(TEXT("Require Use Action")))]]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[SNew(STextBlock)
			.Text(bIsPit ? (bPitUsesSameCellCoordinates
				? FText::Format(FText::FromString(TEXT("Target Cell X/Y are ignored because Use Same Cell Coordinates is enabled. Effective requested landing cell: ({0},{1}). Target Level Id may still explicitly override the lower level.")), FText::AsNumber(Obj.CellX), FText::AsNumber(Obj.CellY))
				: FText::FromString(TEXT("Target Cell X/Y are explicit landing coordinates. Target Level Id may be None for the automatic lower level. If the requested landing cell is not walkable, runtime resolves the nearest usable floor cell.")))
				: FText::FromString(TEXT("Transition data is stored on this object and executed by the runtime.")))
			.AutoWrapText(true).ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f)))];
	return GridEditorWidgetHelpers::BuildGridPanelSection(FText::FromString(TEXT("Transition")), Root);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildLightDetailsSection(const UGridWorldObjectDefinitionAsset& Definition)
{
	return GridEditorWidgetHelpers::BuildGridPanelSection(FText::FromString(TEXT("Light")), SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Light Color")), FText::FromString(Definition.LightColor.ToString()))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Intensity")), FText::AsNumber(Definition.LightIntensity))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Radius")), FText::AsNumber(Definition.LightRadius))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Use Light Flicker (if supported)")), GetBoolText(Definition.bUseLightFlicker))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 1.f, 0.f, 0.f)[SNew(STextBlock)
			.Text(FText::FromString(TEXT("Actual flicker support depends on the runtime light component path."))).AutoWrapText(true)
			.ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f)))]);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildReadableTextSection(FGuid ObjectId)
{
	const FGridWorldObjectInstance* WorldObjectInstance = GetWorldObjectInstance(GetEditorActor(), ObjectId);
	if (!WorldObjectInstance) return SNullWidget::NullWidget;
	return GridEditorWidgetHelpers::BuildGridPanelSection(FText::FromString(TEXT("Readable Text")), SNew(SMultiLineEditableTextBox)
		.Text(WorldObjectInstance->ReadableTextOverride).AutoWrapText(true).HintText(FText::FromString(TEXT("Text displayed when the player reads this object.")))
		.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type)
		{
			if (AGridLevelEditorActor* Editor = GetEditorActor())
			{
				Editor->SetSelectedObjectReadableText(NewText);
				RequestRefresh();
			}
		}));
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildTriggerBehaviorSection(FGuid ObjectId)
{
	return SNew(SBorder).Padding(6.f).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))[SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)[SNew(STextBlock).Text(FText::FromString(TEXT("Trigger"))).Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)[SNew(STextBlock)
			.Text(FText::FromString(TEXT("Triggers emit explicit connector events. Add connectors for Activated or Deactivated to control enter and exit behavior."))).AutoWrapText(true)]
		+ SVerticalBox::Slot().AutoHeight()[BuildExplicitConnectorSummary(FText::FromString(TEXT("Activated, Deactivated")))]];
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildItemDefinitionSection(FGuid ObjectId)
{
	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset) return SNullWidget::NullWidget;
	const FGridLooseItemInstance* LooseItem = CurrentEditorActor->LevelAsset->FindLooseItemInstanceById(ObjectId);
	const FGridItemSpawnInstance* ItemSpawn = CurrentEditorActor->LevelAsset->FindItemSpawnInstanceById(ObjectId);
	if (!LooseItem && !ItemSpawn) return SNullWidget::NullWidget;
	UGridItemDefinitionAsset* Definition = LooseItem ? LooseItem->ItemDefinition.Get() : ItemSpawn->ItemDefinition.Get();
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)[SNew(STextBlock).Text(FText::FromString(TEXT("Item Definition"))).Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Placement")), FText::FromString(LooseItem ? TEXT("Item present in the level") : TEXT("Item generator")))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(FText::FromString(TEXT("ItemDefinitionAsset")), BuildItemDefinitionAssetPicker(Definition,
			[this](UGridItemDefinitionAsset* NewAsset)
			{
				if (AGridLevelEditorActor* Editor = GetEditorActor()) if (Editor->SetSelectedObjectItemDefinitionAsset(NewAsset)) RequestRefresh();
			}))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Item Id")), GetNameText(Definition ? Definition->ItemDefinitionId : NAME_None))];
	if (!Definition)
	{
		Root->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("Assign an Item Definition asset."))).AutoWrapText(true)];
	}
	if (LooseItem)
	{
		Root->AddSlot().AutoHeight().Padding(0.f, 10.f, 0.f, 4.f)[SNew(STextBlock).Text(FText::FromString(TEXT("Item Reading"))).Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))];
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(FText::FromString(TEXT("ReadableContentAsset")), BuildReadableContentAssetPicker(LooseItem->ReadableContentAsset,
			[this](UGridReadableContentAsset* NewAsset){ if (AGridLevelEditorActor* Editor = GetEditorActor()) if (Editor->SetSelectedObjectReadableContentAsset(NewAsset)) RequestRefresh(); }))];
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(FText::FromString(TEXT("ReadableContentId")), SNew(SEditableTextBox).Text(GetNameText(LooseItem->ReadableContentId))
			.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type){ if (AGridLevelEditorActor* Editor = GetEditorActor()){ Editor->SetSelectedObjectReadableContentId(GetNameFromEditorText(NewText)); RequestRefresh(); } }))];
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(FText::FromString(TEXT("ReadTitleOverride")), SNew(SEditableTextBox).Text(LooseItem->ReadTitleOverride)
			.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type){ if (AGridLevelEditorActor* Editor = GetEditorActor()){ Editor->SetSelectedObjectReadTitleOverride(NewText); RequestRefresh(); } }))];
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(FText::FromString(TEXT("ReadTextOverride")), SNew(SMultiLineEditableTextBox).Text(LooseItem->ReadTextOverride).AutoWrapText(true)
			.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type){ if (AGridLevelEditorActor* Editor = GetEditorActor()){ Editor->SetSelectedObjectReadTextOverride(NewText); RequestRefresh(); } }))];
	}
	return SNew(SBorder).Padding(6.f).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))[Root];
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildMonsterSpawnSection(FGuid ObjectId)
{
	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	const FGridMonsterSpawnInstance* MonsterSpawn = CurrentEditorActor && CurrentEditorActor->LevelAsset
		? CurrentEditorActor->LevelAsset->FindMonsterSpawnInstanceById(ObjectId) : nullptr;
	if (!MonsterSpawn) return SNullWidget::NullWidget;
	const FGridMonsterSpawnInstance& Obj = *MonsterSpawn;
	const UGridMonsterDefinitionAsset* Definition = Obj.MonsterDefinition;
	const bool bHasDefinitionAsset = Definition != nullptr;
	const UEnum* EdgeEnum = StaticEnum<EGridEdge>();
	const UEnum* StateEnum = StaticEnum<EGridMonsterState>();
	const UEnum* PatrolModeEnum = StaticEnum<EGridMonsterPatrolMode>();
	const UEnum* AIProfileEnum = StaticEnum<EGridMonsterAIProfile>();
	const bool bPatrolEditing = CurrentEditorActor && CurrentEditorActor->IsPatrolRouteEditModeActive();
	const int32 WaypointCount = Obj.PatrolWaypoints.Num();
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);
	auto AddHeading = [&Root](const TCHAR* Heading)
	{
		Root->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 4.f)[SNew(STextBlock).Text(FText::FromString(Heading)).Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))];
	};
	auto BuildInitialStateMenu = [this, StateEnum]() -> TSharedRef<SWidget>
	{
		TSharedRef<SVerticalBox> Menu = SNew(SVerticalBox);
		const EGridMonsterState States[] = { EGridMonsterState::Idle, EGridMonsterState::Dormant };
		for (const EGridMonsterState State : States)
		{
			Menu->AddSlot().AutoHeight()[SNew(SButton).Text(GridEditorWidgetHelpers::GetGridEnumDisplayText(StateEnum, static_cast<int64>(State)))
				.OnClicked_Lambda([this, State](){ if (AGridLevelEditorActor* Editor = GetEditorActor()){ Editor->SetSelectedObjectInitialMonsterState(State); RequestRefresh(); } return FReply::Handled(); })];
		}
		return Menu;
	};
	auto BuildPatrolModeMenu = [this, PatrolModeEnum, WaypointCount]() -> TSharedRef<SWidget>
	{
		TSharedRef<SVerticalBox> Menu = SNew(SVerticalBox);
		const EGridMonsterPatrolMode Modes[] = { EGridMonsterPatrolMode::None, EGridMonsterPatrolMode::Loop, EGridMonsterPatrolMode::PingPong };
		for (const EGridMonsterPatrolMode Mode : Modes)
		{
			Menu->AddSlot().AutoHeight()[SNew(SButton).IsEnabled(Mode == EGridMonsterPatrolMode::None || WaypointCount >= 2)
				.Text(GridEditorWidgetHelpers::GetGridEnumDisplayText(PatrolModeEnum, static_cast<int64>(Mode)))
				.OnClicked_Lambda([this, Mode](){ if (AGridLevelEditorActor* Editor = GetEditorActor()){ Editor->SetSelectedMonsterPatrolMode(Mode); RequestRefresh(); } return FReply::Handled(); })];
		}
		return Menu;
	};
	AddHeading(TEXT("Definition"));
	Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(FText::FromString(TEXT("Monster Definition")), BuildMonsterDefinitionAssetPicker(Obj.MonsterDefinition,
		[this](UGridMonsterDefinitionAsset* NewAsset){ if (AGridLevelEditorActor* Editor = GetEditorActor()) if (Editor->SetSelectedObjectMonsterDefinitionAsset(NewAsset)) RequestRefresh(); }))];
	Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Definition Id")), GetNameText(Definition ? Definition->MonsterId : NAME_None))];
	if (!bHasDefinitionAsset)
	{
		Root->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[SNew(STextBlock).Text(FText::FromString(TEXT("Error: runtime MonsterSpawn requires a Monster Definition asset."))).AutoWrapText(true).ColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.25f, 0.18f, 1.f)))];
	}
	AddHeading(TEXT("Spawn"));
	Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(FText::FromString(TEXT("Initial State")), SNew(SComboButton)
		.ButtonContent()[SNew(STextBlock).Text(GridEditorWidgetHelpers::GetGridEnumDisplayText(StateEnum, static_cast<int64>(Obj.InitialMonsterState)))]
		.MenuContent()[BuildInitialStateMenu()])];
	Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Initial Facing")), GridEditorWidgetHelpers::GetGridEnumDisplayText(EdgeEnum, static_cast<int64>(Obj.Facing)))];
	Root->AddSlot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)[SNew(STextBlock).Text(FText::FromString(TEXT("Use the North / East / South / West buttons below to change the initial facing."))).AutoWrapText(true).ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f)))];
	AddHeading(TEXT("Perception — from Monster Definition"));
	if (Definition)
	{
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Sight Range (cells)")), FText::AsNumber(Definition->SightRangeCells))];
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Hearing Range (cells)")), FText::AsNumber(Definition->HearingRangeCells))];
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Primary AI Profile")), GridEditorWidgetHelpers::GetGridEnumDisplayText(AIProfileEnum, static_cast<int64>(Definition->PrimaryAIProfile)))];
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Shares Aggro With Group")), GetBoolText(Definition->bSharesAggroWithGroup))];
		if (Definition->bSharesAggroWithGroup) Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Aggro Propagation Range")), FText::AsNumber(Definition->AggroPropagationRange))];
		Root->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[SNew(STextBlock).Text(FText::FromString(TEXT("Sight is directional. Hearing is omnidirectional but follows acoustic grid paths: walls and closed secret doors block it; normal doors transmit it. These values belong to the shared Monster Definition asset."))).AutoWrapText(true).ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f)))];
		if (Obj.InitialMonsterState == EGridMonsterState::Dormant && Definition->SightRangeCells <= 0 && Definition->HearingRangeCells <= 0)
		{
			Root->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[SNew(STextBlock).Text(FText::FromString(TEXT("Warning: this Dormant monster has no sight or hearing range and cannot wake from perception."))).AutoWrapText(true).ColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.55f, 0.18f, 1.f)))];
		}
	}
	else
	{
		Root->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("Assign a Monster Definition to inspect effective perception values."))).AutoWrapText(true).ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f)))];
	}
	AddHeading(TEXT("Patrol"));
	Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(FText::FromString(TEXT("Patrol Mode")), SNew(SComboButton)
		.ButtonContent()[SNew(STextBlock).Text(GridEditorWidgetHelpers::GetGridEnumDisplayText(PatrolModeEnum, static_cast<int64>(Obj.PatrolMode)))]
		.MenuContent()[BuildPatrolModeMenu()])];
	Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Waypoints")), FText::AsNumber(WaypointCount))];
	Root->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)[SNew(SButton).Text(FText::FromString(bPatrolEditing ? TEXT("Finish Patrol Editing") : TEXT("Edit Patrol Route")))
			.OnClicked_Lambda([this](){ if (AGridLevelEditorActor* Editor = GetEditorActor()){ Editor->ToggleSelectedMonsterPatrolRouteEditing(); RequestRefresh(); } return FReply::Handled(); })]
		+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).IsEnabled(WaypointCount > 0).Text(FText::FromString(TEXT("Clear Route")))
			.OnClicked_Lambda([this](){ if (AGridLevelEditorActor* Editor = GetEditorActor()){ Editor->ClearSelectedMonsterPatrolRoute(); RequestRefresh(); } return FReply::Handled(); })]];
	if (bPatrolEditing)
	{
		Root->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[SNew(STextBlock).Text(FText::FromString(TEXT("Patrol editing active: click grid cells to add/select waypoints. F changes facing, +/- changes wait, Delete removes, PageUp/PageDown reorders, P finishes."))).AutoWrapText(true).ColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.78f, 0.88f, 1.f)))];
		const int32 SelectedWaypointIndex = CurrentEditorActor ? CurrentEditorActor->SelectedPatrolWaypointIndex : INDEX_NONE;
		if (Obj.PatrolWaypoints.IsValidIndex(SelectedWaypointIndex))
		{
			const FGridMonsterPatrolWaypoint& Waypoint = Obj.PatrolWaypoints[SelectedWaypointIndex];
			Root->AddSlot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Selected Waypoint")),
				FText::Format(FText::FromString(TEXT("#{0}  Cell ({1},{2})  Facing {3}  Wait {4}s")), FText::AsNumber(SelectedWaypointIndex), FText::AsNumber(Waypoint.Cell.X), FText::AsNumber(Waypoint.Cell.Y),
					GridEditorWidgetHelpers::GetGridEnumDisplayText(EdgeEnum, static_cast<int64>(Waypoint.Facing)), FText::AsNumber(Waypoint.WaitSeconds)))];
		}
	}
	AddHeading(TEXT("Encounter — optional"));
	Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(FText::FromString(TEXT("Encounter Group")), SNew(SEditableTextBox).Text(GetNameText(Obj.EncounterGroupId)).MinDesiredWidth(160.f)
		.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type){ if (AGridLevelEditorActor* Editor = GetEditorActor()) if (Editor->SetSelectedObjectEncounterGroupId(GetNameFromEditorText(NewText))) RequestRefresh(); }))];
	if (!Obj.EncounterGroupId.IsNone())
	{
		Root->AddSlot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(FText::FromString(TEXT("Wave Index")), SNew(SSpinBox<int32>).Value(Obj.EncounterWaveIndex).MinValue(0)
			.OnValueCommitted_Lambda([this](int32 NewValue, ETextCommit::Type){ if (AGridLevelEditorActor* Editor = GetEditorActor()) if (Editor->SetSelectedObjectEncounterWaveIndex(NewValue)) RequestRefresh(); }))];
	}
	else
	{
		Root->AddSlot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)[SNew(STextBlock).Text(FText::FromString(TEXT("Leave None for an independent monster. Use the Connectors tab for Spawn/Despawn or StartEncounter logic."))).AutoWrapText(true).ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f)))];
	}
	return GridEditorWidgetHelpers::BuildGridPanelSection(FText::FromString(TEXT("Monster Spawn")), Root);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildReceptacleBehaviorSection(FGuid ObjectId)
{
	const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	const FGridWorldObjectInstance* WorldObjectInstance = GetWorldObjectInstance(CurrentEditorActor, ObjectId);
	const UGridWorldObjectDefinitionAsset* Definition = GetWorldObjectDefinition(CurrentEditorActor, ObjectId);
	if (!WorldObjectInstance || !Definition) return SNullWidget::NullWidget;
	const auto& Receptacle = Definition->DefaultBehavior.Receptacle;
	const auto& InitialContent = WorldObjectInstance->InstanceConfig.ReceptacleInitialContent;
	const UEnum* PlacementKindEnum = StaticEnum<EGridObjectPlacementKind>();
	const UEnum* VisualPlacementModeEnum = StaticEnum<EGridReceptacleVisualPlacementMode>();
	TSharedRef<SVerticalBox> AcceptedItemsList = SNew(SVerticalBox);
	for (const FGridReceptacleAcceptedItemConfig& AcceptedItem : Receptacle.AcceptedItems)
	{
		AcceptedItemsList->AddSlot().AutoHeight().Padding(0.f, 2.f)[SNew(STextBlock).Text(FText::FromString(GetNameSafe(AcceptedItem.ItemDefinition)))];
	}
	TSharedRef<SVerticalBox> InitialContentList = SNew(SVerticalBox);
	for (int32 InitialIndex = 0; InitialIndex < InitialContent.Num(); ++InitialIndex)
	{
		const FGridReceptacleInitialItemConfig& InitialItem = InitialContent[InitialIndex];
		InitialContentList->AddSlot().AutoHeight().Padding(0.f, 2.f)[SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)[BuildItemDefinitionAssetPicker(InitialItem.ItemDefinition,
				[this, ObjectId, InitialIndex](UGridItemDefinitionAsset* NewAsset){ EditWorldObjectConfig(ObjectId, [InitialIndex, NewAsset](FGridWorldObjectInstanceConfig& Config){ if (Config.ReceptacleInitialContent.IsValidIndex(InitialIndex)) Config.ReceptacleInitialContent[InitialIndex].ItemDefinition = NewAsset; }); })]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)[SNew(SSpinBox<int32>).Value(InitialItem.Quantity).MinValue(1).MinSliderValue(1).Delta(1).MinDesiredWidth(70.f)
				.OnValueCommitted_Lambda([this, ObjectId, InitialIndex](int32 NewValue, ETextCommit::Type){ EditWorldObjectConfig(ObjectId, [InitialIndex, NewValue](FGridWorldObjectInstanceConfig& Config){ if (Config.ReceptacleInitialContent.IsValidIndex(InitialIndex)) Config.ReceptacleInitialContent[InitialIndex].Quantity = FMath::Max(1, NewValue); }); })]
			+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("Remove"))).OnClicked_Lambda([this, ObjectId, InitialIndex](){ EditWorldObjectConfig(ObjectId, [InitialIndex](FGridWorldObjectInstanceConfig& Config){ if (Config.ReceptacleInitialContent.IsValidIndex(InitialIndex)) Config.ReceptacleInitialContent.RemoveAt(InitialIndex); }); return FReply::Handled(); })]];
	}
	InitialContentList->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[SNew(SButton).Text(FText::FromString(TEXT("Add Initial Item"))).OnClicked_Lambda([this, ObjectId](){ EditWorldObjectConfig(ObjectId, [](FGridWorldObjectInstanceConfig& Config){ Config.ReceptacleInitialContent.AddDefaulted(); }); return FReply::Handled(); })];
	return SNew(SBorder).Padding(6.f).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))[SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)[SNew(STextBlock).Text(FText::FromString(TEXT("Receptacle"))).Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))]
		+ SVerticalBox::Slot().AutoHeight()[Definition ? GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Interactable")), GetBoolText(Definition->bIsInteractable)) : SNullWidget::NullWidget]
		+ SVerticalBox::Slot().AutoHeight()[Definition ? GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Placement Kind")), GridEditorWidgetHelpers::GetGridEnumDisplayText(PlacementKindEnum, static_cast<int64>(Definition->PlacementSurface))) : SNullWidget::NullWidget]
		+ SVerticalBox::Slot().AutoHeight()[Definition && Definition->bIsLightSource ? GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Runtime Light Source")), GetBoolText(Definition->bIsLightSource)) : SNullWidget::NullWidget]
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("Acceptance and placement rules come from the shared definition."))).AutoWrapText(true)]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Accept Any Item")), GetBoolText(Receptacle.bAcceptAnyItem))]
		+ SVerticalBox::Slot().AutoHeight()[!Receptacle.bAcceptAnyItem ? GridEditorWidgetHelpers::BuildGridPropertyRow(FText::FromString(TEXT("Accepted Items")), AcceptedItemsList) : SNullWidget::NullWidget]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Max Contained Items")), FText::AsNumber(Receptacle.MaxContainedItems))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Visual Placement Mode")), GridEditorWidgetHelpers::GetGridEnumDisplayText(VisualPlacementModeEnum, static_cast<int64>(Receptacle.VisualPlacementMode)))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Simulate Physics When Placed")), GetBoolText(Receptacle.bSimulatePhysicsWhenPlaced))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Physical Placement Surface Offset")), FText::AsNumber(Receptacle.PhysicalPlacementSurfaceOffset))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Physical Rotation Pitch")), FText::AsNumber(Receptacle.PhysicalPlacementInitialRotationOffset.Pitch))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Physical Rotation Yaw")), FText::AsNumber(Receptacle.PhysicalPlacementInitialRotationOffset.Yaw))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Physical Rotation Roll")), FText::AsNumber(Receptacle.PhysicalPlacementInitialRotationOffset.Roll))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(FText::FromString(TEXT("Initial Content")), InitialContentList)]];
}

FReply SGridEditorObjectInspectorPanel::OnApplySelectedObjectClicked()
{
	if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor())
	{
		CurrentEditorActor->Modify();
		if (CurrentEditorActor->ApplyEditedSelectedObject()) RequestRefresh();
	}
	return FReply::Handled();
}

FReply SGridEditorObjectInspectorPanel::OnResetBehaviorFromDefinitionClicked()
{
	if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor())
	{
		CurrentEditorActor->Modify();
		if (CurrentEditorActor->ResetSelectedObjectBehaviorFromDefinition()) RequestRefresh();
	}
	return FReply::Handled();
}

FReply SGridEditorObjectInspectorPanel::OnMoveSelectedObjectToCurrentCellClicked()
{
	if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor())
	{
		CurrentEditorActor->MoveSelectedObjectToCurrentSelection();
		RequestRefresh();
	}
	return FReply::Handled();
}

FReply SGridEditorObjectInspectorPanel::OnFocusSelectedObjectClicked()
{
	if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor())
	{
		CurrentEditorActor->FocusSelectedObject();
		RequestRefresh();
	}
	return FReply::Handled();
}

FReply SGridEditorObjectInspectorPanel::OnSetSelectedObjectOrientationClicked(EGridEdge Orientation)
{
	if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor())
	{
		if (CurrentEditorActor->SetSelectedObjectOrientation(Orientation)) RequestRefresh();
	}
	return FReply::Handled();
}

#endif

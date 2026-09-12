#include "EditorTools/Widgets/SGridEditorPressurePlateInstancePanel.h"

#if WITH_EDITOR

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectBehavior.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"

#include "Styling/SlateColor.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FGridWorldObjectInstance* GetPressurePlateInstance(const AGridLevelEditorActor* Editor, FGuid ObjectId)
	{
		if (!Editor || !Editor->LevelAsset || Editor->LevelAsset->GetTypedPlacementType(ObjectId) != EGridLevelObjectType::PressurePlate)
		{
			return nullptr;
		}
		return Editor->LevelAsset->FindWorldObjectInstanceById(ObjectId);
	}

	const UGridWorldObjectDefinitionAsset* GetPressurePlateDefinition(
		const AGridLevelEditorActor* Editor,
		const FGridWorldObjectInstance* Instance)
	{
		return Editor && Instance ? Editor->FindWorldObjectDefinitionById(Instance->WorldObjectDefinitionId) : nullptr;
	}

	template <typename TEdit>
	bool EditPressurePlateConfig(AGridLevelEditorActor* Editor, FGuid ObjectId, TEdit&& Edit)
	{
		if (!Editor || !Editor->LevelAsset || Editor->LastSelectedObjectId != ObjectId)
		{
			return false;
		}

		FGridWorldObjectInstance* Instance = Editor->LevelAsset->FindWorldObjectInstanceById(ObjectId);
		if (!Instance || Editor->LevelAsset->GetTypedPlacementType(ObjectId) != EGridLevelObjectType::PressurePlate)
		{
			return false;
		}

		const UGridWorldObjectDefinitionAsset* Definition =
			Editor->FindWorldObjectDefinitionById(Instance->WorldObjectDefinitionId);
		if (!Definition)
		{
			return false;
		}

		Editor->LevelAsset->Modify();
		Edit(Instance->InstanceConfig);
		Editor->ObjectBehavior = GridObjectInstanceBehavior::Resolve(*Instance, Definition);
		Editor->LevelAsset->MarkPackageDirty();
		Editor->RebuildPreview();
		return true;
	}
}

void SGridEditorPressurePlateInstancePanel::Construct(const FArguments& InArgs)
{
	EditorActor = InArgs._EditorActor;
	OnGetEditorActor = InArgs._OnGetEditorActor;
	OnRequestRefresh = InArgs._OnRequestRefresh;
	ChildSlot[BuildContent()];
}

AGridLevelEditorActor* SGridEditorPressurePlateInstancePanel::GetEditorActor() const
{
	if (EditorActor.IsValid())
	{
		return EditorActor.Get();
	}
	return OnGetEditorActor.IsBound() ? OnGetEditorActor.Execute() : nullptr;
}

void SGridEditorPressurePlateInstancePanel::RequestRefresh() const
{
	if (OnRequestRefresh.IsBound())
	{
		OnRequestRefresh.Execute();
	}
}

TSharedRef<SWidget> SGridEditorPressurePlateInstancePanel::BuildContent()
{
	const AGridLevelEditorActor* Editor = GetEditorActor();
	const FGuid ObjectId = Editor ? Editor->LastSelectedObjectId : FGuid();
	const FGridWorldObjectInstance* Instance = GetPressurePlateInstance(Editor, ObjectId);
	const UGridWorldObjectDefinitionAsset* Definition = GetPressurePlateDefinition(Editor, Instance);
	if (!Instance || !Definition)
	{
		return SNullWidget::NullWidget;
	}

	const FGridObjectBehaviorParams EffectiveBehavior = GridObjectInstanceBehavior::Resolve(*Instance, Definition);
	const FGridPressurePlateWeightParams& EffectiveWeight = EffectiveBehavior.PressurePlateWeight;
	const bool bOverrideRules = Instance->InstanceConfig.InteractionOverrides.bOverridePressurePlateWeight;

	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(
			FText::FromString(TEXT("Pressed at Start")),
			SNew(SCheckBox)
				.IsChecked(Instance->bInitiallyActive ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda([this, ObjectId](ECheckBoxState State)
				{
					SetPressedAtStart(ObjectId, State == ECheckBoxState::Checked);
				}))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
			FText::FromString(TEXT("Initial State")),
			FText::FromString(Instance->bInitiallyActive ? TEXT("Pressed / Activated") : TEXT("Released / Deactivated")))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[GridEditorWidgetHelpers::BuildGridPropertyRow(
			FText::FromString(TEXT("Monster Activates")),
			SNew(SCheckBox)
				.IsEnabled(bOverrideRules)
				.IsChecked(EffectiveWeight.bActivateWhenMonsterPresent ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda([this, ObjectId](ECheckBoxState State)
				{
					SetMonsterActivates(ObjectId, State == ECheckBoxState::Checked);
				}))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)[
			SNew(STextBlock)
				.Text(FText::FromString(TEXT("Pressed at Start is the pressure-plate meaning of the generic Active at Start flag. Monster Activates is an instance activation rule; enable Override Activation Rules in the Pressure Plate section immediately above to edit it.")))
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f)))
		];

	return GridEditorWidgetHelpers::BuildGridPanelSection(
		FText::FromString(TEXT("Instance State")),
		Root);
}

void SGridEditorPressurePlateInstancePanel::SetPressedAtStart(FGuid ObjectId, bool bPressed)
{
	AGridLevelEditorActor* Editor = GetEditorActor();
	if (!Editor || Editor->LastSelectedObjectId != ObjectId ||
		!Editor->LevelAsset || Editor->LevelAsset->GetTypedPlacementType(ObjectId) != EGridLevelObjectType::PressurePlate)
	{
		return;
	}

	if (Editor->SetSelectedObjectInitiallyActive(bPressed))
	{
		RequestRefresh();
	}
}

void SGridEditorPressurePlateInstancePanel::SetActivationRulesOverrideEnabled(FGuid ObjectId, bool bEnabled)
{
	AGridLevelEditorActor* Editor = GetEditorActor();
	const FGridWorldObjectInstance* Instance = GetPressurePlateInstance(Editor, ObjectId);
	const UGridWorldObjectDefinitionAsset* Definition = GetPressurePlateDefinition(Editor, Instance);
	if (!Instance || !Definition)
	{
		return;
	}

	const FGridPressurePlateWeightParams DefaultWeight = Definition->DefaultBehavior.PressurePlateWeight;
	if (EditPressurePlateConfig(Editor, ObjectId,
		[bEnabled, DefaultWeight](FGridWorldObjectInstanceConfig& Config)
		{
			if (bEnabled && !Config.InteractionOverrides.bOverridePressurePlateWeight)
			{
				Config.InteractionOverrides.PressurePlateWeight = DefaultWeight;
			}
			Config.InteractionOverrides.bOverridePressurePlateWeight = bEnabled;
		}))
	{
		RequestRefresh();
	}
}

void SGridEditorPressurePlateInstancePanel::SetMonsterActivates(FGuid ObjectId, bool bMonsterActivates)
{
	if (EditPressurePlateConfig(GetEditorActor(), ObjectId,
		[bMonsterActivates](FGridWorldObjectInstanceConfig& Config)
		{
			if (!Config.InteractionOverrides.bOverridePressurePlateWeight)
			{
				return;
			}
			Config.InteractionOverrides.PressurePlateWeight.bActivateWhenMonsterPresent = bMonsterActivates;
		}))
	{
		RequestRefresh();
	}
}

#endif

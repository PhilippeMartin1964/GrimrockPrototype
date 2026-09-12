#include "EditorTools/Widgets/SGridEditorDoorMotionOverridePanel.h"

#if WITH_EDITOR

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"

#include "Styling/SlateColor.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const FGridWorldObjectInstance* GetDoorInstance(const AGridLevelEditorActor* Editor, FGuid ObjectId)
	{
		if (!Editor || !Editor->LevelAsset || Editor->LevelAsset->GetTypedPlacementType(ObjectId) != EGridLevelObjectType::Door)
		{
			return nullptr;
		}
		return Editor->LevelAsset->FindWorldObjectInstanceById(ObjectId);
	}

	const UGridWorldObjectDefinitionAsset* GetDoorDefinition(const AGridLevelEditorActor* Editor, const FGridWorldObjectInstance* Instance)
	{
		return Editor && Instance ? Editor->FindWorldObjectDefinitionById(Instance->WorldObjectDefinitionId) : nullptr;
	}

	const FGridWorldObjectMovingPart* GetDefinitionMovingPart(const UGridWorldObjectDefinitionAsset* Definition, int32 PartIndex)
	{
		if (!Definition)
		{
			return nullptr;
		}
		switch (PartIndex)
		{
			case 0: return &Definition->MovingParts.Part0;
			case 1: return &Definition->MovingParts.Part1;
			default: return nullptr;
		}
	}

	const FGridWorldObjectMovingPartInstanceOverride* FindMovingPartOverride(
		const FGridWorldObjectInstanceConfig& Config,
		int32 PartIndex)
	{
		return Config.MovingPartOverrides.FindByPredicate(
			[PartIndex](const FGridWorldObjectMovingPartInstanceOverride& Override)
			{
				return Override.PartIndex == PartIndex;
			});
	}

	FGridWorldObjectMovingPartInstanceOverride& FindOrAddMovingPartOverride(
		FGridWorldObjectInstanceConfig& Config,
		int32 PartIndex)
	{
		if (FGridWorldObjectMovingPartInstanceOverride* Existing = Config.MovingPartOverrides.FindByPredicate(
			[PartIndex](const FGridWorldObjectMovingPartInstanceOverride& Override)
			{
				return Override.PartIndex == PartIndex;
			}))
		{
			return *Existing;
		}

		FGridWorldObjectMovingPartInstanceOverride& Added = Config.MovingPartOverrides.AddDefaulted_GetRef();
		Added.PartIndex = PartIndex;
		return Added;
	}

	void RemoveMovingPartOverrideIfEmpty(FGridWorldObjectInstanceConfig& Config, int32 PartIndex)
	{
		Config.MovingPartOverrides.RemoveAll(
			[PartIndex](const FGridWorldObjectMovingPartInstanceOverride& Override)
			{
				return Override.PartIndex == PartIndex &&
					!Override.bOverrideLocalTransform &&
					!Override.bOverrideMotionAmount &&
					!Override.bOverrideMotionDuration;
			});
	}

	template <typename TEdit>
	bool EditDoorConfig(AGridLevelEditorActor* Editor, FGuid ObjectId, TEdit&& Edit)
	{
		if (!Editor || !Editor->LevelAsset || Editor->LastSelectedObjectId != ObjectId)
		{
			return false;
		}

		FGridWorldObjectInstance* Instance = Editor->LevelAsset->FindWorldObjectInstanceById(ObjectId);
		if (!Instance || Instance->Type != EGridLevelObjectType::Door)
		{
			return false;
		}

		const UGridWorldObjectDefinitionAsset* Definition = Editor->FindWorldObjectDefinitionById(Instance->WorldObjectDefinitionId);
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

	FText GetMotionTypeText(const FGridWorldObjectMotion& Motion)
	{
		const UEnum* MotionTypeEnum = StaticEnum<EGridWorldObjectMotionType>();
		const UEnum* AxisEnum = StaticEnum<EGridWorldObjectMotionAxis>();
		return FText::Format(
			FText::FromString(TEXT("{0} / {1}")),
			GridEditorWidgetHelpers::GetGridEnumDisplayText(MotionTypeEnum, static_cast<int64>(Motion.Type)),
			GridEditorWidgetHelpers::GetGridEnumDisplayText(AxisEnum, static_cast<int64>(Motion.Axis)));
	}
}

void SGridEditorDoorMotionOverridePanel::Construct(const FArguments& InArgs)
{
	EditorActor = InArgs._EditorActor;
	OnGetEditorActor = InArgs._OnGetEditorActor;
	OnRequestRefresh = InArgs._OnRequestRefresh;
	ChildSlot[BuildContent()];
}

AGridLevelEditorActor* SGridEditorDoorMotionOverridePanel::GetEditorActor() const
{
	if (EditorActor.IsValid())
	{
		return EditorActor.Get();
	}
	return OnGetEditorActor.IsBound() ? OnGetEditorActor.Execute() : nullptr;
}

void SGridEditorDoorMotionOverridePanel::RequestRefresh() const
{
	if (OnRequestRefresh.IsBound())
	{
		OnRequestRefresh.Execute();
	}
}

TSharedRef<SWidget> SGridEditorDoorMotionOverridePanel::BuildContent()
{
	const AGridLevelEditorActor* Editor = GetEditorActor();
	const FGuid ObjectId = Editor ? Editor->LastSelectedObjectId : FGuid();
	const FGridWorldObjectInstance* Instance = GetDoorInstance(Editor, ObjectId);
	const UGridWorldObjectDefinitionAsset* Definition = GetDoorDefinition(Editor, Instance);
	if (!Instance || !Definition)
	{
		return SNullWidget::NullWidget;
	}

	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 5.f)
		[
			SNew(STextBlock)
				.Text(FText::FromString(TEXT("Sparse level-instance overrides. Mesh, motion type, axis, pivot and reverse duration remain Definition-owned.")))
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f)))
		];

	bool bHasMovingPart = false;
	for (int32 PartIndex = 0; PartIndex < 2; ++PartIndex)
	{
		const FGridWorldObjectMovingPart* Part = GetDefinitionMovingPart(Definition, PartIndex);
		if (!Part || !Part->IsDefined())
		{
			continue;
		}

		bHasMovingPart = true;
		Root->AddSlot().AutoHeight().Padding(0.f, 2.f, 0.f, 4.f)
		[
			BuildMovingPartSection(ObjectId, PartIndex)
		];
	}

	if (!bHasMovingPart)
	{
		return SNullWidget::NullWidget;
	}

	return GridEditorWidgetHelpers::BuildGridPanelSection(
		FText::FromString(TEXT("Door Motion — Instance Overrides")),
		Root);
}

TSharedRef<SWidget> SGridEditorDoorMotionOverridePanel::BuildMovingPartSection(FGuid ObjectId, int32 PartIndex)
{
	const AGridLevelEditorActor* Editor = GetEditorActor();
	const FGridWorldObjectInstance* Instance = GetDoorInstance(Editor, ObjectId);
	const UGridWorldObjectDefinitionAsset* Definition = GetDoorDefinition(Editor, Instance);
	const FGridWorldObjectMovingPart* DefinitionPart = GetDefinitionMovingPart(Definition, PartIndex);
	if (!Instance || !DefinitionPart || !DefinitionPart->IsDefined())
	{
		return SNullWidget::NullWidget;
	}

	const FGridWorldObjectMovingPartInstanceOverride* Override = FindMovingPartOverride(Instance->InstanceConfig, PartIndex);
	const bool bOverrideAmount = Override && Override->bOverrideMotionAmount;
	const bool bOverrideDuration = Override && Override->bOverrideMotionDuration;
	const float EffectiveAmount = bOverrideAmount ? Override->MotionAmount : DefinitionPart->Motion.Amount;
	const float EffectiveDuration = bOverrideDuration ? Override->MotionDuration : DefinitionPart->Motion.Duration;
	const bool bTranslation = DefinitionPart->Motion.Type == EGridWorldObjectMotionType::Translation;
	const FText AmountLabel = FText::FromString(bTranslation ? TEXT("Travel (cm)") : TEXT("Angle (deg)"));
	const FText DefinitionAmountLabel = FText::FromString(bTranslation ? TEXT("Definition Travel") : TEXT("Definition Angle"));
	const float AmountDelta = bTranslation ? 5.0f : 1.0f;

	TSharedRef<SVerticalBox> PartRoot = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
			FText::FromString(TEXT("Motion")), GetMotionTypeText(DefinitionPart->Motion))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
			DefinitionAmountLabel, FText::AsNumber(DefinitionPart->Motion.Amount))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(
			FText::FromString(TEXT("Override Travel / Angle")),
			SNew(SCheckBox)
				.IsChecked(bOverrideAmount ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda([this, ObjectId, PartIndex, DefinitionAmount = DefinitionPart->Motion.Amount](ECheckBoxState State)
				{
					SetAmountOverrideEnabled(ObjectId, PartIndex, State == ECheckBoxState::Checked, DefinitionAmount);
				}))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(
			AmountLabel,
			SNew(SSpinBox<float>)
				.Value(EffectiveAmount)
				.Delta(AmountDelta)
				.MinDesiredWidth(90.f)
				.IsEnabled(bOverrideAmount)
				.OnValueCommitted_Lambda([this, ObjectId, PartIndex](float NewValue, ETextCommit::Type)
				{
					SetAmountOverrideValue(ObjectId, PartIndex, NewValue);
				}))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
			FText::FromString(TEXT("Travel / Angle Source")),
			FText::FromString(bOverrideAmount ? TEXT("Instance Override") : TEXT("Definition")))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
			FText::FromString(TEXT("Definition Forward Duration")), FText::AsNumber(DefinitionPart->Motion.Duration))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(
			FText::FromString(TEXT("Override Forward Duration")),
			SNew(SCheckBox)
				.IsChecked(bOverrideDuration ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda([this, ObjectId, PartIndex, DefinitionDuration = DefinitionPart->Motion.Duration](ECheckBoxState State)
				{
					SetDurationOverrideEnabled(ObjectId, PartIndex, State == ECheckBoxState::Checked, DefinitionDuration);
				}))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridPropertyRow(
			FText::FromString(TEXT("Forward Duration (s)")),
			SNew(SSpinBox<float>)
				.Value(EffectiveDuration)
				.MinValue(0.0f)
				.MinSliderValue(0.0f)
				.Delta(0.05f)
				.MinDesiredWidth(90.f)
				.IsEnabled(bOverrideDuration)
				.OnValueCommitted_Lambda([this, ObjectId, PartIndex](float NewValue, ETextCommit::Type)
				{
					SetDurationOverrideValue(ObjectId, PartIndex, NewValue);
				}))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
			FText::FromString(TEXT("Forward Duration Source")),
			FText::FromString(bOverrideDuration ? TEXT("Instance Override") : TEXT("Definition")))]
		+ SVerticalBox::Slot().AutoHeight()[GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
			FText::FromString(TEXT("Reverse Duration (Definition)")),
			FText::AsNumber(DefinitionPart->Motion.GetDuration(true)))];

	return GridEditorWidgetHelpers::BuildGridPanelSection(
		FText::Format(FText::FromString(TEXT("Moving Part {0}")), FText::AsNumber(PartIndex)),
		PartRoot);
}

void SGridEditorDoorMotionOverridePanel::SetAmountOverrideEnabled(
	FGuid ObjectId,
	int32 PartIndex,
	bool bEnabled,
	float DefinitionAmount)
{
	if (EditDoorConfig(GetEditorActor(), ObjectId,
		[PartIndex, bEnabled, DefinitionAmount](FGridWorldObjectInstanceConfig& Config)
		{
			if (bEnabled)
			{
				FGridWorldObjectMovingPartInstanceOverride& Override = FindOrAddMovingPartOverride(Config, PartIndex);
				if (!Override.bOverrideMotionAmount)
				{
					Override.MotionAmount = DefinitionAmount;
				}
				Override.bOverrideMotionAmount = true;
			}
			else if (FGridWorldObjectMovingPartInstanceOverride* Override = Config.MovingPartOverrides.FindByPredicate(
				[PartIndex](const FGridWorldObjectMovingPartInstanceOverride& Entry) { return Entry.PartIndex == PartIndex; }))
			{
				Override->bOverrideMotionAmount = false;
				RemoveMovingPartOverrideIfEmpty(Config, PartIndex);
			}
		}))
	{
		RequestRefresh();
	}
}

void SGridEditorDoorMotionOverridePanel::SetAmountOverrideValue(FGuid ObjectId, int32 PartIndex, float NewValue)
{
	if (EditDoorConfig(GetEditorActor(), ObjectId,
		[PartIndex, NewValue](FGridWorldObjectInstanceConfig& Config)
		{
			FGridWorldObjectMovingPartInstanceOverride& Override = FindOrAddMovingPartOverride(Config, PartIndex);
			Override.bOverrideMotionAmount = true;
			Override.MotionAmount = NewValue;
		}))
	{
		RequestRefresh();
	}
}

void SGridEditorDoorMotionOverridePanel::SetDurationOverrideEnabled(
	FGuid ObjectId,
	int32 PartIndex,
	bool bEnabled,
	float DefinitionDuration)
{
	if (EditDoorConfig(GetEditorActor(), ObjectId,
		[PartIndex, bEnabled, DefinitionDuration](FGridWorldObjectInstanceConfig& Config)
		{
			if (bEnabled)
			{
				FGridWorldObjectMovingPartInstanceOverride& Override = FindOrAddMovingPartOverride(Config, PartIndex);
				if (!Override.bOverrideMotionDuration)
				{
					Override.MotionDuration = FMath::Max(0.0f, DefinitionDuration);
				}
				Override.bOverrideMotionDuration = true;
			}
			else if (FGridWorldObjectMovingPartInstanceOverride* Override = Config.MovingPartOverrides.FindByPredicate(
				[PartIndex](const FGridWorldObjectMovingPartInstanceOverride& Entry) { return Entry.PartIndex == PartIndex; }))
			{
				Override->bOverrideMotionDuration = false;
				RemoveMovingPartOverrideIfEmpty(Config, PartIndex);
			}
		}))
	{
		RequestRefresh();
	}
}

void SGridEditorDoorMotionOverridePanel::SetDurationOverrideValue(FGuid ObjectId, int32 PartIndex, float NewValue)
{
	if (EditDoorConfig(GetEditorActor(), ObjectId,
		[PartIndex, NewValue](FGridWorldObjectInstanceConfig& Config)
		{
			FGridWorldObjectMovingPartInstanceOverride& Override = FindOrAddMovingPartOverride(Config, PartIndex);
			Override.bOverrideMotionDuration = true;
			Override.MotionDuration = FMath::Max(0.0f, NewValue);
		}))
	{
		RequestRefresh();
	}
}

#endif

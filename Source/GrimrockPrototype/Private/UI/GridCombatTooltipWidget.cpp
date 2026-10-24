#include "UI/GridCombatTooltipWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Runtime/Combat/GridCombatEventFeedFormatter.h"
#include "UI/UILocalizationText.h"

namespace
{
	void SetIfPresent(UTextBlock* Target, const FText& Text)
	{
		if (Target)
		{
			Target->SetText(Text);
		}
	}

	FText SafeName(const FText& Text, const FText& Fallback)
	{
		return Text.IsEmpty() ? Fallback : Text;
	}

	FText ArmorSourceName(const FGridArmorAbsorptionSourceTrace& Armor)
	{
		if (!Armor.DisplayName.IsEmpty())
		{
			return Armor.DisplayName;
		}
		return FUILocalizationText::Get(Armor.bEquipment
			? TEXT("Combat.Tooltip.Fallback.Armor")
			: TEXT("Combat.Tooltip.Armor.Character"));
	}
}

void UGridCombatTooltipWidget::InitializeCombatTooltip(const FGridHudEventEntry& Entry)
{
	CombatEntry = Entry;
	RefreshTooltip();
}

void UGridCombatTooltipWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshTooltip();
}

int32 UGridCombatTooltipWidget::CountAbsorbingSources(const FGridCombatDamageTrace& Damage)
{
	int32 Count = 0;
	if (Damage.bResolved)
	{
		for (const FGridArmorAbsorptionSourceTrace& Source : Damage.PhysicalArmorSources)
		{
			if (Source.Absorbed > 0)
			{
				++Count;
			}
		}
	}
	return Count;
}

void UGridCombatTooltipWidget::RefreshTooltip()
{
	if (!CombatEntry.bHasCombatTooltipTrace || !CombatEntry.CombatTooltipTrace.bValid)
	{
		return;
	}

	const FGridCombatResolutionTrace& Trace = CombatEntry.CombatTooltipTrace;
	const FGridCombatDamageTrace& Damage = Trace.Damage;
	SetIfPresent(Text_Attacker, SafeName(CombatEntry.CombatAttacker,
		FUILocalizationText::Get(TEXT("Combat.Tooltip.Label.Attacker"))));
	SetIfPresent(Text_Defender, SafeName(CombatEntry.CombatDefender,
		FUILocalizationText::Get(TEXT("Combat.Tooltip.Label.Defender"))));

	const FGridCombatAttackTrace& Attack = Trace.Attack;
	FFormatNamedArguments AttackArgs;
	AttackArgs.Add(TEXT("Roll"), FGridCombatEventFeedFormatter::FormatRollAsDice(Attack.Roll));
	AttackArgs.Add(TEXT("Total"), FText::AsNumber(Attack.FinalValue));
	SetIfPresent(Text_Attack, FText::Format(
		FUILocalizationText::Get(TEXT("Combat.Tooltip.Value.AttackRoll")), AttackArgs));

	FFormatNamedArguments DefenseArgs;
	DefenseArgs.Add(TEXT("Base"), FText::AsNumber(Trace.Defense.BaseValue));
	DefenseArgs.Add(TEXT("Evasion"), FText::AsNumber(Trace.Defense.EvasionModifier));
	DefenseArgs.Add(TEXT("Total"), FText::AsNumber(Trace.Defense.FinalValue));
	SetIfPresent(Text_Defense, FText::Format(
		FUILocalizationText::Get(TEXT("Combat.Tooltip.Value.DefenseRoll")), DefenseArgs));

	FFormatNamedArguments DamageArgs;
	DamageArgs.Add(TEXT("Damage"), FText::AsNumber(Damage.DamageAfterResistance));
	SetIfPresent(Text_Damage, Damage.bResolved
		? FText::Format(FUILocalizationText::Get(TEXT("Combat.Tooltip.Value.DamageResolved")), DamageArgs)
		: FUILocalizationText::Get(TEXT("Combat.Tooltip.Value.NoDamage")));

	FFormatNamedArguments HealthArgs;
	HealthArgs.Add(TEXT("Before"), FText::AsNumber(Damage.HealthBefore));
	HealthArgs.Add(TEXT("After"), FText::AsNumber(Damage.HealthAfter));
	HealthArgs.Add(TEXT("Lost"), FText::AsNumber(Damage.HealthDamage));
	SetIfPresent(Text_Health, Damage.bResolved
		? FText::Format(FUILocalizationText::Get(TEXT("Combat.Tooltip.Value.HealthAfterAttack")), HealthArgs)
		: FUILocalizationText::Get(TEXT("Combat.Tooltip.Value.NoHealthChange")));

	TArray<FText> ExhaustedNames;
	TArray<FText> AlreadyDepletedNames;
	const bool bShowAbsorption = CountAbsorbingSources(Damage) > 0
		&& VerticalBox_ArmorRows && WidgetTree;

	// The legacy UMG header and all related padding are hidden together when
	// the hit has no actual absorption. The new GridPanel owns BOTH headers and
	// values, ensuring that there is one shared width per column.
	if (WidgetTree)
	{
		const ESlateVisibility SectionVisibility = bShowAbsorption
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		for (const FName WidgetName : { FName(TEXT("Text_SectionAbsorption")),
			FName(TEXT("Spacer_AttackToArmor")),
			FName(TEXT("Spacer_ArmorToConsequences")) })
		{
			if (UWidget* SectionWidget = WidgetTree->FindWidget(WidgetName))
			{
				SectionWidget->SetVisibility(SectionVisibility);
			}
		}
		if (UWidget* LegacyHeader = WidgetTree->FindWidget(FName(TEXT("HorizontalBox_ArmorHeader"))))
		{
			LegacyHeader->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	const FName HeaderWidgetNames[4] = {
		FName(TEXT("Text_HeaderSource")), FName(TEXT("Text_HeaderBefore")),
		FName(TEXT("Text_HeaderAbsorbed")), FName(TEXT("Text_HeaderAfter"))
	};
	const TCHAR* HeaderKeys[4] = {
		TEXT("Combat.Tooltip.Column.Source"), TEXT("Combat.Tooltip.Column.Before"),
		TEXT("Combat.Tooltip.Column.Absorbed"), TEXT("Combat.Tooltip.Column.After")
	};
	const UTextBlock* HeaderTemplates[4] = { nullptr, nullptr, nullptr, nullptr };
	if (WidgetTree)
	{
		for (int32 Column = 0; Column < 4; ++Column)
		{
			HeaderTemplates[Column] = Cast<UTextBlock>(WidgetTree->FindWidget(HeaderWidgetNames[Column]));
		}
	}

	if (VerticalBox_ArmorRows)
	{
		VerticalBox_ArmorRows->ClearChildren();
		VerticalBox_ArmorRows->SetVisibility(bShowAbsorption
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	UGridPanel* ArmorGrid = bShowAbsorption
		? WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass()) : nullptr;
	if (ArmorGrid)
	{
		ArmorGrid->SetColumnFill(0, 2.5f);
		for (int32 Column = 1; Column < 4; ++Column)
		{
			ArmorGrid->SetColumnFill(Column, 1.0f);
		}
		if (UVerticalBoxSlot* GridPanelSlot = VerticalBox_ArmorRows->AddChildToVerticalBox(ArmorGrid))
		{
			GridPanelSlot->SetHorizontalAlignment(HAlign_Fill);
		}
	}

	// Keep all four columns in the same UGridPanel across ALL data rows.
	const auto AddGridCell = [this, ArmorGrid, &HeaderTemplates](
		int32 RowIndex, int32 Column, const FText& Value, const FLinearColor& Color,
		bool bHeader, bool bExhausted)
	{
		if (!ArmorGrid || !WidgetTree)
		{
			return;
		}
		UTextBlock* Cell = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (!Cell)
		{
			return;
		}
		Cell->SetText(Value);
		if (HeaderTemplates[Column])
		{
			Cell->SetFont(HeaderTemplates[Column]->GetFont());
		}
		Cell->SetColorAndOpacity(bHeader && HeaderTemplates[Column]
			? HeaderTemplates[Column]->GetColorAndOpacity()
			: FSlateColor(bExhausted ? WarningTextColor : Color));
		Cell->SetJustification(Column == 0 ? ETextJustify::Left : ETextJustify::Right);
		Cell->SetAutoWrapText(Column == 0);
		if (UGridSlot* GridCellSlot = ArmorGrid->AddChildToGrid(Cell, RowIndex, Column))
		{
			GridCellSlot->SetHorizontalAlignment(HAlign_Fill);
			GridCellSlot->SetVerticalAlignment(VAlign_Center);
			GridCellSlot->SetPadding(FMargin(Column == 0 ? 0.f : 8.f, bHeader ? 2.f : 3.f,
				Column == 0 ? 8.f : 0.f, 3.f));
		}
	};

	if (ArmorGrid)
	{
		for (int32 Column = 0; Column < 4; ++Column)
		{
			const FText Label = HeaderTemplates[Column]
				? HeaderTemplates[Column]->GetText() : FUILocalizationText::Get(HeaderKeys[Column]);
			AddGridCell(0, Column, Label, NormalTextColor, true, false);
		}
	}

	int32 DisplayRowIndex = 1;
	for (const FGridArmorAbsorptionSourceTrace& Armor : Damage.PhysicalArmorSources)
	{
		const FText SourceName = ArmorSourceName(Armor);
		if (Armor.bEquipment && Armor.bExhaustedByHit)
		{
			ExhaustedNames.Add(SourceName);
		}
		else if (Armor.bEquipment && Armor.Before == 0)
		{
			AlreadyDepletedNames.Add(SourceName);
		}
		// A source with 0 absorbed damage is not a visual data row.
		// If every source is zero the whole ABSORPTION section is collapsed.
		if (!ArmorGrid || Armor.Absorbed <= 0)
		{
			continue;
		}
		const FText Values[4] = {
			SourceName, FText::AsNumber(Armor.Before),
			FText::AsNumber(Armor.Absorbed), FText::AsNumber(Armor.After)
		};
		for (int32 Column = 0; Column < 4; ++Column)
		{
			AddGridCell(DisplayRowIndex, Column, Values[Column],
				Column == 2 ? ArmorTextColor : NormalTextColor, false, Armor.bExhaustedByHit);
		}
		++DisplayRowIndex;
	}

	if (Text_Status)
	{
		FText Status;
		if (!ExhaustedNames.IsEmpty())
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Name"), FText::Join(FText::FromString(TEXT(", ")), ExhaustedNames));
			Status = FText::Format(FUILocalizationText::Get(TEXT("Combat.Tooltip.Status.DepletedNamed")), Args);
		}
		else if (!AlreadyDepletedNames.IsEmpty())
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Name"), FText::Join(FText::FromString(TEXT(", ")), AlreadyDepletedNames));
			Status = FText::Format(FUILocalizationText::Get(TEXT("Combat.Tooltip.Status.AlreadyDepletedNamed")), Args);
		}
		Text_Status->SetText(Status);
		Text_Status->SetVisibility(Status.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		Text_Status->SetColorAndOpacity(FSlateColor(WarningTextColor));
	}
}

#include "UI/GridCombatTooltipWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
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
	if (VerticalBox_ArmorRows)
	{
		VerticalBox_ArmorRows->ClearChildren();
	}

	// Dynamic TextBlocks must inherit an authored, valid font. The former
	// uninitialized TableFont caused missing-glyph boxes in the armor table.
	// Text_HeaderSource already exists as a static ST_UI header in the WBP.
	const UTextBlock* HeaderFontSource = WidgetTree
		? Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("Text_HeaderSource"))))
		: nullptr;

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
		if (!VerticalBox_ArmorRows || !WidgetTree)
		{
			continue;
		}

		UHorizontalBox* TableRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (!TableRow)
		{
			continue;
		}
		const FText Cells[4] = {
			SourceName,
			FText::AsNumber(Armor.Before),
			FText::AsNumber(Armor.Absorbed),
			FText::AsNumber(Armor.After)
		};
		for (int32 Column = 0; Column < 4; ++Column)
		{
			UTextBlock* Cell = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			if (!Cell)
			{
				continue;
			}
			Cell->SetText(Cells[Column]);
			Cell->SetColorAndOpacity(FSlateColor(Armor.bExhaustedByHit ? WarningTextColor :
				(Column == 2 ? ArmorTextColor : NormalTextColor)));
			if (HeaderFontSource)
			{
				Cell->SetFont(HeaderFontSource->GetFont());
			}
			Cell->SetAutoWrapText(Column == 0);
			if (UHorizontalBoxSlot* CellLayoutSlot = TableRow->AddChildToHorizontalBox(Cell))
			{
				FSlateChildSize ColumnSize(ESlateSizeRule::Fill);
				ColumnSize.Value = Column == 0 ? 2.5f : 1.0f;
				CellLayoutSlot->SetSize(ColumnSize);
				CellLayoutSlot->SetPadding(FMargin(Column == 0 ? 0.f : 6.f, 2.f));
				CellLayoutSlot->SetHorizontalAlignment(Column == 0 ? HAlign_Left : HAlign_Right);
			}
		}
		VerticalBox_ArmorRows->AddChild(TableRow);
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

#include "UI/GridCombatArmorRowWidget.h"

#include "Components/TextBlock.h"
#include "UI/UILocalizationText.h"

bool UGridCombatArmorRowWidget::SetArmorData(const FGridArmorAbsorptionSourceTrace& Source)
{
	// An incomplete WBP should fail gracefully, allowing the event feed
	// to fall back to its existing text tooltip.
	if (!Text_Source || !Text_Before || !Text_Absorbed || !Text_After)
	{
		return false;
	}

	const FText Name = !Source.DisplayName.IsEmpty()
		? Source.DisplayName
		: FUILocalizationText::Get(Source.bEquipment
			? TEXT("Combat.Tooltip.Fallback.Armor")
			: TEXT("Combat.Tooltip.Armor.Character"));
	Text_Source->SetText(Name);
	Text_Before->SetText(FText::AsNumber(Source.Before));
	Text_Absorbed->SetText(FText::AsNumber(Source.Absorbed));
	Text_After->SetText(FText::AsNumber(Source.After));
	return true;
}

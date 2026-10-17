#include "UI/RPGTalentNodeStyleSettings.h"

bool URPGTalentNodeStyleSettings::IsValidDefinition() const
{
	return AcquiredStyle.IsValid() &&
		AvailableStyle.IsValid() &&
		LockedLevelStyle.IsValid() &&
		LockedPrerequisiteStyle.IsValid() &&
		LockedPointsStyle.IsValid() &&
		LockedExclusiveStyle.IsValid();
}

bool URPGTalentNodeStyleSettings::TryGetStyle(
	EGridTalentNodeState State,
	FRPGTalentNodeStateVisualStyle& OutStyle) const
{
	OutStyle = FRPGTalentNodeStateVisualStyle();

	const FRPGTalentNodeStateVisualStyle* Style = nullptr;
	switch (State)
	{
		case EGridTalentNodeState::Acquired:
			Style = &AcquiredStyle;
			break;
		case EGridTalentNodeState::Available:
			Style = &AvailableStyle;
			break;
		case EGridTalentNodeState::LockedLevel:
			Style = &LockedLevelStyle;
			break;
		case EGridTalentNodeState::LockedPrerequisite:
			Style = &LockedPrerequisiteStyle;
			break;
		case EGridTalentNodeState::LockedPoints:
			Style = &LockedPointsStyle;
			break;
		case EGridTalentNodeState::LockedExclusive:
			Style = &LockedExclusiveStyle;
			break;
		default:
			return false;
	}

	if (!Style || !Style->IsValid())
	{
		return false;
	}

	OutStyle = *Style;
	return true;
}

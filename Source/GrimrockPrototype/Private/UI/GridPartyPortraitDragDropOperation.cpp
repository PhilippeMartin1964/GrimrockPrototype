#include "UI/GridPartyPortraitDragDropOperation.h"

void UGridPartyPortraitDragDropOperation::InitializeFromPortrait(FGuid InSourceCharacterId)
{
	SourceCharacterId = InSourceCharacterId;
}

bool UGridPartyPortraitDragDropOperation::HasValidSource() const
{
	return SourceCharacterId.IsValid();
}

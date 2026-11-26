#include "Runtime/Inspection/GridItemInspectionReadModel.h"

#include "../GridItemInteractionReadModel.h"

bool FGridItemInspectionReadModel::ResolveDefault(const UGridItemSubsystem* Registry, FName ItemId, FGridItemInspectionView& Out)
{
	Out = FGridItemInspectionView();
	int32 Level = INDEX_NONE;
	if (!IsValid(Registry) || !Registry->TryGetDefaultItemLevel(ItemId, Level))
	{
		if (IsValid(Registry) && Registry->IsConfigured())
		{
			Out.Item.Error = EGridItemResolveError::UnknownItemId;
		}
		return false;
	}
	return ResolveExact(Registry, ItemId, Level, Out);
}

bool FGridItemInspectionReadModel::ResolveExact(const UGridItemSubsystem* Registry, FName ItemId, int32 ItemLevel, FGridItemInspectionView& Out)
{
	Out = FGridItemInspectionView();
	if (!IsValid(Registry) || !Registry->ResolveItem(ItemId, ItemLevel, Out.Item))
	{
		return false;
	}
	Out.bCanRead = Out.Item.bReadable || !Out.Item.ReadText.IsEmpty();
	return Out.IsValid();
}

bool FGridItemInspectionReadModel::ResolvePhysical(const UGridItemSubsystem* Registry, const FGridItemInstance& Instance, FGridItemInspectionView& Out)
{
	Out = FGridItemInspectionView();
	if (!Instance.IsValid())
	{
		Out.bInvalidPhysicalInstance = true;
		return false;
	}
	if (!IsValid(Registry) || !Registry->ResolvePhysicalItem(Instance, Out.Item))
	{
		return false;
	}
	Out.bPhysicalInstance = true;
	Out.Quantity = Instance.Quantity;
	Out.bItemLightsEnabled = Instance.bLightsEnabled;
	const FGridItemInteractionReadView Interaction = FGridItemInteractionReadModel::Resolve(Registry, Instance);
	Out.bCanRead = Interaction.bCanRead;
	if (Out.HasProtection())
	{
		Out.bHasPhysicalReserve = Registry->ResolveArmorReserveForInstance(Instance, Out.PhysicalReserve);
	}
	return true;
}

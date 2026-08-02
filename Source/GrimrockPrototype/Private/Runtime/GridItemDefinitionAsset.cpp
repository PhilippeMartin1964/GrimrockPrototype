#include "Runtime/GridItemDefinitionAsset.h"

#include "Engine/StaticMesh.h"

bool UGridItemDefinitionAsset::IsValidDefinition () const
{
    if (ItemDefinitionId.IsNone ())
    {
        return false;
    }

    if (!HasValidCombatActions ())
    {
        return false;
    }

    if (!bProvidesAttack)
    {
        return true;
    }

    return HasValidOffensiveProfile () &&
        (CompatibleEquipmentSlots.Contains (EGridEquipmentSlot::MainHand) ||
            CompatibleEquipmentSlots.Contains (EGridEquipmentSlot::OffHand));
}

bool UGridItemDefinitionAsset::CanEquipToSlot (EGridEquipmentSlot Slot) const
{
    return Slot != EGridEquipmentSlot::None && CompatibleEquipmentSlots.Contains (Slot);
}

bool UGridItemDefinitionAsset::HasValidOffensiveProfile () const
{
    return bProvidesAttack && OffensiveProfile.IsValid ();
}

bool UGridItemDefinitionAsset::CanProvideAttackFromSlot (
    EGridEquipmentSlot Slot) const
{
    if ((Slot != EGridEquipmentSlot::MainHand &&
            Slot != EGridEquipmentSlot::OffHand) ||
        !CompatibleEquipmentSlots.Contains (Slot))
    {
        return false;
    }
    if (!CombatActions.IsEmpty ())
    {
        return CombatActions.ContainsByPredicate (
            [] (const FGridCombatActionDefinition& Action)
            {
                return Action.IsValid () &&
                    Action.SourcePolicy ==
                        EGridCombatActionSourcePolicy::Equipment &&
                    Action.ResolutionProfile ==
                        EGridCombatActionResolutionProfile::Attack;
            });
    }
    return HasValidOffensiveProfile ();
}

bool UGridItemDefinitionAsset::HasValidCombatActions () const
{
    TSet<FName> ActionIds;
    for (const FGridCombatActionDefinition& Action : CombatActions)
    {
        if (!Action.IsValid () ||
            Action.SourcePolicy !=
                EGridCombatActionSourcePolicy::Equipment ||
            ActionIds.Contains (Action.ActionId))
        {
            return false;
        }
        ActionIds.Add (Action.ActionId);
    }
    return true;
}

bool UGridItemDefinitionAsset::HasValidPlayerAttackPresentation () const
{
    return bProvidesAttackPresentation &&
        PlayerAttackPresentationProfile.IsValid () &&
        (PlayerAttackPresentationProfile.MotionStyle !=
                EGridPlayerAttackMotionStyle::Throw ||
            bThrowable);
}

UStaticMesh* UGridItemDefinitionAsset::LoadHeldMesh () const
{
    if (!EquippedMesh.IsNull ())
    {
        if (UStaticMesh* Mesh =
            EquippedMesh.LoadSynchronous ())
        {
            return Mesh;
        }
    }
    return WorldMesh.LoadSynchronous ();
}

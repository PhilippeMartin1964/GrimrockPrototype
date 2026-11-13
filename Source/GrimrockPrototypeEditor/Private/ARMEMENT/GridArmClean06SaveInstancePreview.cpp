#include "ARMEMENT/GridArmClean06SaveInstancePreview.h"

bool GridArmClean06SaveInstancePreview::TryProject(
	const FGridItemInstance& Legacy,
	const UGridItemDefinitionAsset& SourceDefinition,
	const UGridArmamentDefinitionAsset& CanonicalDefinition,
	FRecord& OutRecord, EError& OutError)
{
	OutRecord = FRecord();
	OutError = EError::InvalidInstance;
	if (!Legacy.IsValid())
	{
		return false;
	}
	if (Legacy.ItemLevel == 0)
	{
		// Old saves are NOT silently promoted to N1 or a character level.
		OutError = EError::LegacyLevelZero;
		return false;
	}
	if (Legacy.AbsorbedEquipmentPhysicalArmor < 0 ||
		Legacy.AbsorbedEquipmentMagicalArmor < 0)
	{
		OutError = EError::InvalidArmorDepletion;
		return false;
	}
	if (Legacy.ItemDefinitionId != SourceDefinition.ItemDefinitionId ||
		SourceDefinition.ItemDefinitionId != CanonicalDefinition.ItemId ||
		SourceDefinition.WeaponArchetypeId != CanonicalDefinition.ArchetypeId ||
		!SourceDefinition.bUseWeaponCatalogue)
	{
		OutError = EError::IdentityMismatch;
		return false;
	}
	if (!CanonicalDefinition.HasValidIdentity())
	{
		OutError = EError::InvalidCanonicalDefinition;
		return false;
	}
	if (!CanonicalDefinition.DisplayName.EqualTo(SourceDefinition.DisplayName) ||
		CanonicalDefinition.WeightKg != SourceDefinition.Weight ||
		!Legacy.DisplayName.EqualTo(CanonicalDefinition.DisplayName) ||
		Legacy.Weight != CanonicalDefinition.WeightKg)
	{
		// Never drop an authored per-instance name, a stale weight snapshot,
		// or a historical template mismatch without explicit reconciliation.
		OutError = EError::StaticSnapshotMismatch;
		return false;
	}

	FRecord Candidate;
	Candidate.RuntimeObjectId = Legacy.RuntimeObjectId;
	Candidate.ItemId = Legacy.ItemDefinitionId;
	Candidate.ItemLevel = Legacy.ItemLevel;
	Candidate.Quantity = Legacy.Quantity;
	Candidate.AbsorbedEquipmentPhysicalArmor = Legacy.AbsorbedEquipmentPhysicalArmor;
	Candidate.AbsorbedEquipmentMagicalArmor = Legacy.AbsorbedEquipmentMagicalArmor;
	Candidate.OwnerType = Legacy.OwnerType;
	Candidate.OwnerGuid = Legacy.OwnerGuid;
	Candidate.OwnerCharacterIndex = Legacy.OwnerCharacterIndex;
	Candidate.EquipmentSlot = Legacy.EquipmentSlot;
	Candidate.bLightsEnabled = Legacy.bLightsEnabled;
	Candidate.ReadableContentAsset = Legacy.ReadableContentAsset;
	Candidate.ReadableContentId = Legacy.ReadableContentId;
	Candidate.ReadTitleOverride = Legacy.ReadTitleOverride;
	Candidate.ReadTextOverride = Legacy.ReadTextOverride;
	Candidate.LastWorldTransform = Legacy.LastWorldTransform;

	FGridItemInstance RoundTrip;
	if (!TryRehydrateForParity(Candidate, CanonicalDefinition, RoundTrip) ||
		!IsExactRoundTrip(Legacy, RoundTrip))
	{
		OutError = EError::StaticSnapshotMismatch;
		return false;
	}
	OutRecord = MoveTemp(Candidate);
	OutError = EError::None;
	return true;
}

bool GridArmClean06SaveInstancePreview::TryRehydrateForParity(
	const FRecord& Record,
	const UGridArmamentDefinitionAsset& CanonicalDefinition,
	FGridItemInstance& OutLegacy)
{
	OutLegacy = FGridItemInstance();
	if (!CanonicalDefinition.HasValidIdentity() ||
		!Record.RuntimeObjectId.IsValid() ||
		Record.ItemId != CanonicalDefinition.ItemId ||
		Record.ItemLevel < 1 || Record.ItemLevel > 20 ||
		Record.Quantity < 1 || Record.AbsorbedEquipmentPhysicalArmor < 0 ||
		Record.AbsorbedEquipmentMagicalArmor < 0)
	{
		return false;
	}
	OutLegacy.RuntimeObjectId = Record.RuntimeObjectId;
	OutLegacy.ItemDefinitionId = Record.ItemId;
	OutLegacy.ItemLevel = Record.ItemLevel;
	OutLegacy.AbsorbedEquipmentPhysicalArmor = Record.AbsorbedEquipmentPhysicalArmor;
	OutLegacy.AbsorbedEquipmentMagicalArmor = Record.AbsorbedEquipmentMagicalArmor;
	OutLegacy.DisplayName = CanonicalDefinition.DisplayName;
	OutLegacy.Quantity = Record.Quantity;
	OutLegacy.Weight = CanonicalDefinition.WeightKg;
	OutLegacy.OwnerType = Record.OwnerType;
	OutLegacy.OwnerGuid = Record.OwnerGuid;
	OutLegacy.OwnerCharacterIndex = Record.OwnerCharacterIndex;
	OutLegacy.EquipmentSlot = Record.EquipmentSlot;
	OutLegacy.bLightsEnabled = Record.bLightsEnabled;
	OutLegacy.ReadableContentAsset = Record.ReadableContentAsset;
	OutLegacy.ReadableContentId = Record.ReadableContentId;
	OutLegacy.ReadTitleOverride = Record.ReadTitleOverride;
	OutLegacy.ReadTextOverride = Record.ReadTextOverride;
	OutLegacy.LastWorldTransform = Record.LastWorldTransform;
	return OutLegacy.IsValid();
}

bool GridArmClean06SaveInstancePreview::IsExactRoundTrip(
	const FGridItemInstance& Before,
	const FGridItemInstance& After)
{
	return Before.RuntimeObjectId == After.RuntimeObjectId &&
		Before.ItemDefinitionId == After.ItemDefinitionId &&
		Before.ItemLevel == After.ItemLevel &&
		Before.AbsorbedEquipmentPhysicalArmor == After.AbsorbedEquipmentPhysicalArmor &&
		Before.AbsorbedEquipmentMagicalArmor == After.AbsorbedEquipmentMagicalArmor &&
		Before.DisplayName.EqualTo(After.DisplayName) &&
		Before.Quantity == After.Quantity &&
		Before.Weight == After.Weight &&
		Before.OwnerType == After.OwnerType &&
		Before.OwnerGuid == After.OwnerGuid &&
		Before.OwnerCharacterIndex == After.OwnerCharacterIndex &&
		Before.EquipmentSlot == After.EquipmentSlot &&
		Before.bLightsEnabled == After.bLightsEnabled &&
		Before.ReadableContentAsset == After.ReadableContentAsset &&
		Before.ReadableContentId == After.ReadableContentId &&
		Before.ReadTitleOverride.EqualTo(After.ReadTitleOverride) &&
		Before.ReadTextOverride.EqualTo(After.ReadTextOverride) &&
		Before.LastWorldTransform.Equals(After.LastWorldTransform, 0.0f);
}

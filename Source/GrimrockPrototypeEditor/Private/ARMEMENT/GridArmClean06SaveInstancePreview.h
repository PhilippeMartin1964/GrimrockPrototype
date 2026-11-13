#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"

/**
 * Editor-only, non-serialized ARM-CLEAN06.2AD migration preview.
 *
 * Every mutable field from FGridItemInstance is retained. Only redundant
 * DisplayName and unit Weight are omitted, and ONLY when exact parity against
 * the real, uniquely identified production definition has been proven.
 *
 * The raw readable-content UObject is kept for an exact in-memory proof;
 * final stable reference serialization is a separate mandatory cutover gate.
 */
namespace GridArmClean06SaveInstancePreview
{
	enum class EError : uint8
	{
		None,
		InvalidInstance,
		LegacyLevelZero,
		InvalidArmorDepletion,
		IdentityMismatch,
		InvalidCanonicalDefinition,
		StaticSnapshotMismatch
	};

	struct FRecord
	{
		FGuid RuntimeObjectId;
		FName ItemId = NAME_None;
		int32 ItemLevel = 0;
		int32 Quantity = 0;
		int32 AbsorbedEquipmentPhysicalArmor = 0;
		int32 AbsorbedEquipmentMagicalArmor = 0;
		EGridItemOwnerType OwnerType = EGridItemOwnerType::None;
		FGuid OwnerGuid;
		int32 OwnerCharacterIndex = INDEX_NONE;
		EGridEquipmentSlot EquipmentSlot = EGridEquipmentSlot::None;
		bool bLightsEnabled = false;
		TObjectPtr<UGridReadableContentAsset> ReadableContentAsset = nullptr;
		FName ReadableContentId = NAME_None;
		FText ReadTitleOverride;
		FText ReadTextOverride;
		FTransform LastWorldTransform = FTransform::Identity;
	};

	bool TryProject(const FGridItemInstance& Legacy,
		const UGridItemDefinitionAsset& SourceDefinition,
		const UGridArmamentDefinitionAsset& CanonicalDefinition,
		FRecord& OutRecord, EError& OutError);

	bool TryRehydrateForParity(const FRecord& Record,
		const UGridArmamentDefinitionAsset& CanonicalDefinition,
		FGridItemInstance& OutLegacy);

	/** Compare every historical instance field, including the optional reading state. */
	bool IsExactRoundTrip(const FGridItemInstance& Before,
		const FGridItemInstance& After);
}

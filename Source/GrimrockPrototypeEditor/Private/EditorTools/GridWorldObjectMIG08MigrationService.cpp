#include "EditorTools/GridWorldObjectMIG08MigrationService.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridWorldObjectVisual.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	void AddUniqueId(const FGuid& Id, const FString& Context, TSet<FGuid>& SeenIds, FGridWorldObjectMIG08MigrationResult& Result)
	{
		if (!Id.IsValid())
		{
			Result.Errors.Add(FString::Printf(TEXT("%s has an invalid persistent id."), *Context));
			return;
		}
		if (SeenIds.Contains(Id))
		{
			Result.Errors.Add(FString::Printf(TEXT("%s reuses persistent id %s."), *Context, *Id.ToString(EGuidFormats::DigitsWithHyphens)));
			return;
		}
		SeenIds.Add(Id);
	}

	void MigratePaletteIconToItem(FGridObjectPaletteEntry& Entry, UGridItemDefinitionAsset& ItemDefinition, FGridWorldObjectMIG08MigrationResult& Result)
	{
		if (!Entry.Icon)
		{
			return;
		}

		if (ItemDefinition.Icon.IsNull())
		{
#if WITH_EDITOR
			ItemDefinition.Modify();
#endif
			ItemDefinition.Icon = Entry.Icon.Get();
#if WITH_EDITOR
			ItemDefinition.MarkPackageDirty();
#endif
			Result.bChanged = true;
			Result.Changes.Add(FString::Printf(TEXT("Palette entry '%s': promoted palette icon into ItemDefinition '%s'."), *Entry.EntryId.ToString(),
				*ItemDefinition.GetPathName()));
		}
		else if (ItemDefinition.Icon.ToSoftObjectPath().ToString() != Entry.Icon->GetPathName())
		{
			Result.Warnings.Add(FString::Printf(TEXT("Palette entry '%s': ItemDefinition '%s' already owns a different icon; existing ItemDefinition icon kept."),
				*Entry.EntryId.ToString(), *ItemDefinition.GetPathName()));
		}

		Entry.Icon = nullptr;
		Result.bChanged = true;
	}

	void MigrateLegacyItemWorldMesh(
		const FGridObjectPaletteEntry& Entry, const UGridObjectArchetypeAsset& LegacyArchetype, UGridItemDefinitionAsset& ItemDefinition,
		FGridWorldObjectMIG08MigrationResult& Result)
	{
		if (!ItemDefinition.WorldMesh.IsNull())
		{
			if (LegacyArchetype.StaticPart.Mesh && ItemDefinition.WorldMesh.ToSoftObjectPath().ToString() != LegacyArchetype.StaticPart.Mesh->GetPathName())
			{
				Result.Warnings.Add(FString::Printf(TEXT("Palette entry '%s': ItemDefinition '%s' already owns a different WorldMesh; existing ItemDefinition mesh kept."),
					*Entry.EntryId.ToString(), *ItemDefinition.GetPathName()));
			}
			return;
		}

		if (!LegacyArchetype.StaticPart.Mesh)
		{
			Result.Warnings.Add(FString::Printf(TEXT("Palette entry '%s': legacy item archetype '%s' has no StaticPart mesh to promote into ItemDefinition '%s'."),
				*Entry.EntryId.ToString(), *LegacyArchetype.GetPathName(), *ItemDefinition.GetPathName()));
			return;
		}

#if WITH_EDITOR
		ItemDefinition.Modify();
#endif
		ItemDefinition.WorldMesh = LegacyArchetype.StaticPart.Mesh.Get();
#if WITH_EDITOR
		ItemDefinition.MarkPackageDirty();
#endif
		Result.bChanged = true;
		Result.Changes.Add(FString::Printf(TEXT("Palette entry '%s': promoted StaticPart mesh from legacy item archetype '%s' into ItemDefinition '%s'."),
			*Entry.EntryId.ToString(), *LegacyArchetype.GetPathName(), *ItemDefinition.GetPathName()));
	}

	void BeginArchetypeMutation(UGridObjectArchetypeAsset& Archetype, bool& bMutationStarted)
	{
		if (bMutationStarted)
		{
			return;
		}
#if WITH_EDITOR
		Archetype.Modify(false);
#endif
		bMutationStarted = true;
	}

	void RecordArchetypeChange(UGridObjectArchetypeAsset& Archetype, const FString& Change, bool& bMutationStarted,
		FGridWorldObjectMIG08MigrationResult& Result)
	{
		BeginArchetypeMutation(Archetype, bMutationStarted);
		Result.bChanged = true;
		Result.Changes.Add(Change);
	}

	bool IsMotionUnconfigured(const FGridWorldObjectMotion& Motion)
	{
		return FMath::IsNearlyZero(Motion.Amount) && FMath::IsNearlyZero(Motion.Duration) && Motion.Pivot.IsNearlyZero();
	}

	UStaticMesh* LoadMigrationMesh(const TCHAR* ObjectPath, const UGridObjectArchetypeAsset& Archetype, const TCHAR* Role,
		FGridWorldObjectMIG08MigrationResult& Result)
	{
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, ObjectPath);
		if (!Mesh)
		{
			Result.Errors.Add(FString::Printf(TEXT("Archetype '%s': MIG08 could not load expected %s mesh '%s'."), *Archetype.GetPathName(), Role, ObjectPath));
		}
		return Mesh;
	}

	void EnsureStaticMesh(UGridObjectArchetypeAsset& Archetype, const TCHAR* ObjectPath, bool& bMutationStarted,
		FGridWorldObjectMIG08MigrationResult& Result)
	{
		if (Archetype.StaticPart.IsDefined())
		{
			return;
		}
		UStaticMesh* Mesh = LoadMigrationMesh(ObjectPath, Archetype, TEXT("StaticPart"), Result);
		if (!Mesh)
		{
			return;
		}
		RecordArchetypeChange(Archetype,
			FString::Printf(TEXT("Archetype '%s': restored StaticPart mesh '%s'."), *Archetype.GetPathName(), *Mesh->GetPathName()), bMutationStarted, Result);
		Archetype.StaticPart.Mesh = Mesh;
		Archetype.StaticPart.LocalTransform = FTransform::Identity;
	}

	void EnsureMovingMesh(UGridObjectArchetypeAsset& Archetype, const TCHAR* ObjectPath, const FTransform& DefaultLocalTransform, bool& bMutationStarted,
		FGridWorldObjectMIG08MigrationResult& Result)
	{
		if (Archetype.MovingParts.Part0.IsDefined())
		{
			return;
		}
		UStaticMesh* Mesh = LoadMigrationMesh(ObjectPath, Archetype, TEXT("MovingPart[0]"), Result);
		if (!Mesh)
		{
			return;
		}
		RecordArchetypeChange(Archetype,
			FString::Printf(TEXT("Archetype '%s': restored MovingPart[0] mesh '%s'."), *Archetype.GetPathName(), *Mesh->GetPathName()), bMutationStarted,
			Result);
		Archetype.MovingParts.Part0.Mesh = Mesh;
		Archetype.MovingParts.Part0.LocalTransform = DefaultLocalTransform;
	}

	void EnsureMotion(UGridObjectArchetypeAsset& Archetype, FGridWorldObjectMovingPart& Part, EGridWorldObjectMotionType Type,
		EGridWorldObjectMotionAxis Axis, const FVector& Pivot, float Amount, float Duration, const TCHAR* PartLabel, bool& bMutationStarted,
		FGridWorldObjectMIG08MigrationResult& Result)
	{
		if (!Part.IsDefined() || !IsMotionUnconfigured(Part.Motion))
		{
			return;
		}
		RecordArchetypeChange(Archetype,
			FString::Printf(TEXT("Archetype '%s': restored %s Motion from the validated pre-MIG04 contract."), *Archetype.GetPathName(), PartLabel),
			bMutationStarted, Result);
		Part.Motion.Type = Type;
		Part.Motion.Axis = Axis;
		Part.Motion.Pivot = Pivot;
		Part.Motion.Amount = Amount;
		Part.Motion.Duration = Duration;
	}

	void EnsureLeverRestTransform(UGridObjectArchetypeAsset& Archetype, bool& bMutationStarted, FGridWorldObjectMIG08MigrationResult& Result)
	{
		FGridWorldObjectMovingPart& Part = Archetype.MovingParts.Part0;
		if (!Part.IsDefined() || !IsMotionUnconfigured(Part.Motion) || !Part.LocalTransform.Equals(FTransform::Identity))
		{
			return;
		}
		RecordArchetypeChange(Archetype,
			FString::Printf(TEXT("Archetype '%s': restored lever rest transform Pitch=45 degrees."), *Archetype.GetPathName()), bMutationStarted, Result);
		Part.LocalTransform.SetRotation(FRotator(45.0f, 0.0f, 0.0f).Quaternion());
	}

	void EnsurePressurePlateRestTransform(UGridObjectArchetypeAsset& Archetype, bool& bMutationStarted, FGridWorldObjectMIG08MigrationResult& Result)
	{
		FGridWorldObjectMovingPart& Part = Archetype.MovingParts.Part0;
		if (!Part.IsDefined() || !IsMotionUnconfigured(Part.Motion) || !Part.LocalTransform.Equals(FTransform::Identity))
		{
			return;
		}
		RecordArchetypeChange(Archetype,
			FString::Printf(TEXT("Archetype '%s': restored pressure-plate released height Z=4 cm."), *Archetype.GetPathName()), bMutationStarted,
			Result);
		Part.LocalTransform.SetLocation(FVector(0.0f, 0.0f, 4.0f));
	}

	void NormalizeRequiredPlacement(UGridObjectArchetypeAsset& Archetype, bool& bMutationStarted, FGridWorldObjectMIG08MigrationResult& Result)
	{
		EGridObjectPlacementKind RequiredSurface = Archetype.PlacementSurface;
		bool bHasRequiredSurface = true;
		switch (Archetype.SupportedType)
		{
			case EGridLevelObjectType::Door:
			case EGridLevelObjectType::Button:
			case EGridLevelObjectType::Lever:
				RequiredSurface = EGridObjectPlacementKind::Wall;
				break;
			case EGridLevelObjectType::PressurePlate:
			case EGridLevelObjectType::Trigger:
			case EGridLevelObjectType::Pit:
				RequiredSurface = EGridObjectPlacementKind::Floor;
				break;
			default:
				if (Archetype.bReplacesStandardWall)
				{
					RequiredSurface = EGridObjectPlacementKind::Wall;
				}
				else
				{
					bHasRequiredSurface = false;
				}
				break;
		}

		if (!bHasRequiredSurface || Archetype.PlacementSurface == RequiredSurface)
		{
			return;
		}

		const EGridObjectPlacementKind PreviousSurface = Archetype.PlacementSurface;
		RecordArchetypeChange(Archetype,
			FString::Printf(TEXT("Archetype '%s': normalized PlacementSurface from %d to %d for Gameplay Type %d."), *Archetype.GetPathName(),
				static_cast<int32>(PreviousSurface), static_cast<int32>(RequiredSurface), static_cast<int32>(Archetype.SupportedType)),
			bMutationStarted, Result);
		Archetype.PlacementSurface = RequiredSurface;
		Archetype.RefreshPlacementRuntimeProjection();
	}

	void RestoreKnownVisualContract(UGridObjectArchetypeAsset& Archetype, bool& bMutationStarted, FGridWorldObjectMIG08MigrationResult& Result)
	{
		const FName Id = Archetype.ArchetypeId;

		if (Id == FName(TEXT("Button_Normal")))
		{
			EnsureStaticMesh(Archetype, TEXT("/Game/GrimrockPrototype/Meshes/Button/SM_Button_Mettalic_Static.SM_Button_Mettalic_Static"), bMutationStarted, Result);
			EnsureMovingMesh(Archetype, TEXT("/Game/GrimrockPrototype/Meshes/Button/SM_Button_Mettalic_Mobile.SM_Button_Mettalic_Mobile"),
				FTransform::Identity, bMutationStarted, Result);
			EnsureMotion(Archetype, Archetype.MovingParts.Part0, EGridWorldObjectMotionType::Translation, EGridWorldObjectMotionAxis::X,
				FVector::ZeroVector, 6.0f, 0.08f, TEXT("MovingPart[0]"), bMutationStarted, Result);
			return;
		}

		if (Id == FName(TEXT("Button_Secret")))
		{
			EnsureMovingMesh(Archetype, TEXT("/Game/GrimrockPrototype/Meshes/SM_SecretButton_03.SM_SecretButton_03"), FTransform::Identity,
				bMutationStarted, Result);
			EnsureMotion(Archetype, Archetype.MovingParts.Part0, EGridWorldObjectMotionType::Translation, EGridWorldObjectMotionAxis::X,
				FVector::ZeroVector, 6.0f, 0.08f, TEXT("MovingPart[0]"), bMutationStarted, Result);
			return;
		}

		if (Id == FName(TEXT("Lever")))
		{
			EnsureStaticMesh(Archetype, TEXT("/Game/GrimrockPrototype/Meshes/Button/SM_LeverStatic_01.SM_LeverStatic_01"), bMutationStarted, Result);
			EnsureMovingMesh(Archetype, TEXT("/Game/GrimrockPrototype/Meshes/Button/SM_Lever_01.SM_Lever_01"), FTransform::Identity,
				bMutationStarted, Result);
			EnsureLeverRestTransform(Archetype, bMutationStarted, Result);
			EnsureMotion(Archetype, Archetype.MovingParts.Part0, EGridWorldObjectMotionType::Rotation, EGridWorldObjectMotionAxis::Y,
				FVector::ZeroVector, 90.0f, 0.10f, TEXT("MovingPart[0]"), bMutationStarted, Result);
			return;
		}

		if (Id == FName(TEXT("PressurePlate")))
		{
			EnsureStaticMesh(Archetype, TEXT("/Game/GrimrockPrototype/Meshes/Floor/SM_Grid_PressurePlate_Static.SM_Grid_PressurePlate_Static"), bMutationStarted,
				Result);
			EnsureMovingMesh(Archetype, TEXT("/Game/GrimrockPrototype/Meshes/Floor/SM_Grid_PressurePlate_Moving.SM_Grid_PressurePlate_Moving"),
				FTransform::Identity, bMutationStarted, Result);
			EnsurePressurePlateRestTransform(Archetype, bMutationStarted, Result);
			EnsureMotion(Archetype, Archetype.MovingParts.Part0, EGridWorldObjectMotionType::Translation, EGridWorldObjectMotionAxis::Z,
				FVector::ZeroVector, -3.0f, 0.08f, TEXT("MovingPart[0]"), bMutationStarted, Result);
			return;
		}

		if (Id == FName(TEXT("Door_Wood")))
		{
			EnsureMovingMesh(Archetype, TEXT("/Game/GrimrockPrototype/Meshes/Door/SM_Door_Wood_Mobile_01.SM_Door_Wood_Mobile_01"),
				FTransform::Identity, bMutationStarted, Result);
			EnsureMotion(Archetype, Archetype.MovingParts.Part0, EGridWorldObjectMotionType::Translation, EGridWorldObjectMotionAxis::Z,
				FVector::ZeroVector, 180.0f, 2.5f, TEXT("MovingPart[0]"), bMutationStarted, Result);
			return;
		}

		if (Id == FName(TEXT("Door_Grating")))
		{
			EnsureMovingMesh(Archetype, TEXT("/Game/GrimrockPrototype/Meshes/Door/SM_Door_Grating_Mobile_01.SM_Door_Grating_Mobile_01"),
				FTransform::Identity, bMutationStarted, Result);
			EnsureMotion(Archetype, Archetype.MovingParts.Part0, EGridWorldObjectMotionType::Translation, EGridWorldObjectMotionAxis::Z,
				FVector::ZeroVector, 180.0f, 2.5f, TEXT("MovingPart[0]"), bMutationStarted, Result);
			return;
		}

		if (Id == FName(TEXT("Door_Secret")))
		{
			EnsureStaticMesh(Archetype,
				TEXT("/Game/GrimrockPrototype/Meshes/Wall/SM_Wall_Stone_SecretDoorStatic-01.SM_Wall_Stone_SecretDoorStatic-01"), bMutationStarted,
				Result);
			EnsureMovingMesh(Archetype, TEXT("/Game/GrimrockPrototype/Meshes/Wall/SM_Wall_Stone_SecretDoor-01.SM_Wall_Stone_SecretDoor-01"),
				FTransform::Identity, bMutationStarted, Result);
			EnsureMotion(Archetype, Archetype.MovingParts.Part0, EGridWorldObjectMotionType::Translation, EGridWorldObjectMotionAxis::Z,
				FVector::ZeroVector, 180.0f, 2.5f, TEXT("MovingPart[0]"), bMutationStarted, Result);
			return;
		}

		if (Id == FName(TEXT("Pit_Stone_01")) || Id == FName(TEXT("Pit_Stone")))
		{
			EnsureStaticMesh(Archetype, TEXT("/Game/GrimrockPrototype/Meshes/Floor/SM_Pit_Stone_01.SM_Pit_Stone_01"), bMutationStarted, Result);

			// A static open pit is valid. MIG08 must never invent trapdoor leaves.
			if (Archetype.MovingParts.Part0.IsDefined() && Archetype.MovingParts.Part1.IsDefined())
			{
				EnsureMotion(Archetype, Archetype.MovingParts.Part0, EGridWorldObjectMotionType::Rotation, EGridWorldObjectMotionAxis::Y,
					FVector(-85.0f, 0.0f, -5.0f), -80.0f, 0.75f, TEXT("MovingPart[0]"), bMutationStarted, Result);
				EnsureMotion(Archetype, Archetype.MovingParts.Part1, EGridWorldObjectMotionType::Rotation, EGridWorldObjectMotionAxis::Y,
					FVector(85.0f, 0.0f, -5.0f), 80.0f, 0.75f, TEXT("MovingPart[1]"), bMutationStarted, Result);
			}
		}
	}
}

FGridWorldObjectMIG08MigrationResult FGridWorldObjectMIG08MigrationService::MigrateLevelAsset(UGridLevelAsset& LevelAsset)
{
	FGridWorldObjectMIG08MigrationResult Result;
	const bool bWasTyped = LevelAsset.bTypedPlacementStorageAuthoritative;
	const int32 LegacyCountBefore = LevelAsset.Objects.Num();
	TSet<FGuid> LegacyIdsBefore;

	if (!bWasTyped)
	{
		for (const FGridLevelObjectData& Object : LevelAsset.Objects)
		{
			if (Object.ObjectId.IsValid())
			{
				LegacyIdsBefore.Add(Object.ObjectId);
			}
		}

#if WITH_EDITOR
		LevelAsset.Modify();
#endif
		LevelAsset.EnableTypedPlacementStorageFromLegacy();
#if WITH_EDITOR
		LevelAsset.MarkPackageDirty();
#endif
		Result.bChanged = true;
		Result.Changes.Add(FString::Printf(TEXT("Level '%s': migrated %d legacy placements into typed storage."), *LevelAsset.GetPathName(), LegacyCountBefore));
	}
	else
	{
		LevelAsset.RefreshLegacyObjectMirrorFromTyped();
	}

	const TArray<FGridLevelObjectData>& CompatibilityView = LevelAsset.GetObjectCompatibilityView();
	if (LevelAsset.GetTypedPlacementCount() != CompatibilityView.Num())
	{
		Result.Errors.Add(FString::Printf(TEXT("Level '%s': typed placement count %d does not match compatibility mirror count %d."),
			*LevelAsset.GetPathName(), LevelAsset.GetTypedPlacementCount(), CompatibilityView.Num()));
	}

	if (!bWasTyped && LegacyCountBefore != CompatibilityView.Num())
	{
		Result.Errors.Add(FString::Printf(TEXT("Level '%s': migration changed placement count from %d to %d."), *LevelAsset.GetPathName(), LegacyCountBefore,
			CompatibilityView.Num()));
	}

	if (!bWasTyped)
	{
		for (const FGuid& LegacyId : LegacyIdsBefore)
		{
			if (!CompatibilityView.ContainsByPredicate(
					[&LegacyId](const FGridLevelObjectData& Object)
					{
						return Object.ObjectId == LegacyId;
					}))
			{
				Result.Errors.Add(FString::Printf(TEXT("Level '%s': migration lost legacy ObjectId %s."), *LevelAsset.GetPathName(),
					*LegacyId.ToString(EGuidFormats::DigitsWithHyphens)));
			}
		}
	}

	TSet<FGuid> SeenIds;
	for (const FGridWorldObjectInstance& Instance : LevelAsset.WorldObjectInstances)
	{
		AddUniqueId(Instance.InstanceId, TEXT("WorldObjectInstance"), SeenIds, Result);
		if (Instance.WorldObjectDefinitionId.IsNone())
		{
			Result.Errors.Add(FString::Printf(TEXT("Level '%s': WorldObjectInstance %s has no WorldObjectDefinitionId."), *LevelAsset.GetPathName(),
				*Instance.InstanceId.ToString(EGuidFormats::DigitsWithHyphens)));
		}
	}
	for (const FGridLooseItemInstance& Instance : LevelAsset.LooseItemInstances)
	{
		AddUniqueId(Instance.InstanceId, TEXT("LooseItemInstance"), SeenIds, Result);
		if (!Instance.ItemDefinition)
		{
			Result.Errors.Add(FString::Printf(TEXT("Level '%s': LooseItemInstance %s has no ItemDefinition."), *LevelAsset.GetPathName(),
				*Instance.InstanceId.ToString(EGuidFormats::DigitsWithHyphens)));
		}
	}
	for (const FGridMonsterSpawnInstance& Spawn : LevelAsset.MonsterSpawns)
	{
		AddUniqueId(Spawn.SpawnId, TEXT("MonsterSpawn"), SeenIds, Result);
		if (!Spawn.MonsterDefinition)
		{
			Result.Errors.Add(FString::Printf(TEXT("Level '%s': MonsterSpawn %s has no MonsterDefinition."), *LevelAsset.GetPathName(),
				*Spawn.SpawnId.ToString(EGuidFormats::DigitsWithHyphens)));
		}
	}
	for (const FGridItemSpawnInstance& Spawn : LevelAsset.ItemSpawns)
	{
		AddUniqueId(Spawn.SpawnId, TEXT("ItemSpawn"), SeenIds, Result);
		if (!Spawn.ItemDefinition)
		{
			Result.Errors.Add(FString::Printf(TEXT("Level '%s': ItemSpawn %s has no ItemDefinition."), *LevelAsset.GetPathName(),
				*Spawn.SpawnId.ToString(EGuidFormats::DigitsWithHyphens)));
		}
	}
	for (const FGridLogicObjectInstance& Instance : LevelAsset.LogicObjects)
	{
		AddUniqueId(Instance.InstanceId, TEXT("LogicObject"), SeenIds, Result);
	}

	return Result;
}

FGridWorldObjectMIG08MigrationResult FGridWorldObjectMIG08MigrationService::MigratePaletteAsset(UGridObjectPaletteAsset& PaletteAsset)
{
	FGridWorldObjectMIG08MigrationResult Result;

	for (FGridObjectPaletteEntry& Entry : PaletteAsset.Entries)
	{
		UGridObjectArchetypeAsset* LegacyArchetype = Entry.DefaultArchetype.Get();
		UGridItemDefinitionAsset* ItemDefinition = Entry.DefaultItemDefinition.Get();

		if (ItemDefinition && LegacyArchetype && LegacyArchetype->SupportedType != EGridLevelObjectType::Item)
		{
			Result.Errors.Add(FString::Printf(TEXT("Palette '%s' entry '%s' mixes ItemDefinition '%s' with non-item archetype '%s'."),
				*PaletteAsset.GetPathName(), *Entry.EntryId.ToString(), *ItemDefinition->GetPathName(), *LegacyArchetype->GetPathName()));
			continue;
		}

		if (!ItemDefinition && LegacyArchetype && LegacyArchetype->SupportedType == EGridLevelObjectType::Item)
		{
			ItemDefinition = LegacyArchetype->DefaultBehavior.Item.ItemDefinitionAsset.Get();
			if (!ItemDefinition)
			{
				Result.Errors.Add(FString::Printf(TEXT("Palette '%s' entry '%s' uses legacy item archetype '%s' but no ItemDefinition can be resolved."),
					*PaletteAsset.GetPathName(), *Entry.EntryId.ToString(), *LegacyArchetype->GetPathName()));
				continue;
			}
		}

		if (!ItemDefinition)
		{
			continue;
		}

		if (LegacyArchetype)
		{
			MigrateLegacyItemWorldMesh(Entry, *LegacyArchetype, *ItemDefinition, Result);
		}
		MigratePaletteIconToItem(Entry, *ItemDefinition, Result);

		const bool bNeedsDirectItemContract = Entry.DefaultItemDefinition != ItemDefinition || Entry.DefaultArchetype != nullptr;
		if (bNeedsDirectItemContract)
		{
			Entry.DefaultItemDefinition = ItemDefinition;
			Entry.DefaultArchetype = nullptr;
			Result.bChanged = true;
			Result.Changes.Add(FString::Printf(TEXT("Palette '%s' entry '%s': converted to direct ItemDefinition '%s'."), *PaletteAsset.GetPathName(),
				*Entry.EntryId.ToString(), *ItemDefinition->GetPathName()));
		}
	}

	TMap<const UGridItemDefinitionAsset*, int32> ItemUsageCounts;
	for (const FGridObjectPaletteEntry& Entry : PaletteAsset.Entries)
	{
		if (Entry.DefaultItemDefinition)
		{
			++ItemUsageCounts.FindOrAdd(Entry.DefaultItemDefinition.Get());
		}
		if (!Entry.IsValidEntry())
		{
			Result.Errors.Add(FString::Printf(TEXT("Palette '%s' entry '%s' is invalid after MIG08 migration."), *PaletteAsset.GetPathName(),
				*Entry.EntryId.ToString()));
		}
	}

	for (const TPair<const UGridItemDefinitionAsset*, int32>& Pair : ItemUsageCounts)
	{
		if (Pair.Value > 1 && Pair.Key)
		{
			Result.Warnings.Add(FString::Printf(TEXT("Palette '%s': ItemDefinition '%s' is exposed by %d palette entries; review whether these entries are intentionally distinct."),
				*PaletteAsset.GetPathName(), *Pair.Key->GetPathName(), Pair.Value));
		}
	}

	if (Result.bChanged)
	{
#if WITH_EDITOR
		PaletteAsset.Modify();
		PaletteAsset.MarkPackageDirty();
#endif
	}

	return Result;
}

FGridWorldObjectMIG08MigrationResult FGridWorldObjectMIG08MigrationService::MigrateArchetypeAsset(UGridObjectArchetypeAsset& Archetype)
{
	FGridWorldObjectMIG08MigrationResult Result;
	if (Archetype.SupportedType == EGridLevelObjectType::Item)
	{
		return Result;
	}

	bool bMutationStarted = false;
	NormalizeRequiredPlacement(Archetype, bMutationStarted, Result);
	RestoreKnownVisualContract(Archetype, bMutationStarted, Result);

	if (Result.bChanged)
	{
		Archetype.RefreshPlacementRuntimeProjection();
#if WITH_EDITOR
		Archetype.MarkPackageDirty();
#endif
	}

	return Result;
}

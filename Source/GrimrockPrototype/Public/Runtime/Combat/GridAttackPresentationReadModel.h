#pragma once

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/Combat/GridPlayerAttackPresentationTypes.h"

/** Only the physical ARM source captured at execution can authorize its
 * catalogue presentation. No inventory lookups after debit, no old DA. */
struct FGridAttackPresentationReadModel
{
	static const UGridItemSubsystem* RegistryFor(const UGridPartyInventoryComponent* Inventory)
	{
		const UWorld* World = Inventory ? Inventory->GetWorld() : nullptr;
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<UGridItemSubsystem>() : nullptr;
	}

	static FGridAttackPresentationSource CaptureFromInstance(const UGridItemSubsystem* Registry,
		const FGridItemInstance& Item, const FGuid& CharacterId, int32 CharacterIndex,
		EGridEquipmentSlot Hand, FName AttackId, bool bBorrowWeaponAttack)
	{
		FGridAttackPresentationSource Out;
		if (!IsValid(Registry) || !Registry->ContainsItem(Item.ItemDefinitionId)) return Out;
		FGridResolvedItem Resolved;
		if (!Registry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Resolved) || !Resolved.bIsArmament)
			return Out;
		FGridResolvedBoundAction Bound;
		if (!bBorrowWeaponAttack)
		{
			if (!Registry->ResolveAttackFromEquippedInstance(Item, AttackId, Hand,
				Item.RuntimeObjectId, CharacterId, CharacterIndex, Bound)) return Out;
		}
		else
		{
			// Class/reaction AttackId describes the skill, not the physical
			// weapon's presentation binding. Current ordinary ARMs each
			// have one authored attack. Ambiguity is not a license to guess.
			int32 Count = 0;
			for (const auto& Binding : Resolved.AvailableActions)
			{
				FGridResolvedBoundAction Candidate;
				if (Registry->ResolveAttackFromEquippedInstance(Item, Binding.ActionId, Hand,
					Item.RuntimeObjectId, CharacterId, CharacterIndex, Candidate))
				{
					Bound = MoveTemp(Candidate);
					++Count;
				}
			}
			if (Count != 1) return Out;
		}
		Out.RuntimeObjectId = Item.RuntimeObjectId;
		Out.ItemLevel = Item.ItemLevel;
		Out.ActionId = Bound.ActionId;
		return Out;
	}

	static FGridAttackPresentationSource Capture(const UGridItemSubsystem* Registry,
		const UGridPartyInventoryComponent* Inventory, int32 CharacterIndex,
		FName ItemId, EGridEquipmentSlot Hand, FName AttackId, bool bBorrowWeaponAttack)
	{
		if (!Inventory || !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
			return FGridAttackPresentationSource();
		FGridItemInstance Item;
		if (!Inventory->GetEquippedItem(CharacterIndex, Hand, Item) || Item.ItemDefinitionId != ItemId)
			return FGridAttackPresentationSource();
		return CaptureFromInstance(Registry, Item, Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex].CharacterId,
			CharacterIndex, Hand, AttackId, bBorrowWeaponAttack);
	}

	static bool TryResolve(const UGridItemSubsystem* Registry, const FGridPlayerAttackRequest& Request,
		FGridPlayerAttackPresentationProfile& Out)
	{
		Out = FGridPlayerAttackPresentationProfile();
		FGridResolvedBoundAction Bound;
		if (!IsValid(Registry) || !Request.PresentationSource.RuntimeObjectId.IsValid() ||
			!Registry->ResolveBoundAttackForHand(Request.OffensiveItemDefinitionId,
				Request.PresentationSource.ItemLevel, Request.PresentationSource.ActionId,
				Request.OffensiveEquipmentSlot, Bound)) return false;
		const auto& P = Bound.Presentation;
		FGridPlayerAttackPresentationProfile Profile;
		Profile.MotionStyle = P.MotionStyle;
		Profile.bAnimateHeldItem = P.bAnimateHeldItem;
		Profile.MotionDurationSeconds = P.MotionDurationSeconds;
		Profile.ImpactDelaySeconds = P.ImpactDelaySeconds;
		Profile.RecoveryDelaySeconds = P.RecoveryDelaySeconds;
		Profile.PeakLocationOffset = P.PeakLocationOffset;
		Profile.PeakRotationOffset = P.PeakRotationOffset;
		Profile.FeedbackDurationSeconds = P.FeedbackDurationSeconds;
		const auto CopyAudio = [](const FGridAudioVariants& A, FGridPlayerAttackAudioDefinition& B)
		{
			B.Sounds = A.Sounds;
			B.VolumeMultiplier = A.VolumeMultiplier;
			B.PitchMin = A.PitchMin;
			B.PitchMax = A.PitchMax;
		};
		const auto CopyVFX = [](const FGridVFXVariants& A, FGridPlayerAttackVFXDefinition& B)
		{
			B.Systems = A.Systems;
			B.bAttachToHeldItem = A.bAttachToHeldItem;
			B.SocketName = A.SocketName;
			B.LocationOffset = A.LocationOffset;
			B.RotationOffset = A.RotationOffset;
			B.Scale = A.Scale;
		};
		CopyAudio(P.AttackAudio, Profile.AttackAudio);
		CopyAudio(P.ImpactHitAudio, Profile.ImpactHitAudio);
		CopyAudio(P.ImpactMissAudio, Profile.ImpactMissAudio);
		CopyAudio(P.ImpactFleshAudio, Profile.ImpactFleshAudio);
		CopyAudio(P.ImpactMetalAudio, Profile.ImpactMetalAudio);
		CopyAudio(P.ImpactWoodAudio, Profile.ImpactWoodAudio);
		CopyAudio(P.CriticalAudio, Profile.CriticalAudio);
		CopyVFX(P.AttackVFX, Profile.AttackVFX);
		CopyVFX(P.ImpactHitVFX, Profile.ImpactHitVFX);
		CopyVFX(P.ImpactMissVFX, Profile.ImpactMissVFX);
		if (!Profile.IsValid()) return false;
		Out = MoveTemp(Profile);
		return true;
	}
};

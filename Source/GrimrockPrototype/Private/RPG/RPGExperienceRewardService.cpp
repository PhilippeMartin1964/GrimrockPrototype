#include "RPG/RPGExperienceRewardService.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGLevelUpService.h"
#include "Runtime/GridPartyInventoryComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridExperience, Log, All);

namespace
{
	struct FRPGPendingExperienceAward
	{
		int32 CharacterIndex = INDEX_NONE;
		int32 RequestedExperience = 0;
		int32 AppliedExperience = 0;
		int32 PreviousExperience = 0;
		int32 NewExperience = 0;
	};
}

FGridCharacterExperienceAwardedNativeSignature& FRPGExperienceRewardService::OnCharacterExperienceAwarded()
{
	static FGridCharacterExperienceAwardedNativeSignature Delegate;
	return Delegate;
}

int32 FRPGExperienceRewardService::AwardToActiveParty(UGridPartyInventoryComponent* PartyInventoryComponent, int32 TotalExperienceReward)
{
	if (!IsValid(PartyInventoryComponent) || TotalExperienceReward <= 0)
	{
		return 0;
	}

	TArray<FGridCharacterInventoryState>& ActiveCharacters = PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
	if (ActiveCharacters.IsEmpty())
	{
		return 0;
	}

	const int32 MaximumExperience = URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(URPGCharacterRulesLibrary::GetMaximumLevel());

	TArray<int32> EligibleCharacterIndices;
	EligibleCharacterIndices.Reserve(ActiveCharacters.Num());
	for (int32 CharacterIndex = 0; CharacterIndex < ActiveCharacters.Num(); ++CharacterIndex)
	{
		const int32 Experience = ActiveCharacters[CharacterIndex].Experience;
		if (Experience < 0 || Experience > MaximumExperience)
		{
			UE_LOG(LogGridExperience, Warning, TEXT("[GridExperience] Character=%d Experience=%d Result=Skipped Reason=InvalidStoredExperience"),
				CharacterIndex, Experience);
			continue;
		}

		if (Experience < MaximumExperience)
		{
			EligibleCharacterIndices.Add(CharacterIndex);
		}
	}

	if (EligibleCharacterIndices.IsEmpty())
	{
		UE_LOG(LogGridExperience, Log, TEXT("[GridExperience] Reward=%d Applied=0 Result=NoEligibleActiveCharacter"), TotalExperienceReward);
		return 0;
	}

	const int32 EligibleCount = EligibleCharacterIndices.Num();
	const int32 BaseShare = TotalExperienceReward / EligibleCount;
	const int32 Remainder = TotalExperienceReward % EligibleCount;
	TArray<FRPGPendingExperienceAward> PendingAwards;
	PendingAwards.Reserve(EligibleCount);
	int32 TotalAppliedExperience = 0;

	for (int32 EligibleIndex = 0; EligibleIndex < EligibleCount; ++EligibleIndex)
	{
		const int32 CharacterIndex = EligibleCharacterIndices[EligibleIndex];
		const FGridCharacterInventoryState& Character = ActiveCharacters[CharacterIndex];
		const int32 RequestedShare = BaseShare + (EligibleIndex < Remainder ? 1 : 0);
		if (RequestedShare <= 0)
		{
			continue;
		}

		FRPGPendingExperienceAward Pending;
		Pending.CharacterIndex = CharacterIndex;
		Pending.RequestedExperience = RequestedShare;
		Pending.PreviousExperience = Character.Experience;
		const int64 CandidateExperience = static_cast<int64>(Pending.PreviousExperience) + static_cast<int64>(RequestedShare);
		Pending.NewExperience = static_cast<int32>(FMath::Min<int64>(CandidateExperience, static_cast<int64>(MaximumExperience)));
		Pending.AppliedExperience = Pending.NewExperience - Pending.PreviousExperience;
		if (Pending.AppliedExperience <= 0)
		{
			continue;
		}

		TotalAppliedExperience += Pending.AppliedExperience;
		PendingAwards.Add(Pending);
	}

	// Apply every XP mutation first so level-up calculations see the complete
	// group reward transaction before any external observer is notified.
	for (const FRPGPendingExperienceAward& Pending : PendingAwards)
	{
		ActiveCharacters[Pending.CharacterIndex].Experience = Pending.NewExperience;
	}

	// MON15.3: resolve pending level-ups synchronously before XP/inventory
	// notifications. The later inventory notification covers both mutations.
	for (const FRPGPendingExperienceAward& Pending : PendingAwards)
	{
		FRPGLevelUpService::ApplyPendingLevelUp(PartyInventoryComponent, Pending.CharacterIndex, false);
	}

	for (const FRPGPendingExperienceAward& Pending : PendingAwards)
	{
		const FGridCharacterInventoryState& Character = ActiveCharacters[Pending.CharacterIndex];
		PartyInventoryComponent->NotifyPartyInventoryChanged(Pending.CharacterIndex);
		OnCharacterExperienceAwarded().Broadcast(Pending.CharacterIndex, Pending.AppliedExperience, Pending.PreviousExperience, Pending.NewExperience);

		UE_LOG(LogGridExperience, Log, TEXT("[GridExperience] Character=%d Requested=%d Applied=%d Previous=%d New=%d LevelStored=%d"), Pending.CharacterIndex,
			Pending.RequestedExperience, Pending.AppliedExperience, Pending.PreviousExperience, Pending.NewExperience, Character.Level);
	}

	UE_LOG(LogGridExperience, Log, TEXT("[GridExperience] Reward=%d Eligible=%d Applied=%d Unapplied=%d"), TotalExperienceReward, EligibleCount,
		TotalAppliedExperience, TotalExperienceReward - TotalAppliedExperience);
	return TotalAppliedExperience;
}

#include "Runtime/Monsters/GridMonsterActor.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

bool AGridMonsterActor::StartHurtPresentation()
{
	if (IsDead() || !IsValid(MonsterDefinition) || MonsterDefinition->HurtMontage.IsNull() || !SkeletalMeshComponent)
	{
		return false;
	}

	// Attack and death presentations remain authoritative over hit reactions.
	if ((CombatComponent && CombatComponent->bAttackPresentationActive) ||
		(DeathComponent && (DeathComponent->bDeathCommitted || DeathComponent->bDeathPresentationActive)))
	{
		return false;
	}

	UAnimMontage* HurtMontage = MonsterDefinition->HurtMontage.LoadSynchronous();
	UAnimInstance* AnimInstance = SkeletalMeshComponent->GetAnimInstance();
	if (!IsValid(HurtMontage) || !IsValid(AnimInstance))
	{
		return false;
	}

	// A new effective hit restarts only the hit-reaction montage. Never stop
	// unrelated attack/death montages from this presentation path.
	if (AnimInstance->Montage_IsPlaying(HurtMontage))
	{
		AnimInstance->Montage_Stop(0.0f, HurtMontage);
	}

	return AnimInstance->Montage_Play(HurtMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, false) > 0.0f;
}

void AGridMonsterActor::StopHurtPresentation(float BlendOutTime)
{
	if (!IsValid(MonsterDefinition) || MonsterDefinition->HurtMontage.IsNull() || !SkeletalMeshComponent)
	{
		return;
	}

	// If the soft reference is not loaded, the montage cannot currently be
	// playing on this AnimInstance. Avoid a synchronous load during death,
	// restore and initialization cleanup.
	UAnimMontage* HurtMontage = MonsterDefinition->HurtMontage.Get();
	UAnimInstance* AnimInstance = SkeletalMeshComponent->GetAnimInstance();
	if (!IsValid(HurtMontage) || !IsValid(AnimInstance))
	{
		return;
	}

	AnimInstance->Montage_Stop(FMath::Max(0.0f, BlendOutTime), HurtMontage);
}

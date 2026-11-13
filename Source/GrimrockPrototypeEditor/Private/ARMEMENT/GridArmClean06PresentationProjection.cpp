#include "ARMEMENT/GridArmClean06PresentationProjection.h"

namespace
{
	FGridCanonicalAudioVariants CopyAudio(const FGridPlayerAttackAudioDefinition& Source)
	{
		FGridCanonicalAudioVariants Result;
		Result.Sounds = Source.Sounds;
		Result.VolumeMultiplier = Source.VolumeMultiplier;
		Result.PitchMin = Source.PitchMin;
		Result.PitchMax = Source.PitchMax;
		return Result;
	}

	FGridCanonicalVFXVariants CopyVFX(const FGridPlayerAttackVFXDefinition& Source)
	{
		FGridCanonicalVFXVariants Result;
		Result.Systems = Source.Systems;
		Result.bAttachToHeldItem = Source.bAttachToHeldItem;
		Result.SocketName = Source.SocketName;
		Result.LocationOffset = Source.LocationOffset;
		Result.RotationOffset = Source.RotationOffset;
		Result.Scale = Source.Scale;
		return Result;
	}
}

bool GridArmClean06PresentationProjection::TryProject(
	FName ExplicitPresentationId, const FGridPlayerAttackPresentationProfile& Source,
	FGridCanonicalActionPresentation& OutPresentation)
{
	OutPresentation = FGridCanonicalActionPresentation();
	if (ExplicitPresentationId.IsNone() || !Source.IsValid())
	{
		return false;
	}
	// Only the three legacy channels exist: preserve exact variant ordering,
	// pitches, transforms and timing. Other canonical channels remain empty.
	OutPresentation.PresentationId = ExplicitPresentationId;
	OutPresentation.MotionStyle = Source.MotionStyle;
	OutPresentation.bAnimateHeldItem = Source.bAnimateHeldItem;
	OutPresentation.MotionDurationSeconds = Source.MotionDurationSeconds;
	OutPresentation.ImpactDelaySeconds = Source.ImpactDelaySeconds;
	OutPresentation.RecoveryDelaySeconds = Source.RecoveryDelaySeconds;
	OutPresentation.PeakLocationOffset = Source.PeakLocationOffset;
	OutPresentation.PeakRotationOffset = Source.PeakRotationOffset;
	OutPresentation.FeedbackDurationSeconds = Source.FeedbackDurationSeconds;
	OutPresentation.AttackAudio = CopyAudio(Source.AttackAudio);
	OutPresentation.ImpactHitAudio = CopyAudio(Source.ImpactHitAudio);
	OutPresentation.ImpactMissAudio = CopyAudio(Source.ImpactMissAudio);
	OutPresentation.AttackVFX = CopyVFX(Source.AttackVFX);
	OutPresentation.ImpactHitVFX = CopyVFX(Source.ImpactHitVFX);
	OutPresentation.ImpactMissVFX = CopyVFX(Source.ImpactMissVFX);
	if (!OutPresentation.IsValid())
	{
		OutPresentation = FGridCanonicalActionPresentation();
		return false;
	}
	return true;
}

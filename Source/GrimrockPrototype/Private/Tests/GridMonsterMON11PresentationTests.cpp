#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "NiagaraSystem.h"
#include "Runtime/Combat/GridPlayerAttackPresentationComponent.h"
#include "Runtime/Combat/GridPlayerAttackPresentationTypes.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Sound/SoundWave.h"

namespace
{
	FGridPlayerAttackRequest MakePresentationRequest()
	{
		FGridPlayerAttackRequest Request;
		Request.RequestId = FGuid::NewGuid();
		Request.RoundNumber = 1;
		Request.AttackerCharacterIndex = 0;
		Request.AttackerCharacterId = FGuid::NewGuid();
		Request.TargetMonsterId = FGuid::NewGuid();
		Request.PartyFacing = EGridEdge::North;
		Request.RangeCells = 1;
		Request.AttackId = TEXT("Attack_Unarmed");
		return Request;
	}

	UGridPlayerAttackPresentationComponent* MakePresentationComponent(UGridTurnManagerComponent*& OutTurnManager)
	{
		AGridLevelRuntimeActor* Runtime = NewObject<AGridLevelRuntimeActor>();
		OutTurnManager = NewObject<UGridTurnManagerComponent>(Runtime);
		UGridPlayerAttackPresentationComponent* Presentation = NewObject<UGridPlayerAttackPresentationComponent>(Runtime);
		Presentation->bNativeAudioPlaybackEnabled = false;
		Presentation->bNativeVFXSpawnEnabled = false;
		Presentation->bNativeFeedbackEnabled = false;
		Presentation->InitializePresentation(OutTurnManager);
		return Presentation;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON114ProfileValidationTest, "Grimrock.Monsters.MON11.Presentation.ProfileValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON114ProfileValidationTest::RunTest(const FString& Parameters)
{
	FGridPlayerAttackPresentationProfile Empty;
	TestTrue(TEXT("Empty profile is valid"), Empty.IsValid());

	USoundWave* Sound = NewObject<USoundWave>();
	UNiagaraSystem* System = NewObject<UNiagaraSystem>();
	Empty.AttackAudio.Sounds.Add(Sound);
	Empty.AttackVFX.Systems.Add(System);
	TestTrue(TEXT("Transient media are valid"), Empty.IsValid());

	FGridPlayerAttackPresentationProfile Invalid = Empty;
	Invalid.AttackAudio.VolumeMultiplier = -1.0f;
	TestFalse(TEXT("Negative volume is invalid"), Invalid.IsValid());
	Invalid = Empty;
	Invalid.AttackAudio.PitchMin = 0.0f;
	TestFalse(TEXT("Non-positive pitch is invalid"), Invalid.IsValid());
	Invalid = Empty;
	Invalid.bAnimateHeldItem = true;
	Invalid.MotionDurationSeconds = 2.01f;
	TestFalse(TEXT("Motion duration is bounded"), Invalid.IsValid());
	Invalid = Empty;
	Invalid.ImpactDelaySeconds = 0.0f;
	TestFalse(TEXT("Multi-strike impact must occur after attack"), Invalid.IsValid());
	Invalid = Empty;
	Invalid.RecoveryDelaySeconds = -0.01f;
	TestFalse(TEXT("Negative multi-strike recovery is invalid"), Invalid.IsValid());
	Invalid = Empty;
	Invalid.AttackVFX.Scale.X = 0.0f;
	TestFalse(TEXT("VFX scale must be positive"), Invalid.IsValid());
	Invalid = Empty;
	Invalid.AttackAudio.Sounds.Add(TSoftObjectPtr<USoundBase>());
	TestFalse(TEXT("Explicit null audio is invalid"), Invalid.IsValid());

	UGridItemDefinitionAsset* Item = NewObject<UGridItemDefinitionAsset>();
	Item->ItemDefinitionId = TEXT("Item_Test");
	Item->bProvidesAttackPresentation = true;
	Item->PlayerAttackPresentationProfile = Empty;
	TestTrue(TEXT("Item helper accepts the presentation"), Item->HasValidPlayerAttackPresentation());
	Item->PlayerAttackPresentationProfile.FeedbackDurationSeconds = 0.0f;
	TestFalse(TEXT("Item helper rejects invalid presentation"), Item->HasValidPlayerAttackPresentation());
	TestTrue(TEXT("Presentation never invalidates gameplay definition"), Item->IsValidDefinition());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMON114EventOrderTest, "Grimrock.Monsters.MON11.Presentation.EventOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON114EventOrderTest::RunTest(const FString& Parameters)
{
	UGridTurnManagerComponent* TurnManager = nullptr;
	UGridPlayerAttackPresentationComponent* Presentation = MakePresentationComponent(TurnManager);
	const FGridPlayerAttackRequest Request = MakePresentationRequest();
	FGridAttackResult Result;
	Result.bHit = true;
	Result.RawDamage = 3;
	Result.HealthDamage = 3;
	Result.TargetHealthBefore = 5;
	Result.TargetHealthAfter = 2;

	TurnManager->OnPlayerAttackRequested.Broadcast(Request);
	TestEqual(TEXT("Exactly one Attack"), Presentation->PresentationAttackCount, 1);
	TestEqual(TEXT("No impact before resolution"), Presentation->PresentationImpactHitCount, 0);
	TurnManager->OnPlayerAttackResolved.Broadcast(Request, nullptr, Result);
	TestEqual(TEXT("Exactly one hit impact"), Presentation->PresentationImpactHitCount, 1);
	TestEqual(TEXT("Exactly one feedback"), Presentation->FeedbackCount, 1);
	TestEqual(TEXT("Impact follows Attack sequence"), Presentation->LastPresentationRequest.SequenceNumber, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON114AudioVFXMappingTest, "Grimrock.Monsters.MON11.Presentation.AudioVFXMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON114AudioVFXMappingTest::RunTest(const FString& Parameters)
{
	FGridPlayerAttackPresentationProfile Profile;
	USoundWave* AttackSound = NewObject<USoundWave>();
	USoundWave* HitSound = NewObject<USoundWave>();
	USoundWave* MissSound = NewObject<USoundWave>();
	UNiagaraSystem* AttackSystem = NewObject<UNiagaraSystem>();
	UNiagaraSystem* HitSystem = NewObject<UNiagaraSystem>();
	UNiagaraSystem* MissSystem = NewObject<UNiagaraSystem>();
	Profile.AttackAudio.Sounds.Add(AttackSound);
	Profile.ImpactHitAudio.Sounds.Add(HitSound);
	Profile.ImpactMissAudio.Sounds.Add(MissSound);
	Profile.AttackVFX.Systems.Add(AttackSystem);
	Profile.ImpactHitVFX.Systems.Add(HitSystem);
	Profile.ImpactMissVFX.Systems.Add(MissSystem);
	Profile.AttackAudio.PitchMin = 0.8f;
	Profile.AttackAudio.PitchMax = 1.2f;
	TestTrue(TEXT("Mapped transient profile is valid"), Profile.IsValid());
	TestTrue(TEXT("Pitch lower bound retained"), Profile.AttackAudio.PitchMin <= 1.0f);
	TestTrue(TEXT("Pitch upper bound retained"), Profile.AttackAudio.PitchMax >= 1.0f);
	TestNotEqual(TEXT("Attack and hit audio are exclusive"), Profile.AttackAudio.Sounds[0].Get(), Profile.ImpactHitAudio.Sounds[0].Get());
	TestNotEqual(TEXT("Hit and miss VFX are exclusive"), Profile.ImpactHitVFX.Systems[0].Get(), Profile.ImpactMissVFX.Systems[0].Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON114HeldItemMotionTest, "Grimrock.Monsters.MON11.Presentation.HeldItemMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON114HeldItemMotionTest::RunTest(const FString& Parameters)
{
	UGridTurnManagerComponent* TurnManager = nullptr;
	UGridPlayerAttackPresentationComponent* Presentation = MakePresentationComponent(TurnManager);
	FGridPlayerAttackRequest Request = MakePresentationRequest();
	TurnManager->OnPlayerAttackRequested.Broadcast(Request);
	TestFalse(TEXT("Unarmed attack never moves an item"), Presentation->IsHeldItemMotionActive());
	TestFalse(TEXT("Unarmed attack reports no started movement"), Presentation->bHeldItemMotionStarted);
	Presentation->ResetTransientPresentationState();
	TestFalse(TEXT("Reset leaves Tick disabled"), Presentation->IsComponentTickEnabled());
	TestEqual(TEXT("Reset does not destroy presentation component"), Presentation->IsValidLowLevel(), true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON114FeedbackAndRejectionsTest, "Grimrock.Monsters.MON11.Presentation.FeedbackAndRejections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON114FeedbackAndRejectionsTest::RunTest(const FString& Parameters)
{
	const UEnum* RejectEnum = StaticEnum<EGridPlayerAttackRejectReason>();
	for (int32 Index = 0; Index < RejectEnum->NumEnums() - 1; ++Index)
	{
		const auto Reason = static_cast<EGridPlayerAttackRejectReason>(RejectEnum->GetValueByIndex(Index));
		TestFalse(*FString::Printf(TEXT("Reject text %d is non-empty"), Index), FGridPlayerAttackFeedbackFormatter::FormatRejectReason(Reason).IsEmpty());
	}

	FGridAttackResult Result;
	Result.bHit = true;
	Result.bCriticalHit = true;
	Result.PhysicalArmorDamage = 2;
	Result.MagicalArmorDamage = 1;
	Result.HealthDamage = 3;
	Result.TargetHealthBefore = 4;
	Result.TargetHealthAfter = 1;
	const FGridPlayerAttackFeedbackRequest Critical = FGridPlayerAttackFeedbackFormatter::FormatResolved(
		MakePresentationRequest(), Result, FText::FromString(TEXT("Aëlric")), FText::FromString(TEXT("Rat géant")), 1.25f);
	TestEqual(TEXT("Critical outcome"), Critical.Outcome, EGridPlayerAttackFeedbackOutcome::CriticalHit);
	TestFalse(TEXT("Damage detail is populated"), Critical.DetailText.IsEmpty());

	UGridTurnManagerComponent* TurnManager = nullptr;
	UGridPlayerAttackPresentationComponent* Presentation = MakePresentationComponent(TurnManager);
	TurnManager->OnPlayerAttackRejected.Broadcast(0, EGridPlayerAttackRejectReason::PassageBlocked);
	TestEqual(TEXT("One rejected feedback"), Presentation->FeedbackCount, 1);
	TestEqual(TEXT("No attack media on rejection"), Presentation->AudioPlaybackRequestCount, 0);
	TestEqual(TEXT("No VFX on rejection"), Presentation->VFXSpawnRequestCount, 0);
	TestEqual(TEXT("No attack event on rejection"), Presentation->PresentationAttackCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON114TargetReactionExclusivityTest, "Grimrock.Monsters.MON11.Presentation.TargetReactionExclusivity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON114TargetReactionExclusivityTest::RunTest(const FString& Parameters)
{
	AGridMonsterActor* Monster = NewObject<AGridMonsterActor>();
	UGridMonsterMovementComponent* Movement = NewObject<UGridMonsterMovementComponent>(Monster, TEXT("MON114PresentationMovement"));
	Movement->bAutoInitialize = false;
	Monster->AddInstanceComponent(Movement);
	Monster->CurrentHealth = 8;
	Monster->CurrentPhysicalArmor = 0;
	Monster->CurrentMagicalArmor = 0;
	FGridAttackResult Miss;
	Miss.bHit = false;
	Miss.HealthDamage = 8;
	Monster->ApplyAttackResult(Miss);
	TestEqual(TEXT("Miss does not mutate health"), Monster->CurrentHealth, 8);

	FGridAttackResult Hit;
	Hit.bHit = true;
	Hit.HealthDamage = 3;
	Monster->ApplyAttackResult(Hit);
	TestEqual(TEXT("Non-fatal hit applies once"), Monster->CurrentHealth, 5);
	TestFalse(TEXT("Non-fatal hit is not death"), Monster->IsDead());

	FGridAttackResult Fatal;
	Fatal.bHit = true;
	Fatal.HealthDamage = 5;
	Monster->ApplyAttackResult(Fatal);
	TestEqual(TEXT("Fatal hit reaches zero"), Monster->CurrentHealth, 0);
	TestTrue(TEXT("Fatal hit uses existing death path"), Monster->IsDead());

	UGridTurnManagerComponent* TurnManager = nullptr;
	UGridPlayerAttackPresentationComponent* Presentation = MakePresentationComponent(TurnManager);
	const FGridPlayerAttackRequest Request = MakePresentationRequest();
	TurnManager->OnPlayerAttackRequested.Broadcast(Request);
	TurnManager->OnPlayerAttackResolved.Broadcast(Request, Monster, Fatal);
	TestEqual(TEXT("Player impact remains unique"), Presentation->PresentationImpactHitCount, 1);
	TestEqual(TEXT("Feedback works without media"), Presentation->FeedbackCount, 1);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON114SequencedStrikePresentationTest,
	"Grimrock.RPG.ARMEMENT.LOT02.19.2.2.SequencedStrikePresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON114SequencedStrikePresentationTest::RunTest(const FString&)
{
	UGridTurnManagerComponent* TurnManager = nullptr;
	UGridPlayerAttackPresentationComponent* Presentation = MakePresentationComponent(TurnManager);
	FGridPlayerAttackRequest Main = MakePresentationRequest();
	Main.AttackId = TEXT("Attack_Dagger_01");
	Main.ActionInstanceId = Main.RequestId;
	Main.ResolutionCount = 2;
	Main.ResolutionIndex = 0;
	FGridPlayerAttackRequest Off = Main;
	Off.RequestId = FGuid::NewGuid();
	Off.AttackId = TEXT("Attack_MainGauche_01");
	Off.ResolutionIndex = 1;
	FGridAttackResult MainResult;
	MainResult.bHit = true;
	MainResult.RawDamage = 5;
	MainResult.HealthDamage = 3;
	MainResult.TargetHealthBefore = 20;
	MainResult.TargetHealthAfter = 17;
	FGridAttackResult OffResult = MainResult;
	OffResult.RawDamage = 2;
	OffResult.HealthDamage = 2;
	OffResult.TargetHealthBefore = 17;
	OffResult.TargetHealthAfter = 15;

	TurnManager->OnPlayerAttackRequested.Broadcast(Main);
	TestEqual(TEXT("First weapon starts immediately"), Presentation->PresentationAttackCount, 1);
	TestTrue(TEXT("Multi-hit combat feed is gated until a visible impact"),
		TurnManager->IsSequencedAttackFeedPending());
	TurnManager->OnPlayerAttackResolved.Broadcast(Main, nullptr, MainResult);
	TurnManager->OnPlayerAttackRequested.Broadcast(Off);
	TurnManager->OnPlayerAttackResolved.Broadcast(Off, nullptr, OffResult);
	TestEqual(TEXT("Second physical weapon has not started yet"), Presentation->PresentationAttackCount, 1);
	TestEqual(TEXT("No audiovisual impact before the windup completes"), Presentation->PresentationImpactHitCount, 0);
	TestEqual(TEXT("No feedback emitted while both hits wait for presentation"), Presentation->FeedbackCount, 0);
	TestEqual(TEXT("Two authoritative strike results are queued"), Presentation->GetQueuedStrikeCount(), 2);

	Presentation->AdvanceSequencedPresentation(0.17f);
	TestEqual(TEXT("First hit lands before second weapon starts"), Presentation->PresentationImpactHitCount, 1);
	TestEqual(TEXT("Second attack sound still not requested"), Presentation->PresentationAttackCount, 1);
	TestEqual(TEXT("Only first combat feedback is visible"), Presentation->FeedbackCount, 1);
	TestEqual(TEXT("First hit ID is preserved"), Presentation->LastPresentationRequest.AttackRequest.AttackId, Main.AttackId);
	TestTrue(TEXT("Turn manager still awaits the second visual strike"),
		TurnManager->IsSequencedAttackFeedPending());

	Presentation->AdvanceSequencedPresentation(0.36f);
	TestEqual(TEXT("Second weapon starts after audible minimum separation"), Presentation->PresentationAttackCount, 2);
	TestEqual(TEXT("No second impact at the moment its attack begins"), Presentation->PresentationImpactHitCount, 1);
	TestEqual(TEXT("OffHand attack ID stays distinct"), Presentation->LastPresentationRequest.AttackRequest.AttackId, Off.AttackId);
	Presentation->AdvanceSequencedPresentation(0.17f);
	TestEqual(TEXT("Second hit receives a separate visual impact"), Presentation->PresentationImpactHitCount, 2);
	TestEqual(TEXT("Second strike gets its own damage feedback"), Presentation->FeedbackCount, 2);
	TestEqual(TEXT("Second impact identifies the OffHand attack"), Presentation->LastPresentationRequest.AttackRequest.AttackId, Off.AttackId);
	Presentation->AdvanceSequencedPresentation(0.18f);
	TestEqual(TEXT("Queue drains after second recovery"), Presentation->GetQueuedStrikeCount(), 0);
	TestFalse(TEXT("Feed gate releases after the final impact"), TurnManager->IsSequencedAttackFeedPending());

	// If the first strike killed the target, gameplay authoritatively omits
	// the second request. Neither phantom sounds nor phantom damage appear.
	FGridPlayerAttackRequest Lethal = MakePresentationRequest();
	Lethal.ActionInstanceId = Lethal.RequestId;
	Lethal.ResolutionCount = 2;
	Lethal.ResolutionIndex = 0;
	FGridAttackResult LethalResult = MainResult;
	LethalResult.TargetHealthAfter = 0;
	TurnManager->OnPlayerAttackRequested.Broadcast(Lethal);
	TurnManager->OnPlayerAttackResolved.Broadcast(Lethal, nullptr, LethalResult);
	TurnManager->OnCombatEnded.Broadcast(EGridCombatPhase::Victory);
	Presentation->AdvanceSequencedPresentation(0.17f);
	TestEqual(TEXT("Lethal first hit emits its real impact"), Presentation->PresentationImpactHitCount, 3);
	TestEqual(TEXT("No phantom OffHand attack was enqueued"), Presentation->PresentationAttackCount, 3);
	Presentation->AdvanceSequencedPresentation(0.18f);
	TestEqual(TEXT("Victory does not strand the presentation queue"), Presentation->GetQueuedStrikeCount(), 0);
	TestFalse(TEXT("Victory flushes pending combat-feed messages"),
		TurnManager->IsSequencedAttackFeedPending());
	return true;
}

#endif

#include "Runtime/Monsters/GridMonsterCombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/Combat/GridCombatProjectileActor.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridMonsterCombat, Log, All);

namespace
{
	FVector ResolveProjectileSourceWorldLocation(const AGridMonsterActor* Monster, FName SourceSocketName, const FVector& SourceOffset)
	{
		if (IsValid(Monster) && IsValid(Monster->SkeletalMeshComponent) && Monster->SkeletalMeshComponent->IsRegistered())
		{
			USkeletalMeshComponent* SkeletalMeshComponent = Monster->SkeletalMeshComponent;

			if (!SourceSocketName.IsNone())
			{
				if (SkeletalMeshComponent->DoesSocketExist(SourceSocketName))
				{
					const FTransform SocketTransform = SkeletalMeshComponent->GetSocketTransform(SourceSocketName, RTS_World);
					return SocketTransform.TransformPositionNoScale(SourceOffset);
				}

				UE_LOG(LogGridMonsterCombat, Warning, TEXT("[GridMonsterProjectile] Source socket unavailable Monster=%s Socket=%s Fallback=BoundsCenter"),
					*GetNameSafe(Monster), *SourceSocketName.ToString());
			}

			return SkeletalMeshComponent->Bounds.Origin + SkeletalMeshComponent->GetComponentQuat().RotateVector(SourceOffset);
		}

		if (IsValid(Monster))
		{
			return Monster->GetActorLocation() + Monster->GetActorQuat().RotateVector(SourceOffset);
		}

		return FVector::ZeroVector;
	}

	void LaunchMonsterProjectilePresentation(TWeakObjectPtr<UGridMonsterCombatComponent> WeakCombatComponent, TWeakObjectPtr<AGridMonsterActor> WeakMonster,
		TWeakObjectPtr<AGrimrockPartyPawn> WeakPartyPawn, TSoftObjectPtr<UStaticMesh> ProjectileVisualMesh, FVector ProjectileVisualScale,
		FRotator ProjectileRotationOffset, FName ProjectileSourceSocketName, FVector ProjectileSourceOffset, float ProjectileTravelDuration, FName AttackId)
	{
		UGridMonsterCombatComponent* CombatComponent = WeakCombatComponent.Get();
		AGridMonsterActor* Monster = WeakMonster.Get();
		AGrimrockPartyPawn* PartyPawn = WeakPartyPawn.Get();
		if (!IsValid(CombatComponent) || !CombatComponent->bAttackPresentationActive || CombatComponent->LastAttackId != AttackId || !IsValid(Monster) ||
			Monster->IsDead() || !IsValid(PartyPawn))
		{
			return;
		}

		UStaticMesh* ProjectileMesh = ProjectileVisualMesh.LoadSynchronous();
		if (!IsValid(ProjectileMesh))
		{
			UE_LOG(LogGridMonsterCombat, Warning, TEXT("[GridMonsterProjectile] Presentation skipped Monster=%s Attack=%s Reason=MissingProjectileVisualMesh"),
				*GetNameSafe(Monster), *AttackId.ToString());
			return;
		}

		UWorld* World = Monster->GetWorld();
		if (!IsValid(World))
		{
			return;
		}

		const FVector SourceWorldLocation = ResolveProjectileSourceWorldLocation(Monster, ProjectileSourceSocketName, ProjectileSourceOffset);
		const FVector TargetWorldLocation = PartyPawn->GetActorLocation();

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = Monster;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AGridCombatProjectileActor* Projectile = World->SpawnActor<AGridCombatProjectileActor>(
			AGridCombatProjectileActor::StaticClass(), SourceWorldLocation, FRotator::ZeroRotator, SpawnParameters);
		if (!IsValid(Projectile))
		{
			UE_LOG(LogGridMonsterCombat, Warning, TEXT("[GridMonsterProjectile] Presentation skipped Monster=%s Attack=%s Reason=SpawnFailed"),
				*GetNameSafe(Monster), *AttackId.ToString());
			return;
		}

		if (!Projectile->InitializeProjectilePresentation(
				ProjectileMesh, SourceWorldLocation, TargetWorldLocation, ProjectileTravelDuration, ProjectileVisualScale, ProjectileRotationOffset))
		{
			Projectile->Destroy();
			UE_LOG(LogGridMonsterCombat, Warning, TEXT("[GridMonsterProjectile] Presentation skipped Monster=%s Attack=%s Reason=InitializationFailed"),
				*GetNameSafe(Monster), *AttackId.ToString());
			return;
		}

		UE_LOG(LogGridMonsterCombat, Log,
			TEXT("[GridMonsterProjectile] Launched Monster=%s Attack=%s Travel=%.3f SourceSocket=%s Source=(%.1f,%.1f,%.1f) Target=(%.1f,%.1f,%.1f)"),
			*GetNameSafe(Monster), *AttackId.ToString(), ProjectileTravelDuration,
			ProjectileSourceSocketName.IsNone() ? TEXT("None") : *ProjectileSourceSocketName.ToString(), SourceWorldLocation.X, SourceWorldLocation.Y,
			SourceWorldLocation.Z, TargetWorldLocation.X, TargetWorldLocation.Y, TargetWorldLocation.Z);
	}
}

UGridMonsterCombatComponent::UGridMonsterCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGridMonsterCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoInitialize)
	{
		InitializeCombat(nullptr);
	}
	RefreshTurnManagerBinding();
}

void UGridMonsterCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindTurnManagerEvents();
	Super::EndPlay(EndPlayReason);
}

bool UGridMonsterCombatComponent::InitializeCombat(AGrimrockPartyPawn* InPartyPawn)
{
	AGridMonsterActor* CandidateOwner = Cast<AGridMonsterActor>(GetOwner());
	AGrimrockPartyPawn* CandidateParty = IsValid(InPartyPawn) ? InPartyPawn : FindPartyPawn();

	if (!IsValid(CandidateOwner) || !IsValid(CandidateParty))
	{
		UE_LOG(LogGridMonsterCombat, Warning, TEXT("[GridMonsterCombat] Initialization failed. Owner=%s Party=%s"), *GetNameSafe(CandidateOwner),
			*GetNameSafe(CandidateParty));
		return false;
	}

	OwnerMonster = CandidateOwner;
	PartyPawn = CandidateParty;
	bInitialized = true;
	RefreshTurnManagerBinding();
	return true;
}

bool UGridMonsterCombatComponent::GetPreferredAttackForRange(int32 DistanceCells, FGridMonsterAttackDefinition& OutAttack) const
{
	const_cast<UGridMonsterCombatComponent*>(this)->EnsureCurrentCombatTurnObserved();

	OutAttack = FGridMonsterAttackDefinition();
	if (DistanceCells <= 0 || !IsValid(OwnerMonster) || !IsValid(OwnerMonster->MonsterDefinition))
	{
		return false;
	}

	const FGridMonsterAttackDefinition* BestAttack = nullptr;
	for (const FGridMonsterAttackDefinition& Attack : OwnerMonster->MonsterDefinition->Attacks)
	{
		if (!Attack.IsValidDefinition() || !Attack.SupportsDistance(DistanceCells) || !AttackCooldownState.IsAttackAvailable(Attack))
		{
			continue;
		}

		if (!BestAttack || Attack.Priority > BestAttack->Priority)
		{
			BestAttack = &Attack;
		}
	}

	if (!BestAttack)
	{
		return false;
	}

	OutAttack = *BestAttack;
	return true;
}

bool UGridMonsterCombatComponent::GetPreferredMeleeAttack(FGridMonsterAttackDefinition& OutAttack) const
{
	return GetPreferredAttackForRange(1, OutAttack);
}

int32 UGridMonsterCombatComponent::SelectPartyTarget(FRandomStream& RandomStream) const
{
	if (!IsValid(PartyPawn) || !IsValid(PartyPawn->PartyInventoryComponent))
	{
		return INDEX_NONE;
	}

	return FGridPartyTargetSelector::SelectTarget(PartyPawn->PartyInventoryComponent->PartyInventoryState, RandomStream, FrontLineSlotCount);
}

bool UGridMonsterCombatComponent::ResolveAndApplyPartyAttack(
	int32 TargetCharacterIndex, const FGridMonsterAttackDefinition& Attack, FRandomStream& RandomStream, FGridAttackResult& OutResult)
{
	OutResult = FGridAttackResult();
	if (!bInitialized && !InitializeCombat(nullptr))
	{
		return false;
	}

	EnsureCurrentCombatTurnObserved();
	if (AttackCooldownState.IsOnCooldown(Attack.AttackId))
	{
		UE_LOG(LogGridMonsterCombat, Warning, TEXT("[GridMonsterCooldown] Attack rejected Monster=%s Attack=%s Reason=Cooldown"), *GetNameSafe(OwnerMonster),
			*Attack.AttackId.ToString());
		return false;
	}

	if (!IsValid(OwnerMonster) || OwnerMonster->IsDead() || !IsValid(OwnerMonster->MonsterDefinition) || !IsValid(PartyPawn) ||
		!IsValid(PartyPawn->PartyInventoryComponent) || !Attack.IsValidDefinition())
	{
		return false;
	}

	FGridPartyInventoryState& PartyState = PartyPawn->PartyInventoryComponent->PartyInventoryState;
	if (!PartyState.ActiveCharacters.IsValidIndex(TargetCharacterIndex))
	{
		UE_LOG(LogGridMonsterCombat, Warning, TEXT("[GridMonsterCombat] Invalid party target. Monster=%s TargetIndex=%d"), *GetNameSafe(OwnerMonster),
			TargetCharacterIndex);
		return false;
	}

	FGridCharacterInventoryState& Character = PartyState.ActiveCharacters[TargetCharacterIndex];
	if (Character.DerivedStats.CurrentHealth <= 0)
	{
		return false;
	}

	FGridAttackSourceStats Source;
	Source.Accuracy = OwnerMonster->MonsterDefinition->Accuracy;
	Source.DamageBonus = Attack.DamageBonus;

	const FGridDamageResistanceSet Resistances = PartyPawn->PartyInventoryComponent->ComputeCharacterEquipmentResistances(TargetCharacterIndex);

	FGridAttackTargetStats Target;
	Target.Evasion = Character.DerivedStats.Evasion;
	Target.CurrentHealth = Character.DerivedStats.CurrentHealth;
	Target.PhysicalArmor = Character.DerivedStats.PhysicalArmor;
	Target.MagicalArmor = Character.DerivedStats.MagicalArmor;
	Target.ResistancePercent = GetResistancePercent(Resistances, Attack.DamageType);
	Target.DamageMultiplier = 1.0f;

	FGridAttackDefinition GenericAttack;
	GenericAttack.DamageType = Attack.DamageType;
	GenericAttack.PhysicalSubtype = Attack.PhysicalSubtype;
	GenericAttack.MinDamage = Attack.MinDamage;
	GenericAttack.MaxDamage = Attack.MaxDamage;
	GenericAttack.AccuracyBonus = Attack.AccuracyBonus;

	OutResult = FGridCombatResolver::ResolveAttack(Source, Target, GenericAttack, RandomStream);

	if (OutResult.bHit)
	{
		Character.DerivedStats.PhysicalArmor = FMath::Max(0, Character.DerivedStats.PhysicalArmor - OutResult.PhysicalArmorDamage);
		Character.DerivedStats.MagicalArmor = FMath::Max(0, Character.DerivedStats.MagicalArmor - OutResult.MagicalArmorDamage);
		Character.DerivedStats.CurrentHealth = FMath::Max(0, Character.DerivedStats.CurrentHealth - OutResult.HealthDamage);
	}

	LastAttackId = Attack.AttackId;
	LastTargetCharacterIndex = TargetCharacterIndex;
	LastAttackResult = OutResult;

	if (AttackCooldownState.CommitAttack(Attack))
	{
		UE_LOG(LogGridMonsterCombat, Verbose, TEXT("[GridMonsterCooldown] Started Monster=%s Attack=%s CooldownTurns=%d TurnSerial=%d"),
			*GetNameSafe(OwnerMonster), *Attack.AttackId.ToString(), Attack.CooldownTurns, AttackCooldownState.GetCurrentTurnSerial());
	}

	UE_LOG(LogGridMonsterCombat, Log,
		TEXT(
			"[GridMonsterCombat] Attack Monster=%s Attack=%s Target=%d Name=%s Natural=%d Total=%d Defense=%d Hit=%s Critical=%s Raw=%d ArmorPhysical=%d ArmorMagical=%d Health=%d HP=%d->%d"),
		*GetNameSafe(OwnerMonster), *Attack.AttackId.ToString(), TargetCharacterIndex, *Character.DisplayName.ToString(), OutResult.NaturalAttackRoll,
		OutResult.AttackRoll, OutResult.DefenseValue, OutResult.bHit ? TEXT("true") : TEXT("false"), OutResult.bCriticalHit ? TEXT("true") : TEXT("false"),
		OutResult.RawDamage, OutResult.PhysicalArmorDamage, OutResult.MagicalArmorDamage, OutResult.HealthDamage, OutResult.TargetHealthBefore,
		OutResult.TargetHealthAfter);

	return true;
}

bool UGridMonsterCombatComponent::StartAttackPresentation(const FGridCombatAction& Action, const FGridMonsterAttackDefinition& Attack)
{
	EnsureCurrentCombatTurnObserved();
	if ((!bInitialized && !InitializeCombat(nullptr)) || !IsValid(OwnerMonster) || OwnerMonster->IsDead() || bAttackPresentationActive ||
		!Attack.IsValidDefinition() || AttackCooldownState.IsOnCooldown(Attack.AttackId))
	{
		return false;
	}

	LastAttackId = Attack.AttackId;
	LastTargetCharacterIndex = Action.TargetCharacterIndex;
	bAttackPresentationActive = true;
	OwnerMonster->SetMonsterState(EGridMonsterState::Attacking);
	if (OwnerMonster->AudioComponent)
	{
		OwnerMonster->AudioComponent->PlayAttack(Attack);
	}
	if (OwnerMonster->VFXComponent)
	{
		OwnerMonster->VFXComponent->PlayAttackVFX(Attack);
	}

	UAnimMontage* Montage = Attack.AttackMontage.LoadSynchronous();
	UAnimInstance* AnimInstance = OwnerMonster->SkeletalMeshComponent ? OwnerMonster->SkeletalMeshComponent->GetAnimInstance() : nullptr;
	if (Montage && AnimInstance)
	{
		AnimInstance->Montage_Play(Montage, 1.0f);
	}

	if (Action.Type == EGridCombatActionType::RangedAttack && Attack.Delivery == EGridMonsterAttackDelivery::Projectile &&
		!Attack.ProjectileVisualMesh.IsNull())
	{
		const float LaunchDelay = AGridCombatProjectileActor::CalculateLaunchDelay(Attack.ImpactTimeSeconds, Attack.ProjectileTravelDuration);

		const TWeakObjectPtr<UGridMonsterCombatComponent> WeakCombatComponent(this);
		const TWeakObjectPtr<AGridMonsterActor> WeakMonster(OwnerMonster);
		const TWeakObjectPtr<AGrimrockPartyPawn> WeakPartyPawn(PartyPawn);
		const TSoftObjectPtr<UStaticMesh> ProjectileVisualMesh = Attack.ProjectileVisualMesh;
		const FVector ProjectileVisualScale = Attack.ProjectileVisualScale;
		const FRotator ProjectileRotationOffset = Attack.ProjectileRotationOffset;
		const FName ProjectileSourceSocketName = Attack.ProjectileSourceSocketName;
		const FVector ProjectileSourceOffset = Attack.ProjectileSourceOffset;
		const float ProjectileTravelDuration = Attack.ProjectileTravelDuration;
		const FName ProjectileAttackId = Attack.AttackId;

		FTimerDelegate LaunchDelegate;
		LaunchDelegate.BindLambda(
			[WeakCombatComponent, WeakMonster, WeakPartyPawn, ProjectileVisualMesh, ProjectileVisualScale, ProjectileRotationOffset, ProjectileSourceSocketName,
				ProjectileSourceOffset, ProjectileTravelDuration, ProjectileAttackId]()
			{
				LaunchMonsterProjectilePresentation(WeakCombatComponent, WeakMonster, WeakPartyPawn, ProjectileVisualMesh, ProjectileVisualScale,
					ProjectileRotationOffset, ProjectileSourceSocketName, ProjectileSourceOffset, ProjectileTravelDuration, ProjectileAttackId);
			});

		if (LaunchDelay <= KINDA_SMALL_NUMBER)
		{
			LaunchDelegate.ExecuteIfBound();
		}
		else if (UWorld* World = GetWorld())
		{
			FTimerHandle TimerHandle;
			World->GetTimerManager().SetTimer(TimerHandle, LaunchDelegate, LaunchDelay, false);
		}
	}

	return true;
}

void UGridMonsterCombatComponent::NotifyAttackImpact()
{
	if (bAttackPresentationActive)
	{
		OnAttackImpactNotify.Broadcast();
	}
}

void UGridMonsterCombatComponent::NotifyActionComplete()
{
	if (!bAttackPresentationActive)
	{
		return;
	}

	bAttackPresentationActive = false;
	if (IsValid(OwnerMonster) && !OwnerMonster->IsDead())
	{
		OwnerMonster->SetMonsterState(EGridMonsterState::Pursuing);
	}
	OnActionCompleteNotify.Broadcast();
}

void UGridMonsterCombatComponent::CancelAttackPresentation()
{
	if (IsValid(OwnerMonster) && OwnerMonster->SkeletalMeshComponent)
	{
		if (UAnimInstance* AnimInstance = OwnerMonster->SkeletalMeshComponent->GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.10f);
		}
	}

	bAttackPresentationActive = false;
	if (IsValid(OwnerMonster) && !OwnerMonster->IsDead())
	{
		OwnerMonster->SetMonsterState(EGridMonsterState::Pursuing);
	}
}

void UGridMonsterCombatComponent::LogCombatState() const
{
	UE_LOG(LogGridMonsterCombat, Log,
		TEXT("[GridMonsterCombat] Initialized=%s Owner=%s Party=%s Active=%s Attack=%s Target=%d Hit=%s Damage=%d CooldownTurn=%d"),
		bInitialized ? TEXT("true") : TEXT("false"), *GetNameSafe(OwnerMonster), *GetNameSafe(PartyPawn),
		bAttackPresentationActive ? TEXT("true") : TEXT("false"), *LastAttackId.ToString(), LastTargetCharacterIndex,
		LastAttackResult.bHit ? TEXT("true") : TEXT("false"), LastAttackResult.GetTotalAppliedDamage(), AttackCooldownState.GetCurrentTurnSerial());
}

void UGridMonsterCombatComponent::ResetAttackCooldowns()
{
	AttackCooldownState.Reset();
	LastObservedCombatRound = INDEX_NONE;
}

void UGridMonsterCombatComponent::RefreshTurnManagerBinding()
{
	UGridTurnManagerComponent* CandidateTurnManager = nullptr;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AGridLevelRuntimeActor> It(World); It; ++It)
		{
			if (UGridTurnManagerComponent* TurnManager = It->FindComponentByClass<UGridTurnManagerComponent>())
			{
				CandidateTurnManager = TurnManager;
				break;
			}
		}
	}

	if (BoundTurnManager == CandidateTurnManager)
	{
		return;
	}

	UnbindTurnManagerEvents();
	BoundTurnManager = CandidateTurnManager;
	if (!IsValid(BoundTurnManager))
	{
		return;
	}

	BoundTurnManager->OnMonsterTurnStarted.AddUniqueDynamic(this, &UGridMonsterCombatComponent::HandleMonsterTurnStarted);
	BoundTurnManager->OnPhaseChanged.AddUniqueDynamic(this, &UGridMonsterCombatComponent::HandleCombatPhaseChanged);
}

void UGridMonsterCombatComponent::UnbindTurnManagerEvents()
{
	if (IsValid(BoundTurnManager))
	{
		BoundTurnManager->OnMonsterTurnStarted.RemoveDynamic(this, &UGridMonsterCombatComponent::HandleMonsterTurnStarted);
		BoundTurnManager->OnPhaseChanged.RemoveDynamic(this, &UGridMonsterCombatComponent::HandleCombatPhaseChanged);
	}
	BoundTurnManager = nullptr;
}

void UGridMonsterCombatComponent::EnsureCurrentCombatTurnObserved()
{
	RefreshTurnManagerBinding();
	if (IsValid(BoundTurnManager) && BoundTurnManager->bCombatActive && BoundTurnManager->CurrentMonster == OwnerMonster)
	{
		ObserveCombatTurn(BoundTurnManager->RoundNumber);
	}
}

void UGridMonsterCombatComponent::ObserveCombatTurn(int32 RoundNumber)
{
	if (RoundNumber <= 0 || LastObservedCombatRound == RoundNumber)
	{
		return;
	}

	if (LastObservedCombatRound != INDEX_NONE && RoundNumber < LastObservedCombatRound)
	{
		ResetAttackCooldowns();
	}

	AttackCooldownState.BeginTurn();
	LastObservedCombatRound = RoundNumber;

	UE_LOG(LogGridMonsterCombat, Verbose, TEXT("[GridMonsterCooldown] Turn Monster=%s Round=%d Serial=%d"), *GetNameSafe(OwnerMonster), RoundNumber,
		AttackCooldownState.GetCurrentTurnSerial());
}

void UGridMonsterCombatComponent::HandleMonsterTurnStarted(AGridMonsterActor* Monster)
{
	if (Monster != OwnerMonster || !IsValid(BoundTurnManager))
	{
		return;
	}
	ObserveCombatTurn(BoundTurnManager->RoundNumber);
}

void UGridMonsterCombatComponent::HandleCombatPhaseChanged(EGridCombatPhase NewPhase)
{
	if (NewPhase == EGridCombatPhase::StartingCombat || NewPhase == EGridCombatPhase::Exploration || NewPhase == EGridCombatPhase::Victory ||
		NewPhase == EGridCombatPhase::Defeat)
	{
		ResetAttackCooldowns();
	}
}

AGrimrockPartyPawn* UGridMonsterCombatComponent::FindPartyPawn() const
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AGrimrockPartyPawn> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

int32 UGridMonsterCombatComponent::GetResistancePercent(const FGridDamageResistanceSet& Resistances, EGridDamageType DamageType)
{
	switch (DamageType)
	{
		case EGridDamageType::Physical:
			return Resistances.PhysicalResistance;
		case EGridDamageType::Fire:
			return Resistances.FireResistance;
		case EGridDamageType::Ice:
			return Resistances.IceResistance;
		case EGridDamageType::Lightning:
			return Resistances.LightningResistance;
		case EGridDamageType::Poison:
			return Resistances.PoisonResistance;
		case EGridDamageType::Holy:
			return Resistances.HolyResistance;
		case EGridDamageType::Necrotic:
			return Resistances.NecroticResistance;
		case EGridDamageType::Arcane:
			return Resistances.ArcaneResistance;
		default:
			return 0;
	}
}

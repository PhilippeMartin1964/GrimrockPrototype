#include "Runtime/Monsters/GridMonsterCombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "EngineUtils.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

DEFINE_LOG_CATEGORY_STATIC (LogGridMonsterCombat, Log, All);

UGridMonsterCombatComponent::UGridMonsterCombatComponent ()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGridMonsterCombatComponent::BeginPlay ()
{
    Super::BeginPlay ();

    if (bAutoInitialize)
    {
        InitializeCombat (nullptr);
    }
}

bool UGridMonsterCombatComponent::InitializeCombat (AGrimrockPartyPawn* InPartyPawn)
{
    AGridMonsterActor* CandidateOwner = Cast<AGridMonsterActor> (GetOwner ());
    AGrimrockPartyPawn* CandidateParty = IsValid (InPartyPawn)
        ? InPartyPawn
        : FindPartyPawn ();

    if (!IsValid (CandidateOwner) || !IsValid (CandidateParty))
    {
        UE_LOG (LogGridMonsterCombat, Warning,
            TEXT ("[GridMonsterCombat] Initialization failed. Owner=%s Party=%s"),
            *GetNameSafe (CandidateOwner),
            *GetNameSafe (CandidateParty));
        return false;
    }

    OwnerMonster = CandidateOwner;
    PartyPawn = CandidateParty;
    bInitialized = true;
    return true;
}

bool UGridMonsterCombatComponent::GetPreferredMeleeAttack (
    FGridMonsterAttackDefinition& OutAttack) const
{
    OutAttack = FGridMonsterAttackDefinition ();
    if (!IsValid (OwnerMonster) || !IsValid (OwnerMonster->MonsterDefinition))
    {
        return false;
    }

    static const FName BiteAttackId (TEXT ("Attack_Bite"));
    if (const FGridMonsterAttackDefinition* Bite =
        OwnerMonster->MonsterDefinition->FindAttackDefinition (BiteAttackId))
    {
        if (Bite->IsValidDefinition () && Bite->RangeCells == 1)
        {
            OutAttack = *Bite;
            return true;
        }
    }

    const FGridMonsterAttackDefinition* FirstMelee =
        OwnerMonster->MonsterDefinition->Attacks.FindByPredicate (
            [] (const FGridMonsterAttackDefinition& Attack)
            {
                return Attack.IsValidDefinition () && Attack.RangeCells == 1;
            });
    if (!FirstMelee)
    {
        return false;
    }

    OutAttack = *FirstMelee;
    return true;
}

int32 UGridMonsterCombatComponent::SelectPartyTarget (FRandomStream& RandomStream) const
{
    if (!IsValid (PartyPawn) || !IsValid (PartyPawn->PartyInventoryComponent))
    {
        return INDEX_NONE;
    }

    return FGridPartyTargetSelector::SelectTarget (
        PartyPawn->PartyInventoryComponent->PartyInventoryState,
        RandomStream,
        FrontLineSlotCount);
}

bool UGridMonsterCombatComponent::ResolveAndApplyPartyAttack (
    int32 TargetCharacterIndex,
    const FGridMonsterAttackDefinition& Attack,
    FRandomStream& RandomStream,
    FGridAttackResult& OutResult)
{
    OutResult = FGridAttackResult ();
    if (!bInitialized && !InitializeCombat (nullptr))
    {
        return false;
    }

    if (!IsValid (OwnerMonster) ||
        OwnerMonster->IsDead () ||
        !IsValid (OwnerMonster->MonsterDefinition) ||
        !IsValid (PartyPawn) ||
        !IsValid (PartyPawn->PartyInventoryComponent) ||
        !Attack.IsValidDefinition ())
    {
        return false;
    }

    FGridPartyInventoryState& PartyState =
        PartyPawn->PartyInventoryComponent->PartyInventoryState;
    if (!PartyState.ActiveCharacters.IsValidIndex (TargetCharacterIndex))
    {
        UE_LOG (LogGridMonsterCombat, Warning,
            TEXT ("[GridMonsterCombat] Invalid party target. Monster=%s TargetIndex=%d"),
            *GetNameSafe (OwnerMonster),
            TargetCharacterIndex);
        return false;
    }

    FGridCharacterInventoryState& Character =
        PartyState.ActiveCharacters[TargetCharacterIndex];
    if (Character.DerivedStats.CurrentHealth <= 0)
    {
        return false;
    }

    FGridAttackSourceStats Source;
    Source.Accuracy = OwnerMonster->MonsterDefinition->Accuracy;
    Source.DamageBonus = Attack.DamageBonus;

    const FGridDamageResistanceSet Resistances =
        PartyPawn->PartyInventoryComponent->ComputeCharacterEquipmentResistances (
            TargetCharacterIndex);

    FGridAttackTargetStats Target;
    Target.Evasion = Character.DerivedStats.Evasion;
    Target.CurrentHealth = Character.DerivedStats.CurrentHealth;
    Target.PhysicalArmor = Character.DerivedStats.PhysicalArmor;
    Target.MagicalArmor = Character.DerivedStats.MagicalArmor;
    Target.ResistancePercent = GetResistancePercent (Resistances, Attack.DamageType);
    Target.DamageMultiplier = 1.0f;

    FGridAttackDefinition GenericAttack;
    GenericAttack.DamageType = Attack.DamageType;
    GenericAttack.PhysicalSubtype = Attack.PhysicalSubtype;
    GenericAttack.MinDamage = Attack.MinDamage;
    GenericAttack.MaxDamage = Attack.MaxDamage;
    GenericAttack.AccuracyBonus = Attack.AccuracyBonus;

    OutResult = FGridCombatResolver::ResolveAttack (
        Source,
        Target,
        GenericAttack,
        RandomStream);

    if (OutResult.bHit)
    {
        Character.DerivedStats.PhysicalArmor = FMath::Max (
            0,
            Character.DerivedStats.PhysicalArmor - OutResult.PhysicalArmorDamage);
        Character.DerivedStats.MagicalArmor = FMath::Max (
            0,
            Character.DerivedStats.MagicalArmor - OutResult.MagicalArmorDamage);
        Character.DerivedStats.CurrentHealth = FMath::Max (
            0,
            Character.DerivedStats.CurrentHealth - OutResult.HealthDamage);
    }

    LastAttackId = Attack.AttackId;
    LastTargetCharacterIndex = TargetCharacterIndex;
    LastAttackResult = OutResult;

    UE_LOG (LogGridMonsterCombat, Log,
        TEXT ("[GridMonsterCombat] Attack Monster=%s Attack=%s Target=%d Name=%s Natural=%d Total=%d Defense=%d Hit=%s Critical=%s Raw=%d ArmorPhysical=%d ArmorMagical=%d Health=%d HP=%d->%d"),
        *GetNameSafe (OwnerMonster),
        *Attack.AttackId.ToString (),
        TargetCharacterIndex,
        *Character.DisplayName.ToString (),
        OutResult.NaturalAttackRoll,
        OutResult.AttackRoll,
        OutResult.DefenseValue,
        OutResult.bHit ? TEXT ("true") : TEXT ("false"),
        OutResult.bCriticalHit ? TEXT ("true") : TEXT ("false"),
        OutResult.RawDamage,
        OutResult.PhysicalArmorDamage,
        OutResult.MagicalArmorDamage,
        OutResult.HealthDamage,
        OutResult.TargetHealthBefore,
        OutResult.TargetHealthAfter);

    return true;
}

bool UGridMonsterCombatComponent::StartAttackPresentation (
    const FGridCombatAction& Action,
    const FGridMonsterAttackDefinition& Attack)
{
    if ((!bInitialized && !InitializeCombat (nullptr)) ||
        !IsValid (OwnerMonster) ||
        OwnerMonster->IsDead () ||
        !Attack.IsValidDefinition ())
    {
        return false;
    }

    LastAttackId = Attack.AttackId;
    LastTargetCharacterIndex = Action.TargetCharacterIndex;
    bAttackPresentationActive = true;
    OwnerMonster->SetMonsterState (EGridMonsterState::Attacking);

    UAnimMontage* Montage = Attack.AttackMontage.LoadSynchronous ();
    UAnimInstance* AnimInstance = OwnerMonster->SkeletalMeshComponent
        ? OwnerMonster->SkeletalMeshComponent->GetAnimInstance ()
        : nullptr;
    if (Montage && AnimInstance)
    {
        AnimInstance->Montage_Play (Montage, 1.0f);
    }

    return true;
}

void UGridMonsterCombatComponent::NotifyAttackImpact ()
{
    if (bAttackPresentationActive)
    {
        OnAttackImpactNotify.Broadcast ();
    }
}

void UGridMonsterCombatComponent::NotifyActionComplete ()
{
    if (!bAttackPresentationActive)
    {
        return;
    }

    bAttackPresentationActive = false;
    if (IsValid (OwnerMonster) && !OwnerMonster->IsDead ())
    {
        OwnerMonster->SetMonsterState (EGridMonsterState::Pursuing);
    }
    OnActionCompleteNotify.Broadcast ();
}

void UGridMonsterCombatComponent::CancelAttackPresentation ()
{
    if (IsValid (OwnerMonster) && OwnerMonster->SkeletalMeshComponent)
    {
        if (UAnimInstance* AnimInstance =
            OwnerMonster->SkeletalMeshComponent->GetAnimInstance ())
        {
            AnimInstance->Montage_Stop (0.10f);
        }
    }

    bAttackPresentationActive = false;
    if (IsValid (OwnerMonster) && !OwnerMonster->IsDead ())
    {
        OwnerMonster->SetMonsterState (EGridMonsterState::Pursuing);
    }
}

void UGridMonsterCombatComponent::LogCombatState () const
{
    UE_LOG (LogGridMonsterCombat, Log,
        TEXT ("[GridMonsterCombat] Initialized=%s Owner=%s Party=%s Active=%s Attack=%s Target=%d Hit=%s Damage=%d"),
        bInitialized ? TEXT ("true") : TEXT ("false"),
        *GetNameSafe (OwnerMonster),
        *GetNameSafe (PartyPawn),
        bAttackPresentationActive ? TEXT ("true") : TEXT ("false"),
        *LastAttackId.ToString (),
        LastTargetCharacterIndex,
        LastAttackResult.bHit ? TEXT ("true") : TEXT ("false"),
        LastAttackResult.GetTotalAppliedDamage ());
}

AGrimrockPartyPawn* UGridMonsterCombatComponent::FindPartyPawn () const
{
    if (UWorld* World = GetWorld ())
    {
        for (TActorIterator<AGrimrockPartyPawn> It (World); It; ++It)
        {
            return *It;
        }
    }
    return nullptr;
}

int32 UGridMonsterCombatComponent::GetResistancePercent (
    const FGridDamageResistanceSet& Resistances,
    EGridDamageType DamageType)
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

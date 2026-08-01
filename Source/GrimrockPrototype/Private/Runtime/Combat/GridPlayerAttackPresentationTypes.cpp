#include "Runtime/Combat/GridPlayerAttackPresentationTypes.h"

#define LOCTEXT_NAMESPACE "GridPlayerAttackPresentation"

namespace
{
    bool IsFiniteVector (const FVector& Value)
    {
        return !Value.ContainsNaN () &&
            FMath::IsFinite (Value.X) &&
            FMath::IsFinite (Value.Y) &&
            FMath::IsFinite (Value.Z);
    }

    bool IsFiniteRotator (const FRotator& Value)
    {
        return !Value.ContainsNaN () &&
            FMath::IsFinite (Value.Pitch) &&
            FMath::IsFinite (Value.Yaw) &&
            FMath::IsFinite (Value.Roll);
    }

    template <typename ObjectType>
    bool HasNoExplicitNulls (
        const TArray<TSoftObjectPtr<ObjectType>>& References)
    {
        for (const TSoftObjectPtr<ObjectType>& Reference : References)
        {
            if (Reference.IsNull ())
            {
                return false;
            }
        }
        return true;
    }
}

bool FGridPlayerAttackAudioDefinition::IsValid () const
{
    return FMath::IsFinite (VolumeMultiplier) &&
        VolumeMultiplier >= 0.0f &&
        FMath::IsFinite (PitchMin) &&
        FMath::IsFinite (PitchMax) &&
        PitchMin > 0.0f &&
        PitchMax > 0.0f &&
        PitchMin <= PitchMax &&
        HasNoExplicitNulls (Sounds);
}

bool FGridPlayerAttackVFXDefinition::IsValid () const
{
    return HasNoExplicitNulls (Systems) &&
        IsFiniteVector (LocationOffset) &&
        IsFiniteRotator (RotationOffset) &&
        IsFiniteVector (Scale) &&
        Scale.X > 0.0f &&
        Scale.Y > 0.0f &&
        Scale.Z > 0.0f;
}

bool FGridPlayerAttackPresentationProfile::IsValid () const
{
    const bool bMotionValid =
        !bAnimateHeldItem ||
        (FMath::IsFinite (MotionDurationSeconds) &&
            MotionDurationSeconds >= 0.01f &&
            MotionDurationSeconds <= 2.0f &&
            IsFiniteVector (PeakLocationOffset) &&
            IsFiniteRotator (PeakRotationOffset));
    return bMotionValid &&
        FMath::IsFinite (FeedbackDurationSeconds) &&
        FeedbackDurationSeconds >= 0.1f &&
        FeedbackDurationSeconds <= 10.0f &&
        AttackAudio.IsValid () &&
        ImpactHitAudio.IsValid () &&
        ImpactMissAudio.IsValid () &&
        AttackVFX.IsValid () &&
        ImpactHitVFX.IsValid () &&
        ImpactMissVFX.IsValid ();
}

FText FGridPlayerAttackFeedbackFormatter::FormatRejectReason (
    EGridPlayerAttackRejectReason RejectReason)
{
    switch (RejectReason)
    {
    case EGridPlayerAttackRejectReason::TurnManagerNotInitialized:
        return LOCTEXT ("RejectNotInitialized", "Le gestionnaire de combat n’est pas initialisé.");
    case EGridPlayerAttackRejectReason::CombatInactive:
        return LOCTEXT ("RejectCombatInactive", "Aucun combat n’est actif.");
    case EGridPlayerAttackRejectReason::NotPlayerPhase:
        return LOCTEXT ("RejectNotPlayerPhase", "Ce n’est pas la phase du groupe.");
    case EGridPlayerAttackRejectReason::PartyUnavailable:
        return LOCTEXT ("RejectPartyUnavailable", "Le groupe n’est pas disponible.");
    case EGridPlayerAttackRejectReason::PartyBusy:
        return LOCTEXT ("RejectPartyBusy", "Le groupe est occupé.");
    case EGridPlayerAttackRejectReason::InvalidAttacker:
        return LOCTEXT ("RejectInvalidAttacker", "Ce personnage ne peut pas attaquer.");
    case EGridPlayerAttackRejectReason::AttackerDefeated:
        return LOCTEXT ("RejectAttackerDefeated", "Ce personnage est vaincu.");
    case EGridPlayerAttackRejectReason::AttackerAlreadyActed:
        return LOCTEXT ("RejectAlreadyActed", "Ce personnage a déjà attaqué pendant cette phase.");
    case EGridPlayerAttackRejectReason::InsufficientActionPoints:
        return LOCTEXT ("RejectInsufficientAP", "Ce personnage n’a pas assez de points d’action.");
    case EGridPlayerAttackRejectReason::InvalidFacing:
        return LOCTEXT ("RejectInvalidFacing", "L’orientation du groupe est invalide.");
    case EGridPlayerAttackRejectReason::TargetCellUnavailable:
        return LOCTEXT ("RejectTargetCell", "La cellule ciblée n’est pas disponible.");
    case EGridPlayerAttackRejectReason::PassageBlocked:
        return LOCTEXT ("RejectBlocked", "Le passage est bloqué.");
    case EGridPlayerAttackRejectReason::NoMonsterInFront:
        return LOCTEXT ("RejectNoMonster", "Aucun monstre ne se trouve devant le groupe.");
    case EGridPlayerAttackRejectReason::TargetNotInEncounter:
        return LOCTEXT ("RejectNotEncounter", "Cette cible ne participe pas au combat.");
    case EGridPlayerAttackRejectReason::TargetInactive:
        return LOCTEXT ("RejectInactive", "Cette cible est inactive.");
    case EGridPlayerAttackRejectReason::TargetDefeated:
        return LOCTEXT ("RejectTargetDefeated", "Cette cible est déjà vaincue.");
    case EGridPlayerAttackRejectReason::TargetOutOfRange:
        return LOCTEXT ("RejectRange", "La cible est hors de portée.");
    case EGridPlayerAttackRejectReason::EquippedItemDefinitionUnavailable:
        return LOCTEXT ("RejectMissingDefinition", "La définition de l’objet équipé est indisponible.");
    case EGridPlayerAttackRejectReason::InvalidOffensiveEquipment:
        return LOCTEXT ("RejectInvalidEquipment", "L’équipement offensif est invalide.");
    case EGridPlayerAttackRejectReason::None:
    default:
        return LOCTEXT ("RejectUnknown", "L’attaque ne peut pas être effectuée.");
    }
}

FText FGridPlayerAttackFeedbackFormatter::FormatDamageDetail (
    const FGridAttackResult& Result)
{
    TArray<FText> Parts;
    if (Result.PhysicalArmorDamage > 0)
    {
        Parts.Add (FText::Format (
            LOCTEXT ("PhysicalArmorDetail", "Armure physique : -{0}"),
            FText::AsNumber (Result.PhysicalArmorDamage)));
    }
    if (Result.MagicalArmorDamage > 0)
    {
        Parts.Add (FText::Format (
            LOCTEXT ("MagicalArmorDetail", "Armure magique : -{0}"),
            FText::AsNumber (Result.MagicalArmorDamage)));
    }
    if (Result.HealthDamage > 0)
    {
        Parts.Add (FText::Format (
            LOCTEXT ("HealthDetail", "Points de vie : -{0} ({1} → {2})"),
            FText::AsNumber (Result.HealthDamage),
            FText::AsNumber (Result.TargetHealthBefore),
            FText::AsNumber (Result.TargetHealthAfter)));
    }
    return FText::Join (LOCTEXT ("DetailSeparator", " · "), Parts);
}

FGridPlayerAttackFeedbackRequest
FGridPlayerAttackFeedbackFormatter::FormatResolved (
    const FGridPlayerAttackRequest& Request,
    const FGridAttackResult& Result,
    const FText& SourceDisplayName,
    const FText& TargetDisplayName,
    float DurationSeconds)
{
    FGridPlayerAttackFeedbackRequest Feedback;
    Feedback.bAccepted = true;
    Feedback.AttackRequest = Request;
    Feedback.AttackResult = Result;
    Feedback.SourceDisplayName = SourceDisplayName;
    Feedback.TargetDisplayName = TargetDisplayName;
    Feedback.DurationSeconds = DurationSeconds;
    const int32 Damage = Result.GetTotalAppliedDamage ();
    const bool bDefeated =
        Result.bHit &&
        Result.TargetHealthBefore > 0 &&
        Result.TargetHealthAfter <= 0;
    if (!Result.bHit)
    {
        Feedback.Outcome = EGridPlayerAttackFeedbackOutcome::Miss;
        Feedback.PrimaryText = FText::Format (
            LOCTEXT ("Miss", "{0} rate {1}."),
            SourceDisplayName,
            TargetDisplayName);
        Feedback.SuggestedColor = FLinearColor (0.75f, 0.75f, 0.75f);
    }
    else if (bDefeated)
    {
        Feedback.Outcome =
            EGridPlayerAttackFeedbackOutcome::TargetDefeated;
        Feedback.PrimaryText = FText::Format (
            LOCTEXT ("Defeated", "{0} vaincu : {1} dégâts."),
            TargetDisplayName,
            FText::AsNumber (Damage));
        Feedback.SuggestedColor = FLinearColor (0.95f, 0.55f, 0.15f);
    }
    else if (Result.bCriticalHit)
    {
        Feedback.Outcome =
            EGridPlayerAttackFeedbackOutcome::CriticalHit;
        Feedback.PrimaryText = FText::Format (
            LOCTEXT ("Critical", "Coup critique : {0} dégâts à {1}."),
            FText::AsNumber (Damage),
            TargetDisplayName);
        Feedback.SuggestedColor = FLinearColor (1.0f, 0.8f, 0.1f);
    }
    else
    {
        Feedback.Outcome = EGridPlayerAttackFeedbackOutcome::Hit;
        Feedback.PrimaryText = FText::Format (
            LOCTEXT ("Hit", "{0} inflige {1} dégâts à {2}."),
            SourceDisplayName,
            FText::AsNumber (Damage),
            TargetDisplayName);
        Feedback.SuggestedColor = FLinearColor (0.95f, 0.3f, 0.15f);
    }
    Feedback.DetailText = FormatDamageDetail (Result);
    return Feedback;
}

#undef LOCTEXT_NAMESPACE

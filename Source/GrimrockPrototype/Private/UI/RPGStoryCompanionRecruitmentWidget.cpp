#include "UI/RPGStoryCompanionRecruitmentWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGPartyRecruitmentService.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGStoryCompanionAsset.h"
#include "RPG/RPGStoryCompanionService.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPlayerController.h"

#define LOCTEXT_NAMESPACE "RPGStoryCompanionRecruitmentWidget"

DEFINE_LOG_CATEGORY_STATIC (LogGridRecruitmentWidget, Log, All);

namespace RPGStoryCompanionRecruitmentWidgetPrivate
{
    const FGridCharacterInventoryState* FindCharacterById (
        const TArray<FGridCharacterInventoryState>& Characters,
        const FGuid& CharacterId)
    {
        return Characters.FindByPredicate (
            [&CharacterId] (const FGridCharacterInventoryState& Character)
            {
                return Character.CharacterId == CharacterId;
            });
    }

    bool MatchesDefinitionIdentity (
        const FGridCharacterInventoryState* Character,
        const URPGStoryCompanionAsset& Definition)
    {
        return Character &&
            Definition.RaceDefinition &&
            Definition.ClassDefinition &&
            Character->CharacterId == Definition.CharacterId &&
            Character->RaceId == Definition.RaceDefinition->RaceId &&
            Character->ClassId == Definition.ClassDefinition->ClassId;
    }
}

bool URPGStoryCompanionRecruitmentWidget::InitializeRecruitmentWidget (
    UGridPartyInventoryComponent* InInventoryComponent,
    URPGStoryCompanionAsset* InCompanionDefinition)
{
    InventoryComponent = InInventoryComponent;
    CompanionDefinition = InCompanionDefinition;
    bCloseBroadcast = false;
    View.bDetailsVisible = false;
    RefreshView ();

    return IsValid (InventoryComponent) &&
        IsValid (CompanionDefinition) &&
        CompanionDefinition->IsValidDefinition ();
}

bool URPGStoryCompanionRecruitmentWidget::TryRecruit ()
{
    if (!IsValid (InventoryComponent) ||
        !IsValid (CompanionDefinition) ||
        !CompanionDefinition->IsValidDefinition ())
    {
        RefreshView ();
        View.State = ERPGStoryCompanionRecruitmentState::Invalid;
        View.StatusText = LOCTEXT (
            "InvalidRecruitmentRequest",
            "Impossible de proposer ce recrutement : données invalides.");
        View.bCanRecruit = false;
        RefreshPresentation ();
        return false;
    }

    if (View.State == ERPGStoryCompanionRecruitmentState::AlreadyActive)
    {
        return false;
    }

    FRPGStoryCompanionRegistrationResult RegistrationResult;
    if (!FRPGStoryCompanionService::EnsureCandidateRegistered (
            InventoryComponent,
            CompanionDefinition,
            RegistrationResult))
    {
        RefreshView ();
        View.State = ERPGStoryCompanionRecruitmentState::Failed;
        View.StatusText = BuildRegistrationFailureText (
            RegistrationResult.Error);
        View.bCanRecruit = false;
        RefreshPresentation ();
        return false;
    }

    if (RegistrationResult.Status ==
        ERPGStoryCompanionRegistrationStatus::AlreadyActive)
    {
        RefreshView ();
        View.State = ERPGStoryCompanionRecruitmentState::AlreadyActive;
        View.StatusText = LOCTEXT (
            "AlreadyActive",
            "Ce compagnon fait déjà partie du groupe.");
        View.bCanRecruit = false;
        RefreshPresentation ();
        return false;
    }

    FRPGPartyRecruitmentResult RecruitmentResult;
    if (!FRPGPartyRecruitmentService::TryRecruitFromPool (
            InventoryComponent,
            CompanionDefinition->CharacterId,
            RecruitmentResult))
    {
        RefreshView ();
        if (RecruitmentResult.RejectReason ==
            ERPGPartyRecruitmentRejectReason::PartyFull)
        {
            View.State = ERPGStoryCompanionRecruitmentState::PartyFull;
            View.StatusText = LOCTEXT (
                "PartyFull",
                "Le groupe est complet. Ce compagnon reste disponible dans la réserve.");
        }
        else
        {
            View.State = ERPGStoryCompanionRecruitmentState::Failed;
            View.StatusText = BuildRecruitmentFailureText (
                static_cast<int32> (RecruitmentResult.RejectReason),
                RecruitmentResult.Error);
        }
        View.bCanRecruit = false;
        RefreshPresentation ();
        return false;
    }

    RefreshView ();
    View.State = ERPGStoryCompanionRecruitmentState::Recruited;
    View.StatusText = FText::Format (
        LOCTEXT (
            "Recruited",
            "{0} rejoint le groupe."),
        View.DisplayName);
    View.bCanRecruit = false;
    RefreshPresentation ();

    UE_LOG (
        LogGridRecruitmentWidget,
        Log,
        TEXT ("[GridRecruitmentUI] Recruited Companion=%s CharacterId=%s CharacterIndex=%d Active=%d->%d"),
        *CompanionDefinition->CompanionId.ToString (),
        *CompanionDefinition->CharacterId.ToString (EGuidFormats::Digits),
        RecruitmentResult.CharacterIndex,
        RecruitmentResult.ActiveCountBefore,
        RecruitmentResult.ActiveCountAfter);

    AcceptedDelegate.Broadcast (this);
    CloseModal ();
    return true;
}

void URPGStoryCompanionRecruitmentWidget::DeclineRecruitment ()
{
    if (View.State == ERPGStoryCompanionRecruitmentState::Recruited ||
        View.State == ERPGStoryCompanionRecruitmentState::Declined)
    {
        return;
    }

    View.State = ERPGStoryCompanionRecruitmentState::Declined;
    View.StatusText = LOCTEXT (
        "Declined",
        "Vous refusez pour le moment.");
    View.bCanRecruit = false;
    RefreshPresentation ();

    UE_LOG (
        LogGridRecruitmentWidget,
        Log,
        TEXT ("[GridRecruitmentUI] Declined Companion=%s CharacterId=%s"),
        *GetNameSafe (CompanionDefinition),
        IsValid (CompanionDefinition)
            ? *CompanionDefinition->CharacterId.ToString (EGuidFormats::Digits)
            : TEXT ("Invalid"));

    DeclinedDelegate.Broadcast (this);
    CloseModal ();
}

void URPGStoryCompanionRecruitmentWidget::ToggleDetails ()
{
    View.bDetailsVisible = !View.bDetailsVisible;
    RefreshPresentation ();
}

void URPGStoryCompanionRecruitmentWidget::CloseRecruitment ()
{
    CloseModal ();
}

URPGStoryCompanionAsset*
URPGStoryCompanionRecruitmentWidget::GetCompanionDefinition () const
{
    return CompanionDefinition;
}

FRPGStoryCompanionRecruitmentWidgetAcceptedNativeSignature&
URPGStoryCompanionRecruitmentWidget::OnAccepted ()
{
    return AcceptedDelegate;
}

FRPGStoryCompanionRecruitmentWidgetDeclinedNativeSignature&
URPGStoryCompanionRecruitmentWidget::OnDeclined ()
{
    return DeclinedDelegate;
}

FRPGStoryCompanionRecruitmentWidgetClosedNativeSignature&
URPGStoryCompanionRecruitmentWidget::OnClosed ()
{
    return ClosedDelegate;
}

void URPGStoryCompanionRecruitmentWidget::SynchronizeProperties ()
{
    Super::SynchronizeProperties ();
    RefreshPresentation ();
}

void URPGStoryCompanionRecruitmentWidget::NativeConstruct ()
{
    Super::NativeConstruct ();
    BindButtons ();
    RefreshView ();
    ApplyInputGuard ();
}

void URPGStoryCompanionRecruitmentWidget::NativeDestruct ()
{
    RestoreInputGuard ();
    Super::NativeDestruct ();
}

void URPGStoryCompanionRecruitmentWidget::BindButtons ()
{
    if (Button_Recruit)
    {
        Button_Recruit->OnClicked.RemoveDynamic (
            this,
            &URPGStoryCompanionRecruitmentWidget::HandleRecruitClicked);
        Button_Recruit->OnClicked.AddDynamic (
            this,
            &URPGStoryCompanionRecruitmentWidget::HandleRecruitClicked);
    }

    if (Button_Decline)
    {
        Button_Decline->OnClicked.RemoveDynamic (
            this,
            &URPGStoryCompanionRecruitmentWidget::HandleDeclineClicked);
        Button_Decline->OnClicked.AddDynamic (
            this,
            &URPGStoryCompanionRecruitmentWidget::HandleDeclineClicked);
    }

    if (Button_ShowDetails)
    {
        Button_ShowDetails->OnClicked.RemoveDynamic (
            this,
            &URPGStoryCompanionRecruitmentWidget::HandleShowDetailsClicked);
        Button_ShowDetails->OnClicked.AddDynamic (
            this,
            &URPGStoryCompanionRecruitmentWidget::HandleShowDetailsClicked);
    }
}

void URPGStoryCompanionRecruitmentWidget::RefreshView ()
{
    const bool bDetailsVisible = View.bDetailsVisible;
    View = FRPGStoryCompanionRecruitmentView ();
    View.bDetailsVisible = bDetailsVisible;

    if (IsValid (CompanionDefinition))
    {
        View.CompanionId = CompanionDefinition->CompanionId;
        View.CharacterId = CompanionDefinition->CharacterId;
        View.DisplayName = CompanionDefinition->DisplayName;
        View.ShortDescription = CompanionDefinition->ShortDescription;
        View.Level = CompanionDefinition->Level;
        View.Portrait = CompanionDefinition->Portrait;
        View.FullBody = CompanionDefinition->FullBody;
        View.ClassIcon = CompanionDefinition->ClassIcon;
        View.RecruitmentConditionText =
            CompanionDefinition->RecruitmentConditionText;
        if (CompanionDefinition->RaceDefinition)
        {
            View.RaceName = CompanionDefinition->RaceDefinition->DisplayName;
        }
        if (CompanionDefinition->ClassDefinition)
        {
            View.ClassName = CompanionDefinition->ClassDefinition->DisplayName;
        }
    }

    if (!IsValid (InventoryComponent) ||
        !IsValid (CompanionDefinition) ||
        !CompanionDefinition->IsValidDefinition ())
    {
        View.State = ERPGStoryCompanionRecruitmentState::Invalid;
        View.StatusText = LOCTEXT (
            "InvalidDefinition",
            "Les données de recrutement sont invalides.");
        View.bCanRecruit = false;
        RefreshPresentation ();
        return;
    }

    const FGridPartyInventoryState& State =
        InventoryComponent->PartyInventoryState;
    const FGridCharacterInventoryState* ActiveCharacter =
        RPGStoryCompanionRecruitmentWidgetPrivate::FindCharacterById (
            State.ActiveCharacters,
            CompanionDefinition->CharacterId);
    const FGridCharacterInventoryState* PoolCharacter =
        RPGStoryCompanionRecruitmentWidgetPrivate::FindCharacterById (
            State.CharacterPool,
            CompanionDefinition->CharacterId);
    const bool bAlreadyActive =
        RPGStoryCompanionRecruitmentWidgetPrivate::MatchesDefinitionIdentity (
            ActiveCharacter,
            *CompanionDefinition);
    View.bCandidateAlreadyRegistered =
        RPGStoryCompanionRecruitmentWidgetPrivate::MatchesDefinitionIdentity (
            PoolCharacter,
            *CompanionDefinition);

    if (bAlreadyActive)
    {
        View.State = ERPGStoryCompanionRecruitmentState::AlreadyActive;
        View.StatusText = LOCTEXT (
            "AlreadyActiveView",
            "Ce compagnon fait déjà partie du groupe.");
        View.bCanRecruit = false;
    }
    else
    {
        View.State = ERPGStoryCompanionRecruitmentState::Ready;
        View.StatusText = View.bCandidateAlreadyRegistered
            ? LOCTEXT (
                "ReadyFromPool",
                "Ce compagnon est disponible et peut rejoindre le groupe.")
            : LOCTEXT (
                "Ready",
                "Ce compagnon souhaite rejoindre le groupe.");
        View.bCanRecruit = true;
    }

    RefreshPresentation ();
}

void URPGStoryCompanionRecruitmentWidget::RefreshPresentation ()
{
    RefreshBoundWidgets ();
    RefreshNativeSlate ();
    BP_OnRecruitmentViewRefreshed ();
}

FText URPGStoryCompanionRecruitmentWidget::BuildIdentityLine () const
{
    return FText::Format (
        LOCTEXT (
            "IdentityLine",
            "{0} — {1} — niveau {2}"),
        View.RaceName.IsEmpty ()
            ? LOCTEXT ("UnknownRace", "Race inconnue")
            : View.RaceName,
        View.ClassName.IsEmpty ()
            ? LOCTEXT ("UnknownClass", "Classe inconnue")
            : View.ClassName,
        FText::AsNumber (View.Level));
}

FText URPGStoryCompanionRecruitmentWidget::BuildDetailsText () const
{
    if (!View.bDetailsVisible)
    {
        return FText::GetEmpty ();
    }

    const FText Condition = View.RecruitmentConditionText.IsEmpty ()
        ? LOCTEXT (
            "NoRecruitmentCondition",
            "Aucune condition de recrutement supplémentaire n'est renseignée.")
        : View.RecruitmentConditionText;

    const FText RaceDescription =
        IsValid (CompanionDefinition) && CompanionDefinition->RaceDefinition
            ? CompanionDefinition->RaceDefinition->Description
            : FText::GetEmpty ();
    const FText ClassDescription =
        IsValid (CompanionDefinition) && CompanionDefinition->ClassDefinition
            ? CompanionDefinition->ClassDefinition->Description
            : FText::GetEmpty ();

    return FText::Format (
        LOCTEXT (
            "DetailsText",
            "Condition : {0}\n\nRace : {1}\n{2}\n\nClasse : {3}\n{4}"),
        Condition,
        View.RaceName,
        RaceDescription,
        View.ClassName,
        ClassDescription);
}

FText URPGStoryCompanionRecruitmentWidget::BuildRegistrationFailureText (
    const FString& Error) const
{
    return Error.IsEmpty ()
        ? LOCTEXT (
            "RegistrationFailed",
            "Le compagnon ne peut pas être préparé au recrutement.")
        : FText::FromString (Error);
}

FText URPGStoryCompanionRecruitmentWidget::BuildRecruitmentFailureText (
    int32 RejectReasonValue,
    const FString& Error) const
{
    if (!Error.IsEmpty ())
    {
        return FText::FromString (Error);
    }

    return FText::Format (
        LOCTEXT (
            "RecruitmentFailed",
            "Le recrutement a échoué (raison {0})."),
        FText::AsNumber (RejectReasonValue));
}

void URPGStoryCompanionRecruitmentWidget::ApplyInputGuard ()
{
    if (bInputGuardApplied || !IsValid (InventoryComponent))
    {
        return;
    }

    APawn* PartyPawn = Cast<APawn> (InventoryComponent->GetOwner ());
    AGrimrockPlayerController* PlayerController = PartyPawn
        ? Cast<AGrimrockPlayerController> (PartyPawn->GetController ())
        : nullptr;
    if (!PartyPawn || !PlayerController)
    {
        return;
    }

    bPreviousInventoryUiOpen = PlayerController->bInventoryUiOpen;
    PlayerController->SetInventoryUiOpen (true);
    PartyPawn->DisableInput (PlayerController);

    if (!UGameplayStatics::IsGamePaused (PartyPawn))
    {
        bGamePausedByModal = UGameplayStatics::SetGamePaused (
            PartyPawn,
            true);
    }

    FInputModeUIOnly InputMode;
    InputMode.SetLockMouseToViewportBehavior (
        EMouseLockMode::DoNotLock);
    InputMode.SetWidgetToFocus (TakeWidget ());
    PlayerController->SetInputMode (InputMode);
    PlayerController->bShowMouseCursor = true;
    bInputGuardApplied = true;

    UE_LOG (
        LogGridRecruitmentWidget,
        Log,
        TEXT ("[GridRecruitmentUI] ModalGuard Applied Companion=%s PausedByModal=%s"),
        *GetNameSafe (CompanionDefinition),
        bGamePausedByModal ? TEXT ("true") : TEXT ("false"));
}

void URPGStoryCompanionRecruitmentWidget::RestoreInputGuard ()
{
    if (!bInputGuardApplied)
    {
        return;
    }

    if (bGamePausedByModal)
    {
        if (UWorld* World = GetWorld ())
        {
            UGameplayStatics::SetGamePaused (World, false);
        }
        bGamePausedByModal = false;
    }

    APawn* PartyPawn = IsValid (InventoryComponent)
        ? Cast<APawn> (InventoryComponent->GetOwner ())
        : nullptr;
    AGrimrockPlayerController* PlayerController = PartyPawn
        ? Cast<AGrimrockPlayerController> (PartyPawn->GetController ())
        : nullptr;
    if (PartyPawn && PlayerController)
    {
        PartyPawn->EnableInput (PlayerController);
        PlayerController->SetInventoryUiOpen (bPreviousInventoryUiOpen);

        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture (false);
        PlayerController->SetInputMode (InputMode);
        PlayerController->bShowMouseCursor = true;
    }

    bInputGuardApplied = false;

    UE_LOG (
        LogGridRecruitmentWidget,
        Log,
        TEXT ("[GridRecruitmentUI] ModalGuard Restored Companion=%s"),
        *GetNameSafe (CompanionDefinition));
}

void URPGStoryCompanionRecruitmentWidget::CloseModal ()
{
    RestoreInputGuard ();
    RemoveFromParent ();
    if (!bCloseBroadcast)
    {
        bCloseBroadcast = true;
        ClosedDelegate.Broadcast (this);
    }
}

void URPGStoryCompanionRecruitmentWidget::HandleRecruitClicked ()
{
    TryRecruit ();
}

void URPGStoryCompanionRecruitmentWidget::HandleDeclineClicked ()
{
    DeclineRecruitment ();
}

void URPGStoryCompanionRecruitmentWidget::HandleShowDetailsClicked ()
{
    ToggleDetails ();
}

#undef LOCTEXT_NAMESPACE

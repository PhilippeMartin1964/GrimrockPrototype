#include "UI/RPGLevelUpWidget.h"

#include "Blueprint/WidgetTree.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPlayerController.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "RPGLevelUpWidget"

DEFINE_LOG_CATEGORY_STATIC(LogGridLevelUpWidget, Log, All);

void URPGLevelUpWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	RefreshNativeSlate();
}

TSharedRef<SWidget> URPGLevelUpWidget::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	return SNew(SBorder)
		.Padding(FMargin(36.0f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)[SNew(SBox).WidthOverride(760.0f)[SNew(SVerticalBox) +
			SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 14.0f)[SAssignNew(NativeTitleText, STextBlock)
					.Justification(ETextJustify::Center)
					.Text(View.Presentation.Title.IsEmpty() ? LOCTEXT("Title", "NIVEAU SUPÉRIEUR") : View.Presentation.Title)] +
			SVerticalBox::Slot().AutoHeight().Padding(
				0.0f, 0.0f, 0.0f, 10.0f)[SAssignNew(NativeCharacterText, STextBlock).Justification(ETextJustify::Center).Text(BuildCharacterLine())] +
			SVerticalBox::Slot().AutoHeight().Padding(
				0.0f, 0.0f, 0.0f, 8.0f)[SAssignNew(NativeStatsText, STextBlock).Justification(ETextJustify::Center).Text(BuildStatsLine())] +
			SVerticalBox::Slot().AutoHeight().Padding(
				0.0f, 0.0f, 0.0f, 12.0f)[SAssignNew(NativePointsText, STextBlock).Justification(ETextJustify::Center).Text(BuildPointsLine())] +
			SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f,
				8.0f)[SAssignNew(NativeTalentSectionText, STextBlock).Justification(ETextJustify::Center).Text(View.Presentation.TalentSectionTitle)] +
			SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 0.0f, 0.0f, 12.0f)[SAssignNew(NativeChoicesBox, SVerticalBox)] +
			SVerticalBox::Slot().AutoHeight().Padding(
				0.0f, 0.0f, 0.0f, 12.0f)[SAssignNew(NativeValidationText, STextBlock).Justification(ETextJustify::Center).Text(View.ValidationMessage)] +
			SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox) +
				SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 8.0f, 0.0f)[SAssignNew(NativeConfirmButton, SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("Confirm", "Confirmer"))
						.OnClicked(FOnClicked::CreateUObject(this, &URPGLevelUpWidget::HandleNativeConfirmClicked))] +
				SHorizontalBox::Slot().FillWidth(1.0f).Padding(8.0f, 0.0f, 0.0f, 0.0f)[SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("Cancel", "Annuler"))
						.OnClicked(FOnClicked::CreateUObject(this, &URPGLevelUpWidget::HandleNativeCancelClicked))]]]];
}

void URPGLevelUpWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshView();
	ApplyInputGuard();
}

void URPGLevelUpWidget::NativeDestruct()
{
	RestoreInputGuard();
	Super::NativeDestruct();
}

void URPGLevelUpWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	NativeTitleText.Reset();
	NativeCharacterText.Reset();
	NativeStatsText.Reset();
	NativePointsText.Reset();
	NativeTalentSectionText.Reset();
	NativeValidationText.Reset();
	NativeChoicesBox.Reset();
	NativeConfirmButton.Reset();
}

void URPGLevelUpWidget::RefreshNativeSlate()
{
	if (NativeTitleText.IsValid())
	{
		NativeTitleText->SetText(View.Presentation.Title.IsEmpty() ? LOCTEXT("Title", "NIVEAU SUPÉRIEUR") : View.Presentation.Title);
	}
	if (NativeCharacterText.IsValid())
	{
		NativeCharacterText->SetText(BuildCharacterLine());
	}
	if (NativeStatsText.IsValid())
	{
		NativeStatsText->SetText(BuildStatsLine());
	}
	if (NativePointsText.IsValid())
	{
		NativePointsText->SetText(BuildPointsLine());
	}
	if (NativeTalentSectionText.IsValid())
	{
		NativeTalentSectionText->SetText(View.Presentation.TalentSectionTitle);
	}
	if (NativeValidationText.IsValid())
	{
		NativeValidationText->SetText(View.ValidationMessage);
	}
	if (NativeConfirmButton.IsValid())
	{
		NativeConfirmButton->SetEnabled(View.bCanConfirm);
	}

	if (!NativeChoicesBox.IsValid())
	{
		return;
	}
	NativeChoicesBox->ClearChildren();
	if (View.Choices.IsEmpty())
	{
		NativeChoicesBox->AddSlot().AutoHeight()[SNew(STextBlock)
				.Justification(ETextJustify::Center)
				.Text(View.Presentation.EmptyTalentsMessage.IsEmpty() ? LOCTEXT("NoTalentsFallback", "Aucun talent de classe à sélectionner pour ce niveau.")
																	  : View.Presentation.EmptyTalentsMessage)];
		return;
	}

	for (const FRPGLevelUpChoiceView& Choice : View.Choices)
	{
		const FText Label = FText::Format(LOCTEXT("TalentLabel", "{0} — coût {1} — {2}"),
			Choice.DisplayName.IsEmpty() ? FText::FromName(Choice.ChoiceId) : Choice.DisplayName, FText::AsNumber(Choice.PointCost), Choice.StatusText);
		NativeChoicesBox->AddSlot().AutoHeight().Padding(0.0f, 3.0f)[SNew(SButton)
				.IsEnabled(!Choice.bCommitted && Choice.bAvailable)
				.Text(Label)
				.OnClicked(FOnClicked::CreateUObject(this, &URPGLevelUpWidget::HandleNativeChoiceClicked, Choice.ChoiceId))];
	}
}

void URPGLevelUpWidget::ApplyInputGuard()
{
	if (bInputGuardApplied || !IsValid(InventoryComponent))
	{
		return;
	}

	APawn* PartyPawn = Cast<APawn>(InventoryComponent->GetOwner());
	AGrimrockPlayerController* PlayerController = PartyPawn ? Cast<AGrimrockPlayerController>(PartyPawn->GetController()) : nullptr;
	if (!PartyPawn || !PlayerController)
	{
		return;
	}

	bPreviousInventoryUiOpen = PlayerController->bInventoryUiOpen;
	PlayerController->SetInventoryUiOpen(true);
	PartyPawn->DisableInput(PlayerController);

	if (!UGameplayStatics::IsGamePaused(PartyPawn))
	{
		bGamePausedByModal = UGameplayStatics::SetGamePaused(PartyPawn, true);
	}

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;
	bInputGuardApplied = true;

	UE_LOG(LogGridLevelUpWidget, Log, TEXT("[GridLevelUpUI] ModalGuard Applied Character=%d PausedByModal=%s"), CharacterIndex,
		bGamePausedByModal ? TEXT("true") : TEXT("false"));
}

void URPGLevelUpWidget::RestoreInputGuard()
{
	if (!bInputGuardApplied)
	{
		return;
	}

	if (bGamePausedByModal)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::SetGamePaused(World, false);
		}
		bGamePausedByModal = false;
	}

	if (!IsValid(InventoryComponent))
	{
		bInputGuardApplied = false;
		return;
	}

	APawn* PartyPawn = Cast<APawn>(InventoryComponent->GetOwner());
	AGrimrockPlayerController* PlayerController = PartyPawn ? Cast<AGrimrockPlayerController>(PartyPawn->GetController()) : nullptr;
	if (PartyPawn && PlayerController)
	{
		PartyPawn->EnableInput(PlayerController);
		PlayerController->SetInventoryUiOpen(bPreviousInventoryUiOpen);

		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
	}
	bInputGuardApplied = false;

	UE_LOG(LogGridLevelUpWidget, Log, TEXT("[GridLevelUpUI] ModalGuard Restored Character=%d"), CharacterIndex);
}

void URPGLevelUpWidget::CloseModal()
{
	RestoreInputGuard();
	RemoveFromParent();
	ClosedDelegate.Broadcast(this);
}

FText URPGLevelUpWidget::BuildCharacterLine() const
{
	return FText::Format(LOCTEXT("CharacterLine", "{0} — {1} — niveau {2} → {3}"),
		View.CharacterName.IsEmpty() ? LOCTEXT("UnknownCharacter", "Personnage") : View.CharacterName,
		View.ClassName.IsEmpty() ? LOCTEXT("UnknownClass", "Classe") : View.ClassName, FText::AsNumber(View.PreviousLevel), FText::AsNumber(View.NewLevel));
}

FText URPGLevelUpWidget::BuildStatsLine() const
{
	return FText::Format(LOCTEXT("StatsLine", "PV max {0} → {1}    Mana max {2} → {3}    Armure {4} → {5}    Armure magique {6} → {7}"),
		FText::AsNumber(View.PreviousMaxHealth), FText::AsNumber(View.NewMaxHealth), FText::AsNumber(View.PreviousMaxMana), FText::AsNumber(View.NewMaxMana),
		FText::AsNumber(View.PreviousPhysicalArmor), FText::AsNumber(View.NewPhysicalArmor), FText::AsNumber(View.PreviousMagicalArmor),
		FText::AsNumber(View.NewMagicalArmor));
}

FText URPGLevelUpWidget::BuildPointsLine() const
{
	const FText Label =
		View.Presentation.TalentPointsLabel.IsEmpty() ? LOCTEXT("TalentPointsFallback", "Points de talent") : View.Presentation.TalentPointsLabel;
	return FText::Format(LOCTEXT("TalentPointsLine", "{0} : {1} accordés, {2} dépensés, {3} disponibles"), Label, FText::AsNumber(View.GrantedChoicePoints),
		FText::AsNumber(View.SpentChoicePoints), FText::AsNumber(View.RemainingChoicePoints));
}

FReply URPGLevelUpWidget::HandleNativeChoiceClicked(FName ChoiceId)
{
	StageOrUnstageChoice(ChoiceId);
	return FReply::Handled();
}

FReply URPGLevelUpWidget::HandleNativeConfirmClicked()
{
	ConfirmSelection();
	return FReply::Handled();
}

FReply URPGLevelUpWidget::HandleNativeCancelClicked()
{
	CancelSelection();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

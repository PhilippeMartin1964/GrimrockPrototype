#include "UI/GridCharacterSheetWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridPersistentHudWidget.h"
#include "UI/RPGProgressionFeedbackService.h"

#define LOCTEXT_NAMESPACE "GridCharacterSheetWidget"

void UGridCharacterSheetWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindAttributeButtons();
}

void UGridCharacterSheetWidget::NativeDestruct()
{
	UnbindAttributeButtons();
	Super::NativeDestruct();
}

void UGridCharacterSheetWidget::RefreshInventory()
{
	Super::RefreshInventory();
	RefreshAttributeAllocationPresentation();
}

void UGridCharacterSheetWidget::BeginAttributeAllocationSession()
{
	SessionAttributeFloors.Reset();
	CaptureSelectedCharacterSessionFloors();
	RefreshAttributeAllocationPresentation();
}

void UGridCharacterSheetWidget::RefreshAttributeAllocationPresentation()
{
	if (!InventoryComponent ||
		!InventoryComponent->IsValidCharacterIndex(InventoryComponent->GetSelectedCharacterIndex()))
	{
		if (Text_AttributePoints)
		{
			Text_AttributePoints->SetText(FText::GetEmpty());
		}

		for (UButton* Button : {
			Button_DecreaseStrength.Get(), Button_IncreaseStrength.Get(),
			Button_DecreaseDexterity.Get(), Button_IncreaseDexterity.Get(),
			Button_DecreaseConstitution.Get(), Button_IncreaseConstitution.Get(),
			Button_DecreaseIntelligence.Get(), Button_IncreaseIntelligence.Get(),
			Button_DecreaseWisdom.Get(), Button_IncreaseWisdom.Get(),
			Button_DecreaseCharisma.Get(), Button_IncreaseCharisma.Get() })
		{
			if (Button)
			{
				Button->SetIsEnabled(false);
			}
		}
		return;
	}

	CaptureSelectedCharacterSessionFloors();

	const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex();
	const FGridCharacterInventoryState& Character =
		InventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];

	FRPGAttributePointBalance Balance;
	if (Text_AttributePoints)
	{
		if (FRPGAttributePointService::TryGetBalance(Character, Balance))
		{
			Text_AttributePoints->SetText(FText::Format(
				LOCTEXT("AttributePoints", "Points de caractéristiques : {0}"),
				FText::AsNumber(Balance.RemainingPoints)));
		}
		else
		{
			Text_AttributePoints->SetText(
				LOCTEXT("AttributePointsUnavailable", "Points de caractéristiques : —"));
		}
	}

	SetAttributeButtonState(Button_DecreaseStrength.Get(), Button_IncreaseStrength.Get(), Character, ERPGAttributePointTarget::Strength);
	SetAttributeButtonState(Button_DecreaseDexterity.Get(), Button_IncreaseDexterity.Get(), Character, ERPGAttributePointTarget::Dexterity);
	SetAttributeButtonState(Button_DecreaseConstitution.Get(), Button_IncreaseConstitution.Get(), Character, ERPGAttributePointTarget::Constitution);
	SetAttributeButtonState(Button_DecreaseIntelligence.Get(), Button_IncreaseIntelligence.Get(), Character, ERPGAttributePointTarget::Intelligence);
	SetAttributeButtonState(Button_DecreaseWisdom.Get(), Button_IncreaseWisdom.Get(), Character, ERPGAttributePointTarget::Wisdom);
	SetAttributeButtonState(Button_DecreaseCharisma.Get(), Button_IncreaseCharisma.Get(), Character, ERPGAttributePointTarget::Charisma);
}

void UGridCharacterSheetWidget::BindAttributeButtons()
{
	if (Button_DecreaseStrength) Button_DecreaseStrength->OnClicked.AddUniqueDynamic(this, &UGridCharacterSheetWidget::HandleDecreaseStrengthClicked);
	if (Button_IncreaseStrength) Button_IncreaseStrength->OnClicked.AddUniqueDynamic(this, &UGridCharacterSheetWidget::HandleIncreaseStrengthClicked);
	if (Button_DecreaseDexterity) Button_DecreaseDexterity->OnClicked.AddUniqueDynamic(this, &UGridCharacterSheetWidget::HandleDecreaseDexterityClicked);
	if (Button_IncreaseDexterity) Button_IncreaseDexterity->OnClicked.AddUniqueDynamic(this, &UGridCharacterSheetWidget::HandleIncreaseDexterityClicked);
	if (Button_DecreaseConstitution) Button_DecreaseConstitution->OnClicked.AddUniqueDynamic(this, &UGridCharacterSheetWidget::HandleDecreaseConstitutionClicked);
	if (Button_IncreaseConstitution) Button_IncreaseConstitution->OnClicked.AddUniqueDynamic(this, &UGridCharacterSheetWidget::HandleIncreaseConstitutionClicked);
	if (Button_DecreaseIntelligence) Button_DecreaseIntelligence->OnClicked.AddUniqueDynamic(this, &UGridCharacterSheetWidget::HandleDecreaseIntelligenceClicked);
	if (Button_IncreaseIntelligence) Button_IncreaseIntelligence->OnClicked.AddUniqueDynamic(this, &UGridCharacterSheetWidget::HandleIncreaseIntelligenceClicked);
	if (Button_DecreaseWisdom) Button_DecreaseWisdom->OnClicked.AddUniqueDynamic(this, &UGridCharacterSheetWidget::HandleDecreaseWisdomClicked);
	if (Button_IncreaseWisdom) Button_IncreaseWisdom->OnClicked.AddUniqueDynamic(this, &UGridCharacterSheetWidget::HandleIncreaseWisdomClicked);
	if (Button_DecreaseCharisma) Button_DecreaseCharisma->OnClicked.AddUniqueDynamic(this, &UGridCharacterSheetWidget::HandleDecreaseCharismaClicked);
	if (Button_IncreaseCharisma) Button_IncreaseCharisma->OnClicked.AddUniqueDynamic(this, &UGridCharacterSheetWidget::HandleIncreaseCharismaClicked);
}

void UGridCharacterSheetWidget::UnbindAttributeButtons()
{
	if (Button_DecreaseStrength) Button_DecreaseStrength->OnClicked.RemoveDynamic(this, &UGridCharacterSheetWidget::HandleDecreaseStrengthClicked);
	if (Button_IncreaseStrength) Button_IncreaseStrength->OnClicked.RemoveDynamic(this, &UGridCharacterSheetWidget::HandleIncreaseStrengthClicked);
	if (Button_DecreaseDexterity) Button_DecreaseDexterity->OnClicked.RemoveDynamic(this, &UGridCharacterSheetWidget::HandleDecreaseDexterityClicked);
	if (Button_IncreaseDexterity) Button_IncreaseDexterity->OnClicked.RemoveDynamic(this, &UGridCharacterSheetWidget::HandleIncreaseDexterityClicked);
	if (Button_DecreaseConstitution) Button_DecreaseConstitution->OnClicked.RemoveDynamic(this, &UGridCharacterSheetWidget::HandleDecreaseConstitutionClicked);
	if (Button_IncreaseConstitution) Button_IncreaseConstitution->OnClicked.RemoveDynamic(this, &UGridCharacterSheetWidget::HandleIncreaseConstitutionClicked);
	if (Button_DecreaseIntelligence) Button_DecreaseIntelligence->OnClicked.RemoveDynamic(this, &UGridCharacterSheetWidget::HandleDecreaseIntelligenceClicked);
	if (Button_IncreaseIntelligence) Button_IncreaseIntelligence->OnClicked.RemoveDynamic(this, &UGridCharacterSheetWidget::HandleIncreaseIntelligenceClicked);
	if (Button_DecreaseWisdom) Button_DecreaseWisdom->OnClicked.RemoveDynamic(this, &UGridCharacterSheetWidget::HandleDecreaseWisdomClicked);
	if (Button_IncreaseWisdom) Button_IncreaseWisdom->OnClicked.RemoveDynamic(this, &UGridCharacterSheetWidget::HandleIncreaseWisdomClicked);
	if (Button_DecreaseCharisma) Button_DecreaseCharisma->OnClicked.RemoveDynamic(this, &UGridCharacterSheetWidget::HandleDecreaseCharismaClicked);
	if (Button_IncreaseCharisma) Button_IncreaseCharisma->OnClicked.RemoveDynamic(this, &UGridCharacterSheetWidget::HandleIncreaseCharismaClicked);
}

void UGridCharacterSheetWidget::CaptureSelectedCharacterSessionFloors()
{
	if (!InventoryComponent ||
		!InventoryComponent->IsValidCharacterIndex(InventoryComponent->GetSelectedCharacterIndex()))
	{
		return;
	}

	const FGridCharacterInventoryState& Character =
		InventoryComponent->PartyInventoryState.ActiveCharacters[InventoryComponent->GetSelectedCharacterIndex()];

	for (const ERPGAttributePointTarget Target : {
		ERPGAttributePointTarget::Strength,
		ERPGAttributePointTarget::Dexterity,
		ERPGAttributePointTarget::Constitution,
		ERPGAttributePointTarget::Intelligence,
		ERPGAttributePointTarget::Wisdom,
		ERPGAttributePointTarget::Charisma })
	{
		const FString Key = MakeSessionKey(Character.CharacterId, Target);
		if (!SessionAttributeFloors.Contains(Key))
		{
			SessionAttributeFloors.Add(Key, FRPGAttributePointService::GetAttributeValue(Character, Target));
		}
	}
}

FString UGridCharacterSheetWidget::MakeSessionKey(
	const FGuid& CharacterId,
	ERPGAttributePointTarget Target) const
{
	return FString::Printf(
		TEXT("%s|%d"),
		*CharacterId.ToString(EGuidFormats::Digits),
		static_cast<int32>(Target));
}

int32 UGridCharacterSheetWidget::GetSessionFloor(
	const FGridCharacterInventoryState& Character,
	ERPGAttributePointTarget Target) const
{
	const int32* Floor = SessionAttributeFloors.Find(MakeSessionKey(Character.CharacterId, Target));
	return Floor ? *Floor : FRPGAttributePointService::GetAttributeValue(Character, Target);
}

void UGridCharacterSheetWidget::SetAttributeButtonState(
	UButton* DecreaseButton,
	UButton* IncreaseButton,
	const FGridCharacterInventoryState& Character,
	ERPGAttributePointTarget Target)
{
	if (IncreaseButton)
	{
		IncreaseButton->SetIsEnabled(
			FRPGAttributePointService::GetIncreaseAvailability(Character, Target) ==
			ERPGAttributePointMutationRejectReason::None);
	}

	if (DecreaseButton)
	{
		const int32 CurrentValue = FRPGAttributePointService::GetAttributeValue(Character, Target);
		DecreaseButton->SetIsEnabled(CurrentValue > GetSessionFloor(Character, Target));
	}
}

void UGridCharacterSheetWidget::CommitAttributeIncrease(
	ERPGAttributePointTarget Target,
	const FText& DisplayName)
{
	if (!InventoryComponent ||
		!InventoryComponent->IsValidCharacterIndex(InventoryComponent->GetSelectedCharacterIndex()))
	{
		return;
	}

	CaptureSelectedCharacterSessionFloors();

	FRPGAttributePointMutationResult Result;
	FRPGAttributePointService::TryPurchasePoint(
		InventoryComponent,
		InventoryComponent->GetSelectedCharacterIndex(),
		Target,
		Result);

	ShowAttributeFeedback(
		FRPGProgressionFeedbackService::MakeAttributePointPurchaseNotification(Result, DisplayName));
	RefreshAttributeAllocationPresentation();
}

void UGridCharacterSheetWidget::CommitAttributeDecrease(
	ERPGAttributePointTarget Target,
	const FText& DisplayName)
{
	if (!InventoryComponent ||
		!InventoryComponent->IsValidCharacterIndex(InventoryComponent->GetSelectedCharacterIndex()))
	{
		return;
	}

	CaptureSelectedCharacterSessionFloors();
	const FGridCharacterInventoryState& Character =
		InventoryComponent->PartyInventoryState.ActiveCharacters[InventoryComponent->GetSelectedCharacterIndex()];

	FRPGAttributePointMutationResult Result;
	FRPGAttributePointService::TryRefundPurchasedPoint(
		InventoryComponent,
		InventoryComponent->GetSelectedCharacterIndex(),
		Target,
		GetSessionFloor(Character, Target),
		Result);

	ShowAttributeFeedback(
		FRPGProgressionFeedbackService::MakeAttributePointRefundNotification(Result, DisplayName));
	RefreshAttributeAllocationPresentation();
}

void UGridCharacterSheetWidget::ShowAttributeFeedback(
	const FRPGProgressionNotificationView& Notification)
{
	if (OwningPartyPawn && OwningPartyPawn->PersistentHudWidgetInstance)
	{
		OwningPartyPawn->PersistentHudWidgetInstance->ShowProgressionNotification(Notification);
	}
}

void UGridCharacterSheetWidget::HandleDecreaseStrengthClicked()
{
	CommitAttributeDecrease(ERPGAttributePointTarget::Strength, LOCTEXT("Strength", "Force"));
}

void UGridCharacterSheetWidget::HandleIncreaseStrengthClicked()
{
	CommitAttributeIncrease(ERPGAttributePointTarget::Strength, LOCTEXT("Strength", "Force"));
}

void UGridCharacterSheetWidget::HandleDecreaseDexterityClicked()
{
	CommitAttributeDecrease(ERPGAttributePointTarget::Dexterity, LOCTEXT("Dexterity", "Dextérité"));
}

void UGridCharacterSheetWidget::HandleIncreaseDexterityClicked()
{
	CommitAttributeIncrease(ERPGAttributePointTarget::Dexterity, LOCTEXT("Dexterity", "Dextérité"));
}

void UGridCharacterSheetWidget::HandleDecreaseConstitutionClicked()
{
	CommitAttributeDecrease(ERPGAttributePointTarget::Constitution, LOCTEXT("Constitution", "Constitution"));
}

void UGridCharacterSheetWidget::HandleIncreaseConstitutionClicked()
{
	CommitAttributeIncrease(ERPGAttributePointTarget::Constitution, LOCTEXT("Constitution", "Constitution"));
}

void UGridCharacterSheetWidget::HandleDecreaseIntelligenceClicked()
{
	CommitAttributeDecrease(ERPGAttributePointTarget::Intelligence, LOCTEXT("Intelligence", "Intelligence"));
}

void UGridCharacterSheetWidget::HandleIncreaseIntelligenceClicked()
{
	CommitAttributeIncrease(ERPGAttributePointTarget::Intelligence, LOCTEXT("Intelligence", "Intelligence"));
}

void UGridCharacterSheetWidget::HandleDecreaseWisdomClicked()
{
	CommitAttributeDecrease(ERPGAttributePointTarget::Wisdom, LOCTEXT("Wisdom", "Sagesse"));
}

void UGridCharacterSheetWidget::HandleIncreaseWisdomClicked()
{
	CommitAttributeIncrease(ERPGAttributePointTarget::Wisdom, LOCTEXT("Wisdom", "Sagesse"));
}

void UGridCharacterSheetWidget::HandleDecreaseCharismaClicked()
{
	CommitAttributeDecrease(ERPGAttributePointTarget::Charisma, LOCTEXT("Charisma", "Charisme"));
}

void UGridCharacterSheetWidget::HandleIncreaseCharismaClicked()
{
	CommitAttributeIncrease(ERPGAttributePointTarget::Charisma, LOCTEXT("Charisma", "Charisme"));
}

#undef LOCTEXT_NAMESPACE

#include "UI/RPGCharacterCreationWizardWidget.h"

#include "UI/UILocalizationText.h"

#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableText.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/World.h"
#include "RPG/RPGCharacterPortraitSetAsset.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "RPG/RPGClassVisualAsset.h"
#include "RPG/RPGCustomRecruitService.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockGameInstance.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/RPGCharacterCreationAttributesStepWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogGrimrockFrontendUI, Log, All);

namespace
{
	constexpr int32 CharacterCreationWizardStepCount = 5;
	const FName AttributeStrength(TEXT("Strength"));
	const FName AttributeDexterity(TEXT("Dexterity"));
	const FName AttributeConstitution(TEXT("Constitution"));
	const FName AttributeIntelligence(TEXT("Intelligence"));
	const FName AttributeWisdom(TEXT("Wisdom"));
	const FName AttributeCharisma(TEXT("Charisma"));

	int32 GetWizardStepIndex(ERPGCharacterCreationWizardStep Step)
	{
		switch (Step)
		{
			case ERPGCharacterCreationWizardStep::Race:
				return 0;
			case ERPGCharacterCreationWizardStep::Class:
				return 1;
			case ERPGCharacterCreationWizardStep::Attributes:
				return 2;
			case ERPGCharacterCreationWizardStep::Identity:
				return 3;
			case ERPGCharacterCreationWizardStep::Summary:
				return 4;
			default:
				return 0;
		}
	}

	ERPGCharacterCreationWizardStep GetWizardStepFromIndex(int32 StepIndex)
	{
		switch (FMath::Clamp(StepIndex, 0, CharacterCreationWizardStepCount - 1))
		{
			case 0:
				return ERPGCharacterCreationWizardStep::Race;
			case 1:
				return ERPGCharacterCreationWizardStep::Class;
			case 2:
				return ERPGCharacterCreationWizardStep::Attributes;
			case 3:
				return ERPGCharacterCreationWizardStep::Identity;
			case 4:
				return ERPGCharacterCreationWizardStep::Summary;
			default:
				return ERPGCharacterCreationWizardStep::Race;
		}
	}

	FText GetWizardStepTitleText(ERPGCharacterCreationWizardStep Step)
	{
		switch (Step)
		{
			case ERPGCharacterCreationWizardStep::Race:
				return FUILocalizationText::Get(TEXT("CharacterCreation.Wizard.Step.Race"));
			case ERPGCharacterCreationWizardStep::Class:
				return FUILocalizationText::Get(TEXT("CharacterCreation.Wizard.Step.Class"));
			case ERPGCharacterCreationWizardStep::Attributes:
				return FUILocalizationText::Get(TEXT("CharacterCreation.Wizard.Step.Attributes"));
			case ERPGCharacterCreationWizardStep::Identity:
				return FUILocalizationText::Get(TEXT("CharacterCreation.Wizard.Step.Identity"));
			case ERPGCharacterCreationWizardStep::Summary:
				return FUILocalizationText::Get(TEXT("CharacterCreation.Wizard.Step.Summary"));
			default:
				return FText::GetEmpty();
		}
	}

	ESlateVisibility GetVisibleWhen(bool bShouldBeVisible)
	{
		return bShouldBeVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	}

	int32 GetSafeAllocationMinimum(int32 InMinimum, int32 InMaximum)
	{
		return FMath::Max(1, FMath::Min(InMinimum, InMaximum));
	}

	int32 GetSafeAllocationMaximum(int32 InMinimum, int32 InMaximum)
	{
		return FMath::Max(GetSafeAllocationMinimum(InMinimum, InMaximum), InMaximum);
	}

	int32 GetAttributeValue(const FRPGAttributes& Attributes, FName AttributeId)
	{
		if (AttributeId == AttributeStrength)
			return Attributes.Strength;
		if (AttributeId == AttributeDexterity)
			return Attributes.Dexterity;
		if (AttributeId == AttributeConstitution)
			return Attributes.Constitution;
		if (AttributeId == AttributeIntelligence)
			return Attributes.Intelligence;
		if (AttributeId == AttributeWisdom)
			return Attributes.Wisdom;
		if (AttributeId == AttributeCharisma)
			return Attributes.Charisma;
		return 0;
	}

	void SetAttributeValue(FRPGAttributes& Attributes, FName AttributeId, int32 Value)
	{
		if (AttributeId == AttributeStrength)
			Attributes.Strength = Value;
		else if (AttributeId == AttributeDexterity)
			Attributes.Dexterity = Value;
		else if (AttributeId == AttributeConstitution)
			Attributes.Constitution = Value;
		else if (AttributeId == AttributeIntelligence)
			Attributes.Intelligence = Value;
		else if (AttributeId == AttributeWisdom)
			Attributes.Wisdom = Value;
		else if (AttributeId == AttributeCharisma)
			Attributes.Charisma = Value;
	}

	FRPGAttributes ClampAllocatedAttributes(const FRPGAttributes& Attributes, int32 InMinimum, int32 InMaximum)
	{
		const int32 Minimum = GetSafeAllocationMinimum(InMinimum, InMaximum);
		const int32 Maximum = GetSafeAllocationMaximum(InMinimum, InMaximum);
		FRPGAttributes Result = Attributes;
		Result.Strength = FMath::Clamp(Result.Strength, Minimum, Maximum);
		Result.Dexterity = FMath::Clamp(Result.Dexterity, Minimum, Maximum);
		Result.Constitution = FMath::Clamp(Result.Constitution, Minimum, Maximum);
		Result.Intelligence = FMath::Clamp(Result.Intelligence, Minimum, Maximum);
		Result.Wisdom = FMath::Clamp(Result.Wisdom, Minimum, Maximum);
		Result.Charisma = FMath::Clamp(Result.Charisma, Minimum, Maximum);
		return Result;
	}

	int32 SumAttributePointsAboveMinimum(const FRPGAttributes& Attributes, int32 Minimum)
	{
		return FMath::Max(0, Attributes.Strength - Minimum) + FMath::Max(0, Attributes.Dexterity - Minimum) + FMath::Max(0, Attributes.Constitution - Minimum) +
			FMath::Max(0, Attributes.Intelligence - Minimum) + FMath::Max(0, Attributes.Wisdom - Minimum) + FMath::Max(0, Attributes.Charisma - Minimum);
	}

	FString NormalizeCharacterName(UEditableText* EditableText)
	{
		FString Name = EditableText ? EditableText->GetText().ToString() : FString();
		Name.TrimStartAndEndInline();
		return Name;
	}

	FText GetDisplayNameText(const FText& DisplayName, FName FallbackId)
	{
		return DisplayName.IsEmpty() ? (FallbackId.IsNone() ? FText::FromString(TEXT("-")) : FText::FromName(FallbackId)) : DisplayName;
	}

	FText GetGenderDisplayNameText(ERPGCharacterPortraitGender Gender)
	{
		return FUILocalizationText::Get(
			Gender == ERPGCharacterPortraitGender::Female ? TEXT("CharacterCreation.Gender.Female") : TEXT("CharacterCreation.Gender.Male"));
	}

	FText FormatAttributeSummaryLine(
		const TCHAR* LabelKey, FName AttributeId, const FRPGAttributes& ClassAttributes, const FRPGAttributes& RaceBonuses, const FRPGAttributes& FinalAttributes)
	{
		const int32 ClassValue = GetAttributeValue(ClassAttributes, AttributeId);
		const int32 RaceValue = GetAttributeValue(RaceBonuses, AttributeId);
		const int32 FinalValue = GetAttributeValue(FinalAttributes, AttributeId);
		const int32 Modifier = URPGCharacterRulesLibrary::GetAttributeModifier(FinalValue);
		return FText::FormatNamed(
			FUILocalizationText::Get(TEXT("CharacterCreation.Summary.Attribute.Format")),
			TEXT("Label"), FUILocalizationText::Get(LabelKey),
			TEXT("ClassValue"), FText::AsNumber(ClassValue),
			TEXT("RaceBonus"), FText::FromString(FString::Printf(TEXT("%+d"), RaceValue)),
			TEXT("TotalValue"), FText::AsNumber(FinalValue),
			TEXT("Modifier"), FText::FromString(FString::Printf(TEXT("%+d"), Modifier)));
	}

	FText FormatUnavailableAttributeSummaryLine(const TCHAR* LabelKey)
	{
		return FText::FormatNamed(
			FUILocalizationText::Get(TEXT("CharacterCreation.Summary.Attribute.Unavailable")),
			TEXT("Label"), FUILocalizationText::Get(LabelKey));
	}

	void SetWizardOptionalText(UTextBlock* TextBlock, const FText& Value)
	{
		if (TextBlock)
		{
			TextBlock->SetText(Value);
		}
	}

	void SetWizardOptionalImageBrush(UImage* Image, const TSoftObjectPtr<UTexture2D>& TextureRef, bool bCollapseWhenMissing)
	{
		if (!Image)
		{
			return;
		}

		if (TextureRef.IsNull())
		{
			if (bCollapseWhenMissing)
			{
				Image->SetVisibility(ESlateVisibility::Collapsed);
			}
			return;
		}

		UTexture2D* Texture = TextureRef.LoadSynchronous();
		if (Texture)
		{
			Image->SetBrushFromTexture(Texture, true);
			Image->SetVisibility(ESlateVisibility::Visible);
		}
		else if (bCollapseWhenMissing)
		{
			Image->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	FText BuildWizardValidationMessage(const URPGCharacterCreationWizardWidget* Widget, bool& bOutIsError)
	{
		bOutIsError = false;

		if (!Widget)
		{
			return FText::GetEmpty();
		}

		if (!Widget->InventoryComponent)
		{
			bOutIsError = true;
			return FUILocalizationText::Get(TEXT("CharacterCreation.Validation.InventoryUnavailable"));
		}

		const FGridPartyInventoryState& PartyState = Widget->InventoryComponent->PartyInventoryState;
		const bool bCustomRecruit = Widget->CreationContext == ERPGCharacterCreationContext::CustomRecruit;

		if (!bCustomRecruit && PartyState.bInitialCharacterCreationCompleted)
		{
			bOutIsError = true;
			return FUILocalizationText::Get(TEXT("CharacterCreation.Validation.InitialAlreadyCompleted"));
		}

		if (bCustomRecruit && !PartyState.bInitialCharacterCreationCompleted)
		{
			bOutIsError = true;
			return FUILocalizationText::Get(TEXT("CharacterCreation.Validation.MainHeroRequiredForRecruit"));
		}

		if (bCustomRecruit &&
			(PartyState.ActiveCharacters.IsEmpty() || PartyState.ActiveEquipment.Num() != PartyState.ActiveCharacters.Num() ||
				!PartyState.ActiveCharacters.IsValidIndex(PartyState.SelectedCharacterIndex) ||
				PartyState.MaxActiveCharacters < PartyState.ActiveCharacters.Num()))
		{
			bOutIsError = true;
			return FUILocalizationText::Get(TEXT("CharacterCreation.Validation.InvalidPartyState"));
		}

		if (bCustomRecruit && PartyState.ActiveCharacters.Num() >= PartyState.MaxActiveCharacters)
		{
			bOutIsError = true;
			return FUILocalizationText::Get(TEXT("CharacterCreation.Validation.PartyFull"));
		}

		if (!Widget->RaceDefinition || !Widget->RaceDefinition->IsValidDefinition())
		{
			bOutIsError = true;
			return FUILocalizationText::Get(TEXT("CharacterCreation.Validation.RaceNotConfigured"));
		}

		if (!Widget->ClassDefinition || !Widget->ClassDefinition->IsValidDefinition())
		{
			bOutIsError = true;
			return FUILocalizationText::Get(TEXT("CharacterCreation.Validation.ClassNotConfigured"));
		}

		const int32 RemainingAttributePoints = Widget->GetRemainingAttributePoints();
		const FString NormalizedName = NormalizeCharacterName(Widget->EditableText_Name);

		if (RemainingAttributePoints > 0 &&
			(Widget->CurrentWizardStep == ERPGCharacterCreationWizardStep::Attributes ||
				Widget->CurrentWizardStep == ERPGCharacterCreationWizardStep::Identity ||
				Widget->CurrentWizardStep == ERPGCharacterCreationWizardStep::Summary))
		{
			return FText::FormatNamed(
				FUILocalizationText::Get(TEXT("CharacterCreation.Validation.RemainingAttributePoints")),
				TEXT("Count"), FText::AsNumber(RemainingAttributePoints));
		}

		if (NormalizedName.Len() < 1 &&
			(Widget->CurrentWizardStep == ERPGCharacterCreationWizardStep::Identity || Widget->CurrentWizardStep == ERPGCharacterCreationWizardStep::Summary))
		{
			return FUILocalizationText::Get(
				bCustomRecruit ? TEXT("CharacterCreation.Validation.NameRequired.Recruit") : TEXT("CharacterCreation.Validation.NameRequired.Create"));
		}

		if (NormalizedName.Len() > 24)
		{
			bOutIsError = true;
			return FUILocalizationText::Get(TEXT("CharacterCreation.Validation.NameTooLong"));
		}

		switch (Widget->CurrentWizardStep)
		{
			case ERPGCharacterCreationWizardStep::Race:
				return FUILocalizationText::Get(TEXT("CharacterCreation.Wizard.Guide.Race"));
			case ERPGCharacterCreationWizardStep::Class:
				return FUILocalizationText::Get(TEXT("CharacterCreation.Wizard.Guide.Class"));
			case ERPGCharacterCreationWizardStep::Attributes:
				return FUILocalizationText::Get(TEXT("CharacterCreation.Wizard.Guide.AttributesReady"));
			case ERPGCharacterCreationWizardStep::Identity:
				return FUILocalizationText::Get(TEXT("CharacterCreation.Wizard.Guide.IdentityReady"));
			case ERPGCharacterCreationWizardStep::Summary:
				if (Widget->CanSubmitCharacterCreation())
				{
					return FUILocalizationText::Get(
						bCustomRecruit ? TEXT("CharacterCreation.Validation.Ready.Recruit") : TEXT("CharacterCreation.Validation.Ready.Create"));
				}

				bOutIsError = true;
				return FUILocalizationText::Get(
					bCustomRecruit ? TEXT("CharacterCreation.Validation.PreviousChoices.Recruit")
								   : TEXT("CharacterCreation.Validation.PreviousChoices.Create"));
			default:
				return FText::GetEmpty();
		}
	}

	void RefreshWizardValidationMessage(URPGCharacterCreationWizardWidget* Widget)
	{
		if (!Widget || !Widget->Text_ValidationMessage)
		{
			return;
		}

		bool bIsError = false;
		const FText Message = BuildWizardValidationMessage(Widget, bIsError);
		Widget->Text_ValidationMessage->SetText(Message);
		Widget->Text_ValidationMessage->SetColorAndOpacity(
			bIsError ? FSlateColor(FLinearColor(0.85f, 0.12f, 0.08f)) : FSlateColor(FLinearColor(0.80f, 0.72f, 0.55f)));
		Widget->Text_ValidationMessage->SetVisibility(Message.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void URPGCharacterCreationWizardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CurrentWizardStep = InitialWizardStep;
	BindWizardButtons();
	BindWizardSubmitButton();
	if (ComboBox_StartingKit)
	{
		ComboBox_StartingKit->OnSelectionChanged.RemoveDynamic(this, &URPGCharacterCreationWizardWidget::HandleStartingKitSelectionChanged);
		ComboBox_StartingKit->OnSelectionChanged.AddDynamic(this, &URPGCharacterCreationWizardWidget::HandleStartingKitSelectionChanged);
	}
	if (Widget_StepAttributes)
	{
		Widget_StepAttributes->InitializeAttributesStep(this);
	}
	else
	{
		UE_LOG(LogGrimrockFrontendUI, Error, TEXT("CharacterCreationWizard MissingRequiredWidget Widget=%s Expected=Widget_StepAttributes"), *GetName());
	}
	ApplyWizardStepToSwitcher();
	RefreshWizardShell();
	RefreshPreview();
}

TArray<FRPGStartingEquipmentKit> URPGCharacterCreationWizardWidget::GetAvailableStartingEquipmentKits() const
{
    if (CreationContext == ERPGCharacterCreationContext::CustomRecruit ||
        !IsValid(ClassDefinition.Get()) || !ClassDefinition->IsValidDefinition())
    {
        return {};
    }

    if (FRPGStartingEquipmentPresetCatalog::IsProductionClassAsset(ClassDefinition) &&
        ClassDefinition->StartingEquipmentKits.IsEmpty() &&
        ClassDefinition->DefaultStartingEquipmentKitId.IsNone())
    {
        // The production DataAsset is read-only. Recipes live on a transient
        // class copy, exactly as in CreateInitialCharacter().
        URPGClassAsset* PreviewClass = DuplicateObject<URPGClassAsset>(ClassDefinition, GetTransientPackage());
        FString Error;
        if (!PreviewClass || !FRPGStartingEquipmentPresetCatalog::TryPopulateTransientClass(*PreviewClass, Error))
        {
            UE_LOG(LogGrimrockFrontendUI, Warning, TEXT("Starting kit preview unavailable: %s"), *Error);
            return {};
        }
        return PreviewClass->StartingEquipmentKits;
    }
    return ClassDefinition->StartingEquipmentKits;
}

FName URPGCharacterCreationWizardWidget::GetStartingKitDefaultId() const
{
    const TArray<FRPGStartingEquipmentKit> Kits = GetAvailableStartingEquipmentKits();
    if (Kits.IsEmpty())
    {
        return NAME_None;
    }
    if (ClassDefinition && !ClassDefinition->DefaultStartingEquipmentKitId.IsNone())
    {
        return ClassDefinition->DefaultStartingEquipmentKitId;
    }
    // Source-controlled production presets always list their default first.
    return Kits[0].KitId;
}

FName URPGCharacterCreationWizardWidget::GetSelectedStartingEquipmentKitId() const
{
    if (CreationContext == ERPGCharacterCreationContext::CustomRecruit)
    {
        return NAME_None;
    }
    return StartingKitSelectionClass == ClassDefinition
        ? SelectedStartingEquipmentKitId : GetStartingKitDefaultId();
}

void URPGCharacterCreationWizardWidget::RefreshStartingKitSelection()
{
    if (StartingKitSelectionClass != ClassDefinition)
    {
        StartingKitSelectionClass = ClassDefinition;
        SelectedStartingEquipmentKitId = GetStartingKitDefaultId();
        return;
    }
    const TArray<FRPGStartingEquipmentKit> Kits = GetAvailableStartingEquipmentKits();
    if (!Kits.ContainsByPredicate([this](const FRPGStartingEquipmentKit& Kit)
        { return Kit.KitId == SelectedStartingEquipmentKitId; }))
    {
        SelectedStartingEquipmentKitId = GetStartingKitDefaultId();
    }
}

bool URPGCharacterCreationWizardWidget::SelectStartingEquipmentKit(FName KitId)
{
    if (KitId.IsNone())
    {
        return false;
    }
    const TArray<FRPGStartingEquipmentKit> Kits = GetAvailableStartingEquipmentKits();
    if (!Kits.ContainsByPredicate([KitId](const FRPGStartingEquipmentKit& Kit)
        { return Kit.KitId == KitId; }))
    {
        return false;
    }
    StartingKitSelectionClass = ClassDefinition;
    SelectedStartingEquipmentKitId = KitId;
    SetValidationMessage(FText::GetEmpty(), false);
    RefreshPreview();
    return true;
}

FText URPGCharacterCreationWizardWidget::GetStartingKitOptionLabel(const FRPGStartingEquipmentKit& Kit) const
{
    // Player-facing selection: never expose the internal FName as a suffix.
    return Kit.DisplayName.IsEmpty() ? FText::FromName(Kit.KitId) : Kit.DisplayName;
}

FText URPGCharacterCreationWizardWidget::GetSelectedStartingEquipmentKitSummary() const
{
    const FName SelectedId = GetSelectedStartingEquipmentKitId();
    for (const FRPGStartingEquipmentKit& Kit : GetAvailableStartingEquipmentKits())
    {
        if (Kit.KitId == SelectedId)
        {
            const FString Name = GetStartingKitOptionLabel(Kit).ToString();
            const FString Details = GetSelectedStartingEquipmentKitDescription().ToString();
            return FText::FromString(Details.IsEmpty() ? Name : Name + TEXT("\n") + Details);
        }
    }
    return FText::GetEmpty();
}

FText URPGCharacterCreationWizardWidget::GetSelectedStartingEquipmentKitDescription() const
{
    const FName KitId = GetSelectedStartingEquipmentKitId();
    for (const FRPGStartingEquipmentKit& Kit : GetAvailableStartingEquipmentKits())
    {
        if (Kit.KitId != KitId)
        {
            continue;
        }
        // The selected kit name is already displayed in the ComboBox.
        // Keep this text for the actual contents, followed by the technical ID.
        FString Description;
        for (const FRPGStartingEquipmentEntry& Entry : Kit.Items)
        {
            const UGridItemDefinitionAsset* Item = Entry.ItemDefinition.Get();
            if (!Item)
            {
                continue;
            }
            const FString ItemName = Item->DisplayName.IsEmpty() ? Item->ItemDefinitionId.ToString() : Item->DisplayName.ToString();
            const FString Location = Entry.TargetSlot == EGridEquipmentSlot::None ? TEXT("Sac") :
                StaticEnum<EGridEquipmentSlot>()->GetDisplayNameTextByValue(static_cast<int64>(Entry.TargetSlot)).ToString();
            const int32 Level = Entry.ItemLevel == 0 && Item->bUseWeaponCatalogue
                ? Item->DefaultWeaponItemLevel : Entry.ItemLevel;
            if (!Description.IsEmpty())
            {
                Description += TEXT("\n");
            }
            Description += FString::Printf(TEXT("%d × %s — %s%s"),
                Entry.Quantity, *ItemName, *Location,
                Level > 0 ? *FString::Printf(TEXT(" (niv. %d)"), Level) : TEXT(""));
        }
        if (!Description.IsEmpty())
        {
            Description += TEXT("\n\n");
        }
        Description += FString::Printf(TEXT("Référence technique : %s"), *Kit.KitId.ToString());
        return FText::FromString(Description);
    }
    return FText::GetEmpty();
}

void URPGCharacterCreationWizardWidget::RefreshStartingKitWidgets()
{
    const TArray<FRPGStartingEquipmentKit> Kits = GetAvailableStartingEquipmentKits();
    if (ComboBox_StartingKit)
    {
        bUpdatingStartingKitCombo = true;
        ComboBox_StartingKit->ClearOptions();
        const FName SelectedId = GetSelectedStartingEquipmentKitId();
        FString SelectedOption;
        for (const FRPGStartingEquipmentKit& Kit : Kits)
        {
            const FString Label = GetStartingKitOptionLabel(Kit).ToString();
            ComboBox_StartingKit->AddOption(Label);
            if (Kit.KitId == SelectedId)
            {
                SelectedOption = Label;
            }
        }
        if (!SelectedOption.IsEmpty())
        {
            ComboBox_StartingKit->SetSelectedOption(SelectedOption);
        }
        ComboBox_StartingKit->SetIsEnabled(Kits.Num() > 1);
        ComboBox_StartingKit->SetVisibility(GetVisibleWhen(!Kits.IsEmpty()));
        bUpdatingStartingKitCombo = false;
    }
    SetWizardOptionalText(Text_StartingKitDescription, GetSelectedStartingEquipmentKitDescription());
}

void URPGCharacterCreationWizardWidget::HandleStartingKitSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    (void)SelectionType;
    if (bUpdatingStartingKitCombo)
    {
        return;
    }
    for (const FRPGStartingEquipmentKit& Kit : GetAvailableStartingEquipmentKits())
    {
        if (SelectedItem == GetStartingKitOptionLabel(Kit).ToString())
        {
            SelectStartingEquipmentKit(Kit.KitId);
            return;
        }
    }
}

void URPGCharacterCreationWizardWidget::BindWizardButtons()
{
	if (Button_Previous)
	{
		Button_Previous->OnClicked.RemoveDynamic(this, &URPGCharacterCreationWizardWidget::HandlePreviousClicked);
		Button_Previous->OnClicked.AddDynamic(this, &URPGCharacterCreationWizardWidget::HandlePreviousClicked);
	}
	if (Button_Next)
	{
		Button_Next->OnClicked.RemoveDynamic(this, &URPGCharacterCreationWizardWidget::HandleNextClicked);
		Button_Next->OnClicked.AddDynamic(this, &URPGCharacterCreationWizardWidget::HandleNextClicked);
	}
	if (Button_Cancel)
	{
		Button_Cancel->OnClicked.RemoveDynamic(this, &URPGCharacterCreationWizardWidget::HandleCancelClicked);
		Button_Cancel->OnClicked.AddDynamic(this, &URPGCharacterCreationWizardWidget::HandleCancelClicked);
	}
}

void URPGCharacterCreationWizardWidget::BindWizardSubmitButton()
{
	if (!Button_CreateCharacter)
	{
		return;
	}
	Button_CreateCharacter->OnClicked.RemoveAll(this);
	Button_CreateCharacter->OnClicked.AddDynamic(this, &URPGCharacterCreationWizardWidget::HandleWizardCreateCharacterClicked);
}

void URPGCharacterCreationWizardWidget::SetCurrentWizardStep(ERPGCharacterCreationWizardStep NewStep)
{
	const ERPGCharacterCreationWizardStep PreviousStep = CurrentWizardStep;
	CurrentWizardStep = NewStep;
	ApplyWizardStepToSwitcher();
	RefreshWizardShell();
	if (PreviousStep != CurrentWizardStep)
	{
		OnWizardStepChanged(PreviousStep, CurrentWizardStep);
	}
	if (bFocusNameInputOnIdentityStep && CurrentWizardStep == ERPGCharacterCreationWizardStep::Identity)
	{
		FocusNameInput();
	}
}

bool URPGCharacterCreationWizardWidget::GoToNextWizardStep()
{
	if (!CanGoToNextWizardStep())
		return false;
	SetCurrentWizardStep(GetWizardStepFromIndex(GetCurrentWizardStepIndex() + 1));
	return true;
}

bool URPGCharacterCreationWizardWidget::GoToPreviousWizardStep()
{
	if (!CanGoToPreviousWizardStep())
		return false;
	SetCurrentWizardStep(GetWizardStepFromIndex(GetCurrentWizardStepIndex() - 1));
	return true;
}

void URPGCharacterCreationWizardWidget::CancelWizard()
{
	UE_LOG(LogGrimrockFrontendUI, Verbose, TEXT("CharacterCreationWizard Cancelled Widget=%s Context=%d"), *GetName(), static_cast<int32>(CreationContext));

	if (CreationContext == ERPGCharacterCreationContext::CustomRecruit)
	{
		NotifyCustomRecruitCancelled();
		RemoveFromParent();
		return;
	}

	if (!OwningPartyPawn)
	{
		NotifyInitialCharacterCreationCancelled();
		RemoveFromParent();
		return;
	}

	RemoveFromParent();

	UGrimrockGameInstance* GrimrockGameInstance = GetWorld() ? GetWorld()->GetGameInstance<UGrimrockGameInstance>() : nullptr;

	if (GrimrockGameInstance)
	{
		GrimrockGameInstance->RequestReturnToMainMenu(this);
		return;
	}

	UE_LOG(LogGrimrockFrontendUI, Error, TEXT("CharacterCreationWizard Cancel Failed Widget=%s Reason=NoGrimrockGameInstance"), *GetName());
}

bool URPGCharacterCreationWizardWidget::CanGoToNextWizardStep() const
{
	return GetCurrentWizardStepIndex() < CharacterCreationWizardStepCount - 1;
}
bool URPGCharacterCreationWizardWidget::CanGoToPreviousWizardStep() const
{
	return GetCurrentWizardStepIndex() > 0;
}
bool URPGCharacterCreationWizardWidget::IsWizardOnLastStep() const
{
	return GetCurrentWizardStepIndex() == CharacterCreationWizardStepCount - 1;
}
int32 URPGCharacterCreationWizardWidget::GetCurrentWizardStepIndex() const
{
	return GetWizardStepIndex(CurrentWizardStep);
}
int32 URPGCharacterCreationWizardWidget::GetCurrentWizardStepNumber() const
{
	return GetCurrentWizardStepIndex() + 1;
}
int32 URPGCharacterCreationWizardWidget::GetWizardStepCount() const
{
	return CharacterCreationWizardStepCount;
}
FText URPGCharacterCreationWizardWidget::GetCurrentWizardStepTitle() const
{
	return GetWizardStepTitleText(CurrentWizardStep);
}

void URPGCharacterCreationWizardWidget::RefreshPreview()
{
	RefreshStartingKitSelection();
	EnsureAttributeAllocationInitialized();
	Super::RefreshPreview();
	if (Widget_StepAttributes)
	{
		Widget_StepAttributes->RefreshFromWizardState();
	}
	RefreshWizardShell();
}

bool URPGCharacterCreationWizardWidget::CanSubmitCharacterCreation() const
{
	if (!Super::CanSubmitCharacterCreation() || GetRemainingAttributePoints() != 0)
	{
		return false;
	}
	const TArray<FRPGStartingEquipmentKit> Kits = GetAvailableStartingEquipmentKits();
	if (Kits.IsEmpty())
	{
		return true;
	}
	const FName SelectedId = GetSelectedStartingEquipmentKitId();
	return Kits.ContainsByPredicate([SelectedId](const FRPGStartingEquipmentKit& Kit) { return Kit.KitId == SelectedId; });
}

bool URPGCharacterCreationWizardWidget::GetPreviewAttributes(FRPGAttributes& OutAttributes) const
{
	OutAttributes = FRPGAttributes();
	if (!RaceDefinition || !RaceDefinition->IsValidDefinition() || !ClassDefinition || !ClassDefinition->IsValidDefinition())
		return false;
	OutAttributes = URPGCharacterRulesLibrary::AddAttributes(GetAllocatedClassAttributesForPreview(), RaceDefinition->AttributeBonuses);
	return true;
}

bool URPGCharacterCreationWizardWidget::GetPreviewDerivedStats(FRPGDerivedStats& OutDerivedStats) const
{
	OutDerivedStats = FRPGDerivedStats();
	FRPGAttributes Attributes;
	if (!GetPreviewAttributes(Attributes))
		return false;
	OutDerivedStats = URPGCharacterRulesLibrary::CalculateDerivedStats(Attributes, ClassDefinition, 1);
	return true;
}

float URPGCharacterCreationWizardWidget::GetPreviewCarryWeight() const
{
	FRPGAttributes Attributes;
	return GetPreviewAttributes(Attributes) ? URPGCharacterRulesLibrary::CalculateMaxCarryWeight(Attributes) : 0.0f;
}

void URPGCharacterCreationWizardWidget::RefreshWizardShell()
{
	RefreshStartingKitWidgets();
	if (Text_StepTitle)
		Text_StepTitle->SetText(GetCurrentWizardStepTitle());
	if (Text_StepCounter)
	{
		Text_StepCounter->SetText(
			FText::Format(FText::FromString(TEXT("{0} / {1}")), FText::AsNumber(GetCurrentWizardStepNumber()), FText::AsNumber(GetWizardStepCount())));
	}
	if (Button_Previous)
		Button_Previous->SetIsEnabled(CanGoToPreviousWizardStep());
	if (Button_Next)
	{
		Button_Next->SetVisibility(GetVisibleWhen(CanGoToNextWizardStep()));
		Button_Next->SetIsEnabled(CanGoToNextWizardStep());
	}
	if (Button_CreateCharacter)
	{
		Button_CreateCharacter->SetVisibility(GetVisibleWhen(IsWizardOnLastStep()));
		Button_CreateCharacter->SetIsEnabled(IsWizardOnLastStep() && CanSubmitCharacterCreation());
	}
	if (Button_Cancel)
	{
		Button_Cancel->SetVisibility(ESlateVisibility::Visible);
		Button_Cancel->SetIsEnabled(true);
	}
	RefreshWizardValidationMessage(this);
	RefreshSummaryStep();
	UE_LOG(LogGrimrockFrontendUI, Verbose, TEXT("CharacterCreationWizard Refreshed Widget=%s Step=%d StepName=%s"), *GetName(), GetCurrentWizardStepIndex(),
		*GetCurrentWizardStepTitle().ToString());
}

void URPGCharacterCreationWizardWidget::RefreshSummaryStep()
{
	const FString Name = NormalizeCharacterName(EditableText_Name);
	const FText DisplayName = Name.IsEmpty() ? FText::FromString(TEXT("-")) : FText::FromString(Name);
	const FText RaceName = RaceDefinition && RaceDefinition->IsValidDefinition()
		? GetDisplayNameText(RaceDefinition->DisplayName, RaceDefinition->RaceId)
		: FText::FromString(TEXT("-"));
	const FText ClassName = ClassDefinition && ClassDefinition->IsValidDefinition()
		? GetDisplayNameText(ClassDefinition->DisplayName, ClassDefinition->ClassId)
		: FText::FromString(TEXT("-"));
	const FText GenderName = GetGenderDisplayNameText(SelectedPortraitGender);
	const bool bCustomRecruit = CreationContext == ERPGCharacterCreationContext::CustomRecruit;

	FRPGCharacterPortraitVariant PortraitVariant;
	FText PortraitName = FText::FromString(TEXT("-"));
	if (TryResolveWizardSelectedPortraitVariant(PortraitVariant))
	{
		PortraitName = GetDisplayNameText(PortraitVariant.DisplayName, PortraitVariant.VariantId);
	}
	else if (!SelectedPortraitVariantId.IsNone())
	{
		PortraitName = FText::FromName(SelectedPortraitVariantId);
	}

	FRPGAttributes FinalAttributes;
	const bool bHasAttributes = GetPreviewAttributes(FinalAttributes);
	const FRPGAttributes ClassAttributes = GetAllocatedClassAttributesForPreview();
	const FRPGAttributes RaceBonuses = RaceDefinition ? RaceDefinition->AttributeBonuses : FRPGAttributes(0, 0, 0, 0, 0, 0);
	FRPGDerivedStats DerivedStats;
	const bool bHasDerivedStats = GetPreviewDerivedStats(DerivedStats);

	const FText StrengthLine = bHasAttributes
		? FormatAttributeSummaryLine(TEXT("CharacterCreation.Attribute.Strength"), AttributeStrength, ClassAttributes, RaceBonuses, FinalAttributes)
		: FormatUnavailableAttributeSummaryLine(TEXT("CharacterCreation.Attribute.Strength"));
	const FText DexterityLine = bHasAttributes
		? FormatAttributeSummaryLine(TEXT("CharacterCreation.Attribute.Dexterity"), AttributeDexterity, ClassAttributes, RaceBonuses, FinalAttributes)
		: FormatUnavailableAttributeSummaryLine(TEXT("CharacterCreation.Attribute.Dexterity"));
	const FText ConstitutionLine = bHasAttributes
		? FormatAttributeSummaryLine(TEXT("CharacterCreation.Attribute.Constitution"), AttributeConstitution, ClassAttributes, RaceBonuses, FinalAttributes)
		: FormatUnavailableAttributeSummaryLine(TEXT("CharacterCreation.Attribute.Constitution"));
	const FText IntelligenceLine = bHasAttributes
		? FormatAttributeSummaryLine(TEXT("CharacterCreation.Attribute.Intelligence"), AttributeIntelligence, ClassAttributes, RaceBonuses, FinalAttributes)
		: FormatUnavailableAttributeSummaryLine(TEXT("CharacterCreation.Attribute.Intelligence"));
	const FText WisdomLine = bHasAttributes
		? FormatAttributeSummaryLine(TEXT("CharacterCreation.Attribute.Wisdom"), AttributeWisdom, ClassAttributes, RaceBonuses, FinalAttributes)
		: FormatUnavailableAttributeSummaryLine(TEXT("CharacterCreation.Attribute.Wisdom"));
	const FText CharismaLine = bHasAttributes
		? FormatAttributeSummaryLine(TEXT("CharacterCreation.Attribute.Charisma"), AttributeCharisma, ClassAttributes, RaceBonuses, FinalAttributes)
		: FormatUnavailableAttributeSummaryLine(TEXT("CharacterCreation.Attribute.Charisma"));
	const FText HealthLine = bHasDerivedStats
		? FText::FormatNamed(FUILocalizationText::Get(TEXT("CharacterCreation.Summary.Health.Format")), TEXT("Value"), FText::AsNumber(DerivedStats.MaxHealth))
		: FUILocalizationText::Get(TEXT("CharacterCreation.Summary.Health.Unavailable"));
	const FText ManaLine = bHasDerivedStats
		? FText::FormatNamed(FUILocalizationText::Get(TEXT("CharacterCreation.Summary.Mana.Format")), TEXT("Value"), FText::AsNumber(DerivedStats.MaxMana))
		: FUILocalizationText::Get(TEXT("CharacterCreation.Summary.Mana.Unavailable"));
	const FText CarryLine = bHasAttributes
		? FText::FormatNamed(
			FUILocalizationText::Get(TEXT("CharacterCreation.Summary.CarryWeight.Format")),
			TEXT("Value"), FText::AsNumber(FMath::RoundToInt(GetPreviewCarryWeight())))
		: FUILocalizationText::Get(TEXT("CharacterCreation.Summary.CarryWeight.Unavailable"));

	FText State;
	if (CanSubmitCharacterCreation())
	{
		State = FUILocalizationText::Get(
			bCustomRecruit ? TEXT("CharacterCreation.Summary.State.Ready.Recruit") : TEXT("CharacterCreation.Summary.State.Ready.Create"));
	}
	else if (!InventoryComponent)
	{
		State = FUILocalizationText::Get(TEXT("CharacterCreation.Summary.State.InventoryUnavailable"));
	}
	else if (bCustomRecruit && !InventoryComponent->HasCompletedInitialCharacterCreation())
	{
		State = FUILocalizationText::Get(TEXT("CharacterCreation.Summary.State.MainHeroMissing"));
	}
	else if (bCustomRecruit && InventoryComponent->GetActiveCharacterCount() >= InventoryComponent->GetMaxActiveCharacters())
	{
		State = FUILocalizationText::Get(TEXT("CharacterCreation.Summary.State.PartyFull"));
	}
	else if (Name.Len() < 1)
	{
		State = FUILocalizationText::Get(
			bCustomRecruit ? TEXT("CharacterCreation.Summary.State.NameMissing.Recruit") : TEXT("CharacterCreation.Summary.State.NameMissing.Create"));
	}
	else if (Name.Len() > 24)
	{
		State = FUILocalizationText::Get(
			bCustomRecruit ? TEXT("CharacterCreation.Summary.State.NameTooLong.Recruit") : TEXT("CharacterCreation.Summary.State.NameTooLong.Create"));
	}
	else if (!RaceDefinition || !RaceDefinition->IsValidDefinition())
	{
		State = FUILocalizationText::Get(
			bCustomRecruit ? TEXT("CharacterCreation.Summary.State.RaceInvalid.Recruit") : TEXT("CharacterCreation.Summary.State.RaceInvalid.Create"));
	}
	else if (!ClassDefinition || !ClassDefinition->IsValidDefinition())
	{
		State = FUILocalizationText::Get(
			bCustomRecruit ? TEXT("CharacterCreation.Summary.State.ClassInvalid.Recruit") : TEXT("CharacterCreation.Summary.State.ClassInvalid.Create"));
	}
	else if (GetRemainingAttributePoints() > 0)
	{
		State = FText::FormatNamed(
			FUILocalizationText::Get(
				bCustomRecruit ? TEXT("CharacterCreation.Summary.State.PointsRemaining.Recruit")
							   : TEXT("CharacterCreation.Summary.State.PointsRemaining.Create")),
			TEXT("Count"), FText::AsNumber(GetRemainingAttributePoints()));
	}
	else
	{
		State = FUILocalizationText::Get(
			bCustomRecruit ? TEXT("CharacterCreation.Summary.State.PreviousChoices.Recruit")
						   : TEXT("CharacterCreation.Summary.State.PreviousChoices.Create"));
	}

	SetWizardOptionalText(
		Text_SummaryName,
		FText::FormatNamed(FUILocalizationText::Get(TEXT("CharacterCreation.Summary.NameFormat")), TEXT("Name"), DisplayName));
	SetWizardOptionalText(
		Text_SummaryRace,
		FText::FormatNamed(FUILocalizationText::Get(TEXT("CharacterCreation.Summary.RaceFormat")), TEXT("Race"), RaceName));
	SetWizardOptionalText(
		Text_SummaryClass,
		FText::FormatNamed(FUILocalizationText::Get(TEXT("CharacterCreation.Summary.ClassFormat")), TEXT("Class"), ClassName));
	SetWizardOptionalText(
		Text_SummaryGender,
		FText::FormatNamed(FUILocalizationText::Get(TEXT("CharacterCreation.Summary.GenderFormat")), TEXT("Gender"), GenderName));
	SetWizardOptionalText(
		Text_SummaryPortrait,
		FText::FormatNamed(FUILocalizationText::Get(TEXT("CharacterCreation.Summary.PortraitFormat")), TEXT("Portrait"), PortraitName));
	SetWizardOptionalText(Text_SummaryStrength, StrengthLine);
	SetWizardOptionalText(Text_SummaryDexterity, DexterityLine);
	SetWizardOptionalText(Text_SummaryConstitution, ConstitutionLine);
	SetWizardOptionalText(Text_SummaryIntelligence, IntelligenceLine);
	SetWizardOptionalText(Text_SummaryWisdom, WisdomLine);
	SetWizardOptionalText(Text_SummaryCharisma, CharismaLine);
	SetWizardOptionalText(Text_SummaryHealth, HealthLine);
	SetWizardOptionalText(Text_SummaryMana, ManaLine);
	SetWizardOptionalText(Text_SummaryCarryWeight, CarryLine);
	SetWizardOptionalText(Text_SummaryStartingKit, GetSelectedStartingEquipmentKitSummary());
	SetWizardOptionalText(Text_SummaryValidationState, State);

	const bool bCanCreateCharacter = CanSubmitCharacterCreation();
	RefreshSummaryVisuals(bCanCreateCharacter);

	const FText FullSummary = FText::FormatNamed(
		FUILocalizationText::Get(TEXT("CharacterCreation.Summary.Full")),
		TEXT("Name"), DisplayName,
		TEXT("Race"), RaceName,
		TEXT("Class"), ClassName,
		TEXT("Gender"), GenderName,
		TEXT("Portrait"), PortraitName,
		TEXT("Strength"), StrengthLine,
		TEXT("Dexterity"), DexterityLine,
		TEXT("Constitution"), ConstitutionLine,
		TEXT("Intelligence"), IntelligenceLine,
		TEXT("Wisdom"), WisdomLine,
		TEXT("Charisma"), CharismaLine,
		TEXT("Health"), HealthLine,
		TEXT("Mana"), ManaLine,
		TEXT("Carry"), CarryLine,
		TEXT("State"), State);
	SetWizardOptionalText(Text_SummaryHelp, FullSummary);
}

void URPGCharacterCreationWizardWidget::RefreshSummaryVisuals(bool bCanCreateCharacter)
{
	SetWizardOptionalImageBrush(Image_SummaryPortrait, ResolveWizardSelectedPortrait(), true);
	SetWizardOptionalImageBrush(Image_SummaryClassIcon, ResolveWizardSelectedClassIcon(), true);
	SetWizardOptionalImageBrush(Image_SummaryValidationIcon, bCanCreateCharacter ? SummaryReadyIcon : SummaryBlockedIcon, false);
	SetWizardOptionalImageBrush(Image_SummaryStrengthIcon, SummaryStrengthIcon, false);
	SetWizardOptionalImageBrush(Image_SummaryDexterityIcon, SummaryDexterityIcon, false);
	SetWizardOptionalImageBrush(Image_SummaryConstitutionIcon, SummaryConstitutionIcon, false);
	SetWizardOptionalImageBrush(Image_SummaryIntelligenceIcon, SummaryIntelligenceIcon, false);
	SetWizardOptionalImageBrush(Image_SummaryWisdomIcon, SummaryWisdomIcon, false);
	SetWizardOptionalImageBrush(Image_SummaryCharismaIcon, SummaryCharismaIcon, false);
	SetWizardOptionalImageBrush(Image_SummaryHealthIcon, SummaryHealthIcon, false);
	SetWizardOptionalImageBrush(Image_SummaryManaIcon, SummaryManaIcon, false);
	SetWizardOptionalImageBrush(Image_SummaryCarryWeightIcon, SummaryCarryWeightIcon, false);
}

void URPGCharacterCreationWizardWidget::ApplyWizardStepToSwitcher()
{
	if (!WidgetSwitcher_Steps)
		return;
	UWidget* StepPanel = GetPanelForWizardStep(CurrentWizardStep);
	if (StepPanel && StepPanel->GetParent() == WidgetSwitcher_Steps)
	{
		WidgetSwitcher_Steps->SetActiveWidget(StepPanel);
		return;
	}
	WidgetSwitcher_Steps->SetActiveWidgetIndex(GetCurrentWizardStepIndex());
}

UWidget* URPGCharacterCreationWizardWidget::GetPanelForWizardStep(ERPGCharacterCreationWizardStep Step) const
{
	switch (Step)
	{
		case ERPGCharacterCreationWizardStep::Race:
			return Panel_StepRace;
		case ERPGCharacterCreationWizardStep::Class:
			return Panel_StepClass;
		case ERPGCharacterCreationWizardStep::Attributes:
			return Panel_StepAttributes;
		case ERPGCharacterCreationWizardStep::Identity:
			return Panel_StepIdentity;
		case ERPGCharacterCreationWizardStep::Summary:
			return Panel_StepSummary;
		default:
			return nullptr;
	}
}

void URPGCharacterCreationWizardWidget::ResetAttributeAllocationToClassDefinition()
{
	if (!ClassDefinition || !ClassDefinition->IsValidDefinition())
	{
		AllocatedClassAttributes = FRPGAttributes();
		AllocatedClassId = NAME_None;
		bHasInitializedAllocatedClassAttributes = false;
		return;
	}
	AllocatedClassAttributes = ClampAllocatedAttributes(ClassDefinition->BaseAttributes, AttributeAllocationMinimum, AttributeAllocationMaximum);
	AllocatedClassId = ClassDefinition->ClassId;
	bHasInitializedAllocatedClassAttributes = true;
}

void URPGCharacterCreationWizardWidget::EnsureAttributeAllocationInitialized()
{
	if (!bHasInitializedAllocatedClassAttributes || !ClassDefinition || !ClassDefinition->IsValidDefinition() || AllocatedClassId != ClassDefinition->ClassId)
	{
		ResetAttributeAllocationToClassDefinition();
	}
}

void URPGCharacterCreationWizardWidget::AdjustAllocatedAttribute(FName AttributeId, int32 Delta)
{
	EnsureAttributeAllocationInitialized();
	if (Delta > 0 && !CanIncreaseAllocatedAttribute(AttributeId))
		return;
	if (Delta < 0 && !CanDecreaseAllocatedAttribute(AttributeId))
		return;
	const int32 CurrentValue = GetAttributeValue(AllocatedClassAttributes, AttributeId);
	SetAttributeValue(AllocatedClassAttributes, AttributeId,
		FMath::Clamp(CurrentValue + Delta, GetSafeAllocationMinimum(AttributeAllocationMinimum, AttributeAllocationMaximum),
			GetSafeAllocationMaximum(AttributeAllocationMinimum, AttributeAllocationMaximum)));
	SetValidationMessage(FText::GetEmpty(), false);
	RefreshPreview();
}

bool URPGCharacterCreationWizardWidget::CanIncreaseAllocatedAttribute(FName AttributeId) const
{
	return ClassDefinition && ClassDefinition->IsValidDefinition() && GetRemainingAttributePoints() > 0 &&
		GetAttributeValue(GetAllocatedClassAttributesForPreview(), AttributeId) <
		GetSafeAllocationMaximum(AttributeAllocationMinimum, AttributeAllocationMaximum);
}

bool URPGCharacterCreationWizardWidget::CanDecreaseAllocatedAttribute(FName AttributeId) const
{
	return ClassDefinition && ClassDefinition->IsValidDefinition() &&
		GetAttributeValue(GetAllocatedClassAttributesForPreview(), AttributeId) >
		GetSafeAllocationMinimum(AttributeAllocationMinimum, AttributeAllocationMaximum);
}

int32 URPGCharacterCreationWizardWidget::GetAttributePointBudget() const
{
	if (!ClassDefinition || !ClassDefinition->IsValidDefinition())
		return 0;
	return SumAttributePointsAboveMinimum(ClampAllocatedAttributes(ClassDefinition->BaseAttributes, AttributeAllocationMinimum, AttributeAllocationMaximum),
		GetSafeAllocationMinimum(AttributeAllocationMinimum, AttributeAllocationMaximum));
}

int32 URPGCharacterCreationWizardWidget::GetAllocatedAttributePointsSpent() const
{
	return SumAttributePointsAboveMinimum(
		GetAllocatedClassAttributesForPreview(), GetSafeAllocationMinimum(AttributeAllocationMinimum, AttributeAllocationMaximum));
}

int32 URPGCharacterCreationWizardWidget::GetRemainingAttributePoints() const
{
	return FMath::Max(0, GetAttributePointBudget() - GetAllocatedAttributePointsSpent());
}

FRPGAttributes URPGCharacterCreationWizardWidget::GetAllocatedClassAttributesForPreview() const
{
	if (bHasInitializedAllocatedClassAttributes && ClassDefinition && ClassDefinition->IsValidDefinition() && AllocatedClassId == ClassDefinition->ClassId)
	{
		return ClampAllocatedAttributes(AllocatedClassAttributes, AttributeAllocationMinimum, AttributeAllocationMaximum);
	}
	return ClassDefinition && ClassDefinition->IsValidDefinition()
		? ClampAllocatedAttributes(ClassDefinition->BaseAttributes, AttributeAllocationMinimum, AttributeAllocationMaximum)
		: FRPGAttributes();
}

bool URPGCharacterCreationWizardWidget::IsUsingRecommendedAttributes() const
{
	if (!ClassDefinition || !ClassDefinition->IsValidDefinition())
		return true;
	const FRPGAttributes Current = GetAllocatedClassAttributesForPreview();
	const FRPGAttributes Recommended = ClampAllocatedAttributes(ClassDefinition->BaseAttributes, AttributeAllocationMinimum, AttributeAllocationMaximum);
	return Current.Strength == Recommended.Strength && Current.Dexterity == Recommended.Dexterity && Current.Constitution == Recommended.Constitution &&
		Current.Intelligence == Recommended.Intelligence && Current.Wisdom == Recommended.Wisdom && Current.Charisma == Recommended.Charisma;
}

TSoftObjectPtr<UTexture2D> URPGCharacterCreationWizardWidget::ResolveWizardSelectedClassIcon() const
{
	if (!ClassDefinition || !ClassDefinition->IsValidDefinition())
		return TSoftObjectPtr<UTexture2D>();
	for (const URPGClassVisualAsset* ClassVisual : AvailableClassVisuals)
	{
		if (ClassVisual && ClassVisual->IsValidForClass(ClassDefinition->ClassId))
			return ClassVisual->ClassIcon;
	}
	return TSoftObjectPtr<UTexture2D>();
}

TSoftObjectPtr<UTexture2D> URPGCharacterCreationWizardWidget::ResolveWizardSelectedPortrait() const
{
	FRPGCharacterPortraitVariant PortraitVariant;
	return TryResolveWizardSelectedPortraitVariant(PortraitVariant) && !PortraitVariant.Portrait.IsNull() ? PortraitVariant.Portrait : DefaultPortrait;
}

bool URPGCharacterCreationWizardWidget::TryResolveWizardSelectedPortraitVariant(FRPGCharacterPortraitVariant& OutVariant) const
{
	if (!RaceDefinition || !RaceDefinition->IsValidDefinition())
		return false;
	for (const URPGCharacterPortraitSetAsset* PortraitSet : AvailablePortraitSets)
	{
		if (PortraitSet && PortraitSet->IsValidForRace(RaceDefinition->RaceId) &&
			PortraitSet->FindPortraitVariant(SelectedPortraitGender, SelectedPortraitVariantId, OutVariant))
			return true;
	}
	return false;
}

bool URPGCharacterCreationWizardWidget::SubmitCharacterCreation()
{
	if (!InventoryComponent)
	{
		SetValidationMessage(FUILocalizationText::Get(TEXT("CharacterCreation.Validation.InventoryUnavailable")), true);
		return false;
	}
	if (GetRemainingAttributePoints() > 0)
	{
		SetValidationMessage(FUILocalizationText::Get(TEXT("CharacterCreation.Submit.DistributeAllPoints")), true);
		return false;
	}
	const FString NormalizedName = NormalizeCharacterName(EditableText_Name);
	if (NormalizedName.Len() < 1 || NormalizedName.Len() > 24)
	{
		SetValidationMessage(FUILocalizationText::Get(TEXT("CharacterCreation.Submit.NameLength")), true);
		return false;
	}
	if (!RaceDefinition || !RaceDefinition->IsValidDefinition() || !ClassDefinition || !ClassDefinition->IsValidDefinition())
	{
		SetValidationMessage(FUILocalizationText::Get(TEXT("CharacterCreation.Submit.RaceClassRequired")), true);
		return false;
	}

	URPGClassAsset* EffectiveClassDefinition = DuplicateObject<URPGClassAsset>(ClassDefinition, this);
	if (!EffectiveClassDefinition)
	{
		SetValidationMessage(FUILocalizationText::Get(TEXT("CharacterCreation.Submit.ClassPrepareFailed")), true);
		return false;
	}
	EffectiveClassDefinition->BaseAttributes = GetAllocatedClassAttributesForPreview();

	FRPGCharacterCreationRequest Request;
	Request.DisplayName = FText::FromString(NormalizedName);
	Request.RaceDefinition = RaceDefinition;
	Request.ClassDefinition = EffectiveClassDefinition;
	Request.CombatActionSourceClassDefinition = ClassDefinition;
	Request.PortraitGender = SelectedPortraitGender;
	Request.PortraitVariantId = SelectedPortraitVariantId;
	Request.Portrait = DefaultPortrait;
	Request.ClassIcon = ResolveWizardSelectedClassIcon();
	if (CreationContext != ERPGCharacterCreationContext::CustomRecruit)
	{
		const TArray<FRPGStartingEquipmentKit> Kits = GetAvailableStartingEquipmentKits();
		const FName KitId = GetSelectedStartingEquipmentKitId();
		if (!Kits.IsEmpty() && !Kits.ContainsByPredicate([KitId](const FRPGStartingEquipmentKit& Kit) { return Kit.KitId == KitId; }))
		{
			SetValidationMessage(FText::FromString(TEXT("Veuillez choisir un équipement de départ valide.")), true);
			return false;
		}
		Request.StartingEquipmentKitId = KitId;
	}

	const FRPGAttributes CreatedAttributes = GetAllocatedClassAttributesForPreview();

	if (CreationContext == ERPGCharacterCreationContext::CustomRecruit)
	{
		FRPGCustomRecruitResult RecruitResult;
		if (!FRPGCustomRecruitService::TryCreateAndRecruit(InventoryComponent, Request, RecruitResult))
		{
			UE_LOG(LogGrimrockFrontendUI, Warning, TEXT("CharacterCreationWizard CustomRecruit rejected Reason=%d Detail=%s"),
				static_cast<int32>(RecruitResult.RejectReason), *RecruitResult.Error);
			SetValidationMessage(FUILocalizationText::Get(TEXT("CharacterCreation.Validation.RecruitmentFailed")), true);
			RefreshSummaryStep();
			return false;
		}

		UE_LOG(LogGrimrockFrontendUI, Verbose,
			TEXT("CharacterCreationWizard RecruitedCustomCharacter Name=%s Race=%s Class=%s CharacterIndex=%d ClassAttributes=%d/%d/%d/%d/%d/%d"),
			*NormalizedName, RaceDefinition ? *RaceDefinition->RaceId.ToString() : TEXT("None"),
			ClassDefinition ? *ClassDefinition->ClassId.ToString() : TEXT("None"), RecruitResult.CharacterIndex, CreatedAttributes.Strength,
			CreatedAttributes.Dexterity, CreatedAttributes.Constitution, CreatedAttributes.Intelligence, CreatedAttributes.Wisdom, CreatedAttributes.Charisma);

		SetValidationMessage(FText::GetEmpty(), false);
		NotifyCustomRecruitCommitted(RecruitResult.CharacterIndex);
		return true;
	}

	FText Error;
	if (!InventoryComponent->CreateInitialCharacter(Request, Error))
	{
		UE_LOG(LogGrimrockFrontendUI, Warning, TEXT("CharacterCreationWizard InitialCreate rejected Detail=%s"), *Error.ToString());
		SetValidationMessage(FUILocalizationText::Get(TEXT("CharacterCreation.Validation.CreationFailed")), true);
		RefreshSummaryStep();
		return false;
	}

	InventoryComponent->SetCharacterVisualSelection(0, Request.PortraitGender, Request.PortraitVariantId, Request.Portrait, Request.ClassIcon);
	UE_LOG(LogGrimrockFrontendUI, Verbose, TEXT("CharacterCreationWizard CreatedCharacter Name=%s Race=%s Class=%s ClassAttributes=%d/%d/%d/%d/%d/%d"), *NormalizedName,
		RaceDefinition ? *RaceDefinition->RaceId.ToString() : TEXT("None"), ClassDefinition ? *ClassDefinition->ClassId.ToString() : TEXT("None"),
		CreatedAttributes.Strength, CreatedAttributes.Dexterity, CreatedAttributes.Constitution, CreatedAttributes.Intelligence, CreatedAttributes.Wisdom,
		CreatedAttributes.Charisma);

	SetValidationMessage(FText::GetEmpty(), false);
	NotifyInitialCharacterCreationCommitted();
	if (OwningPartyPawn)
	{
		OwningPartyPawn->HandleInitialCharacterCreated();
	}
	return true;
}

void URPGCharacterCreationWizardWidget::HandlePreviousClicked()
{
	GoToPreviousWizardStep();
}
void URPGCharacterCreationWizardWidget::HandleNextClicked()
{
	GoToNextWizardStep();
}
void URPGCharacterCreationWizardWidget::HandleCancelClicked()
{
	CancelWizard();
}
void URPGCharacterCreationWizardWidget::HandleWizardCreateCharacterClicked()
{
	SubmitCharacterCreation();
}

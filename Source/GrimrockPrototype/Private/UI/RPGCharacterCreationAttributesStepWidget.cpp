#include "UI/RPGCharacterCreationAttributesStepWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGRaceAsset.h"
#include "UI/RPGCharacterCreationWizardWidget.h"

namespace
{
    const FName CCAttributeStepStrength (TEXT ("Strength"));
    const FName CCAttributeStepDexterity (TEXT ("Dexterity"));
    const FName CCAttributeStepConstitution (TEXT ("Constitution"));
    const FName CCAttributeStepIntelligence (TEXT ("Intelligence"));
    const FName CCAttributeStepWisdom (TEXT ("Wisdom"));
    const FName CCAttributeStepCharisma (TEXT ("Charisma"));

    int32 CCAttributeStepGetValue (const FRPGAttributes& Attributes, FName AttributeId)
    {
        if (AttributeId == CCAttributeStepStrength)
        {
            return Attributes.Strength;
        }
        if (AttributeId == CCAttributeStepDexterity)
        {
            return Attributes.Dexterity;
        }
        if (AttributeId == CCAttributeStepConstitution)
        {
            return Attributes.Constitution;
        }
        if (AttributeId == CCAttributeStepIntelligence)
        {
            return Attributes.Intelligence;
        }
        if (AttributeId == CCAttributeStepWisdom)
        {
            return Attributes.Wisdom;
        }
        if (AttributeId == CCAttributeStepCharisma)
        {
            return Attributes.Charisma;
        }
        return 0;
    }

    FText CCAttributeStepFormatSignedInteger (int32 Value)
    {
        return FText::FromString (FString::Printf (TEXT ("%+d"), Value));
    }

    FText CCAttributeStepFormatModifier (int32 Value)
    {
        return CCAttributeStepFormatSignedInteger (URPGCharacterRulesLibrary::GetAttributeModifier (Value));
    }

    void CCAttributeStepSetOptionalText (UTextBlock* TextBlock, const FText& Value)
    {
        if (TextBlock)
        {
            TextBlock->SetText (Value);
        }
    }
}

void URPGCharacterCreationAttributesStepWidget::NativeConstruct ()
{
    Super::NativeConstruct ();
    if (!OwningWizard)
    {
        OwningWizard = ResolveOwningWizardFromOuterChain ();
    }
    BindAttributeButtons ();
    RefreshFromWizardState ();
}

void URPGCharacterCreationAttributesStepWidget::InitializeAttributesStep (
    URPGCharacterCreationWizardWidget* InOwningWizard)
{
    OwningWizard = InOwningWizard;
    BindAttributeButtons ();
    RefreshFromWizardState ();
}

void URPGCharacterCreationAttributesStepWidget::RefreshFromWizardState ()
{
    if (!GetResolvedWizard ())
    {
        return;
    }
    RefreshAttributeAllocationPreview ();
    RefreshAttributeAllocationControls ();
}

void URPGCharacterCreationAttributesStepWidget::BindAttributeButtons ()
{
    if (Button_ResetRecommendedAttributes)
    {
        Button_ResetRecommendedAttributes->OnClicked.RemoveDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleResetRecommendedAttributesClicked);
        Button_ResetRecommendedAttributes->OnClicked.AddDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleResetRecommendedAttributesClicked);
    }
    if (Button_StrengthMinus)
    {
        Button_StrengthMinus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleStrengthMinusClicked);
        Button_StrengthMinus->OnClicked.AddDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleStrengthMinusClicked);
    }
    if (Button_StrengthPlus)
    {
        Button_StrengthPlus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleStrengthPlusClicked);
        Button_StrengthPlus->OnClicked.AddDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleStrengthPlusClicked);
    }
    if (Button_DexterityMinus)
    {
        Button_DexterityMinus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleDexterityMinusClicked);
        Button_DexterityMinus->OnClicked.AddDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleDexterityMinusClicked);
    }
    if (Button_DexterityPlus)
    {
        Button_DexterityPlus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleDexterityPlusClicked);
        Button_DexterityPlus->OnClicked.AddDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleDexterityPlusClicked);
    }
    if (Button_ConstitutionMinus)
    {
        Button_ConstitutionMinus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleConstitutionMinusClicked);
        Button_ConstitutionMinus->OnClicked.AddDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleConstitutionMinusClicked);
    }
    if (Button_ConstitutionPlus)
    {
        Button_ConstitutionPlus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleConstitutionPlusClicked);
        Button_ConstitutionPlus->OnClicked.AddDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleConstitutionPlusClicked);
    }
    if (Button_IntelligenceMinus)
    {
        Button_IntelligenceMinus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleIntelligenceMinusClicked);
        Button_IntelligenceMinus->OnClicked.AddDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleIntelligenceMinusClicked);
    }
    if (Button_IntelligencePlus)
    {
        Button_IntelligencePlus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleIntelligencePlusClicked);
        Button_IntelligencePlus->OnClicked.AddDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleIntelligencePlusClicked);
    }
    if (Button_WisdomMinus)
    {
        Button_WisdomMinus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleWisdomMinusClicked);
        Button_WisdomMinus->OnClicked.AddDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleWisdomMinusClicked);
    }
    if (Button_WisdomPlus)
    {
        Button_WisdomPlus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleWisdomPlusClicked);
        Button_WisdomPlus->OnClicked.AddDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleWisdomPlusClicked);
    }
    if (Button_CharismaMinus)
    {
        Button_CharismaMinus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleCharismaMinusClicked);
        Button_CharismaMinus->OnClicked.AddDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleCharismaMinusClicked);
    }
    if (Button_CharismaPlus)
    {
        Button_CharismaPlus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleCharismaPlusClicked);
        Button_CharismaPlus->OnClicked.AddDynamic (this, &URPGCharacterCreationAttributesStepWidget::HandleCharismaPlusClicked);
    }
}

URPGCharacterCreationWizardWidget* URPGCharacterCreationAttributesStepWidget::ResolveOwningWizardFromOuterChain () const
{
    UObject* OuterObject = GetOuter ();
    while (OuterObject)
    {
        if (URPGCharacterCreationWizardWidget* Wizard = Cast<URPGCharacterCreationWizardWidget> (OuterObject))
        {
            return Wizard;
        }
        OuterObject = OuterObject->GetOuter ();
    }
    return nullptr;
}

URPGCharacterCreationWizardWidget* URPGCharacterCreationAttributesStepWidget::GetResolvedWizard ()
{
    if (!OwningWizard)
    {
        OwningWizard = ResolveOwningWizardFromOuterChain ();
    }
    return OwningWizard;
}

void URPGCharacterCreationAttributesStepWidget::RefreshAttributeAllocationPreview ()
{
    URPGCharacterCreationWizardWidget* Wizard = GetResolvedWizard ();
    if (!Wizard)
    {
        return;
    }

    const int32 Budget = Wizard->GetAttributePointBudget ();
    const int32 Remaining = Wizard->GetRemainingAttributePoints ();

    CCAttributeStepSetOptionalText (
        Text_AttributePointsRemaining,
        FText::Format (FText::FromString (TEXT ("Points restants : {0} / {1}")), FText::AsNumber (Remaining), FText::AsNumber (Budget)));
    CCAttributeStepSetOptionalText (
        Text_AttributeHelp,
        FText::FromString (TEXT ("Les valeurs de classe peuvent être ajustées. Les bonus de race sont appliqués ensuite.")));

    FRPGAttributes FinalAttributes;
    const bool bHasAttributes = Wizard->GetPreviewAttributes (FinalAttributes);
    FRPGDerivedStats DerivedStats;
    const bool bHasDerivedStats = Wizard->GetPreviewDerivedStats (DerivedStats);
    const FRPGAttributes ClassAttributes = Wizard->GetAllocatedClassAttributesForPreview ();
    const FRPGAttributes RaceBonuses = Wizard->RaceDefinition ? Wizard->RaceDefinition->AttributeBonuses : FRPGAttributes (0, 0, 0, 0, 0, 0);
    const FText Unavailable = FText::FromString (TEXT ("-"));

    auto RefreshRow = [&] (
        FName AttributeId,
        UTextBlock* ClassText,
        UTextBlock* RaceText,
        UTextBlock* TotalText,
        UTextBlock* ModifierText,
        UTextBlock* EffectsText)
    {
        const int32 ClassValue = CCAttributeStepGetValue (ClassAttributes, AttributeId);
        const int32 RaceValue = CCAttributeStepGetValue (RaceBonuses, AttributeId);
        const int32 TotalValue = CCAttributeStepGetValue (FinalAttributes, AttributeId);
        CCAttributeStepSetOptionalText (ClassText, bHasAttributes ? FText::AsNumber (ClassValue) : Unavailable);
        CCAttributeStepSetOptionalText (RaceText, bHasAttributes ? CCAttributeStepFormatSignedInteger (RaceValue) : Unavailable);
        CCAttributeStepSetOptionalText (TotalText, bHasAttributes ? FText::AsNumber (TotalValue) : Unavailable);
        CCAttributeStepSetOptionalText (ModifierText, bHasAttributes ? CCAttributeStepFormatModifier (TotalValue) : Unavailable);

        if (!EffectsText)
        {
            return;
        }

        if (!bHasAttributes)
        {
            EffectsText->SetText (Unavailable);
            return;
        }

        const int32 Modifier = URPGCharacterRulesLibrary::GetAttributeModifier (TotalValue);
        if (AttributeId == CCAttributeStepStrength)
        {
            EffectsText->SetText (FText::FromString (FString::Printf (
                TEXT ("Charge %d · CàC %+d"),
                FMath::RoundToInt (Wizard->GetPreviewCarryWeight ()),
                Modifier)));
        }
        else if (AttributeId == CCAttributeStepDexterity)
        {
            EffectsText->SetText (FText::FromString (FString::Printf (
                TEXT ("Précision %+d · Esquive %+d"),
                bHasDerivedStats ? DerivedStats.Accuracy : Modifier,
                bHasDerivedStats ? DerivedStats.Evasion : Modifier)));
        }
        else if (AttributeId == CCAttributeStepConstitution)
        {
            EffectsText->SetText (FText::FromString (FString::Printf (
                TEXT ("Santé %d · Résistance %+d"),
                bHasDerivedStats ? DerivedStats.MaxHealth : 0,
                Modifier)));
        }
        else if (AttributeId == CCAttributeStepIntelligence)
        {
            EffectsText->SetText (FText::FromString (FString::Printf (TEXT ("Savoirs %+d · Alchimie %+d"), Modifier, Modifier)));
        }
        else if (AttributeId == CCAttributeStepWisdom)
        {
            EffectsText->SetText (FText::FromString (FString::Printf (TEXT ("Perception %+d · Volonté %+d"), Modifier, Modifier)));
        }
        else if (AttributeId == CCAttributeStepCharisma)
        {
            EffectsText->SetText (FText::FromString (FString::Printf (TEXT ("Influence %+d · Recrutement %+d"), Modifier, Modifier)));
        }
    };

    RefreshRow (CCAttributeStepStrength, Text_StrengthClassValue, Text_StrengthRaceBonus, Text_StrengthTotalValue, Text_StrengthModifier, Text_StrengthEffects);
    RefreshRow (CCAttributeStepDexterity, Text_DexterityClassValue, Text_DexterityRaceBonus, Text_DexterityTotalValue, Text_DexterityModifier, Text_DexterityEffects);
    RefreshRow (CCAttributeStepConstitution, Text_ConstitutionClassValue, Text_ConstitutionRaceBonus, Text_ConstitutionTotalValue, Text_ConstitutionModifier, Text_ConstitutionEffects);
    RefreshRow (CCAttributeStepIntelligence, Text_IntelligenceClassValue, Text_IntelligenceRaceBonus, Text_IntelligenceTotalValue, Text_IntelligenceModifier, Text_IntelligenceEffects);
    RefreshRow (CCAttributeStepWisdom, Text_WisdomClassValue, Text_WisdomRaceBonus, Text_WisdomTotalValue, Text_WisdomModifier, Text_WisdomEffects);
    RefreshRow (CCAttributeStepCharisma, Text_CharismaClassValue, Text_CharismaRaceBonus, Text_CharismaTotalValue, Text_CharismaModifier, Text_CharismaEffects);
}

void URPGCharacterCreationAttributesStepWidget::RefreshAttributeAllocationControls ()
{
    URPGCharacterCreationWizardWidget* Wizard = GetResolvedWizard ();
    if (!Wizard)
    {
        return;
    }

    auto RefreshButtons = [Wizard] (FName AttributeId, UButton* MinusButton, UButton* PlusButton)
    {
        if (MinusButton)
        {
            MinusButton->SetIsEnabled (Wizard->CanDecreaseAllocatedAttribute (AttributeId));
        }
        if (PlusButton)
        {
            PlusButton->SetIsEnabled (Wizard->CanIncreaseAllocatedAttribute (AttributeId));
        }
    };

    RefreshButtons (CCAttributeStepStrength, Button_StrengthMinus, Button_StrengthPlus);
    RefreshButtons (CCAttributeStepDexterity, Button_DexterityMinus, Button_DexterityPlus);
    RefreshButtons (CCAttributeStepConstitution, Button_ConstitutionMinus, Button_ConstitutionPlus);
    RefreshButtons (CCAttributeStepIntelligence, Button_IntelligenceMinus, Button_IntelligencePlus);
    RefreshButtons (CCAttributeStepWisdom, Button_WisdomMinus, Button_WisdomPlus);
    RefreshButtons (CCAttributeStepCharisma, Button_CharismaMinus, Button_CharismaPlus);

    if (Button_ResetRecommendedAttributes)
    {
        Button_ResetRecommendedAttributes->SetIsEnabled (!Wizard->IsUsingRecommendedAttributes ());
    }
}

void URPGCharacterCreationAttributesStepWidget::HandleResetRecommendedAttributesClicked ()
{
    if (URPGCharacterCreationWizardWidget* Wizard = GetResolvedWizard ())
    {
        Wizard->ResetAttributeAllocationToClassDefinition ();
        Wizard->RefreshPreview ();
    }
}

void URPGCharacterCreationAttributesStepWidget::HandleStrengthMinusClicked ()
{
    if (URPGCharacterCreationWizardWidget* Wizard = GetResolvedWizard ())
    {
        Wizard->AdjustAllocatedAttribute (CCAttributeStepStrength, -1);
    }
}

void URPGCharacterCreationAttributesStepWidget::HandleStrengthPlusClicked ()
{
    if (URPGCharacterCreationWizardWidget* Wizard = GetResolvedWizard ())
    {
        Wizard->AdjustAllocatedAttribute (CCAttributeStepStrength, 1);
    }
}

void URPGCharacterCreationAttributesStepWidget::HandleDexterityMinusClicked ()
{
    if (URPGCharacterCreationWizardWidget* Wizard = GetResolvedWizard ())
    {
        Wizard->AdjustAllocatedAttribute (CCAttributeStepDexterity, -1);
    }
}

void URPGCharacterCreationAttributesStepWidget::HandleDexterityPlusClicked ()
{
    if (URPGCharacterCreationWizardWidget* Wizard = GetResolvedWizard ())
    {
        Wizard->AdjustAllocatedAttribute (CCAttributeStepDexterity, 1);
    }
}

void URPGCharacterCreationAttributesStepWidget::HandleConstitutionMinusClicked ()
{
    if (URPGCharacterCreationWizardWidget* Wizard = GetResolvedWizard ())
    {
        Wizard->AdjustAllocatedAttribute (CCAttributeStepConstitution, -1);
    }
}

void URPGCharacterCreationAttributesStepWidget::HandleConstitutionPlusClicked ()
{
    if (URPGCharacterCreationWizardWidget* Wizard = GetResolvedWizard ())
    {
        Wizard->AdjustAllocatedAttribute (CCAttributeStepConstitution, 1);
    }
}

void URPGCharacterCreationAttributesStepWidget::HandleIntelligenceMinusClicked ()
{
    if (URPGCharacterCreationWizardWidget* Wizard = GetResolvedWizard ())
    {
        Wizard->AdjustAllocatedAttribute (CCAttributeStepIntelligence, -1);
    }
}

void URPGCharacterCreationAttributesStepWidget::HandleIntelligencePlusClicked ()
{
    if (URPGCharacterCreationWizardWidget* Wizard = GetResolvedWizard ())
    {
        Wizard->AdjustAllocatedAttribute (CCAttributeStepIntelligence, 1);
    }
}

void URPGCharacterCreationAttributesStepWidget::HandleWisdomMinusClicked ()
{
    if (URPGCharacterCreationWizardWidget* Wizard = GetResolvedWizard ())
    {
        Wizard->AdjustAllocatedAttribute (CCAttributeStepWisdom, -1);
    }
}

void URPGCharacterCreationAttributesStepWidget::HandleWisdomPlusClicked ()
{
    if (URPGCharacterCreationWizardWidget* Wizard = GetResolvedWizard ())
    {
        Wizard->AdjustAllocatedAttribute (CCAttributeStepWisdom, 1);
    }
}

void URPGCharacterCreationAttributesStepWidget::HandleCharismaMinusClicked ()
{
    if (URPGCharacterCreationWizardWidget* Wizard = GetResolvedWizard ())
    {
        Wizard->AdjustAllocatedAttribute (CCAttributeStepCharisma, -1);
    }
}

void URPGCharacterCreationAttributesStepWidget::HandleCharismaPlusClicked ()
{
    if (URPGCharacterCreationWizardWidget* Wizard = GetResolvedWizard ())
    {
        Wizard->AdjustAllocatedAttribute (CCAttributeStepCharisma, 1);
    }
}

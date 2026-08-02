#include "UI/RPGCharacterCreationWidget.h"

#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableText.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "RPG/RPGCharacterPortraitSetAsset.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassVisualAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace
{
    bool bIsSynchronizingPortraitOptions = false;

    struct FPortraitOptionsSyncGuard
    {
        explicit FPortraitOptionsSyncGuard (bool& InFlag)
            : Flag (InFlag)
            , bPreviousValue (InFlag)
        {
            Flag = true;
        }

        ~FPortraitOptionsSyncGuard ()
        {
            Flag = bPreviousValue;
        }

        bool& Flag;
        bool bPreviousValue = false;
    };

    FText GetDefinitionDisplayName (const FText& DisplayName, FName DefinitionId)
    {
        return DisplayName.IsEmpty () ? FText::FromName (DefinitionId) : DisplayName;
    }

    FText GetPortraitVariantDisplayName (const FRPGCharacterPortraitVariant& PortraitVariant)
    {
        return PortraitVariant.DisplayName.IsEmpty ()
            ? FText::FromName (PortraitVariant.VariantId)
            : PortraitVariant.DisplayName;
    }

    FText GetGenderDisplayName (ERPGCharacterPortraitGender Gender)
    {
        return Gender == ERPGCharacterPortraitGender::Female
            ? FText::FromString (TEXT ("Féminin"))
            : FText::FromString (TEXT ("Masculin"));
    }

    bool AreSamePortraitTexture (
        const TSoftObjectPtr<UTexture2D>& Left,
        const TSoftObjectPtr<UTexture2D>& Right)
    {
        return !Left.IsNull () && !Right.IsNull () &&
            Left.ToSoftObjectPath () == Right.ToSoftObjectPath ();
    }

    void SetOptionalText (UTextBlock* TextBlock, const FText& Value)
    {
        if (TextBlock)
        {
            TextBlock->SetText (Value);
        }
    }
}

void URPGCharacterCreationWidget::NativeConstruct ()
{
    Super::NativeConstruct ();

    BindWidgetEvents ();
    PopulateDefinitionOptions ();
    RefreshPreview ();
}

void URPGCharacterCreationWidget::BindWidgetEvents ()
{
    if (Button_CreateCharacter)
    {
        Button_CreateCharacter->OnClicked.RemoveDynamic (
            this,
            &URPGCharacterCreationWidget::HandleCreateCharacterClicked);
        Button_CreateCharacter->OnClicked.AddDynamic (
            this,
            &URPGCharacterCreationWidget::HandleCreateCharacterClicked);
    }

    if (Button_GenderMale)
    {
        Button_GenderMale->OnClicked.RemoveDynamic (
            this,
            &URPGCharacterCreationWidget::HandleGenderMaleClicked);
        Button_GenderMale->OnClicked.AddDynamic (
            this,
            &URPGCharacterCreationWidget::HandleGenderMaleClicked);
    }

    if (Button_GenderFemale)
    {
        Button_GenderFemale->OnClicked.RemoveDynamic (
            this,
            &URPGCharacterCreationWidget::HandleGenderFemaleClicked);
        Button_GenderFemale->OnClicked.AddDynamic (
            this,
            &URPGCharacterCreationWidget::HandleGenderFemaleClicked);
    }

    if (ComboBox_Race)
    {
        ComboBox_Race->OnSelectionChanged.RemoveDynamic (
            this,
            &URPGCharacterCreationWidget::HandleRaceSelectionChanged);
        ComboBox_Race->OnSelectionChanged.AddDynamic (
            this,
            &URPGCharacterCreationWidget::HandleRaceSelectionChanged);
    }

    if (ComboBox_Class)
    {
        ComboBox_Class->OnSelectionChanged.RemoveDynamic (
            this,
            &URPGCharacterCreationWidget::HandleClassSelectionChanged);
        ComboBox_Class->OnSelectionChanged.AddDynamic (
            this,
            &URPGCharacterCreationWidget::HandleClassSelectionChanged);
    }

    if (ComboBox_Gender)
    {
        ComboBox_Gender->OnSelectionChanged.RemoveDynamic (
            this,
            &URPGCharacterCreationWidget::HandleGenderSelectionChanged);
        ComboBox_Gender->OnSelectionChanged.AddDynamic (
            this,
            &URPGCharacterCreationWidget::HandleGenderSelectionChanged);
    }

    if (ComboBox_PortraitVariant)
    {
        ComboBox_PortraitVariant->OnSelectionChanged.RemoveDynamic (
            this,
            &URPGCharacterCreationWidget::HandlePortraitVariantSelectionChanged);
        ComboBox_PortraitVariant->OnSelectionChanged.AddDynamic (
            this,
            &URPGCharacterCreationWidget::HandlePortraitVariantSelectionChanged);
    }

    if (EditableText_Name)
    {
        EditableText_Name->OnTextChanged.RemoveDynamic (
            this,
            &URPGCharacterCreationWidget::HandleNameChanged);
        EditableText_Name->OnTextChanged.AddDynamic (
            this,
            &URPGCharacterCreationWidget::HandleNameChanged);
        EditableText_Name->OnTextCommitted.RemoveDynamic (
            this,
            &URPGCharacterCreationWidget::HandleNameCommitted);
        EditableText_Name->OnTextCommitted.AddDynamic (
            this,
            &URPGCharacterCreationWidget::HandleNameCommitted);
    }
}

void URPGCharacterCreationWidget::PopulateDefinitionOptions ()
{
    if (ComboBox_Race)
    {
        ComboBox_Race->ClearOptions ();

        TSet<FString> AddedOptions;
        auto AddRaceOption = [this, &AddedOptions] (URPGRaceAsset* Race)
        {
            if (!Race || !Race->IsValidDefinition ())
            {
                return;
            }

            const FString Option = GetDefinitionDisplayName (Race->DisplayName, Race->RaceId).ToString ();
            if (!AddedOptions.Contains (Option))
            {
                AddedOptions.Add (Option);
                ComboBox_Race->AddOption (Option);
            }
        };

        AddRaceOption (RaceDefinition);
        for (URPGRaceAsset* Race : AvailableRaceDefinitions)
        {
            AddRaceOption (Race);
        }

        if (!RaceDefinition || !RaceDefinition->IsValidDefinition ())
        {
            for (URPGRaceAsset* Race : AvailableRaceDefinitions)
            {
                if (Race && Race->IsValidDefinition ())
                {
                    RaceDefinition = Race;
                    break;
                }
            }
        }

        if (RaceDefinition && RaceDefinition->IsValidDefinition ())
        {
            ComboBox_Race->SetSelectedOption (
                GetDefinitionDisplayName (RaceDefinition->DisplayName, RaceDefinition->RaceId).ToString ());
        }
    }

    if (ComboBox_Class)
    {
        ComboBox_Class->ClearOptions ();

        TSet<FString> AddedOptions;
        auto AddClassOption = [this, &AddedOptions] (URPGClassAsset* CharacterClass)
        {
            if (!CharacterClass || !CharacterClass->IsValidDefinition ())
            {
                return;
            }

            const FString Option = GetDefinitionDisplayName (
                CharacterClass->DisplayName,
                CharacterClass->ClassId).ToString ();
            if (!AddedOptions.Contains (Option))
            {
                AddedOptions.Add (Option);
                ComboBox_Class->AddOption (Option);
            }
        };

        AddClassOption (ClassDefinition);
        for (URPGClassAsset* CharacterClass : AvailableClassDefinitions)
        {
            AddClassOption (CharacterClass);
        }

        if (!ClassDefinition || !ClassDefinition->IsValidDefinition ())
        {
            for (URPGClassAsset* CharacterClass : AvailableClassDefinitions)
            {
                if (CharacterClass && CharacterClass->IsValidDefinition ())
                {
                    ClassDefinition = CharacterClass;
                    break;
                }
            }
        }

        if (ClassDefinition && ClassDefinition->IsValidDefinition ())
        {
            ComboBox_Class->SetSelectedOption (
                GetDefinitionDisplayName (ClassDefinition->DisplayName, ClassDefinition->ClassId).ToString ());
        }
    }

    PopulatePortraitOptions ();
}

void URPGCharacterCreationWidget::PopulatePortraitOptions ()
{
    FPortraitOptionsSyncGuard GuardSynchronizingPortraitOptions (bIsSynchronizingPortraitOptions);

    if (ComboBox_Gender)
    {
        ComboBox_Gender->ClearOptions ();
        ComboBox_Gender->AddOption (GetGenderDisplayName (ERPGCharacterPortraitGender::Male).ToString ());
        ComboBox_Gender->AddOption (GetGenderDisplayName (ERPGCharacterPortraitGender::Female).ToString ());
        ComboBox_Gender->SetSelectedOption (GetGenderDisplayName (SelectedPortraitGender).ToString ());
    }

    const URPGCharacterPortraitSetAsset* PortraitSet = FindPortraitSetForSelectedRace ();
    if (!PortraitSet)
    {
        if (ComboBox_PortraitVariant)
        {
            ComboBox_PortraitVariant->ClearOptions ();
        }
        SelectedPortraitVariantId = NAME_None;
        DefaultPortrait.Reset ();
        return;
    }

    FRPGCharacterPortraitVariant SelectedVariant;
    if (!TryResolveSelectedPortraitVariant (SelectedVariant))
    {
        SelectFirstValidPortraitForCurrentRaceAndGender ();
    }

    if (ComboBox_PortraitVariant)
    {
        ComboBox_PortraitVariant->ClearOptions ();

        TSet<FString> AddedOptions;
        for (const FRPGCharacterPortraitVariant& PortraitVariant :
            PortraitSet->GetPortraitsForGenderRef (SelectedPortraitGender))
        {
            if (!PortraitVariant.IsValidDefinition ())
            {
                continue;
            }

            const FString Option = GetPortraitVariantDisplayName (PortraitVariant).ToString ();
            if (!AddedOptions.Contains (Option))
            {
                AddedOptions.Add (Option);
                ComboBox_PortraitVariant->AddOption (Option);
            }
        }

        FRPGCharacterPortraitVariant CurrentVariant;
        if (TryResolveSelectedPortraitVariant (CurrentVariant))
        {
            ComboBox_PortraitVariant->SetSelectedOption (
                GetPortraitVariantDisplayName (CurrentVariant).ToString ());
        }
    }
}

void URPGCharacterCreationWidget::InitializeCharacterCreationWidget (AGrimrockPartyPawn* InPartyPawn)
{
    OwningPartyPawn = InPartyPawn;
    InventoryComponent = InPartyPawn ? InPartyPawn->PartyInventoryComponent.Get () : nullptr;
    SetValidationMessage (FText::GetEmpty (), false);
    RefreshPreview ();
}

void URPGCharacterCreationWidget::RefreshPreview ()
{
    if (InventoryComponent && InventoryComponent->HasCompletedInitialCharacterCreation ())
    {
        if (Button_CreateCharacter)
        {
            Button_CreateCharacter->SetIsEnabled (false);
        }
        return;
    }

    const bool bHasRaceDefinition = RaceDefinition && RaceDefinition->IsValidDefinition ();
    const bool bHasClassDefinition = ClassDefinition && ClassDefinition->IsValidDefinition ();
    const FText UnavailableValue = FText::FromString (TEXT ("-"));
    const FText RaceDisplayValue = bHasRaceDefinition
        ? GetDefinitionDisplayName (RaceDefinition->DisplayName, RaceDefinition->RaceId)
        : FText::FromString (TEXT ("Race non configurée"));
    const FText ClassDisplayValue = bHasClassDefinition
        ? GetDefinitionDisplayName (ClassDefinition->DisplayName, ClassDefinition->ClassId)
        : FText::FromString (TEXT ("Classe non configurée"));
    const FText GenderDisplayValue = GetGenderDisplayName (SelectedPortraitGender);

    FText PortraitDescription = ResolveSelectedPortraitDescription ();
    if (PortraitDescription.IsEmpty ())
    {
        PortraitDescription = DefaultPortrait.IsNull ()
            ? FText::FromString (TEXT ("Aucun portrait disponible pour cette race et ce sexe."))
            : FText::FromString (TEXT ("Aucune description disponible pour ce portrait."));
    }

    SetOptionalText (Text_RaceValue, RaceDisplayValue);
    SetOptionalText (Text_ClassValue, ClassDisplayValue);
    SetOptionalText (Text_RaceDescription, bHasRaceDefinition ? RaceDefinition->Description : FText::GetEmpty ());
    SetOptionalText (Text_ClassDescription, bHasClassDefinition ? ClassDefinition->Description : FText::GetEmpty ());
    SetOptionalText (Text_PortraitDescription, PortraitDescription);

    SetOptionalText (Text_IdentityTitle, FText::FromString (TEXT ("Identité du personnage")));
    SetOptionalText (
        Text_IdentityHelp,
        FText::FromString (TEXT ("Donnez un nom à votre personnage et choisissez le portrait qui sera utilisé dans l'interface de jeu.")));
    SetOptionalText (Text_NameLabel, FText::FromString (TEXT ("Nom du personnage")));
    SetOptionalText (Text_IdentitySummaryTitle, FText::FromString (TEXT ("Choix actuels")));
    SetOptionalText (
        Text_IdentityRace,
        FText::Format (FText::FromString (TEXT ("Race : {0}")), RaceDisplayValue));
    SetOptionalText (
        Text_IdentityClass,
        FText::Format (FText::FromString (TEXT ("Classe : {0}")), ClassDisplayValue));
    SetOptionalText (
        Text_IdentityGender,
        FText::Format (FText::FromString (TEXT ("Sexe : {0}")), GenderDisplayValue));
    SetOptionalText (Text_PortraitVariantLabel, FText::FromString (TEXT ("Variante de portrait")));
    SetOptionalText (Text_PortraitDescriptionTitle, FText::FromString (TEXT ("Description du portrait")));
    SetOptionalText (Text_PortraitCaption, FText::FromString (TEXT ("Portrait final")));

    FRPGAttributes Attributes;
    const bool bHasAttributes = GetPreviewAttributes (Attributes);
    SetOptionalText (Text_StrengthValue, bHasAttributes ? FText::AsNumber (Attributes.Strength) : UnavailableValue);
    SetOptionalText (Text_DexterityValue, bHasAttributes ? FText::AsNumber (Attributes.Dexterity) : UnavailableValue);
    SetOptionalText (Text_ConstitutionValue, bHasAttributes ? FText::AsNumber (Attributes.Constitution) : UnavailableValue);
    SetOptionalText (Text_IntelligenceValue, bHasAttributes ? FText::AsNumber (Attributes.Intelligence) : UnavailableValue);
    SetOptionalText (Text_WisdomValue, bHasAttributes ? FText::AsNumber (Attributes.Wisdom) : UnavailableValue);
    SetOptionalText (Text_CharismaValue, bHasAttributes ? FText::AsNumber (Attributes.Charisma) : UnavailableValue);

    FRPGDerivedStats DerivedStats;
    const bool bHasDerivedStats = GetPreviewDerivedStats (DerivedStats);
    SetOptionalText (Text_HealthValue, bHasDerivedStats ? FText::AsNumber (DerivedStats.MaxHealth) : UnavailableValue);
    SetOptionalText (Text_ManaValue, bHasDerivedStats ? FText::AsNumber (DerivedStats.MaxMana) : UnavailableValue);
    SetOptionalText (
        Text_CarryWeightValue,
        bHasAttributes
            ? FText::AsNumber (FMath::RoundToInt (GetPreviewCarryWeight ()))
            : UnavailableValue);

    if (Image_Portrait)
    {
        if (DefaultPortrait.IsNull ())
        {
            Image_Portrait->SetVisibility (ESlateVisibility::Collapsed);
        }
        else
        {
            Image_Portrait->SetBrushFromSoftTexture (DefaultPortrait, false);
            Image_Portrait->SetVisibility (ESlateVisibility::HitTestInvisible);
        }
    }

    RefreshRaceEnvironmentPreview ();
    RefreshRaceIllustrationPreview ();
    RefreshClassIconPreview ();
    RefreshGenderButtonVisualState ();

    if (!bHasRaceDefinition || !bHasClassDefinition)
    {
        SetValidationMessage (
            FText::FromString (
                TEXT ("Ajoutez au moins une race et une classe valides dans Class Defaults du widget.")),
            true);
    }
    else if (!InventoryComponent)
    {
        SetValidationMessage (FText::FromString (TEXT ("Composant d'inventaire indisponible.")), true);
    }

    const bool bCanSubmit = CanSubmitCharacterCreation ();
    if (Button_CreateCharacter)
    {
        Button_CreateCharacter->SetIsEnabled (bCanSubmit);
    }
}

void URPGCharacterCreationWidget::FocusNameInput ()
{
    if (EditableText_Name)
    {
        EditableText_Name->SetKeyboardFocus ();
    }
}

bool URPGCharacterCreationWidget::CanSubmitCharacterCreation () const
{
    if (!InventoryComponent ||
        InventoryComponent->HasCompletedInitialCharacterCreation () ||
        !RaceDefinition ||
        !RaceDefinition->IsValidDefinition () ||
        !ClassDefinition ||
        !ClassDefinition->IsValidDefinition ())
    {
        return false;
    }

    const FString NormalizedName = GetNormalizedNameText ().ToString ();
    FRPGAttributes Attributes;
    return NormalizedName.Len () >= 1 &&
        NormalizedName.Len () <= 24 &&
        GetPreviewAttributes (Attributes) &&
        URPGCharacterRulesLibrary::AreAttributesInRange (Attributes);
}

bool URPGCharacterCreationWidget::GetPreviewAttributes (FRPGAttributes& OutAttributes) const
{
    OutAttributes = FRPGAttributes ();
    if (!RaceDefinition || !RaceDefinition->IsValidDefinition () ||
        !ClassDefinition || !ClassDefinition->IsValidDefinition ())
    {
        return false;
    }

    OutAttributes = URPGCharacterRulesLibrary::AddAttributes (
        ClassDefinition->BaseAttributes,
        RaceDefinition->AttributeBonuses);
    return true;
}

bool URPGCharacterCreationWidget::GetPreviewDerivedStats (FRPGDerivedStats& OutDerivedStats) const
{
    OutDerivedStats = FRPGDerivedStats ();
    FRPGAttributes Attributes;
    if (!GetPreviewAttributes (Attributes))
    {
        return false;
    }

    OutDerivedStats = URPGCharacterRulesLibrary::CalculateDerivedStats (
        Attributes,
        ClassDefinition,
        1);
    return true;
}

float URPGCharacterCreationWidget::GetPreviewCarryWeight () const
{
    FRPGAttributes Attributes;
    return GetPreviewAttributes (Attributes)
        ? URPGCharacterRulesLibrary::CalculateMaxCarryWeight (Attributes)
        : 0.0f;
}

bool URPGCharacterCreationWidget::SubmitCharacterCreation ()
{
    if (!InventoryComponent)
    {
        SetValidationMessage (FText::FromString (TEXT ("Composant d'inventaire indisponible.")), true);
        return false;
    }

    FText Error;
    const FRPGCharacterCreationRequest Request = BuildCreationRequest ();
    if (!InventoryComponent->CreateInitialCharacter (Request, Error))
    {
        SetValidationMessage (
            Error.IsEmpty () ? FText::FromString (TEXT ("Création du personnage impossible.")) : Error,
            true);
        RefreshPreview ();
        return false;
    }

    InventoryComponent->SetCharacterVisualSelection (
        0,
        Request.PortraitGender,
        Request.PortraitVariantId,
        Request.Portrait,
        Request.ClassIcon);

    SetValidationMessage (FText::GetEmpty (), false);
    if (OwningPartyPawn)
    {
        OwningPartyPawn->HandleInitialCharacterCreated ();
    }
    return true;
}

void URPGCharacterCreationWidget::HandleCreateCharacterClicked ()
{
    SubmitCharacterCreation ();
}

void URPGCharacterCreationWidget::HandleGenderMaleClicked ()
{
    SelectPortraitGender (ERPGCharacterPortraitGender::Male);
}

void URPGCharacterCreationWidget::HandleGenderFemaleClicked ()
{
    SelectPortraitGender (ERPGCharacterPortraitGender::Female);
}

void URPGCharacterCreationWidget::HandleNameChanged (const FText& NewText)
{
    (void)NewText;
    if (InventoryComponent && InventoryComponent->HasCompletedInitialCharacterCreation ())
    {
        return;
    }

    SetValidationMessage (FText::GetEmpty (), false);
    RefreshPreview ();
}

void URPGCharacterCreationWidget::HandleNameCommitted (
    const FText& NewText,
    ETextCommit::Type CommitMethod)
{
    (void)NewText;
    if (InventoryComponent && InventoryComponent->HasCompletedInitialCharacterCreation ())
    {
        return;
    }

    RefreshPreview ();
    if (CommitMethod == ETextCommit::OnEnter && CanSubmitCharacterCreation ())
    {
        SubmitCharacterCreation ();
    }
}

void URPGCharacterCreationWidget::HandleRaceSelectionChanged (
    FString SelectedItem,
    ESelectInfo::Type SelectionType)
{
    (void)SelectionType;

    for (URPGRaceAsset* Race : AvailableRaceDefinitions)
    {
        if (Race &&
            Race->IsValidDefinition () &&
            GetDefinitionDisplayName (Race->DisplayName, Race->RaceId).ToString () == SelectedItem)
        {
            RaceDefinition = Race;
            PopulatePortraitOptions ();
            SetValidationMessage (FText::GetEmpty (), false);
            RefreshPreview ();
            return;
        }
    }
}

void URPGCharacterCreationWidget::HandleClassSelectionChanged (
    FString SelectedItem,
    ESelectInfo::Type SelectionType)
{
    (void)SelectionType;

    for (URPGClassAsset* CharacterClass : AvailableClassDefinitions)
    {
        if (CharacterClass &&
            CharacterClass->IsValidDefinition () &&
            GetDefinitionDisplayName (
                CharacterClass->DisplayName,
                CharacterClass->ClassId).ToString () == SelectedItem)
        {
            ClassDefinition = CharacterClass;
            SetValidationMessage (FText::GetEmpty (), false);
            RefreshPreview ();
            return;
        }
    }
}

void URPGCharacterCreationWidget::HandleGenderSelectionChanged (
    FString SelectedItem,
    ESelectInfo::Type SelectionType)
{
    (void)SelectionType;

    if (bIsSynchronizingPortraitOptions)
    {
        return;
    }

    SelectPortraitGender (
        SelectedItem == GetGenderDisplayName (ERPGCharacterPortraitGender::Female).ToString ()
            ? ERPGCharacterPortraitGender::Female
            : ERPGCharacterPortraitGender::Male);
}

void URPGCharacterCreationWidget::HandlePortraitVariantSelectionChanged (
    FString SelectedItem,
    ESelectInfo::Type SelectionType)
{
    (void)SelectionType;

    if (bIsSynchronizingPortraitOptions)
    {
        return;
    }

    const URPGCharacterPortraitSetAsset* PortraitSet = FindPortraitSetForSelectedRace ();
    if (!PortraitSet)
    {
        return;
    }

    for (const FRPGCharacterPortraitVariant& PortraitVariant :
        PortraitSet->GetPortraitsForGenderRef (SelectedPortraitGender))
    {
        if (PortraitVariant.IsValidDefinition () &&
            GetPortraitVariantDisplayName (PortraitVariant).ToString () == SelectedItem)
        {
            SelectPortraitVariant (PortraitVariant);
            SetValidationMessage (FText::GetEmpty (), false);
            RefreshPreview ();
            return;
        }
    }
}

FRPGCharacterCreationRequest URPGCharacterCreationWidget::BuildCreationRequest () const
{
    FRPGCharacterCreationRequest Request;
    Request.DisplayName = GetNormalizedNameText ();
    Request.RaceDefinition = RaceDefinition;
    Request.ClassDefinition = ClassDefinition;
    Request.CombatActionSourceClassDefinition = ClassDefinition;
    Request.PortraitGender = SelectedPortraitGender;
    Request.PortraitVariantId = SelectedPortraitVariantId;
    Request.Portrait = DefaultPortrait;
    Request.ClassIcon = ResolveSelectedClassIcon ();
    return Request;
}

FText URPGCharacterCreationWidget::GetNormalizedNameText () const
{
    FString Name = EditableText_Name ? EditableText_Name->GetText ().ToString () : FString ();
    Name.TrimStartAndEndInline ();
    return FText::FromString (Name);
}

FText URPGCharacterCreationWidget::ResolveSelectedPortraitDescription () const
{
    FRPGCharacterPortraitVariant PortraitVariant;
    if (TryResolveSelectedPortraitVariant (PortraitVariant))
    {
        return PortraitVariant.Description;
    }

    return FText::GetEmpty ();
}

const URPGCharacterPortraitSetAsset* URPGCharacterCreationWidget::FindPortraitSetForSelectedRace () const
{
    if (!RaceDefinition || !RaceDefinition->IsValidDefinition ())
    {
        return nullptr;
    }

    for (const URPGCharacterPortraitSetAsset* PortraitSet : AvailablePortraitSets)
    {
        if (PortraitSet && PortraitSet->IsValidForRace (RaceDefinition->RaceId))
        {
            return PortraitSet;
        }
    }

    return nullptr;
}

const URPGClassVisualAsset* URPGCharacterCreationWidget::FindClassVisualForSelectedClass () const
{
    if (!ClassDefinition || !ClassDefinition->IsValidDefinition ())
    {
        return nullptr;
    }

    for (const URPGClassVisualAsset* ClassVisual : AvailableClassVisuals)
    {
        if (ClassVisual && ClassVisual->IsValidForClass (ClassDefinition->ClassId))
        {
            return ClassVisual;
        }
    }

    return nullptr;
}

const FRPGRaceIllustrationOption* URPGCharacterCreationWidget::FindRaceIllustrationForSelectedRaceAndGender () const
{
    if (!RaceDefinition || !RaceDefinition->IsValidDefinition ())
    {
        return nullptr;
    }

    for (const FRPGRaceIllustrationOption& RaceIllustration : AvailableRaceIllustrations)
    {
        if (RaceIllustration.Matches (RaceDefinition->RaceId, SelectedPortraitGender))
        {
            return &RaceIllustration;
        }
    }

    return nullptr;
}

bool URPGCharacterCreationWidget::TryResolveSelectedPortraitVariant (
    FRPGCharacterPortraitVariant& OutVariant) const
{
    OutVariant = FRPGCharacterPortraitVariant ();
    const URPGCharacterPortraitSetAsset* PortraitSet = FindPortraitSetForSelectedRace ();
    if (!PortraitSet)
    {
        return false;
    }

    if (!SelectedPortraitVariantId.IsNone () &&
        PortraitSet->FindPortraitVariant (SelectedPortraitGender, SelectedPortraitVariantId, OutVariant))
    {
        return true;
    }

    for (const FRPGCharacterPortraitVariant& PortraitVariant :
        PortraitSet->GetPortraitsForGenderRef (SelectedPortraitGender))
    {
        if (PortraitVariant.IsValidDefinition () &&
            AreSamePortraitTexture (DefaultPortrait, PortraitVariant.Portrait))
        {
            OutVariant = PortraitVariant;
            return true;
        }
    }

    return false;
}

void URPGCharacterCreationWidget::SelectPortraitVariant (
    const FRPGCharacterPortraitVariant& PortraitVariant)
{
    if (!PortraitVariant.IsValidDefinition ())
    {
        return;
    }

    SelectedPortraitVariantId = PortraitVariant.VariantId;
    DefaultPortrait = PortraitVariant.Portrait;
}

void URPGCharacterCreationWidget::SelectFirstValidPortraitForCurrentRaceAndGender ()
{
    const URPGCharacterPortraitSetAsset* PortraitSet = FindPortraitSetForSelectedRace ();
    if (!PortraitSet)
    {
        return;
    }

    FRPGCharacterPortraitVariant FirstValidVariant;
    if (PortraitSet->GetFirstValidPortrait (SelectedPortraitGender, FirstValidVariant))
    {
        SelectPortraitVariant (FirstValidVariant);
        return;
    }

    SelectedPortraitVariantId = NAME_None;
    DefaultPortrait.Reset ();
}

void URPGCharacterCreationWidget::SelectPortraitGender (ERPGCharacterPortraitGender NewGender)
{
    if (SelectedPortraitGender != NewGender)
    {
        SelectedPortraitGender = NewGender;
        SelectedPortraitVariantId = NAME_None;
        DefaultPortrait.Reset ();
        PopulatePortraitOptions ();
        SetValidationMessage (FText::GetEmpty (), false);
    }

    RefreshPreview ();
}

TSoftObjectPtr<UTexture2D> URPGCharacterCreationWidget::ResolveSelectedClassIcon () const
{
    const URPGClassVisualAsset* ClassVisual = FindClassVisualForSelectedClass ();
    return ClassVisual ? ClassVisual->ClassIcon : TSoftObjectPtr<UTexture2D> ();
}

TSoftObjectPtr<UTexture2D> URPGCharacterCreationWidget::ResolveSelectedRaceIllustration () const
{
    const FRPGRaceIllustrationOption* RaceIllustration = FindRaceIllustrationForSelectedRaceAndGender ();
    return RaceIllustration ? RaceIllustration->Illustration : TSoftObjectPtr<UTexture2D> ();
}

TSoftObjectPtr<UTexture2D> URPGCharacterCreationWidget::ResolveSelectedRaceEnvironment () const
{
    const FRPGRaceIllustrationOption* RaceIllustration = FindRaceIllustrationForSelectedRaceAndGender ();
    return RaceIllustration ? RaceIllustration->RaceEnvironment : TSoftObjectPtr<UTexture2D> ();
}

void URPGCharacterCreationWidget::RefreshClassIconPreview ()
{
    if (!Image_ClassIcon)
    {
        return;
    }

    const TSoftObjectPtr<UTexture2D> ClassIcon = ResolveSelectedClassIcon ();
    if (ClassIcon.IsNull ())
    {
        Image_ClassIcon->SetVisibility (ESlateVisibility::Collapsed);
        return;
    }

    Image_ClassIcon->SetBrushFromSoftTexture (ClassIcon, false);
    Image_ClassIcon->SetVisibility (ESlateVisibility::HitTestInvisible);
}

void URPGCharacterCreationWidget::RefreshRaceIllustrationPreview ()
{
    if (!Image_RaceIllustration)
    {
        return;
    }

    const TSoftObjectPtr<UTexture2D> RaceIllustration = ResolveSelectedRaceIllustration ();
    if (RaceIllustration.IsNull ())
    {
        Image_RaceIllustration->SetVisibility (ESlateVisibility::Collapsed);
        return;
    }

    Image_RaceIllustration->SetBrushFromSoftTexture (RaceIllustration, false);
    Image_RaceIllustration->SetVisibility (ESlateVisibility::HitTestInvisible);
}

void URPGCharacterCreationWidget::RefreshRaceEnvironmentPreview ()
{
    if (!Image_RaceEnvironment)
    {
        return;
    }
    const TSoftObjectPtr<UTexture2D> RaceEnvironment = ResolveSelectedRaceEnvironment ();
    if (RaceEnvironment.IsNull ())
    {
        Image_RaceEnvironment->SetVisibility (ESlateVisibility::Collapsed);
        return;
    }
    Image_RaceEnvironment->SetBrushFromSoftTexture (RaceEnvironment, false);
    Image_RaceEnvironment->SetVisibility (ESlateVisibility::HitTestInvisible);
}

void URPGCharacterCreationWidget::RefreshGenderButtonVisualState ()
{
    const FLinearColor SelectedButtonColor (0.55f, 0.42f, 0.18f, 1.0f);
    const FLinearColor UnselectedButtonColor (0.18f, 0.18f, 0.18f, 1.0f);

    if (Button_GenderMale)
    {
        Button_GenderMale->SetBackgroundColor (
            SelectedPortraitGender == ERPGCharacterPortraitGender::Male
                ? SelectedButtonColor
                : UnselectedButtonColor);
    }

    if (Button_GenderFemale)
    {
        Button_GenderFemale->SetBackgroundColor (
            SelectedPortraitGender == ERPGCharacterPortraitGender::Female
                ? SelectedButtonColor
                : UnselectedButtonColor);
    }

    if (Image_GenderMale && !GenderMaleButtonIcon.IsNull ())
    {
        Image_GenderMale->SetBrushFromSoftTexture (GenderMaleButtonIcon, false);
        Image_GenderMale->SetVisibility (ESlateVisibility::HitTestInvisible);
    }

    if (Image_GenderFemale && !GenderFemaleButtonIcon.IsNull ())
    {
        Image_GenderFemale->SetBrushFromSoftTexture (GenderFemaleButtonIcon, false);
        Image_GenderFemale->SetVisibility (ESlateVisibility::HitTestInvisible);
    }
}

void URPGCharacterCreationWidget::SetValidationMessage (const FText& Message, bool bIsError)
{
    if (!Text_ValidationMessage)
    {
        return;
    }

    Text_ValidationMessage->SetText (Message);
    Text_ValidationMessage->SetColorAndOpacity (
        bIsError ? FSlateColor (FLinearColor (0.85f, 0.12f, 0.08f)) : FSlateColor (FLinearColor::White));
    Text_ValidationMessage->SetVisibility (
        Message.IsEmpty () ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

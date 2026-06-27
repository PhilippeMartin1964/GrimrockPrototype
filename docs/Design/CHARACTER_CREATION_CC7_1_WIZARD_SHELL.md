# CC7.1 - Shell du vrai wizard de création de personnage

## 1. Objet

CC7.1 introduit le parent C++ du futur wizard de création de personnage.

Objectif : remplacer progressivement l'écran monobloc `WBP_CharacterCreation` par un flux lisible en cinq étapes, sans réimplémenter les règles RPG déjà validées.

```text
Race
-> Classe
-> Caractéristiques
-> Identité
-> Résumé
-> Création du personnage
```

Le shell ne change pas encore les règles de création. Il encadre l'existant.

---

## 2. Ajouts C++

Fichiers ajoutés :

```text
Source/GrimrockPrototype/Public/UI/RPGCharacterCreationWizardWidget.h
Source/GrimrockPrototype/Private/UI/RPGCharacterCreationWizardWidget.cpp
```

Classe ajoutée :

```text
RPGCharacterCreationWizardWidget
```

Parent C++ :

```text
RPGCharacterCreationWidget
```

Raison : le wizard réutilise directement :

```text
InitializeCharacterCreationWidget
RefreshPreview
CanSubmitCharacterCreation
SubmitCharacterCreation
FocusNameInput
```

Ainsi, le `AGrimrockPartyPawn::CharacterCreationWidgetClass` peut pointer vers un Blueprint enfant du nouveau wizard sans changer le type C++ existant.

---

## 3. Étapes disponibles

Enum Blueprint :

```text
ERPGCharacterCreationWizardStep
```

Valeurs techniques :

```text
Race
Class
Attributes
Identity
Summary
```

Affichage conseillé :

| Étape | Rôle |
|---|---|
| Race | Choisir la race et afficher sa description |
| Classe | Choisir la classe et afficher son rôle |
| Caractéristiques | Voir les 6 caractéristiques, PV, mana et charge |
| Identité | Saisir le nom, le genre et le portrait |
| Résumé | Vérifier le récapitulatif avant validation |

---

## 4. Fonctions Blueprint disponibles

Navigation :

```text
SetCurrentWizardStep
GoToNextWizardStep
GoToPreviousWizardStep
CancelWizard
```

Lecture d'état :

```text
CanGoToNextWizardStep
CanGoToPreviousWizardStep
IsWizardOnLastStep
GetCurrentWizardStepIndex
GetCurrentWizardStepNumber
GetWizardStepCount
GetCurrentWizardStepTitle
```

Événement Blueprint optionnel :

```text
OnWizardStepChanged
```

Le bouton final continue d'utiliser la logique existante :

```text
Button_CreateCharacter
-> SubmitCharacterCreation
```

Ce bind est déjà fait par le parent `RPGCharacterCreationWidget`.

---

## 5. Créer le Blueprint WBP_CharacterCreationWizard

Créer :

```text
Content/GrimrockPrototype/Blueprints/UI/CharacterCreation/WBP_CharacterCreationWizard
```

Parent Class :

```text
RPGCharacterCreationWizardWidget
```

Dans `BP_GrimrockPartyPawn`, ou dans le pawn runtime utilisé :

```text
Character Creation Widget Class = WBP_CharacterCreationWizard
```

L'ancien `WBP_CharacterCreation` peut rester disponible comme secours tant que CC7 n'est pas entièrement validé.

---

## 6. Hiérarchie UMG recommandée

Structure cible :

```text
WBP_CharacterCreationWizard
-> CanvasPanel_Root
   -> Border_ModalDim
      -> SizeBox_Wizard
         -> Border_WizardBackground
            -> VerticalBox_Wizard
               -> HorizontalBox_Header
                  -> Text_StepTitle
                  -> Spacer_HeaderFill
                  -> Text_StepCounter
               -> WidgetSwitcher_Steps
                  -> Panel_StepRace
                  -> Panel_StepClass
                  -> Panel_StepAttributes
                  -> Panel_StepIdentity
                  -> Panel_StepSummary
               -> Text_ValidationMessage
               -> HorizontalBox_Footer
                  -> Button_Cancel
                  -> Spacer_FooterFill
                  -> Button_Previous
                  -> Button_Next
                  -> Button_CreateCharacter
```

Noms lus par le C++ du shell :

```text
WidgetSwitcher_Steps
Panel_StepRace
Panel_StepClass
Panel_StepAttributes
Panel_StepIdentity
Panel_StepSummary
Text_StepTitle
Text_StepCounter
Button_Previous
Button_Next
Button_Cancel
Button_CreateCharacter
```

Noms hérités du parent existant et encore utiles :

```text
EditableText_Name
ComboBox_Race
ComboBox_Class
ComboBox_Gender
ComboBox_PortraitVariant
ComboBox_Portrait
Image_Portrait
Image_ClassIcon
Text_RaceValue
Text_ClassValue
Text_RaceDescription
Text_ClassDescription
Text_PortraitDescription
Text_StrengthValue
Text_DexterityValue
Text_ConstitutionValue
Text_IntelligenceValue
Text_WisdomValue
Text_CharismaValue
Text_HealthValue
Text_ManaValue
Text_CarryWeightValue
Text_ValidationMessage
```

`EditableText_Name` et `Button_CreateCharacter` restent importants parce qu'ils sont utilisés par la logique de création déjà existante.

---

## 7. Répartition conseillée des widgets existants

### Panel_StepRace

Widgets à placer :

```text
ComboBox_Race
Text_RaceValue
Text_RaceDescription
Image_Portrait, optionnel si vous voulez déjà voir le portrait race/genre
```

### Panel_StepClass

Widgets à placer :

```text
ComboBox_Class
Image_ClassIcon
Text_ClassValue
Text_ClassDescription
```

### Panel_StepAttributes

Widgets à placer :

```text
Text_StrengthValue
Text_DexterityValue
Text_ConstitutionValue
Text_IntelligenceValue
Text_WisdomValue
Text_CharismaValue
Text_HealthValue
Text_ManaValue
Text_CarryWeightValue
```

### Panel_StepIdentity

Widgets à placer :

```text
EditableText_Name
ComboBox_Gender
ComboBox_PortraitVariant
ComboBox_Portrait
Image_Portrait
Text_PortraitDescription
```

### Panel_StepSummary

Widgets à placer :

```text
Text_RaceValue
Text_ClassValue
Text_StrengthValue
Text_DexterityValue
Text_ConstitutionValue
Text_IntelligenceValue
Text_WisdomValue
Text_CharismaValue
Text_HealthValue
Text_ManaValue
Text_CarryWeightValue
Image_Portrait
Image_ClassIcon
```

Attention : dans UMG, un même widget ne peut pas être physiquement présent dans deux panels à la fois. Pour le résumé, utilisez soit des TextBlocks dédiés remplis plus tard en CC7.2, soit un résumé très simple pour CC7.1.

---

## 8. Réglages Class Defaults du wizard

Dans `WBP_CharacterCreationWizard` :

```text
Initial Wizard Step = Race
Allow Cancel = false
Focus Name Input On Identity Step = true
```

Pour un lancement via `Nouvelle partie`, `Allow Cancel` doit rester `false`. Le joueur ne doit pas fermer la création et se promener sans personnage valide.

---

## 9. Comportement attendu

Au lancement d'une nouvelle partie :

```text
1. Le wizard s'ouvre sur Race.
2. Suivant avance vers Classe.
3. Précédent revient à l'étape précédente.
4. Le compteur affiche 1 / 5, 2 / 5, etc.
5. Button_CreateCharacter est visible seulement sur Résumé.
6. Button_Next est masqué sur Résumé.
7. Button_Cancel est masqué si Allow Cancel = false.
8. La validation finale crée le personnage avec le code déjà existant.
```

Le shell ne doit pas encore gérer :

```text
- budget de points ;
- validation bloquante par étape ;
- résumé dédié complet ;
- choix multiples de portraits en grille ;
- équipement de départ.
```

Ces points appartiennent à CC7.2+.

---

## 10. Logs utiles

Filtre Output Log :

```text
CharacterCreationWizard
RPGCharacterCreation
PartySave
```

Logs attendus en navigation :

```text
CharacterCreationWizard Refreshed Widget=... Step=0 StepName=Race
CharacterCreationWizard Refreshed Widget=... Step=1 StepName=Classe
CharacterCreationWizard Refreshed Widget=... Step=4 StepName=Résumé
```

Le log est en `Verbose`. Si vous ne le voyez pas, c'est normal selon le niveau de log actif.

---

## 11. Critère de validation CC7.1

CC7.1 est validé lorsque :

```text
- le projet recompile ;
- WBP_CharacterCreationWizard peut être créé avec le parent RPGCharacterCreationWizardWidget ;
- CharacterCreationWidgetClass peut pointer vers WBP_CharacterCreationWizard ;
- une nouvelle partie affiche le wizard ;
- Précédent/Suivant naviguent entre les cinq étapes ;
- le bouton final apparaît uniquement au résumé ;
- valider crée encore le personnage et ferme le modal comme avant.
```

Statut attendu après validation UE5 :

```text
CC7.1 validé - shell wizard fonctionnel, logique RPG inchangée.
```

# CC7.1 - Shell du vrai wizard de création de personnage

## 1. Objet

CC7.1 introduit le parent C++ du wizard de création de personnage.

Objectif : remplacer l'ancien écran monobloc `WBP_CharacterCreation` par un flux lisible en cinq étapes, sans réimplémenter les règles RPG déjà validées.

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

## 2. Source des portraits

Le wizard utilise désormais uniquement les sets de portraits :

```text
AvailablePortraitSets
```

Ces assets doivent pointer vers les DataAssets placés dans :

```text
Content/GrimrockPrototype/Core/DataAssets/RPG/Visuals/PortraitSets
```

La propriété legacy `AvailablePortraits` a été supprimée du widget. Elle faisait double emploi avec les sets de portraits et ne doit plus être renseignée dans `WBP_CharacterCreationWizard`.

Le choix de portrait se fait donc ainsi :

```text
RaceDefinition.RaceId
-> recherche du PortraitSet correspondant dans AvailablePortraitSets
-> filtrage par genre
-> choix de ComboBox_PortraitVariant
-> affichage dans Image_Portrait
```

---

## 3. Ajouts C++

Fichiers ajoutés pour CC7.1 :

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
FocusNameInput
```

Le wizard surcharge maintenant :

```text
SubmitCharacterCreation
```

Cette surcharge garantit que la création finale passe toujours par les attributs répartis dans le wizard, même si l'appel se fait via le type parent `RPGCharacterCreationWidget`.

Ainsi, le `AGrimrockPartyPawn::CharacterCreationWidgetClass` peut pointer vers un Blueprint enfant du nouveau wizard sans changer le type C++ existant.

---

## 4. Étapes disponibles

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
| Identité | Saisir le nom, le genre et choisir le portrait |
| Résumé | Vérifier le récapitulatif avant validation |

---

## 5. Fonctions Blueprint disponibles

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

Le bouton final utilise le chemin wizard :

```text
Button_CreateCharacter
-> HandleWizardCreateCharacterClicked
-> SubmitCharacterCreation override
```

Le parent `RPGCharacterCreationWidget` garde le bind générique, mais `RPGCharacterCreationWizardWidget` rebind le bouton final afin de forcer la validation complète du wizard.

---

## 6. Créer le Blueprint WBP_CharacterCreationWizard

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

L'ancien `WBP_CharacterCreation` a été supprimé et ne doit plus être référencé. Le seul chemin valide est désormais `WBP_CharacterCreationWizard`.

---

## 7. Structure UMG complète

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
               -> Border_StepFrame
                  -> WidgetSwitcher_Steps
                     -> Panel_StepRace
                     -> Panel_StepClass
                     -> Panel_StepAttributes
                     -> Panel_StepIdentity
                     -> Panel_StepSummary
               -> Text_ValidationMessage
               -> HorizontalBox_Footer
                  -> Button_Cancel
                     -> Text_Cancel
                  -> Spacer_FooterFill
                  -> Button_Previous
                     -> Text_Previous
                  -> Button_Next
                     -> Text_Next
                  -> Button_CreateCharacter
                     -> Text_CreateCharacter
```

Les cinq widgets suivants doivent être des enfants directs de `WidgetSwitcher_Steps` :

```text
Panel_StepRace
Panel_StepClass
Panel_StepAttributes
Panel_StepIdentity
Panel_StepSummary
```

Type recommandé pour ces cinq panneaux :

```text
Border ou Overlay
```

Éviter `Canvas Panel` pour ces cinq panneaux, sauf si vous avez vraiment besoin d'un placement libre. Un `Border` avec un `VerticalBox`, ou un `Overlay` interne, est plus propre et plus facile à redimensionner.

---

## 8. Précision importante sur les réglages UE5

Dans UE5, certains réglages ne sont pas des propriétés du widget lui-même, mais du **slot du parent**.

Exemples :

```text
- Un widget placé dans un VerticalBox possède un Vertical Box Slot.
- Un widget placé dans un HorizontalBox possède un Horizontal Box Slot.
- Un widget placé dans un CanvasPanel possède un Canvas Panel Slot.
```

Donc, dans ce document :

```text
Slot du parent : Size = Fill
```

signifie : régler le **slot dans le panneau parent**, pas chercher une propriété `Fill` directement sur le widget.

À ne pas confondre :

```text
Image_Portrait n'a pas de propriété Fill.
HorizontalBox_RaceLayout n'a pas de propriété Fill.
```
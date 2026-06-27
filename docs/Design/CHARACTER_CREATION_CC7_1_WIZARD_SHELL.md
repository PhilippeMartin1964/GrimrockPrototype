# CC7.1 - Shell du vrai wizard de creation de personnage

## 1. Objet

CC7.1 introduit le parent C++ du futur wizard de creation de personnage.

Objectif : remplacer progressivement l'ecran monobloc `WBP_CharacterCreation` par un flux lisible en cinq etapes, sans reimplementer les regles RPG deja validees.

```text
Race
-> Classe
-> Caracteristiques
-> Identite
-> Resume
-> Creation du personnage
```

Le shell ne change pas encore les regles de creation. Il encadre l'existant.

---

## 2. Ajouts C++

Fichiers ajoutes :

```text
Source/GrimrockPrototype/Public/UI/RPGCharacterCreationWizardWidget.h
Source/GrimrockPrototype/Private/UI/RPGCharacterCreationWizardWidget.cpp
```

Classe ajoutee :

```text
RPGCharacterCreationWizardWidget
```

Parent C++ :

```text
RPGCharacterCreationWidget
```

Raison : le wizard reutilise directement :

```text
InitializeCharacterCreationWidget
RefreshPreview
CanSubmitCharacterCreation
SubmitCharacterCreation
FocusNameInput
```

Ainsi, le `AGrimrockPartyPawn::CharacterCreationWidgetClass` peut pointer vers un Blueprint enfant du nouveau wizard sans changer le type C++ existant.

---

## 3. Etapes disponibles

Enum Blueprint :

```text
ERPGCharacterCreationWizardStep
```

Valeurs :

```text
Race
Class
Attributes
Identity
Summary
```

Affichage conseille :

| Etape | Role |
|---|---|
| Race | Choisir la race et afficher sa description |
| Class | Choisir la classe et afficher son role |
| Attributes | Voir les 6 caracteristiques, PV, mana, charge |
| Identity | Saisir le nom, genre et portrait |
| Summary | Verifier le recapitulatif avant validation |

---

## 4. Fonctions Blueprint disponibles

Navigation :

```text
SetCurrentWizardStep
GoToNextWizardStep
GoToPreviousWizardStep
CancelWizard
```

Lecture d'etat :

```text
CanGoToNextWizardStep
CanGoToPreviousWizardStep
IsWizardOnLastStep
GetCurrentWizardStepIndex
GetCurrentWizardStepNumber
GetWizardStepCount
GetCurrentWizardStepTitle
```

Evenement Blueprint optionnel :

```text
OnWizardStepChanged
```

Le bouton final continue d'utiliser la logique existante :

```text
Button_CreateCharacter
-> SubmitCharacterCreation
```

Ce bind est deja fait par le parent `RPGCharacterCreationWidget`.

---

## 5. Creer le Blueprint WBP_CharacterCreationWizard

Creer :

```text
Content/GrimrockPrototype/Blueprints/UI/CharacterCreation/WBP_CharacterCreationWizard
```

Parent Class :

```text
RPGCharacterCreationWizardWidget
```

Dans `BP_GrimrockPartyPawn` ou le pawn runtime utilise :

```text
Character Creation Widget Class = WBP_CharacterCreationWizard
```

L'ancien `WBP_CharacterCreation` peut rester disponible comme secours tant que CC7 n'est pas entierement valide.

---

## 6. Hierarchie UMG recommandee

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

Noms herites du parent existant et encore utiles :

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

`EditableText_Name` et `Button_CreateCharacter` restent importants parce qu'ils sont utilises par la logique de creation deja existante.

---

## 7. Repartition conseillee des widgets existants

### Panel_StepRace

Widgets a placer :

```text
ComboBox_Race
Text_RaceValue
Text_RaceDescription
Image_Portrait, optionnel si vous voulez deja voir le portrait race/genre
```

### Panel_StepClass

Widgets a placer :

```text
ComboBox_Class
Image_ClassIcon
Text_ClassValue
Text_ClassDescription
```

### Panel_StepAttributes

Widgets a placer :

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

Widgets a placer :

```text
EditableText_Name
ComboBox_Gender
ComboBox_PortraitVariant
ComboBox_Portrait
Image_Portrait
Text_PortraitDescription
```

### Panel_StepSummary

Widgets a placer :

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

Attention : dans UMG, un meme widget ne peut pas etre physiquement present dans deux panels a la fois. Pour le resume, utilisez soit des TextBlocks dedies remplis plus tard en CC7.2, soit gardez le resume tres simple pour CC7.1.

---

## 8. Reglages Class Defaults du wizard

Dans `WBP_CharacterCreationWizard` :

```text
Initial Wizard Step = Race
Allow Cancel = false
Focus Name Input On Identity Step = true
```

Pour un lancement via `Nouvelle partie`, `Allow Cancel` doit rester `false`. Le joueur ne doit pas fermer la creation et se promener sans personnage valide.

---

## 9. Comportement attendu

Au lancement d'une nouvelle partie :

```text
1. Le wizard s'ouvre sur Race.
2. Suivant avance vers Classe.
3. Precedent revient a l'etape precedente.
4. Le compteur affiche 1 / 5, 2 / 5, etc.
5. Button_CreateCharacter est visible seulement sur Resume.
6. Button_Next est masque sur Resume.
7. Button_Cancel est masque si Allow Cancel = false.
8. La validation finale cree le personnage avec le code deja existant.
```

Le shell ne doit pas encore gerer :

```text
- budget de points ;
- validation bloquante par etape ;
- resume dedie complet ;
- choix multiples de portraits en grille ;
- equipement de depart.
```

Ces points appartiennent a CC7.2+.

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
CharacterCreationWizard Refreshed Widget=... Step=4 StepName=Resume
```

Le log est en `Verbose`. Si vous ne le voyez pas, c'est normal selon le niveau de log actif.

---

## 11. Critere de validation CC7.1

CC7.1 est valide lorsque :

```text
- le projet recompile ;
- WBP_CharacterCreationWizard peut etre cree avec le parent RPGCharacterCreationWizardWidget ;
- CharacterCreationWidgetClass peut pointer vers WBP_CharacterCreationWizard ;
- une nouvelle partie affiche le wizard ;
- Precedent/Suivant naviguent entre les cinq etapes ;
- le bouton final apparait uniquement au resume ;
- valider cree encore le personnage et ferme le modal comme avant.
```

Statut attendu apres validation UE5 :

```text
CC7.1 valide - shell wizard fonctionnel, logique RPG inchangee.
```

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
| Identité | Saisir le nom, le genre et choisir le portrait |
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

## 6. Structure UMG complète

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

## 7. Précision importante sur les réglages UE5

Dans UE5, certains réglages ne sont pas des propriétés du widget lui-même, mais du **slot du parent**.

Exemples :

```text
- Un widget placé dans un VerticalBox possède un Vertical Box Slot.
- Un widget placé dans un HorizontalBox possède un Horizontal Box Slot.
- Un widget placé dans un CanvasPanel possède un Canvas Panel Slot.
```

Donc, dans ce document :

```text
"Slot du parent : Size = Fill"
```

signifie : régler le **slot dans le panneau parent**, pas chercher une propriété `Fill` directement sur le widget.

À ne pas confondre :

```text
Image_Portrait n'a pas de propriété Fill.
HorizontalBox_RaceLayout n'a pas de propriété Fill.
```

Pour contrôler la taille d'une image, utiliser plutôt :

```text
SizeBox autour de l'image
-> Width Override
-> Height Override
```

Puis régler l'image elle-même avec :

```text
Brush Draw As = Image
Brush Image = texture souhaitée
```

---

## 8. Réglages globaux de layout

| Widget | Type conseillé | Réglage UE5 exact | Padding | Couleur / apparence | Is Variable |
|---|---|---|---|---|---|
| `CanvasPanel_Root` | Canvas Panel | Anchors plein écran, offsets 0 | 0 | Transparent | Non |
| `Border_ModalDim` | Border | Canvas Panel Slot : anchors plein écran, offsets 0 | 0 | Noir, alpha 0.75 à 0.85 | Non |
| `SizeBox_Wizard` | Size Box | Canvas/Border content centré selon parent | 0 | Aucun brush | Non |
| `Border_WizardBackground` | Border | Contenu du `SizeBox_Wizard` | 32 | Noir charbon / brun très foncé, alpha 0.96 à 1.0 | Non |
| `VerticalBox_Wizard` | Vertical Box | Contenu du `Border_WizardBackground` | 0 | Aucun brush | Non |
| `HorizontalBox_Header` | Horizontal Box | Vertical Box Slot : Auto, hauteur contrôlée par contenu | 0 | Aucun brush | Non |
| `Border_StepFrame` | Border | Vertical Box Slot : Size = Fill | 18 | Fond légèrement plus clair que le panneau | Non |
| `WidgetSwitcher_Steps` | Widget Switcher | Contenu du `Border_StepFrame` | 0 | Aucun brush | Oui |
| `Text_ValidationMessage` | Text Block | Vertical Box Slot : Auto | 8 / 8 / 8 / 0 | Rouge doux si erreur, blanc cassé sinon | Oui |
| `HorizontalBox_Footer` | Horizontal Box | Vertical Box Slot : Auto | Top 16 | Aucun brush | Non |

Réglages conseillés pour `SizeBox_Wizard` :

```text
Width Override  = 1180
Height Override = 760
```

Si l'écran est trop petit :

```text
Width Override  = 1040
Height Override = 700
```

Réglages conseillés pour les couleurs :

| Usage | Couleur conseillée |
|---|---|
| Fond modal | Noir, alpha 0.80 |
| Fond principal | Noir charbon, légèrement brun |
| Cadre d'étape | Gris très foncé, alpha 1.0 |
| Texte principal | Blanc cassé |
| Texte secondaire | Gris clair |
| Texte désactivé | Gris moyen |
| Erreur | Rouge sombre lisible |
| Accent | Or pâle / bronze discret |

Ne pas utiliser de fond blanc par défaut sur les boutons, ComboBox ou champs texte. Si un widget apparaît comme un bloc blanc, régler son style explicitement.

---

## 9. Widgets obligatoires lus par le C++

Ces widgets doivent porter exactement ces noms.

| Widget | Type UE5 | Rôle | Is Variable |
|---|---|---|---|
| `WidgetSwitcher_Steps` | Widget Switcher | Contient les cinq étapes | Oui |
| `Panel_StepRace` | Border ou Overlay | Panneau étape Race | Oui |
| `Panel_StepClass` | Border ou Overlay | Panneau étape Classe | Oui |
| `Panel_StepAttributes` | Border ou Overlay | Panneau étape Caractéristiques | Oui |
| `Panel_StepIdentity` | Border ou Overlay | Panneau étape Identité | Oui |
| `Panel_StepSummary` | Border ou Overlay | Panneau étape Résumé | Oui |
| `Text_StepTitle` | Text Block | Titre de l'étape courante | Oui |
| `Text_StepCounter` | Text Block | Compteur `1 / 5`, `2 / 5`, etc. | Oui |
| `Button_Previous` | Button | Étape précédente | Oui |
| `Button_Next` | Button | Étape suivante | Oui |
| `Button_Cancel` | Button | Annulation optionnelle | Oui |
| `Button_CreateCharacter` | Button | Validation finale | Oui |

Le C++ masque automatiquement `Button_CreateCharacter` sauf à l'étape `Résumé`. Il masque aussi `Button_Next` à l'étape finale.

---

## 10. Widgets hérités de RPGCharacterCreationWidget

Ces noms existent déjà dans la logique de création actuelle. Ils doivent être présents si vous voulez réutiliser l'affichage et les binds existants.

| Widget | Type UE5 | Étape conseillée | Is Variable | Remarque |
|---|---|---|---|---|
| `EditableText_Name` | Editable Text | Identité | Oui | Champ de nom utilisé pour créer le personnage |
| `ComboBox_Race` | ComboBox String | Race | Oui | Liste des races disponibles |
| `ComboBox_Class` | ComboBox String | Classe | Oui | Liste des classes disponibles |
| `ComboBox_Gender` | ComboBox String | Identité | Oui | Masculin / Féminin |
| `ComboBox_PortraitVariant` | ComboBox String | Identité | Oui | Variante de portrait par race et genre |
| `ComboBox_Portrait` | ComboBox String | Identité | Oui | Ancien fallback portrait |
| `Image_Portrait` | Image | Identité | Oui | Aperçu unique du portrait utilisé par le C++ |
| `Image_ClassIcon` | Image | Classe | Oui | Icône de classe |
| `Text_RaceValue` | Text Block | Race ou résumé | Oui | Nom de la race sélectionnée |
| `Text_ClassValue` | Text Block | Classe ou résumé | Oui | Nom de la classe sélectionnée |
| `Text_RaceDescription` | Text Block | Race | Oui | Description de la race |
| `Text_ClassDescription` | Text Block | Classe | Oui | Description de la classe |
| `Text_PortraitDescription` | Text Block | Identité | Oui | Description du portrait |
| `Text_StrengthValue` | Text Block | Caractéristiques | Oui | Force |
| `Text_DexterityValue` | Text Block | Caractéristiques | Oui | Dextérité |
| `Text_ConstitutionValue` | Text Block | Caractéristiques | Oui | Constitution |
| `Text_IntelligenceValue` | Text Block | Caractéristiques | Oui | Intelligence |
| `Text_WisdomValue` | Text Block | Caractéristiques | Oui | Sagesse |
| `Text_CharismaValue` | Text Block | Caractéristiques | Oui | Charisme |
| `Text_HealthValue` | Text Block | Caractéristiques | Oui | Points de vie |
| `Text_ManaValue` | Text Block | Caractéristiques | Oui | Mana |
| `Text_CarryWeightValue` | Text Block | Caractéristiques | Oui | Charge maximale |
| `Text_ValidationMessage` | Text Block | Sous le switcher | Oui | Message d'erreur ou d'information |

Attention : un même widget UMG ne peut pas être placé physiquement dans deux panneaux différents. Pour le résumé, ne déplacez pas `Text_RaceValue`, `Image_Portrait`, etc. Créez plutôt des widgets dédiés au résumé en CC7.2, par exemple `Text_SummaryRace`, `Image_SummaryPortrait`, etc.

Point corrigé : il ne doit y avoir **qu'un seul** widget nommé `Image_Portrait` dans `WBP_CharacterCreationWizard`.

---

## 11. Style commun des contrôles

### Textes

| Type de texte | Taille | Couleur | Justification |
|---|---:|---|---|
| `Text_StepTitle` | 28 à 32 | Blanc cassé / or pâle | Gauche |
| `Text_StepCounter` | 16 à 18 | Gris clair | Droite |
| Titre de section | 20 à 22 | Blanc cassé | Gauche |
| Libellé | 14 à 16 | Gris clair | Gauche |
| Valeur importante | 18 à 22 | Blanc cassé | Gauche |
| Description | 14 à 16 | Gris clair | Wrap activé |
| Message d'erreur | 14 à 16 | Rouge doux | Gauche |

### Boutons

Taille recommandée :

```text
Min Desired Width = 140
Height            = 42 à 48
```

Padding interne conseillé :

```text
Left   = 16
Top    = 8
Right  = 16
Bottom = 8
```

Texte conseillé :

```text
Button_Cancel          -> Annuler
Button_Previous        -> Précédent
Button_Next            -> Suivant
Button_CreateCharacter -> Créer le personnage
```

Pour `Button_CreateCharacter`, prévoir une largeur plus grande :

```text
Min Desired Width = 220
```

### ComboBox et champ de nom

Taille recommandée :

```text
Placer le champ dans un SizeBox si vous voulez une hauteur fixe.
SizeBox Height Override = 38 à 44
```

Pour occuper la largeur de la colonne :

```text
Dans le slot du parent VerticalBox :
Horizontal Alignment = Fill
```

Style recommandé :

```text
Fond = gris très foncé
Texte = blanc cassé
Padding = 8 / 4 / 8 / 4
```

---

## 12. Détail de Panel_StepRace

Hiérarchie recommandée :

```text
Panel_StepRace
-> HorizontalBox_RaceLayout
   -> VerticalBox_RaceChoices
      -> Text_RaceTitle
      -> ComboBox_Race
      -> Text_RaceValue
      -> Text_RaceDescription
   -> SizeBox_RaceIllustrationPreview
      -> Image_RaceIllustration
```

Important : ne pas utiliser `Image_Portrait` dans cette étape. Le portrait réel utilisé par le C++ est placé dans l'étape Identité. Ici, `Image_RaceIllustration` est seulement décoratif ou informatif.

Réglages conseillés :

| Widget | Réglage UE5 exact | Padding | Is Variable |
|---|---|---|---|
| `Panel_StepRace` | Enfant direct de `WidgetSwitcher_Steps` | 24 si Border | Oui |
| `HorizontalBox_RaceLayout` | Slot dans `Panel_StepRace` : contenu unique | 0 | Non |
| `VerticalBox_RaceChoices` | Horizontal Box Slot : Size = Fill, Padding Right 24 | 0 / 0 / 24 / 0 | Non |
| `Text_RaceTitle` | Vertical Box Slot : Auto | Bottom 12 | Non |
| `ComboBox_Race` | Dans un `SizeBox` optionnel Height 40, ou Vertical Box Slot Auto | Bottom 16 | Oui |
| `Text_RaceValue` | Vertical Box Slot : Auto | Bottom 8 | Oui |
| `Text_RaceDescription` | Vertical Box Slot : Auto ou Fill selon place disponible | 0 | Oui |
| `SizeBox_RaceIllustrationPreview` | Horizontal Box Slot : Auto, Width 280, Height 420 | 0 | Non |
| `Image_RaceIllustration` | Placée dans le SizeBox, Brush Draw As = Image | 0 | Non |

Si l'illustration est trop grande, modifier le `SizeBox_RaceIllustrationPreview`, pas l'image directement.

---

## 13. Détail de Panel_StepClass

Hiérarchie recommandée :

```text
Panel_StepClass
-> HorizontalBox_ClassLayout
   -> VerticalBox_ClassChoices
      -> Text_ClassTitle
      -> ComboBox_Class
      -> Text_ClassValue
      -> Text_ClassDescription
   -> SizeBox_ClassIconPreview
      -> Image_ClassIcon
```

Réglages conseillés :

| Widget | Réglage UE5 exact | Padding | Is Variable |
|---|---|---|---|
| `Panel_StepClass` | Enfant direct de `WidgetSwitcher_Steps` | 24 si Border | Oui |
| `HorizontalBox_ClassLayout` | Slot dans `Panel_StepClass` : contenu unique | 0 | Non |
| `VerticalBox_ClassChoices` | Horizontal Box Slot : Size = Fill, Padding Right 24 | 0 / 0 / 24 / 0 | Non |
| `ComboBox_Class` | Dans un `SizeBox` optionnel Height 40, ou Vertical Box Slot Auto | Bottom 16 | Oui |
| `Text_ClassValue` | Vertical Box Slot : Auto | Bottom 8 | Oui |
| `Text_ClassDescription` | Vertical Box Slot : Auto ou Fill selon place disponible | 0 | Oui |
| `SizeBox_ClassIconPreview` | Horizontal Box Slot : Auto, Width 180, Height 180 | 0 | Non |
| `Image_ClassIcon` | Placée dans le SizeBox, Brush Draw As = Image | 0 | Oui |

L'icône de classe doit rester lisible et ne pas être étirée. Prévoir un fond sombre derrière l'image si l'icône a un canal alpha.

---

## 14. Détail de Panel_StepAttributes

Hiérarchie recommandée :

```text
Panel_StepAttributes
-> VerticalBox_AttributesLayout
   -> Text_AttributesTitle
   -> UniformGridPanel_Attributes
      -> Border_AttributeStrength
      -> Border_AttributeDexterity
      -> Border_AttributeConstitution
      -> Border_AttributeIntelligence
      -> Border_AttributeWisdom
      -> Border_AttributeCharisma
   -> Border_DerivedStatsFrame
      -> HorizontalBox_DerivedStats
         -> Text_HealthValue
         -> Text_ManaValue
         -> Text_CarryWeightValue
```

Réglages conseillés :

| Widget | Réglage UE5 exact | Padding | Is Variable |
|---|---|---|---|
| `Panel_StepAttributes` | Enfant direct de `WidgetSwitcher_Steps` | 24 si Border | Oui |
| `Text_AttributesTitle` | Vertical Box Slot : Auto | Bottom 16 | Non |
| `UniformGridPanel_Attributes` | Vertical Box Slot : Auto ou Size = Fill | Bottom 24 | Non |
| `Text_StrengthValue` | Text Block | 8 | Oui |
| `Text_DexterityValue` | Text Block | 8 | Oui |
| `Text_ConstitutionValue` | Text Block | 8 | Oui |
| `Text_IntelligenceValue` | Text Block | 8 | Oui |
| `Text_WisdomValue` | Text Block | 8 | Oui |
| `Text_CharismaValue` | Text Block | 8 | Oui |
| `Border_DerivedStatsFrame` | Vertical Box Slot : Auto | 16 | Non |
| `Text_HealthValue` | Text Block | 8 | Oui |
| `Text_ManaValue` | Text Block | 8 | Oui |
| `Text_CarryWeightValue` | Text Block | 8 | Oui |

Présentation recommandée pour chaque caractéristique :

```text
Border_AttributeStrength
-> HorizontalBox_AttributeStrength
   -> Text_LabelStrength = Force
   -> Spacer_AttributeStrength
   -> Text_StrengthValue
```

Pour pousser la valeur à droite, utiliser un `Spacer` entre le libellé et la valeur, avec son **Horizontal Box Slot** réglé en `Size = Fill`.

Utiliser une hauteur de ligne d'environ 42 à 48 px. Les valeurs doivent être plus visibles que les libellés.

---

## 15. Détail de Panel_StepIdentity

Hiérarchie recommandée :

```text
Panel_StepIdentity
-> HorizontalBox_IdentityLayout
   -> VerticalBox_IdentityForm
      -> Text_IdentityTitle
      -> Text_NameLabel
      -> EditableText_Name
      -> Text_GenderLabel
      -> ComboBox_Gender
      -> Text_PortraitLabel
      -> ComboBox_PortraitVariant
      -> ComboBox_Portrait
      -> Text_PortraitDescription
   -> SizeBox_IdentityPortraitPreview
      -> Image_Portrait
```

C'est ici, et uniquement ici, que doit être placé le widget nommé `Image_Portrait`.

Réglages conseillés :

| Widget | Réglage UE5 exact | Padding | Is Variable |
|---|---|---|---|
| `Panel_StepIdentity` | Enfant direct de `WidgetSwitcher_Steps` | 24 si Border | Oui |
| `HorizontalBox_IdentityLayout` | Slot dans `Panel_StepIdentity` : contenu unique | 0 | Non |
| `VerticalBox_IdentityForm` | Horizontal Box Slot : Size = Fill, Padding Right 24 | 0 / 0 / 24 / 0 | Non |
| `EditableText_Name` | Dans un `SizeBox` optionnel Height 42, ou Vertical Box Slot Auto | Bottom 16 | Oui |
| `ComboBox_Gender` | Dans un `SizeBox` optionnel Height 40, ou Vertical Box Slot Auto | Bottom 16 | Oui |
| `ComboBox_PortraitVariant` | Dans un `SizeBox` optionnel Height 40, ou Vertical Box Slot Auto | Bottom 12 | Oui |
| `ComboBox_Portrait` | Dans un `SizeBox` optionnel Height 40, ou Vertical Box Slot Auto | Bottom 16 | Oui |
| `Text_PortraitDescription` | Vertical Box Slot : Auto ou Size = Fill selon place disponible | 0 | Oui |
| `SizeBox_IdentityPortraitPreview` | Horizontal Box Slot : Auto, Width 300, Height 450 | 0 | Non |
| `Image_Portrait` | Placée dans le SizeBox, Brush Draw As = Image | 0 | Oui |

`EditableText_Name` doit être assez large pour 24 caractères. Le C++ peut lui donner le focus automatiquement quand le wizard arrive sur l'étape Identité.

---

## 16. Détail de Panel_StepSummary

Pour CC7.1, le résumé peut rester simple. Ne dupliquez pas trop de logique Blueprint.

Hiérarchie recommandée :

```text
Panel_StepSummary
-> VerticalBox_SummaryLayout
   -> Text_SummaryTitle
   -> Text_SummaryInstruction
   -> Border_SummaryFrame
      -> VerticalBox_SummaryValues
         -> Text_SummaryPlaceholder
```

Texte conseillé :

```text
Text_SummaryTitle       = Résumé
Text_SummaryInstruction = Vérifiez vos choix avant de créer le personnage.
Text_SummaryPlaceholder = Le résumé détaillé sera enrichi en CC7.2. Les valeurs actives sont déjà visibles dans les étapes précédentes.
```

Réglages conseillés :

| Widget | Réglage UE5 exact | Padding | Is Variable |
|---|---|---|---|
| `Panel_StepSummary` | Enfant direct de `WidgetSwitcher_Steps` | 24 si Border | Oui |
| `Text_SummaryTitle` | Vertical Box Slot : Auto | Bottom 12 | Non |
| `Text_SummaryInstruction` | Vertical Box Slot : Auto | Bottom 20 | Non |
| `Border_SummaryFrame` | Vertical Box Slot : Size = Fill | 18 | Non |
| `Text_SummaryPlaceholder` | Text Block | 0 | Non |

Le bouton `Button_CreateCharacter` apparaît seulement sur cette étape. Il doit être visuellement plus fort que `Button_Previous` et `Button_Next`.

---

## 17. Réglages Class Defaults du wizard

Dans `WBP_CharacterCreationWizard` :

```text
Initial Wizard Step = Race
Allow Cancel = false
Focus Name Input On Identity Step = true
```

Pour un lancement via `Nouvelle partie`, `Allow Cancel` doit rester `false`. Le joueur ne doit pas fermer la création et se promener sans personnage valide.

---

## 18. Comportement attendu

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

## 19. Contrôle rapide avant test PIE

Avant de lancer PIE, vérifier :

```text
1. Parent Class = RPGCharacterCreationWizardWidget.
2. Tous les widgets obligatoires existent avec le nom exact.
3. Is Variable = true pour tous les widgets lus par le C++.
4. Les cinq panels sont enfants directs de WidgetSwitcher_Steps.
5. Button_CreateCharacter existe, même s'il est masqué au départ.
6. Text_ValidationMessage existe sous le WidgetSwitcher.
7. Character Creation Widget Class du pawn pointe vers WBP_CharacterCreationWizard.
8. Les DataAssets de races, classes, portraits et icônes sont encore renseignés dans les Class Defaults du widget.
9. Il n'existe qu'un seul widget nommé Image_Portrait dans toute la hiérarchie.
```

Erreur typique : placer `Panel_StepRace` dans un `Canvas Panel` intermédiaire au lieu d'en faire un enfant direct du `WidgetSwitcher_Steps`. Dans ce cas, le C++ peut ne pas sélectionner le bon panneau.

Autre erreur typique : chercher une propriété `Fill` sur une `Image` ou une `HorizontalBox`. Le `Fill` se règle sur le **slot du parent**, par exemple le `Horizontal Box Slot` ou le `Vertical Box Slot`.

---

## 20. Logs utiles

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

## 21. Critère de validation CC7.1

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

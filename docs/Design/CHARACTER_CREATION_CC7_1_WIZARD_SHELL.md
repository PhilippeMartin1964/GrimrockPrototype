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

Les cinq widgets `Panel_StepRace`, `Panel_StepClass`, `Panel_StepAttributes`, `Panel_StepIdentity` et `Panel_StepSummary` doivent être des enfants directs de `WidgetSwitcher_Steps`.

Type recommandé pour ces cinq panneaux :

```text
Border ou Overlay
```

Éviter `Canvas Panel` pour ces cinq panneaux, sauf si vous avez vraiment besoin d'un placement libre. Un `Border` avec un `VerticalBox` ou un `Overlay` interne est plus propre, plus stable et plus facile à redimensionner.

---

## 7. Réglages globaux de layout

| Widget | Type conseillé | Taille / ancrage | Padding | Couleur / apparence | Is Variable |
|---|---|---|---|---|---|
| `CanvasPanel_Root` | Canvas Panel | Plein écran | 0 | Transparent | Non |
| `Border_ModalDim` | Border | Anchors plein écran, offsets 0 | 0 | Noir, alpha 0.75 à 0.85 | Non |
| `SizeBox_Wizard` | Size Box | Alignement horizontal et vertical centré | 0 | Aucun brush | Non |
| `Border_WizardBackground` | Border | Contenu du SizeBox | 32 | Noir charbon / brun très foncé, alpha 0.96 à 1.0 | Non |
| `VerticalBox_Wizard` | Vertical Box | Fill | 0 | Aucun brush | Non |
| `HorizontalBox_Header` | Horizontal Box | Hauteur souhaitée 64 à 72 | 0 | Aucun brush | Non |
| `Border_StepFrame` | Border | Fill dans le VerticalBox | 18 | Fond légèrement plus clair que le panneau | Non |
| `WidgetSwitcher_Steps` | Widget Switcher | Fill | 0 | Aucun brush | Oui |
| `Text_ValidationMessage` | Text Block | Auto height | 8 / 8 / 8 / 0 | Rouge doux si erreur, blanc cassé sinon | Oui |
| `HorizontalBox_Footer` | Horizontal Box | Hauteur souhaitée 56 à 64 | Top 16 | Aucun brush | Non |

Réglages conseillés pour `SizeBox_Wizard` :

```text
Width Override  = 1180
Height Override = 760
```

Si l'écran est trop petit, vous pouvez descendre à :

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

## 8. Widgets obligatoires lus par le C++

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

## 9. Widgets hérités de RPGCharacterCreationWidget

Ces noms existent déjà dans la logique de création actuelle. Ils doivent être présents si vous voulez réutiliser l'affichage et les binds existants.

| Widget | Type UE5 | Étape conseillée | Is Variable | Remarque |
|---|---|---|---|---|
| `EditableText_Name` | Editable Text | Identité | Oui | Champ de nom utilisé pour créer le personnage |
| `ComboBox_Race` | ComboBox String | Race | Oui | Liste des races disponibles |
| `ComboBox_Class` | ComboBox String | Classe | Oui | Liste des classes disponibles |
| `ComboBox_Gender` | ComboBox String | Identité | Oui | Masculin / Féminin |
| `ComboBox_PortraitVariant` | ComboBox String | Identité | Oui | Variante de portrait par race et genre |
| `ComboBox_Portrait` | ComboBox String | Identité | Oui | Ancien fallback portrait |
| `Image_Portrait` | Image | Identité ou résumé | Oui | Aperçu du portrait |
| `Image_ClassIcon` | Image | Classe ou résumé | Oui | Icône de classe |
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

---

## 10. Style commun des contrôles

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
Height = 38 à 44
Width  = Fill dans sa colonne
```

Style recommandé :

```text
Fond = gris très foncé
Texte = blanc cassé
Padding = 8 / 4 / 8 / 4
```

---

## 11. Détail de Panel_StepRace

Hiérarchie recommandée :

```text
Panel_StepRace
-> HorizontalBox_RaceLayout
   -> VerticalBox_RaceChoices
      -> Text_RaceTitle
      -> ComboBox_Race
      -> Text_RaceValue
      -> Text_RaceDescription
   -> SizeBox_RacePortraitPreview
      -> Image_Portrait
```

Réglages conseillés :

| Widget | Taille / slot | Padding | Is Variable |
|---|---|---|---|
| `Panel_StepRace` | Fill | 24 | Oui |
| `HorizontalBox_RaceLayout` | Fill | 0 | Non |
| `VerticalBox_RaceChoices` | Fill, environ 65% largeur | 0 / 0 / 24 / 0 | Non |
| `Text_RaceTitle` | Auto | Bottom 12 | Non |
| `ComboBox_Race` | Width Fill, Height 40 | Bottom 16 | Oui |
| `Text_RaceValue` | Auto | Bottom 8 | Oui |
| `Text_RaceDescription` | Fill ou Auto | 0 | Oui |
| `SizeBox_RacePortraitPreview` | Width 280, Height 420 | 0 | Non |
| `Image_Portrait` | Fill | 0 | Oui |

Si le portrait est trop grand, utiliser :

```text
Image_Portrait Brush Draw As = Image
Image_Portrait Desired Size  = 256 x 384 ou 280 x 420
```

---

## 12. Détail de Panel_StepClass

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

| Widget | Taille / slot | Padding | Is Variable |
|---|---|---|---|
| `Panel_StepClass` | Fill | 24 | Oui |
| `VerticalBox_ClassChoices` | Fill, environ 70% largeur | 0 / 0 / 24 / 0 | Non |
| `ComboBox_Class` | Width Fill, Height 40 | Bottom 16 | Oui |
| `Text_ClassValue` | Auto | Bottom 8 | Oui |
| `Text_ClassDescription` | Fill ou Auto | 0 | Oui |
| `SizeBox_ClassIconPreview` | Width 180, Height 180 | 0 | Non |
| `Image_ClassIcon` | Fill | 0 | Oui |

L'icône de classe doit rester lisible et ne pas être étirée. Prévoir un fond sombre derrière l'image si l'icône a un canal alpha.

---

## 13. Détail de Panel_StepAttributes

Hiérarchie recommandée :

```text
Panel_StepAttributes
-> VerticalBox_AttributesLayout
   -> Text_AttributesTitle
   -> UniformGridPanel_Attributes
      -> Attribute row Force
      -> Attribute row Dextérité
      -> Attribute row Constitution
      -> Attribute row Intelligence
      -> Attribute row Sagesse
      -> Attribute row Charisme
   -> Border_DerivedStatsFrame
      -> HorizontalBox_DerivedStats
         -> Text_HealthValue
         -> Text_ManaValue
         -> Text_CarryWeightValue
```

Réglages conseillés :

| Widget | Taille / slot | Padding | Is Variable |
|---|---|---|---|
| `Panel_StepAttributes` | Fill | 24 | Oui |
| `Text_AttributesTitle` | Auto | Bottom 16 | Non |
| `UniformGridPanel_Attributes` | Auto ou Fill | Bottom 24 | Non |
| `Text_StrengthValue` | Auto | 8 | Oui |
| `Text_DexterityValue` | Auto | 8 | Oui |
| `Text_ConstitutionValue` | Auto | 8 | Oui |
| `Text_IntelligenceValue` | Auto | 8 | Oui |
| `Text_WisdomValue` | Auto | 8 | Oui |
| `Text_CharismaValue` | Auto | 8 | Oui |
| `Border_DerivedStatsFrame` | Auto | 16 | Non |
| `Text_HealthValue` | Auto | 8 | Oui |
| `Text_ManaValue` | Auto | 8 | Oui |
| `Text_CarryWeightValue` | Auto | 8 | Oui |

Présentation recommandée pour chaque caractéristique :

```text
Border_AttributeStrength
-> HorizontalBox
   -> Text_LabelStrength = Force
   -> Spacer
   -> Text_StrengthValue
```

Utiliser une hauteur de ligne d'environ 42 à 48 px. Les valeurs doivent être plus visibles que les libellés.

---

## 14. Détail de Panel_StepIdentity

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

Réglages conseillés :

| Widget | Taille / slot | Padding | Is Variable |
|---|---|---|---|
| `Panel_StepIdentity` | Fill | 24 | Oui |
| `VerticalBox_IdentityForm` | Fill, environ 60% largeur | 0 / 0 / 24 / 0 | Non |
| `EditableText_Name` | Width Fill, Height 42 | Bottom 16 | Oui |
| `ComboBox_Gender` | Width Fill, Height 40 | Bottom 16 | Oui |
| `ComboBox_PortraitVariant` | Width Fill, Height 40 | Bottom 12 | Oui |
| `ComboBox_Portrait` | Width Fill, Height 40 | Bottom 16 | Oui |
| `Text_PortraitDescription` | Fill ou Auto | 0 | Oui |
| `SizeBox_IdentityPortraitPreview` | Width 300, Height 450 | 0 | Non |
| `Image_Portrait` | Fill | 0 | Oui |

`EditableText_Name` doit être assez large pour 24 caractères. Le C++ peut lui donner le focus automatiquement quand le wizard arrive sur l'étape Identité.

---

## 15. Détail de Panel_StepSummary

Pour CC7.1, le résumé peut rester simple. Ne dupliquez pas trop de logique Blueprint.

Hiérarchie recommandée minimale :

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

| Widget | Taille / slot | Padding | Is Variable |
|---|---|---|---|
| `Panel_StepSummary` | Fill | 24 | Oui |
| `Text_SummaryTitle` | Auto | Bottom 12 | Non |
| `Text_SummaryInstruction` | Auto | Bottom 20 | Non |
| `Border_SummaryFrame` | Fill | 18 | Non |
| `Text_SummaryPlaceholder` | Auto | 0 | Non |

Le bouton `Button_CreateCharacter` apparaît seulement sur cette étape. Il doit être visuellement plus fort que `Button_Previous` et `Button_Next`.

---

## 16. Réglages Class Defaults du wizard

Dans `WBP_CharacterCreationWizard` :

```text
Initial Wizard Step = Race
Allow Cancel = false
Focus Name Input On Identity Step = true
```

Pour un lancement via `Nouvelle partie`, `Allow Cancel` doit rester `false`. Le joueur ne doit pas fermer la création et se promener sans personnage valide.

---

## 17. Comportement attendu

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

## 18. Contrôle rapide avant test PIE

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
```

Erreur typique : placer `Panel_StepRace` dans un `Canvas Panel` intermédiaire au lieu d'en faire un enfant direct du `WidgetSwitcher_Steps`. Dans ce cas, le C++ peut ne pas sélectionner le bon panneau.

---

## 19. Logs utiles

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

## 20. Critère de validation CC7.1

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

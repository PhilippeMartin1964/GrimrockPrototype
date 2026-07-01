# CC7.4.2 - Refonte de l'écran Identité

## 1. Objectif

L'écran **Identité** correspond à l'étape `4 / 5` du wizard de création de personnage.

Il ne sert pas à choisir la race ou la classe. Ces choix sont déjà faits dans les étapes précédentes.

Son rôle est :

```text
- saisir le nom du personnage ;
- rappeler la race choisie ;
- rappeler la classe choisie ;
- rappeler le sexe choisi à l'étape Race ;
- choisir la variante de portrait compatible avec la race et le sexe ;
- afficher le portrait final ;
- afficher une description lisible du portrait ;
- préparer le résumé final.
```

Depuis le correctif C++ CC7.4.2, les textes importants de cet écran peuvent être alimentés automatiquement par `URPGCharacterCreationWidget::RefreshPreview()`.

---

## 2. Widgets C++ obligatoires

Ces widgets doivent exister avec ces noms exacts :

```text
EditableText_Name
ComboBox_PortraitVariant
Image_Portrait
Text_PortraitDescription
```

Rôle :

```text
EditableText_Name          -> nom final du personnage ;
ComboBox_PortraitVariant   -> variante de portrait issue du PortraitSet ;
Image_Portrait             -> portrait final réellement utilisé ;
Text_PortraitDescription   -> description de la variante sélectionnée.
```

`Image_Portrait` doit rester unique dans tout le wizard.

---

## 3. Textes désormais alimentés par le C++

Les widgets suivants sont optionnels, mais recommandés.

S'ils existent dans `WBP_CharacterCreationWizard`, ils doivent avoir `Is Variable = true`, car le C++ les recherche par `BindWidgetOptional`.

```text
Text_IdentityTitle
Text_IdentityHelp
Text_NameLabel
Text_IdentitySummaryTitle
Text_IdentityRace
Text_IdentityClass
Text_IdentityGender
Text_PortraitVariantLabel
Text_PortraitDescriptionTitle
Text_PortraitCaption
```

Valeurs remplies automatiquement :

```text
Text_IdentityTitle              = Identité du personnage
Text_IdentityHelp               = Donnez un nom à votre personnage et choisissez le portrait qui sera utilisé dans l'interface de jeu.
Text_NameLabel                  = Nom du personnage
Text_IdentitySummaryTitle       = Choix actuels
Text_IdentityRace               = Race : <race sélectionnée>
Text_IdentityClass              = Classe : <classe sélectionnée>
Text_IdentityGender             = Sexe : Masculin ou Sexe : Féminin
Text_PortraitVariantLabel       = Variante de portrait
Text_PortraitDescriptionTitle   = Description du portrait
Text_PortraitCaption            = Portrait final
```

`Text_PortraitDescription` reçoit maintenant aussi un texte de secours :

```text
Aucun portrait disponible pour cette race et ce sexe.
```

ou :

```text
Aucune description disponible pour ce portrait.
```

---

## 4. Point important : le sexe ne doit plus être choisi ici

Depuis CC7.2, le choix du sexe est fait dans l'étape Race avec :

```text
Button_GenderMale
Button_GenderFemale
```

L'écran Identité ne doit donc pas afficher `ComboBox_Gender`.

À faire :

```text
- masquer ou supprimer visuellement ComboBox_Gender de Panel_StepIdentity ;
- afficher uniquement Text_IdentityGender ;
- modifier le sexe en revenant à l'étape Race.
```

Le C++ conserve encore `ComboBox_Gender` comme fallback legacy, mais le nouveau wizard ne doit pas l'utiliser visuellement.

---

## 5. Cible visuelle

La cible est un écran en deux colonnes :

```text
Gauche : formulaire d'identité et choix de portrait
Droite : grand aperçu du personnage / portrait
```

Structure visuelle :

```text
Identité                                      4 / 5

┌──────────────────────────────────────────────────────────────┐
│ Identité du personnage                         Portrait final │
│ Donnez un nom à votre personnage et choisissez son portrait.  │
│                                                              │
│ Nom du personnage                                            │
│ [________________________]                                   │
│                                                              │
│ Choix actuels                                                │
│ Race : Humain                                                │
│ Classe : Rôdeur                                              │
│ Sexe : Masculin                                              │
│                                                              │
│ Variante de portrait                                         │
│ [Humain masculin 01               v]                         │
│                                                              │
│ Description du portrait                                      │
│ Portrait humain masculin de base.                            │
│                                                              │
│                                      [grand portrait à droite]│
└──────────────────────────────────────────────────────────────┘
```

---

## 6. Hiérarchie UMG recommandée

Dans `WBP_CharacterCreationWizard`, aller dans :

```text
WidgetSwitcher_Steps
└── Panel_StepIdentity
```

Créer :

```text
Panel_StepIdentity                                      Border ou Overlay, Is Variable = true
└── Border_IdentityFrame                                Border, Is Variable = false
    └── HorizontalBox_IdentityLayout                    Horizontal Box, Is Variable = false
        ├── VerticalBox_IdentityForm                    Vertical Box, Is Variable = false
        │   ├── Text_IdentityTitle                      Text Block, Is Variable = true
        │   ├── Text_IdentityHelp                       Text Block, Is Variable = true
        │   ├── Spacer_IdentityTopGap                   Spacer, Is Variable = false
        │   ├── Text_NameLabel                          Text Block, Is Variable = true
        │   ├── SizeBox_NameInput                       Size Box, Is Variable = false
        │   │   └── EditableText_Name                   Editable Text, Is Variable = true
        │   ├── Border_IdentitySummaryFrame             Border, Is Variable = false
        │   │   └── VerticalBox_IdentitySummary         Vertical Box, Is Variable = false
        │   │       ├── Text_IdentitySummaryTitle       Text Block, Is Variable = true
        │   │       ├── Text_IdentityRace               Text Block, Is Variable = true
        │   │       ├── Text_IdentityClass              Text Block, Is Variable = true
        │   │       └── Text_IdentityGender             Text Block, Is Variable = true
        │   ├── Text_PortraitVariantLabel               Text Block, Is Variable = true
        │   ├── SizeBox_PortraitVariantCombo            Size Box, Is Variable = false
        │   │   └── ComboBox_PortraitVariant            ComboBox String, Is Variable = true
        │   ├── Text_PortraitDescriptionTitle           Text Block, Is Variable = true
        │   └── Border_PortraitDescriptionFrame         Border, Is Variable = false
        │       └── Text_PortraitDescription            Text Block, Is Variable = true
        └── SizeBox_IdentityPortraitColumn              Size Box, Is Variable = false
            └── Border_IdentityPortraitFrame            Border, Is Variable = false
                └── Overlay_IdentityPortrait            Overlay, Is Variable = false
                    ├── Image_Portrait                  Image, Is Variable = true
                    └── Border_PortraitCaptionOverlay   Border, Is Variable = false
                        └── Text_PortraitCaption        Text Block, Is Variable = true
```

---

## 7. Réglages du panneau principal

`Panel_StepIdentity` :

```text
Type        = Border ou Overlay
Is Variable = true
Padding     = 0 si le frame interne gère le padding
```

`Border_IdentityFrame` :

```text
Type        = Border
Is Variable = false
Padding     = 24
Brush Color = R 0.00 / G 0.00 / B 0.00 / A 0.78
```

`HorizontalBox_IdentityLayout` :

```text
Type = Horizontal Box
Horizontal Alignment = Fill
Vertical Alignment   = Fill
```

---

## 8. Colonne gauche : formulaire

`VerticalBox_IdentityForm` :

```text
Type = Vertical Box
Slot dans HorizontalBox_IdentityLayout : Size = Fill
Padding = 0 ; 0 ; 24 ; 0
```

Largeur visuelle recommandée : environ 60 % de l'espace.

### 8.1. Titre et aide

`Text_IdentityTitle` :

```text
Is Variable = true
Font Size   = 28
Color       = blanc cassé
Justification = Left
```

`Text_IdentityHelp` :

```text
Is Variable = true
Font Size   = 17
Color       = R 0.82 / G 0.82 / B 0.82 / A 1.00
Auto Wrap Text = true
```

### 8.2. Nom du personnage

`Text_NameLabel` :

```text
Is Variable = true
Font Size   = 18
Color       = blanc cassé
```

`SizeBox_NameInput` :

```text
Height Override = 44
```

`EditableText_Name` :

```text
Is Variable = true
Hint Text   = Entrez un nom
Text        = vide
Font Size   = 20
Select All Text When Focused = true
Revert Text On Escape        = true
```

Le C++ accepte un nom de 1 à 24 caractères.

---

## 9. Bloc de rappel Race / Classe / Sexe

Ce bloc remplace les `Text Block` anonymes visibles dans l'ancien écran.

Hiérarchie :

```text
Border_IdentitySummaryFrame
└── VerticalBox_IdentitySummary
    ├── Text_IdentitySummaryTitle
    ├── Text_IdentityRace
    ├── Text_IdentityClass
    └── Text_IdentityGender
```

Réglages :

```text
Border_IdentitySummaryFrame Padding = 12
Brush Color = R 0.06 / G 0.05 / B 0.04 / A 0.85
Slot Padding = 0 ; 20 ; 0 ; 20
```

Tous les textes de ce bloc doivent être `Is Variable = true`.

Le C++ les remplit automatiquement.

---

## 10. Variante de portrait

`Text_PortraitVariantLabel` :

```text
Is Variable = true
Font Size   = 18
Color       = blanc cassé
```

`SizeBox_PortraitVariantCombo` :

```text
Height Override = 42
```

`ComboBox_PortraitVariant` :

```text
Is Variable = true
```

Ce ComboBox est alimenté par le C++ à partir de :

```text
AvailablePortraitSets
+ RaceId sélectionné
+ Sexe sélectionné
```

Ne pas recréer `ComboBox_Portrait`.

---

## 11. Description du portrait

`Text_PortraitDescriptionTitle` :

```text
Is Variable = true
Font Size   = 18
Color       = blanc cassé
```

`Border_PortraitDescriptionFrame` :

```text
Padding     = 12
Brush Color = R 0.03 / G 0.03 / B 0.03 / A 0.85
```

`Text_PortraitDescription` :

```text
Is Variable = true
Font Size   = 18 à 20
Color       = blanc cassé
Auto Wrap Text = true
```

---

## 12. Colonne droite : portrait final

`SizeBox_IdentityPortraitColumn` :

```text
Width Override = 380
```

`Border_IdentityPortraitFrame` :

```text
Padding     = 12
Brush Color = R 0.02 / G 0.02 / B 0.02 / A 0.90
```

`Image_Portrait` :

```text
Is Variable = true
Brush Draw As = Image
Stretch       = Scale To Fit
Color and Opacity = blanc
```

Zone recommandée pour les portraits de personnage debout ou en buste :

```text
Width  = 360 à 420
Height = 500 à 560
```

`Text_PortraitCaption` :

```text
Is Variable = true
Font Size   = 16
Color       = gris clair
Justification = Center
```

---

## 13. Ce qu'il faut retirer de l'écran actuel

Supprimer ou remplacer :

```text
- les Text Block génériques ;
- les labels non explicites ;
- ComboBox_Gender si elle est visible ;
- les grands espaces inutiles à gauche ;
- le portrait sans cadre ;
- les fonds blancs par défaut.
```

Conserver impérativement :

```text
EditableText_Name
ComboBox_PortraitVariant
Image_Portrait
Text_PortraitDescription
```

---

## 14. Procédure UE5 recommandée

```text
1. Recompiler le projet C++.
2. Ouvrir WBP_CharacterCreationWizard.
3. Aller dans WidgetSwitcher_Steps > Panel_StepIdentity.
4. Supprimer les Text Block génériques.
5. Supprimer la ComboBox de sexe visible.
6. Créer Border_IdentityFrame.
7. Créer HorizontalBox_IdentityLayout.
8. Créer VerticalBox_IdentityForm à gauche.
9. Ajouter les textes optionnels nommés exactement comme dans ce document.
10. Mettre Is Variable = true sur ces textes optionnels.
11. Placer EditableText_Name dans SizeBox_NameInput.
12. Ajouter le bloc Race / Classe / Sexe.
13. Placer ComboBox_PortraitVariant dans SizeBox_PortraitVariantCombo.
14. Placer Text_PortraitDescription dans Border_PortraitDescriptionFrame.
15. Créer la colonne droite avec Image_Portrait.
16. Ajouter Text_PortraitCaption.
17. Compiler WBP_CharacterCreationWizard.
18. Sauvegarder.
19. Lancer PIE.
20. Vérifier que tous les textes sont remplis automatiquement.
```

---

## 15. Critères de validation

L'écran Identité est validé lorsque :

```text
- aucun Text Block générique n'est visible ;
- le titre affiche Identité du personnage ;
- le champ de nom est clair et utilisable ;
- Race / Classe / Sexe sont rappelés automatiquement ;
- la ComboBox de portrait est lisible ;
- la description du portrait est affichée dans un cadre propre ;
- le portrait est bien cadré ;
- le sexe n'est pas redemandé ici par ComboBox ;
- la navigation Précédent / Suivant fonctionne ;
- le résumé final reçoit bien les choix effectués.
```

---

## 16. Note pour une future extraction en sous-widget

Comme pour l'étape Caractéristiques, l'étape Identité pourra être extraite plus tard dans un sous-widget :

```text
WBP_CCStep_Identity
Parent Class = RPGCharacterCreationIdentityStepWidget
```

Mais cette extraction n'est pas nécessaire immédiatement.

Priorité actuelle : nettoyer l'UMG dans `Panel_StepIdentity` et exploiter les bindings C++ ajoutés par CC7.4.2.

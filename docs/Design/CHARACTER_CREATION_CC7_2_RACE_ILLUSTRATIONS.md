# CC7.2 - Illustration de race dynamique selon race et sexe

## 1. Objectif

CC7.2 ajoute l'illustration dynamique de la race dans le wizard de création de personnage.

Le widget ne doit plus afficher une image fixe dans `Image_RaceIllustration`. L'image affichée dépend maintenant de deux informations :

```text
RaceDefinition.RaceId
SelectedPortraitGender
```

Le choix du sexe se fait dans l'étape Race avec deux boutons :

```text
Button_GenderMale
Button_GenderFemale
```

La `ComboBox_Gender` reste supportée par le C++ comme fallback optionnel, mais elle ne doit plus être utilisée dans `WBP_CharacterCreationWizard`.

---

## 2. Ajouts C++

Structure ajoutée :

```text
FRPGRaceIllustrationOption
```

Champs :

```text
RaceId
Gender
Illustration
DisplayName
```

Propriété ajoutée dans `RPGCharacterCreationWidget` :

```text
AvailableRaceIllustrations
```

Widgets optionnels ajoutés :

```text
Image_RaceIllustration
Button_GenderMale
Button_GenderFemale
Image_GenderMale
Image_GenderFemale
```

Icônes optionnelles ajoutées pour les deux boutons :

```text
GenderMaleButtonIcon
GenderFemaleButtonIcon
```

---

## 3. Structure UMG recommandée pour Panel_StepRace

Dans `WBP_CharacterCreationWizard`, modifier l'étape Race ainsi :

```text
Panel_StepRace
-> HorizontalBox_RaceLayout
   -> VerticalBox_RaceChoices
      -> Text_RaceTitle
      -> ComboBox_Race
      -> Text_GenderTitle
      -> HorizontalBox_GenderButtons
         -> Button_GenderMale
            -> Image_GenderMale
         -> Button_GenderFemale
            -> Image_GenderFemale
      -> Text_RaceValue
      -> Text_RaceDescription
   -> SizeBox_RaceIllustrationPreview
      -> Image_RaceIllustration
```

Supprimer de l'étape Identité :

```text
ComboBox_Gender
```

L'étape Identité doit garder :

```text
EditableText_Name
ComboBox_PortraitVariant
Image_Portrait
Text_PortraitDescription
```

---

## 4. Réglages des widgets

### Image_RaceIllustration

```text
Type                = Image
Is Variable         = true
Brush Draw As       = Image
Visibility initiale = Visible ou Collapsed
```

Placement conseillé :

```text
SizeBox_RaceIllustrationPreview
Width Override  = 300 à 360
Height Override = 450 à 520
```

Le C++ remplace automatiquement la texture de `Image_RaceIllustration` selon la race et le sexe sélectionnés.

### Button_GenderMale

```text
Type        = Button
Is Variable = true
Largeur     = 72 à 96
Hauteur     = 72 à 96
```

Contenu provisoire possible :

```text
Image_GenderMale
```

Tant que l'icône définitive n'est pas dessinée, vous pouvez mettre une image temporaire ou un `TextBlock` avec `M`.

### Button_GenderFemale

```text
Type        = Button
Is Variable = true
Largeur     = 72 à 96
Hauteur     = 72 à 96
```

Contenu provisoire possible :

```text
Image_GenderFemale
```

Tant que l'icône définitive n'est pas dessinée, vous pouvez mettre une image temporaire ou un `TextBlock` avec `F`.

### Image_GenderMale et Image_GenderFemale

```text
Type        = Image
Is Variable = true
```

Ces deux images sont optionnelles. Si vous renseignez `GenderMaleButtonIcon` et `GenderFemaleButtonIcon` dans les Class Defaults, le C++ les applique automatiquement.

---

## 5. Class Defaults à renseigner

Dans `WBP_CharacterCreationWizard`, section :

```text
RPG | Character Creation | Choices
```

Renseigner :

```text
AvailableRaceIllustrations
```

Ajouter 12 entrées :

```text
[0] RaceId = Human    Gender = Male    Illustration = T_RaceHuman_Male
[1] RaceId = Human    Gender = Female  Illustration = T_RaceHuman_Female
[2] RaceId = Dwarf    Gender = Male    Illustration = T_RaceDwarf_Male
[3] RaceId = Dwarf    Gender = Female  Illustration = T_RaceDwarf_Female
[4] RaceId = Elf      Gender = Male    Illustration = T_RaceElf_Male
[5] RaceId = Elf      Gender = Female  Illustration = T_RaceElf_Female
[6] RaceId = Halfling Gender = Male    Illustration = T_RaceHalfling_Male
[7] RaceId = Halfling Gender = Female  Illustration = T_RaceHalfling_Female
[8] RaceId = Gnome    Gender = Male    Illustration = T_RaceGnome_Male
[9] RaceId = Gnome    Gender = Female  Illustration = T_RaceGnome_Female
[10] RaceId = HalfOrc Gender = Male    Illustration = T_RaceHalfOrc_Male
[11] RaceId = HalfOrc Gender = Female  Illustration = T_RaceHalfOrc_Female
```

Important : les `RaceId` doivent correspondre exactement aux `RaceId` des DataAssets de race.

Vérifier donc les valeurs réelles dans :

```text
DA_Race_Human
DA_Race_Dwarf
DA_Race_Elf
DA_Race_Halfling
DA_Race_Gnome
DA_Race_HalfOrc
```

Si vos identifiants sont en français, par exemple `Humain` au lieu de `Human`, utiliser exactement l'identifiant du DataAsset.

---

## 6. Icônes des boutons masculin / féminin

Pour l'instant, les icônes peuvent rester provisoires.

Quand les images définitives seront prêtes, renseigner dans les Class Defaults :

```text
GenderMaleButtonIcon
GenderFemaleButtonIcon
```

Ces textures servent uniquement à décorer les boutons de sexe. Elles ne remplacent pas les illustrations de race.

Réglages texture conseillés :

```text
Compression Settings = UserInterface2D (RGBA)
Texture Group        = UI
Mip Gen Settings     = NoMipmaps
sRGB                 = true
```

---

## 7. Comportement attendu

Dans l'étape Race :

```text
1. Le joueur choisit une race avec ComboBox_Race.
2. Le joueur choisit Masculin ou Féminin avec Button_GenderMale / Button_GenderFemale.
3. Image_RaceIllustration affiche l'image correspondant à RaceId + Gender.
4. Le choix du sexe met aussi à jour les portraits disponibles dans ComboBox_PortraitVariant.
5. L'étape Identité réutilise le même sexe déjà sélectionné.
```

Le bouton sélectionné reçoit automatiquement une couleur de fond plus accentuée.

---

## 8. Checklist UE5

Après recompilation :

```text
1. Ouvrir WBP_CharacterCreationWizard.
2. Dans Panel_StepRace, créer Button_GenderMale et Button_GenderFemale.
3. Créer Image_GenderMale et Image_GenderFemale dans les boutons.
4. Déplacer ou supprimer ComboBox_Gender de l'étape Identité.
5. Créer Image_RaceIllustration dans SizeBox_RaceIllustrationPreview.
6. Cocher Is Variable pour les widgets lus par le C++.
7. Renseigner AvailableRaceIllustrations avec les 12 images race + sexe.
8. Compiler le widget.
9. Sauvegarder.
10. Tester Nouvelle partie.
```

---

## 9. Critère de validation CC7.2

CC7.2 est validé lorsque :

```text
- le projet recompile ;
- les deux boutons Masculin / Féminin répondent au clic ;
- le bouton actif est visuellement distingué ;
- Image_RaceIllustration change quand la race change ;
- Image_RaceIllustration change quand le sexe change ;
- les portraits de l'étape Identité restent cohérents avec le sexe sélectionné ;
- la création finale du personnage fonctionne encore.
```
